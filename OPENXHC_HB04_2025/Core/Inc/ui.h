/*
 * ui.h
 *
 *  Created on: Sep 14, 2025
 *      Author: Thomas Weckmann
 */

#ifndef INC_UI_H_
#define INC_UI_H_

#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "st7735.h"
#include "fonts.h"

// Farben (RGB565)
#define UI_BG        ST7735_WHITE
#define UI_FG        ST7735_BLACK
#define UI_LINE      0x8410   // ~Grauton
#define UI_BLUE      0x001F   // unten
#define UI_GREEN     0x07E0
#define UI_DARK      0x0000

// Layout-Konstanten
#define UI_MARGIN_L   2
#define UI_MARGIN_R   2
#define UI_TOP_H1_Y   2       // "WC" Block Top
#define UI_TOP_H2_Y   50      // "MC" Block Top (ca. mittig)
#define UI_LINE_Y     45      // Trennlinie

// Statusleiste unten
#define UI_BAR_H      26
#define UI_BAR_Y     (ST7735_HEIGHT - UI_BAR_H)
#define UI_BAR_PAD    4
#define UI_BAR_BOX_W  120
#define UI_BAR_BOX_H  18

// Public API
void UI_DrawStatic(void);
void UI_UpdateWC(float x, float y, float z);
void UI_UpdateMC(float x, float y, float z);
void UI_UpdatePosText(const char *text);       // z.B. "OFF"
void UI_UpdateStepText(const char *text);      // z.B. "0.001"
void UI_UpdateBarS(uint8_t percent);           // 0..100
void UI_UpdateBarF(uint8_t percent);           // 0..100

#endif /* INC_UI_H_ */
