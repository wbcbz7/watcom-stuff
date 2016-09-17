// (c) 1994-1997 Niklas Beisert, this file is part of "Indoor Music System".
// you must not read, modify, print, compile or copy this file or parts of
// it unless you have accepted the license in the accompanying file IMS.TXT.

#include <string.h>
#include <conio.h>
#include <stdlib.h>
#include "imsdev.h"
#include "mcp.h"
#include "irq.h"
#include "dma.h"
#include "player.h"
#include "imsrtns.h"

extern "C" extern sounddevice plrUltraSound;

static unsigned short gusPort;
static unsigned char gusDMA;
static unsigned char gusIRQ;
static unsigned char gusDMA2;
static unsigned char gusIRQ2;
static unsigned char activevoices;


static char getcfg()
{
  char *ptr=getenv("ULTRASND");
  if (!ptr)
    return 0;
  while (*ptr==' ')
    ptr++;
  if (!ptr)
    return 0;
  gusPort=strtoul(ptr, 0, 16);
  while ((*ptr!=',')&&*ptr)
    ptr++;
  if (!*ptr++)
    return 0;
  gusDMA=strtoul(ptr, 0, 10)&7;
  while ((*ptr!=',')&&*ptr)
    ptr++;
  if (!*ptr++)
    return 0;
  gusDMA2=strtoul(ptr, 0, 10)&7;
  while ((*ptr!=',')&&*ptr)
    ptr++;
  if (!*ptr++)
    return 0;
  gusIRQ=strtoul(ptr, 0, 10)&15;
  while ((*ptr!=',')&&*ptr)
    ptr++;
  if (!*ptr++)
    return 0;
  gusIRQ2=strtoul(ptr, 0, 10)&15;
  return 1;
}

static unsigned char inpGUS(unsigned short p)
{
  return inp(gusPort+p);
}

static unsigned short inpwGUS(unsigned short p)
{
  return inpw(gusPort+p);
}

static void delayGUS()
{
  inp(gusPort+0x107);
  inp(gusPort+0x107);
  inp(gusPort+0x107);
  inp(gusPort+0x107);
  inp(gusPort+0x107);
  inp(gusPort+0x107);
  inp(gusPort+0x107);
  inp(gusPort+0x107);
}

static void outpGUS(unsigned short p, unsigned char v)
{
  outp(gusPort+p,v);
}

static void outpwGUS(unsigned short p, unsigned short v)
{
  outpw(gusPort+p,v);
}

static void outGUS(unsigned char c, unsigned char v)
{
  outp(gusPort+0x103, c);
  outp(gusPort+0x105, v);
}

static void outdGUS(unsigned char c, unsigned char v)
{
  outp(gusPort+0x103, c);
  outp(gusPort+0x105, v);
  delayGUS();
  outp(gusPort+0x105, v);
}

static void outwGUS(unsigned char c, unsigned short v)
{
  outp(gusPort+0x103, c);
  outpw(gusPort+0x104, v);
}

static unsigned char inGUS(unsigned char c)
{
  outp(gusPort+0x103, c);
  return inp(gusPort+0x105);
}

static unsigned short inwGUS(unsigned char c)
{
  outp(gusPort+0x103, c);
  return inpw(gusPort+0x104);
}

static void outpGUS0(unsigned char v)
{
  outpGUS(0x00, v);
}

static void outpGUSB(unsigned char v)
{
  outpGUS(0x0B, v);
}

static void outpGUSF(unsigned char v)
{
  outpGUS(0x0F, v);
}

static unsigned char peekGUS(unsigned long adr)
{
  outwGUS(0x43, adr);
  outGUS(0x44, adr>>16);
  return inpGUS(0x107);
}

static void pokeGUS(unsigned long adr, unsigned char data)
{
  outwGUS(0x43, adr);
  outGUS(0x44, adr>>16);
  outpGUS(0x107, data);
}

static void selvoc(char ch)
{
  outpGUS(0x102, ch);
}

