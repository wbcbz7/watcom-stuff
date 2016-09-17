// (c) 1994-1997 Niklas Beisert, this file is part of "Indoor Music System".
// you must not read, modify, print, compile or copy this file or parts of
// it unless you have accepted the license in the accompanying file IMS.TXT.

#include <string.h>
#include <conio.h>
#include <stdlib.h>
#include "imsdev.h"
#include "mcp.h"
#include "mix.h"
#include "timer.h"
#include "imsrtns.h"

#define MAXSAMPLES 256

extern "C" extern sounddevice mcpSoundBlaster32;

static signed short awePort;
static unsigned long aweMem;


static char getcfg()
{
  awePort=-1;
  char *s=getenv("BLASTER");
  if (!s)
    return 0;
  while (1)
  {
    while (*s==' ')
      s++;
    if (!*s)
      break;
    if (((*s=='a')||(*s=='A'))&&(awePort==-1))
      awePort=strtoul(s+1, 0, 16)+0x400;
    if ((*s=='e')||(*s=='E'))
    {
      awePort=strtoul(s+1, 0, 16);
      return 1;
    }
    while ((*s!=' ')&&*s)
      s++;
  }
  return awePort!=-1;
}

static unsigned char linvol[513]=
{
  0x00,0x2d,0x36,0x3c,0x40,0x42,0x45,0x47,0x49,0x4a,0x4c,0x4d,0x4e,0x4f,0x50,0x51,
  0x52,0x53,0x54,0x54,0x55,0x56,0x56,0x57,0x57,0x58,0x58,0x59,0x59,0x5a,0x5a,0x5b,
  0x5b,0x5c,0x5c,0x5c,0x5d,0x5d,0x5e,0x5e,0x5e,0x5f,0x5f,0x5f,0x60,0x60,0x60,0x60,
  0x61,0x61,0x61,0x61,0x62,0x62,0x62,0x62,0x63,0x63,0x63,0x63,0x64,0x64,0x64,0x64,
  0x65,0x65,0x65,0x65,0x65,0x66,0x66,0x66,0x66,0x66,0x66,0x67,0x67,0x67,0x67,0x67,
  0x67,0x68,0x68,0x68,0x68,0x68,0x68,0x69,0x69,0x69,0x69,0x69,0x69,0x69,0x6a,0x6a,
  0x6a,0x6a,0x6a,0x6a,0x6a,0x6b,0x6b,0x6b,0x6b,0x6b,0x6b,0x6b,0x6b,0x6c,0x6c,0x6c,
  0x6c,0x6c,0x6c,0x6c,0x6c,0x6d,0x6d,0x6d,0x6d,0x6d,0x6d,0x6d,0x6d,0x6d,0x6e,0x6e,
  0x6e,0x6e,0x6e,0x6e,0x6e,0x6e,0x6e,0x6e,0x6f,0x6f,0x6f,0x6f,0x6f,0x6f,0x6f,0x6f,
  0x6f,0x6f,0x70,0x70,0x70,0x70,0x70,0x70,0x70,0x70,0x70,0x70,0x70,0x70,0x71,0x71,
  0x71,0x71,0x71,0x71,0x71,0x71,0x71,0x71,0x71,0x71,0x72,0x72,0x72,0x72,0x72,0x72,
  0x72,0x72,0x72,0x72,0x72,0x72,0x72,0x73,0x73,0x73,0x73,0x73,0x73,0x73,0x73,0x73,
  0x73,0x73,0x73,0x73,0x73,0x74,0x74,0x74,0x74,0x74,0x74,0x74,0x74,0x74,0x74,0x74,
  0x74,0x74,0x74,0x74,0x74,0x75,0x75,0x75,0x75,0x75,0x75,0x75,0x75,0x75,0x75,0x75,
  0x75,0x75,0x75,0x75,0x75,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,
  0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,
  0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x78,0x78,0x78,0x78,0x78,0x78,
  0x78,0x78,0x78,0x78,0x78,0x78,0x78,0x78,0x78,0x78,0x78,0x78,0x78,0x78,0x78,0x79,
  0x79,0x79,0x79,0x79,0x79,0x79,0x79,0x79,0x79,0x79,0x79,0x79,0x79,0x79,0x79,0x79,
  0x79,0x79,0x79,0x79,0x79,0x7a,0x7a,0x7a,0x7a,0x7a,0x7a,0x7a,0x7a,0x7a,0x7a,0x7a,
  0x7a,0x7a,0x7a,0x7a,0x7a,0x7a,0x7a,0x7a,0x7a,0x7a,0x7a,0x7a,0x7a,0x7b,0x7b,0x7b,
  0x7b,0x7b,0x7b,0x7b,0x7b,0x7b,0x7b,0x7b,0x7b,0x7b,0x7b,0x7b,0x7b,0x7b,0x7b,0x7b,
  0x7b,0x7b,0x7b,0x7b,0x7b,0x7b,0x7b,0x7c,0x7c,0x7c,0x7c,0x7c,0x7c,0x7c,0x7c,0x7c,
  0x7c,0x7c,0x7c,0x7c,0x7c,0x7c,0x7c,0x7c,0x7c,0x7c,0x7c,0x7c,0x7c,0x7c,0x7c,0x7c,
  0x7c,0x7c,0x7c,0x7d,0x7d,0x7d,0x7d,0x7d,0x7d,0x7d,0x7d,0x7d,0x7d,0x7d,0x7d,0x7d,
  0x7d,0x7d,0x7d,0x7d,0x7d,0x7d,0x7d,0x7d,0x7d,0x7d,0x7d,0x7d,0x7d,0x7d,0x7d,0x7d,
  0x7d,0x7e,0x7e,0x7e,0x7e,0x7e,0x7e,0x7e,0x7e,0x7e,0x7e,0x7e,0x7e,0x7e,0x7e,0x7e,
  0x7e,0x7e,0x7e,0x7e,0x7e,0x7e,0x7e,0x7e,0x7e,0x7e,0x7e,0x7e,0x7e,0x7e,0x7e,0x7e,
  0x7e,0x7e,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,
  0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,
  0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,
  0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,
  0x7f
};

