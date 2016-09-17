// (c) 1994-1997 Niklas Beisert, this file is part of "Indoor Music System".
// you must not read, modify, print, compile or copy this file or parts of
// it unless you have accepted the license in the accompanying file IMS.TXT.

#include <string.h>
#include <conio.h>
#include <stdlib.h>
#include "imsdev.h"
#include "mcp.h"
#include "mix.h"
#include "irq.h"
#include "dma.h"
#include "timer.h"
#include "imsrtns.h"

#define MAXSAMPLES 256

#define SS_GUS_FASTUPLOAD 1
#define SS_GUS_GUSTIMER 2

extern "C" extern sounddevice mcpUltraSound;

static unsigned short gusPort;
static unsigned char gusDMA;
static unsigned char gusIRQ;
static unsigned char gusDMA2;
static unsigned char gusIRQ2;
static unsigned long gusMem;
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

static void outpGUS8(unsigned char v)
{
  outpGUS(0x08, v);
}

static void outpGUS9(unsigned char v)
{
  outpGUS(0x09, v);
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
  outdGUS(0x44, adr>>16);
  return inpGUS(0x107);
}

static void pokeGUS(unsigned long adr, unsigned char data)
{
  outwGUS(0x43, adr);
  outdGUS(0x44, adr>>16);
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

static unsigned short getvol()
{
  return inwGUS(0x89)>>4;
}

static void setpan(unsigned char pan)
{
  outGUS(0x0C, pan);
}

static void setpoint(unsigned long p, unsigned char t)
{
  t=(t==1)?0x02:(t==2)?0x04:0x0A;
  outwGUS(t, (p>>7)&0x1FFF);
  outwGUS(t+1, p<<9);
}

static unsigned long getpoint()
{
  return (inwGUS(0x8A)<<16)|inwGUS(0x8B);
}

static void setmode(unsigned char m)
{
  outdGUS(0x00, m);
}

static unsigned char getmode()
{
  return inGUS(0x80);
}

static void setvst(unsigned char s)
{
  outGUS(0x07, s);
}

static void setvend(unsigned char s)
{
  outGUS(0x08, s);
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

static void settimerlen(unsigned char l)
{
  outGUS(0x46, l);
}

static int testPort(unsigned short port)
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

  if (!gus)
    return 0;

  int i,j;
  unsigned char oldmem[4];
  for (i=0; i<4; i++)
    oldmem[i]=peekGUS(i*256*1024);

  pokeGUS(0,1);
  pokeGUS(0,1);
  gusMem=256*1024;
  for (i=1; i<4; i++)
  {
    pokeGUS(i*256*1024, 15+i);
    pokeGUS(i*256*1024, 15+i);
    for (j=0; j<i; j++)
    {
      if (peekGUS(j*256*1024)!=(1+j))
        break;
      if (peekGUS(j*256*1024)!=(1+j))
        break;
    }
    if (j!=i)
      break;
    if (peekGUS(i*256*1024)!=(15+i))
      break;
    if (peekGUS(i*256*1024)!=(15+i))
      break;
    pokeGUS(i*256*1024, 1+i);
    pokeGUS(i*256*1024, 1+i);
    gusMem+=256*1024;
  }

  for (i=3; i>=0; i--)
  {
    pokeGUS(i*256*1024, oldmem[i]);
    pokeGUS(i*256*1024, oldmem[i]);
  }

  return 1;
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
    setpoint(0,0);
    outGUS(0x06, 63);
    delayGUS();
  }

  inpGUS(0x6);
  inGUS(0x41);
  inGUS(0x49);
  inGUS(0x8F);

  outGUS(0x4C,0x07);

  selvoc(0);
  outpGUS0(0x08);
  selvoc(0);
}



struct guschan
{
  unsigned long startpos;
  unsigned long endpos;
  unsigned long curstart;
  unsigned long curend;
  unsigned long loopstart;
  unsigned long loopend;
  unsigned long sloopstart;
  unsigned long sloopend;
  unsigned long samprate;
  unsigned char redlev;

