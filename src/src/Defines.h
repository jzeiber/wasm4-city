#pragma once

#include "wasm4.h"

#define TILE_SIZE 8
#define TILE_SIZE_SHIFT 3

#ifdef _WIN32
//#define DISPLAY_WIDTH 192
//#define DISPLAY_HEIGHT 192
#define DISPLAY_WIDTH SCREEN_SIZE
#define DISPLAY_HEIGHT SCREEN_SIZE
#define MAP_WIDTH 48
#define MAP_HEIGHT 48
#else
//#define DISPLAY_WIDTH 128
//#define DISPLAY_HEIGHT 64
#define DISPLAY_WIDTH SCREEN_SIZE
#define DISPLAY_HEIGHT SCREEN_SIZE
#define MAP_WIDTH 48
#define MAP_HEIGHT 48
#endif


#define MAX_SCROLL_X (MAP_WIDTH * TILE_SIZE - DISPLAY_WIDTH)
#define MAX_SCROLL_Y (MAP_HEIGHT * TILE_SIZE - DISPLAY_HEIGHT)

#define VISIBLE_TILES_X ((DISPLAY_WIDTH / TILE_SIZE) + 1)
#define VISIBLE_TILES_Y ((DISPLAY_HEIGHT / TILE_SIZE) + 1)

#define MAX_BUILDINGS 130

// How long a button has to be held before the first event repeats
#define INPUT_REPEAT_TIME 10

// When repeating, how long between each event is fired
#define INPUT_REPEAT_FREQUENCY 2

#define BULLDOZER_COST 1
#define ROAD_COST 10
#define POWERLINE_COST 5

#define FIRST_TERRAIN_TILE 1
#define FIRST_WATER_TILE 17
#define LAST_WATER_TILE (FIRST_WATER_TILE + 3)

#define FIRST_FIRE_TILE 232
#define LAST_FIRE_TILE (FIRST_FIRE_TILE + 3)

#define FIRST_ROAD_TILE 5
#define FIRST_ROAD_TRAFFIC_TILE (FIRST_ROAD_TILE + 16)
#define LAST_ROAD_TRAFFIC_TILE (FIRST_ROAD_TRAFFIC_TILE + 10)
#define FIRST_POWERLINE_TILE 53
#define FIRST_POWERLINE_ROAD_TILE 49

#define FIRST_ROAD_BRIDGE_TILE 32
#define FIRST_POWERLINE_BRIDGE_TILE 34

#define FIRST_BUILDING_TILE 224

#define FIRST_EDGE_TILE 79
#define NORTH_WEST_EDGE_TILE FIRST_EDGE_TILE
#define NORTH_EAST_EDGE_TILE (FIRST_EDGE_TILE + 16)
#define SOUTH_WEST_EDGE_TILE (FIRST_EDGE_TILE + 32)
#define SOUTH_EAST_EDGE_TILE (FIRST_EDGE_TILE + 48)

#define POWERCUT_TILE 48
#define RUBBLE_TILE 51

#define FIRST_BRUSH_TILE 238

#define NUM_TOOLBAR_BUTTONS 16

#define MAX_POPULATION_DENSITY 15

#define NUM_TERRAIN_TYPES 6

#define STARTING_TAX_RATE 7
#define STARTING_FUNDS 10000

#define FIRE_AND_POLICE_MAINTENANCE_COST 100
#define ROAD_MAINTENANCE_COST 10

#define POPULATION_MULTIPLIER 17

#define MIN_BUDGET_DISPLAY_TIME 16

#define BUILDING_MAX_FIRE_COUNTER 3

#define MIN_FRAMES_BETWEEN_DISASTER 2500
//#define FRAMES_PER_YEAR (MAX_BUILDINGS * 12)
#define FRAMES_PER_YEAR (MAX_BUILDINGS * 30)
#define MIN_TIME_BETWEEN_DISASTERS (FRAMES_PER_YEAR * 2)
#define MAX_TIME_BETWEEN_DISASTERS (FRAMES_PER_YEAR * 6)

//#define DISASTER_MESSAGE_DISPLAY_TIME 60
#define DISASTER_MESSAGE_DISPLAY_TIME 255


// following originally in Simulation.cpp

#define SIM_INCREMENT_POP_THRESHOLD 20				// Score must be more than this to grow
#define SIM_DECREMENT_POP_THRESHOLD -30				// Score must be less than this to shrink

#define AVERAGE_POPULATION_DENSITY 8
#define SIM_BASE_SCORE 15							// When population is zero
#define SIM_AVERAGING_STRENGTH 0
#define SIM_EMPLOYMENT_BOOST 10
#define SIM_UNEMPLOYMENT_PENALTY 100
#define SIM_INDUSTRIAL_OPPORTUNITY_BOOST 10
#define SIM_COMMERCIAL_OPPORTUNITY_BOOST 10
#define SIM_LOCAL_BUILDING_DISTANCE 32
#define SIM_LOCAL_BUILDING_INFLUENCE 4
#define SIM_STADIUM_BOOST 100
#define SIM_PARK_BOOST 5
#define SIM_MAX_CRIME 50
#define SIM_RANDOM_STRENGTH_MASK 31
#define SIM_POLLUTION_INFLUENCE 2
#define SIM_MAX_POLLUTION 50
#define SIM_INDUSTRIAL_BASE_POLLUTION 8
#define SIM_TRAFFIC_BASE_POLLUTION 8
#define SIM_POWERPLANT_BASE_POLLUTION 32
#define SIM_HEAVY_TRAFFIC_THRESHOLD 12
#define SIM_IDEAL_TAX_RATE 6
#define SIM_TAX_RATE_PENALTY 10
#define SIM_FIRE_SPREAD_CHANCE 64					// If 8 bit rand value is less than this then attempt to spread fire
#define SIM_FIRE_BURN_CHANCE 64						// If 8 bit rand value is less than this then increase fire counter
#define SIM_FIRE_DEPT_BASE_INFLUENCE 64				// Higher means less influence
#define SIM_FIRE_DEPT_INFLUENCE_MULTIPLIER 5		// Higher means less influence (based on distance)