static unsigned long inwAWE(unsigned short p, unsigned short c)
{
  outpw(awePort+0x802,c);
  return inpw(awePort+p);
}

static unsigned long indAWE(unsigned short p, unsigned short c)
{
  outpw(awePort+0x802,c);
//  return inpd(awePort+p)|(inpd(awePort+p+2)<<16);
  return inpd(awePort+p);
}

static void outwAWE(unsigned short p, unsigned short c, unsigned short v)
{
  outpw(awePort+0x802,c);
  outpw(awePort+p,v);
}

static void outdAWE(unsigned short p, unsigned short c, unsigned long v)
{
  outpw(awePort+0x802,c);
  outpd(awePort+p,v);
//  outpw(awePort+p, v);
//  outpw(awePort+p+2, v>>16);
}

static void delayAWE(unsigned short n)
{
  unsigned short t0=inwAWE(0x402,0x3B);
  unsigned short t=t0;
  while (n>(unsigned short)(t-t0))
    t=inwAWE(0x402,0x3B);
}

static void aweEnableDRAM(int read)
{
  int k;
  outwAWE(0x400,0x3E,0x0020);
  for (k=0; k<30; k++)
  {
    outwAWE(0x400,0xA0+k,0x0080);
    outdAWE(0x000,0x40+k,0x00000000);
    outdAWE(0x000,0x60+k,0x00000000);
    outdAWE(0x000,0xC0+k,0x00000000);
    outdAWE(0x000,0xE0+k,0x00000000);
    outdAWE(0x000,0x20+k,0x40000000);
    outdAWE(0x000,0x00+k,0x40000000);
    outdAWE(0x400,0x00+k,((k&1)&&read)?0x04000000:0x06000000);
// 0x04002000:0x06002000;
  }
}


static void aweDisableDRAM()
{
  int k;
  for (k=0; k<30; k++)
  {
    outdAWE(0x400,0x00+k,0x00000000);
    outwAWE(0x400,0xA0+k,0x807F);
  }
}


static unsigned long aweGetMem()
{
  aweEnableDRAM(1);

  outdAWE(0x400,0x36,0x200000);
  outwAWE(0x400,0x3A,0x1234);
  outwAWE(0x400,0x3A,0x7777);

  aweMem=0;
  while (aweMem<14*0x100000)
  {
    delayAWE(2);
    outdAWE(0x400,0x34,0x200000);
    inwAWE(0x400,0x3A);
    if (inwAWE(0x400,0x3A)!=0x1234)
      break;
    if (inwAWE(0x400,0x3A)!=0x7777)
      break;
    aweMem+=0x8000;
    outdAWE(0x400,0x36,0x200000+aweMem);
    outwAWE(0x400,0x3A,0xFFFF);
  }

  aweDisableDRAM();

  return !!aweMem;
}


static char aweTestPort(unsigned short port)
{
  awePort=port;

  if (inwAWE(0x800,0xE0)==0xFFFF)
    return 0;
  if (inwAWE(0x400,0x3D)==0xFFFF)
    return 0;
  if (inwAWE(0x400,0x3E)==0xFFFF)
    return 0;
  if (inwAWE(0x400,0x3F)==0xFFFF)
    return 0;

  if ((inwAWE(0x800,0xE0)&0x0C)!=0x0C)
    return 0;
  if ((inwAWE(0x400,0x3D)&0x7E)!=0x58)
    return 0;
  if ((inwAWE(0x400,0x3E)&0x03)!=0x03)
    return 0;

  aweEnableDRAM(1);

  outdAWE(0x400,0x36,0x200000);
  outwAWE(0x400,0x3A,0x1234);
  outwAWE(0x400,0x3A,0x7777);

  delayAWE(2);
  outdAWE(0x400,0x34,0x200000);
  inwAWE(0x400,0x3A);
  if (inwAWE(0x400,0x3A)!=0x1234)
    return 0;
  if (inwAWE(0x400,0x3A)!=0x7777)
    return 0;

  aweDisableDRAM();

  return 1;
}