  unsigned short cursamp;
  unsigned char mode;

  unsigned short voll;
  unsigned short volr;

  int samptype;
  char curloop;
  unsigned char inited;
  signed char chstatus;
  signed short nextsample;
  signed long nextpos;
  unsigned char orgloop;
  signed char loopchange;
  signed char dirchange;

  unsigned long orgfreq;
  unsigned long orgdiv;
  unsigned short orgvol;
  signed short orgpan;
  unsigned char pause;
  unsigned char wasplaying;

  void *smpptr;
};

struct gussample
{
  signed long pos;
  unsigned long length;
  unsigned long loopstart;
  unsigned long loopend;
  unsigned long sloopstart;
  unsigned long sloopend;
  unsigned long samprate;
  int type;
  unsigned char redlev;
  void *ptr;
};

static unsigned short linvol[513];
static unsigned long mempos;
static gussample samples[MAXSAMPLES];
static unsigned short samplenum;

static unsigned char channelnum;
static void (*playerproc)();
static guschan channels[32];
static unsigned long gtimerlen;
static unsigned long gtimerpos;
static unsigned long stimerlen;
static unsigned long stimerpos;
static unsigned long cmdtimerpos;
static unsigned short relspeed;
static unsigned long orgspeed;
static unsigned char mastervol;
static signed char masterpan;
static signed char masterbal;
static unsigned short masterfreq;
static unsigned long amplify;

static unsigned char paused;
static unsigned char doublechan;

static __segment dmasel;
static void *dmabuf;
static int dmalen;
static volatile unsigned char dmaactive;
static void *dmaxfer;
static unsigned long dmaleft;
static unsigned long dmapos;
static unsigned char dma16bit;
static unsigned char doslowupload;
static unsigned char usesystimer;

static unsigned char filter;


static void fadevol(unsigned short v)
{
  unsigned short start=getvol();
  unsigned short end=v;
  unsigned char vmode;
  if (abs((short)(start-end))<64)
  {
    setvol(end);
    return;
  }

  if (start>end)
  {
    unsigned short t=start;
    start=end;
    end=t;
    vmode=0x40;
  }
  else
    vmode=0;
  if (start<64)
    start=64;
  if (end>4032)
    end=4032;
  setvst(start>>4);
  setvend(end>>4);
  setvmode(vmode);
}

static void fadevoldown()
{
  setvst(0x04);
  setvend(0xFC);
  setvmode(0x40);
}


