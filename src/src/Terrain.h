#pragma once

uint8_t GetTerrainTile(int x, int y);
uint8_t GetAnimatedTerrainTile(int x, int y);
bool IsTerrainClear(int x, int y);

//const char* GetTerrainDescription(uint8_t index);
const uint8_t* GetTerrainData(uint8_t index);
void GenerateRandomTerrain(const uint8_t terrainType, const uint64_t seed);

/*
extern const uint8_t Terrain1Data[];
extern const uint8_t Terrain2Data[];
extern const uint8_t Terrain3Data[];
extern const uint8_t Terrain4Data[];
extern const uint8_t Terrain5Data[];
extern uint8_t Terrain6Data[];
*/