/*
global registers:
4 0x400 0x34    DRAM read address
2 0x400 0x35
4 0x400 0x36    DRAM write address
2 0x400 0x3A    DRAM port
2 0x402 0x3B    44.1khz counter

2 0x400 0x3D
2 0x400 0x3E
2 0x400 0x3F
2 0x800 0xE0

2 0x802 sel     register select


channel registers:
4 0x000 0x00+c
4 0x000 0x20+c  16:???/8:reverb send/8:???
4 0x000 0x40+c
4 0x000 0x60+c  16:overall volume/16:overall filter cutoff
4 0x000 0x80+c
4 0x000 0xA0+c
4 0x000 0xC0+c  8:panning/24:loop start
4 0x000 0xE0+c  8:chorus send/24:loop end
4 0x400 0x00+c  4:filter coeff/4:channel mode/24:start address

2 0x400 0x80+c  16:EG2 delay
2 0x402 0x80+c  8:EG2 hold/8:EG2 attack
2 0x400 0xA0+c  1:EG2 override/7:EG2 sustain/8:EG2 decay
2 0x402 0xA0+c  16:LFO1 delay
2 0x400 0xC0+c  16:EG1 delay
2 0x402 0xC0+c  8:EG1 hold/8:EG1 attack
2 0x400 0xE0+c  1:EG1 override/7:EG1 sustain/8:EG1 decay
2 0x402 0xE0+c  16:LFO2 delay

2 0x800 0x00+c  16:pitch
2 0x800 0x20+c  8:filter cutoff/8:volume
2 0x800 0x40+c  8:EG1 to pitch/8:EG1 to filter cutoff
2 0x800 0x60+c  8:LFO1 to pitch/8:LFO1 to filter cutoff
2 0x800 0x80+c  8:LFO1 to volume/8:LFO1 frequency
2 0x800 0xA0+c  8:LFO2 to pitch/8:LFO2 frequency
2 0x800 0xC0+c

2 0x800 0xE0+c  no channel reg??

effects engine registers:
4 0x400 0x20+?  effects engine: 0x14*
4 0x400 0x29    effects engine:
4 0x400 0x2A    effects engine: chorus freq.
4 0x400 0x2D    effects engine:
4 0x400 0x2E    effects engine:

2 0x400 0x40+e  effect 00-15 a
2 0x400 0x41+e  effect 00-15 b
2 0x402 0x40+e  effect 16-31 a
2 0x402 0x41+e  effect 16-31 b
2 0x400 0x60+e  effect 32-47 a
2 0x400 0x61+e  effect 32-47 b
2 0x402 0x60+e  effect 48-63 a
2 0x402 0x61+e  effect 48-63 b
*/







static void setfx(const unsigned short *par, int t2)
{
  int i,j;
  for (j=0; j<4; j++)
    for (i=0; i<0x20; i++)
      outwAWE((j&1)?0x402:0x400, ((j&2)?0x60:0x40)+i, par[j*32+i]|((t2&&(j&1))?0x8000:0x0000));
}

static unsigned short effectdata0[128]=
{
  0x03FF,0x0030,0x07FF,0x0130,0x0BFF,0x0230,0x0FFF,0x0330,
  0x13FF,0x0430,0x17FF,0x0530,0x1BFF,0x0630,0x1FFF,0x0730,
  0x23FF,0x0830,0x27FF,0x0930,0x2BFF,0x0A30,0x2FFF,0x0B30,
  0x33FF,0x0C30,0x37FF,0x0D30,0x3BFF,0x0E30,0x3FFF,0x0F30,
  0x43FF,0x0030,0x47FF,0x0130,0x4BFF,0x0230,0x4FFF,0x0330,
  0x53FF,0x0430,0x57FF,0x0530,0x5BFF,0x0630,0x5FFF,0x0730,
  0x63FF,0x0830,0x67FF,0x0930,0x6BFF,0x0A30,0x6FFF,0x0B30,
  0x73FF,0x0C30,0x77FF,0x0D30,0x7BFF,0x0E30,0x7FFF,0x0F30,
  0x83FF,0x0030,0x87FF,0x0130,0x8BFF,0x0230,0x8FFF,0x0330,
  0x93FF,0x0430,0x97FF,0x0530,0x9BFF,0x0630,0x9FFF,0x0730,
  0xA3FF,0x0830,0xA7FF,0x0930,0xABFF,0x0A30,0xAFFF,0x0B30,
  0xB3FF,0x0C30,0xB7FF,0x0D30,0xBBFF,0x0E30,0xBFFF,0x0F30,
  0xC3FF,0x0030,0xC7FF,0x0130,0xCBFF,0x0230,0xCFFF,0x0330,
  0xD3FF,0x0430,0xD7FF,0x0530,0xDBFF,0x0630,0xDFFF,0x0730,
  0xE3FF,0x0830,0xE7FF,0x0930,0xEBFF,0x0A30,0xEFFF,0x0B30,
  0xF3FF,0x0C30,0xF7FF,0x0D30,0xFBFF,0x0E30,0xFFFF,0x0F30
};

static unsigned short effectdata1[128]=
{
  0x0C10,0x8470,0x14FE,0xB488,0x167F,0xA470,0x18E7,0x84B5,
  0x1B6E,0x842A,0x1F1D,0x852A,0x0DA3,0x0F7C,0x167E,0x7254,
  0x0000,0x842A,0x0001,0x852A,0x18E6,0x0BAA,0x1B6D,0x7234,
  0x229F,0x8429,0x2746,0x8529,0x1F1C,0x06E7,0x229E,0x7224,
  0x0DA4,0x8429,0x2C29,0x8529,0x2745,0x07F6,0x2C28,0x7254,
  0x383B,0x8428,0x320F,0x8528,0x320E,0x0F02,0x1341,0x7264,
  0x3EB6,0x8428,0x3EB9,0x8528,0x383A,0x0FA9,0x3EB5,0x7294,
  0x3EB7,0x8474,0x3EBA,0x8575,0x3EB8,0x44C3,0x3EBB,0x45C3,
  0x0000,0xA404,0x0001,0xA504,0x141F,0x0671,0x14FD,0x0287,
  0x3EBC,0xE610,0x3EC8,0x0C7B,0x031A,0x07E6,0x3EC8,0x86F7,
  0x3EC0,0x821E,0x3EBE,0xD280,0x3EBD,0x021F,0x3ECA,0x0386,
  0x3EC1,0x0C03,0x3EC9,0x031E,0x3ECA,0x8C4C,0x3EBF,0x0C55,
  0x3EC9,0xC280,0x3EC4,0xBC84,0x3EC8,0x0EAD,0x3EC8,0xD380,
  0x3EC2,0x8F7E,0x3ECB,0x0219,0x3ECB,0xD2E6,0x3EC5,0x031F,
  0x3EC6,0xC380,0x3EC3,0x327F,0x3EC9,0x0265,0x3EC9,0x8319,
  0x1342,0xD3E6,0x3EC7,0x337F,0x0000,0x8365,0x1420,0x9570
};

