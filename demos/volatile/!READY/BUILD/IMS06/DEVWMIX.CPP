// (c) 1994-1997 Niklas Beisert, this file is part of "Indoor Music System".
// you must not read, modify, print, compile or copy this file or parts of
// it unless you have accepted the license in the accompanying file IMS.TXT.

#include <string.h>
#include <stdlib.h>
#include "imsdev.h"
#include "mcp.h"
#include "poll.h"
#include "mix.h"
#include "player.h"
#include "devwmix.h"
#include "imsrtns.h"

#define MIXBUFLEN 4096
#define MAXCHAN 256

#define MIXRQ_PLAYING 1
#define MIXRQ_MUTE 2
#define MIXRQ_LOOPED 4
#define MIXRQ_PINGPONGLOOP 8
#define MIXRQ_PLAY16BIT 16
#define MIXRQ_INTERPOLATE 32
#define MIXRQ_INTERPOLATEMAX 64
#define MIXRQ_PLAYSTEREO 64

#define MIXRQ_RESAMPLE 1

extern "C" extern sounddevice mcpMixer;

static mixqpostprocregstruct *postprocs;

struct channel
{
  void *samp;
  unsigned long length;
  unsigned long loopstart;
  unsigned long loopend;
  unsigned long replen;
  long step;
  unsigned long pos;
  unsigned short fpos;
  unsigned short status;
  int curvols[4];
  int dstvols[4];

  int vol[2];
  int orgvol[2];
  int orgrate;
  long orgfrq;
  long orgdiv;
  int volopt;
  int orgvolx;
  int orgpan;
  int samptype;
  long orgloopstart;
  long orgloopend;
  long orgsloopstart;
  long orgsloopend;
};

static int quality;
static int resample;

static int pause;
static long playsamps;
static long pausesamps;

static sampleinfo *samples;
static int samplenum;

static short (*amptab)[256];
static unsigned long clipmax;
static int clipbusy;
static unsigned long amplify;
static unsigned char transform[2][2];
static int volopt;
static unsigned long relpitch;
static int interpolation;
static int restricted;

static unsigned char stereo;
static unsigned char bit16;
static unsigned char signedout;
static unsigned long samprate;
static unsigned char reversestereo;

static int channelnum;
static channel *channels;
static int fadedown[2];

static long (*voltabsr)[256];
static short (*voltabsq)[2][256];

static unsigned char (*interpoltabr)[256][2];
static unsigned short (*interpoltabq)[32][256][2];
static unsigned short (*interpoltabq2)[16][256][4];

static short *scalebuf;
static long *buf32;
static int bufpos;
static int buflen;
static void *plrbuf;

static void (*playerproc)();
static unsigned long tickwidth;
static unsigned long tickplayed;
static unsigned long orgspeed;
static unsigned short relspeed;
static unsigned long newtickwidth;
static unsigned long cmdtimerpos;

static int mastervol;
static int masterbal;
static int masterpan;
static int mastersrnd;

