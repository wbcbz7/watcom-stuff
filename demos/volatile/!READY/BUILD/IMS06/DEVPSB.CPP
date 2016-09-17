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

#define SS_SB_REVSTEREO 1


extern "C" extern sounddevice plrSoundBlaster;

static signed short sbPort;
static signed char sbIRQ;
static signed char sbDMA;
static signed char sbDMA16;
static signed char sbUseVer;
static volatile char rightirq;

static unsigned char getcfg()
{
  sbPort=-1;
  sbIRQ=-1;
  sbDMA=-1;
  sbDMA16=-1;
  sbUseVer=-1;
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
    case 'h': case 'H':
      sbDMA16=strtoul(s, 0, 10);
      break;
    case 't': case 'T':
      sbUseVer=strtoul(s, 0, 10);
      switch (sbUseVer)
      {
      case 6: sbUseVer=4; break;
      case 2: case 4: sbUseVer=3; break;
      case 3: sbUseVer=2; break;
      case 1: sbUseVer=1; break;
      default: sbUseVer=-1;
      }
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
  while (inpSB(0xC)&0x80);
  outpSB(0xC,v);
}

static unsigned char inSB()
{
  while (!(inpSB(0xE)&0x80));
  return inpSB(0xA);
}

static void initSB()
{
  outpSB(0x6,1);
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
  outpSB(0x6,1);
  biosdelay(3);
  outpSB(0x6,0);
  biosdelay(3);
}

static void setrateSB(unsigned char r)
{
  outSB(0x40);
  outSB(r);
}

static void setrateSB16(unsigned short r)
{
  outSB(0x41);
  outSB(r>>8);
  outSB(r);
}

static void resetSB()
{
  inpSB(0xE);
}

static void resetSB16()
{
  inpSB(0xF);
}

static void spkronSB()
{
  outSB(0xD1);
}

static void spkroffSB()
{
  outSB(0xD3);
}

static void writeMixer(unsigned char p, unsigned char v)
{
  outpSB(0x4, p);
  outpSB(0x5, v);
}

static unsigned char readMixer(unsigned char p)
{
  outpSB(0x4, p);
  return inpSB(0x5);
}

static void test()
{
  rightirq=1;
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
      return inpSB(0xA)==0xAA;
  return 0;
}

static char testirq(unsigned char irq)
{
  irqInit(irq, test, 0, 512);

  initSB();
  setrateSB(0xD3);

  rightirq=0;

  outSB(0x80);
  outSB(0x03);
  outSB(0x00);
  int i;
  for (i=0; i<0x10000; i++)
    if (rightirq)
      break;

  irqClose();

  initSB();
  resetSB();

  return rightirq;
}

static char testdma(unsigned char irq, unsigned char dma)
{
  outp(0x0A,4);
  outp(0x0A,5);
  outp(0x0A,7);

  irqInit(irq, test, 0, 512);
  dmaStart(dma, 0, 10, 0x48);

  initSB();
  setrateSB(0xD3);

  rightirq=0;

  outSB(0x14);
  outSB(0x03);
  outSB(0x00);
  for (unsigned short i=0; i!=0xFFFF; i++)
    if (rightirq)
      break;
  irqClose();
  dmaStop();

  initSB();
  resetSB();

  return rightirq;
}

static char testdma16(unsigned char irq, unsigned char dma)
{
  outp(0x0A,4);
  outp(0x0A,5);
  outp(0x0A,7);
  outp(0xD4,5);
  outp(0xD4,6);
  outp(0xD4,7);

  irqInit(irq, test, 0, 512);
  dmaStart(dma, 0, 2, 0x48);

  initSB();
  setrateSB(0xEA);

  rightirq=0;

  outSB(0xB6);
  outSB(0x10);
  outSB(0x00);
  outSB(0x00);
  for (unsigned i=0; i!=0xFFFF; i++)
    if (rightirq)
      break;
  irqClose();
  dmaStop();

  initSB();
  resetSB16();

  return rightirq;
}

static unsigned short getVersion()
{
  outSB(0xE1);
  while(1)
  {
    unsigned char verhi=inSB();
    if (verhi==0xAA)
      continue;
    return ((unsigned short)verhi<<8)+inSB();
  }
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
  resetSB();
  outSB(0x14);
  outSB(0xFF);
  outSB(0xFF);
}