static void initawe()
{
// init
  outwAWE(0x400,0x3D,0x0059);
  outwAWE(0x400,0x3E,0x0020);
  outwAWE(0x400,0x3F,0x0000);

// clear chn
  int i;
  for (i=0; i<0x20; i++)
  {
    outwAWE(0x400,0xA0+i,0x0080);
    outwAWE(0x402,0xC0+i,0x0000);
    outwAWE(0x400,0xE0+i,0x0000);
    outwAWE(0x800,0x00+i,0xE000);
    outwAWE(0x800,0x20+i,0xFF00);
    outwAWE(0x800,0x40+i,0x0000);
    outwAWE(0x800,0x60+i,0x0000);
    outwAWE(0x800,0x80+i,0x0018);
    outwAWE(0x800,0xA0+i,0x0018);
    outwAWE(0x800,0xC0+i,0x0000);
    outwAWE(0x402,0xE0+i,0x0000);
    outwAWE(0x402,0xA0+i,0x0000);
    outwAWE(0x402,0x80+i,0x0000);
    outwAWE(0x400,0xA0+i,0x0000);
    outwAWE(0x400,0xC0+i,0x0000);
  }
  delayAWE(2);
  for (i=0; i<0x20; i++)
  {
    outdAWE(0x000,0x20+i,0x00000000);
    outdAWE(0x000,0x60+i,0x0000FFFF);
    outdAWE(0x000,0xC0+i,0x00000000);
    outdAWE(0x000,0xE0+i,0x00000000);
    outdAWE(0x000,0x00+i,0x00000000);
    outdAWE(0x000,0x40+i,0x0000FFFF);
    outdAWE(0x400,0x00+i,0x00000000);
    outdAWE(0x000,0xA0+i,0x00000000);
    outdAWE(0x000,0x80+i,0x00000000);
    outwAWE(0x400,0xA0+i,0x807F);
  }

// fx
  outwAWE(0x400,0x34,0x0000);
  outwAWE(0x400,0x35,0x0000);
  outwAWE(0x400,0x36,0x0000);
  outwAWE(0x400,0x35,0x0000);
  setfx(effectdata0,0);
  delayAWE(0x400);
  setfx(effectdata0,1);
  for (i=0; i<0x14; i++)
    outdAWE(0x400,0x20+i,0x00000000);
  setfx(effectdata1,1);
  outdAWE(0x400,0x29,0x00000000);
  outdAWE(0x400,0x2A,0x00000083);
  outdAWE(0x400,0x2D,0x00008000);
  outdAWE(0x400,0x2E,0x00000000);
  setfx(effectdata1,0);

// refresh
  outwAWE(0x400,0xBE,0x0080);
  outdAWE(0x000,0xDE,0xFFFFFFE0);
  outdAWE(0x000,0xFE,0x00FFFFE8);
  outdAWE(0x000,0x3E,0x00000000);
  outdAWE(0x000,0x1E,0x00000000);
  outdAWE(0x400,0x1E,0x00FFFFE3);
  outwAWE(0x400,0xBF,0x0080);
  outdAWE(0x000,0xDF,0xFFFFFFF0);
  outdAWE(0x000,0xFF,0x00FFFFF8);
  outdAWE(0x000,0x3F,0x000000FF);
  outdAWE(0x000,0x1F,0x00008000);
  outdAWE(0x400,0x1F,0x00FFFFF3);
  outpw(awePort+0x802,0x3E);
  outpw(awePort+0x000,0x0000);
  while (!(inpw(awePort+0x802)&0x1000));
  while (inpw(awePort+0x802)&0x1000);
  outpw(awePort+0x002,0x4828);
  outwAWE(0x400,0x3C,0x0000);
  outdAWE(0x000,0x7E,0xFFFFFFFF);
  outdAWE(0x000,0x7F,0xFFFFFFFF);

// init
  outwAWE(0x400,0x3F,0x0004);
  ((!inwAWE(0x400,0x2F))&0x40); //???????
}


