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

extern "C" extern sounddevice mcpDoubleGUS;

static unsigned short gusPort;
static unsigned short gusPort2;
static unsigned long gusMem;
static unsigned char activevoices;
static unsigned char doublemode;


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
  return 1;
}

static unsigned char inpGUS(unsigned short p)
{
  return inp(gusPort+p);
}

static unsigned char inpGUS2(unsigned short p)
{
  return inp(gusPort2+p);
}

static unsigned short inpwGUS(unsigned short p)
{
  return inpw(gusPort+p);
}

static unsigned short inpwGUS2(unsigned short p)
{
  return inpw(gusPort2+p);
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

static void outpGUS2(unsigned short p, unsigned char v)
{
  outp(gusPort2+p,v);
}

static void outpwGUS(unsigned short p, unsigned short v)
{
  outpw(gusPort+p,v);
}

static void outpwGUS2(unsigned short p, unsigned short v)
{
  outpw(gusPort2+p,v);
}

static void outGUS(unsigned char c, unsigned char v)
{
  outp(gusPort+0x103, c);
  outp(gusPort+0x105, v);
}

static void outGUS2(unsigned char c, unsigned char v)
{
  outp(gusPort2+0x103, c);
  outp(gusPort2+0x105, v);
}

static void outdGUS(unsigned char c, unsigned char v)
{
  outp(gusPort+0x103, c);
  outp(gusPort+0x105, v);
  delayGUS();
  outp(gusPort+0x105, v);
}

static void outdGUS2(unsigned char c, unsigned char v)
{
  outp(gusPort2+0x103, c);
  outp(gusPort2+0x105, v);
  delayGUS();
  outp(gusPort2+0x105, v);
}

static void outwGUS(unsigned char c, unsigned short v)
{
  outp(gusPort+0x103, c);
  outpw(gusPort+0x104, v);
}

static void outwGUS2(unsigned char c, unsigned short v)
{
  outp(gusPort2+0x103, c);
  outpw(gusPort2+0x104, v);
}

static unsigned char inGUS(unsigned char c)
{
  outp(gusPort+0x103, c);
  return inp(gusPort+0x105);
}

static unsigned char inGUS2(unsigned char c)
{
  outp(gusPort2+0x103, c);
  return inp(gusPort2+0x105);
}

static unsigned short inwGUS(unsigned char c)
{
  outp(gusPort+0x103, c);
  return inpw(gusPort+0x104);
}

static unsigned short inwGUS2(unsigned char c)
{
  outp(gusPort2+0x103, c);
  return inpw(gusPort2+0x104);
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
  outpGUS2(0x102, ch);
}

static void setfreq(unsigned short frq)
{
  outwGUS(0x01, frq&~1);
  outwGUS2(0x01, frq&~1);
}

static void setvol(unsigned short vol)
{
  outwGUS(0x09, vol<<4);
}

static void setvol2(unsigned short vol)
{
  outwGUS2(0x09, vol<<4);
}

static unsigned short getvol()
{
  return inwGUS(0x89)>>4;
}

static unsigned short getvol2()
{
  return inwGUS2(0x89)>>4;
}

static void setpan(unsigned char pan)
{
  outGUS(0x0C, pan);
}

static void setpan2(unsigned char pan)
{
  outGUS2(0x0C, pan);
}

static void setpoint8(unsigned long p, unsigned char t)
{
  t=(t==1)?0x02:(t==2)?0x04:0x0A;
  outwGUS(t, (p>>7)&0x1FFF);
  outwGUS(t+1, p<<9);
  outwGUS2(t, (p>>7)&0x1FFF);
  outwGUS2(t+1, p<<9);
}

static unsigned long getpoint()
{
  return (inwGUS(0x8A)<<16)|inwGUS(0x8B);
}

static void setmode(unsigned char m)
{
  outdGUS(0x00, m);
  outdGUS2(0x00, m);
}

static unsigned char getmode()
{
  return inGUS(0x80);
}

static void setvst(unsigned char s)
{
  outGUS(0x07, s);
}

static void setvst2(unsigned char s)
{
  outGUS2(0x07, s);
}

static void setvend(unsigned char s)
{
  outGUS(0x08, s);
}

static void setvend2(unsigned char s)
{
  outGUS2(0x08, s);
}

static void setvmode(unsigned char m)
{
  outdGUS(0x0D, m);
}

static void setvmode2(unsigned char m)
{
  outdGUS2(0x0D, m);
}

static unsigned char getvmode()
{
  return inGUS(0x8D);
}

static unsigned char getvmode2()
{
  return inGUS2(0x8D);
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
  outGUS2(0x4C, 0);
  delayGUS();
  delayGUS();

  outGUS(0x4C, 1);
  outGUS2(0x4C, 1);
  delayGUS();
  delayGUS();

  outGUS(0xE, (activevoices-1)|0xC0);
  outGUS2(0xE, (activevoices-1)|0xC0);

  for (i=0; i<32; i++)
  {
    selvoc(i);
    setvol(0);  // vol=0
    setmode(3);  // stop voice
    setvmode(3);  // stop volume
    setpoint8(0,0);
    outGUS(0x06, 63);
    setvol2(0);  // vol=0
    setvmode2(3);  // stop volume
    setpoint8(0,0);
    outGUS2(0x06, 63);
    delayGUS();
  }

  outGUS(0x4C,0x07);
  outGUS2(0x4C,0x07);
  selvoc(0);
  outpGUS(0x0, 0x08);
  outpGUS2(0x0, 0x08);
}



struct guschan
{
  unsigned long startpos;
  unsigned long endpos;
  unsigned long loopstart;
  unsigned long loopend;
  unsigned long samprate;
  unsigned char redlev;

  unsigned short cursamp;
  unsigned char mode;

  unsigned short voll;
  unsigned short volr;
  unsigned short voll2;
  unsigned short volr2;

  unsigned char inited;
  unsigned char stopit;
  signed short nextsample;
  signed long nextpos;
  unsigned char orgloop;
  signed char loopchange;

  unsigned long orgfreq;
  unsigned long orgdiv;
  unsigned short orgvol;
  signed short orgpan;
  signed short orgpany;
  signed short orgpanz;
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

static void *dmaxfer;
static unsigned long dmaleft;
static unsigned long dmapos;
static unsigned char dma16bit;

static unsigned char filter;
static unsigned char surround;


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

static void fadevol2(unsigned short v)
{
  unsigned short start=getvol2();
  unsigned short end=v;
  unsigned char vmode;
  if (abs((short)(start-end))<64)
  {
    setvol2(end);
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
  setvst2(start>>4);
  setvend2(end>>4);
  setvmode2(vmode);
}

static void fadevoldown()
{
  setvst(0x04);
  setvend(0xFC);
  setvmode(0x40);

  setvst2(0x04);
  setvend2(0xFC);
  setvmode2(0x40);
}


static void processtick()
{
  int i;

  for (i=0; i<channelnum; i++)
  {
    guschan &c=channels[i];
    if (c.inited&&(c.stopit||(c.nextpos!=-1)))
    {
      selvoc(i);
      setmode(c.mode|3);
      fadevoldown();
    }

    c.stopit=0;
  }

  for (i=0; i<channelnum; i++)
  {
    selvoc(i);
    while (!(getvmode()&1));
    while (!(getvmode2()&1));
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
        c.endpos=c.startpos+s.length;
        c.loopstart=c.startpos+s.loopstart;
        c.loopend=c.startpos+s.loopend;
        c.samprate=s.samprate;
        c.redlev=s.redlev;
        c.smpptr=s.ptr;
        c.mode=0;
        if (bit16)
          c.mode|=0x04;
        c.cursamp=c.nextsample;
        c.orgloop=0;
        if (s.type&mcpSampLoop)
        {
          c.orgloop=1;
          c.mode|=0x08;
          if (s.type&mcpSampBiDi)
            c.mode|=0x10;
          setpoint8(c.loopstart, 1);
          setpoint8(c.loopend, 2);
        }
        else
        {
          setpoint8(c.startpos, 1);
          setpoint8(c.endpos, 2);
        }
      }
      if (c.nextpos!=-1)
      {
        unsigned long pos=c.startpos+(c.nextpos>>c.redlev);
        if (c.mode&0x08)
        {
          if (pos>=c.loopend)
            pos=c.loopstart+(pos-c.loopstart)%(c.loopend-c.loopstart);
        }
        else
          if (pos>=c.endpos)
            pos=c.endpos-1;
        setpoint8(pos, 0);
        setmode(c.mode|(getmode()&0x40));
      }
      if (c.loopchange!=-1)
      {
        if (c.loopchange&&c.orgloop)
          c.mode|=0x08;
        else
          c.mode&=~0x08;
        setmode(c.mode|(getmode()&0x40));
        if (c.mode&0x08)
        {
          setpoint8(c.loopstart, 1);
          setpoint8(c.loopend, 2);
        }
        else
        {
          setpoint8(c.startpos, 1);
          setpoint8(c.endpos, 2);
        }
      }
      if (!(getmode()&1))
      {
        short v,p;
        v=c.voll+c.volr;
        if (v)
          setpan((15*c.volr+v/2)/v);
        fadevol(c.pause?0:linvol[v]);
        v=c.voll2+c.volr2;
        if (v)
          setpan2((15*c.volr2+v/2)/v);
        fadevol2(c.pause?0:linvol[v]);
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
  }
}

static void processtickdc()
{
  int i;

  for (i=0; i<channelnum; i++)
  {
    guschan &c=channels[i];
    if (c.inited&&(c.stopit||(c.nextpos!=-1)))
    {
      selvoc(2*i);
      setmode(c.mode|3);
      fadevoldown();
      selvoc(2*i+1);
      setmode(c.mode|3);
      fadevoldown();
    }

    c.stopit=0;
  }

  for (i=0; i<channelnum; i++)
  {
    selvoc(2*i);
    while (!(getvmode()&1));
    while (!(getvmode2()&1));
    selvoc(2*i+1);
    while (!(getvmode()&1));
    while (!(getvmode2()&1));
  }

  for (i=0; i<channelnum; i++)
  {
    guschan &c=channels[i];
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
        c.samprate=s.samprate;
        c.redlev=s.redlev;
        c.smpptr=s.ptr;
        c.mode=0;
        if (bit16)
          c.mode|=0x04;
        c.cursamp=c.nextsample;
        c.orgloop=0;
        if (s.type&mcpSampLoop)
        {
          c.orgloop=1;
          c.mode|=0x08;
          if (s.type&mcpSampBiDi)
            c.mode|=0x10;
          selvoc(2*i);
          setpoint8(c.loopstart, 1);
          setpoint8(c.loopend, 2);
          selvoc(2*i+1);
          setpoint8(c.loopstart, 1);
          setpoint8(c.loopend, 2);
        }
        else
        {
          selvoc(2*i);
          setpoint8(c.startpos, 1);
          setpoint8(c.endpos, 2);
          selvoc(2*i+1);
          setpoint8(c.startpos, 1);
          setpoint8(c.endpos, 2);
        }
      }
      if (c.nextpos!=-1)
      {
        unsigned long pos=c.startpos+(c.nextpos>>c.redlev);
        if (c.mode&0x08)
        {
          if (pos>=c.loopend)
            pos=c.loopstart+(pos-c.loopstart)%(c.loopend-c.loopstart);
        }
        else
          if (pos>=c.endpos)
            pos=c.endpos;
        selvoc(2*i);
        setpoint8(pos, 0);
        selvoc(2*i+1);
        setpoint8(pos, 0);
        selvoc(2*i);
        setmode(c.mode|(getmode()&0x40));
        selvoc(2*i+1);
        setmode(c.mode|(getmode()&0x40));
      }
      if (c.loopchange!=-1)
      {
        if (c.loopchange&&c.orgloop)
          c.mode|=0x08;
        else
          c.mode&=~0x08;
        selvoc(2*i);
        setmode(c.mode|(getmode()&0x40));
        if (c.mode&0x08)
        {
          setpoint8(c.loopstart, 1);
          setpoint8(c.loopend, 2);
        }
        else
        {
          setpoint8(c.startpos, 1);
          setpoint8(c.endpos, 2);
        }
        selvoc(2*i+1);
        setmode(c.mode|(getmode()&0x40));
        if (c.mode&0x08)
        {
          setpoint8(c.loopstart, 1);
          setpoint8(c.loopend, 2);
        }
        else
        {
          setpoint8(c.startpos, 1);
          setpoint8(c.endpos, 2);
        }
      }
      selvoc(2*i);
      if (!(getmode()&1))
      {
        unsigned long frq=umuldivrnd(umuldivrnd(c.orgfreq, c.samprate*masterfreq, c.orgdiv), activevoices, 154350);
        selvoc(2*i);
        setfreq(frq);
        fadevol(c.pause?0:linvol[c.voll]);
        fadevol2(c.pause?0:linvol[c.voll2]);
        selvoc(2*i+1);
        setfreq(frq);
        fadevol(c.pause?0:linvol[c.volr]);
        fadevol2(c.pause?0:linvol[c.volr2]);
      }
      else
      {
        selvoc(2*i);
        fadevoldown();
        selvoc(2*i+1);
        fadevoldown();
      }
    }
    else
    {
      selvoc(2*i);
      fadevoldown();
      selvoc(2*i+1);
      fadevoldown();
    }

    c.nextsample=-1;
    c.nextpos=-1;
    c.loopchange=-1;
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

static void slowupload()
{
  unsigned long endpos=dmapos+dmaleft;
  unsigned long stpos=dmapos;
  if (!dma16bit)
    while (dmapos<endpos)
    {
      doupload8((char*)dmaxfer+dmapos-stpos, dmapos, endpos-dmapos, gusPort2);
      dmapos=doupload8((char*)dmaxfer+dmapos-stpos, dmapos, endpos-dmapos, gusPort);
    }
  else
    while (dmapos<endpos)
    {
      doupload16((char*)dmaxfer+dmapos-stpos, dmapos, endpos-dmapos, gusPort2);
      dmapos=doupload16((char*)dmaxfer+dmapos-stpos, dmapos, endpos-dmapos, gusPort);
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
  short vl=(c.orgvol*mastervol/16)*amplify/65536;
  if (vl>=0x200)
    vl=0x1FF;
  short vr=(vl*((c.orgpan*masterpan/64)+0x80))>>8;
  vl-=vr;

  if (masterbal)
    if (masterbal<0)
      vr=(vr*(64+masterbal))>>6;
    else
      vl=(vl*(64-masterbal))>>6;
  c.voll=vl;
  c.volr=vr;

  if (!doublemode)
  {
    unsigned short z=0x80-c.orgpanz;
    if (surround)
      z=(((signed char)(c.cursamp*0xB7))>>1)&0xFF;

    c.voll2=(c.voll*z)>>8;
    c.voll-=c.voll2;
    c.volr2=(c.volr*z)>>8;
    c.volr-=c.volr2;
  }
  else
  {
    unsigned short z=0x80-c.orgpanz;
    unsigned short y=0x80-c.orgpany;
    if (surround)
    {
      y=(((signed char)(c.cursamp*0x35))>>1)&0xFF;
      z=(((signed char)(c.cursamp*0xB7))>>1)&0xFF;
    }

    unsigned short tot=c.voll+c.volr;

    c.voll2=(tot*y)>>8;
    c.volr2=tot-c.voll2;

    c.voll=(c.voll*(0x200-z))>>9;
    c.volr=(c.volr*(0x200-z))>>9;
    c.voll2=(c.voll2*(0x100+z))>>9;
    c.volr2=(c.volr2*(0x100+z))>>9;
  }
}

static int LoadSamples(sampleinfo *sil, int n)
{
  if (n>MAXSAMPLES)
    return 0;

  if (!mcpReduceSamples(sil, n, gusMem, mcpRedGUS|mcpRedToMono))
    return 0;

  mempos=0;
  int i;
  for (i=0; i<(2*n); i++)
  {
    sampleinfo &si=sil[i%n];
    gussample &s=samples[i%n];
    if ((!!(si.type&mcpSamp16Bit))^(i<n))
      continue;
    s.length=si.length;
    s.loopstart=si.loopstart;
    s.loopend=si.loopend;
    s.samprate=si.samprate;
    s.type=si.type;
    int bit16=!!(si.type&mcpSamp16Bit);
    s.redlev=(si.type&mcpSampRedRate2)?2:(si.type&mcpSampRedRate4)?1:0;
    s.pos=mempos;
    mempos+=(s.length+2)<<bit16;

    if (s.loopstart==s.loopend)
      s.type&=~mcpSampLoop;

    dma16bit=bit16;
    dmaleft=(s.length+2)<<dma16bit;
    dmaxfer=si.ptr;
    dmapos=s.pos;
    slowupload();
  }

  samplenum=n;
//  smSamplesTo8(sil, n);

  for (i=0; i<n; i++)
    samples[i].ptr=sil[i].ptr;

  return 1;
}






static void SetSpeed(unsigned long s)
{
  orgspeed=s;
}

static void SetFilter(unsigned char f)
{
  filter=f;
}

static void SetMasterVol(unsigned char vol)
{
  mastervol=vol;
  int i;
  for (i=0; i<channelnum; i++)
    calcvols(channels[i]);
}

static void SetMasterPan(signed char pan)
{
  masterpan=pan;
  int i;
  for (i=0; i<channelnum; i++)
    calcvols(channels[i]);
}

static void SetMasterSrnd(unsigned char opt)
{
  surround=!!opt;
  int i;
  for (i=0; i<channelnum; i++)
    calcvols(channels[i]);
}

static void SetMasterBal(signed char bal)
{
  masterbal=bal;
  int i;
  for (i=0; i<channelnum; i++)
    calcvols(channels[i]);
}

static void SetMasterSpeed(unsigned short sp)
{
  if (sp<16)
    sp=16;
  relspeed=sp;
}

static void SetMasterFreq(unsigned short p)
{
  masterfreq=p;
}

static void SetAmplify(unsigned long amp)
{
  amplify=amp;
  int i;
  for (i=0; i<channelnum; i++)
    calcvols(channels[i]);
  if (channelnum)
    mixSetAmplify(2*amplify);
}


static void SetInstr(unsigned char ch, unsigned short samp)
{
  channels[ch].stopit=1;
  channels[ch].nextsample=samp;
  channels[ch].loopchange=1;
  channels[ch].inited=1;
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
  if (!doublemode)
  {
    resvoll=c.voll+c.voll2;
    resvolr=c.volr+c.volr2;
  }
  else
  {
    resvoll=(2*c.voll+c.voll2+c.volr2)>>1;
    resvolr=(2*c.volr+c.voll2+c.volr2)>>1;
  }

  chn.vols[0]=resvoll*4096/amplify;
  chn.vols[1]=resvolr*4096/amplify;
  chn.status|=((mode&0x08)?MIX_LOOPED:0)|((mode&0x10)?MIX_PINGPONGLOOP:0)|((mode&0x04)?MIX_PLAY16BIT:0);
  chn.step=umuldivrnd(umuldivrnd(umuldivrnd(c.orgfreq, masterfreq, 256), c.samprate, c.orgdiv), 1<<16, rate);
  if (mode&0x40)
    chn.step=-chn.step;
  chn.samp=c.smpptr;
  chn.length=c.endpos-c.startpos;
  chn.loopstart=c.loopstart-c.startpos;
  chn.loopend=c.loopend-c.startpos;
  chn.fpos=pos<<7;
  chn.pos=((pos>>9)&0xFFFFF)-c.startpos;
  if (filter)
    chn.status|=MIX_INTERPOLATE;
  chn.status|=MIX_PLAYING;
}



static void SetVolume(unsigned char ch, signed short v)
{
  if (v>=0x100)
    v=0x100;
  if (v<0)
    v=0;
  channels[ch].orgvol=v;
  calcvols(channels[ch]);
}

static void SetPan(unsigned char ch, signed short p)
{
  if (p>=0x80)
    p=0x80;
  if (p<=-0x80)
    p=-0x80;
  channels[ch].orgpan=p;
  calcvols(channels[ch]);
}

static void SetPanY(unsigned char ch, signed short y)
{
  if (y>=0x80)
    y=0x80;
  if (y<=-0x80)
    y=-0x80;
  channels[ch].orgpany=y;
  calcvols(channels[ch]);
}

static void SetPanZ(unsigned char ch, signed short z)
{
  if (z>=0x80)
    z=0x80;
  if (z<=-0x80)
    z=-0x80;
  channels[ch].orgpanz=z;
  calcvols(channels[ch]);
}

static void SetPos(unsigned char ch, unsigned long pos)
{
  channels[ch].nextpos=pos;
}

static void SetFreq(unsigned char ch, unsigned long frq, unsigned long div)
{
  channels[ch].orgfreq=frq;
  channels[ch].orgdiv=div;
}

static void SetFreqLog(unsigned char ch, signed short note)
{
  channels[ch].orgfreq=8363;
  channels[ch].orgdiv=mcpGetFreq8363(-note);
}

static void Play(unsigned char ch, unsigned long frq, unsigned long div)
{
  channels[ch].orgfreq=frq;
  channels[ch].orgdiv=div;
  channels[ch].nextpos=0;
  channels[ch].loopchange=1;
}

static void PlayLog(unsigned char ch, signed short note)
{
  Play(ch, 8363, mcpGetFreq8363(-note));
}

static void Stop(unsigned char ch)
{
  channels[ch].nextpos=-1;
  channels[ch].stopit=1;
}

static void LeaveLoop(unsigned char ch)
{
  channels[ch].loopchange=0;
}

static void Reset(unsigned char ch)
{
  unsigned char p=channels[ch].pause;
  memset(channels+ch, 0, sizeof(guschan));
  channels[ch].pause=p;
}

static void Pause(unsigned char p)
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
    stimerpos=0;
    paused=0;
  }
  else
  {
    paused=1;
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

static int GetStatus(int ch)
{
  unsigned is=_disableint();
  selvoc(doublechan?2*ch:ch);
  int rv=!(getmode()&1);
  if (paused&&channels[ch].wasplaying)
    rv=1;
  _restoreint(is);
  return rv;
}


static void SET(int ch, int opt, int val)
{
  switch (opt)
  {
  case mcpGSpeed:
    SetSpeed(val);
    break;
  case mcpCInstrument:
    SetInstr(ch, val);
    break;
  case mcpCMute:
    channels[ch].pause=val;
    break;
  case mcpCStop:
    Stop(ch);
    break;
  case mcpCReset:
    Reset(ch);
    break;
  case mcpCVolume:
    SetVolume(ch, val);
    break;
  case mcpCPanning:
    SetPan(ch, val);
    break;
  case mcpCPanY:
    SetPanY(ch, val);
    break;
  case mcpCPanZ:
    SetPanZ(ch, val);
    break;
  case mcpMasterAmplify:
    SetAmplify(val);
    break;
  case mcpMasterPause:
    Pause(val);
    break;
  case mcpCPosition:
    SetPos(ch, val);
    break;
  case mcpCPitch:
    SetFreqLog(ch, val);
    break;
  case mcpCPitchFix:
    SetFreq(ch, val, 0x10000);
    break;
  case mcpCPitch6848:
    SetFreq(ch, 6848, val);
    break;
  case mcpMasterVolume:
    SetMasterVol(val);
    break;
  case mcpMasterPanning:
    SetMasterPan(val);
    break;
  case mcpMasterBalance:
    SetMasterBal(val);
    break;
  case mcpMasterSpeed:
    SetMasterSpeed(val);
    break;
  case mcpMasterPitch:
    SetMasterFreq(val);
    break;
  case mcpMasterSurround:
    SetMasterSrnd(val);
    break;
  case mcpMasterFilter:
    SetFilter(val);
    break;
  }
}

static int GET(int ch, int opt)
{
  switch (opt)
  {
  case mcpCStatus:
    return !!GetStatus(ch);
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

  if (!mixInit(GetMixChannel, 1, chan, 2*amplify))
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
      setpan2(0);
      selvoc(2*i+1);
      setpan(15);
      setpan2(15);
    }
  }
  else
    initgus(chan);
  channelnum=chan;

  selvoc(0);
  delayGUS();
  outpGUS(0x0, 0x09);
  outpGUS2(0x0, 0x09);
  delayGUS();

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

  initgus(14);
  channelnum=0;
  mixClose();
}




static int initu(const deviceinfo &c)
{
  doublemode=c.subtype;

  if (c.port==c.port2)
    return 0;

  int i;

  if (!testPort(c.port))
    return 0;
  unsigned long memmax=gusMem;
  if (!testPort(c.port2))
    return 0;
  if (memmax<gusMem)
    gusMem=memmax;

  gusPort=c.port;
  gusPort2=c.port2;

  channelnum=0;
  filter=0;
  surround=0;

  initgus(14);

  mempos=0;

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
}

static int detectu(deviceinfo &c)
{
  if (c.port2==-1)
    return 0;
  if (!getcfg())
  {
    if (c.port==-1)
      return 0;
  }
  else
  {
    if (c.port==-1)
      c.port=gusPort;
  }

  if (!testPort(c.port))
    return 0;
  unsigned long memmax=gusMem;
  if (!testPort(c.port2))
    return 0;
  if (memmax<gusMem)
    gusMem=memmax;

  if (c.subtype==-1)
    c.subtype=0;
  c.dev=&mcpDoubleGUS;
  c.irq=-1;
  c.irq2=-1;
  c.dma=-1;
  c.dma2=-1;
  c.chan=32;
  c.mem=gusMem;
  return 1;
}

extern "C" sounddevice mcpDoubleGUS={SS_WAVETABLE, "Double GUS", detectu, initu, closeu};
