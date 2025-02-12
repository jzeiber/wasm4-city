#include "Game.h"
#include "Connectivity.h"
#include "Draw.h"
#include "Interface.h"
#include "Simulation.h"
#include "scenario.h"

enum SimulationSteps
{
	SimulateBuildings = 0,
	SimulatePower = MAX_BUILDINGS,
	SimulatePopulation,
	SimulateFastNextMonth,
	SimulateNextMonth = 360
};

#ifdef _WIN32
void DebugBuildingScore(Building* building, int score, int crime, int pollution, int localInfluence, int populationEffect, int randomEffect);
#else
inline void DebugBuildingScore(Building* building, int score, int crime, int pollution, int localInfluence, int populationEffect, int randomEffect) {}
#endif

uint8_t GetNumRoadConnections(Building* building)
{
	const BuildingInfo* info = GetBuildingInfo(building->type);
	uint8_t width = info->width;
	uint8_t height = info->height;
	uint8_t count = 0;

	if(building->y > 0)
	{
		for(uint8_t i = 0; i < width; i++)
		{
			if(GetConnections(building->x + i, building->y - 1) & RoadMask)
			{
				count++;
			}
		}
	}
	if(building->y + height < MAP_HEIGHT)
	{
		for(uint8_t i = 0; i < width; i++)
		{
			if(GetConnections(building->x + i, building->y + height) & RoadMask)
			{
				count++;
			}
		}
	}
	if(building->x > 0)
	{
		for(uint8_t i = 0; i < height; i++)
		{
			if(GetConnections(building->x - 1, building->y + i) & RoadMask)
			{
				count++;
			}
		}
	}
	if(building->x + width < MAP_WIDTH)
	{
		for(uint8_t i = 0; i < height; i++)
		{
			if(GetConnections(building->x + width, building->y + i) & RoadMask)
			{
				count++;
			}
		}
	}
		
	return count;
}

double MonthlyTaxes()
{
	const int32_t totalPopulation = static_cast<int32_t>(State.residentialPopulation + State.commercialPopulation + State.industrialPopulation) * POPULATION_MULTIPLIER;
	return static_cast<double>(totalPopulation * State.taxRate) / 1200.0;
}

int32_t GetEstimatedYearTaxes()
{
	return State.accumulatedMonthlyTaxes + (State.month <= 11 ? (static_cast<double>(12-State.month) * MonthlyTaxes()) : 0);
}

void DoMonthEndBudget()
{
	// Accumulate monthly taxes
	State.accumulatedMonthlyTaxes += (MonthlyTaxes() + 0.5);		// round any decimal to nearest integer
}

void DoYearEndBudget()
{
	// Collect taxes (taxes now accumulated monthly and then lump sum added at end of year)
	//int32_t totalPopulation = (State.residentialPopulation + State.commercialPopulation + State.residentialPopulation) * POPULATION_MULTIPLIER;
	//State.taxesCollected = (totalPopulation * State.taxRate) / 100;
	State.taxesCollected = State.accumulatedMonthlyTaxes;
	State.accumulatedMonthlyTaxes = 0;

	State.money += State.taxesCollected;

	// Count police and fire departments for costing
	uint8_t numPoliceDept = 0;
	uint8_t numFireDept = 0;

	for (int n = 0; n < MAX_BUILDINGS; n++)
	{
		if (State.buildings[n].type == PoliceDept)
		{
			numPoliceDept++;
		}
		else if (State.buildings[n].type == FireDept)
		{
			numFireDept++;
		}
	}

	State.fireBudget = numFireDept;
	State.policeBudget = numPoliceDept;

	State.money -= FIRE_AND_POLICE_MAINTENANCE_COST * numFireDept;
	State.money -= FIRE_AND_POLICE_MAINTENANCE_COST * numPoliceDept;

	// Count road tiles for cost of road maintenance
	int numRoadTiles = 0;
	for (int y = 0; y < MAP_HEIGHT; y++)
	{
		for (int x = 0; x < MAP_WIDTH; x++)
		{
			if (GetConnections(x, y) & RoadMask)
				numRoadTiles++;
		}
	}

	State.roadBudget = (numRoadTiles * ROAD_MAINTENANCE_COST) / 100;
	State.money -= State.roadBudget;

#ifdef _WIN32
	printf("Budget for %d:\n", State.year + 1899);
	printf("Population: %d\n", totalPopulation);
	printf("Taxes collected: $%d\n", State.taxesCollected);
	printf("Police cost: %d x $%d = $%d\n", numPoliceDept, FIRE_AND_POLICE_MAINTENANCE_COST, FIRE_AND_POLICE_MAINTENANCE_COST * numPoliceDept);
	printf("Fire cost: %d x $%d = $%d\n", numFireDept, FIRE_AND_POLICE_MAINTENANCE_COST, FIRE_AND_POLICE_MAINTENANCE_COST * numFireDept);
	printf("Road maintenance: %d tiles = $%d\n", numRoadTiles, State.roadBudget);
#endif

	int32_t cashFlow = State.taxesCollected - State.roadBudget - State.policeBudget * FIRE_AND_POLICE_MAINTENANCE_COST - State.fireBudget * FIRE_AND_POLICE_MAINTENANCE_COST;
	if (!UIState.autoBudget || cashFlow <= 0 || State.money <= 0)
	{
		UIState.state = BudgetMenu;
		UIState.selection = 0;
	}
}

