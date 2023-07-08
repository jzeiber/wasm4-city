#include "Game.h"
#include "Interface.h"
#include "Draw.h"
#include "global.h"
#include "exportcityppm.h"
#include "scenario.h"

UIStateStruct UIState;

static uint8_t LastInput = 0;
static uint8_t InputRepeatCounter = 0;

void UpdateInterface()
{
	// Scroll screen to center the selected tile
	int targetX = UIState.selectX * 8 + TILE_SIZE / 2 - DISPLAY_WIDTH / 2;
	int targetY = UIState.selectY * 8 + TILE_SIZE / 2 - DISPLAY_HEIGHT / 2;

	if(UIState.state == StartScreen)
	{
		if(global::ticks%2==0)
		{
			if(UIState.scrollX != targetX)
			{
				UIState.scrollX += (targetX < UIState.scrollX ? -1 : 1);
			}
			if(UIState.scrollY != targetY)
			{
				UIState.scrollY += (targetY < UIState.scrollY ? -1 : 1);
			}
		}
	}
	else
	{
		int diffX = targetX - UIState.scrollX;
		UIState.scrollX = targetX - (diffX / 2);
		int diffY = targetY - UIState.scrollY;
		UIState.scrollY = targetY - (diffY / 2);
	}

	/*if(UIState.scrollX < targetX)
	UIState.scrollX ++;
	else if(UIState.scrollX > targetX)
	UIState.scrollX --;
	if(UIState.scrollY < targetY)
	UIState.scrollY++;
	else if(UIState.scrollY > targetY)
	UIState.scrollY --;*/
}

void GetBuildingBrushLocation(BuildingType buildingType, uint8_t* outX, uint8_t* outY)
{
	const BuildingInfo* buildingInfo = GetBuildingInfo(buildingType);
	uint8_t width = buildingInfo->width;
	uint8_t height = buildingInfo->height;

	if (UIState.selectX > 0)
	{
		if (UIState.selectX - 1 > MAP_WIDTH - width)
		{
			*outX = MAP_WIDTH - width;
		}
		else
		{
			*outX = UIState.selectX - 1;
		}
	}
	else
	{
		*outX = 0;
	}

	if (UIState.selectY > 0)
	{
		if (UIState.selectY - 1 > MAP_HEIGHT - height)
		{
			*outY = MAP_HEIGHT - height;
		}
		else
		{
			*outY = UIState.selectY - 1;
		}
	}
	else
	{
		*outY = 0;
	}
}

void WrapMenuInput(uint8_t input, uint8_t numOptions)
{
	if (input & INPUT_UP)
	{
		if (UIState.selection > 0)
			UIState.selection--;
		else UIState.selection = numOptions - 1;
	}
	if (input & INPUT_DOWN)
	{
		if (UIState.selection == numOptions - 1)
			UIState.selection = 0;
		else UIState.selection++;
	}
}

void HandleMovementInput(uint8_t input)
{
	if ((input & INPUT_LEFT) && UIState.selectX > 0)
	{
		UIState.selectX--;
	}
	if ((input & INPUT_UP) && UIState.selectY > 0)
	{
		UIState.selectY--;
	}
	if ((input & INPUT_RIGHT) && UIState.selectX < MAP_WIDTH - 1)
	{
		UIState.selectX++;
	}
	if ((input & INPUT_DOWN) && UIState.selectY < MAP_HEIGHT - 1)
	{
		UIState.selectY++;
	}
}