static void initawechan()
{
  int k;
  for (k=0; k<30; k++) // SHUT UP!
    outwAWE(0x400,0xA0+k,0x807F);

  delayAWE(2);

  for (k=0; k<30; k++)  // init channels
  {
    outdAWE(0x000,0x00+k,0x00000000); // 32:?
    outdAWE(0x000,0x20+k,0x00000000); // 16:?/8:reverb/8:?
    outdAWE(0x000,0x40+k,0x0000FFFF); // 32:?
    outdAWE(0x000,0x60+k,0x0000FFFF); // 16:c.volume/16:c.filter
    outdAWE(0x000,0x80+k,0x00000000); // 32:?
    outdAWE(0x000,0xA0+k,0x00000000); // 32:?
    outdAWE(0x000,0xC0+k,0x80000000); // 8:panning/24:loopstart
    outdAWE(0x000,0xE0+k,0x00000000); // 8:chorus/24:loopend
    outdAWE(0x400,0x00+k,0x00000000); // 4:filter c./4:mode/24:start

// EG1: filter,pitch
    outwAWE(0x800,0x00+k,0xE000); // base pitch
    outwAWE(0x400,0xE0+k,0x0000); // 16:delay
    outwAWE(0x402,0xE0+k,0x0000); // 8:hold/8:attack
    outwAWE(0x400,0xC0+k,0x0000); // 1:override/7:sustain/8:decay
    outwAWE(0x800,0x40+k,0x0000); // 8:to pitch/8:to filter

// EG2: volume
    outwAWE(0x800,0x20+k,0xFF00); // 8:EG1 base filter/8:base volume
    outwAWE(0x400,0x80+k,0x0000); // 16:delay
    outwAWE(0x402,0x80+k,0x0000); // 8:hold/8:attack
    outwAWE(0x400,0xA0+k,0x807F); // 1:override/7:sustain/8:decay

// LFO1: pitch,volume,filter
    outwAWE(0x402,0xA0+k,0x0000); // 16:delay
    outwAWE(0x800,0x60+k,0x0000); // 8:to pitch/8:to filter cutoff
    outwAWE(0x800,0x80+k,0x0000); // 8:to volume/8:frequency

// LFO2: pitch
    outwAWE(0x402,0xC0+k,0x0000); // 16:delay
    outwAWE(0x800,0xA0+k,0x0000); // 8:to pitch/8:frequency


    outwAWE(0x800,0xC0+k,0x0000); // 16:?
    outwAWE(0x800,0xE0+k,0x0000); // 16:? (my idea)
  }
}


struct awechan
{
  signed char awechan;

  unsigned long startpos;
  unsigned long endpos;
  unsigned long loopstart;
  unsigned long loopend;
  unsigned long samprate;
  signed short sawepitch;
  unsigned char redlev;

  unsigned short cursamp;
  unsigned char mode;

  unsigned short voll;
  unsigned short volr;
  int vtot;
  int pan;
  unsigned char reverb;
  unsigned char chorus;
  signed short awepitch;

  unsigned char inited;
  unsigned char playing;
  unsigned char stopit;
  unsigned char looped;
  signed long nextpos;
  int samptype;

  int curloop;
  int setloop;
  unsigned short orgvol;
  signed short orgpan;
  unsigned char orgreverb;
  unsigned char orgchorus;
  unsigned char pause;
  unsigned long pausepos;
  unsigned short pausevol;

  void *smpptr;
};

struct awesample
{
  signed long pos;
  unsigned long endpos;
  unsigned long loopstart;
  unsigned long loopend;
  unsigned long sloopstart;
  unsigned long sloopend;
  unsigned long samprate;
  int type;
  unsigned char redlev;
  signed short awepitch;
  void *ptr;
};

static unsigned long mempos;
static awesample samples[MAXSAMPLES];
static unsigned short samplenum;

static unsigned char channelnum;
static void (*playerproc)();
static awechan channels[32];
static unsigned long cmdtimerpos;
static unsigned long stimerlen;
static unsigned long stimerpos;
static unsigned short relspeed;
static unsigned long orgspeed;
static unsigned char mastervol;
static signed char masterreverb;
static signed char masterchorus;
static signed char masterpan;
static signed char masterbal;
static signed short masterawepitch;
static unsigned long amplify;

static unsigned char paused;

static unsigned char filter;