static void setfreq(unsigned short frq)
{
  outwGUS(0x01, frq&~1);
}

static void setvol(unsigned short vol)
{
  outwGUS(0x09, vol<<4);
}

static void setpan(unsigned char pan)
{
  outGUS(0x0C, pan);
}

static void setpoint8(unsigned long p, unsigned char t)
{
  t=(t==1)?0x02:(t==2)?0x04:0x0A;
  outwGUS(t, (p>>7)&0x1FFF);
  outwGUS(t+1, p<<9);
}

static unsigned long getpoint8(unsigned char t)
{
  t=(t==1)?0x82:(t==2)?0x84:0x8A;
  return (((inwGUS(t)<<16)|inwGUS(t+1))&0x1FFFFFFF)>>9;
}

static void setmode(unsigned char m)
{
  outdGUS(0x00, m);
}

static unsigned char getmode()
{
  return inGUS(0x80);
}

static void setvmode(unsigned char m)
{
  outdGUS(0x0D, m);
}

static unsigned char getvmode()
{
  return inGUS(0x8D);
}

static void settimer(unsigned char o)
{
  outGUS(0x45, o);
}

static char testPort(unsigned short port)
{
  gusPort=port;

  outGUS(0x4C, 0);

  delayGUS();
  delayGUS();

  outGUS(0x4C, 1);

  delayGUS();
  delayGUS();

  char v0=peekGUS(0);
  char v1=peekGUS(1);

  pokeGUS(0,0xAA);
  pokeGUS(1,0x55);

  char gus=peekGUS(0)==0xAA;

  pokeGUS(0,v0);
  pokeGUS(1,v1);

  return gus;
}

static void initgus(char voices)
{
  if (voices<14)
    voices=14;
  if (voices>32)
    voices=32;

  activevoices=voices;

  int i;

  outGUS(0x4C, 0);
  for (i=0; i<10; i++)
    delayGUS();

  outGUS(0x4C, 1);
  for (i=0; i<10; i++)
    delayGUS();

  outGUS(0x41, 0x00);
  outGUS(0x45, 0x00);
  outGUS(0x49, 0x00);

  outGUS(0xE, (voices-1)|0xC0);

  inpGUS(0x6);
  inGUS(0x41);
  inGUS(0x49);
  inGUS(0x8F);

  for (i=0; i<32; i++)
  {
    selvoc(i);
    setvol(0);  // vol=0
    setmode(3);  // stop voice
    setvmode(3);  // stop volume
    setpoint8(0,0);
    delayGUS();
  }

  inpGUS(0x6);
  inGUS(0x41);
  inGUS(0x49);
  inGUS(0x8F);

  outGUS(0x4C,0x07);

  unsigned char l1="\x00\x00\x01\x03\x00\x02\x00\x04\x00\x00\x00\x05\x06\x00\x00\x07"[gusIRQ]|((gusIRQ==gusIRQ2)?0x40:"\x00\x00\x08\x18\x00\x10\x00\x20\x00\x00\x00\x28\x30\x00\x00\x38"[gusIRQ2]);
  unsigned char l2="\x00\x01\x00\x02\x00\x03\x04\x05"[gusDMA]|((gusDMA==gusDMA2)?0x40:"\x00\x08\x00\x10\x00\x18\x20\x28"[gusDMA2]);

  outpGUSF(5);
  outpGUS0(0x0B);
  outpGUSB(0);
  outpGUSF(0);

  outpGUS0(0x0B);
  outpGUSB(l2|0x80);
  outpGUS0(0x4B);
  outpGUSB(l1);
  outpGUS0(0x0B);
  outpGUSB(l2);
  outpGUS0(0x4B);
  outpGUSB(l1);

  selvoc(0);
  outpGUS0(0x08);
  selvoc(0);
}



static char bit16;
static char stereo;
static char signedout;
static char timeconst;
static char *dmabuf;
static int buflen2;
static long gusBPS;
static long playpos;
static long buflen;
static __segment dmabufsel;


