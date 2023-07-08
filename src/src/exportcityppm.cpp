#include "exportcityppm.h"

#include <stdint.h>

#include "printf.h"
#include "wasmnew.h"
#include "wasm4.h"
#include "Defines.h"
#include "Terrain.h"

uint8_t CalculateTile(int x, int y);
const uint8_t GetTileColor(const uint8_t tile);
const uint8_t* GetTileData(uint8_t tile);

void ExportCityPPM()
{
  trace("P3");
  trace("384 384");
  trace("255");
  // output ppm
  for(int y=0; y<MAP_HEIGHT; y++)
  {
    uint8_t terrain[MAP_WIDTH];
    uint8_t terraincolors[MAP_WIDTH];
    uint8_t tiles[MAP_WIDTH];
    uint8_t colors[MAP_WIDTH];
    for(int x=0; x<MAP_WIDTH; x++)
    {
      terrain[x]=GetAnimatedTerrainTile(x,y);
      terraincolors[x]=GetTileColor(terrain[x]);
      tiles[x]=CalculateTile(x,y);
      colors[x]=GetTileColor(tiles[x]);
    }
    for(int py=0; py<8; py++)
    {
      int lp=0;
      //char line[384*4*3+1];
      char *line=new char[384*4*3+1];
      for(int i=0; i<385*4*3; i++)
      {
        line[i]=' ';
      }
      line[384*4*3]='\0';
      uint8_t shift=(1 << py);
      for(int x=0; x<MAP_WIDTH; x++)
      {
        for(int px=0; px<8; px++)
        {
          uint8_t p=GetTileData(tiles[x])[px];
          uint8_t color=colors[x];
          
          // hack for black power lines over water
          if(tiles[x]==FIRST_POWERLINE_BRIDGE_TILE || tiles[x]==(FIRST_POWERLINE_BRIDGE_TILE+1))
          {
            p=GetTileData(terrain[x])[px];
            color=terraincolors[x];
          }
          // hack for drawing powerline over land with transparent background
          else if(tiles[x] >= (FIRST_POWERLINE_TILE) && tiles[x] < (FIRST_POWERLINE_TILE+11))
          {
            p=GetTileData(terrain[x])[px];
            color=terraincolors[x];
          }

          uint8_t pidx=(p & shift ? color >> 4 & 0xf : color & 0xf)-1;    // palette index

          // hack for black power lines over water
          if(tiles[x]==FIRST_POWERLINE_BRIDGE_TILE && (py==1 || py==3))
          {
            pidx=0;
          }
          else if(tiles[x]==FIRST_POWERLINE_BRIDGE_TILE+1 && (px==3 || px==5))
          {
            pidx=0;
          }

          // overlay land powerline over terrain
          if(tiles[x]>=FIRST_POWERLINE_TILE && tiles[x]<(FIRST_POWERLINE_TILE+11))
          {
            p=GetTileData(tiles[x])[px];
            color=colors[x];
            const uint8_t cidx=(p & shift ? color >> 4 & 0xf : color & 0xf);
            if(cidx>0)  // 0=transparent - so skip over drawing
            {
              pidx=cidx-1;
            }
          }

          int r=(PALETTE[pidx] >> 16) & 0xff;
          int g=(PALETTE[pidx] >> 8) & 0xff;
          int b=(PALETTE[pidx] >> 0) & 0xff;

          char cbuff[(4*3)+1];
          cbuff[4*3]='\0';
          int len=snprintf(cbuff,4*3,"%i %i %i",r,g,b);

          for(int cp=0; cp<len; cp++)
          {
            line[lp+cp]=cbuff[cp];
          }
          lp+=(4*3);

        }
      }
      trace(line);
      delete [] line;
    }
  }
}
