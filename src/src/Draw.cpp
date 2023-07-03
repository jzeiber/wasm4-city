#include "Game.h"
#include "Draw.h"
#include "Interface.h"
#include "Font.h"
#include "Strings.h"
#include "Simulation.h"
#include "Defines.h"

#include "wasmstring.h"
#include "palette.h"
#include "global.h"

const uint8_t TileImageData[] =
{
#include "TileData.h"
};

#include "LogoBitmap.h"

// Currently visible tiles are cached so they don't need to be recalculated between frames
uint8_t VisibleTileCache[VISIBLE_TILES_X * VISIBLE_TILES_Y];
int8_t CachedScrollX, CachedScrollY;
uint8_t AnimationFrame = 0;

// A map of which tiles should be on fire when a building is on fire
#define FIREMAP_SIZE 16
const uint8_t FireMap[FIREMAP_SIZE] =
{ 1,2,3,1,2,3,1,3,1,2,3,2,2,1,3,1 };

const uint8_t BuildingPopulaceMap[] =
{ 1,0xd,5,0xb,7,0xe,3,4,1,6,0xc,2,0xa,9,0xb,8 };

const uint8_t* GetTileData(uint8_t tile)
{
	return TileImageData + (tile * 8);
}

const uint8_t GetTileColor(const uint8_t tile)
{
	if(tile>=1 && tile<=4)
	{
		return (PALETTE_GREEN << 4) | PALETTE_WHITE;
	}
	else if(tile>=17 && tile<=20)
	{
		return (PALETTE_WHITE << 4) | PALETTE_BLUE;
	}
	else if(tile==79 || tile==95 || tile==111 || tile==127)	// land/water corner pieces
	{
		return (PALETTE_GREEN << 4) | PALETTE_BLUE;
	}
	return (PALETTE_WHITE << 4) | PALETTE_BLACK;
}

const uint8_t GetTileForegroundColor(const uint8_t tile)
{
	return (GetTileColor(tile) >> 4) & 0b00001111;
}

const uint8_t GetTileBackgroundColor(const uint8_t tile)
{
	return (GetTileColor(tile) & 0b00001111);
}

inline uint8_t GetProcAtTile(uint8_t x, uint8_t y)
{
	return (uint8_t)((((y * 359)) ^ ((x * 431))));
}

bool HasHighTraffic(int x, int y)
{
	// First check for buildings
	for (int n = 0; n < MAX_BUILDINGS; n++)
	{
		Building* building = &State.buildings[n];

		if (building->type && building->heavyTraffic)
		{
			if (x < building->x - 1 || y < building->y - 1)
				continue;
			const BuildingInfo* info = GetBuildingInfo(building->type);
			uint8_t width = info->width;
			uint8_t height = info->height;
			if (x > building->x + width || y > building->y + height)
				continue;

			return true;
		}
	}

	return false;
}

uint8_t CalculateBuildingTile(Building* building, uint8_t x, uint8_t y)
{
	const BuildingInfo* info = GetBuildingInfo(building->type);
	uint8_t width = info->width;
	uint8_t height = info->height;
	uint8_t tile = info->drawTile;

	if (building->onFire)
	{
		if (!((building->type == Industrial || building->type == Commercial || building->type == Residential)
			&& x == 1 && y == 1))
		{
			int index = y * height + x + GetProcAtTile(building->x, building->y);
			bool onFire = building->onFire >= FireMap[index & (FIREMAP_SIZE - 1)];

			if (onFire)
			{
				uint8_t procVal = GetProcAtTile(building->x + x, building->y + y);
				return FIRST_FIRE_TILE + (procVal & 3);
			}
		}
	}

	if (IsRubble(building->type))
		return RUBBLE_TILE;

	// Industrial, commercial and residential buildings have different tiles based on the population density
	if (building->type == Industrial || building->type == Commercial || building->type == Residential)
	{
		if (building->populationDensity >= MAX_POPULATION_DENSITY - 1)
		{
			tile += 48;
		}
		else if (x != 1 || y != 1)
		{
			int index = y * height + x + GetProcAtTile(building->x, building->y);
			bool hasBuilding = building->populationDensity >= BuildingPopulaceMap[index & 0xf];

			if (hasBuilding)
			{
				uint8_t procVal = GetProcAtTile(x, y);
				return FIRST_BUILDING_TILE + (procVal & 7);
			}
		}
	}

	tile += y * 16;
	tile += x;
	return tile;
}

// Calculate which visible tile to use
uint8_t CalculateTile(int x, int y)
{
	// Check out of range
	if (x < 0 || y < 0 || x >= MAP_WIDTH || y >= MAP_HEIGHT)
		return 0;

	// First check for buildings
	for (int n = 0; n < MAX_BUILDINGS; n++)
	{
		Building* building = &State.buildings[n];

		if (building->type)
		{
			if (x < building->x || y < building->y)
				continue;
			const BuildingInfo* info = GetBuildingInfo(building->type);
			uint8_t width = info->width;
			uint8_t height = info->height;
			uint8_t tile = info->drawTile;
			if (x < building->x + width && y < building->y + height)
			{
				return CalculateBuildingTile(building, x - building->x, y - building->y);
			}
		}
	}

	// Next check for roads / powerlines
	uint8_t connections = GetConnections(x, y);

	if (connections == RoadMask)
	{
		int variant = GetConnectivityTileVariant(x, y, connections);

		if(!IsTerrainClear(x, y))
			return FIRST_ROAD_BRIDGE_TILE + (variant & 1);
    
		if (HasHighTraffic(x, y))
			return FIRST_ROAD_TRAFFIC_TILE + variant;
		return FIRST_ROAD_TILE + variant;
	}
	else if (connections == PowerlineMask)
	{
		int variant = GetConnectivityTileVariant(x, y, connections);

		 if(!IsTerrainClear(x, y))
			return FIRST_POWERLINE_BRIDGE_TILE + (variant & 1);
		
		return FIRST_POWERLINE_TILE + variant;
	}
	else if (connections == (PowerlineMask | RoadMask))
	{
		int variant = GetConnectivityTileVariant(x, y, RoadMask) & 1;
		return FIRST_POWERLINE_ROAD_TILE + variant;
	}

	return GetTerrainTile(x, y);
}

