/*
 * ui.c
 *
 *  Created on: Sep 14, 2025
 *      Author: Thomas Weckmann
 */


#include "ui.h"
#include <stdio.h>
#include <string.h>

// wähle Fonts, die du im Projekt hast (aus fonts.h)
#define FONT_L  Font_16x26   // große Zahlen
#define FONT_M  Font_9  // Labels
#define FONT_S  Font_7x10   // Statusleiste

// kleine Helfer
static inline void HLine(int x, int y, int w, uint16_t c){
    for (int i=0;i<w;i++) ST7735_DrawPixel(x+i, y, c);
}
static inline void VLine(int x, int y, int h, uint16_t c){
    for (int i=0;i<h;i++) ST7735_DrawPixel(x, y+i, c);
}

static void Print(int x, int y, const char *s, FontDef f, uint16_t col){
    ST7735_WriteString(x, y, s, f, col, UI_BG);
}

// Koordinaten der Zahlenblöcke
static const int VAL_X = 60;             // Start X der Werte (rechts neben "X:")
static const int LABEL_X = UI_MARGIN_L;  // "WC", "MC", "X:", "Y:", "Z:"

static void DrawAxisTriplet(int top_y, const char *hdr){
    // Header (WC / MC)
    Print(LABEL_X, top_y, hdr, FONT_L, UI_FG);

    // Achsen-Labels links
    Print(40, top_y,     "X:", FONT_M, UI_FG);
    Print(40, top_y+14,  "Y:", FONT_M, UI_FG);
    Print(40, top_y+28,  "Z:", FONT_M, UI_FG);

    // Platz für Werte zuerst "leeren"
    ST7735_FillRectangleFast(VAL_X, top_y, ST7735_WIDTH-VAL_X-UI_MARGIN_R, 26, UI_BG);
    ST7735_FillRectangleFast(VAL_X, top_y+14, ST7735_WIDTH-VAL_X-UI_MARGIN_R, 26, UI_BG);
    ST7735_FillRectangleFast(VAL_X, top_y+28, ST7735_WIDTH-VAL_X-UI_MARGIN_R, 26, UI_BG);

    // Dummy: „-1000.0000“
    //Print(VAL_X, top_y,     "0.0000", FONT_M, UI_FG);
    //Print(VAL_X, top_y+14,  "-1000.0000", FONT_M, UI_FG);
    //Print(VAL_X, top_y+28,  "-1000.0000", FONT_M, UI_FG);
}

void UI_DrawStatic(void){
    // Hintergrund & Blauleiste
    ST7735_FillScreen(UI_BG);

    // oberer Block (WC)
    DrawAxisTriplet(UI_TOP_H1_Y, "WC");

    // Trennlinie
    HLine(UI_MARGIN_L, UI_LINE_Y, ST7735_WIDTH-UI_MARGIN_L-UI_MARGIN_R, UI_LINE);

    // mittlerer Block (MC)
    DrawAxisTriplet(UI_TOP_H2_Y, "MC");

    // Untere blaue Fläche
    ST7735_FillRectangleFast(0, UI_BAR_Y, ST7735_WIDTH, UI_BAR_H, UI_BLUE);

    // Texte unten
    Print(4, UI_BAR_Y+3, "POS:",  FONT_S, ST7735_WHITE);
    Print(4+40, UI_BAR_Y+3, "OFF", FONT_S, ST7735_WHITE);

    Print(ST7735_WIDTH/2+4, UI_BAR_Y+3, "STEP:", FONT_S, ST7735_WHITE);
    Print(ST7735_WIDTH/2+4+50, UI_BAR_Y+3, "0.001", FONT_S, ST7735_WHITE);

    // Boxen S / F
    // S-Box links
    Print(4, UI_BAR_Y+UI_BAR_H- (FONT_S.height+3), "S", FONT_S, ST7735_WHITE);
    ST7735_FillRectangleFast(20, UI_BAR_Y+UI_BAR_PAD, UI_BAR_BOX_W, UI_BAR_BOX_H, UI_DARK);
    // F-Box rechts
    Print(ST7735_WIDTH/2-14, UI_BAR_Y+UI_BAR_H- (FONT_S.height+3), "F", FONT_S, ST7735_WHITE);
    ST7735_FillRectangleFast(ST7735_WIDTH/2+4, UI_BAR_Y+UI_BAR_PAD, UI_BAR_BOX_W, UI_BAR_BOX_H, UI_DARK);

    // Startfüllung (25%)
    UI_UpdateBarS(25);
    UI_UpdateBarF(25);
}

