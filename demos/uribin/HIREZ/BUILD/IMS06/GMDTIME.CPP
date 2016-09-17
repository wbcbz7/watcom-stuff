// (c) 1994-1997 Niklas Beisert, this file is part of "Indoor Music System".
// you must not read, modify, print, compile or copy this file or parts of
// it unless you have accepted the license in the accompanying file IMS.TXT.

#include <stdlib.h>
#include <string.h>
#include "gmdplay.h"

static int timerval;
static int timerfrac;
static int gspeed;
static unsigned char currenttick;
static unsigned char tempo;
static unsigned short currentrow;
static unsigned short patternlen;
static unsigned short currentpattern;
static unsigned short patternnum;
static unsigned short looppat;
static unsigned short endpat;
static gmdtrack gtrack;
static const gmdpattern *patterns;
static const gmdtrack *tracks;
static const unsigned short *orders;
static unsigned short speed;
static short brkpat;
static short brkrow;
static unsigned char patlooprow[MP_MAXCHANNELS];
static unsigned char patloopcount[MP_MAXCHANNELS];
static unsigned char globchan;
static unsigned char patdelay;
static char looped;
static int (*calctimer)[2];
static int calcn;
static int sync;

static void trackmoveto(gmdtrack &t, unsigned char row)
{
  while (1)
  {
    if (t.ptr>=t.end)
      break;
    if (t.ptr[0]>=row)
      break;
    t.ptr+=t.ptr[1]+2;
  }
}

static void LoadPattern(unsigned short p, unsigned char r)
{
  const gmdpattern &pat=patterns[orders[p]];
  patternlen=pat.patlen;
  if (r>=patternlen)
    r=0;
  currenttick=0;
  currentrow=r;
  currentpattern=p;

  gtrack=tracks[pat.gtrack];
  trackmoveto(gtrack, r);
}

static void PlayGCommand(const unsigned char *cmd, unsigned char len)
{
  const unsigned char *cend=cmd+len;
  while (cmd<cend)
  {
    switch (*cmd++)
    {
    case cmdTempo:
      tempo=*cmd;
      break;
    case cmdSpeed:
      speed=*cmd;
      gspeed=speed*10;
      break;
    case cmdFineSpeed:
      gspeed=10*speed+*cmd;
      break;
    case cmdBreak:
      if (brkpat==-1)
      {
        brkpat=currentpattern+1;
        if (brkpat==endpat)
        {
          brkpat=looppat;
          looped=1;
        }
      }
      brkrow=*cmd;
      break;
    case cmdGoto:
      brkpat=*cmd;
      if (brkpat<=currentpattern)
        looped=1;
//      brkrow=0;
      break;
    case cmdPatLoop:
      if (*cmd)
        if (patloopcount[globchan]++<*cmd)
        {
          brkpat=currentpattern;
          brkrow=patlooprow[globchan];
        }
        else
        {
          patloopcount[globchan]=0;
          patlooprow[globchan]=currentrow+1;
        }
      else
        patlooprow[globchan]=currentrow;
      break;
    case cmdPatDelay:
      if (!patdelay&&*cmd)
        patdelay=*cmd+1;
      break;
    case cmdSetChan:
      globchan=*cmd;
      break;
    }
    cmd++;
  }
}

static int FindTick()
{
  int i;

  currenttick++;
  if (currenttick>=tempo)
    currenttick=0;

  if (!currenttick&&patdelay)
  {
    brkpat=currentpattern;
    brkrow=currentrow;
//    patdelay--;
  }

  if (!currenttick/*&&!patdelay*/)
  {
    currenttick=0;

    currentrow++;

    if ((currentrow>=patternlen)&&(brkpat==-1))
    {
      brkpat=currentpattern+1;
      if (brkpat==endpat)
      {
        looped=1;
        brkpat=looppat;
      }
      brkrow=0;
    }
    if (brkpat!=-1)
    {
      if (currentpattern!=brkpat)
      {
        memset(patloopcount, 0, sizeof(patloopcount));
        memset(patlooprow, 0, sizeof(patlooprow));
      }
      currentpattern=brkpat;
      currentrow=brkrow;
      brkpat=-1;
      brkrow=0;
      while ((currentpattern<patternnum)&&(orders[currentpattern]==0xFFFF))
        currentpattern++;
      if ((currentpattern>=patternnum)||(currentpattern==endpat))
      {
        currentpattern=looppat;
        looped=1;
      }
      if (!currentpattern&&!currentrow&&!patdelay)
      {
        currentpattern=0;
        currentrow=0;
        tempo=6;
        speed=125;
        gspeed=1250;
      }
      LoadPattern(currentpattern, currentrow);
    }

    while (1)
    {
      if (gtrack.ptr>=gtrack.end)
        break;
      if (gtrack.ptr[0]!=currentrow)
        break;
      PlayGCommand(gtrack.ptr+2, gtrack.ptr[1]);
      gtrack.ptr+=gtrack.ptr[1]+2;
    }

    if (patdelay)
      patdelay--;
  }

  int p=(currentpattern<<16)|(currentrow<<8)|currenttick;
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

  timerfrac+=1638400*1024/gspeed;
  timerval+=timerfrac>>10;
  timerfrac&=1023;

  for (i=0; i<calcn; i++)
    if (calctimer[i][1]<0)
      return 0;

  return 1;
}

int gmdPrecalcTime(gmdmodule &m, int startpos, int (*calc)[2], int n, int ite)
{
  if (m.orders[0]==0xFFFF)
    return 0;

  sync=-1;
  calcn=n;
  calctimer=calc;
  patterns=m.patterns;
  orders=m.orders;
  patternnum=m.ordnum;
  tracks=m.tracks;
  looppat=(m.loopord<m.ordnum)?m.loopord:0;
  while (m.orders[looppat]==0xFFFF)
    looppat--;

  endpat=m.endord;

  tempo=6;
  patdelay=0;
  patternlen=0;
  currenttick=tempo;
  currentrow=0;
  currentpattern=0;
  looped=0;
  brkpat=0;
  brkrow=0;
  speed=125;
  gspeed=1250;
  timerval=0;
  timerfrac=0;

  int i;
  for (i=0; i<ite; i++)
    if (FindTick())
      return 1;

  return 0;
}