extern "C" void mixrPlayChannel(long *buf, int *fade, unsigned long len, channel &ch, int st);
#pragma aux mixrPlayChannel "*_" parm [] caller modify [eax ebx ecx edx esi edi]
extern "C" void mixrFadeChannel(int *fade, channel &ch);
#pragma aux mixrFadeChannel "*_" parm [esi] [edi] modify [eax ebx ecx]
extern "C" void mixrFade(long *buf, int *fade, int len, int stereo);
#pragma aux mixrFade "*_" parm [edi] [esi] [ecx] [edx] modify [eax ebx ecx edx esi edi]
extern "C" void mixrClip(void *dst, const void *src, int len, const void *tab, long max, int b16);
#pragma aux mixrClip "*_" parm [edi] [esi] [ecx] [ebx] [edx] [eax]
extern "C" void mixrSetupAddresses(const void *vol, const void *intr);
#pragma aux mixrSetupAddresses "*_" parm [eax] [ebx]
extern "C" void mixqPlayChannel(short *buf, unsigned long len, channel &ch, char quiet);
#pragma aux mixqPlayChannel "*_" parm [] caller modify [eax ebx ecx edx esi edi]
extern "C" void mixqSetupAddresses(const void *voltab, const void *intrtab1, const void *intrtab2);
#pragma aux mixqSetupAddresses "*_" parm [eax] [ebx] [ecx] modify [eax]
extern "C" void mixqAmplifyChannel(long *buf, short *src, int len, int vol, int step);
#pragma aux mixqAmplifyChannel "*_" parm [edi] [esi] [ecx] [ebx] [edx] modify [eax]
extern "C" void mixqAmplifyChannelUp(long *buf, short *src, int len, int vol, int step);
#pragma aux mixqAmplifyChannelUp "*_" parm [edi] [esi] [ecx] [ebx] [edx] modify [eax]
extern "C" void mixqAmplifyChannelDown(long *buf, short *src, int len, int vol, int step);
#pragma aux mixqAmplifyChannelDown "*_" parm [edi] [esi] [ecx] [ebx] [edx] modify [eax]

static void calcinterpoltabr()
{
  int i,j;
  for (i=0; i<16; i++)
    for (j=0; j<256; j++)
    {
      interpoltabr[i][j][1]=(i*(signed char)j)>>4;
      interpoltabr[i][j][0]=(signed char)j-interpoltabr[i][j][1];
    }
}

static void calcinterpoltabq()
{
  int i,j;
  for (i=0; i<32; i++)
    for (j=0; j<256; j++)
    {
      interpoltabq[0][i][j][1]=(i*(signed char)j)<<3;
      interpoltabq[0][i][j][0]=(((signed char)j)<<8)-interpoltabq[0][i][j][1];
      interpoltabq[1][i][j][1]=(i*j)>>5;
      interpoltabq[1][i][j][0]=j-interpoltabq[1][i][j][1];
    }
  for (i=0; i<16; i++)
    for (j=0; j<256; j++)
    {
      interpoltabq2[0][i][j][0]=((16-i)*(16-i)*(signed char)j)>>1;
      interpoltabq2[0][i][j][2]=(i*i*(signed char)j)>>1;
      interpoltabq2[0][i][j][1]=(((signed char)j)<<8)-interpoltabq2[0][i][j][0]-interpoltabq2[0][i][j][2];
      interpoltabq2[1][i][j][0]=((16-i)*(16-i)*j)>>9;
      interpoltabq2[1][i][j][2]=(i*i*j)>>9;
      interpoltabq2[1][i][j][1]=j-interpoltabq2[1][i][j][0]-interpoltabq2[1][i][j][2];
    }
}

static void calcvoltabsr()
{
  int i,j;
  for (i=0; i<=128; i++)
    for (j=0; j<256; j++)
      voltabsr[i][j]=4*(i-64)*(signed char)j;
}

static void calcvoltabsq()
{
  int i,j;
  for (j=0; j<=128; j++)
  {
    long amp=4*(j-64);
    for (i=0; i<256; i++)
    {
      int v=amp*(signed char)i;
      voltabsq[j][0][i]=(v==0x8000)?0x7FFF:v;
      voltabsq[j][1][i]=(amp*i)>>8;
    }
  }
}

static void calcamptab(signed long amp)
{
  clipbusy++;

  amp>>=4;

  int i;
  for (i=0; i<256; i++)
  {
    amptab[0][i]=(amp*i)>>12;
    amptab[1][i]=(amp*i)>>4;
    amptab[2][i]=(amp*(signed char)i)<<4;
  }

  clipmax=0x07FFF000/amp;

  if (!signedout)
    for (i=0; i<256; i++)
      amptab[0][i]^=0x8000;

  clipbusy--;
}