static void processtick()
{
  int i;

  for (i=0; i<channelnum; i++)
  {
    guschan &c=channels[i];
    if (c.inited&&(c.chstatus||(c.nextpos!=-1)))
    {
      selvoc(i);
      setmode(c.mode|3);
      fadevoldown();
    }

    c.chstatus=0;
  }

  for (i=0; i<channelnum; i++)
  {
    selvoc(i);
    while (!(getvmode()&1));
  }

  for (i=0; i<channelnum; i++)
  {
    guschan &c=channels[i];
    selvoc(i);
    if (c.inited)
    {
      if (c.nextsample!=-1)
      {
	gussample &s=samples[c.nextsample];
	unsigned char bit16=!!(s.type&mcpSamp16Bit);
        c.startpos=s.pos;
        if (bit16)
          c.startpos=(c.startpos&0xC0000)|((c.startpos>>1)&0x1FFFF)|0x20000;
        c.curloop=10;
	c.endpos=c.startpos+s.length;
	c.loopstart=c.startpos+s.loopstart;
	c.loopend=c.startpos+s.loopend;
	c.sloopstart=c.startpos+s.sloopstart;
	c.sloopend=c.startpos+s.sloopend;
	c.samprate=s.samprate;
	c.samptype=s.type;
	c.redlev=s.redlev;
	c.smpptr=s.ptr;
	c.mode=(bit16)?0x07:0x03;
	c.cursamp=c.nextsample;
	setmode(c.mode|3);
      }

      if (c.nextpos!=-1)
	c.nextpos=c.startpos+(c.nextpos>>c.redlev);
      if ((c.loopchange==1)&&!(c.samptype&mcpSampSLoop))
	c.loopchange=2;
      if ((c.loopchange==2)&&!(c.samptype&mcpSampLoop))
	c.loopchange=0;
      if (c.loopchange==0)
      {
	c.curstart=c.startpos;
	c.curend=c.endpos;
	c.mode&=~0x18;
      }
      if (c.loopchange==1)
      {
	c.curstart=c.sloopstart;
	c.curend=c.sloopend;
	c.mode|=0x08;
	if (c.samptype&mcpSampSBiDi)
	  c.mode|=0x10;
      }
      if (c.loopchange==2)
      {
        c.curstart=c.loopstart;
        c.curend=c.loopend;
        c.mode|=0x08;
        if (c.samptype&mcpSampBiDi)
          c.mode|=0x10;
      }
      int dir=getmode()&0x40;
      if (c.loopchange!=-1)
      {
        c.curloop=c.loopchange;
        setpoint(c.curstart, 1);
        setpoint(c.curend, 2);
      }
      if (c.dirchange!=-1)
        dir=(c.dirchange==2)?(dir^0x40):c.dirchange?0x40:0;
      int pos=-1;
      if ((c.loopchange!=-1)||(c.dirchange!=-1))
        pos=getpoint()>>9;
      if (c.nextpos!=-1)
        pos=c.nextpos;
      if (pos!=-1)
      {
        if (((pos<c.curstart)&&dir)||((pos>=c.curend)&&!dir))
          dir^=0x40;
        if (c.nextpos!=-1)
        {
          c.mode&=~3;
          if (pos>=c.endpos)
            pos=c.endpos;
          if (pos<=c.startpos)
            pos=c.startpos;
          setpoint(pos, 0);
        }
        setmode(c.mode|dir);
      }

      if (!(getmode()&1))
      {
        int v,p;
        v=c.voll+c.volr;
        if (v)
          setpan((15*c.volr+v/2)/v);
        fadevol(c.pause?0:linvol[v]);
        setfreq(umuldivrnd(umuldivrnd(c.orgfreq, c.samprate*masterfreq, c.orgdiv), activevoices, 154350));
      }
      else
        fadevoldown();
    }
    else
      fadevoldown();

    c.nextsample=-1;
    c.nextpos=-1;
    c.loopchange=-1;
    c.dirchange=-1;
  }
}

