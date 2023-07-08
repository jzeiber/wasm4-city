#include "Draw.h"
#include "Interface.h"
#include "Game.h"
#include "Simulation.h"

#include "wasm4.h"
#include "wasmmalloc.h"
#include "wasmmemcpy.h"
#include "global.h"

#include "democity.h"

#include "printf.h"

uint8_t GetInput()
{
  uint8_t result = 0;

  if((*GAMEPAD1 & BUTTON_1) == BUTTON_1)
  {
    result|=INPUT_A;
  }
  if((*GAMEPAD1 & BUTTON_2) == BUTTON_2)
  {
    result|=INPUT_B;
  }
  if((*GAMEPAD1 & BUTTON_UP) == BUTTON_UP)
  {
    result|=INPUT_UP;
  }
  if((*GAMEPAD1 & BUTTON_RIGHT) == BUTTON_RIGHT)
  {
    result|=INPUT_RIGHT;
  }
  if((*GAMEPAD1 & BUTTON_DOWN) == BUTTON_DOWN)
  {
    result|=INPUT_DOWN;
  }
  if((*GAMEPAD1 & BUTTON_LEFT) == BUTTON_LEFT)
  {
    result|=INPUT_LEFT;
  }

  return result;
}

void PutPixel(int32_t x, int32_t y, uint8_t color)
{
  *DRAW_COLORS=color;
  line(x,y,x,y);
}

void DrawBitmap(const uint8_t* bmp, int32_t x, int32_t y, int32_t w, int32_t h, uint8_t fg, uint8_t bg)
{
  *DRAW_COLORS = (bg << 4) | fg;
  blit(bmp,x,y,w,h,BLIT_1BPP);
}

/*
  TODO - save/load - don't need to save building heavyTraffic or hasPower (they can be calculated when loaded)
  First sort buildings so that pos index ((y*mapwidth) + x) is in order
  Also change storing x,y pos to offset array index pos.  Can use building type 0 when needed to adjust current x,y pos

  0   - 4 bits for building type 0      (building type 0 will only use 16 bits)
  x   - 6 bits for x pos
  y   - 6 bits for y pos
  #   - 4 bits for real building type   (then add as many real buildings as needed with their data) (real building can fit in 16 bits)
  off - 6 bits for offset from previous building x,y (wrap around on map width)
  d   - 4 bits for population density
  f   - 2 bits for on fire
*/

template<typename T>
void WriteVal(uint8_t *buff, int32_t &pos, T val)
{
  for(int32_t shift=(sizeof(T)-1)*8; shift>=0; shift-=8)
  {
    buff[pos]=((val >> shift) & 0xff);
    pos++;
  }
}

template<typename T>
void ReadVal(const uint8_t *buff, int32_t &pos, T &val)
{
  val=0;
  for(int32_t shift=(sizeof(T)-1)*8; shift>=0; shift-=8)
  {
    val|=(static_cast<T>(buff[pos]) << shift);
    pos++;
  }
}

void SortBuildingsByPosIndex(GameState &state)
{
  // simple bubble sort
  for(int i=1; i<MAX_BUILDINGS; i++)
  {
    for(int j=0; j<MAX_BUILDINGS-i; j++)
    {
      bool swap=false;
      if(state.buildings[j+1].type!=BuildingType_None && state.buildings[j].type==BuildingType_None)
      {
          swap=true;
      }
      else if(state.buildings[j].type!=BuildingType_None && state.buildings[j+1].type!=BuildingType_None)
      {
        const int32_t pos1=((state.buildings[j].y * MAP_WIDTH) + state.buildings[j].x);
        const int32_t pos2=((state.buildings[j+1].y * MAP_WIDTH) + state.buildings[j+1].x);
        if(pos2<pos1)
        {
          swap=true;
        }
      }
      if(swap==true)
      {
        Building b=state.buildings[j];
        state.buildings[j]=state.buildings[j+1];
        state.buildings[j+1]=b;
      }
    }
  }
}

typedef struct
{
  uint16_t type : 4;
  uint16_t x : 6;
  uint16_t y : 6;
} SaveAbsolutePos;

typedef struct
{
  uint8_t type : 4;
  uint8_t populationDensity : 4;
  uint8_t onFire : 2;
  uint8_t posoffset : 6;
} SaveBuilding;