static void calcstep(channel &c)
{
  if (!(c.status&MIXRQ_PLAYING))
    return;
  if (c.orgdiv)
    c.step=imuldiv(imuldiv((c.step>=0)?c.orgfrq:-c.orgfrq, c.orgrate, c.orgdiv)<<8, relpitch, samprate);
  else
    c.step=0;
  c.status&=~MIXRQ_INTERPOLATE;
  if (!quality)
  {
    if (interpolation>1)
      c.status|=MIXRQ_INTERPOLATE;
    if (interpolation==1)
      if (abs(c.step)<=(3<<15))
        c.status|=MIXRQ_INTERPOLATE;
  }
  else
  {
    if (interpolation>1)
      c.status|=MIXRQ_INTERPOLATEMAX|MIXRQ_INTERPOLATE;
    if (interpolation==1)
    {
      c.status|=MIXRQ_INTERPOLATE;
      c.status&=~MIXRQ_INTERPOLATEMAX;
    }
  }
}

static void calcsteps()
{
  int i;
  for (i=0; i<channelnum; i++)
    calcstep(channels[i]);
}

static void calcspeed()
{
  if (channelnum)
    newtickwidth=imuldiv(256*256*256, samprate, orgspeed*relspeed);
}

static void transformvol(channel &ch)
{
  signed short v;
  v=transform[0][0]*ch.orgvol[0]+transform[0][1]*ch.orgvol[1];
  ch.vol[0]=(v>4096)?64:(v<-4096)?-64:((v+32)>>6);

  v=transform[1][0]*ch.orgvol[0]+transform[1][1]*ch.orgvol[1];
  if (volopt^ch.volopt)
    v=-v;
  ch.vol[1]=(v>4096)?64:(v<-4096)?-64:((v+32)>>6);

  if (ch.status&MIXRQ_MUTE)
  {
    ch.dstvols[0]=ch.dstvols[1]=0;
    return;
  }
  if (!stereo)
  {
    ch.dstvols[0]=(abs(ch.vol[0])+abs(ch.vol[1])+1)>>1;
    ch.dstvols[1]=0;
  }
  else
  {
    ch.dstvols[0]=ch.vol[0];
    ch.dstvols[1]=ch.vol[1];
    if (reversestereo)
    {
      int t=ch.dstvols[1];
      ch.dstvols[1]=ch.dstvols[0];
      ch.dstvols[0]=t;
    }
  }
}

static void calcvol(channel &chn)
{
  if (chn.orgpan<0)
  {
    chn.orgvol[1]=(chn.orgvolx*(0x80+chn.orgpan))>>10;
    chn.orgvol[0]=((chn.orgvolx+3)>>2)-chn.orgvol[1];
  }
  else
  {
    chn.orgvol[0]=(chn.orgvolx*(0x80-chn.orgpan))>>10;
    chn.orgvol[1]=((chn.orgvolx+3)>>2)-chn.orgvol[0];
  }
  transformvol(chn);
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

  volopt=mastersrnd;

  transform[0][0]=vols[0][0];
  transform[0][1]=vols[0][1];
  transform[1][0]=vols[1][0];
  transform[1][1]=vols[1][1];
  int i;
  for (i=0; i<channelnum; i++)
    transformvol(channels[i]);
}

static void fadechanq(int *fade, channel &c)
{
  int s;
  if (c.status&MIXRQ_PLAY16BIT)
    s=((short*)((unsigned long)c.samp*2))[c.pos];
  else
    s=(((signed char*)c.samp)[c.pos])<<8;
  fade[0]+=(c.curvols[0]*s)>>6;
  fade[1]+=(c.curvols[1]*s)>>6;
  c.curvols[0]=c.curvols[1]=0;
}

static void stopchan(channel &c)
{
  if (!(c.status&MIXRQ_PLAYING))
    return;
  if (!quality)
    mixrFadeChannel(fadedown, c);
  else
    fadechanq(fadedown, c);
  c.status&=~MIXRQ_PLAYING;
}