static void processtick()
{
  unsigned long chanwait=0;
  unsigned long chanfree=0x3FFFFFFF;

  int i,j,k;

  for (i=0; i<channelnum; i++)
  {
    awechan &c=channels[i];
    unsigned long cmask=1<<c.awechan;
    if (c.playing&&c.stopit)
    {
      c.playing=0;
      outwAWE(0x400,0xA0+c.awechan,0x807F);
      chanwait|=cmask;
      chanfree&=~cmask;
    }
    c.stopit=0;
    if (c.playing||(c.nextpos!=-1))
      chanfree&=~cmask;
  }
  for (i=0; i<channelnum; i++)
  {
    if (!chanfree)
      break;
    awechan &c=channels[i];
    if ((c.nextpos==-1)||!(chanwait&(1<<c.awechan)))
      continue;
    for (j=0; j<30; j++)
      if ((1<<j)&chanfree)
        break;
    for (k=0; k<channelnum; k++)
      if (channels[k].awechan==j)
        break;
    if (k!=channelnum)
      channels[k].awechan=c.awechan;
    c.awechan=j;
    chanfree&=~(1<<c.awechan);
  }

  chanfree=0;
  for (i=0; i<channelnum; i++)
    if (channels[i].nextpos!=-1)
      chanfree|=1<<channels[i].awechan;
  chanwait&=chanfree;

  for (i=0; i<30; i++)
    if (chanwait&(1<<i))
      while (indAWE(0x000,0x60+i)&0xFF000000);

  for (i=0; i<channelnum; i++)
  {
    awechan &c=channels[i];
    if (c.inited)
    {
      int ac=c.awechan;
      if (!c.playing&&(c.nextpos==-1))
        c.setloop=0;
      awesample &s=samples[c.cursamp];
      if (c.nextpos!=-1)
        c.setloop=1;

      if (c.nextpos!=-1)
        c.nextpos=s.pos+(c.nextpos>>s.redlev);

      if (c.setloop)
      {
        if ((c.curloop==1)&&!(s.type&mcpSampSLoop))
          c.curloop=2;
        if ((c.curloop==2)&&!(s.type&mcpSampLoop))
          c.curloop=0;

        c.startpos=s.pos;
        c.endpos=s.endpos;
        c.samprate=s.samprate;
        c.sawepitch=s.awepitch;
        c.redlev=s.redlev;
        c.smpptr=s.ptr;
        c.samptype=s.type;
        if (c.curloop==2)
        {
          c.loopstart=s.loopstart;
          c.loopend=s.loopend;
          c.looped=1;
        }
        else
        if (c.curloop==1)
        {
          c.loopstart=s.sloopstart;
          c.loopend=s.sloopend;
          c.looped=1;
        }
        else
        {
          c.loopstart=s.endpos+2;
          c.loopend=s.endpos+5;
          c.looped=0;
        }

        outdAWE(0x000,0xC0+ac,c.loopstart|(c.pan<<24));
        outdAWE(0x000,0xE0+ac,c.loopend|(c.chorus<<24));

        if (c.nextpos>=c.loopend)
          c.nextpos=c.loopstart;

        if (c.nextpos==-1)
          if (indAWE(0x400,0x00+ac)>=c.loopend)
            c.nextpos=c.loopstart;
      }
      if (c.nextpos!=-1)
      {
        outdAWE(0x400,0x00+ac,c.nextpos);
        c.playing=1;
      }

      if (indAWE(0x400,0x00+ac)>=c.endpos)
        c.playing=0;

      if (c.playing)
      {
        outdAWE(0x000,0xC0+ac,c.loopstart|(c.pan<<24));
        outdAWE(0x000,0xE0+ac,c.loopend|(c.chorus<<24));
        int pit=0xB99F+c.sawepitch+c.awepitch+masterawepitch;
        outwAWE(0x800,0x00+ac,(pit<0)?(pit&4095):(pit>0xFFFF)?((pit&4095)+0xF000):pit);
        outdAWE(0x000,0x20+ac,(c.reverb<<8)|(indAWE(0x000,0x20+ac)&0xFFFF00FF));
        outwAWE(0x400,0xA0+ac,0x807F+(c.pause?0:(linvol[c.vtot]<<8)));
      }
      else
        outwAWE(0x400,0xA0+c.awechan,0x807F);
    }
    else
      outwAWE(0x400,0xA0+c.awechan,0x807F);

    c.nextpos=-1;
    c.setloop=0;
  }
}


static void timerrout()
{
  if (paused)
    return;
  if (stimerpos<=65536)
    stimerpos=stimerlen;
  else
    stimerpos-=65536;
  tmSetNewRate((stimerpos<=65536)?stimerpos:65536);
  if (stimerpos==stimerlen)
  {
    processtick();
    playerproc();
    cmdtimerpos+=stimerlen;
    stimerlen=umuldiv(256, 1193046*256, orgspeed*relspeed);
  }
}
                 




static void calcrevcho(awechan &c)
{
  if (masterreverb>0)
    c.reverb=((masterreverb<<2)+((c.orgreverb*(64-masterreverb))>>6));
  else
    c.reverb=(c.orgreverb*(64+masterreverb))>>6;

  if (masterchorus>0)
    c.chorus=((masterchorus<<2)+((c.orgchorus*(64-masterchorus))>>6));
  else
    c.chorus=(c.orgchorus*(64+masterchorus))>>6;
}

static void calcvols(awechan &c)
{
  int vl=(c.orgvol*mastervol/16)*amplify/65536;
  if (vl>=0x200)
    vl=0x1FF;
  int vr=(vl*((c.orgpan*masterpan/64)+0x80))>>8;
  vl-=vr;

  if (masterbal)
    if (masterbal<0)
      vr=(vr*(64+masterbal))>>6;
    else
      vl=(vl*(64-masterbal))>>6;
  c.voll=vl;
  c.volr=vr;
  c.vtot=c.voll+c.volr;
  if (c.vtot)
    c.pan=(255*c.voll+c.vtot/2)/c.vtot;
}


void doupload8(const void *buf, unsigned long pos, unsigned long len, unsigned short port);
#pragma aux doupload8 parm [esi] [ebx] [ecx] [edx] modify [eax] = \
  "pushf" \
  "cli" \
  "add dx,802h" \
  "mov ax,36h" \
  "out dx,ax" \
  "sub dx,402h" \
  "mov eax,ebx" \
  "out dx,eax" \
  "add dx,402h" \
  "mov ax,3Ah" \
  "out dx,ax" \
  "sub dx,402h" \
  "xor al,al" \
"lp:" \
    "mov ah,[esi]" \
    "inc esi" \
    "out dx,ax" \
  "dec ecx" \
  "jnz lp" \
  "popf"