inline uint8_t GetCachedTile(int x, int y)
{
	//// Uncomment to visualise power connectivity
	/*
	if (AnimationFrame & 4)
	{
		x += CachedScrollX;
		y += CachedScrollY;
		int index = y * MAP_WIDTH + x;
		int mask = 1 << (index & 7);
		uint8_t val = GetPowerGrid()[index >> 3];

		return (val & mask) != 0 ? 1 : 0;
	}
	*/
	////


	uint8_t tile = VisibleTileCache[y * VISIBLE_TILES_X + x];

	// Animate water tiles
	if (tile >= FIRST_WATER_TILE && tile <= LAST_WATER_TILE)
	{
		//tile = FIRST_WATER_TILE + ((tile - FIRST_WATER_TILE + (AnimationFrame >> 1)) & 3);
		tile = FIRST_WATER_TILE + ((tile - FIRST_WATER_TILE + (AnimationFrame >> 3)) & 3);
	}

	// Animate fire tiles
	if (tile >= FIRST_FIRE_TILE && tile <= LAST_FIRE_TILE)
	{
		//tile = FIRST_FIRE_TILE + ((tile - FIRST_FIRE_TILE + (AnimationFrame >> 1)) & 3);
		tile = FIRST_FIRE_TILE + ((tile - FIRST_FIRE_TILE + (AnimationFrame >> 3)) & 3);
	}

	// Animate traffic tiles
	if ((AnimationFrame & 8) && tile >= FIRST_ROAD_TRAFFIC_TILE && tile <= LAST_ROAD_TRAFFIC_TILE)
	{
		tile += 16;
	}

	return tile;
}

void ResetVisibleTileCache()
{
	CachedScrollX = UIState.scrollX >> TILE_SIZE_SHIFT;
	CachedScrollY = UIState.scrollY >> TILE_SIZE_SHIFT;

	for (int y = 0; y < VISIBLE_TILES_Y; y++)
	{
		for (int x = 0; x < VISIBLE_TILES_X; x++)
		{
			VisibleTileCache[y * VISIBLE_TILES_X + x] = CalculateTile(x + CachedScrollX, y + CachedScrollY);
		}
	}
}

void DrawTiles()
{
	int tileX = 0;
	int offsetX = UIState.scrollX & (TILE_SIZE - 1);

	for (int col = 0; col < DISPLAY_WIDTH; col++)
	{
		int tileY = 0;
		int offsetY = UIState.scrollY & (TILE_SIZE - 1);
		uint8_t currentTile = GetCachedTile(tileX, tileY);
		uint8_t readBuf = GetTileData(currentTile)[offsetX];
		uint8_t fg = GetTileForegroundColor(currentTile);
		uint8_t bg = GetTileBackgroundColor(currentTile);
		readBuf >>= offsetY;

		for (int row = 0; row < DISPLAY_HEIGHT; row++)
		{
			PutPixel(col, row, ((readBuf & 1)==0 ? bg : fg));

			offsetY = (offsetY + 1) & 7;
			readBuf >>= 1;

			if (!offsetY)
			{
				tileY++;
				currentTile = GetCachedTile(tileX, tileY);
				readBuf = GetTileData(currentTile)[offsetX];
				fg = GetTileForegroundColor(currentTile);
				bg = GetTileBackgroundColor(currentTile);
			}
		}

		offsetX = (offsetX + 1) & 7;
		if (!offsetX)
		{
			tileX++;
		}
	}
}

void ScrollUp(int amount)
{
	CachedScrollY -= amount;
	int y = VISIBLE_TILES_Y - 1;

	for (int n = 0; n < VISIBLE_TILES_Y - amount; n++)
	{
		for (int x = 0; x < VISIBLE_TILES_X; x++)
		{
			VisibleTileCache[y * VISIBLE_TILES_X + x] = VisibleTileCache[(y - amount) * VISIBLE_TILES_X + x];
		}
		y--;
	}

	for (y = 0; y < amount; y++)
	{
		for (int x = 0; x < VISIBLE_TILES_X; x++)
		{
			VisibleTileCache[y * VISIBLE_TILES_X + x] = CalculateTile(x + CachedScrollX, y + CachedScrollY);
		}
	}
}

void ScrollDown(int amount)
{
	CachedScrollY += amount;
	int y = 0;

	for (int n = 0; n < VISIBLE_TILES_Y - amount; n++)
	{
		for (int x = 0; x < VISIBLE_TILES_X; x++)
		{
			VisibleTileCache[y * VISIBLE_TILES_X + x] = VisibleTileCache[(y + amount) * VISIBLE_TILES_X + x];
		}
		y++;
	}

	y = VISIBLE_TILES_Y - 1;
	for (int n = 0; n < amount; n++)
	{
		for (int x = 0; x < VISIBLE_TILES_X; x++)
		{
			VisibleTileCache[y * VISIBLE_TILES_X + x] = CalculateTile(x + CachedScrollX, y + CachedScrollY);
		}
		y--;
	}
}

void ScrollLeft(int amount)
{
	CachedScrollX -= amount;
	int x = VISIBLE_TILES_X - 1;

	for (int n = 0; n < VISIBLE_TILES_X - amount; n++)
	{
		for (int y = 0; y < VISIBLE_TILES_Y; y++)
		{
			VisibleTileCache[y * VISIBLE_TILES_X + x] = VisibleTileCache[y * VISIBLE_TILES_X + x - amount];
		}
		x--;
	}

	for (x = 0; x < amount; x++)
	{
		for (int y = 0; y < VISIBLE_TILES_Y; y++)
		{
			VisibleTileCache[y * VISIBLE_TILES_X + x] = CalculateTile(x + CachedScrollX, y + CachedScrollY);
		}
	}
}