static void amplifyfadeq(int pos, int cl, int &curvol, int dstvol)
{
  int l=abs(dstvol-curvol);
  if (l>cl)
    l=cl;
  if (dstvol<curvol)
  {
    mixqAmplifyChannelDown(buf32+pos, scalebuf, l, curvol, 4<<stereo);
    curvol-=l;
  }
  else
  if (dstvol>curvol)
  {
    mixqAmplifyChannelUp(buf32+pos, scalebuf, l, curvol, 4<<stereo);
    curvol+=l;
  }
  cl-=l;
  if (curvol&&cl)
    mixqAmplifyChannel(buf32+pos+(l<<stereo), scalebuf+l, cl, curvol, 4<<stereo);
}

static void playchannelq(int ch, int pos, int len)
{
  channel &c=channels[ch];
  if (c.status&MIXRQ_PLAYING)
  {
    int quiet=!c.curvols[0]&&!c.curvols[1]&&!c.dstvols[0]&&!c.dstvols[1];
    mixqPlayChannel(scalebuf, len, c, quiet);
    if (quiet)
      return;

    if (stereo)
    {
      amplifyfadeq(pos*2, len, c.curvols[0], c.dstvols[0]);
      amplifyfadeq(pos*2+1, len, c.curvols[1], c.dstvols[1]);
    }
    else
      amplifyfadeq(pos, len, c.curvols[0], c.dstvols[0]);

    if (!(c.status&MIXRQ_PLAYING))
      fadechanq(fadedown, c);
  }
}



static void mixer(int min)
{
  int i;
  if (!channelnum)
    return;
  if (clipbusy)
    return;
  clipbusy++;

  int bufmax=imulshr16(mcpMixMax, samprate);
  if (bufmax>buflen)
    bufmax=buflen;
  int bufmin=bufmax-imulshr16(min, samprate);
  if (bufmin<0)
    bufmin=0;

  int bufdeltatot=((buflen+(plrGetBufPos()>>(stereo+bit16))-bufpos)%buflen)-buflen+bufmax;
  if (bufdeltatot<bufmin)
    bufdeltatot=0;

  if (pause)
  {
    int pass2=((bufpos+bufdeltatot)>buflen)?(bufpos+bufdeltatot-buflen):0;

    if (bit16)
    {
      memsetw((short*)plrbuf+(bufpos<<stereo), signedout?0:0x8000, (bufdeltatot-pass2)<<stereo);
      if (pass2)
        memsetw((short*)plrbuf, signedout?0:0x8000, pass2<<stereo);
    }
    else
    {
      memsetb((char*)plrbuf+(bufpos<<stereo), signedout?0:0x80, (bufdeltatot-pass2)<<stereo);
      if (pass2)
        memsetb((char*)plrbuf, signedout?0:0x80, pass2<<stereo);
    }

    bufpos+=bufdeltatot;
    if (bufpos>=buflen)
      bufpos-=buflen;

    plrAdvanceTo(bufpos<<(stereo+bit16));
    pausesamps+=bufdeltatot;
  }
  else
    while (bufdeltatot>0)
    {
      int bufdelta=(bufdeltatot>MIXBUFLEN)?MIXBUFLEN:bufdeltatot;
      if (bufdelta>(buflen-bufpos))
        bufdelta=buflen-bufpos;
      if (bufdelta>((tickwidth-tickplayed)>>8))
        bufdelta=(tickwidth-tickplayed)>>8;

      mixrFade(buf32, fadedown, bufdelta, stereo);
      if (!quality)
        for (i=0; i<channelnum; i++)
          mixrPlayChannel(buf32, fadedown, bufdelta, channels[i], stereo);
      else
        for (i=0; i<channelnum; i++)
          playchannelq(i, 0, bufdelta);

      mixqpostprocregstruct *mode;
      for (mode=postprocs; mode; mode=mode->next)
        mode->Process(buf32, bufdelta, samprate, stereo);

      mixrClip((char*)plrbuf+(bufpos<<(stereo+bit16)), buf32, bufdelta<<stereo, amptab, clipmax, bit16);

      tickplayed+=bufdelta<<8;
      if (!((tickwidth-tickplayed)>>8))
      {
        tickplayed-=tickwidth;
        playerproc();
        cmdtimerpos+=tickwidth;
        tickwidth=newtickwidth;
      }
      bufpos+=bufdelta;
      if (bufpos>=buflen)
        bufpos-=buflen;

      plrAdvanceTo(bufpos<<(stereo+bit16));
      bufdeltatot-=bufdelta;
      playsamps+=bufdelta;
    }

  clipbusy--;
}

