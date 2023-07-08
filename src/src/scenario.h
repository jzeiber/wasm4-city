#pragma once

#include <stdint.h>

enum ScenarioFlag
{
SCENARIO_FLAG_SANDBOX=1,
SCENARIO_FLAG_SCENARIO
};

#define SCENARIO_COUNT  10
#define SCENARIO_GOAL_BUILDING_COUNT 6

typedef struct
{
    const char *title;
    const char *description;
    uint8_t flags;
    uint8_t mapidx;
    const uint8_t *stateptr;                        // pointer to initial saved game state
    uint16_t startyear;
    uint16_t goalyear;                              // year goal
    int32_t startfunds;
    int32_t goalfunds;                              // fund goal
    uint32_t goalrespop;                            // residential population
    uint32_t goalcompop;                            // commercial population
    uint32_t goalindpop;                            // industrial population
    uint8_t goalbuilding[SCENARIO_GOAL_BUILDING_COUNT];            // goal for specific building types
    uint8_t goalbuildingcount[SCENARIO_GOAL_BUILDING_COUNT];       // how many of each building are needed
} Scenario;

extern const Scenario ScenarioData[];

