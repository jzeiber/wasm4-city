#pragma once

#include <stdint.h>

#include "Building.h"

void PutPixel(int32_t x, int32_t y, uint8_t color);
void DrawFilledRect(int32_t x, int32_t y, int32_t w, int32_t h, uint8_t color);
void DrawRect(int32_t x, int32_t y, int32_t w, int32_t h, uint8_t color);
void DrawBitmap(const uint8_t* bmp, int32_t x, int32_t y, int32_t w, int32_t h, uint8_t fg, uint8_t bg);

void Draw(void);

void ResetVisibleTileCache(void);
void RefreshBuildingTiles(Building* building);
void RefreshTile(uint8_t x, uint8_t y);
void RefreshTileAndConnectedNeighbours(uint8_t x, uint8_t y);

void SetTile(uint8_t x, uint8_t y, uint8_t tile);