static void timerproc()
{
  mixer(mcpMixPoll);
}











static void SET(int ch, int opt, int val)
{
  if (ch>=channelnum)
    ch=channelnum-1;
  if (ch<0)
    ch=0;
  channel &chn=channels[ch];
  switch (opt)
  {
  case mcpCReset:
    stopchan(chn);
    int reswasmute;
    reswasmute=chn.status&MIXRQ_MUTE;
    memset(&chn, 0, sizeof(channel));
    chn.status=reswasmute;
    break;
  case mcpCInstrument:
    stopchan(chn);
    if ((val<0)||(val>=samplenum))
      break;
    sampleinfo *samp;
    samp=&samples[val];
    chn.samptype=samp->type;
    chn.length=samp->length;
    chn.orgrate=samp->samprate;
    chn.samp=samp->ptr;
    chn.orgloopstart=samp->loopstart;
    chn.orgloopend=samp->loopend;
    chn.orgsloopstart=samp->sloopstart;
    chn.orgsloopend=samp->sloopend;

    chn.status&=~(MIXRQ_PLAYING|MIXRQ_LOOPED|MIXRQ_PINGPONGLOOP|MIXRQ_PLAY16BIT|MIXRQ_PLAYSTEREO);
    if (chn.samptype&mcpSamp16Bit)
    {
      chn.status|=MIXRQ_PLAY16BIT;
      chn.samp=(void*)((unsigned long)chn.samp>>1);
    }
    if (chn.samptype&mcpSampStereo)
    {
      chn.status|=MIXRQ_PLAYSTEREO;
      chn.samp=(void*)((unsigned long)chn.samp>>1);
    }
    if (chn.samptype&mcpSampSLoop)
    {
      chn.status|=MIXRQ_LOOPED;
      chn.loopstart=chn.orgsloopstart;
      chn.loopend=chn.orgsloopend;
      if (chn.samptype&mcpSampSBiDi)
        chn.status|=MIXRQ_PINGPONGLOOP;
    }
    else
    if (chn.samptype&mcpSampLoop)
    {
      chn.status|=MIXRQ_LOOPED;
      chn.loopstart=chn.orgloopstart;
      chn.loopend=chn.orgloopend;
      if (chn.samptype&mcpSampBiDi)
        chn.status|=MIXRQ_PINGPONGLOOP;
     }
    chn.replen=(chn.status&MIXRQ_LOOPED)?(chn.loopend-chn.loopstart):0;
    chn.step=0;
    chn.pos=0;
    chn.fpos=0;
    chn.orgvol[0]=0;
    chn.orgvol[1]=0;
    chn.vol[0]=0;
    chn.vol[1]=0;
    chn.volopt=0;
    break;
  case mcpCStatus:
    if (!val)
      stopchan(chn);
    else
    {
      if (chn.pos>=chn.length)
        break;
      chn.status|=MIXRQ_PLAYING;
      calcstep(chn);
    }
    break;
  case mcpCMute:
    if (val)
      chn.status|=MIXRQ_MUTE;
    else
      chn.status&=~MIXRQ_MUTE;
    transformvol(chn);
    break;
  case mcpCVolume:
    chn.orgvolx=(val>0x100)?0x100:(val<0)?0:val;
    calcvol(chn);
    break;
  case mcpCPanning:
    chn.orgpan=(val>0x80)?0x80:(val<-0x80)?-0x80:val;
    calcvol(chn);
    break;
  case mcpCSurround:
    chn.volopt=val?1:0;
    transformvol(chn);
    break;
  case mcpCDirect:
    if (val==0)
      chn.step=abs(chn.step);
    else
    if (val==1)
      chn.step=-abs(chn.step);
    else
      chn.step=-chn.step;
    break;
  case mcpCLoop:
    chn.status&=~(MIXRQ_LOOPED|MIXRQ_PINGPONGLOOP);
    if ((val==1)&&!(chn.samptype&mcpSampSLoop))
      val=2;
    if ((val==2)&&!(chn.samptype&mcpSampLoop))
      val=0;
    if (val==1)
    {
      chn.status|=MIXRQ_LOOPED;
      chn.loopstart=chn.orgsloopstart;
      chn.loopend=chn.orgsloopend;
      if (chn.samptype&mcpSampSBiDi)
        chn.status|=MIXRQ_PINGPONGLOOP;
    }
    if (val==2)
    {
      chn.status|=MIXRQ_LOOPED;
      chn.loopstart=chn.orgloopstart;
      chn.loopend=chn.orgloopend;
      if (chn.samptype&mcpSampBiDi)
        chn.status|=MIXRQ_PINGPONGLOOP;
    }
    chn.replen=(chn.status&MIXRQ_LOOPED)?(chn.loopend-chn.loopstart):0;
    if (chn.replen)
      if (((chn.pos<chn.loopstart)&&(chn.step<0))||((chn.pos>=chn.loopend)&&(chn.step>0)))
        chn.step=-chn.step;
    break;
  case mcpCPosition:
    int poswasplaying;
    poswasplaying=chn.status&MIXRQ_PLAYING;
    stopchan(chn);
    if (val<0)
      val=0;
    if (val>=chn.length)
      val=chn.length-1;
    chn.pos=val;
    chn.fpos=0;
    chn.status|=poswasplaying;
    break;
  case mcpCPitch:
    chn.orgfrq=8363;
    chn.orgdiv=mcpGetFreq8363(-val);
    calcstep(chn);
    break;
  case mcpCPitchFix:
    chn.orgfrq=val;
    chn.orgdiv=0x10000;
    calcstep(chn);
    break;
  case mcpCPitch6848:
    chn.orgfrq=6848;
    chn.orgdiv=val;
    calcstep(chn);
    break;

  case mcpGSpeed:
    orgspeed=val;
    calcspeed();
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
  case mcpMasterSurround:
    mastersrnd=val?1:0;
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
    interpolation=val;
    break;
  case mcpMasterAmplify:
    amplify=val;
    if (channelnum)
    {
      calcamptab(amplify);
      mixSetAmplify(amplify);
    }
    break;
  case mcpMasterPause:
    pause=val;
    break;
  case mcpGRestrict:
    restricted=val;
    break;
  }
}

