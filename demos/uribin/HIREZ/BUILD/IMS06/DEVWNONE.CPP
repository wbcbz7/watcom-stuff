// (c) 1994-1997 Niklas Beisert, this file is part of "Indoor Music System".
// you must not read, modify, print, compile or copy this file or parts of
// it unless you have accepted the license in the accompanying file IMS.TXT.

#include <string.h>
#include <conio.h>
#include <stdlib.h>
#include "imsdev.h"
#include "mcp.h"
#include "timer.h"
#include "mix.h"
#include "dma.h"
#include "imsrtns.h"

#define TIMERRATE 17100
#define MAXCHAN 256

#define NONE_PLAYING 1
#define NONE_MUTE 2
#define NONE_LOOPED 4
#define NONE_PINGPONGLOOP 8
#define NONE_PLAY16BIT 16

static const unsigned long samprate=44100;

extern "C" extern sounddevice mcpNone;

struct channel
{
  void *samp;
  unsigned long length;
  unsigned long loopstart;
  unsigned long loopend;
  unsigned long replen;
  signed long step;
  unsigned long pos;
  unsigned short fpos;
  unsigned char status;
  signed char vol[2];
  unsigned char orgvol[2];
  unsigned short orgrate;
  signed long orgfrq;
  signed long orgdiv;
  unsigned char direct;
  unsigned char volopt;
  unsigned char orgloop;
  int orgvolx;
  int orgpan;
};

static int pause;

static sampleinfo *samples;
static int samplenum;

static unsigned long amplify;
static unsigned char transform[2][2];
static int volopt;
static unsigned long relpitch;
static int filter;

static int channelnum;
static channel *channels;

static void (*playerproc)();
static unsigned long tickwidth;
static unsigned long tickplayed;
static unsigned long orgspeed;
static unsigned short relspeed;
static unsigned long newtickwidth;
static unsigned long cmdtimerpos;

static int mastervol;
static int masterpan;
static int masterbal;

extern "C" int nonePlayChannel(unsigned long len, channel &ch);
#pragma aux nonePlayChannel "*_" parm [] caller modify [eax ebx ecx edx esi edi] value [eax]


static void calcstep(channel &c)
{
  if (!(c.status&NONE_PLAYING))
    return;
  c.step=imuldiv(imuldiv(c.orgrate, ((c.step>=0)^c.direct)?c.orgfrq:-c.orgfrq, c.orgdiv)<<8, relpitch, samprate);
  c.direct=(c.orgfrq<0)^(c.orgdiv<0);
}


static void calcspeed()
{
  if (channelnum)
    newtickwidth=imuldiv(256*256, samprate, orgspeed*relspeed);
}

static void transformvol(channel &ch)
{
  int v;
  v=transform[0][0]*ch.orgvol[0]+transform[0][1]*ch.orgvol[1];
  ch.vol[0]=(v>4096)?64:(v<-4096)?-64:((v+32)>>6);

  v=transform[1][0]*ch.orgvol[0]+transform[1][1]*ch.orgvol[1];
  ch.vol[1]=(v>4096)?64:(v<-4096)?-64:((v+32)>>6);
}

static void calcvol(channel &chn)
{
  if (chn.orgpan<0)
  {
    chn.orgvol[1]=(chn.orgvolx*(0x80+chn.orgpan))>>10;
    chn.orgvol[0]=(chn.orgvolx>>2)-chn.orgvol[1];
  }
  else
  {
    chn.orgvol[0]=(chn.orgvolx*(0x80-chn.orgpan))>>10;
    chn.orgvol[1]=(chn.orgvolx>>2)-chn.orgvol[0];
  }
  transformvol(chn);
}



static void playchannels(unsigned short len)
{
  if (!len)
    return;
  int i;
  for (i=0; i<channelnum; i++)
  {
    channel &c=channels[i];
    if (c.status&NONE_PLAYING)
      nonePlayChannel(len, c);
  }
}


static void timerproc()
{
  int bufdelta=samprate*TIMERRATE/1193046;

  if (channelnum&&!pause)
  {
    while ((tickwidth-tickplayed)<=bufdelta)
    {
      playchannels(tickwidth-tickplayed);
      bufdelta-=tickwidth-tickplayed;
      tickplayed=0;
      playerproc();
      cmdtimerpos+=tickwidth;
      tickwidth=newtickwidth;
    }
    playchannels(bufdelta);
    tickplayed+=bufdelta;
  }
}







