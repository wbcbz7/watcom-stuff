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
#include "timer.h"
#include "imsrtns.h"

#define MAXSAMPLES   256


#define FXBUFFERSIZE 65536

extern "C" extern sounddevice mcpInterWave;

static unsigned short gusPort;
static unsigned char gusIRQ;

static unsigned char bufferbank;
static unsigned long bufferpos;

static unsigned long gusMem[4];
static unsigned long memsize;
static unsigned long largestbank;
static unsigned long lbsize;

static unsigned short linvol[513];

static unsigned char usesystimer;
static unsigned char effectsoff;
static unsigned char forceeffects;

static int  rdelay[4]={ 3395, 2023, 2857, 1796};
static int  rfeedback[4]={ 0x80, 0xc0, 0xc0,0x100};
static char rfxchans[4]={ 0xC0, 0xC0 ,0x30, 0x30};
static int  rpanning[4]={0x180, 0x80,0x200,0x000};

static char pan2chan[4]={0xA0, 0x30, 0x30, 0x50};

static char getcfg()
{
  if (!getenv("INTERWAVE"))
    return 0;
  char *ptr=getenv("ULTRASND");
  if (!ptr)
    return 0;
  while (*ptr==' ') ptr++;
  if (!ptr)
    return 0;
  gusPort=strtoul(ptr, 0, 16);
  while ((*ptr!=',')&&*ptr)
    ptr++;
  if (!*ptr++)
    return 0;
  while ((*ptr!=',')&&*ptr)
    ptr++;
  if (!*ptr++)
    return 0;
  while ((*ptr!=',')&&*ptr)
    ptr++;
  if (!*ptr++)
    return 0;
  gusIRQ=strtoul(ptr, 0, 10)&15;
  return 1;
}

static unsigned char inpGUS(unsigned short p)
{
  return inp(gusPort+p);
}

static void outpGUS(unsigned short p, unsigned char v)
{
  outp(gusPort+p,v);
}

