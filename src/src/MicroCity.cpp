#include "Draw.h"
#include "Interface.h"
#include "Game.h"
#include "Simulation.h"

#include "wasm4.h"
#include "wasmmalloc.h"
#include "wasmmemcpy.h"
#include "global.h"

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

void SaveCity()
{
  size_t len=4+sizeof(GameState);
  uint8_t *buffer=new uint8_t[len];
  if(buffer!=nullptr)
  {
    buffer[0]='C';
    buffer[1]='T';
    buffer[2]='Y';
    buffer[3]='1';
    memcpy((void *)&buffer[4],(void *)&State,sizeof(GameState));
    diskw(buffer,len);
    delete [] buffer;
  }

}

bool LoadCity()
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
    if(buffer[3]!='1')
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

uint8_t *powergridptr=nullptr;

uint8_t* GetPowerGrid()
{
  if(powergridptr==nullptr)
  {
    powergridptr=(uint8_t *)malloc(4096);  // should be at least map size (w*h) bytes?
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
}

void update()
{
  global::ticks++;
  TickGame();
}