static void dmaupload(int dest, const void *buf, int len)
{
  inGUS(0x41);
  dest>>=4;
  if (gusDMA&4)
    dest>>=1;
  outGUS(0x41, 0);
  dmaStart(gusDMA, (void *)buf, len, 0x08);
  outwGUS(0x42, dest);
  outGUS(0x41, (gusDMA&4)|(signedout?0x00:0x80)|(bit16?0x40:0x00)|0x1);
}


static void handle_voice()
{
  unsigned long wave_ignore=0;
  unsigned long volume_ignore=0;

  while (1)
  {
    unsigned char irq_source=inGUS(0x8F);

    unsigned char voice=irq_source&0x1F;

    if ((irq_source&0xC0)==0xC0)
      break;

    unsigned long voice_bit=1<<voice;

    if (!(irq_source&0x80))
      if (!(wave_ignore&voice_bit))
      {
        selvoc(voice);
	if (!((inGUS(0x80)&0x08)||(inGUS(0x8D)&0x04)))
	  wave_ignore|=voice_bit;
        if ((voice!=0)&&(voice!=1))
          continue;
        if (voice)
        {
          if (bit16)
          {
            pokeGUS((buflen2<<1)+0, dmabuf[0]);
            pokeGUS((buflen2<<1)+1, dmabuf[1]^(signedout?0x00:0x80));
            pokeGUS((buflen2<<1)+2, dmabuf[2]);
            pokeGUS((buflen2<<1)+3, dmabuf[3]^(signedout?0x00:0x80));
          }
          else
          {
            pokeGUS((buflen2<<1)+0, dmabuf[0]^(signedout?0x00:0x80));
            pokeGUS((buflen2<<1)+1, dmabuf[1]^(signedout?0x00:0x80));
          }
          dmaupload(0, dmabuf, buflen2);
        }
        else
          dmaupload(buflen2, dmabuf+buflen2, buflen2);
//        playpos+=buflen2;
      }
    if (!(irq_source&0x40))
      if (!(volume_ignore&voice_bit))
      {
        selvoc(voice);
	if (!(inGUS(0x8D)&0x08))
          volume_ignore|=voice_bit;
      }
  }
}

static void irqrout()
{
  while (1)
  {
    unsigned char source=inpGUS(0x6);
    if (!source)
      break;
    if (source&0x03)
      inpGUS(0x100);
    if (source&0x80)
      inGUS(0x41);
    if (source&0x0C)
      settimer(0x00);
    if (source&0x60)
      handle_voice();
  }
}



static void gusSetOptions(int rate, int opt)
{
  stereo=!!(opt&PLR_STEREO);
  bit16=!!(opt&PLR_16BIT);
  signedout=!!(opt&PLR_SIGNEDOUT);

  if (rate<19293)
    rate=19293;
  if (rate>44100)
    rate=44100;

  timeconst=617400/rate;
  rate=617400/timeconst;

  gusBPS=rate<<(stereo+bit16);
  plrRate=rate;
  plrOpt=opt;
}

static int getpos()
{
  selvoc(0);
  int p=getpoint8(0);
  while (1)
  {
    selvoc(0);
    int p2=getpoint8(0);
    if (abs(p-p2)<0x40)
      break;
    p=p2;
  }

  return ((p<<(bit16+stereo))+buflen2)%(buflen2<<1);
}

static void advance(int pos)
{
  playpos+=(pos-(playpos%buflen)+buflen)%buflen;
}

static long gettimer()
{
  return imuldiv(playpos-((playpos-getpos()+buflen)%buflen), 65536, gusBPS);
}