static void processtickdc()
{
  int i;

  for (i=0; i<channelnum; i++)
  {
    guschan &c=channels[i];
    if (c.inited&&(c.chstatus||(c.nextpos!=-1)))
    {
      selvoc(2*i);
      setmode(c.mode|3);
      fadevoldown();
      selvoc(2*i+1);
      setmode(c.mode|3);
      fadevoldown();
    }

    c.chstatus=0;
  }

  for (i=0; i<channelnum; i++)
  {
    selvoc(2*i);
    while (!(getvmode()&1));
    selvoc(2*i+1);
    while (!(getvmode()&1));
  }

  for (i=0; i<channelnum; i++)
  {
    guschan &c=channels[i];
    selvoc(2*i);
    if (c.inited)
    {
      if (c.nextsample!=-1)
      {
	gussample &s=samples[c.nextsample];
	unsigned char bit16=!!(s.type&mcpSamp16Bit);
        c.startpos=s.pos;
        if (bit16)
          c.startpos=(c.startpos&0xC0000)|((c.startpos>>1)&0x1FFFF)|0x20000;
	c.endpos=c.startpos+s.length;
	c.loopstart=c.startpos+s.loopstart;
	c.loopend=c.startpos+s.loopend;
	c.sloopstart=c.startpos+s.sloopstart;
	c.sloopend=c.startpos+s.sloopend;
	c.samprate=s.samprate;
	c.samptype=s.type;
	c.redlev=s.redlev;
	c.smpptr=s.ptr;
	c.mode=(bit16)?0x07:0x03;
	c.cursamp=c.nextsample;
	setmode(c.mode|3);
        selvoc(2*i+1);
	setmode(c.mode|3);
        selvoc(2*i);
      }

      if (c.nextpos!=-1)
	c.nextpos=c.startpos+(c.nextpos>>c.redlev);
      if ((c.loopchange==1)&&!(c.samptype&mcpSampSLoop))
	c.loopchange=2;
      if ((c.loopchange==2)&&!(c.samptype&mcpSampLoop))
	c.loopchange=0;
      if (c.loopchange==0)
      {
	c.curstart=c.startpos;
	c.curend=c.endpos;
	c.mode&=~0x18;
      }
      if (c.loopchange==1)
      {
	c.curstart=c.sloopstart;
	c.curend=c.sloopend;
	c.mode|=0x08;
	if (c.samptype&mcpSampSBiDi)
	  c.mode|=0x10;
      }
      if (c.loopchange==2)
      {
	c.curstart=c.loopstart;
	c.curend=c.loopend;
	c.mode|=0x08;
	if (c.samptype&mcpSampBiDi)
	  c.mode|=0x10;
      }

      int dir=getmode()&0x40;
      if (c.loopchange!=-1)
      {
        c.curloop=c.loopchange;
	setpoint(c.curstart, 1);
	setpoint(c.curend, 2);
        selvoc(2*i+1);
	setpoint(c.curstart, 1);
	setpoint(c.curend, 2);
        selvoc(2*i);
      }
      if (c.dirchange!=-1)
        dir=(c.dirchange==2)?(dir^0x40):c.dirchange?0x40:0;
      int pos=-1;
      if ((c.loopchange!=-1)||(c.dirchange!=-1))
        pos=getpoint()>>9;
      if (c.nextpos!=-1)
        pos=c.nextpos;
      if (pos!=-1)
      {
        if (((pos<c.curstart)&&dir)||((pos>=c.curend)&&!dir))
          dir^=0x40;
        if (c.nextpos!=-1)
        {
          c.mode&=~3;
          if (pos>=c.endpos)
            pos=c.endpos;
          if (pos<=c.startpos)
            pos=c.startpos;
          setpoint(pos, 0);
          selvoc(2*i+1);
          setpoint(pos, 0);
          selvoc(2*i);
        }
	setmode(c.mode|dir);
        selvoc(2*i+1);
	setmode(c.mode|dir);
        selvoc(2*i);
      }

      if (!(getmode()&1))
      {
        unsigned long frq=umuldivrnd(umuldivrnd(c.orgfreq, c.samprate*masterfreq, c.orgdiv), activevoices, 154350);
        setfreq(frq);
        fadevol(c.pause?0:linvol[c.voll]);
        selvoc(2*i+1);
        setfreq(frq);
        fadevol(c.pause?0:linvol[c.volr]);
      }
      else
      {
        fadevoldown();
        selvoc(2*i+1);
        fadevoldown();
      }
    }
    else
    {
      fadevoldown();
      selvoc(2*i+1);
      fadevoldown();
    }

    c.nextsample=-1;
    c.nextpos=-1;
    c.loopchange=-1;
    c.dirchange=-1;
  }
}

unsigned long doupload8(const void *buf, unsigned long guspos, unsigned long maxlen, unsigned short port);
#pragma aux doupload8 parm [esi] [ebx] [ecx] [edx] modify [eax] value [ebx] = \
  "pushf" \
  "cli" \
  "add dx,103h" \
  "mov al,44h" \
  "out dx,al" \
  "add dx,2" \
  "mov eax,ebx" \
  "shr eax,16" \
  "out dx,al" \
  "sub dx,2" \
  "mov al,43h" \
  "out dx,al" \
  "inc dx" \
"lp:" \
    "mov ax,bx" \
    "out dx,ax" \
    "mov al,[esi]" \
    "add dx,3" \
    "out dx,al" \
    "sub dx,3" \
    "inc ebx" \
    "inc esi" \
  "test bl,bl" \
  "loopnz lp" \
  "popf"

