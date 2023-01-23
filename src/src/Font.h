#pragma once

#include <stdint.h>

#define FONT_WIDTH 4
#define FONT_HEIGHT 6

void DrawString(const char* str, int32_t x, int32_t y);
void DrawInt(int16_t val, int32_t x, int32_t y);
uint8_t DrawRightJustifiedInt(int32_t val, int32_t x, int32_t y);
uint8_t DrawCurrency(int32_t val, int32_t x, int32_t y);
