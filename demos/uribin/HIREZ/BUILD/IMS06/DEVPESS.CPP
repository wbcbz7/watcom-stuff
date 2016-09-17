// (c) 1994-1997 Niklas Beisert, this file is part of "Indoor Music System".
// you must not read, modify, print, compile or copy this file or parts of
// it unless you have accepted the license in the accompanying file IMS.TXT.

#include <stdlib.h>
#include <string.h>
#include <conio.h>
#include "irq.h"
#include "dma.h"
#include "imsdev.h"
#include "player.h"
#include "imsrtns.h"

extern "C" extern sounddevice plrESSAudioDrive;

static signed short sbPort;
static signed char sbIRQ;
static signed char sbDMA;

static unsigned char getcfg()
{
  sbPort=-1;
  sbIRQ=-1;
  sbDMA=-1;
  char *s=getenv("BLASTER");
  if (!s)
    return 0;
  while (1)
  {
    while (*s==' ')
      s++;
    if (!*s)
      break;
    switch (*s++)
    {
    case 'a': case 'A':
      sbPort=strtoul(s, 0, 16);
      break;
    case 'i': case 'I':
      sbIRQ=strtoul(s, 0, 10);
      break;
    case 'd': case 'D':
      sbDMA=strtoul(s, 0, 10);
      break;
    }
    while ((*s!=' ')&&*s)
      s++;
  }
  return 1;
}

static unsigned char inpSB(unsigned char p)
{
  return inp(sbPort+p);
}

static void outpSB(unsigned char p, unsigned char v)
{
  outp(sbPort+p,v);
}

static void outSB(unsigned char v)
{
  int i;
//  for (i=0; (i<10000)&&(inpSB(0xC)&0x80); i++);
  for (i=0; i<65536; i++);
  outpSB(0xC,v);
}

static unsigned char inSB()
{
  int i;
  for (i=0; i<65536; i++);
//  for (i=0; (i<10000)&&!(inpSB(0xE)&0x80); i++);
  return inpSB(0xA);
}

static void initSB()
{
  outpSB(0x6,3);
  inpSB(0x6);
  inpSB(0x6);
  inpSB(0x6);
  inpSB(0x6);
  inpSB(0x6);
  inpSB(0x6);
  inpSB(0x6);
  inpSB(0x6);
  outpSB(0x6,0);
}

static void biosdelay(unsigned long c)
{
  volatile unsigned long &biosclock=*(volatile unsigned long*)0x46C;
  unsigned long t0=biosclock;
  while ((biosclock-t0)<c)
    break;
}

static void initSBp()
{
  outpSB(0x6,3);
  biosdelay(3);
  outpSB(0x6,0);
  biosdelay(3);
}

static void initESS()
{
  int tmp;

  outSB(0xC6);

  outSB(0xB8);
  outSB(0x04);

  outSB(0xC0);
  outSB(0xA8);
  tmp=(inSB()&~3)+1;

  outSB(0xA8);
  outSB(tmp);

  outSB(0xB6);
  outSB(0x00);

  outSB(0xB7);
  outSB(0x71);

  outSB(0xB7);
  outSB(0xBC);

  outSB(0xC0);
  outSB(0xB2);
  tmp=inSB()|0x50;
  outSB(0xB2);
  outSB(tmp);
}

static void resetSB()
{
  inpSB(0xE);
}

static void spkronSB()
{
  outSB(0xD1);
}

static void spkroffSB()
{
  outSB(0xD3);
}

static char testPort(unsigned short port, int delay)
{
  sbPort=port;
  if (delay)
    initSBp();
  else
    initSB();
  int i;
  for (i=0; i<1000; i++)
    if (inpSB(0xE)&0x80)
      break;
  if (i==1000)
    return 0;
  if (inpSB(0xA)!=0xAA)
    return 0;
  outSB(0xE7);
  for (i=0; i<1000; i++)
    if (inpSB(0xE)&0x80)
      break;
  if (i==1000)
    return 0;
  if (inpSB(0xA)!=0x68)
    return 0;
  return 1;
}

static long playpos;
static long buflen;
static int lastpos=17977;
static void (*playproc)();
static int regengetpos()
{
  int p=dmaGetBufPos();
/*
  if (p==lastpos)
  {
    playproc();
    irqReInit();
  }
  lastpos=p;
*/
  return p;
}

static void advance(int pos)
{
  playpos+=(pos-(playpos%buflen)+buflen)%buflen;
}

static void sb1Player()
{
  outSB(0xA4);
  outSB(0x80);
  outSB(0xA5);
  outSB(0x80);

  outSB(0xB8);
  outSB(0x00);
  outSB(0xB8);
  outSB(0x05);
}