static int GET(int ch, int opt)
{
  if (ch>=channelnum)
    ch=channelnum-1;
  if (ch<0)
    ch=0;
  channel &chn=channels[ch];
  switch (opt)
  {
  case mcpCStatus:
    return !!(chn.status&MIXRQ_PLAYING);
  case mcpCMute:
    return !!(chn.status&MIXRQ_MUTE);
  case mcpGTimer:
    if (pause)
      return imuldiv(playsamps, 65536, samprate);
    else
      return plrGetTimer()-imuldiv(pausesamps, 65536, samprate);
  case mcpGCmdTimer:
    return umuldiv(cmdtimerpos, 256, samprate);
  }
  return 0;
}

static void Idle()
{
  mixer(mcpMixMax);
  if (plrIdle)
    plrIdle();
}



static void GetMixChannel(int ch, mixchannel &chn, int rate)
{
  channel &c=channels[ch];
  if (c.status&MIXRQ_PLAY16BIT)
    chn.samp=(void*)((unsigned long)c.samp<<1);
  else
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
  if (c.status&MIXRQ_MUTE)
    chn.status|=MIX_MUTE;
  if (c.status&MIXRQ_PLAY16BIT)
    chn.status|=MIX_PLAY16BIT;
  if (c.status&MIXRQ_LOOPED)
    chn.status|=MIX_LOOPED;
  if (c.status&MIXRQ_PINGPONGLOOP)
    chn.status|=MIX_PINGPONGLOOP;
  if (c.status&MIXRQ_PLAYING)
    chn.status|=MIX_PLAYING;
  if (c.status&MIXRQ_INTERPOLATE)
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
  fadedown[0]=fadedown[1]=0;
  playsamps=pausesamps=0;
  if (chan>MAXCHAN)
    chan=MAXCHAN;

  if (!plrPlay)
    return 0;

  unsigned long currentrate=mcpMixProcRate/chan;
  unsigned short mixrate=(currentrate>mcpMixMaxRate)?mcpMixMaxRate:currentrate;
  plrSetOptions(mixrate, mcpMixOpt|(restricted?PLR_RESTRICTED:0));

  playerproc=proc;

  if (!quality)
  {
    scalebuf=0;
    voltabsq=0;
    interpoltabq=0;
    interpoltabq2=0;
    voltabsr=new long [129][256];
    interpoltabr=new unsigned char [16][256][2];
    if (!interpoltabr||!voltabsr)
    {
      delete interpoltabr;
      delete voltabsr;
      return 0;
    }
  }
  else
  {
    voltabsr=0;
    interpoltabr=0;
    scalebuf=new short [MIXBUFLEN];
    voltabsq=new short [129][2][256];
    interpoltabq=new unsigned short [2][32][256][2];
    interpoltabq2=new unsigned short [2][16][256][4];
    if (!voltabsq||!interpoltabq2||!interpoltabq||!scalebuf)
    {
      delete interpoltabq;
      delete interpoltabq2;
      delete voltabsq;
      delete scalebuf;
      return 0;
    }
  }
  buf32=new long [MIXBUFLEN<<1];
  amptab=new short [3][256];
  channels=new channel[chan];
  if (!buf32||!amptab||!channels)
  {
    delete buf32;
    delete amptab;
    delete channels;
    return 0;
  }

  mcpGetMasterSample=plrGetMasterSample;
  mcpGetRealMasterVolume=plrGetRealMasterVolume;
  if (!mixInit(GetMixChannel, resample, chan, amplify))
    return 0;

  memset(channels, 0, sizeof(channel)*chan);
  calcvols();

  if (!quality)
  {
    mixrSetupAddresses(voltabsr+64, interpoltabr);
    calcinterpoltabr();
    calcvoltabsr();
  }
  else
  {
    mixqSetupAddresses(voltabsq+64, interpoltabq, interpoltabq2);
    calcinterpoltabq();
    calcvoltabsq();
  }

  if (!plrOpenPlayer(plrbuf, buflen, mcpMixBufSize))
  {
    mixClose();
    return 0;
  }

  stereo=(plrOpt&PLR_STEREO)?1:0;
  bit16=(plrOpt&PLR_16BIT)?1:0;
  signedout=!!(plrOpt&PLR_SIGNEDOUT);
  reversestereo=!!(plrOpt&PLR_REVERSESTEREO);
  samprate=plrRate;
  bufpos=0;
  pause=0;
  orgspeed=12800;

  channelnum=chan;
  mcpNChan=chan;
  mcpIdle=Idle;

  calcamptab(amplify);
  calcspeed();
//  playerproc();  // some timing is wrong here!
  tickwidth=newtickwidth;
  tickplayed=0;
  cmdtimerpos=0;

  if (!pollInit(timerproc))
  {
    mcpNChan=0;
    mcpIdle=0;
    plrClosePlayer();
    mixClose();
    return 0;
  }

  mixqpostprocregstruct *mode;
  for (mode=postprocs; mode; mode=mode->next)
    mode->Init(samprate, stereo);

  return 1;
}