// buffer must be 1024 bytes
int32_t SaveCityToBuffer(GameState &state, uint8_t *buff, const bool withheader)
{
  int32_t pos=0;

  if(withheader==true)
  {
    buff[pos++]='C';
    buff[pos++]='T';
    buff[pos++]='Y';
    buff[pos++]='3';
  }

  const size_t len=(uint8_t *)&(state.buildings)-(uint8_t *)&(state.year);
  memcpy((void *)&buff[pos],(void *)&(state.year),len);
  pos+=len;

  // save buildings
  // first sort buildings by pos index ((y*mapwidth)+x)
  // empty building slots are now at end of array, so first empty building we get to we can stop
  SortBuildingsByPosIndex(state);
  int32_t lastposidx=0;
  int32_t bi=0;
  for(int i=0; i<MAX_BUILDINGS; i++)
  {
    if(state.buildings[bi].type!=BuildingType_None)
    {
      const int32_t posidx=((state.buildings[bi].y * MAP_WIDTH) + state.buildings[bi].x);
      int32_t posdiff=posidx-lastposidx;
      if(posdiff>63)
      {
        // must insert absolute position
        SaveAbsolutePos ap;
        ap.type=BuildingType_None;
        ap.x=state.buildings[bi].x;
        ap.y=state.buildings[bi].y;
        memcpy((void *)&buff[pos],(void *)&ap,2);
        pos+=2;
        posdiff=0;
      }
      SaveBuilding sb;
      sb.type=state.buildings[bi].type;
      sb.populationDensity=state.buildings[bi].populationDensity;
      sb.onFire=state.buildings[bi].onFire;
      sb.posoffset=static_cast<uint8_t>(posdiff);
      memcpy((void *)&buff[pos],(void *)&sb,2);
      pos+=2;

      lastposidx=posidx;
      bi++;

    }
    else
    {
      uint16_t val=~0;        // 0xffff signifies end of building list
      WriteVal(buff,pos,val);
      i=MAX_BUILDINGS;        // only empty building slots remaining so skip writing them
    }
  }

  return pos;
}

bool LoadCityFromBuffer(GameState &state, const uint8_t *buff, const bool withheader)
{
  int32_t pos=0;

  if(withheader==true)
  {
    if(buff[pos++]!='C' || buff[pos++]!='T' || buff[pos++]!='Y' || buff[pos++]!='3')
    {
      return false;
    }
  }

  const size_t len=(uint8_t *)&(state.buildings)-(uint8_t *)&(state.year);
  memcpy((void *)&(state.year),(void *)&buff[pos],len);
  pos+=len;

  // load buildings
  constexpr uint16_t endval=~0;   // end of buildings (either we get this marker, or we read the max number of buildings)
  int32_t lastx=0;
  int32_t lasty=0;
  for(int i=0; i<MAX_BUILDINGS; i++)
  {
    uint16_t val=0;
    ReadVal(buff,pos,val);

    if(val!=endval)
    {
      SaveBuilding sb;
      memcpy((void *)&sb,(void *)&buff[pos-2],2);

      if(sb.type==BuildingType_None)
      {
        SaveAbsolutePos sp;
        memcpy((void *)&sp,(void *)&buff[pos-2],2);
        lastx=sp.x;
        lasty=sp.y;

        // read the next val for the actual building data
        memcpy((void *)&sb,(void *)&buff[pos],2);
        pos+=2;
      }

      state.buildings[i].type=sb.type;
      state.buildings[i].populationDensity=sb.populationDensity;
      state.buildings[i].onFire=sb.onFire;

      lastx+=sb.posoffset;
      while(lastx>MAP_WIDTH)
      {
        lastx-=MAP_WIDTH;
        lasty++;
      }

      state.buildings[i].x=lastx;
      state.buildings[i].y=lasty;

    }
    else    // populate the remaining building slots with none
    {
      for(i; i<MAX_BUILDINGS; i++)
      {
        state.buildings[i].type=BuildingType_None;
      }
    }
  }

  return true;
}