bool SpreadFire(Building* building)
{
	const BuildingInfo* info = GetBuildingInfo(building->type);
	uint8_t width = info->width;
	uint8_t height = info->height;
	uint8_t x1 = building->x > 1 ? building->x - 2 : building->x;
	uint8_t y1 = building->y > 1 ? building->y - 2 : building->y;
	uint8_t x2 = building->x + width + 2;
	uint8_t y2 = building->y + height + 2;
	uint8_t spreadDirection = GetRand() & 3;

	if (spreadDirection & 1)
	{
		for (uint8_t j = building->y; j < building->y + height; j++)
		{
			Building* neighbour = GetBuilding(spreadDirection & 2 ? x1 : x2, j);

			if (neighbour && !neighbour->onFire && neighbour->type != Park && !IsRubble(neighbour->type))
			{
				neighbour->onFire = 1;
				RefreshBuildingTiles(neighbour);
				return true;
			}
		}
	}
	else
	{
		for (uint8_t i = building->x; i < building->x + width; i++)
		{
			Building* neighbour = GetBuilding(i, spreadDirection & 2 ? y1 : y2);

			if (neighbour && !neighbour->onFire && neighbour->type != Park && !IsRubble(neighbour->type))
			{
				neighbour->onFire = 1;
				RefreshBuildingTiles(neighbour);
				return true;
			}
		}
	}

	return false;
}