void doupload16(const void *buf, unsigned long pos, unsigned long len, unsigned short port);
#pragma aux doupload16 parm [esi] [ebx] [ecx] [edx] modify [eax] = \
  "pushf" \
  "cli" \
  "add dx,802h" \
  "mov ax,36h" \
  "out dx,ax" \
  "sub dx,402h" \
  "mov eax,ebx" \
  "out dx,eax" \
  "add dx,402h" \
  "mov ax,3Ah" \
  "out dx,ax" \
  "sub dx,402h" \
  "rep outsw" \
  "popf"


static void upload8(unsigned long addr, const void *smp, unsigned long len)
{
  doupload8(smp, addr, len, awePort);
}

static void upload16(unsigned long addr, const void *smp, unsigned long len)
{
  doupload16(smp, addr, len, awePort);
}

static int LoadSamples(sampleinfo *sil, int n)
{
  if (n>MAXSAMPLES)
    return 0;

  if (!mcpReduceSamples(sil, n, aweMem, mcpRedNoPingPong|mcpRedAlways16Bit|mcpRedToMono))
    return 0;

  aweEnableDRAM(0);

  mempos=0x200000;
  int i;
  for (i=0; i<n; i++)
  {
    sampleinfo &si=sil[i];
    awesample &s=samples[i];
    s.pos=mempos;
    s.endpos=mempos+si.length;
    s.loopstart=mempos+si.loopstart;
    s.loopend=mempos+si.loopend;
    s.sloopstart=mempos+si.sloopstart;
    s.sloopend=mempos+si.sloopend;
    s.samprate=si.samprate;
    s.type=si.type;
    s.redlev=(si.type&mcpSampRedRate4)?2:(si.type&mcpSampRedRate2)?1:0;
    s.awepitch=mcpGetNote8363(si.samprate)*4/3;
    mempos+=si.length+6;

    if (s.loopstart==s.loopend)
      s.type&=~mcpSampLoop;

    if (s.type&mcpSamp16Bit)
      upload16(s.pos, si.ptr, si.length+8);
    else
      upload8(s.pos, si.ptr, si.length+8);
  }

  aweDisableDRAM();

  samplenum=n;

  for (i=0; i<n; i++)
    samples[i].ptr=sil[i].ptr;

  return 1;
}


static void recalcvols()
{
  int i;
  for (i=0; i<channelnum; i++)
    calcvols(channels[i]);
}

static void recalcrevcho()
{
  int i;
  for (i=0; i<channelnum; i++)
    calcrevcho(channels[i]);
}



static void GetMixChannel(int ch, mixchannel &chn, int rate)
{
  chn.status=0;

  awechan &c=channels[ch];

  if (!c.playing||!c.inited)
    return;

  if (c.pause)
    chn.status|=MIX_MUTE;

  unsigned long pos=paused?c.pausepos:indAWE(0x400,0x00+c.awechan);

  if (pos>=c.endpos)
    return;

  unsigned short resvoll=c.voll;
  unsigned short resvolr=c.volr;

  chn.status|=(c.looped?MIX_LOOPED:0)|((c.samptype&mcpSamp16Bit)?MIX_PLAY16BIT:0);
  chn.step=umuldivrnd(mcpGetFreq8363((masterawepitch+c.sawepitch+c.awepitch)*3/4), 1<<16, rate);
  chn.vols[0]=resvoll*4096/amplify;
  chn.vols[1]=resvolr*4096/amplify;
  chn.samp=c.smpptr;
  chn.length=c.endpos-c.startpos;
  chn.loopstart=c.loopstart-c.startpos;
  chn.loopend=c.loopend-c.startpos;
  chn.replen=(chn.status&MIX_LOOPED)?(chn.loopend-chn.loopstart):0;
  chn.fpos=0;
  chn.pos=pos-c.startpos;
  if (filter)
    chn.status|=MIX_INTERPOLATE;
  if (chn.pos>=chn.length)
    return;

  chn.status|=MIX_PLAYING;
}











static void Pause(unsigned char p)
{
  if (p==paused)
    return;
  int i;
  if (paused)
  {
    for (i=0; i<channelnum; i++)
    {
      int c=channels[i].awechan;
      if (c!=-1)
      {
        outdAWE(0x400,0x00+c,channels[i].pausepos);
        outwAWE(0x400,0xA0+c,channels[i].pausevol);
      }
    }
    stimerpos=0;
    paused=0;
  }
  else
  {
    paused=1;
    for (i=0; i<channelnum; i++)
    {
      int c=channels[i].awechan;
      if (c!=-1)
      {
        channels[i].pausepos=indAWE(0x400,0x00+c);
        channels[i].pausevol=inwAWE(0x400,0xA0+c);
        outwAWE(0x400,0xA0+c,0x807F);
      }
    }
  }
}