void SaveCity()
{
  trace("Saving City");
  uint8_t *buffer=new uint8_t[1024];
  if(buffer!=nullptr)
  {
    int32_t savelen=SaveCityToBuffer(State,buffer,true);
    
    char buff[32];
    buff[31]='\0';
    snprintf(buff,31,"saved %i bytes",savelen);
    trace(buff);

    // debug - output bytes of game state
    {
      trace("game data ={");
      char cbuff[64];
      int pos=0;
      for(;pos+8<savelen;pos+=8)
      {
        int r=snprintf(cbuff,63,"0x%.2hhx,0x%.2hhx,0x%.2hhx,0x%.2hhx,0x%.2hhx,0x%.2hhx,0x%.2hhx,0x%.2hhx,",buffer[pos],buffer[pos+1],buffer[pos+2],buffer[pos+3],buffer[pos+4],buffer[pos+5],buffer[pos+6],buffer[pos+7]);
        cbuff[r]='\0';
        trace(cbuff);
      }
      while(pos<savelen)
      {
        int r=snprintf(cbuff,63,"0x%.2hhx,",buffer[pos]);
        cbuff[r]='\0';
        trace(cbuff);
        pos++;
      }
      trace("};");
    }
    
    diskw(buffer,1024);
    delete [] buffer;
  }
}

/*
void SaveCityOrig()
{
  size_t len=4+sizeof(GameState);
  uint8_t *buffer=new uint8_t[len];
  if(buffer!=nullptr)
  {
    buffer[0]='C';
    buffer[1]='T';
    buffer[2]='Y';
    buffer[3]='2';
    memcpy((void *)&buffer[4],(void *)&State,sizeof(GameState));

    diskw(buffer,len);
    delete [] buffer;
  }

}
*/

bool LoadStaticCity(const uint8_t *citydata)
{
  if(LoadCityFromBuffer(State,citydata,false)==true)
  {
    CalculatePowerConnectivity();
    for(int i=0; i<MAX_BUILDINGS; i++)
    {
      if(State.buildings[i].type!=BuildingType_None && !State.buildings[i].onFire && State.buildings[i].hasPower)
      {
        State.buildings[i].heavyTraffic = State.buildings[i].populationDensity > SIM_HEAVY_TRAFFIC_THRESHOLD;
      }
    }
  }
  else
  {
    return false;
  }
  return true;
}

bool LoadCity()
{
  bool loaded=true;
  uint8_t *buffer=new uint8_t[1024];
  if(buffer!=nullptr)
  {
    diskr(buffer,1024);
    if(LoadCityFromBuffer(State,buffer,true)==true)
    {
      CalculatePowerConnectivity();
      for(int i=0; i<MAX_BUILDINGS; i++)
      {
        if(State.buildings[i].type!=BuildingType_None && !State.buildings[i].onFire && State.buildings[i].hasPower)
        {
          State.buildings[i].heavyTraffic = State.buildings[i].populationDensity > SIM_HEAVY_TRAFFIC_THRESHOLD;
        }
      }
    }
    else
    {
      loaded=false;
    }
    delete [] buffer;
  }
  else
  {
    loaded=false;
  }
  if(loaded==true)
  {
    trace("Loaded city");
  }
  else
  {
    trace("Could not load city");
  }
  return loaded;
}

/*
bool LoadCityOrig()
{
  bool loaded=true;
  size_t len=4+sizeof(GameState);
  uint8_t *buffer=new uint8_t[len];
  if(buffer!=nullptr)
  {
    diskr(buffer,len);
    if(buffer[0]!='C')
    {
      loaded=false;
    }
    if(buffer[1]!='T')
    {
      loaded=false;
    }
    if(buffer[2]!='Y')
    {
      loaded=false;
    }
    if(buffer[3]!='2')
    {
      loaded=false;
    }

    memcpy((void *)&State,(void *)&buffer[4],sizeof(GameState));

    delete [] buffer;
  }
  else
  {
    loaded=false;
  }
  return loaded;
}
*/

uint8_t *powergridptr=nullptr;

uint8_t* GetPowerGrid()
{
  if(powergridptr==nullptr)
  {
    powergridptr=(uint8_t *)malloc(((MAP_WIDTH*MAP_HEIGHT)/8)+128);  // should be at least map size (w*h) bits? PowerFloodFill() function uses bytes after the map size for a stack - so make sure there's extra bytes
  }
  return powergridptr;
}

void start()
{
  PALETTE[0]=0x000000;      // black
  PALETTE[1]=0xffffff;      // white
  PALETTE[2]=0x35A54D;      // green
  PALETTE[3]=0x5183C1;      // blue
  InitGame();

  //load demo city for title screen
  LoadStaticCity(democity);
  ResetVisibleTileCache();
}

void update()
{
  global::ticks++;
  TickGame();
}