void SimulateBuilding(Building* building)
{
	int8_t populationDensityChange = 0;

	if (building->onFire)
	{
		if (IsRubble(building->type))
		{
			building->onFire--;
			if ((GetRand() & 0xff) > SIM_FIRE_SPREAD_CHANCE)
			{
				SpreadFire(building);
			}
			RefreshBuildingTiles(building);
			return;
		}

		// Find closest fire department
		uint8_t closestFireDept = 0xff;

		for (int n = 0; n < MAX_BUILDINGS; n++)
		{
			Building* otherBuilding = &State.buildings[n];

			if (otherBuilding->type == FireDept && otherBuilding->hasPower)
			{
				uint8_t distance = GetManhattanDistance(building, otherBuilding);

				if (distance < closestFireDept)
				{
					closestFireDept = distance;
				}
			}
		}

		int fireDeptInfluence = SIM_FIRE_DEPT_BASE_INFLUENCE + closestFireDept * SIM_FIRE_DEPT_INFLUENCE_MULTIPLIER;
		
		if (fireDeptInfluence <= 255 && (GetRand() & 0xff) > (uint8_t)(fireDeptInfluence))
		{
			building->onFire--;
		}
		else if ((GetRand() & 0xff) > SIM_FIRE_SPREAD_CHANCE || !SpreadFire(building))
		{
			if ((GetRand() & 0xff) < SIM_FIRE_BURN_CHANCE)
			{
				if (building->onFire >= BUILDING_MAX_FIRE_COUNTER)
				{
					DestroyBuilding(building);
					building->onFire = BUILDING_MAX_FIRE_COUNTER;
				}
				else
				{
					building->onFire++;
				}
			}
		}
		building->heavyTraffic = false;
	}
	else if (building->type == Residential || building->type == Commercial || building->type == Industrial)
	{
		if (building->hasPower)
		{
			int score = 0;
			
			// random effect
			int randomEffect = (GetRand() & SIM_RANDOM_STRENGTH_MASK) - (SIM_RANDOM_STRENGTH_MASK / 2);
			score += randomEffect;
			
			// tend towards average population density
			score += (AVERAGE_POPULATION_DENSITY - building->populationDensity) * SIM_AVERAGING_STRENGTH;
			
			// tax rate effect
			score -= (State.taxRate - SIM_IDEAL_TAX_RATE) * SIM_TAX_RATE_PENALTY;

			// general population effect
			int populationEffect = 0;
			switch(building->type)
			{
				case Residential:
				if(State.residentialPopulation < State.industrialPopulation)
				{
					populationEffect += SIM_EMPLOYMENT_BOOST;
				}
				else if(State.residentialPopulation > State.industrialPopulation + State.commercialPopulation)
				{
					populationEffect -= SIM_UNEMPLOYMENT_PENALTY;
				}
				break;
				case Industrial:
				if(State.industrialPopulation < State.residentialPopulation || State.industrialPopulation < State.commercialPopulation)
				{
					populationEffect += SIM_INDUSTRIAL_OPPORTUNITY_BOOST;
				}
				break;
				case Commercial:
				if(State.commercialPopulation < State.residentialPopulation || State.commercialPopulation < State.industrialPopulation)
				{
					populationEffect += SIM_COMMERCIAL_OPPORTUNITY_BOOST;
				}
				break;
			}
			score += populationEffect;
			
			// If at least 3 road tiles are adjacent then assume that it is connected to the road network
			bool isRoadConnected = GetNumRoadConnections(building) >= 3;
			
			uint8_t closestPoliceStationDistance = 24;
			int16_t pollution = 0;
			int16_t localInfluence = 0;
			
			// influence from local buildings
			if(isRoadConnected)
			{
				if (building->populationDensity == 0)
				{
					score += SIM_BASE_SCORE;
				}

				for(int n = 0; n < MAX_BUILDINGS; n++)
				{
					Building* otherBuilding = &State.buildings[n];
					
					if(building != otherBuilding && otherBuilding->type && (otherBuilding->hasPower || otherBuilding->type == Park) && !otherBuilding->onFire)
					{
						uint8_t distance = GetManhattanDistance(building, otherBuilding);
						
						if(otherBuilding->type == PoliceDept && distance < closestPoliceStationDistance)
						{
							closestPoliceStationDistance = distance;
						}
						
						int buildingPollution = 0;
						
						if(otherBuilding->type == Industrial)
						{
							buildingPollution = SIM_INDUSTRIAL_BASE_POLLUTION + otherBuilding->populationDensity - distance;
						}
						else if(otherBuilding->type == Powerplant)
						{
							buildingPollution = SIM_POWERPLANT_BASE_POLLUTION - distance;
						}
						else if(otherBuilding->heavyTraffic)
						{
							buildingPollution = SIM_TRAFFIC_BASE_POLLUTION - distance;
						}
						
						if(buildingPollution > 0)
							pollution += buildingPollution;
						
						if(distance <= SIM_LOCAL_BUILDING_DISTANCE && GetNumRoadConnections(otherBuilding) >= 3)
						{
							switch(otherBuilding->type)
							{
								case Industrial:
								if(otherBuilding->populationDensity >= building->populationDensity && building->type == Residential)
								{
									localInfluence += SIM_LOCAL_BUILDING_INFLUENCE;
								}
								else if (otherBuilding->populationDensity > building->populationDensity && building->type == Commercial)
								{
									localInfluence += SIM_LOCAL_BUILDING_INFLUENCE;
								}
								break;
								case Residential:
								if(otherBuilding->populationDensity > building->populationDensity && (building->type == Commercial || building->type == Industrial))
								{
									localInfluence += SIM_LOCAL_BUILDING_INFLUENCE;
								}
								break;
								case Commercial:
								if(otherBuilding->populationDensity >= building->populationDensity && building->type == Residential)
								{
									localInfluence += SIM_LOCAL_BUILDING_INFLUENCE;
								}
								break;
								case Stadium:
								if(building->type == Residential || building->type == Commercial)
								{
									localInfluence += SIM_STADIUM_BOOST;
								}
								break;
								case Park:
								if(building->type == Residential)
								{
									localInfluence += SIM_PARK_BOOST;
								}
								break;
								default:
								break;
							}
						}
					}
				}
			}

			score += localInfluence;
			
			// negative effect from pollution
			if (building->type == Residential)
			{
				if (pollution > SIM_MAX_POLLUTION)
					pollution = SIM_MAX_POLLUTION;
				score -= pollution * SIM_POLLUTION_INFLUENCE;
#if _WIN32
//				printf("Pollution: %d\n", pollution * SIM_POLLUTION_INFLUENCE);
#endif
			}
			
			// simulate crime based on how far the closest police station is and how populated the area is
			int crime = (building->populationDensity * (closestPoliceStationDistance - 16));
			if(crime > SIM_MAX_CRIME)
			{
				crime = SIM_MAX_CRIME;
			}
			else if (crime < 0)
			{
				crime = 0;
			}

			score -= crime;

			DebugBuildingScore(building, score, crime, pollution * SIM_POLLUTION_INFLUENCE, localInfluence, populationEffect, randomEffect);
			
			// increase or decrease population density based on score
			if (building->populationDensity < MAX_POPULATION_DENSITY && score >= SIM_INCREMENT_POP_THRESHOLD)
			{
				populationDensityChange = 1;
			}
			else if(building->populationDensity > 0 && score <= SIM_DECREMENT_POP_THRESHOLD)
			{
				populationDensityChange = -1;
			}
			
			building->heavyTraffic = building->populationDensity > SIM_HEAVY_TRAFFIC_THRESHOLD;
		}
		else
		{
			building->heavyTraffic = false;
			if (building->populationDensity > 0)
			{
				populationDensityChange = -1;
			}
		}
	}

	building->populationDensity += populationDensityChange;
	switch (building->type)
	{
	case Residential:
		State.residentialPopulation += populationDensityChange;
		break;
	case Industrial:
		State.industrialPopulation += populationDensityChange;
		break;
	case Commercial:
		State.commercialPopulation += populationDensityChange;
		break;
	}

	RefreshBuildingTiles(building);
}