void HandleInput(uint8_t input)
{
	if (UIState.state == ShowingToolbar)
	{
		if (input & INPUT_LEFT)
		{
			if (UIState.selection == 0)
			{
				UIState.selection = NUM_TOOLBAR_BUTTONS - 1;
			}
			else UIState.selection--;
		}
		if (input & INPUT_RIGHT)
		{
			if (UIState.selection == NUM_TOOLBAR_BUTTONS - 1)
			{
				UIState.selection = 0;
			}
			else UIState.selection++;
		}
		if (input & (INPUT_A | INPUT_B))
		{
			if (UIState.selection <= LastBuildingBrush)
			{
				UIState.brush = UIState.selection;
				UIState.state = InGame;
			}
			else if (UIState.selection == SaveLoadToolbarButton)
			{
				UIState.state = SaveLoadMenu;
				UIState.selection = 0;
			}
			else if (UIState.selection == BudgetToolbarButton)
			{
				UIState.state = BudgetMenu;
				UIState.selection = MIN_BUDGET_DISPLAY_TIME;
			}
			else if (UIState.selection == DemographicsToolbarButton)
			{
				UIState.state = DemographicsMenu;
				UIState.selection = 0;
			}
			else if (UIState.selection == MapToolbarButton)
			{
				UIState.state = MapMenu;
				UIState.selection = 0;
			}
			else if (UIState.selection == PauseGoToolbarButton)
			{
				State.flags^=FLAG_PAUSE;
			}
		}
		if(input & (INPUT_UP | INPUT_DOWN))
		{
			if(UIState.selection == PauseGoToolbarButton)
			{
				State.flags^=FLAG_FAST;
			}
		}
	}
	else if (UIState.state == InGameDisaster)
	{
		HandleMovementInput(input);
	}
	else if (UIState.state == StartScreen)
	{
		WrapMenuInput(input, 2);
		if (input & (INPUT_B))
		{
			switch (UIState.selection)
			{
			case 0:
				InitGame();
				UIState.state = NewCityMenu;
				UIState.selection = 0;
				State.terrainType = ScenarioData[UIState.selection].mapidx;
				break;
			case 1:
				if (LoadCity())
				{
					if(State.terrainType==(NUM_TERRAIN_TYPES-1))
					{
						GenerateRandomTerrain(State.terrainType,State.seed);
					}
					UIState.state = InGame;
					ResetVisibleTileCache();
				}
				break;
			}
		}
	}
	else if (UIState.state == NewCityMenu)
	{
		if (input & INPUT_LEFT)
		{
			if(UIState.selection == 0)
			{
				UIState.selection = SCENARIO_COUNT - 1;
			}
			else
			{
				UIState.selection--;
			}
			if(ScenarioData[UIState.selection].stateptr)
			{
				LoadStaticCity(ScenarioData[UIState.selection].stateptr);
			}
			else
			{
				InitGame();
			}
			State.terrainType = ScenarioData[UIState.selection].mapidx;
			if(State.terrainType==NUM_TERRAIN_TYPES-1)
			{
				State.seed=global::ticks;
				GenerateRandomTerrain(State.terrainType,State.seed);
			}
		}
		if (input & INPUT_RIGHT)
		{
			if(UIState.selection == SCENARIO_COUNT - 1)
			{
				UIState.selection = 0;
			}
			else
			{
				UIState.selection++;
			}
			if(ScenarioData[UIState.selection].stateptr)
			{
				LoadStaticCity(ScenarioData[UIState.selection].stateptr);
			}
			else
			{
				InitGame();
			}
			State.terrainType=ScenarioData[UIState.selection].mapidx;
			if(State.terrainType==NUM_TERRAIN_TYPES-1)
			{
				State.seed=global::ticks;
				GenerateRandomTerrain(State.terrainType,State.seed);
			}
		}
		if (input & (INPUT_B))
		{
			uint8_t terrainType = State.terrainType;
			uint32_t seed = State.seed;
			const uint8_t scenario = UIState.selection;
			InitGame();
			if(ScenarioData[scenario].flags & SCENARIO_FLAG_SCENARIO)
			{
				LoadStaticCity(ScenarioData[scenario].stateptr);
				State.data[0] = scenario << 2;
				State.year=ScenarioData[scenario].startyear-1900;
				State.month=0;
				State.money=ScenarioData[scenario].startfunds;
				State.taxesCollected=0;
				State.taxRate=7;
				State.accumulatedMonthlyTaxes=0;
				State.flags&=~FLAG_FAST;
				State.flags&=~FLAG_PAUSE;
				State.flags&=~FLAG_MAP_SHOWBUILDINGS;
				State.flags&=~FLAG_MAP_SHOWFDRANGE;
				State.flags&=~FLAG_MAP_SHOWROADS;
				State.flags&=~FLAG_MAP_SHOWELECTRIC;
			}
			State.terrainType = terrainType;
			State.seed = seed;
			ResetVisibleTileCache();
			UIState.state = ShowingToolbar;
			UIState.selection = 0;
		}
	}
	else if (UIState.state == SaveLoadMenu)
	{
		WrapMenuInput(input, 5);
		if (input & (INPUT_A))
		{
			UIState.state = ShowingToolbar;
			UIState.selection = SaveLoadToolbarButton;
		}
		if (input & INPUT_B)
		{
			switch (UIState.selection)
			{
			case 0:
				SaveCity();
				UIState.state = InGame;
				break;
			case 1:
				if (LoadCity())
				{
					if(State.terrainType==(NUM_TERRAIN_TYPES-1))
					{
						GenerateRandomTerrain(State.terrainType,State.seed);
					}
					UIState.state = InGame;
					ResetVisibleTileCache();
				}
				break;
			case 2:
				InitGame();
				UIState.state = NewCityMenu;
				UIState.selection = 0;
				State.terrainType = ScenarioData[UIState.selection].mapidx;
				break;
			case 3:
				UIState.autoBudget = !UIState.autoBudget;
				break;
			case 4:
				ExportCityPPM();
				break;
			}
		}
	}
	else if (UIState.state == InGame)
	{
		HandleMovementInput(input);

		if (input & INPUT_A)
		{
			UIState.state = ShowingToolbar;
			UIState.selection = UIState.brush;
		}

		if (input & INPUT_B)
		{
			if (UIState.brush == Bulldozer)
			{
				Building* building = GetBuilding(UIState.selectX, UIState.selectY);
				if (building && !IsRubble(building->type))
				{
					const BuildingInfo* buildingInfo = GetBuildingInfo(building->type);
					uint8_t width = buildingInfo->width;
					uint8_t height = buildingInfo->height;
					int cost = width * height * BULLDOZER_COST;

					if (State.money >= cost)
					{
						State.money -= cost;

						DestroyBuilding(building);
					}
					else
					{
						// TODO: not enough cash
					}
				}
				else
				{
					if (GetConnections(UIState.selectX, UIState.selectY))
					{
						if (State.money >= BULLDOZER_COST)
						{
							State.money -= BULLDOZER_COST;

							SetConnections(UIState.selectX, UIState.selectY, 0);
							RefreshTileAndConnectedNeighbours(UIState.selectX, UIState.selectY);
							SetTile(UIState.selectX, UIState.selectY, RUBBLE_TILE);
						}
						else
						{
							// TODO: not enough cash
						}
					}
					else
					{
						// TODO: nothing to bulldoze
					}
				}
			}
			else if (UIState.brush < FirstBuildingBrush)
			{
				// Is powerline or road

				Building* building = GetBuilding(UIState.selectX, UIState.selectY);
				if (building && !IsRubble(building->type))
				{
					// TODO: can't build here
				}
				else
				{
					int cost = UIState.brush == RoadBrush ? ROAD_COST : POWERLINE_COST;
					uint8_t mask = UIState.brush == RoadBrush ? RoadMask : PowerlineMask;
					uint8_t currentConnections = GetConnections(UIState.selectX, UIState.selectY);
					bool onGround =  IsTerrainClear(UIState.selectX, UIState.selectY);
					
					if(onGround || (currentConnections == 0 && IsSuitableForBridgedTile(UIState.selectX, UIState.selectY, mask)))
					{
						if ((currentConnections & mask) == 0)
						{
							if (State.money >= cost)
							{
								State.money -= cost;
								SetConnections(UIState.selectX, UIState.selectY, currentConnections | mask);

								// Remove rubble
								if (building)
								{
									building->type = 0;
								}

								RefreshTileAndConnectedNeighbours(UIState.selectX, UIState.selectY);
							}
							else
							{
								// TODO: not enough cash
							}
						}
					}
				}
			}
			else
			{
				// Is building placement
				BuildingType buildingType = (BuildingType)(UIState.brush - FirstBuildingBrush + 1);
				const BuildingInfo* buildingInfo = GetBuildingInfo(buildingType);
				uint8_t placeX, placeY;
				GetBuildingBrushLocation(buildingType, &placeX, &placeY);
				uint16_t cost = buildingInfo->cost;

				if (CanPlaceBuilding(buildingType, placeX, placeY))
				{
					if (State.money >= cost)
					{
						if (PlaceBuilding(buildingType, placeX, placeY))
						{
							State.money -= cost;
						}
						else
						{
							// too many buildings
						}
					}
					else
					{
						// TODO: not enough cash
					}
				}
				else
				{
					// TODO: cannot place here, e.g. obstructed
				}
			}
		}
	}
	else if (UIState.state == BudgetMenu)
	{
		// Display for a minimum of a few frames to prevent closing the budget menu by accident
		if (UIState.selection >= MIN_BUDGET_DISPLAY_TIME)
		{
			if ((input & INPUT_LEFT) && State.taxRate > 0)
			{
				State.taxRate--;
			}
			if ((input & INPUT_RIGHT) && State.taxRate < 99)
			{
				State.taxRate++;
			}
			if (input & (INPUT_A | INPUT_B))
			{
				UIState.state = ShowingToolbar;
				UIState.selection = BudgetToolbarButton;
			}
		}
	}
	else if (UIState.state == ScenarioWinScreen || UIState.state == ScenarioLoseScreen)
	{
		if(input & INPUT_UP)
		{
			if(UIState.selection==0)
			{
				UIState.selection=1;
			}
			else
			{
				UIState.selection--;
			}
		}
		if(input & INPUT_DOWN)
		{
			if(UIState.selection==1)
			{
				UIState.selection=0;
			}
			else
			{
				UIState.selection++;
			}
		}
		if (input & INPUT_B)
		{
			if(UIState.selection==0)
			{
				// TODO - reset scenario data in State
				UIState.state=ShowingToolbar;
				UIState.selection=0;
			}
			if(UIState.selection==1)
			{
				UIState.state=NewCityMenu;
				UIState.selection=0;
				State.terrainType = ScenarioData[UIState.selection].mapidx;
			}
		}
	}
	else if (UIState.state == DemographicsMenu)
	{
		if(input & INPUT_LEFT)
		{
			if(UIState.selection==0)
			{
				UIState.selection=1;
			}
			else
			{
				UIState.selection--;
			}
		}
		if(input & INPUT_RIGHT)
		{
			if(UIState.selection==1)
			{
				UIState.selection=0;
			}
			else
			{
				UIState.selection++;
			}
		}
		if(input & (INPUT_A | INPUT_B))
		{
			UIState.state = ShowingToolbar;
			UIState.selection = DemographicsToolbarButton;
		}
	}
	else if (UIState.state == MapMenu)
	{
		if(input & (INPUT_A))
		{
			UIState.state = ShowingToolbar;
			UIState.selection = MapToolbarButton;
		}
		else if(input & (INPUT_B))
		{
			switch(UIState.selection)
			{
			case 0:
				State.flags^=FLAG_MAP_SHOWBUILDINGS;
				break;
			case 1:
				State.flags^=FLAG_MAP_SHOWROADS;
				break;
			case 2:
				State.flags^=FLAG_MAP_SHOWELECTRIC;
				break;
			case 3:
				State.flags^=FLAG_MAP_SHOWFDRANGE;
				break;
			}
		}
		else if(input & (INPUT_DOWN | INPUT_RIGHT))
		{
			UIState.selection++;
			if(UIState.selection>3)
			{
				UIState.selection=0;
			}
		}
		else if(input & (INPUT_UP | INPUT_LEFT))
		{
			if(UIState.selection==0)
			{
				UIState.selection=3;
			}
			else
			{
				UIState.selection--;
			}
		}
	}
}

void ProcessInput()
{
	uint8_t input = GetInput();

	if (input != LastInput)
	{
		InputRepeatCounter = 0;

		uint8_t newInput = (LastInput ^ input) & input;

		if (newInput)
		{
			HandleInput(input);
		}
	}
	else
	{
		InputRepeatCounter++;
		if (InputRepeatCounter > INPUT_REPEAT_TIME)
		{
			HandleInput(LastInput);
			InputRepeatCounter -= INPUT_REPEAT_FREQUENCY;
		}
	}

	LastInput = input;
}
