#include "Draw.h"
#include "Interface.h"
#include "Game.h"
#include "Simulation.h"

#include "wasm4.h"
#include "wasmmalloc.h"
#include "wasmmemcpy.h"
#include "global.h"

#include "democity.h"

//debug
//#include "printf.h"

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
    buffer[3]='2';
    memcpy((void *)&buffer[4],(void *)&State,sizeof(GameState));

    // debug - output bytes of game state
    /*
    {
      trace("game data ={");
      char cbuff[64];
      int pos=0;
      for(;pos+8<len;pos+=8)
      {
        int r=snprintf(cbuff,63,"0x%.2hhx,0x%.2hhx,0x%.2hhx,0x%.2hhx,0x%.2hhx,0x%.2hhx,0x%.2hhx,0x%.2hhx,",buffer[pos],buffer[pos+1],buffer[pos+2],buffer[pos+3],buffer[pos+4],buffer[pos+5],buffer[pos+6],buffer[pos+7]);
        buffer[r]='\0';
        trace(cbuff);
      }
      while(pos<len)
      {
        int r=snprintf(cbuff,63,"0x%.2hhx,",buffer[pos]);
        buffer[r]='\0';
        trace(cbuff);
        pos++;
      }
      trace("};");
    }
    */

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

  //load demo city for title screen
  memcpy((void *)&State,(void *)&democity[0],sizeof(GameState));
}

void update()
{
  global::ticks++;
  TickGame();
}
