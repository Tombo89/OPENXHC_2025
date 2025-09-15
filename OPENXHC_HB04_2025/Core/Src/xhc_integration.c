/*
 * xhc_integration.c
 *
 *  Created on: Sep 15, 2025
 *      Author: Thomas Weckmann
 *      Integration der XHC USB-Funktionalität
 */


#include "xhc_integration.h"
#include "ui.h"
#include "usbd_customhid.h"
#include <string.h>
#include <stdio.h>

/* External USB Device Handle */
extern USBD_HandleTypeDef hUsbDeviceFS;

/* Global Data */
struct whb04_out_data xhc_output_report = {0};
struct whb0x_in_data xhc_input_report = {.id = 0x04};
uint8_t xhc_day = 0;

/* USB Reception State Machine */
#define TMP_BUFF_SIZE 42
#define CHUNK_SIZE    7
static uint8_t tmp_buff[TMP_BUFF_SIZE];
static int offset = 0;
static uint8_t magic_found = 0;


/* Timing and State Tracking */
static uint32_t last_display_update = 0;
static uint32_t last_usb_send = 0;
static struct {
    uint32_t wc_pos_cache[3];    // Cache für WC X,Y,Z
    uint32_t mc_pos_cache[3];    // Cache für MC X,Y,Z
    uint8_t  first_update;
} position_cache = {0};

/**
 * @brief Initialisierung der XHC Integration
 */
void xhc_init(void) {
    memset(&xhc_output_report, 0, sizeof(xhc_output_report));
    memset(&xhc_input_report, 0, sizeof(xhc_input_report));
    xhc_input_report.id = 0x04;
    position_cache.first_update = 1;

    // UI bereits initialisiert - nur Reset der Position-Caches
    for (int i = 0; i < 3; i++) {
        position_cache.wc_pos_cache[i] = 0xFFFFFFFF;
        position_cache.mc_pos_cache[i] = 0xFFFFFFFF;
    }
}

/**
 * @brief USB-Datenempfang (wird von USB-Stack aufgerufen)
 * @param data 7-Byte Chunks vom Host
 */
void xhc_receive_data(uint8_t *data) {
    /* Prüfe auf Magic-Wert am Anfang */
    if (*(uint16_t*)data == WHBxx_MAGIC) {
        offset = 0;
        magic_found = 1;
    }

    if (!magic_found || (offset + CHUNK_SIZE) > TMP_BUFF_SIZE) {
        return;
    }

    /* Kopiere Chunk in temporären Puffer */
    memcpy(&tmp_buff[offset], data, CHUNK_SIZE);
    offset += CHUNK_SIZE;

    /* Prüfe ob komplettes Paket empfangen */
    if (offset >= DEV_WHB04) {
        magic_found = 0;

        /* Kopiere empfangene Daten */
        xhc_output_report = *((struct whb04_out_data*)tmp_buff);
        xhc_day = xhc_output_report.day;

        /* Verarbeite die Daten */
        xhc_process_received_data();
    }
}

/**
 * @brief Verarbeitung empfangener Daten mit optimiertem Display-Update
 */