unsigned long doupload16(const void *buf, unsigned long guspos, unsigned long maxlen, unsigned short port);
#pragma aux doupload16 parm [esi] [ebx] [ecx] [edx] modify [eax] value [ebx] = \
  "pushf" \
  "cli" \
  "add dx,103h" \
  "mov al,44h" \
  "out dx,al" \
  "add dx,2" \
  "mov eax,ebx" \
  "shr eax,16" \
  "out dx,al" \
  "sub dx,2" \
  "mov al,43h" \
  "out dx,al" \
  "inc dx" \
  "shr ecx,1" \
"lp:" \
    "mov ax,bx" \
    "out dx,ax" \
    "mov al,[esi]" \
    "add dx,3" \
    "out dx,al" \
    "sub dx,3" \
    "inc ebx" \
    "mov ax,bx" \
    "out dx,ax" \
    "mov al,[esi+1]" \
    "add dx,3" \
    "out dx,al" \
    "sub dx,3" \
    "inc ebx" \
    "add esi,2" \
  "test bl,bl" \
  "loopnz lp" \
  "popf"

static void dmaupload()
{
  int upbytes;
  if (dmapos&31)
  {
    upbytes=(-dmapos)&31;
    if (upbytes>dmaleft)
      upbytes=dmaleft;
    if (!dma16bit)
      doupload8(dmaxfer, dmapos, upbytes, gusPort);
    else
      doupload16(dmaxfer, dmapos, upbytes, gusPort);
    dmaxfer=((char*)dmaxfer)+upbytes;
    dmaleft-=upbytes;
    dmapos+=upbytes;
  }
  if (!dmaleft)
  {
    dmaactive=0;
    return;
  }
  upbytes=dmaleft;
  if (upbytes>dmalen)
    upbytes=dmalen;
  if (((dmapos+upbytes)&0xC0000)>(dmapos&0xC0000))
    upbytes=0x40000-(dmapos&0x3FFFF);

  dmaactive=1;
  unsigned short adr=dmapos>>4;
  if (gusDMA&4)
    adr=(adr&0xC000)|((adr&0x3FFF)>>1);
  outGUS(0x41, 0);
  memcpy(dmabuf, dmaxfer, upbytes);
  dmaStart(gusDMA, dmabuf, upbytes, 0x08);
  dmaxfer=((char*)dmaxfer)+upbytes;
  dmaleft-=upbytes;
  dmapos+=upbytes;
  outwGUS(0x42, adr);
  outGUS(0x41, (gusDMA&4)|0x21|(dma16bit?0x40:0x00));
}

static void slowupload()
{
  unsigned long endpos=dmapos+dmaleft;
  unsigned long stpos=dmapos;
  if (!dma16bit)
    while (dmapos<endpos)
      dmapos=doupload8((char*)dmaxfer+dmapos-stpos, dmapos, endpos-dmapos, gusPort);
  else
    while (dmapos<endpos)
      dmapos=doupload16((char*)dmaxfer+dmapos-stpos, dmapos, endpos-dmapos, gusPort);
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
      if (inGUS(0x41)&0x40)
        dmaupload();
    if (source&0x04)
    {
      if (!paused)
      {
        if ((gtimerpos>>8)<=256)
          gtimerpos=(gtimerpos&255)+gtimerlen;
        else
          gtimerpos-=256<<8;
        settimer(0x00);
        settimerlen(((gtimerpos>>8)<=256)?(256-(gtimerpos>>8)):0);
        settimer(0x04);
        if (!((gtimerpos-gtimerlen)>>8))
        {
          if (doublechan)
            processtickdc();
          else
            processtick();
          playerproc();
          cmdtimerpos+=umuldiv(gtimerlen, 256*65536, 12500*3600);
          gtimerlen=umuldiv(256, 12500*256*256, orgspeed*relspeed);
        }
      }
      else
      {
        settimer(0x00);
        settimer(0x04);
      }
    }
    if (source&0x08)
    {
      settimer(0x00);
      settimer(0x04);
    }
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
    if (doublechan)
      processtickdc();
    else
      processtick();
    playerproc();
    cmdtimerpos+=stimerlen;
    stimerlen=umuldiv(256, 1193046*256, orgspeed*relspeed);
  }
}

