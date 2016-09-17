// (c) 1994-1997 Niklas Beisert, this file is part of "Indoor Music System".
// you must not read, modify, print, compile or copy this file or parts of
// it unless you have accepted the license in the accompanying file IMS.TXT.

#include <stdlib.h>
#include <string.h>
#include <conio.h>
#include "dma.h"
#include "imsdev.h"
#include "player.h"
#include "imsrtns.h"

#define SS_WSS_64000 1

extern "C" extern sounddevice plrWinSoundSys;

static unsigned short gusPort;
static unsigned char gusDMA;
static unsigned char gusDMA2;
static unsigned short wssPort;
static unsigned char wssDMA;
static unsigned char wssDMA2;
static unsigned char wssType;
static unsigned char gusmaxwssSetup;
static unsigned char oldmuteleft, oldmuteright, oldmutemono;
static unsigned char wssSpeed;
static unsigned int wssMaxRate;
static long wssBPS;
static long playpos;
static long buflen;
static __segment dmabufsel;


static unsigned char wssinp(unsigned short r)
{
  return inp(wssPort+r);
}

static void wssoutp(unsigned short r, unsigned char v)
{
  outp(wssPort+r, v);
}

static unsigned char wssin(unsigned char r)
{
  wssoutp(4, r);
  return wssinp(5);
}

static void wssout(unsigned char r, unsigned char v)
{
  wssoutp(4, r);
  wssoutp(5, v);
}

static char wssGetCfg()
{
  if (!getenv("ULTRA16"))
    return 0;

  char *ptr=getenv("ULTRA16");
  while (*ptr==' ')
    ptr++;
  if (!*ptr)
    return 0;

  wssPort=strtoul(ptr, 0, 16);
  while ((*ptr!=',')&&*ptr)
    ptr++;
  if (!*ptr++)
    return 0;
  wssDMA=wssDMA2=strtoul(ptr, 0, 10)&7;
  while ((*ptr!=',')&&*ptr)
    ptr++;
  if (!*ptr++)
    return 0;
  strtoul(ptr, 0, 10)&15;
  while ((*ptr!=',')&&*ptr)
    ptr++;
  if (!*ptr++)
    return 0;
  wssType=strtoul(ptr, 0, 10)+1;

  if (wssType==2)
  {
    ptr=getenv("ULTRASND");
    while (*ptr==' ')
      ptr++;
    if (!*ptr)
      return 0;
    gusPort=strtoul(ptr, 0, 16);
    while ((*ptr!=',')&&*ptr)
      ptr++;
    if (!*ptr++)
      return 0;
    wssDMA2=gusDMA=strtoul(ptr, 0, 10)&7;
    while ((*ptr!=',')&&*ptr)
      ptr++;
    if (!*ptr++)
      return 0;
    wssDMA=gusDMA2=strtoul(ptr, 0, 10)&7;
    while ((*ptr!=',')&&*ptr)
      ptr++;
    if (!*ptr++)
      return 0;
    strtoul(ptr, 0, 10)&15;
    while ((*ptr!=',')&&*ptr)
      ptr++;
    if (!*ptr++)
      return 0;
    strtoul(ptr, 0, 10)&15;
  }

  return 1;
}

static void wssSetOptions(int rate, int opt)
{
  char stereo=!!(opt&PLR_STEREO);
  char bit16=!!(opt&PLR_16BIT);
  if (bit16)
    opt|=PLR_SIGNEDOUT;
  else
    opt&=~PLR_SIGNEDOUT;

  if (rate>wssMaxRate)
    rate=wssMaxRate;

  static unsigned short rates[16]=
  { 8000, 5513,16000,11025,27429,18900,32000,22050,
   54857,37800,64000,44100,48000,33075, 9600, 6615};

// bit   0=> base frq: 384000,264600
// bit 3-1=> frq div:  48,24,14,12,7,6,8,40

  unsigned char spd=10;
  int i;
  for (i=0; i<16; i++)
    if ((rates[i]>=rate)&&(rates[i]<rates[spd]))
      spd=i;

  rate=rates[spd];

  wssSpeed=spd|(bit16?0x40:0x00)|(stereo?0x10:0x00);

  wssBPS=rate<<(stereo+bit16);
  plrRate=rate;
  plrOpt=opt;
}

static void advance(int pos)
{
  playpos+=(pos-(playpos%buflen)+buflen)%buflen;
}

static long gettimer()
{
  return imuldiv(playpos+(dmaGetBufPos()-playpos%buflen+buflen)%buflen, 65536, wssBPS);
}

static int wssPlay(void *&buf, int &len)
{
  buf=(unsigned char*)dmaAlloc(len, dmabufsel);
  if (!buf)
    return 0;
  memsetd(buf, (plrOpt&PLR_SIGNEDOUT)?0:(plrOpt&PLR_16BIT)?0x80008000:0x80808080, len>>2);

  wssout(0x48, wssSpeed);
  wssout(0x48, wssSpeed);
  wssinp(5);
  wssinp(5);

  while (wssinp(4)&0x80);

  while (wssinp(4)!=0x08)
    wssoutp(4, 0x08);

  while (wssinp(4)!=0x0B)
    wssoutp(4, 0x0B);

  while (wssinp(5)&0x20)
    wssoutp(4, 0x0B);

  if (wssType==2)
  {
    if (wssDMA&4)
    {
      outp(gusPort+0x106, gusmaxwssSetup&~0x20);
      outp(gusPort+0x106, gusmaxwssSetup);
    }
  }

  wssout(0x0C, 0);

  dmaStart(wssDMA, buf, len, 0x58);

  wssout(0x0F, 0xF0);
  wssout(0x0E, 0xFF);

  wssout(0x09, ((wssDMA==wssDMA2)&&(wssType!=2))?5:1);

  oldmuteleft=wssin(0x06);
  oldmuteright=wssin(0x07);
  oldmutemono=wssin(0x1A);
  wssout(0x1A, oldmutemono&~0x40);
  wssout(0x06, oldmuteleft&~0x80);
  wssout(0x07, oldmuteright&~0x80);

  plrGetBufPos=dmaGetBufPos;
  plrGetPlayPos=dmaGetBufPos;
  plrAdvanceTo=advance;
  plrGetTimer=gettimer;
  buflen=len;
  playpos=-buflen;

  return 1;
}