static unsigned char sbTimerConst;
static unsigned char sbStereo;
static unsigned char sb16Bit;
static unsigned char sbSignedOut;
static __segment dmabufsel;

static long gettimer()
{
  return imuldiv(playpos+(dmaGetBufPos()-playpos%buflen+buflen)%buflen, 65536, plrRate<<(sbStereo+sb16Bit));
}

static void sbSetOptions(int rate, int opt)
{
  opt&=~(PLR_STEREO|PLR_16BIT|PLR_SIGNEDOUT);
  opt|=PLR_STEREO|PLR_16BIT|PLR_SIGNEDOUT;

  sbStereo=!!(opt&PLR_STEREO);
  sb16Bit=!!(opt&PLR_16BIT);
  sbSignedOut=!!(opt&PLR_SIGNEDOUT);

  if (rate<4000)
    rate=4000;
  if (rate>44100)
    rate=44100;
  if (rate>22094)
  {
    sbTimerConst=256-795500/rate;
    rate=795500/(256-sbTimerConst);
  }
  else
  {
    sbTimerConst=128-397700/rate;
    rate=397700/(128-sbTimerConst);
  }

  plrRate=rate;
  plrOpt=opt;
}

static int sbPlay(void *&buf, int &len)
{
  buf=(unsigned char*)dmaAlloc(len, dmabufsel);
  if (!buf)
    return 0;
  memsetd(buf, (plrOpt&PLR_SIGNEDOUT)?0:(plrOpt&PLR_16BIT)?0x80008000:0x80808080, len>>2);

  initSBp();
  inSB();
  resetSB();
  initESS();

  outSB(0xA1);
  outSB(sbTimerConst);
  spkronSB();

  playproc=sb1Player;

  irqInit(sbIRQ, playproc, 0, 512);
  dmaStart(sbDMA, buf, len, 0x58);

  outSB(0xA4);
  outSB(0x80);
  outSB(0xA5);
  outSB(0x80);

  outSB(0xB8);
  outSB(0x00);
  outSB(0xB8);
  outSB(0x05);

  buflen=len;
  playpos=-buflen;

  plrGetBufPos=dmaGetBufPos;
  plrGetPlayPos=dmaGetBufPos;
  plrAdvanceTo=advance;
  plrGetTimer=gettimer;

  return 1;
}

static void sbStop()
{
  irqClose();
  dmaStop();
  initSBp();
  inSB();
  resetSB();
  dmaFree(dmabufsel);
}



static int sbInit(const deviceinfo &card)
{
  if (!testPort(card.port, 1))
    return 0;
  resetSB();

  sbPort=card.port;
  sbIRQ=card.irq;
  sbDMA=card.dma;

  plrSetOptions=sbSetOptions;
  plrPlay=sbPlay;
  plrStop=sbStop;

  return 1;
}

static void sbClose()
{
  plrPlay=0;
}

static int sbDetect(deviceinfo &card)
{
  getcfg();
  if (card.port!=-1)
    sbPort=card.port;
  if (card.irq!=-1)
    sbIRQ=card.irq;
  if (card.dma!=-1)
    sbDMA=card.dma;

  int i;
  if (sbPort==-1)
  {
    unsigned short ports[]={0x220, 0x230, 0x240, 0x250};
    for (i=0; i<(sizeof(ports)>>1); i++)
      if (testPort(ports[i],0))
        break;
    if (i==(sizeof(ports)>>1))
      return 0;
    sbPort=ports[i];
  }
  else
    if (!testPort(sbPort, 1))
      return 0;

  if (sbIRQ==-1)
  {
    return 0;
    outSB(0xC0);
    outSB(0xB1);
    int b=inSB()&0xC;
    sbIRQ=(b==0xC)?10:(b==0x0)?9:(b==0x8)?7:5; //????
  }

  if (sbDMA==-1)
  {
    return 0;
    outSB(0xC0);
    outSB(0xB2);
    int b=inSB()&0xC;
    sbDMA=(b==0xC)?3:(b==0x8)?1:0;
  }

  card.dev=&plrESSAudioDrive;
  card.port=sbPort;
  card.port2=-1;
  card.irq=sbIRQ;
  card.irq2=-1;
  card.dma=sbDMA;
  card.dma2=-1;
  card.subtype=-1;
  card.mem=0;
  card.chan=2;

  return 1;
}

extern "C" sounddevice plrESSAudioDrive={SS_PLAYER, "ESS AudioDrive 688", sbDetect, sbInit, sbClose};