void ScrollRight(int amount)
{
	CachedScrollX += amount;
	int x = 0;

	for (int n = 0; n < VISIBLE_TILES_X - amount; n++)
	{
		for (int y = 0; y < VISIBLE_TILES_Y; y++)
		{
			VisibleTileCache[y * VISIBLE_TILES_X + x] = VisibleTileCache[y * VISIBLE_TILES_X + x + amount];
		}
		x++;
	}

	x = VISIBLE_TILES_X - 1;
	for (int n = 0; n < amount; n++)
	{
		for (int y = 0; y < VISIBLE_TILES_Y; y++)
		{
			VisibleTileCache[y * VISIBLE_TILES_X + x] = CalculateTile(x + CachedScrollX, y + CachedScrollY);
		}
		x--;
	}
}

void DrawCursorRect(int32_t cursorDrawX, int32_t cursorDrawY, int32_t cursorWidth, int32_t cursorHeight)
{
	for (int n = 0; n < cursorWidth; n++)
	{
		uint8_t color = ((n + (AnimationFrame >> 1)) & 4) != 0 ? PALETTE_WHITE : PALETTE_BLACK;
		PutPixel(cursorDrawX + n, cursorDrawY + cursorHeight - 1, color);
		PutPixel(cursorDrawX + cursorWidth - n - 1, cursorDrawY, color);
	}

	for (int n = 0; n < cursorHeight; n++)
	{
		uint8_t color = ((n + (AnimationFrame >> 1)) & 4) != 0 ? PALETTE_WHITE : PALETTE_BLACK;
		PutPixel(cursorDrawX, cursorDrawY + n, color);
		PutPixel(cursorDrawX + cursorWidth - 1, cursorDrawY + cursorHeight - n - 1, color);
	}
}

void DrawCursor()
{
	uint8_t cursorX, cursorY;
	int cursorWidth = TILE_SIZE, cursorHeight = TILE_SIZE;

	if (UIState.brush >= FirstBuildingBrush)
	{
		BuildingType buildingType = (BuildingType)(UIState.brush - FirstBuildingBrush + 1);
		GetBuildingBrushLocation(buildingType, &cursorX, &cursorY);
		const BuildingInfo* buildingInfo = GetBuildingInfo(buildingType);
		cursorWidth *= buildingInfo->width;
		cursorHeight *= buildingInfo->height;
	}
	else
	{
		cursorX = UIState.selectX;
		cursorY = UIState.selectY;
	}

	int cursorDrawX, cursorDrawY;
	cursorDrawX = (cursorX * 8) - UIState.scrollX;
	cursorDrawY = (cursorY * 8) - UIState.scrollY;

	if (cursorDrawX >= 0 && cursorDrawY >= 0 && cursorDrawX + cursorWidth < DISPLAY_WIDTH && cursorDrawY + cursorHeight < DISPLAY_HEIGHT)
	{
		DrawCursorRect(cursorDrawX, cursorDrawY, cursorWidth, cursorHeight);
	}
}

void AnimatePowercuts()
{
	bool showPowercut = (AnimationFrame & 8) != 0;

	for (int n = 0; n < MAX_BUILDINGS; n++)
	{
		Building* building = &State.buildings[n];

		if (building->type && building->type != Park && !IsRubble(building->type))
		{
			int screenX = building->x + 1 - CachedScrollX;
			int screenY = building->y + 1 - CachedScrollY;

			if (screenX >= 0 && screenY >= 0 && screenX < VISIBLE_TILES_X && screenY < VISIBLE_TILES_Y)
			{
				if (showPowercut && !building->hasPower)
				{
					VisibleTileCache[screenY * VISIBLE_TILES_X + screenX] = POWERCUT_TILE;
				}
				else
				{
					VisibleTileCache[screenY * VISIBLE_TILES_X + screenX] = CalculateBuildingTile(building, 1, 1);
				}
			}
		}

	}
}

void RefreshTile(uint8_t x, uint8_t y)
{
	int screenX = x - CachedScrollX;
	int screenY = y - CachedScrollY;

	if (screenX >= 0 && screenY >= 0 && screenX < VISIBLE_TILES_X && screenY < VISIBLE_TILES_Y)
	{
		VisibleTileCache[screenY * VISIBLE_TILES_X + screenX] = CalculateTile(x, y);
	}
}

void SetTile(uint8_t x, uint8_t y, uint8_t tile)
{
	int screenX = x - CachedScrollX;
	int screenY = y - CachedScrollY;

	if (screenX >= 0 && screenY >= 0 && screenX < VISIBLE_TILES_X && screenY < VISIBLE_TILES_Y)
	{
		VisibleTileCache[screenY * VISIBLE_TILES_X + screenX] = tile;
	}
}

void RefreshTileAndConnectedNeighbours(uint8_t x, uint8_t y)
{
	RefreshTile(x, y);

	if (x > 0 && GetConnections(x - 1, y))
		RefreshTile(x - 1, y);
	if (x < MAP_WIDTH - 1 && GetConnections(x + 1, y))
		RefreshTile(x + 1, y);
	if (y > 0 && GetConnections(x, y - 1))
		RefreshTile(x, y - 1);
	if (y < MAP_HEIGHT - 1 && GetConnections(x, y + 1))
		RefreshTile(x, y + 1);

}