static void ClosePlayer()
{
  mcpNChan=0;
  mcpIdle=0;

  pollClose();

  plrClosePlayer();

  channelnum=0;

  mixClose();

  delete interpoltabr;
  delete voltabsr;
  delete voltabsq;
  delete interpoltabq;
  delete interpoltabq2;
  delete scalebuf;
  delete channels;
  delete amptab;
  delete buf32;
}

static int Init(const deviceinfo &dev)
{
  resample=!!(dev.opt&MIXRQ_RESAMPLE);
  quality=!!dev.subtype;

  restricted=0;
  amplify=65535;
  relspeed=256;
  relpitch=256;
  interpolation=0;
  mastervol=64;
  masterbal=0;
  masterpan=64;
  mastersrnd=0;
  channelnum=0;

  mcpLoadSamples=LoadSamples;
  mcpOpenPlayer=OpenPlayer;
  mcpClosePlayer=ClosePlayer;
  mcpGet=GET;
  mcpSet=SET;

  return 1;
}

static void Close()
{
  mcpOpenPlayer=0;
}


static int Detect(deviceinfo &c)
{
  c.dev=&mcpMixer;
  c.port=-1;
  c.port2=-1;
  c.irq=-1;
  c.irq2=-1;
  c.dma=-1;
  c.dma2=-1;
  if (c.subtype==-1)
    c.subtype=0;
  c.chan=(MAXCHAN>99)?99:MAXCHAN;
  c.mem=0;
  return 1;
}

