// (c) 1994-1997 Niklas Beisert, this file is part of "Indoor Music System".
// you must not read, modify, print, compile or copy this file or parts of
// it unless you have accepted the license in the accompanying file IMS.TXT.

#include <string.h>
#include "itplay.h"

int itplayerclass::module::precalctime(int startpos, int (*calctimer)[2], int calcn, int ite)
{
  unsigned char *patptr;
  int patlen;

  int patdelaytick=0;
  int patdelayrow=0;
  int sync=-1;
  int looped=0;
  int gotorow=(startpos>>8)&0xFF;
  int gotoord=startpos&0xFF;
  int curord=-1;
  int currow=-1;

  int curspeed=inispeed;
  int curtick=inispeed-1;

  int loopord=0;
  int tempo=initempo;
  int timerval=0;
  int timerfrac=0;

  unsigned char tempos[64];
  unsigned char cmds[64];
  unsigned char specials[64];
  unsigned char patloopcount[64];
  unsigned char patloopstart[64];
  memset(tempos,0,nchan);
  memset(specials,0,nchan);
  memset(cmds,0,nchan);


  int it;
  for (it=0; it<ite; it++)
  {
    int i;
    curtick++;
    if ((curtick==(curspeed+patdelaytick))&&patdelayrow)
    {
      curtick=0;
      patdelayrow--;
    }

    if (curtick==(curspeed+patdelaytick))
    {
      patdelaytick=0;
      curtick=0;
      currow++;
      if ((gotoord==-1)&&(currow==patlens[orders[curord]]))
      {
        gotoord=curord+1;
        gotorow=0;
      }
      if (gotoord!=-1)
      {
        if (gotoord!=curord)
        {
          memset(patloopcount,0,nchan);
          memset(patloopstart,0,nchan);
        }

        if (gotoord>=endord)
          gotoord=0;
        while (orders[gotoord]==0xFFFF)
          gotoord++;
        if (gotoord==endord)
          gotoord=0;
        if (gotorow>=patlens[orders[gotoord]])
        {
          gotoord++;
          gotorow=0;
          while (orders[gotoord]==0xFFFF)
            gotoord++;
          if (gotoord==endord)
            gotoord=0;
        }
        if (gotoord<curord)
          looped=1;
        curord=gotoord;
        patptr=patterns[orders[curord]];
        for (currow=0; currow<gotorow; currow++)
        {
          while (*patptr)
            patptr+=6;
          patptr++;
        }
        gotoord=-1;
      }

      for (i=0; i<nchan; i++)
        cmds[i]=0;
      while (*patptr)
      {
        int ch=*patptr++-1;

        int data=patptr[4];

        cmds[ch]=patptr[3];
        switch (cmds[ch])
        {
        case cmdSpeed:
          if (data)
            curspeed=data;
          break;
        case cmdJump:
          gotorow=0;
          gotoord=data;
          break;
        case cmdBreak:
          if (gotoord==-1)
            gotoord=curord+1;
          gotorow=data;
          break;
        case cmdSpecial:
          if (data)
            specials[ch]=data;
          switch (specials[ch]>>4)
          {
          case cmdSPatDelayTick:
            patdelaytick=specials[ch]&0xF;
            break;
          case cmdSPatLoop:
            if (!(specials[ch]&0xF))
              patloopstart[ch]=currow;
            else
            {
              patloopcount[ch]++;
              if (patloopcount[ch]<=(specials[ch]&0xF))
              {
                gotorow=patloopstart[ch];
                gotoord=curord;
              }
              else
              {
                patloopcount[ch]=0;
                patloopstart[ch]=currow+1;
              }
            }
            break;
          case cmdSPatDelayRow:
            patdelayrow=specials[ch]&0xF;
            break;
          }
          break;
        case cmdTempo:
          if (data)
            tempos[ch]=data;
          if (tempos[ch]>=0x20)
            tempo=tempos[ch];
          break;
        case cmdSync:
          sync=data;
          break;
        }
        patptr+=5;
      }
      patptr++;
    }
    else
      for (i=0; i<nchan; i++)
        if ((cmds[i]==cmdTempo)&&(tempos[i]<0x20))
        {
          tempo+=(tempos[i]<0x10)?-tempos[i]:(tempos[i]&0xF);
          tempo=(tempo<0x20)?0x20:(tempo>0xFF)?0xFF:tempo;
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

    timerfrac+=4096*163840/tempo;
    timerval+=timerfrac>>12;
    timerfrac&=4095;

    for (i=0; i<calcn; i++)
      if (calctimer[i][1]<0)
        break;

    if (i==calcn)
      break;
  }

  return 1;
}