void RefreshBuildingTiles(Building* building)
{
	const BuildingInfo* info = GetBuildingInfo(building->type);
	uint8_t width = info->width;
	uint8_t height = info->height;

	for (int j = 0; j < height; j++)
	{
		uint8_t y = building->y + j;
		for (int i = 0; i < width; i++)
		{
			uint8_t x = building->x + i;
			int screenX = x - CachedScrollX;
			int screenY = y - CachedScrollY;

			if (screenX >= 0 && screenY >= 0 && screenX < VISIBLE_TILES_X && screenY < VISIBLE_TILES_Y)
			{
				VisibleTileCache[screenY * VISIBLE_TILES_X + screenX] = CalculateBuildingTile(building, i, j);
			}
		}
	}

	// Refresh traffic
	uint8_t x1 = building->x > 0 ? building->x - 1 : 0;
	uint8_t x2 = building->x + width < MAP_WIDTH ? building->x + width : MAP_WIDTH - 1;
	uint8_t y1 = building->y > 0 ? building->y - 1 : 0;
	uint8_t y2 = building->y + height < MAP_HEIGHT ? building->y + height : MAP_HEIGHT - 1;

	for (int i = x1; i <= x2; i++)
	{
		if (GetConnections(i, y1) & RoadMask)
			RefreshTile(i, y1);
		if (GetConnections(i, y2) & RoadMask)
			RefreshTile(i, y2);
	}
	for (int i = y1; i <= y2; i++)
	{
		if (GetConnections(x1, i) & RoadMask)
			RefreshTile(x1, i);
		if (GetConnections(x2, i) & RoadMask)
			RefreshTile(x2, i);
	}

}

void DrawTileAt(uint8_t tile, int x, int y)
{
	for (int col = 0; col < TILE_SIZE; col++)
	{
		uint8_t readBuf = GetTileData(tile)[col];
		uint8_t fg = GetTileForegroundColor(tile);
		uint8_t bg = GetTileBackgroundColor(tile);

		for (int row = 0; row < TILE_SIZE; row++)
		{
			uint8_t color = ((readBuf & 1)==0 ? bg : fg);
			readBuf >>= 1;
			PutPixel(x + col, y + row, color);
		}
	}
}

void DrawFilledRect(int32_t x, int32_t y, int32_t w, int32_t h, uint8_t color)
{
	for (int j = 0; j < h; j++)
	{
		for (int i = 0; i < w; i++)
		{
			PutPixel(x + i, y + j, color);
		}
	}
}

void DrawRect(int32_t x, int32_t y, int32_t w, int32_t h, uint8_t color)
{
	for (int j = 0; j < w; j++)
	{
		PutPixel(x + j, y, color);
		PutPixel(x + j, y + h - 1, color);
	}

	for (int j = 1; j < h - 1; j++)
	{
		PutPixel(x, y + j, color);
		PutPixel(x + w - 1, y + j, color);
	}
}

const char FireReportedStr[] = "Fire reported!";

void DrawUI()
{
	if (UIState.state == ShowingToolbar)
	{
		uint8_t buttonX = 1;

		DrawFilledRect(0, DISPLAY_HEIGHT - TILE_SIZE - 2, NUM_TOOLBAR_BUTTONS * (TILE_SIZE + 1) + 2, TILE_SIZE + 2, PALETTE_WHITE);
		DrawFilledRect(0, DISPLAY_HEIGHT - TILE_SIZE - 2 - FONT_HEIGHT - 1, DISPLAY_WIDTH / 2 + FONT_WIDTH + 1, FONT_HEIGHT + 2, PALETTE_WHITE);

		for (int n = 0; n < NUM_TOOLBAR_BUTTONS; n++)
		{
			if(n==PauseGoToolbarButton && (State.flags & FLAG_PAUSE) == FLAG_PAUSE)
			{
				n++;	// we're currently paused, play icon is next icon
				if((State.flags & FLAG_FAST) == FLAG_FAST)
				{
					n++;	// fast icon is +1 from play icon
				}
			}
			DrawTileAt(FIRST_BRUSH_TILE + n, buttonX, DISPLAY_HEIGHT - TILE_SIZE - 1);
			buttonX += TILE_SIZE + 1;
		}

		DrawCursorRect(UIState.selection * (TILE_SIZE + 1), DISPLAY_HEIGHT - TILE_SIZE - 2, TILE_SIZE + 2, TILE_SIZE + 2);
		const char* currentSelection = GetToolbarString(UIState.selection);
		DrawString(currentSelection, 1, DISPLAY_HEIGHT - FONT_HEIGHT - TILE_SIZE - 2);

		uint16_t cost = 0;

		switch (UIState.selection)
		{
		case 0: cost = BULLDOZER_COST; break;
		case 1: cost = ROAD_COST; break;
		case 2: cost = POWERLINE_COST; break;
		default:
		{
			int buildingIndex = 1 + UIState.selection - FirstBuildingBrush;
			if (buildingIndex < Num_BuildingTypes)
			{
				const BuildingInfo* buildingInfo = GetBuildingInfo(buildingIndex);
				cost = buildingInfo->cost;
			}
		}
		break;
		}
		if (cost > 0)
		{
			DrawCurrency(cost, DISPLAY_WIDTH / 2, DISPLAY_HEIGHT - FONT_HEIGHT - TILE_SIZE - 2);
		}
	}
	else if (UIState.state == InGameDisaster)
	{
		int strLen = strlen(FireReportedStr);
		int x = DISPLAY_WIDTH / 2 - strLen * (FONT_WIDTH / 2);
		DrawFilledRect(x - 1, DISPLAY_HEIGHT - TILE_SIZE - 2, 2 + strlen(FireReportedStr) * FONT_WIDTH + 2, TILE_SIZE + 2, PALETTE_WHITE);

		if (UIState.selection & 4)
		{
			DrawString(FireReportedStr, x, DISPLAY_HEIGHT - FONT_HEIGHT - 1);
		}
	}
	else
	{
		// Current brush at bottom left
		const char* currentSelection = GetToolbarString(UIState.brush);
		DrawFilledRect(0, DISPLAY_HEIGHT - TILE_SIZE - 2, TILE_SIZE + 2 + strlen(currentSelection) * FONT_WIDTH + 2, TILE_SIZE + 2, PALETTE_WHITE);
		DrawTileAt(FIRST_BRUSH_TILE + UIState.brush, 1, DISPLAY_HEIGHT - TILE_SIZE - 1);
		DrawString(currentSelection, TILE_SIZE + 2, DISPLAY_HEIGHT - FONT_HEIGHT - 1);
	}

	// Date at top left
	DrawFilledRect(0, 0, FONT_WIDTH * 8 + 2, FONT_HEIGHT + 2, PALETTE_WHITE);
	DrawString(GetMonthString(State.month), 1, 1);
	DrawInt(State.year + 1900, FONT_WIDTH * 4 + 1, 1);

	if((State.flags & FLAG_PAUSE) == FLAG_PAUSE && ((AnimationFrame & 16) != 0))
	{
		DrawFilledRect(0,FONT_HEIGHT+2,FONT_WIDTH*6 + 2, FONT_HEIGHT+2, PALETTE_WHITE);
		DrawString("Paused",1,FONT_HEIGHT + 3);
	}
	if((State.flags & FLAG_PAUSE) != FLAG_PAUSE && (State.flags & FLAG_FAST) == FLAG_FAST && ((AnimationFrame & 16) !=0))
	{
		DrawFilledRect(0,FONT_HEIGHT+2,FONT_WIDTH*4 + 2, FONT_HEIGHT+2, PALETTE_WHITE);
		DrawString("Fast",1,FONT_HEIGHT + 3);
	}

	// Funds at top right
	uint8_t currencyStrLen = DrawCurrency(State.money, DISPLAY_WIDTH - FONT_WIDTH - 1, 1);
	DrawRect(DISPLAY_WIDTH - 2 - currencyStrLen * FONT_WIDTH, 0, currencyStrLen * FONT_WIDTH + 2, FONT_HEIGHT + 2, PALETTE_WHITE);
}