static void calcvols()
{
  signed char vols[2][2];
  vols[0][0]=0x20+(masterpan>>1);
  vols[0][1]=0x20-(masterpan>>1);
  vols[1][0]=0x20-(masterpan>>1);
  vols[1][1]=0x20+(masterpan>>1);

  if (masterbal>0)
  {
    vols[0][0]=((signed short)vols[0][0]*(0x40-masterbal))>>6;
    vols[0][1]=((signed short)vols[0][1]*(0x40-masterbal))>>6;
  }
  else
  {
    vols[1][0]=((signed short)vols[1][0]*(0x40+masterbal))>>6;
    vols[1][1]=((signed short)vols[1][1]*(0x40+masterbal))>>6;
  }

  vols[0][0]=((signed short)vols[0][0]*mastervol)>>6;
  vols[0][1]=((signed short)vols[0][1]*mastervol)>>6;
  vols[1][0]=((signed short)vols[1][0]*mastervol)>>6;
  vols[1][1]=((signed short)vols[1][1]*mastervol)>>6;

  memcpy(transform, vols, 4);
  int i;
  for (i=0; i<channelnum; i++)
    transformvol(channels[i]);
}


static void calcsteps()
{
  int i;
  for (i=0; i<channelnum; i++)
    calcstep(channels[i]);
}






static void SetInstr(channel &chn, unsigned short samp)
{
  sampleinfo &s=samples[samp];
  chn.status&=~(NONE_PLAYING|NONE_LOOPED|NONE_PINGPONGLOOP|NONE_PLAY16BIT);
  chn.samp=s.ptr;
  if (s.type&mcpSamp16Bit)
    chn.status|=NONE_PLAY16BIT;
  if (s.type&mcpSampLoop)
    chn.status|=NONE_LOOPED;
  if (s.type&mcpSampBiDi)
    chn.status|=NONE_PINGPONGLOOP;
  chn.length=s.length;
  chn.loopstart=s.loopstart;
  chn.loopend=s.loopend;
  chn.replen=(chn.status&NONE_LOOPED)?(s.loopend-s.loopstart):0;
  chn.orgloop=chn.status&NONE_LOOPED;
  chn.orgrate=s.samprate;
  chn.step=0;
  chn.pos=0;
  chn.fpos=0;
  chn.orgvol[0]=0;
  chn.orgvol[1]=0;
  chn.vol[0]=0;
  chn.vol[1]=0;
}

static void SET(int ch, int opt, int val)
{
  int tmp;
  switch (opt)
  {
  case mcpGSpeed:
    orgspeed=val;
    calcspeed();
    break;
  case mcpCInstrument:
    SetInstr(channels[ch], val);
    break;
  case mcpCMute:
    if (val)
      channels[ch].status|=NONE_MUTE;
    else
      channels[ch].status&=~NONE_MUTE;
    break;
  case mcpCStatus:
    if (!val)
      channels[ch].status&=~NONE_PLAYING;
    break;
  case mcpCReset:
    tmp=channels[ch].status&NONE_MUTE;
    memset(&channels[ch], 0, sizeof(channel));
    channels[ch].status=tmp;
    break;
  case mcpCVolume:
    channels[ch].orgvolx=(val>0xF8)?0x100:(val<0)?0:(val+3);
    calcvol(channels[ch]);
    break;
  case mcpCPanning:
    channels[ch].orgpan=(val>0x78)?0x80:(val<-0x78)?-0x80:val;
    calcvol(channels[ch]);
    break;
  case mcpMasterAmplify:
    amplify=val;
    if (channelnum)
      mixSetAmplify(amplify);
    break;
  case mcpMasterPause:
    pause=val;
    break;
  case mcpCPosition:
    channels[ch].status&=~NONE_PLAYING;
    if (val>=channels[ch].length)
      if (channels[ch].status&NONE_LOOPED)
        val=channels[ch].loopstart;
      else
        break;

    channels[ch].step=0;
    channels[ch].direct=0;
    calcstep(channels[ch]);
    channels[ch].pos=val;
    channels[ch].fpos=0;
    channels[ch].status|=NONE_PLAYING;
    break;
  case mcpCPitch:
    channels[ch].orgfrq=8363;
    channels[ch].orgdiv=mcpGetFreq8363(-val);
    calcstep(channels[ch]);
    break;
  case mcpCPitchFix:
    channels[ch].orgfrq=val;
    channels[ch].orgdiv=0x10000;
    calcstep(channels[ch]);
    break;
  case mcpCPitch6848:
    channels[ch].orgfrq=6848;
    channels[ch].orgdiv=val;
    calcstep(channels[ch]);
    break;
  case mcpMasterVolume:
    mastervol=val;
    calcvols();
    break;
  case mcpMasterPanning:
    masterpan=val;
    calcvols();
    break;
  case mcpMasterBalance:
    masterbal=val;
    calcvols();
    break;
  case mcpMasterSpeed:
    relspeed=(val<16)?16:val;
    calcspeed();
    break;
  case mcpMasterPitch:
    relpitch=val;
    calcsteps();
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
    return !!(channels[ch].status&NONE_PLAYING);
  case mcpCMute:
    return !!(channels[ch].status&NONE_MUTE);
  case mcpGTimer:
    return tmGetTimer();
  case mcpGCmdTimer:
    return umuldiv(cmdtimerpos, 65536, samprate);
  }
  return 0;
}