static void outGUS(unsigned char c, unsigned char v)
{
  outp(gusPort+0x103, c);
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

static void resetGUS()
{
  int i;

  outGUS(0x4C, 0);
  for (i=0; i<20; i++)
    inp(gusPort+0x107);

  outGUS(0x4C, 1);
  for (i=0; i<20; i++)
    inp(gusPort+0x107);
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
  outwGUS(0x01, frq);
}

static void setvol(unsigned short vol)
{
  outwGUS(0x09, vol<<4);
}

static unsigned short getvol()
{
  return inwGUS(0x89)>>4;
}

static void setrelvoll(unsigned short vol,char mode)
{
  vol=0xfff-vol;
  if (mode)
    outwGUS(0x13,vol<<4);
  outwGUS(0x1C,vol<<4);
}

static void setrelvolr(unsigned short vol,char mode)
{
  vol=0xfff-vol;
  if (mode)
    outwGUS(0x0c,vol<<4);
  outwGUS(0x1B,vol<<4);
}

static void seteffvol(unsigned short vol)
{
  vol=0xfff-vol;
  outwGUS(0x16,vol<<4);
  outwGUS(0x1D,vol<<4);
}

static void seteffchan(char ch)
{
  outGUS(0x14,ch);
}

static void setbank(char b)
{
  outGUS(0x10,b);
}

static void setpoint(unsigned long p, unsigned char t)
{
  t=(t==1)?0x02:(t==2)?0x04:(t==3)?0x11:0x0A;  // new: t==3 -> FX buffer write position
  outwGUS(t, p>>7);
  outwGUS(t+1, p<<9);
}

static unsigned char getbank()
{
  return (inGUS(0x90)&0x03);
}

static unsigned long getpoint()
{
  return (inwGUS(0x8A)<<7)|(inwGUS(0x8B)>>9);
}

static void setmode(unsigned char m)
{
  outGUS(0x00, m);
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
  outGUS(0x0D, m);
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

static char setenhmode(unsigned char m)
{
  if (m)
  {
    outGUS(0x19,inGUS(0x99)|0x01);
    return inGUS(0x99)&0x01;
  }
  outGUS(0x19,inGUS(0x99)&~0x01);
  return 1;
}

static void findMem()
{
  char v0,v1,v2,v3;

  for (int b=0; b<4; b++)
  {
    int ba=b<<22;
    gusMem[b]=ba;

    v0=peekGUS(ba);
    v1=peekGUS(ba+1);
    pokeGUS(ba,0x55);
    pokeGUS(ba+1,0x56);

    char testval=0x55;
    int i;
    for (i=0; i<64; i++)
    {
      v2=peekGUS(gusMem[b]);
      v3=peekGUS(gusMem[b]+1);
      pokeGUS(gusMem[b],testval);
      pokeGUS(gusMem[b]+1,testval+1);
      if ((peekGUS(gusMem[b])!=testval)||(peekGUS(gusMem[b]+1)!=(testval+1))||(peekGUS(ba)!=0x55)||(peekGUS(ba+1)!=0x56))
	break;
      gusMem[b]+=0x10000;
      testval++;
      pokeGUS(gusMem[b],v2);
      pokeGUS(gusMem[b]+1,v3);
    }

    pokeGUS(0,v0);
    pokeGUS(1,v1);
    gusMem[b]-=ba;

  }
}

static char testPort(unsigned short port)
{
  gusPort=port;

  resetGUS();

  char v0,v1;

  v0=peekGUS(0);
  v1=peekGUS(1);

  pokeGUS(0,0xAA);
  pokeGUS(1,0x55);

  char gus=peekGUS(0)==0xAA;

  pokeGUS(0,v0);
  pokeGUS(1,v1);

  if (!gus)
    return 0;

  outwGUS(0x43, 0);
  outGUS(0x44, 0);
  outwGUS(0x51, 0x1234);

  if ((peekGUS(0)!=0x34)||(peekGUS(1)!=0x12))
    gus=0;

  pokeGUS(0,v0);
  pokeGUS(1,v1);

  if (!gus)
    return 0;

  if (!setenhmode(1))
    return 0;

  outwGUS(0x52,inwGUS(0x52)&0xFFF0|0x0c);
  outGUS(0x53,inGUS(0x53)&0xFD);
  findMem();

  unsigned long memcfg=(gusMem[3]>>18);
  memcfg=(memcfg<<8)|(gusMem[2]>>18);
  memcfg=(memcfg<<8)|(gusMem[1]>>18);
  memcfg=(memcfg<<8)|(gusMem[0]>>18);
  char lmcfi;
  switch (memcfg)
  {
    case 0x00000001: lmcfi=0x0; break;
    case 0x00000101: lmcfi=0x1; break;
    case 0x00010101: lmcfi=0x1; break;
    case 0x01010101: lmcfi=0x2; break;
    case 0x00000401: lmcfi=0x3; break;
    case 0x00010401: lmcfi=0x4; break;
    case 0x00040401: lmcfi=0x4; break;
    case 0x01040401: lmcfi=0x4; break;
    case 0x04040401: lmcfi=0x4; break;
    case 0x00040101: lmcfi=0x5; break;
    case 0x01040101: lmcfi=0x6; break;
    case 0x04040101: lmcfi=0x6; break;
    case 0x00000004: lmcfi=0x7; break;
    case 0x00000104: lmcfi=0x8; break;
    case 0x00000404: lmcfi=0x8; break;
    case 0x00010404: lmcfi=0x9; break;
    case 0x00040404: lmcfi=0x9; break;
    case 0x01040404: lmcfi=0x9; break;
    case 0x04040404: lmcfi=0x9; break;
    case 0x00000010: lmcfi=0xA; break;
    case 0x00000110: lmcfi=0xB; break;
    case 0x00000410: lmcfi=0xB; break;
    case 0x00001010: lmcfi=0xB; break;
    default: lmcfi=0xC;
  }

  outwGUS(0x52,inwGUS(0x52)&0xFFF0|lmcfi);
  findMem();

  unsigned long maxsize=4096*1024;
  for (int b=1; b<4; b++) if (gusMem[b]&&gusMem[b]<maxsize)
  {
    maxsize=gusMem[b];
    bufferbank=b;
  }

//  bufferbank=1;

  if (!effectsoff)
    gusMem[bufferbank]-=FXBUFFERSIZE;

  bufferpos=gusMem[bufferbank]>>1;

  memsize=gusMem[0]+gusMem[1]+gusMem[2]+gusMem[3];

  setenhmode(0);

  return 1;
}

void dofill(unsigned long guspos, unsigned long maxlen, unsigned short port);
#pragma aux dofill parm [ebx] [ecx] [edx] modify [eax] = \
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
  "adc ecx,0" \
  "mov ax,bx" \
  "out dx,ax" \
  "dec dx" \
  "mov al,51h" \
  "out dx,ax" \
  "inc dx" \
  "mov ax,0" \
"lp:" \
  "out dx,ax" \
  "loop lp" \
  "popf"

static void FillGusMem(unsigned long pos, unsigned long len)
{
  unsigned char lmci=inGUS(0x53);
  outGUS(0x53,(lmci|0x01)&0x4D);
  dofill(pos,len,gusPort);
  outGUS(0x53,lmci);
}


static void SetupReverb()
{
  FillGusMem((bufferbank<<22)+(bufferpos<<1),FXBUFFERSIZE);

  unsigned long actpos=((bufferbank&0x01)<<21)|bufferpos;
  for (int i=28; i<32; i++)
  {
    int rc=i-28;
    selvoc(i);

    setmode(0x0c);
    setvmode(7);

    outGUS(0x10,bufferbank>>1);
    setpoint(actpos,0);
    setpoint(actpos,1);
    setpoint(actpos+0xfff,2);
    setpoint(actpos+rdelay[rc],3);

    setrelvoll(0,1);
    setrelvolr(0,1);
    setvol(0xfff);
    seteffvol(linvol[rfeedback[rc]]);
    seteffchan(rfxchans[rc]);

    setfreq(0x400);

    outGUS(0x15,0x21);

    actpos+=0x2000;
  }
}


static void initgus(char enhmode,char chans)
{
  int i;

  if (chans>32) chans=32;
  if (forceeffects&&(chans>28)) chans=28;

  resetGUS();

  setenhmode(enhmode);

  outGUS(0x41, 0x00);
  outGUS(0x45, 0x00);
  outGUS(0x49, 0x00);

  outGUS(0xE, 0xff);  // only for GUS compatibility

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
    setpoint(1,0);
    outGUS(0x06,63);
    outwGUS(0x0C,0xfff0); // reset panning
    if (enhmode)
    {
      outGUS(0x15,(i<chans)?0x20:0x02);   // fine panning vs. voice off
      setbank(0);
      outwGUS(0x13,0xfff0);                // reset vol offsets
      outwGUS(0x1b,0xfff0);
      outwGUS(0x1c,0xfff0);
      outwGUS(0x16,0);                     // reset effects depth
      outwGUS(0x1d,0);
      outGUS(0x14,0x00);		     	// disable FX channels;
    }
    else
      outGUS(0x15,0x00);
  }

  outGUS(0x4C,0x07);

  if (!effectsoff&&enhmode&&chans&&(chans<29))
    SetupReverb();

  selvoc(0);
  outpGUS(0x00,0x08);
}


struct guschan
{
  unsigned char bank;
  unsigned long startpos;
  unsigned long endpos;
  unsigned long loopstart;
  unsigned long loopend;
  unsigned long sloopstart;
  unsigned long sloopend;
  unsigned long samprate;
  unsigned long curstart;
  unsigned long curend;
  unsigned char redlev;

  unsigned char curloop;
  int samptype;

  unsigned short cursamp;
  unsigned char mode;

  unsigned short volume;
  unsigned short voll;
  unsigned short volr;
  unsigned short reverb;
  unsigned char fxsend;

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
  unsigned char orgrev;
  unsigned char pause;
  unsigned char wasplaying;

  void *smpptr;
};

struct gussample
{
  unsigned char bank;
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

static unsigned long mempos[4];
static gussample samples[MAXSAMPLES];
static unsigned short samplenum;

static unsigned char channelnum;
static void (*playerproc)();
static guschan channels[32];
static unsigned long gtimerlen;
static unsigned long gtimerpos;
static unsigned long cmdtimerpos;
static unsigned long stimerlen;
static unsigned long stimerpos;
static unsigned short relspeed;
static unsigned long orgspeed;
static unsigned char mastervol;
static signed char masterpan;
static signed char masterbal;
static unsigned short masterfreq;
static signed short masterreverb;
static unsigned long amplify;

static unsigned char paused;

static void *dmaxfer;
static unsigned long dmaleft;
static unsigned long dmapos;
static unsigned char dma16bit;

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
	c.bank=s.bank>>bit16;
	c.startpos=(s.pos+(s.bank<<22))>>bit16;
	c.endpos=c.startpos+s.length;
	c.loopstart=c.startpos+s.loopstart;
	c.loopend=c.startpos+s.loopend;
	c.sloopstart=c.startpos+s.sloopstart;
	c.sloopend=c.startpos+s.sloopend;
	c.samprate=s.samprate;
	c.samptype=s.type;
	c.redlev=s.redlev;
	c.smpptr=s.ptr;
	if (c.loopchange==-1)
	  c.loopchange=1;
	c.mode=(bit16)?0x07:0x03;
	c.cursamp=c.nextsample;
	setbank(c.bank>>bit16);
	setmode(c.mode|3);
        setrelvoll(linvol[c.voll],1);
        setrelvolr(linvol[c.volr],1);
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
        pos=getpoint();
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
	if (c.pause)
	  fadevoldown();
	else
	{
	  fadevol(linvol[c.volume]);
	  setrelvoll(linvol[c.voll],0);
	  setrelvolr(linvol[c.volr],0);
	  seteffvol(linvol[c.reverb]);
	  seteffchan(pan2chan[c.fxsend]);
	}
	setfreq(umuldivrnd(c.orgfreq, c.samprate*masterfreq, c.orgdiv)/10915/*11135*/);
                       	    // ich weigere mich btw. dies einzusehen. Danke.
                              // =)
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


void doupload8(const void *buf, unsigned long guspos, unsigned long maxlen, unsigned short port);
#pragma aux doupload8 parm [esi] [ebx] [ecx] [edx] modify [eax] = \
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
  "adc ecx,0" \
  "mov ax,bx" \
  "out dx,ax" \
  "dec dx" \
  "mov al,51h" \
  "out dx,ax" \
  "inc dx" \
  "rep outsw" \
  "popf"

void doupload16(const void *buf, unsigned long guspos, unsigned long maxlen, unsigned short port);
#pragma aux doupload16 parm [esi] [ebx] [ecx] [edx] modify [eax] = \
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
  "mov ax,bx" \
  "out dx,ax" \
  "dec dx" \
  "mov al,51h" \
  "out dx,ax" \
  "inc dx" \
  "rep outsw" \
  "popf"

static void slowupload()
{
  unsigned char lmci=inGUS(0x53);
  outGUS(0x53,(lmci|0x01)&0x4D);

  if (!dma16bit)
  {
    if ((dmapos&1)&&dmaleft)
    {
      pokeGUS(dmapos, *(char*)dmaxfer);
      dmaxfer=(char*)dmaxfer+1;
      dmapos++;
      dmaleft--;
    }
    if (dmaleft&1)
    {
      pokeGUS(dmapos+dmaleft-1, ((char*)dmaxfer)[dmaleft-1]);
      dmaleft--;
    }
    doupload8(dmaxfer, dmapos, dmaleft, gusPort);
  }
  else
    doupload16(dmaxfer, dmapos, dmaleft, gusPort);

  outGUS(0x53,lmci);

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
    processtick();
    playerproc();
    cmdtimerpos+=stimerlen;
    stimerlen=umuldiv(256, 1193046*256, orgspeed*relspeed);
  }
}

static void voidtimer()
{
}

static void calcfxvols()
{
  short vl,vr;
  if (channelnum<29)
    for (int i=28; i<32; i++)
    {
      vr=rpanning[i-28];
      vl=0x200-vr;
      if (masterbal)
	if (masterbal<0)
	  vr=(vr*(64+masterbal))>>6;
	else
	  vl=(vl*(64-masterbal))>>6;
      selvoc(i);
      setrelvoll(linvol[vl],1);
      setrelvolr(linvol[vr],1);
    }
}


static void calcvols(guschan &c)
{
  short cv=(c.orgvol*mastervol*amplify)>>20;
  if (cv>=0x200) cv=0x1ff;

  short vr=(((c.orgpan*masterpan)>>6)+128)<<1;
  if (vr>=0x200) vr=0x1ff;
  short vl=0x1ff-vr;
  char  ch=vr>>8;

  short rv;
  if (masterreverb>0)
    rv=((masterreverb<<2)+((c.orgrev*(64-masterreverb))>>6));
  else
    rv=(c.orgrev*(masterreverb+64))>>6;
  if (rv>=0x200) rv=0x1ff;

  if (masterbal)
    if (masterbal<0)
      vr=(vr*(64+masterbal))>>6;
    else
      vl=(vl*(64-masterbal))>>6;

  c.volume=cv;
  c.voll=vl;
  c.volr=vr;
  c.reverb=rv;
  c.fxsend=ch;
}

static int LoadSamples(sampleinfo *sil, int n)
{
  unsigned long samplen[MAXSAMPLES];

  if (n>MAXSAMPLES) return 0;

  for (int sc=0; sc<n; sc++)
  {
    samplen[sc]=sil[sc].length << (!!(sil[sc].type&mcpSamp16Bit));
  }

  int largestsample=0;

  for (int sa=0; sa<n; sa++) if (samplen[sa]>samplen[largestsample])
    largestsample=sa;

  if (!mcpReduceSamples(sil, n, memsize-samplen[largestsample], mcpRedToMono))
    return 0;

  samplenum=n;

  mempos[0]=0;mempos[1]=0;mempos[2]=0;mempos[3]=0;
  while(1)
  {
    largestbank=0;
    lbsize=gusMem[0];
    for (char b=1; b<4; b++) if (gusMem[b]>lbsize)
    {
      lbsize=gusMem[b]-mempos[b];
      largestbank=b;
    }

    for (int sa=0; sa<n; sa++) if (samplen[sa]>samplen[largestsample])
      largestsample=sa;

    if (!samplen[largestsample]) return 1;

    if (samplen[largestsample]>lbsize) return 0;

    sampleinfo &si=sil[largestsample];
    gussample &s=samples[largestsample];
    s.pos=mempos[largestbank];
    s.length=si.length;
    s.loopstart=si.loopstart;
    s.loopend=si.loopend;
    s.sloopstart=si.sloopstart;
    s.sloopend=si.sloopend;
    s.samprate=si.samprate;
    s.type=si.type;
    s.redlev=(si.type&mcpSampRedRate4)?2:(si.type&mcpSampRedRate2)?1:0;
    int bit16=!!(si.type&mcpSamp16Bit);
    s.bank=largestbank;
    mempos[largestbank]+=(((s.length+2)<<bit16)+1)&~1;

    dma16bit=bit16;
    dmaleft=(s.length+2)<<dma16bit;
    dmaxfer=si.ptr;
    dmapos=s.pos|(s.bank<<22);
    slowupload();

    samplen[largestsample]=0;
    s.ptr=si.ptr;
  }
}


static void recalcvols()
{
  int i;
  for (i=0; i<channelnum; i++)
    calcvols(channels[i]);
}



static void GetMixChannel(int ch, mixchannel &chn, int rate)
{
  chn.status=0;

  unsigned short is=_disableint();
  selvoc(ch);
  unsigned long pos=getpoint()+(getbank()<<22);
  unsigned char mode=getmode();
  _restoreint(is);
  guschan &c=channels[ch];

  if ((paused&&!c.wasplaying)||(!paused&&(mode&1))||!c.inited)
    return;

  if (c.pause)
    chn.status|=MIX_MUTE;

  int resvoll,resvolr;
  resvoll=c.volume*c.voll; resvolr=c.volume*c.volr;

  chn.vols[0]=resvoll*8/amplify;
  chn.vols[1]=resvolr*8/amplify;
  chn.status|=((mode&0x08)?MIX_LOOPED:0)|((mode&0x10)?MIX_PINGPONGLOOP:0)|((mode&0x04)?MIX_PLAY16BIT:0);
  chn.step=umuldivrnd(umuldivrnd(c.orgfreq, c.samprate*masterfreq, c.orgdiv), 256, rate);
  if (mode&0x40) chn.step=-chn.step;
  chn.samp=c.smpptr;
  chn.length=c.endpos-c.startpos;
  chn.loopstart=c.curstart-c.startpos;
  chn.loopend=c.curend-c.startpos;
  chn.fpos=0;
  chn.pos=pos-c.startpos;
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
      if (channels[i].wasplaying)
      {
	selvoc(i);
	setmode(channels[i].mode|(getmode()&0x40));
      }
    stimerpos=0;
    gtimerpos=0;
    if (!usesystimer)
      settimer(0x04);
    paused=0;
  }
  else
  {
    paused=1;
    if (!usesystimer)
      settimer(0x00);
    for (i=0; i<channelnum; i++)
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
    channels[ch].loopchange=((val>2)||(val<0))?-1:val;
    break;
  case mcpCDirect:
    channels[ch].dirchange=((val>2)||(val<0))?-1:val;
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
    channels[ch].orgpan=(val>0x80)?0x80:(val<-0x80)?-0x80:val;
    calcvols(channels[ch]);
    break;
  case mcpCReverb:
    channels[ch].orgrev=(val<0)?0:(val>0x100)?0x100:val;
    calcvols(channels[ch]);
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
    if (channelnum)
      calcfxvols();
    break;
  case mcpMasterReverb:
    masterreverb=val;
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
    selvoc(ch);
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
  if (chan>32) chan=32;
  if (forceeffects&&(chan>28)) chan=28;

  if (!mixInit(GetMixChannel, 1, chan, amplify))
    return 0;

  orgspeed=50*256;

  memset(channels, 0, sizeof(guschan)*chan);
  playerproc=proc;
  initgus(1,chan);
  calcfxvols();
  channelnum=chan;

  selvoc(0);
  outpGUS(0x00,0x09);

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
  outpGUS(8,0x04);
  outpGUS(9,0x01);

  mcpNChan=chan;

  return 1;
}

static void ClosePlayer()
{
  mcpNChan=0;

  tmClose();
  if (!usesystimer)
    irqClose();

  initgus(1,0);
  channelnum=0;
  mixClose();
}


static int initu(const deviceinfo &c)
{
  usesystimer=(c.irq==-1)||!(c.opt&0x01);
  effectsoff=(c.opt&0x02);
  forceeffects=(c.opt&0x04);

  int i;

  if (!testPort(c.port))
    return 0;

  gusPort=c.port;
  gusIRQ=c.irq;

  channelnum=0;
  filter=0;

  initgus(1,0);

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
    int k=i;
    int j;
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
  initgus(0,14);
}


static int detectu(deviceinfo &c)
{
  if (!getcfg())
  {
    if (c.port==-1)
      return 0;
    gusPort=c.port;
  }
  else
  {
    if (c.port!=-1)
      gusPort=c.port;
    if (c.irq!=-1)
      gusIRQ=c.irq;
  }

  if (!testPort(gusPort))
    return 0;
  c.dev=&mcpInterWave;
  c.port=gusPort;
  c.port2=-1;
  c.irq=(c.opt&0x01)?gusIRQ:-1;
  c.irq2=-1;
  c.dma=-1;
  c.dma2=-1;
  c.subtype=-1;
  c.chan=32;
  c.mem=memsize;
  return 1;
}

extern "C" sounddevice mcpInterWave={SS_WAVETABLE, "AMD InterWave", detectu, initu, closeu};

#ifdef CPDOS

#include "devigen.h"
#include "psetting.h"

static unsigned long iwGetOpt(const char *sec)
{
  unsigned long opt=0;
  if (cfGetProfileBool(sec, "iwiwtimer", 0, 0))
    opt|=0x01;
  if (!cfGetProfileBool(sec, "iweffects", 1, 1))
    opt|=0x02;
  if (cfGetProfileBool(sec, "iwforceeffects", 0, 0))
    opt|=0x04;
  return opt;
}

extern "C" devaddstruct mcpIWAdd = {iwGetOpt, 0, 0, 0};

#endif