void DrawInGame()
{
	// Check to see if scrolled to a new location and need to update the visible tile cache
	int tileScrollX = UIState.scrollX >> TILE_SIZE_SHIFT;
	int tileScrollY = UIState.scrollY >> TILE_SIZE_SHIFT;
	int scrollDiffX = tileScrollX - CachedScrollX;
	int scrollDiffY = tileScrollY - CachedScrollY;

	if (scrollDiffX < 0)
	{
		if (scrollDiffX > -VISIBLE_TILES_X)
		{
			ScrollLeft(-scrollDiffX);
		}
		else
		{
			ResetVisibleTileCache();
		}
	}
	else if (scrollDiffX > 0)
	{
		if (scrollDiffX < VISIBLE_TILES_X)
		{
			ScrollRight(scrollDiffX);
		}
		else
		{
			ResetVisibleTileCache();
		}
	}

	if (scrollDiffY < 0)
	{
		if (scrollDiffY > -VISIBLE_TILES_Y)
		{
			ScrollUp(-scrollDiffY);
		}
		else
		{
			ResetVisibleTileCache();
		}
	}
	else if (scrollDiffY > 0)
	{
		if (scrollDiffY < VISIBLE_TILES_Y)
		{
			ScrollDown(scrollDiffY);
		}
		else
		{
			ResetVisibleTileCache();
		}
	}

	AnimatePowercuts();

	DrawTiles();

	if (UIState.state == InGame || UIState.state == InGameDisaster)
	{
		DrawCursor();
	}

	if (UIState.state != StartScreen)
	{
		DrawUI();
	}
}

const char SaveCityStr[] = "Save City";
const char LoadCityStr[] = "Load City";
const char NewCityStr[] = "New City";
const char AutoBudgetStr[] = "Auto Budget:";
const char OnStr[] = "On";
const char OffStr[] = "Off";
const char VersionStr[] = "v0.3";

void DrawSaveLoadMenu()
{
	const int menuWidth = 68;
	const int menuHeight = 50;
	const int spacing = 10;
	DrawRect(DISPLAY_WIDTH / 2 - menuWidth / 2 + 1, DISPLAY_HEIGHT / 2 - menuHeight / 2 + 1, menuWidth, menuHeight, PALETTE_BLACK);
	DrawFilledRect(DISPLAY_WIDTH / 2 - menuWidth / 2, DISPLAY_HEIGHT / 2 - menuHeight / 2, menuWidth, menuHeight, PALETTE_WHITE);
	DrawRect(DISPLAY_WIDTH / 2 - menuWidth / 2, DISPLAY_HEIGHT / 2 - menuHeight / 2, menuWidth, menuHeight, PALETTE_BLACK);

	int32_t y = DISPLAY_HEIGHT / 2 - menuHeight / 2 + spacing / 2 + 2;
	int32_t x = DISPLAY_WIDTH / 2 - menuWidth / 2 + FONT_WIDTH;

	int32_t cursorRectY = y + spacing * UIState.selection - 2;
	DrawCursorRect(x - 2, cursorRectY, menuWidth - FONT_WIDTH * 2 + 4, FONT_HEIGHT + 4);

	DrawString(SaveCityStr, x, y);
	y += spacing;
	DrawString(LoadCityStr, x, y);
	y += spacing;
	DrawString(NewCityStr, x, y);
	y += spacing;
	DrawString(AutoBudgetStr, x, y);

	DrawString(UIState.autoBudget ? OnStr : OffStr, x + 12 * FONT_WIDTH, y);
}