static int gusPlay(void *&buf, int &len)
{
  buf=(unsigned char*)dmaAlloc(len, dmabufsel);
  if (!buf)
    return 0;
  memsetd(buf, (plrOpt&PLR_SIGNEDOUT)?0:(plrOpt&PLR_16BIT)?0x80008000:0x80808080, len>>2);

  len&=~63;

  initgus(timeconst);

  selvoc(0);
  delayGUS();
  outpGUS0(0x09);
  delayGUS();

// set rate & options

// start playing

  irqInit(gusIRQ, irqrout, 0, 512);

  dmabuf=(char*)buf;
  buflen2=len>>1;

  dmaupload(0, dmabuf, buflen2<<1);

  selvoc(0);
  setpoint8(0,0);
  setpoint8(0,1);
  setpoint8((buflen2<<1)>>(bit16+stereo),2);
  setfreq(1024);

  selvoc(1);
  setpoint8(buflen2>>(bit16+stereo),0);
  setpoint8(0,1);
  setpoint8((buflen2<<1)>>(bit16+stereo),2);
  setfreq(1024);

  if (!stereo)
  {
    selvoc(2);
    setpoint8(0,0);
    setpoint8(0,1);
    setpoint8(buflen2<<(1-bit16),2);
    setfreq(1024);
    setpan(8);
    setvol(0xFFF);
    setmode(bit16?0x0C:0x08);
  }
  else
  {
    selvoc(2);
    setpoint8(0,0);
    setpoint8(0,1);
    setpoint8(buflen2<<(1-bit16),2);
    setfreq(2048);
    setpan(0);
    setvol(0xFFF);

    selvoc(3);
    setpoint8(1,0);
    setpoint8(0,1);
    setpoint8(buflen2<<(1-bit16),2);
    setfreq(2048);
    setpan(15);
    setvol(0xFFF);

    selvoc(2);
    setmode(bit16?0x0C:0x08);
    selvoc(3);
    setmode(bit16?0x0C:0x08);
  }
  selvoc(0);
  setmode(0x28);
  selvoc(1);
  setmode(0x28);

  buflen=len;
  playpos=-buflen;

  plrGetBufPos=getpos;
  plrGetPlayPos=getpos;
  plrAdvanceTo=advance;
  plrGetTimer=gettimer;

  return 1;
}

static void gusStop()
{
// stop playing
  irqClose();
  initgus(14);
  dmaFree(dmabufsel);
}



static int initu(const deviceinfo &c)
{
  int i;
  if (!testPort(c.port))
    return 0;

  gusPort=c.port;
  gusIRQ=c.irq;
  gusDMA=c.dma;
  gusIRQ2=c.irq2;
  gusDMA2=c.dma2;

  initgus(14);

  plrSetOptions=gusSetOptions;
  plrPlay=gusPlay;
  plrStop=gusStop;

  return 1;
}

static void closeu()
{
  plrPlay=0;
}

static int detectu(deviceinfo &c)
{
  if (!getcfg())
  {
    if ((c.port==-1)||(c.irq==-1)||(c.dma==-1))
      return 0;
    gusPort=c.port;
    gusIRQ=c.irq;
    gusIRQ2=(c.irq2==-1)?c.irq:c.irq2;
    gusDMA=c.dma;
    gusDMA2=(c.dma2==-1)?c.dma:c.dma2;
  }
  else
  {
    if (c.port!=-1)
      gusPort=c.port;
    if (c.irq!=-1)
      gusIRQ=c.irq;
    if (c.irq2!=-1)
      gusIRQ2=c.irq2;
    if (c.dma!=-1)
      gusDMA=c.dma;
    if (c.dma2!=-1)
      gusDMA2=c.dma2;
  }

  if (!testPort(gusPort))
    return 0;
  c.subtype=-1;
  c.dev=&plrUltraSound;
  c.port=gusPort;
  c.port2=-1;
  c.irq=(gusIRQ<gusIRQ2)?gusIRQ:gusIRQ2;
  c.irq2=(gusIRQ<gusIRQ2)?gusIRQ2:gusIRQ;
  c.dma=gusDMA;
  c.dma2=gusDMA2;
  c.mem=0;
  c.chan=2;
  return 1;
}

extern "C" sounddevice plrUltraSound={SS_PLAYER, "Gravis UltraSound", detectu, initu, closeu};