// ------ dynamische Updates ------

static void DrawValue(int y, float v, char* lastStr) {
    char buf[20];
    snprintf(buf, sizeof(buf), "%+7.4f", v);

    if (strcmp(buf, lastStr) != 0) {
        int charWidth = 9; // Annahme: Font-Breite, musst du anpassen
        int totalWidth = strlen(buf) * charWidth;
        int rightEdge = ST7735_WIDTH - UI_MARGIN_R-5; // Rechter Rand
        int startX = rightEdge - totalWidth; // Startposition für rechtsbündige Ausrichtung

        // Zeichen für Zeichen vergleichen
        for (int i = 0; i < strlen(buf) && i < strlen(lastStr); i++) {
            if (buf[i] != lastStr[i]) {
                // Nur dieses Zeichen neu zeichnen
                ST7735_FillRectangleFast(startX + i * charWidth, y, charWidth, 12, UI_BG);
                char singleChar[2] = {buf[i], '\0'};
                Print(startX + i * charWidth, y, singleChar, FONT_M, UI_FG);
            }
        }

        // Falls neuer String länger ist
        if (strlen(buf) > strlen(lastStr)) {
            for (int i = strlen(lastStr); i < strlen(buf); i++) {
                char singleChar[2] = {buf[i], '\0'};
                Print(startX + i * charWidth, y, singleChar, FONT_M, UI_FG);
            }
        }
        // Falls neuer String kürzer ist - mehr Bereich löschen
        else if (strlen(buf) < strlen(lastStr)) {
            int oldTotalWidth = strlen(lastStr) * charWidth;
            int oldStartX = rightEdge - oldTotalWidth;
            // Lösche den Bereich zwischen dem neuen und alten String
            ST7735_FillRectangleFast(oldStartX, y, oldStartX - startX, 12, UI_BG);
        }

        strcpy(lastStr, buf);
    }
}

// Globale Arrays für die letzten Werte
static char lastX[20] = "";
static char lastY[20] = "";
static char lastZ[20] = "";

void UI_UpdateWC(float x, float y, float z) {
    DrawValue(UI_TOP_H1_Y, x, lastX);
    DrawValue(UI_TOP_H1_Y+14, y, lastY);
    DrawValue(UI_TOP_H1_Y+28, z, lastZ);
}


//void UI_UpdateMC(float x, float y, float z){
//    DrawValue(UI_TOP_H2_Y,     x);
//    DrawValue(UI_TOP_H2_Y+14,  y);
//    DrawValue(UI_TOP_H2_Y+28,  z);
//}

void UI_UpdatePosText(const char *text){
    // überschreibe „POS: <...>“ rechts vom Label
    ST7735_FillRectangleFast(4+40, UI_BAR_Y+3, 60, FONT_S.height, UI_BLUE);
    Print(4+40, UI_BAR_Y+3, text, FONT_S, ST7735_WHITE);
}

void UI_UpdateStepText(const char *text){
    int x = ST7735_WIDTH/2+4+50;
    ST7735_FillRectangleFast(x, UI_BAR_Y+3, 60, FONT_S.height, UI_BLUE);
    Print(x, UI_BAR_Y+3, text, FONT_S, ST7735_WHITE);
}

static void DrawBar(int x, int y, int w, int h, uint8_t p){
    if (p>100) p=100;
    // Rahmen bleibt, Innenfläche schwarz + grüner Anteil
    ST7735_FillRectangleFast(x, y, w, h, UI_DARK);
    int fill = (w * p) / 100;
    if (fill>0) ST7735_FillRectangleFast(x, y, fill, h, UI_GREEN);
}

void UI_UpdateBarS(uint8_t percent){
    DrawBar(20, UI_BAR_Y+UI_BAR_PAD, UI_BAR_BOX_W, UI_BAR_BOX_H, percent);
}

void UI_UpdateBarF(uint8_t percent){
    DrawBar(ST7735_WIDTH/2+4, UI_BAR_Y+UI_BAR_PAD, UI_BAR_BOX_W, UI_BAR_BOX_H, percent);
}