void DrawStartScreen()
{
	const int32_t logoWidth = 72;
	const int32_t logoHeight = 40;
	const int32_t logoY = (DISPLAY_HEIGHT / 2) - 31;
	const int spacing = 9;

	//DrawFilledRect(0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT, PALETTE_WHITE);
	DrawFilledRect((DISPLAY_WIDTH / 2 - (logoWidth /2 )) - 1, logoY -1 , logoWidth + 2, logoHeight + spacing * 2 + 2, PALETTE_WHITE);
	DrawFilledRect(DISPLAY_WIDTH / 2 - logoWidth / 2, logoY, logoWidth, logoHeight, PALETTE_BLACK);
	DrawBitmap(LogoBitmap, DISPLAY_WIDTH / 2 - logoWidth / 2, logoY, logoWidth, logoHeight, PALETTE_BLACK, PALETTE_WHITE);

	int32_t y = logoY + logoHeight - 2;
	int32_t x = DISPLAY_WIDTH / 2 - FONT_WIDTH * 5;

	int32_t cursorRectY = y + spacing * UIState.selection - 2;
	DrawCursorRect(x - 2, cursorRectY, FONT_WIDTH * 10 + 4, FONT_HEIGHT + 4);

	DrawString(NewCityStr, x, y);
	y += spacing;
	DrawString(LoadCityStr, x, y);

	DrawString(VersionStr, DISPLAY_WIDTH - (FONT_WIDTH * strlen(VersionStr)) - 1, DISPLAY_HEIGHT - FONT_HEIGHT - 1);
}

const char LeftArrowStr[] = "<";
const char RightArrowStr[] = ">";

void DrawNewCityMenu()
{
	const int32_t mapY = DISPLAY_HEIGHT / 2 - MAP_HEIGHT / 2 - 4;
	DrawFilledRect(0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT, PALETTE_WHITE);

	DrawFilledRect(DISPLAY_WIDTH / 2 - MAP_WIDTH / 2, mapY, MAP_WIDTH, MAP_HEIGHT, PALETTE_BLACK);
	DrawBitmap(GetTerrainData(State.terrainType), DISPLAY_WIDTH / 2 - MAP_WIDTH / 2, mapY, MAP_WIDTH, MAP_HEIGHT, PALETTE_BLUE, PALETTE_GREEN);
	DrawRect(DISPLAY_WIDTH / 2 - MAP_WIDTH / 2 - 2, mapY - 2, MAP_WIDTH + 4, MAP_HEIGHT + 4, PALETTE_BLACK);

	DrawString(GetTerrainDescription(State.terrainType), DISPLAY_WIDTH / 2 - FONT_WIDTH * 3, mapY + MAP_HEIGHT + 5);
	DrawString(LeftArrowStr, DISPLAY_WIDTH / 2 - MAP_WIDTH / 2 - 6 - FONT_WIDTH, DISPLAY_HEIGHT / 2 - FONT_HEIGHT / 2);
	DrawString(RightArrowStr, DISPLAY_WIDTH / 2 + MAP_WIDTH / 2 + 6, DISPLAY_HEIGHT / 2 - FONT_HEIGHT / 2);
}

const char BudgetHeaderStr[] =		"Budget report for";
const char TaxRateStr[] =			"Tax rate         <   % >";
const char EstYearTaxes[] =			"Est. year taxes";
const char TaxesCollectedStr[] =	"Taxes collected";
const char PoliceBudgetStr[] =		"Police budget";
const char FireBudgetStr[] =		"Fire budget";
const char RoadBudgetStr[] =		"Road budget";
const char CashFlowStr[] =			"Cash flow";

void DrawBudgetMenu()
{
	const int menuWidth = 100;
	const int menuHeight = 63;
	const int spacing = FONT_HEIGHT + 1;
	DrawRect(DISPLAY_WIDTH / 2 - menuWidth / 2 + 1, DISPLAY_HEIGHT / 2 - menuHeight / 2 + 1, menuWidth, menuHeight, PALETTE_BLACK);
	DrawFilledRect(DISPLAY_WIDTH / 2 - menuWidth / 2, DISPLAY_HEIGHT / 2 - menuHeight / 2, menuWidth, menuHeight, PALETTE_WHITE);
	DrawRect(DISPLAY_WIDTH / 2 - menuWidth / 2, DISPLAY_HEIGHT / 2 - menuHeight / 2, menuWidth, menuHeight, PALETTE_BLACK);

	int32_t y = DISPLAY_HEIGHT / 2 - menuHeight / 2 + 2;
	int32_t x = DISPLAY_WIDTH / 2 - menuWidth / 2 + 2;
	int32_t x2 = DISPLAY_WIDTH / 2 + menuWidth / 2 - 2 - FONT_WIDTH;

	DrawString(BudgetHeaderStr, x, y);
	int year = State.year > 0 ? State.year + 1899 : 1900;
	DrawInt(year, x + FONT_WIDTH * 18, y);
	y += spacing + 2;

	DrawString(TaxRateStr, x, y);
	DrawInt(State.taxRate, x + FONT_WIDTH * 19, y);
	y += spacing;

	DrawString(EstYearTaxes, x, y);
	DrawCurrency(GetEstimatedYearTaxes(), x2, y);
	y += spacing;

	DrawString(TaxesCollectedStr, x, y);
	DrawCurrency(State.taxesCollected, x2, y);
	y += spacing;

	DrawString(FireBudgetStr, x, y);
	DrawCurrency(State.fireBudget * FIRE_AND_POLICE_MAINTENANCE_COST, x2, y);
	y += spacing;

	DrawString(PoliceBudgetStr, x, y);
	DrawCurrency(State.policeBudget * FIRE_AND_POLICE_MAINTENANCE_COST, x2, y);
	y += spacing;

	DrawString(RoadBudgetStr, x, y);
	DrawCurrency(State.roadBudget, x2, y);
	y += spacing + 2;

	DrawString(CashFlowStr, x, y);
	DrawCurrency(State.taxesCollected - State.roadBudget - State.policeBudget * FIRE_AND_POLICE_MAINTENANCE_COST - State.fireBudget * FIRE_AND_POLICE_MAINTENANCE_COST, x2, y);
	y += spacing;

	if (UIState.selection < MIN_BUDGET_DISPLAY_TIME)
	{
		UIState.selection++;
	}
}