static void voidtimer()
{
}

static void calcvols(guschan &c)
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
}

static void recalcvols()
{
  int i;
  for (i=0; i<channelnum; i++)
    calcvols(channels[i]);
}

static int LoadSamples(sampleinfo *sil, int n)
{
  if (n>MAXSAMPLES)
    return 0;

  if (!mcpReduceSamples(sil, n, gusMem, mcpRedGUS|mcpRedToMono))
    return 0;

  if (!doslowupload)
    irqInit(gusIRQ, irqrout, 1, 8192);

  mempos=0;
  int i;
  for (i=0; i<(2*n); i++)
  {
    sampleinfo &si=sil[i%n];
    if ((!!(si.type&mcpSamp16Bit))^(i<n))
      continue;
    gussample &s=samples[i%n];
    s.length=si.length;
    s.loopstart=si.loopstart;
    s.loopend=si.loopend;
    s.sloopstart=si.sloopstart;
    s.sloopend=si.sloopend;
    s.samprate=si.samprate;
    s.type=si.type;
    int bit16=!!(si.type&mcpSamp16Bit);
    s.redlev=(si.type&mcpSampRedRate4)?2:(si.type&mcpSampRedRate2)?1:0;
    s.pos=mempos;
    mempos+=(s.length+2)<<bit16;

    if (s.loopstart==s.loopend)
      s.type&=~mcpSampLoop;

    dma16bit=bit16;
    dmaleft=(s.length+2)<<dma16bit;
    dmaxfer=si.ptr;
    dmapos=s.pos;
    if (doslowupload)
    {
      slowupload();
      continue;
    }

    dmaupload();
    volatile unsigned long &biosclock=*(volatile unsigned long*)0x46C;
    unsigned long t0=biosclock;
    while (36>(biosclock-t0))
      if (!dmaactive)
        break;
    if (!dmaactive)
      continue;

    irqClose();
    doslowupload=1;
    dmaactive=0;

    dma16bit=bit16;
    dmaleft=(s.length+2)<<dma16bit;
    dmaxfer=si.ptr;
    dmapos=s.pos;
    slowupload();
  }

  if (!doslowupload)
    irqClose();

  samplenum=n;

  for (i=0; i<n; i++)
    samples[i].ptr=sil[i].ptr;

  return 1;
}









static void GetMixChannel(int ch, mixchannel &chn, int rate)
{
  chn.status=0;

  unsigned short is=_disableint();
  selvoc(doublechan?2*ch:ch);
  unsigned long pos=getpoint();
  unsigned char mode=getmode();
  _restoreint(is);
  guschan &c=channels[ch];

  if ((paused&&!c.wasplaying)||(!paused&&(mode&1))||!c.inited)
    return;

  if (c.pause)
    chn.status|=MIX_MUTE;

  unsigned short resvoll,resvolr;
  resvoll=c.voll;
  resvolr=c.volr;

  chn.vols[0]=resvoll*4096/amplify;
  chn.vols[1]=resvolr*4096/amplify;
  chn.status|=((mode&0x08)?MIX_LOOPED:0)|((mode&0x10)?MIX_PINGPONGLOOP:0)|((mode&0x04)?MIX_PLAY16BIT:0);
  chn.step=umuldivrnd(umuldivrnd(umuldivrnd(c.orgfreq, masterfreq, 256), c.samprate, c.orgdiv), 1<<16, rate);
  if (mode&0x40)
    chn.step=-chn.step;
  chn.samp=c.smpptr;
  chn.length=c.endpos-c.startpos;
  chn.loopstart=c.curstart-c.startpos;
  chn.loopend=c.curend-c.startpos;
  chn.fpos=pos<<7;
  chn.pos=((pos>>9)&0xFFFFF)-c.startpos;
  if (filter)
    chn.status|=MIX_INTERPOLATE;
  chn.status|=MIX_PLAYING;
}