void xhc_process_received_data(void) {
    uint32_t current_time = HAL_GetTick();
    uint8_t need_update = 0;

    /* Prüfe Änderungen an WC-Positionen */
    for (int i = 0; i < 3; i++) {
        uint32_t current_pos = (xhc_output_report.pos[i].p_int << 16) |
                              (xhc_output_report.pos[i].p_frac & 0x7FFF);

        if (position_cache.wc_pos_cache[i] != current_pos || position_cache.first_update) {
            position_cache.wc_pos_cache[i] = current_pos;
            need_update = 1;
        }
    }

    /* Prüfe Änderungen an MC-Positionen */
    for (int i = 0; i < 3; i++) {
        uint32_t current_pos = (xhc_output_report.pos[i+3].p_int << 16) |
                              (xhc_output_report.pos[i+3].p_frac & 0x7FFF);

        if (position_cache.mc_pos_cache[i] != current_pos || position_cache.first_update) {
            position_cache.mc_pos_cache[i] = current_pos;
            need_update = 1;
        }
    }

    /* Update Display nur bei Änderungen oder nach Timeout */
    if (need_update || (current_time - last_display_update) > 500) {
        // Update WC-Koordinaten mit der optimierten UI-Funktion
        float wc_x = xhc_get_wc_position(0);
        float wc_y = xhc_get_wc_position(1);
        float wc_z = xhc_get_wc_position(2);

        UI_UpdateWC(wc_x, wc_y, wc_z);

        // Update weitere UI-Elemente
        char pos_text[10];
        snprintf(pos_text, sizeof(pos_text), "ON");
        UI_UpdatePosText(pos_text);

        char step_text[10];
        snprintf(step_text, sizeof(step_text), "%d.%03d",
                 xhc_output_report.step_mul / 1000,
                 xhc_output_report.step_mul % 1000);
        UI_UpdateStepText(step_text);

        // Update Balken (Beispiel mit Feedrate/Spindle)
        uint8_t feed_percent = (xhc_output_report.feedrate_ovr > 200) ? 100 :
                               (xhc_output_report.feedrate_ovr / 2);
        uint8_t spin_percent = (xhc_output_report.sspeed_ovr > 200) ? 100 :
                               (xhc_output_report.sspeed_ovr / 2);

        UI_UpdateBarF(feed_percent);
        UI_UpdateBarS(spin_percent);

        last_display_update = current_time;
        position_cache.first_update = 0;
    }
}

/**
 * @brief Sendet Input Report an Host
 */
uint8_t xhc_send_input_report(uint8_t btn1, uint8_t btn2, uint8_t wheel_mode, int8_t wheel_value) {
    uint32_t current_time = HAL_GetTick();

    /* Rate Limiting: mindestens 10ms zwischen Sends */
    if (current_time - last_usb_send < 10) {
        return USBD_BUSY;
    }

    /* Fülle Input Report */
    xhc_input_report.btn_1 = btn1;
    xhc_input_report.btn_2 = btn2;
    xhc_input_report.wheel_mode = wheel_mode;
    xhc_input_report.wheel = wheel_value;
    xhc_input_report.xor_day = xhc_day ^ btn1;

    /* Sende Report */
    uint8_t result = USBD_CUSTOM_HID_SendReport(&hUsbDeviceFS,
                                               (uint8_t*)&xhc_input_report,
                                               sizeof(xhc_input_report));

    if (result == USBD_OK) {
        last_usb_send = current_time;
    }

    return result;
}


/**
 * @brief Hauptschleife für XHC-Integration
 */
void xhc_main_loop(void) {
    static uint32_t last_keepalive = 0;
    uint32_t current_time = HAL_GetTick();

    /* Hier würden normalerweise Inputs gelesen werden:
     * - Button Matrix
     * - Encoder
     * - Rotary Switch
     *
     * Für jetzt: Keepalive alle 100ms
     */
    if (current_time - last_keepalive >= 100) {
        xhc_send_input_report(0, 0, ROTARY_X, 0);
        last_keepalive = current_time;
    }
}

/**
 * @brief Extrahiert WC-Position für gegebene Achse
 */
float xhc_get_wc_position(uint8_t axis) {
    if (axis >= 3) return 0.0f;

    float result = (float)xhc_output_report.pos[axis].p_int;
    uint16_t frac = xhc_output_report.pos[axis].p_frac;
    uint8_t negative = (frac & 0x8000) ? 1 : 0;
    frac &= 0x7FFF;

    result += (float)frac / 10000.0f;

    return negative ? -result : result;
}

/**
 * @brief Extrahiert MC-Position für gegebene Achse
 */
float xhc_get_mc_position(uint8_t axis) {
    if (axis >= 3) return 0.0f;

    float result = (float)xhc_output_report.pos[axis + 3].p_int;
    uint16_t frac = xhc_output_report.pos[axis + 3].p_frac;
    uint8_t negative = (frac & 0x8000) ? 1 : 0;
    frac &= 0x7FFF;

    result += (float)frac / 10000.0f;

    return negative ? -result : result;
}