void DrawDemographicsMenu()
{
	const int menuWidth = 100;
	const int menuHeight = 56;
	const int spacing = FONT_HEIGHT + 1;
	DrawRect(DISPLAY_WIDTH / 2 - menuWidth / 2 + 1, DISPLAY_HEIGHT / 2 - menuHeight / 2 + 1, menuWidth, menuHeight, PALETTE_BLACK);
	DrawFilledRect(DISPLAY_WIDTH / 2 - menuWidth / 2, DISPLAY_HEIGHT / 2 - menuHeight / 2, menuWidth, menuHeight, PALETTE_WHITE);
	DrawRect(DISPLAY_WIDTH / 2 - menuWidth / 2, DISPLAY_HEIGHT / 2 - menuHeight / 2, menuWidth, menuHeight, PALETTE_BLACK);

	int32_t y = DISPLAY_HEIGHT / 2 - menuHeight / 2 + 2;
	int32_t x = DISPLAY_WIDTH / 2 - menuWidth / 2 + 2;
	int32_t x2 = DISPLAY_WIDTH / 2 + menuWidth / 2 - 2 - FONT_WIDTH;

	DrawString("Demographics", x, y);
	y += spacing + 2;

	DrawString("Population", x, y);
	y += spacing;

	DrawString("Residential", x, y);
	DrawRightJustifiedInt(State.residentialPopulation, x2, y);
	y += spacing;

	DrawString("Commercial", x, y);
	DrawRightJustifiedInt(State.commercialPopulation, x2, y);
	y += spacing;

	DrawString("Industrial", x, y);
	DrawRightJustifiedInt(State.industrialPopulation, x2, y);
	y += spacing;

	y += 2;
	DrawString("Total", x, y);
	DrawRightJustifiedInt(State.residentialPopulation + State.commercialPopulation + State.industrialPopulation, x2, y);
	y += spacing;

}

void DrawTile(const uint8_t tile, const int32_t x, const int32_t y, const uint8_t fg, const uint8_t bg, const bool transparent)
{
	const uint8_t *data=GetTileData(tile);
	int32_t byte=0;
	int32_t bit=0;
	for(int32_t xx=0; xx<TILE_SIZE; xx++)
	{
		for(int32_t yy=0; yy<TILE_SIZE; yy++)
		{
			if((data[byte] & (1 << bit))==0)
			{
				PutPixel(x+xx,y+yy,fg);
			}
			else if(transparent==false)
			{
				PutPixel(x+xx,y+yy,bg);
			}
			bit++;
			if(bit==8)
			{
				byte++;
				bit=0;
			}
		}
	}
}