static void SET(int ch, int opt, int val)
{
  switch (opt)
  {
  case mcpGSpeed:
    orgspeed=val;
    break;
  case mcpCInstrument:
    channels[ch].cursamp=val;
    channels[ch].stopit=1;
    channels[ch].nextpos=-1;
    channels[ch].inited=1;
    channels[ch].curloop=1;
    break;
  case mcpCLoop:
    if (channels[ch].curloop==val)
      break;
    channels[ch].curloop=val;
    channels[ch].setloop=1;
    break;
  case mcpCMute:
    channels[ch].pause=val;
    break;
  case mcpCStatus:
    if (!val)
    {
      channels[ch].nextpos=-1;
      channels[ch].stopit=1;
    }
    break;
  case mcpCReset:
    int reswasch,reswasmute,reswasplaying;
    reswasch=channels[ch].awechan;
    reswasmute=channels[ch].pause;
    reswasplaying=channels[ch].playing;
    memset(channels+ch, 0, sizeof(awechan));
    channels[ch].playing=reswasplaying;
    channels[ch].stopit=1;
    channels[ch].pause=reswasmute;
    channels[ch].awechan=reswasch;
    calcrevcho(channels[ch]);
    break;
  case mcpCVolume:
    channels[ch].orgvol=(val<0)?0:(val>0x100)?0x100:val;
    calcvols(channels[ch]);
    break;
  case mcpCPanning:
    channels[ch].orgpan=(val<-0x80)?-0x80:(val>0x80)?0x80:val;
    calcvols(channels[ch]);
    break;
  case mcpCPosition:
    channels[ch].nextpos=val;
    channels[ch].stopit=1;
    break;
  case mcpCPitch:
    channels[ch].awepitch=val*4/3;
    break;
  case mcpCPitchFix:
    channels[ch].awepitch=mcpGetNote8363(8363*val/0x10000)*4/3;
    break;
  case mcpCPitch6848:
    channels[ch].awepitch=mcpGetNote8363(8363*6848/val)*4/3;
    break;
  case mcpCChorus:
    channels[ch].orgchorus=val;
    calcrevcho(channels[ch]);
    break;
  case mcpCReverb:
    channels[ch].orgreverb=val;
    calcrevcho(channels[ch]);
    break;
  case mcpMasterAmplify:
    amplify=val;
    recalcvols();
    if (channelnum)
      mixSetAmplify(amplify);
    break;
  case mcpMasterPause:
    Pause(val);
    break;
  case mcpMasterChorus:
    masterchorus=(val>=64)?63:(val<=-64)?-64:val;
    recalcrevcho();
    break;
  case mcpMasterReverb:
    masterreverb=(val>=64)?63:(val<=-64)?-64:val;
    recalcrevcho();
    break;
  case mcpMasterVolume:
    mastervol=val;
    recalcvols();
    break;
  case mcpMasterPanning:
    masterpan=val;
    recalcvols();
    break;
  case mcpMasterBalance:
    masterbal=val;
    recalcvols();
    break;
  case mcpMasterSpeed:
    relspeed=(val<16)?16:val;
    break;
  case mcpMasterPitch:
    masterawepitch=mcpGetNote8363((8363*val)>>8)*4/3;
    break;
  case mcpMasterFilter:
    filter=val;
    break;
  }
}

static int GET(int ch, int opt)
{
  switch (opt)
  {
  case mcpCStatus:
    return channels[ch].playing?1:0;
  case mcpCMute:
    return channels[ch].pause?1:0;
  case mcpGTimer:
    return tmGetTimer();
  case mcpGCmdTimer:
    return umulshr16(cmdtimerpos, 3600);
  }
  return 0;
}







static int OpenPlayer(int chan, void (*proc)())
{
  if (chan>30)
    chan=30;

  if (!mixInit(GetMixChannel, 1, chan, amplify))
    return 0;

  orgspeed=50*256;

  memset(channels, 0, sizeof(awechan)*chan);
  playerproc=proc;

  int i;

  for (i=0; i<30; i++)
    channels[i].awechan=i;
  channels[30].awechan=-1;
  channels[31].awechan=-1;

  initawechan();
  channelnum=chan;

  for (i=0; i<channelnum; i++)
    calcrevcho(channels[i]);

  cmdtimerpos=0;
  stimerlen=umuldiv(256, 1193046*256, orgspeed*relspeed);
  stimerpos=stimerlen;
  tmInit(timerrout, (stimerpos<=65536)?stimerpos:65536, 8192);

  mcpNChan=chan;

  return 1;
}

static void ClosePlayer()
{
  mcpNChan=0;

  tmClose();
  initawe();
  channelnum=0;
  mixClose();
}



static int inita(const deviceinfo &c)
{
  if (!aweTestPort(c.port))
    return 0;
  awePort=c.port;
  initawe();
  initawechan();
  if (!aweGetMem())
    return 0;

  int i;

  channelnum=0;
  filter=0;

  mempos=0x200000;

  relspeed=256;
  paused=0;

  mastervol=64;
  masterpan=64;
  masterbal=0;
  masterawepitch=0;
  masterchorus=0;
  masterreverb=0;
  amplify=65536;

  mcpOpenPlayer=OpenPlayer;
  mcpClosePlayer=ClosePlayer;
  mcpLoadSamples=LoadSamples;
  mcpSet=SET;
  mcpGet=GET;

  return 1;
}


static void closea()
{
  mcpOpenPlayer=0;
}

static int detecta(deviceinfo &c)
{
  if (!getcfg())
  {
    if (c.port==-1)
      return 0;
    awePort=c.port;
  }
  else
    if (c.port!=-1)
      awePort=c.port;

  if (!aweTestPort(awePort))
    return 0;
  initawe();
  initawechan();
  if (!aweGetMem())
    return 0;

  c.dev=&mcpSoundBlaster32;
  c.port=awePort;
  c.subtype=-1;
  c.irq=-1;
  c.irq2=-1;
  c.dma=-1;
  c.dma2=-1;
  c.chan=30;
  c.mem=aweMem*2;
  return 1;
}

extern "C" sounddevice mcpSoundBlaster32={SS_WAVETABLE, "SoundBlaster 32", detecta, inita, closea};