void CountPopulation()
{
	State.residentialPopulation = State.industrialPopulation = State.commercialPopulation = 0;

	for (int n = 0; n < MAX_BUILDINGS; n++)
	{
		switch (State.buildings[n].type)
		{
		case Residential:
			State.residentialPopulation += State.buildings[n].populationDensity;
			break;
		case Industrial:
			State.industrialPopulation += State.buildings[n].populationDensity;
			break;
		case Commercial:
			State.commercialPopulation += State.buildings[n].populationDensity;
			break;
		default:
			break;
		}
	}
}

void CheckScenarioWinLose()	// only called once after December of each year (but before year is incremented)
{
	uint8_t scenario=(State.data[0] >> 2);
	if(scenario)
	{
		if(ScenarioData[scenario].goalyear>0 && (ScenarioData[scenario].goalyear-1900) == State.year)
		{
			bool won=true;
			if(ScenarioData[scenario].goalfunds>State.money)
			{
				won=false;
			}
			if(ScenarioData[scenario].goalrespop>State.residentialPopulation)
			{
				won=false;
			}
			if(ScenarioData[scenario].goalcompop>State.commercialPopulation)
			{
				won=false;
			}
			if(ScenarioData[scenario].goalindpop>State.industrialPopulation)
			{
				won=false;
			}

			for(int i=0; i<SCENARIO_GOAL_BUILDING_COUNT; i++)
			{
				if(ScenarioData[scenario].goalbuilding[i]!=BuildingType_None && ScenarioData[scenario].goalbuildingcount[i]>0)
				{
					int count=0;
					for(int i=0; i<MAX_BUILDINGS; i++)
					{
						if(State.buildings[i].type==ScenarioData[scenario].goalbuilding[i])
						{
							count++;
						}
					}
					if(ScenarioData[scenario].goalbuildingcount[i]>count)
					{
						won=false;
					}
				}
			}
		
			UIState.selection=0;
			if(won==true)
			{
				State.data[0]|=1<<1;
				UIState.state=ScenarioWinScreen;
			}
			else
			{
				State.data[0]|=1;
				UIState.state=ScenarioLoseScreen;
			}
		}
	}
}

void Simulate()
{
	if((State.flags & FLAG_PAUSE) != FLAG_PAUSE)
	{
		if (State.simulationStep < MAX_BUILDINGS)
		{
			SimulateBuilding(&State.buildings[State.simulationStep]);
		}
		else switch (State.simulationStep)
		{
		case SimulatePower:
			CalculatePowerConnectivity();
			break;
		case SimulatePopulation:
			CountPopulation();
			break;
		case SimulateFastNextMonth:
		case SimulateNextMonth:
			if(State.simulationStep==SimulateNextMonth || ((State.flags & FLAG_FAST) == FLAG_FAST && State.simulationStep==SimulateFastNextMonth))
			{
				DoMonthEndBudget();

				State.simulationStep = 0;
				State.month++;
				if (State.month >= 12)
				{
					DoYearEndBudget();

					CheckScenarioWinLose();		// do this after budget so budget screen won't pop up

					State.month = 0;
					State.year++;
				}
				return;
			}
		}

		State.simulationStep++;
		State.timeToNextDisaster--;

		if (State.timeToNextDisaster == 0)
		{
			StartRandomFire();
			State.timeToNextDisaster = (GetRand() % (MAX_TIME_BETWEEN_DISASTERS - MIN_TIME_BETWEEN_DISASTERS)) + MIN_TIME_BETWEEN_DISASTERS;
		}
	}
}

bool StartRandomFire()
{
	int attemptsLeft = MAX_BUILDINGS;

	while (attemptsLeft)
	{
		int index = GetRand() & 0xff;
		if (index < MAX_BUILDINGS && State.buildings[index].type && !State.buildings[index].onFire && !IsRubble(State.buildings[index].type) && State.buildings[index].type != Park)
		{
			State.buildings[index].onFire = 1;
			RefreshBuildingTiles(&State.buildings[index]);
			FocusTile(State.buildings[index].x + 1, State.buildings[index].y + 1);

			UIState.state = InGameDisaster;
			UIState.selection = DISASTER_MESSAGE_DISPLAY_TIME;
			return true;
		}
		attemptsLeft--;
	}

	return false;
}
