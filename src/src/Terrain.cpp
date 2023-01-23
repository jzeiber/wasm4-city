#include <stdint.h>
#include "Terrain.h"
#include "Game.h"
#include "Defines.h"
#include "randommt.h"
#include "global.h"
#include "perlinnoise.h"

const uint8_t Terrain1Data[] =
{
#include "Terrain1.inc.h"
};
const uint8_t Terrain2Data[] =
{
#include "Terrain2.inc.h"
};
const uint8_t Terrain3Data[] =
{
#include "Terrain3.inc.h"
};
const uint8_t Terrain4Data[] =
{
#include "Terrain4.inc.h"
};
const uint8_t Terrain5Data[] =
{
#include "Terrain5.inc.h"
};
uint8_t Terrain6Data[288] = {0};

const char Terrain1Str[] = "River";
const char Terrain2Str[] = "Island";
const char Terrain3Str[] = "Lake";
const char Terrain4Str[] = "Plains";
const char Terrain5Str[] = "Seaside";
const char Terrain6Str[] = "Random";

const char* GetTerrainDescription(uint8_t index)
{
	switch (index)
	{
	default:
	case 0:
		return Terrain1Str;
	case 1:
		return Terrain2Str;
	case 2:
		return Terrain3Str;
	case 3:
		return Terrain4Str;
	case 4:
		return Terrain5Str;
	case 5:
		return Terrain6Str;
	}
}

const uint8_t* GetTerrainData(uint8_t index)
{
	switch (index)
	{
	default:
	case 0: return Terrain1Data;
	case 1: return Terrain2Data;
	case 2: return Terrain3Data;
	case 3: return Terrain4Data;
	case 4: return Terrain5Data;
	case 5: return Terrain6Data;
	}

}

bool IsTerrainClear(int x, int y)
{
	int32_t bitpos=(y*MAP_WIDTH)+x;
	int32_t byte=bitpos/8;
	int32_t bit=7-bitpos%8;
	
	const uint8_t *terrain=GetTerrainData(State.terrainType);
	return ((terrain[byte] & (1 << bit)) != 0);
}

uint8_t GetTerrainTile(int x, int y)
{
	bool northClear = y == 0 || IsTerrainClear(x, y - 1);
	bool eastClear = x >= MAP_WIDTH - 1 || IsTerrainClear(x + 1, y);
	bool southClear = y >= MAP_HEIGHT - 1 || IsTerrainClear(x, y + 1);
	bool westClear = x == 0 || IsTerrainClear(x - 1, y);

	if (IsTerrainClear(x, y))
	{
		if (!northClear && !westClear)
			return NORTH_WEST_EDGE_TILE;
		if (!northClear && !eastClear)
			return NORTH_EAST_EDGE_TILE;
		if (!southClear && !westClear)
			return SOUTH_WEST_EDGE_TILE;
		if (!southClear && !eastClear)
			return SOUTH_EAST_EDGE_TILE;

		return FIRST_TERRAIN_TILE + ((((y * 359)) ^ ((x * 431))) & 3);
	}
	else
	{
		return FIRST_WATER_TILE + ((((y * 359)) ^ ((x * 431))) & 3);
	}
}

void GenerateRandomTerrain(const uint8_t terrainType, const uint64_t seed)
{
	if(terrainType!=5)
	{
		return;
	}

	RandomMT rand;
	rand.Seed(seed);

	PerlinNoise perlin;
	perlin.Setup(rand.Next());

	int32_t offsetx=rand.Next();
	int32_t offsety=rand.Next();

	if(offsetx<0)
	{
		offsetx=-offsetx;
	}
	if(offsety<0)
	{
		offsety=-offsety;
	}

	// Use perlin noise to generate terrain
	for(int32_t y=0; y<MAP_HEIGHT; y++)
	{
		for(int32_t x=0; x<MAP_WIDTH; x++)
		{
			double px=static_cast<double>(offsetx+x)/32.0;
			double py=static_cast<double>(offsety+y)/32.0;
			double ph=perlin.Get(px,py,128,10);

			int32_t bitpos=(y*MAP_WIDTH)+x;
			int32_t byte=bitpos/8;
			int32_t bit=(7-(bitpos%8));

			if(ph<-0.3)
			{

				Terrain6Data[byte]=Terrain6Data[byte] & ~(0xff & (1 << bit));
			}
			else
			{
				Terrain6Data[byte]=Terrain6Data[byte] | (0xff & (1 << bit));
			}

		}
	}
}