static void sb2proPlayerHS()
{
  resetSB();
  outSB(0x91);
}

static void sb16Player8()
{
  resetSB();
  outSB(0x45);
}

static void sb16Player16()
{
  outSB(0x47);
  resetSB16();
}

static int sbRate;
static unsigned char sbTimerConst;
static unsigned char sbStereo;
static unsigned char sb16Bit;
static unsigned char sbSignedOut;
static unsigned char revstereo;
static __segment dmabufsel;

static void sbSetOptions(int rate, int opt)
{
  switch (sbUseVer)
  {
  case 1: case 2:
    opt&=~(PLR_STEREO|PLR_16BIT|PLR_SIGNEDOUT);
    break;
  case 3:
    opt&=~(PLR_16BIT|PLR_SIGNEDOUT);
    break;
  }
  if (revstereo)
    opt^=PLR_REVERSESTEREO;
  sbStereo=!!(opt&PLR_STEREO);
  sb16Bit=!!(opt&PLR_16BIT);
  sbSignedOut=!!(opt&PLR_SIGNEDOUT);

  unsigned long rt=rate;
  if ((sbUseVer==3)&&sbStereo)
    rt<<=1;

  if (rt<4000)
    rt=4000;
  if (sbUseVer==4)
    if (rt<5000)
      rt=5000;
  if ((sbUseVer==2)||(sbUseVer==3))
    if (rt<8000)
      rt=8000;

  if (rt>45454)
    rt=45454;
  if ((sbUseVer==2)||(sbUseVer==3))
    if (rt>43478)
      rt=43478;
  if ((sbUseVer==1))
    if (rt>22222)
      rt=22222;

  sbTimerConst=256-1000000/rt;
  if (sbUseVer!=4)
    rate=1000000/(256-sbTimerConst);

  if ((sbUseVer==3)&&sbStereo)
    rate>>=1;

  sbRate=rate;
  plrRate=rate;
  plrOpt=opt;
}

static long gettimer()
{
  return imuldiv(playpos+(dmaGetBufPos()-playpos%buflen+buflen)%buflen, 65536, sbRate<<(sbStereo+sb16Bit));
}

static int sbPlay(void *&buf, int &len)
{
  buf=(unsigned char*)dmaAlloc(len, dmabufsel);
  if (!buf)
    return 0;
  memsetd(buf, (plrOpt&PLR_SIGNEDOUT)?0:(plrOpt&PLR_16BIT)?0x80008000:0x80808080, len>>2);

  initSBp();
  resetSB();
  resetSB16();
  if (sbUseVer==4)
    setrateSB16(sbRate);
  else
  {
    setrateSB(sbTimerConst);
    setrateSB(sbTimerConst);
  }

  spkronSB();

  switch (sbUseVer)
  {
  case 1:
    playproc=sb1Player;
    break;
  case 2: case 3:
    playproc=sb2proPlayerHS;
    break;
  case 4:
    playproc=sb16Bit?sb16Player16:sb16Player8;
    break;
  }

  irqInit(sbIRQ, playproc, 0, 512);
  dmaStart(sb16Bit?sbDMA16:sbDMA, buf, len, 0x58);

  switch (sbUseVer)
  {
  case 1:
    outSB(0x14);
    outSB(0xFF);
    outSB(0xFF);
    break;
  case 2: case 3:
    outSB(0x48);
    outSB(0xFF);
    outSB(0xFF);
    outSB(0x91);
    if (sbUseVer==3)
      writeMixer(0xE, readMixer(0xE)&~2|(sbStereo?0x22:0x20));
    break;
  case 4:
    outSB(sb16Bit?0xB6:0xC6);
    outSB((sbStereo?0x20:0x00)|(sbSignedOut?0x10:0x00));
    outSB(0xFC);
    outSB(0xFF);
    break;
  }

  buflen=len;
  playpos=-buflen;
  plrGetBufPos=regengetpos;
  plrGetPlayPos=regengetpos;
  plrAdvanceTo=advance;
  plrGetTimer=gettimer;

  return 1;
}

static void sbStop()
{
  irqClose();
  dmaStop();
  initSBp();
  resetSB();
  resetSB16();
  dmaFree(dmabufsel);
}



