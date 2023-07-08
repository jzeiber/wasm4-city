#include "scenario.h"

#include "democity.h"
#include "Terrain.h"
#include "Building.h"

const Scenario ScenarioData[]=
{
/*TITLE        DESC       FLAGS                      MAP    STATE  ,           SY    EY     SF     EF   RP   CP   IP  BUILDINGS    BUILDCNT*/
{"River",      "Sandbox", SCENARIO_FLAG_SANDBOX,     0,     nullptr,            0,    0,     0,     0,   0,   0,   0, {0,0,0,0,0,0}, {0,0,0,0,0,0}},
{"Island",     "Sandbox", SCENARIO_FLAG_SANDBOX,     1,     nullptr,            0,    0,     0,     0,   0,   0,   0, {0,0,0,0,0,0}, {0,0,0,0,0,0}},
{"Lake",       "Sandbox", SCENARIO_FLAG_SANDBOX,     2,     nullptr,            0,    0,     0,     0,   0,   0,   0, {0,0,0,0,0,0}, {0,0,0,0,0,0}},
{"Plains",     "Sandbox", SCENARIO_FLAG_SANDBOX,     3,     nullptr,            0,    0,     0,     0,   0,   0,   0, {0,0,0,0,0,0}, {0,0,0,0,0,0}},
{"Seaside",    "Sandbox", SCENARIO_FLAG_SANDBOX,     4,     nullptr,            0,    0,     0,     0,   0,   0,   0, {0,0,0,0,0,0}, {0,0,0,0,0,0}},
{"Random",     "Sandbox", SCENARIO_FLAG_SANDBOX,     5,     nullptr,            0,    0,     0,     0,   0,   0,   0, {0,0,0,0,0,0}, {0,0,0,0,0,0}},
{"Coastal Rescue", "A coastal city was just devastated \nby a hurricane.  You must rebuild the \ncity with funds from a 30 year loan.", 
                          SCENARIO_FLAG_SCENARIO,    4, coastalscenario,     1970, 2000, 10000, 30000, 400, 350, 300, {0,0,0,0,0,0}, {0,0,0,0,0,0}},
{"Rural Growth", "Grow a small rural community into a \ncity.", 
                          SCENARIO_FLAG_SCENARIO,     3, ruralscenario,      1900, 1950,  3000, 25000, 550, 400, 400, {Stadium,FireDept,PoliceDept,0,0,0}, {1,1,1,0,0,0}},
{"Urban Revitalization", "Poor planning has stagnated the growth \nof the city. Rebuild and revitalize \nthe area.", 
                          SCENARIO_FLAG_SCENARIO,     0, revitalizescenario, 1980, 2010,  7500, 70000, 500, 500, 500, {Stadium,Park,FireDept,PoliceDept,0,0},{1,5,2,2,0,0}},
{"Island Paradise", "The tourist idustry has dried up. \nRebuild the island to be a self \nsufficient utopia.", 
                          SCENARIO_FLAG_SCENARIO,     1, islandscenario,     2000, 2020,  5000, 20000, 400, 300, 300, {0,0,0,0,0,0}, {0,0,0,0,0,0}}
};