extern "C" sounddevice mcpMixer={SS_WAVETABLE|SS_NEEDPLAYER, "Mixer", Detect, Init, Close};

void mixrRegisterPostProc(mixqpostprocregstruct *mode)
{
  mode->next=postprocs;
  postprocs=mode;
}

#ifdef CPDOS

#include "devigen.h"
#include "psetting.h"
#include "deviplay.h"
#include "pmain.h"
#include "plinkman.h"

static mixqpostprocaddregstruct *postprocadds;

static unsigned long mixGetOpt(const char *sec)
{
  unsigned long opt=0;
  if (cfGetProfileBool(sec, "mixresample", 0, 0))
    opt|=MIXRQ_RESAMPLE;
  return opt;
}

static void mixrInit(const char *sec)
{
  postprocs=0;
  char regname[50];
  const char *regs;
  regs=cfGetProfileString(sec, "postprocs", "");
  while (cfGetSpaceListEntry(regname, regs, 49))
  {
    void *reg=lnkGetSymbol(regname);
    if (reg)
      mixrRegisterPostProc((mixqpostprocregstruct*)reg);
  }
  postprocadds=0;
  regs=cfGetProfileString(sec, "postprocadds", "");
  while (cfGetSpaceListEntry(regname, regs, 49))
  {
    void *reg=lnkGetSymbol(regname);
    if (reg)
    {
      ((mixqpostprocaddregstruct*)reg)->next=postprocadds;
      postprocadds=(mixqpostprocaddregstruct*)reg;
    }
  }
}

static int mixProcKey(unsigned short key)
{
  mixqpostprocaddregstruct *mode;
  for (mode=postprocadds; mode; mode=mode->next)
  {
    int r=mode->ProcessKey(key);
    if (r)
      return r;
  }

  if (plrProcessKey)
    return plrProcessKey(key);
  return 0;
}

extern "C" devaddstruct mcpMixAdd = {mixGetOpt, mixrInit, 0, mixProcKey};

#endif