static void Pause(int p)
{
  if (p==paused)
    return;
  int i;
  if (paused)
  {
    for (i=0; i<channelnum; i++)
      if (doublechan)
      {
        if (channels[i].wasplaying)
        {
          selvoc(2*i);
          setmode(channels[i].mode|(getmode()&0x40));
          selvoc(2*i+1);
          setmode(channels[i].mode|(getmode()&0x40));
        }
      }
      else
        if (channels[i].wasplaying)
        {
          selvoc(i);
          setmode(channels[i].mode|(getmode()&0x40));
        }
    gtimerpos=0;
    stimerpos=0;
    paused=0;
    if (!usesystimer)
      settimer(0x04);
  }
  else
  {
    paused=1;
    if (!usesystimer)
      settimer(0x00);
    for (i=0; i<channelnum; i++)
      if (doublechan)
      {
        selvoc(2*i);
        channels[i].wasplaying=!(getmode()&1);
        setmode(3|(getmode()&0x40));
        selvoc(2*i+1);
        setmode(3|(getmode()&0x40));
      }
      else
      {
        selvoc(i);
        channels[i].wasplaying=!(getmode()&1);
        setmode(3|(getmode()&0x40));
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
    channels[ch].chstatus=1;
    channels[ch].nextpos=-1;
    channels[ch].nextsample=val;
    channels[ch].loopchange=1;
    channels[ch].inited=1;
    break;
  case mcpCMute:
    channels[ch].pause=val;
    break;
  case mcpCStatus:
    if (!val)
    {
      channels[ch].nextpos=-1;
      channels[ch].chstatus=1;
    }
    break;
  case mcpCLoop:
    channels[ch].loopchange=((val>2)||(val<0))?channels[ch].loopchange:val;
    break;
  case mcpCDirect:
    channels[ch].dirchange=((val>2)||(val<0))?channels[ch].dirchange:val;
    break;
  case mcpCReset:
    int reswasmute;
    reswasmute=channels[ch].pause;
    memset(channels+ch, 0, sizeof(guschan));
    channels[ch].pause=reswasmute;
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
    break;
  case mcpCPitch:
    channels[ch].orgfreq=8363;
    channels[ch].orgdiv=mcpGetFreq8363(-val);
    break;
  case mcpCPitchFix:
    channels[ch].orgfreq=val;
    channels[ch].orgdiv=0x10000;
    break;
  case mcpCPitch6848:
    channels[ch].orgfreq=6848;
    channels[ch].orgdiv=val;
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
    masterfreq=val;
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
    selvoc(doublechan?2*ch:ch);
    return !(getmode()&1)||(paused&&channels[ch].wasplaying);
  case mcpCMute:
    return !!channels[ch].pause;
  case mcpGTimer:
    return tmGetTimer();
  case mcpGCmdTimer:
    return umulshr16(cmdtimerpos, 3600);
  }
  return 0;
}







static int OpenPlayer(int chan, void (*proc)())
{
  if (chan>32)
    chan=32;

  if (!mixInit(GetMixChannel, 1, chan, amplify))
    return 0;

  orgspeed=50*256;

  memset(channels, 0, sizeof(guschan)*chan);
  playerproc=proc;
  doublechan=chan<8;
  int i;
  if (doublechan)
  {
    initgus(2*chan);
    for (i=0; i<chan; i++)
    {
      selvoc(2*i);
      setpan(0);
      selvoc(2*i+1);
      setpan(15);
    }
  }
  else
    initgus(chan);
  channelnum=chan;

  selvoc(0);
  delayGUS();
  outpGUS0(0x09);
  delayGUS();

  cmdtimerpos=0;

  if (usesystimer)
  {
    stimerlen=umuldiv(256, 1193046*256, orgspeed*relspeed);
    stimerpos=stimerlen;
    tmInit(timerrout, (stimerpos<=65536)?stimerpos:65536, 8192);
  }
  else
  {
    irqInit(gusIRQ, irqrout, 1, 8192);
    gtimerlen=umuldiv(256, 12500*256*256, orgspeed*relspeed);
    gtimerpos=gtimerlen;
    settimerlen(((gtimerpos>>8)<=256)?(256-(gtimerpos>>8)):0);
    settimer(0x04);

    tmInit(voidtimer, 65536, 256);
  }
  outpGUS8(0x04);
  outpGUS9(0x01);

  mcpNChan=chan;

  return 1;
}

static void ClosePlayer()
{
  mcpNChan=0;

  tmClose();
  if (!usesystimer)
    irqClose();

  initgus(14);
  channelnum=0;
  mixClose();
}


static int initu(const deviceinfo &c)
{
  doslowupload=!(c.opt&SS_GUS_FASTUPLOAD)||(c.dma==-1)||(c.irq==-1);
  usesystimer=!(c.opt&SS_GUS_GUSTIMER)||(c.irq==-1);

  dmaactive=0;
  dmalen=32768;
  dmabuf=dmaAlloc(dmalen, dmasel);
  dmalen&=~31;
  if (!dmabuf)
    return 0;

  int i;

  if (!testPort(c.port))
    return 0;

  gusPort=c.port;
  gusIRQ=c.irq;
  gusDMA=c.dma;
  gusDMA2=c.dma2;

  channelnum=0;
  filter=0;

  initgus(14);

  relspeed=256;
  paused=0;

  mastervol=64;
  masterpan=64;
  masterbal=0;
  masterfreq=256;
  amplify=65536;

  linvol[0]=0;
  linvol[512]=0x0FFF;
  for (i=1; i<512; i++)
  {
    int j,k;
    k=i;
    for (j=0x0600; k; j+=0x0100)
      k>>=1;
    linvol[i]=j|((i<<(8-((j-0x700)>>8)))&0xFF);
  }

  mcpLoadSamples=LoadSamples;
  mcpOpenPlayer=OpenPlayer;
  mcpClosePlayer=ClosePlayer;
  mcpSet=SET;
  mcpGet=GET;

  return 1;
}


static void closeu()
{
  mcpOpenPlayer=0;
  dmaFree(dmasel);
}

static int detectu(deviceinfo &c)
{
  if (!getcfg())
  {
    if (c.port==-1)
      return 0;
    gusPort=c.port;
    gusIRQ=c.irq;
    gusDMA=c.dma;
    gusDMA2=(c.dma2==-1)?c.dma:c.dma2;
  }
  else
  {
    if (c.port!=-1)
      gusPort=c.port;
    if (c.irq!=-1)
      gusIRQ=c.irq;
    if (c.dma!=-1)
      gusDMA=c.dma;
    if (c.dma2!=-1)
      gusDMA2=c.dma2;
  }

  if (!testPort(gusPort))
    return 0;
  c.subtype=-1;
  c.dev=&mcpUltraSound;
  c.port=gusPort;
  c.port2=-1;
  c.irq=gusIRQ;
  c.irq2=-1;
  c.dma=gusDMA;
  c.dma2=gusDMA2;
  if (!(c.opt&SS_GUS_FASTUPLOAD)&&!(c.opt&SS_GUS_GUSTIMER))
    c.irq=-1;
  if (!(c.opt&SS_GUS_FASTUPLOAD)||(c.irq==-1))
  {
    c.dma=-1;
    c.dma2=-1;
  }
  c.mem=gusMem;
  c.chan=32;
  return 1;
}

extern "C" sounddevice mcpUltraSound={SS_WAVETABLE, "Gravis UltraSound", detectu, initu, closeu};

#ifdef CPDOS

#include "devigen.h"
#include "psetting.h"

static unsigned long gusGetOpt(const char *sec)
{
  unsigned long opt=0;
  if (cfGetProfileBool(sec, "gusfastupload", 0, 0))
    opt|=SS_GUS_FASTUPLOAD;
  if (cfGetProfileBool(sec, "gusgustimer", 0, 0))
    opt|=SS_GUS_GUSTIMER;
  return opt;
}

extern "C" devaddstruct mcpGUSAdd = {gusGetOpt, 0, 0, 0};

#endif
