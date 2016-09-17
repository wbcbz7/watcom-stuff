// (c) 1994-1997 Niklas Beisert, this file is part of "Indoor Music System".
// you must not read, modify, print, compile or copy this file or parts of
// it unless you have accepted the license in the accompanying file IMS.TXT.

#include <string.h>
#include "mcp.h"
#include "binfile.h"
#include "gmdinst.h"
#include "xmplay.h"
#include "err.h"

static unsigned char chPatLoopCount[32];
static unsigned char chPatLoopStart[32];

static unsigned char curtick;
static unsigned char curtempo;

static int looped;
static int currow;
static unsigned char (*patptr)[5];
static int patlen;
static int curord;

static int nord;
static int nchan;
static int loopord;
static unsigned char (**patterns)[5];
static unsigned short *orders;
static unsigned short *patlens;

static int jumptoord;
static int jumptorow;
static int patdelay;

static int timerval;
static int timerfrac;
static int speed;

static int (*calctimer)[2];
static int calcn;
static int sync;

static int xmpFindTick()
{
  int i;
  curtick++;
  if (curtick>=curtempo)
    curtick=0;

  if (!curtick&&patdelay)
  {
    if (jumptoord!=-1)
    {
      if (jumptoord!=curord)
        for (i=0; i<nchan; i++)
        {
          chPatLoopCount[i]=0;
          chPatLoopStart[i]=0;
        }

      if (jumptoord>=nord)
        jumptoord=loopord;

      curord=jumptoord;
      currow=jumptorow;
      jumptoord=-1;
      patlen=patlens[orders[curord]];
      patptr=patterns[orders[curord]];
    }
  }

  if (!curtick&&patdelay)
  {
    patdelay--;
  }
  else
  if (!curtick)
  {
    currow++;
    if ((jumptoord==-1)&&(currow>=patlen))
    {
      jumptoord=curord+1;
      jumptorow=0;
    }
    if (jumptoord!=-1)
    {
      if (jumptoord!=curord)
        for (i=0; i<nchan; i++)
        {
          chPatLoopCount[i]=0;
          chPatLoopStart[i]=0;
        }

      if (jumptoord>=nord)
        jumptoord=loopord;
      if (jumptoord<curord)
        looped=1;

      curord=jumptoord;
      currow=jumptorow;
      jumptoord=-1;
      patlen=patlens[orders[curord]];
      patptr=patterns[orders[curord]];
    }


    for (i=0; i<nchan; i++)
    {
      int procdat=patptr[nchan*currow+i][4];

      switch (patptr[nchan*currow+i][3])
      {
      case xmpCmdSync1: case xmpCmdSync2: case xmpCmdSync3:
        sync=procdat;
        break;
      case xmpCmdJump:
        jumptoord=procdat;
        jumptorow=0;
        break;
      case xmpCmdBreak:
        if (jumptoord==-1)
          jumptoord=curord+1;
        jumptorow=(procdat&0xF)+(procdat>>4)*10;
        break;
      case xmpCmdSpeed:
        if (!procdat)
        {
          jumptoord=procdat;
          jumptorow=0;
          break;
        }
        if (procdat>=0x20)
          speed=procdat;
        else
          curtempo=procdat;
        break;
      case xmpCmdPatLoop:
        if (!procdat)
          chPatLoopStart[i]=currow;
        else
        {
          chPatLoopCount[i]++;
          if (chPatLoopCount[i]<=procdat)
          {
            jumptorow=chPatLoopStart[i];
            jumptoord=curord;
          }
          else
          {
            chPatLoopCount[i]=0;
            chPatLoopStart[i]=currow+1;
          }
        }
        break;
      case xmpCmdPatDelay:
        patdelay=procdat;
        break;
      }
    }
  }
  int p=(curord<<16)|(currow<<8)|curtick;
  for (i=0; i<calcn; i++)
    if ((p==calctimer[i][0])&&(calctimer[i][1]<0))
      if (!++calctimer[i][1])
        calctimer[i][1]=timerval;

  if (sync!=-1)
    for (i=0; i<calcn; i++)
      if ((calctimer[i][0]==(-256-sync))&&(calctimer[i][1]<0))
        if (!++calctimer[i][1])
          calctimer[i][1]=timerval;

  sync=-1;

  if (looped)
    for (i=0; i<calcn; i++)
      if ((calctimer[i][0]==-1)&&(calctimer[i][1]<0))
        if (!++calctimer[i][1])
          calctimer[i][1]=timerval;

  looped=0;

  timerfrac+=4096*163840/speed;
  timerval+=timerfrac>>12;
  timerfrac&=4095;

  for (i=0; i<calcn; i++)
    if (calctimer[i][1]<0)
      return 0;

  return 1;
}

int xmpPrecalcTime(xmodule &m, int startpos, int (*calc)[2], int n, int ite)
{
  int i;

  patdelay=0;
  sync=-1;
  looped=0;
  calcn=n;
  calctimer=calc;
  jumptorow=(startpos>>8)&0xFF;
  jumptoord=startpos&0xFF;
  curord=-1;
  currow=-1;
  nord=m.nord;
  patterns=m.patterns;
  orders=m.orders;
  patlens=m.patlens;
  nchan=m.nchan;
  loopord=m.loopord;

  curtempo=m.initempo;
  curtick=m.initempo-1;

  speed=m.inibpm;
  timerval=0;
  timerfrac=0;

  for (i=0; i<ite; i++)
    if (xmpFindTick())
      break;

  return 1;
}