static void GetMixChannel(int ch, mixchannel &chn, int rate)
{
  channel &c=channels[ch];
  chn.samp=c.samp;
  chn.length=c.length;
  chn.loopstart=c.loopstart;
  chn.loopend=c.loopend;
  chn.fpos=c.fpos;
  chn.pos=c.pos;
  chn.vols[0]=abs(c.vol[0]);
  chn.vols[1]=abs(c.vol[1]);
  chn.step=imuldiv(c.step, samprate, rate);
  chn.status=0;
  if (c.status&NONE_MUTE)
    chn.status|=MIX_MUTE;
  if (c.status&NONE_PLAY16BIT)
    chn.status|=MIX_PLAY16BIT;
  if (c.status&NONE_LOOPED)
    chn.status|=MIX_LOOPED;
  if (c.status&NONE_PINGPONGLOOP)
    chn.status|=MIX_PINGPONGLOOP;
  if (c.status&NONE_PLAYING)
    chn.status|=MIX_PLAYING;
  if (filter)
    chn.status|=MIX_INTERPOLATE;
}



static int LoadSamples(sampleinfo *sil, int n)
{
  if (!mcpReduceSamples(sil, n, 0x40000000, mcpRedToMono))
    return 0;

  samples=sil;
  samplenum=n;

  return 1;
}


static int OpenPlayer(int chan, void (*proc)())
{
  if (chan>MAXCHAN)
    chan=MAXCHAN;

  channels=new channel[chan];
  if (!channels)
  {
    delete channels;
    return 0;
  }


  playerproc=proc;

  if (!mixInit(GetMixChannel, 1, chan, amplify))
    return 0;

  memset(channels, 0, sizeof(channel)*chan);

  calcvols();
  pause=0;
  orgspeed=12800;
  calcspeed();
  tickwidth=newtickwidth;
  tickplayed=0;
  cmdtimerpos=0;

  channelnum=chan;
  tmInit(timerproc, TIMERRATE, 8192);

  mcpNChan=chan;

  return 1;
}

static void ClosePlayer()
{
  mcpNChan=0;
  tmClose();
  channelnum=0;
  mixClose();
}

static int Init(const deviceinfo &dev)
{
  amplify=65535;
  relspeed=256;
  relpitch=256;
  filter=0;
  mastervol=64;
  masterpan=64;
  masterbal=0;

  channelnum=0;

  mcpLoadSamples=LoadSamples;
  mcpOpenPlayer=OpenPlayer;
  mcpClosePlayer=ClosePlayer;
  mcpSet=SET;
  mcpGet=GET;

  return 1;
}

static void Close()
{
  mcpOpenPlayer=0;
}


static int Detect(deviceinfo &c)
{
  c.dev=&mcpNone;
  c.port=-1;
  c.port2=-1;
  c.irq=-1;
  c.irq2=-1;
  c.dma=-1;
  c.dma2=-1;
  c.subtype=-1;
  c.chan=(MAXCHAN>99)?99:MAXCHAN;
  c.mem=0;
  return 1;
}

extern "C" sounddevice mcpNone={SS_WAVETABLE, "None", Detect, Init, Close};