void DrawMapMenu()
{
	bool showbuildings=(State.flags & FLAG_MAP_SHOWBUILDINGS)==FLAG_MAP_SHOWBUILDINGS;
	bool showroads=(State.flags & FLAG_MAP_SHOWROADS)==FLAG_MAP_SHOWROADS;
	bool showpower=(State.flags & FLAG_MAP_SHOWELECTRIC)==FLAG_MAP_SHOWELECTRIC;
	bool showfdrange=(State.flags & FLAG_MAP_SHOWFDRANGE)==FLAG_MAP_SHOWFDRANGE;

	const int32_t offsetx=(SCREEN_SIZE-(3*MAP_WIDTH))/2;

	*DRAW_COLORS=(PALETTE_WHITE << 4) | PALETTE_WHITE;
	rect(0,0,SCREEN_SIZE,SCREEN_SIZE);

	/*
	for(int i=0; i<MAX_BUILDINGS; i++)
	{
		if(State.buildings[i].type)
		{
			State.buildings[i].type
		}
	}
	*/
	for(int y=0; y<MAP_HEIGHT; y++)
	{
		for(int x=0; x<MAP_WIDTH; x++)
		{
			if(IsTerrainClear(x,y)==true)
			{
				*DRAW_COLORS=(PALETTE_GREEN << 4) | PALETTE_GREEN;
			}
			else
			{
				*DRAW_COLORS=(PALETTE_BLUE << 4) | PALETTE_BLUE;
			}
			rect(offsetx+(x*3),y*3,3,3);

			if(showroads==true)
			{
				uint8_t connections=GetConnections(x,y);
				if((connections & RoadMask) == RoadMask)
				{
					*DRAW_COLORS=PALETTE_BLACK;
					line(offsetx+1+(x*3),1+(y*3),offsetx+1+(x*3),1+(y*3));
					if(x>0 && (GetConnections(x-1,y) & RoadMask)==RoadMask)
					{
						line(offsetx+(x*3),1+(y*3),offsetx+(x*3),1+(y*3));
					}
					if(x<MAP_WIDTH-1 && (GetConnections(x+1,y) & RoadMask)==RoadMask)
					{
						line(offsetx+2+(x*3),1+(y*3),offsetx+2+(x*3),1+(y*3));
					}
					if(y>0 && (GetConnections(x,y-1) & RoadMask)==RoadMask)
					{
						line(offsetx+1+(x*3),(y*3),offsetx+1+(x*3),(y*3));
					}
					if(y<MAP_HEIGHT-1 && (GetConnections(x,y+1) & RoadMask)==RoadMask)
					{
						line(offsetx+1+(x*3),2+(y*3),offsetx+1+(x*3),2+(y*3));
					}
				}
			}

			if(showpower==true)
			{
				uint8_t connections=GetConnections(x,y);
				if((connections & PowerlineMask) == PowerlineMask)
				{
					*DRAW_COLORS=PALETTE_WHITE;
					line(offsetx+1+(x*3),1+(y*3),offsetx+1+(x*3),1+(y*3));
					if(x>0 && (GetConnections(x-1,y) & PowerlineMask)==PowerlineMask)
					{
						line(offsetx+(x*3),1+(y*3),offsetx+(x*3),1+(y*3));
					}
					if(x<MAP_WIDTH-1 && (GetConnections(x+1,y) & PowerlineMask)==PowerlineMask)
					{
						line(offsetx+2+(x*3),1+(y*3),offsetx+2+(x*3),1+(y*3));
					}
					if(y>0 && (GetConnections(x,y-1) & PowerlineMask)==PowerlineMask)
					{
						line(offsetx+1+(x*3),(y*3),offsetx+1+(x*3),(y*3));
					}
					if(y<MAP_HEIGHT-1 && (GetConnections(x,y+1) & PowerlineMask)==PowerlineMask)
					{
						line(offsetx+1+(x*3),2+(y*3),offsetx+1+(x*3),2+(y*3));
					}
				}
			}

		}
	}

	if(showbuildings==true)
	{
		for(int i=0; i<MAX_BUILDINGS; i++)
		{
			if(State.buildings[i].type!=BuildingType_None)
			{
				const BuildingInfo *bi=GetBuildingInfo(State.buildings[i].type);
				*DRAW_COLORS=(PALETTE_WHITE << 4) | PALETTE_WHITE;
				rect(offsetx+(State.buildings[i].x*3),(State.buildings[i].y*3),3*bi->width,3*bi->height);
				if(showpower==true && State.buildings[i].type != Park && !IsRubble(State.buildings[i].type))
				{
					if(State.buildings[i].type==Powerplant || (State.buildings[i].hasPower==false && (AnimationFrame & 8)))
					{
						DrawTile(POWERCUT_TILE,offsetx+(State.buildings[i].x*3),(State.buildings[i].y*3),PALETTE_BLACK,PALETTE_WHITE,false);
					}
				}
				if(showfdrange==true)
				{
					uint8_t closestdistance = 0xff;
					for(int j=0; j<MAX_BUILDINGS; j++)
					{
						if(i!=j && State.buildings[j].type==FireDept && State.buildings[j].hasPower==true)
						{
							uint8_t dist=GetManhattanDistance(&State.buildings[i],&State.buildings[j]);
							if(dist<closestdistance)
							{
								closestdistance=dist;
							}
						}
					}
					
					//calc from Simulation.cpp - SimulateBuilding
					int fireDeptInfluence = SIM_FIRE_DEPT_BASE_INFLUENCE + closestdistance * SIM_FIRE_DEPT_INFLUENCE_MULTIPLIER;

					if(fireDeptInfluence <= 255 || (State.buildings[i].type==FireDept && State.buildings[i].hasPower==true))
					{
						// draw fire extingisher icon - if building is a fire dept, then flash icon
						if(State.buildings[i].type!=FireDept || (AnimationFrame & 8))
						{
							DrawTile(247,offsetx+(State.buildings[i].x*3),(State.buildings[i].y*3),PALETTE_BLACK,PALETTE_WHITE,false);
						}
					}

				}
				// if on fire - show flasing fire icon
				//if(State.buildings[i].onFire)
				if(State.buildings[i].onFire && (AnimationFrame & 8))
				{
					DrawTile(FIRST_FIRE_TILE,offsetx+(State.buildings[i].x*3),(State.buildings[i].y*3),PALETTE_BLACK,PALETTE_WHITE,false);
					//const uint8_t tile = FIRST_FIRE_TILE + ((tile - FIRST_FIRE_TILE + (AnimationFrame >> 3)) & 3)
					//GetTileData(tile);
					//DrawTileAt()
				}
			}
		}
	}

	DrawString("Buildings",9,SCREEN_SIZE-15);
	DrawString("Roads",9,SCREEN_SIZE-7);
	DrawString("Electric",SCREEN_SIZE/2,SCREEN_SIZE-15);
	DrawString("FD Range",SCREEN_SIZE/2,SCREEN_SIZE-7);

	if(showbuildings==true)
	{
		DrawString("*",2,SCREEN_SIZE-14);
	}
	if(showroads==true)
	{
		DrawString("*",2,SCREEN_SIZE-6);
	}
	if(showpower==true)
	{
		DrawString("*",SCREEN_SIZE/2-8,SCREEN_SIZE-14);
	}
	if(showfdrange==true)
	{
		DrawString("*",SCREEN_SIZE/2-8,SCREEN_SIZE-6);
	}

	int32_t xpos=0;
	int32_t ypos=SCREEN_SIZE-16;
	switch(UIState.selection)
	{
	case 0:
		break;
	case 1:
		ypos=SCREEN_SIZE-8;
		break;
	case 2:
		xpos=SCREEN_SIZE/2-10;
		break;
	case 3:
		xpos=SCREEN_SIZE/2-10;
		ypos=SCREEN_SIZE-8;
	}
	DrawCursorRect(xpos, ypos, 8, 8);
}

void Draw()
{
	switch (UIState.state)
	{
	case StartScreen:
		DrawInGame();
		DrawStartScreen();
		break;
	case NewCityMenu:
		DrawNewCityMenu();
		break;
	case InGame:
	case InGameDisaster:
	case ShowingToolbar:
		DrawInGame();
		break;
	case SaveLoadMenu:
		DrawSaveLoadMenu();
		break;
	case BudgetMenu:
		DrawBudgetMenu();
		break;
	case DemographicsMenu:
		DrawDemographicsMenu();
		break;
	case MapMenu:
		DrawMapMenu();
		break;
	}

	AnimationFrame++;

}
