/*
 * xhc_integration.h
 *
 *  Created on: Sep 15, 2025
 *      Author: Thomas Weckmann
 */

#ifndef INC_XHC_INTEGRATION_H_
#define INC_XHC_INTEGRATION_H_

#include <stdint.h>
#include "ui.h"

/* XHC Protocol Constants */
#define WHBxx_MAGIC 0xFDFE
#define DEV_WHB04   37  // sizeof(struct whb04_out_data)
#define WHBxx_VID   0x10CE
#define WHB04_PID   0xEB70

/* Rotary Switch Positions */
#define ROTARY_OFF     0x00
#define ROTARY_X       0x11
#define ROTARY_Y       0x12
#define ROTARY_Z       0x13
#define ROTARY_A       0x14
#define ROTARY_SPINDLE 0x15
#define ROTARY_FEED    0x16

#pragma pack(push, 1)

/* Host→Device Data Structure */
struct whb04_out_data {
    uint16_t magic;      // 0xFDFE
    uint8_t  day;        // XOR-Schlüssel

    /* 6 Achsen: [0-2]=WC(X,Y,Z), [3-5]=MC(X,Y,Z) */
    struct {
        uint16_t p_int;   // Integer-Teil
        uint16_t p_frac;  // Fraktionaler Teil (16-bit, MSB=Vorzeichen)
    } pos[6];

    uint16_t feedrate_ovr;
    uint16_t sspeed_ovr;
    uint16_t feedrate;
    uint16_t sspeed;
    uint8_t  step_mul;
    uint8_t  state;
};

/* Device→Host Data Structure */
struct whb0x_in_data {
    uint8_t  id;         // Report ID (0x04)
    uint8_t  btn_1;      // Button matrix 1
    uint8_t  btn_2;      // Button matrix 2
    uint8_t  wheel_mode; // Rotary position
    int8_t   wheel;      // Encoder value
    uint8_t  xor_day;    // day XOR btn_1
};

#pragma pack(pop)

/* Global Variables */
extern struct whb04_out_data xhc_output_report;
extern struct whb0x_in_data xhc_input_report;
extern uint8_t xhc_day;

/* Core Functions */
void xhc_init(void);
void xhc_main_loop(void);

/* USB Communication */
void xhc_receive_data(uint8_t *data);
uint8_t xhc_send_input_report(uint8_t btn1, uint8_t btn2, uint8_t wheel_mode, int8_t wheel_value);

/* Data Processing */
void xhc_process_received_data(void);

/* Position Helpers */
float xhc_get_wc_position(uint8_t axis);  // axis: 0=X, 1=Y, 2=Z
float xhc_get_mc_position(uint8_t axis);  // axis: 0=X, 1=Y, 2=Z

/* Status Helpers */
static inline uint8_t xhc_get_day(void) { return xhc_day; }
static inline uint16_t xhc_get_feedrate(void) { return xhc_output_report.feedrate; }
static inline uint16_t xhc_get_spindle_speed(void) { return xhc_output_report.sspeed; }

#endif /* XHC_INTEGRATION_H */