static int sbInit(const deviceinfo &card)
{
  if (!testPort(card.port, 1))
    return 0;
  resetSB();
  resetSB16();

  revstereo=!!(card.opt&SS_SB_REVSTEREO);

  sbPort=card.port;
  sbIRQ=card.irq;
  sbDMA=card.dma;
  sbDMA16=card.dma2;
  unsigned char ver=getVersion()>>8;
  if (card.subtype==-1)
    sbUseVer=ver;
  else
    sbUseVer=card.subtype;
  if (sbUseVer>ver)
    return 0;
  if ((sbUseVer==3)&&(ver==4))
    return 0;

/*
  if (!testirq(sbIRQ))
    return 0;
  if (!testdma(sbIRQ, sbDMA))
    return 0;
  if (sbUseVer==4)
    if (!testdma16(sbIRQ, sbDMA16))
      return 0;
*/

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
  if (card.dma2!=-1)
    sbDMA16=card.dma2;
  if (card.subtype!=-1)
    sbUseVer=card.subtype;

  int i;
  if (sbPort==-1)
  {
    unsigned short ports[8]={0x220, 0x240, 0x260, 0x280, 0x210, 0x230, 0x250};
    for (i=0; i<(sizeof(ports)>>1); i++)
      if (testPort(ports[i],0))
        break;
    if (i==(sizeof(ports)>>1))
      return 0;
    sbPort=ports[i];
  }
  else
    if (!testPort(sbPort, 0))
      return 0;

  unsigned short ver=getVersion()>>8;
  if (sbUseVer==-1)
    sbUseVer=ver;
  if ((sbUseVer<1)||(sbUseVer>4))
    return 0;

  if (ver<sbUseVer)
    return 0;
  if ((ver==4)&&(sbUseVer==3))
    return 0;

  if (sbIRQ==-1)
  {
    if (sbUseVer!=4)
    {
      unsigned char irqs[5]={7, 5, 2, 3, 10};
      for (i=0; i<sizeof(irqs); i++)
        if (testirq(irqs[i]))
          break;
      if (i==sizeof(irqs))
        return 0;
      sbIRQ=irqs[i];
    }
    else
    {
      unsigned char a=readMixer(0x80);
      if (!(a&0x0F))
        return 0;
      sbIRQ=(a&2)?5:(a&4)?7:(a&1)?2:10;
    }
  }
//  else
//    if (!testirq(sbIRQ))
//      return 0;

  if (sbDMA==-1)
  {
    if (sbUseVer!=4)
    {
      unsigned char dmas[3]={1, 0, 3};
      for (i=0; i<sizeof(dmas); i++)
        if (testdma(sbIRQ, dmas[i]))
          break;
      if (i==sizeof(dmas))
        return 0;
      sbDMA=dmas[i];
    }
    else
    {
      unsigned char b=readMixer(0x81);
      if (!(b&0x0B))
        return 0;
      sbDMA=(b&2)?1:(b&1)?0:3;
    }
  }
//  else
//    if (!testdma(sbIRQ, sbDMA))
//      return 0;

  if (sbUseVer==4)
  {
    if (sbDMA16==-1)
    {
      unsigned char b=readMixer(0x81);
      if (!(b&0x0B))
        return 0;
      sbDMA16=(b&0x20)?5:(b&0x40)?6:(b&0x80)?7:sbDMA;
    }
//    else
//      if (!testdma16(sbIRQ, sbDMA16))
//        return 0;
  }
  else
    sbDMA16=-1;

  card.dev=&plrSoundBlaster;
  card.port=sbPort;
  card.port2=-1;
  card.irq=sbIRQ;
  card.irq2=-1;
  card.dma=sbDMA;
  card.dma2=sbDMA16;
  card.subtype=sbUseVer;
  card.mem=0;
  card.chan=(card.subtype<3)?1:2;

  return 1;
}

extern "C" sounddevice plrSoundBlaster={SS_PLAYER, "SoundBlaster", sbDetect, sbInit, sbClose};

#ifdef CPDOS

#include "devigen.h"
#include "psetting.h"

static unsigned long sbGetOpt(const char *sec)
{
  unsigned long opt=0;
  if (cfGetProfileBool(sec, "sbrevstereo", 0, 0))
    opt|=SS_SB_REVSTEREO;
  return opt;
}

extern "C" devaddstruct plrSBAdd = {sbGetOpt, 0, 0, 0};

#endif