static void wssStop()
{
  wssout(0x06, oldmuteleft);
  wssout(0x07, oldmuteright);
  wssout(0x1A, oldmutemono);
  wssout(0x0A, 0);
//  wssinp(6);
//  wssoutp(6, 0);
  wssout(0x09, ((wssDMA==wssDMA2)&&(wssType!=2))?4:0);
  dmaStop();
  dmaFree(dmabufsel);
}

static void waste_time(unsigned int val)
{
  int i,j;
  for (i=0; i<val; i++)
    for (j=0; j<1000; j++);
}

static int wssInit(const deviceinfo &card)
{
  if ((card.subtype!=-1)&&((card.subtype<0)||(card.subtype>2)))
    return 0;

  if (card.opt&SS_WSS_64000)
    wssMaxRate=64000;
  else
    wssMaxRate=48000;
  if (card.subtype==-1)
    wssType=0;
  else
    wssType=card.subtype;
  wssDMA=gusDMA2=card.dma;
  if (card.dma2!=-1)
    wssDMA2=gusDMA=card.dma2;
  else
    wssDMA2=gusDMA=card.dma;
  wssPort=card.port-(wssType?4:0);
  gusPort=card.port2;

  if (wssType==2)
  {
    // setup normal gus...

    gusmaxwssSetup=((wssPort-0x308)>>4)|0x40;
    if (wssDMA2&4)
      gusmaxwssSetup|=0x10;
    if (wssDMA&4)
      gusmaxwssSetup|=0x20;
    outp(gusPort+0x106, gusmaxwssSetup);
    waste_time(100);
  }
/*
  if (wssType==0)
  {
    int cfg;
    switch (card.dma)
    {
    case 0: cfg=0x01; break;
    case 1: cfg=0x02; break;
    case 3: cfg=0x03; break;
    default:
      return 0;
    }
    switch (card.irq)
    {
    case 7: cfg|=0x08; break;
    case 9: cfg|=0x10; break;
    case 10: cfg|=0x18; break;
    case 11: cfg|=0x20; break;
    }
    wssoutp(0, cfg);
  }
*/

  int i;
  unsigned char ver;

//  wssinp(6);
//  wssoutp(6, 0);

  for (i=0; i<1000; i++)
  {
    if (wssinp(4)&0x80)
      waste_time(1);
    else
    {
      wssout(0x0C, 0);
      ver=wssinp(4)&0x0F;
      if ((ver>=1)&&(ver<15))
        break;
    }
  }

  if (i==1000)
    return 0;

  wssout(0x0C, 0);
  ver=wssinp(5);
  wssoutp(5, 0);
  if (wssinp(5)!=ver)
    return 0;

  if (wssinp(4)&0x80)
    return 0;

  wssout(0x49, ((wssDMA==wssDMA2)&&(wssType!=2))?4:0);
  wssoutp(4, 0x09);

  wssout(0x0C, 0);

  plrSetOptions=wssSetOptions;
  plrPlay=wssPlay;
  plrStop=wssStop;

  return 1;
}

static void wssClose()
{
  plrPlay=0;
}

static int wssDetect(deviceinfo &card)
{
  int type=card.subtype;
  if (type==-1)
  {
    if (!wssGetCfg())
      type=0;
    else
      type=wssType;
  }

  if (type==0)
  {
    if ((card.port==-1)||(card.dma==-1))
      return 0;
    wssType=0;
  }
  else
  {
    if (!wssGetCfg())
    {
      if ((card.port==-1)||(card.dma==-1))
        return 0;
    }
    else
      if (wssType!=type)
        return 0;
  }

  card.dev=&plrWinSoundSys;
  if (card.port==-1)
    card.port=wssPort;
  if (card.dma==-1)
    card.dma=wssDMA;
  card.subtype=wssType;
  card.irq=-1;
  card.irq2=-1;
  if (type==2)
  {
    if (card.port2==-1)
      card.port2=gusPort;
    if (card.dma2==-1)
      card.dma2=wssDMA2;
  }
  else
  {
    card.port2=-1;
    card.dma2=-1;
  }
  card.mem=0;
  card.chan=2;

  return 1;
}

extern "C" sounddevice plrWinSoundSys={SS_PLAYER, "Windows Sound System", wssDetect, wssInit, wssClose};

#ifdef CPDOS

#include "devigen.h"
#include "psetting.h"

static unsigned long wssGetOpt(const char *sec)
{
  unsigned long opt=0;
  if (cfGetProfileBool(sec, "wss64000", 0, 0))
    opt|=SS_WSS_64000;
  return opt;
}

extern "C" devaddstruct plrWSSAdd = {wssGetOpt, 0, 0, 0};

#endif
