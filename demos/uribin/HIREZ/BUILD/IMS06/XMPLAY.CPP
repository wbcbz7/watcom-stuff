// (c) 1994-1997 Niklas Beisert, this file is part of "Indoor Music System".
// you must not read, modify, print, compile or copy this file or parts of
// it unless you have accepted the license in the accompanying file IMS.TXT.

// channel mapper

#include <string.h>
#include "mcp.h"
#include "binfile.h"
#include "gmdinst.h"
#include "xmplay.h"
#include "err.h"

struct channel
{
  int chVol;
  int chFinalVol;
  int chPan;
  int chFinalPan;
  long chPitch;
  long chFinalPitch;

  unsigned char chCurIns;
  int chCurNormNote;
  unsigned char chSustain;
  unsigned short chFadeVol;
  unsigned short chAVibPos;
  unsigned long chAVibSwpPos;
  unsigned long chVolEnvPos;
  unsigned long chPanEnvPos;

  unsigned char chDefVol;
  int chDefPan;
  unsigned char chCommand;
  unsigned char chVCommand;
  long chPortaToPitch;
  long chPortaToVal;
  unsigned char chVolSlideVal;
  unsigned char chGVolSlideVal;
  unsigned char chVVolPanSlideVal;
  unsigned char chPanSlideVal;
  unsigned char chFineVolSlideUVal;
  unsigned char chFineVolSlideDVal;
  long chPortaUVal;
  long chPortaDVal;
  unsigned char chFinePortaUVal;
  unsigned char chFinePortaDVal;
  unsigned char chXFinePortaUVal;
  unsigned char chXFinePortaDVal;
  unsigned char chVibRate;
  unsigned char chVibPos;
  unsigned char chVibType;
  unsigned char chVibDep;
  unsigned char chTremRate;
  unsigned char chTremPos;
  unsigned char chTremType;
  unsigned char chTremDep;
  unsigned char chPatLoopCount;
  unsigned char chPatLoopStart;
  unsigned char chArpPos;
  unsigned char chArpNotes[3];
  unsigned char chActionTick;
  unsigned char chMRetrigPos;
  unsigned char chMRetrigLen;
  unsigned char chMRetrigAct;
  unsigned char chDelayNote;
  unsigned char chDelayVol;
  unsigned char chOffset;
  unsigned char chGlissando;
  unsigned char chTremorPos;
  unsigned char chTremorLen;
  unsigned char chTremorOff;
  unsigned char chSync;
  int chSyncTime;
  int delayfreq;

  int nextstop;
  int nextsamp;
  int nextpos;
  gmdsample *cursamp;

  int evpos0;
  int evmodtype;
  int evmod;
  int evmodpos;
  int evpos;
  int evtime;
};

static int looping;
static int looped;
static int usersetpos;
static channel channels[32];

static unsigned char mutech[32];
static unsigned char globalvol;

static unsigned char curtick;
static unsigned char curtempo;
static unsigned char tick0;

static int currow;
static unsigned char (*patptr)[5];
static int patlen;
static int curord;

static int nord;
static int ninst;
static int nsamp;
static int linearfreq;
static int nchan;
static int loopord;
static int nenv;
static gmdinstrument *instruments;
static gmdsample *samples;
static gmdenvelope *envelopes;
static unsigned char (**patterns)[5];
static unsigned short *orders;
static unsigned short *patlens;

static int jumptoord;
static int jumptorow;
static int patdelay;

static unsigned char procnot;
static unsigned char procins;
static unsigned char procvol;
static unsigned char proccmd;
static unsigned char procdat;
static int firstspeed;
static int curbpm;

static int realsync;
static int realsynctime;

static int realpos;

static int (*que)[4];
static int querpos;
static int quewpos;
static int quelen;
static int cmdtime;
static int realtempo;
static int realspeed;
static int realgvol;


enum
{
  quePos, queSync, queTempo, queSpeed, queGVol,
};

static short sintab[256]=
{
      0,    50,   100,   151,   201,   251,   301,   350,
    400,   449,   498,   546,   595,   642,   690,   737,
    784,   830,   876,   921,   965,  1009,  1053,  1096,
   1138,  1179,  1220,  1260,  1299,  1338,  1375,  1412,
   1448,  1483,  1517,  1551,  1583,  1615,  1645,  1674,
   1703,  1730,  1757,  1782,  1806,  1829,  1851,  1872,
   1892,  1911,  1928,  1945,  1960,  1974,  1987,  1998,
   2009,  2018,  2026,  2033,  2038,  2042,  2046,  2047,
   2048,  2047,  2046,  2042,  2038,  2033,  2026,  2018,
   2009,  1998,  1987,  1974,  1960,  1945,  1928,  1911,
   1892,  1872,  1851,  1829,  1806,  1782,  1757,  1730,
   1703,  1674,  1645,  1615,  1583,  1551,  1517,  1483,
   1448,  1412,  1375,  1338,  1299,  1260,  1220,  1179,
   1138,  1096,  1053,  1009,   965,   921,   876,   830,
    784,   737,   690,   642,   595,   546,   498,   449,
    400,   350,   301,   251,   201,   151,   100,    50,
      0,   -50,  -100,  -151,  -201,  -251,  -301,  -350,
   -400,  -449,  -498,  -546,  -595,  -642,  -690,  -737,
   -784,  -830,  -876,  -921,  -965, -1009, -1053, -1096,
  -1138, -1179, -1220, -1260, -1299, -1338, -1375, -1412,
  -1448, -1483, -1517, -1551, -1583, -1615, -1645, -1674,
  -1703, -1730, -1757, -1782, -1806, -1829, -1851, -1872,
  -1892, -1911, -1928, -1945, -1960, -1974, -1987, -1998,
  -2009, -2018, -2026, -2033, -2038, -2042, -2046, -2047,
  -2048, -2047, -2046, -2042, -2038, -2033, -2026, -2018,
  -2009, -1998, -1987, -1974, -1960, -1945, -1928, -1911,
  -1892, -1872, -1851, -1829, -1806, -1782, -1757, -1730,
  -1703, -1674, -1645, -1615, -1583, -1551, -1517, -1483,
  -1448, -1412, -1375, -1338, -1299, -1260, -1220, -1179,
  -1138, -1096, -1053, -1009,  -965,  -921,  -876,  -830,
   -784,  -737,  -690,  -642,  -595,  -546,  -498,  -449,
   -400,  -350,  -301,  -251,  -201,  -151,  -100,   -50
};

static int freqrange(int x)
{
  if (linearfreq)
    return (x<-72*256)?-72*256:(x>96*256)?96*256:x;
  else
    return (x<107)?107:(x>438272)?438272:x;
}

static int volrange(int x)
{
  return (x<0)?0:(x>0x40)?0x40:x;
}

static int panrange(int x)
{
  return (x<0)?0:(x>0xFF)?0xFF:x;
}


static void ReadQue()
{
  int type,val1,val2,t;
  int i;
  int time=mcpGet(-1, mcpGTimer);
  while (1)
  {
    if (querpos==quewpos)
      break;
    if (time<que[querpos][0])
      break;

    t=que[querpos][0];
    type=que[querpos][1];
    val1=que[querpos][2];
    val2=que[querpos][3];
    querpos=(querpos+1)%quelen;

    switch (type)
    {
    case queSync:
      realsync=val2;
      realsynctime=t;
      channels[val1].chSync=val2;
      channels[val1].chSyncTime=t;
      break;
    case quePos:
      realpos=val2;
      for (i=0; i<nchan; i++)
      {
        channel &c=channels[i];
        if (c.evpos==-1)
        {
          if (c.evpos0==realpos)
          {
            c.evpos=realpos;
            c.evtime=t;
          }
        }
        else
        {
          switch (c.evmodtype)
          {
          case 1:
            c.evmodpos++;
            break;
          case 2:
            if (!(realpos&0xFF))
              c.evmodpos++;
            break;
          case 3:
            if (!(realpos&0xFFFF))
              c.evmodpos++;
            break;
          }
          if ((c.evmodpos==c.evmod)&&c.evmod)
          {
            c.evmodpos=0;
            c.evpos=realpos;
            c.evtime=t;
          }
        }
      }
      break;
    case queGVol: realgvol=val2; break;
    case queTempo: realtempo=val2; break;
    case queSpeed: realspeed=val2; break;
    }
  }
}

static void putque(int type, int val1, int val2)
{
  if (((quewpos+1)%quelen)==querpos)
    return;
  que[quewpos][0]=cmdtime;
  que[quewpos][1]=type;
  que[quewpos][2]=val1;
  que[quewpos][3]=val2;
  quewpos=(quewpos+1)%quelen;
}

static void PlayNote(channel &ch)
{
  int portatmp=0;

  if (proccmd==xmpCmdPortaNote)
    portatmp=1;
  if (proccmd==xmpCmdPortaVol)
    portatmp=1;
  if ((procvol>>4)==xmpVCmdPortaNote)
    portatmp=1;

  int keyoff=0;
  if (procnot==97)
  {
    procnot=0;
    keyoff=1;
  }
  if ((proccmd==xmpCmdKeyOff)&&!procdat)
    keyoff=1;

  if (!ch.chCurIns)
    return;

  if (procnot)
  {
    if (procins!=0)
      ch.chSustain=1;

    if ((proccmd==xmpCmdDelayNote)&&procdat)
      return;

    if (!portatmp)
    {
      ch.nextstop=1;

      gmdinstrument &ins=instruments[ch.chCurIns-1];
      if (ins.samples[procnot-1]>nsamp)
        return;
      ch.cursamp=&samples[ins.samples[procnot-1]];

      ch.nextsamp=ch.cursamp->handle;

      if (procins)
      {
        ch.chDefVol=(ch.cursamp->stdvol+1)>>2;
        ch.chDefPan=ch.cursamp->stdpan;
      }

      ch.chCurNormNote=ch.cursamp->normnote;

      int frq=48*256-(((procnot-1)<<8)-ch.chCurNormNote);
      if (!linearfreq)
        frq=mcpGetFreq6848(frq);
      ch.chPitch=frq;
      ch.chFinalPitch=frq;
      ch.chPortaToPitch=frq;

      ch.nextpos=0;

      if (proccmd==xmpCmdOffset)
      {
        if (procdat!=0)
          ch.chOffset=procdat;
        ch.nextpos=ch.chOffset<<8;
      }

      ch.chVibPos=0;
      ch.chTremPos=0;
      ch.chArpPos=0;
      ch.chMRetrigPos=0;
      ch.chTremorPos=0;
    }
    else
    {
      int frq=48*256-(((procnot-1)<<8)-ch.chCurNormNote);
      if (!linearfreq)
        frq=mcpGetFreq6848(frq);
      ch.chPortaToPitch=frq;
    }
  }

  if (keyoff&&ch.cursamp)
  {
    ch.chSustain=0;
    if ((ch.cursamp->volenv>=nenv)&&!procins)
      ch.chFadeVol=0;
  }

  if (!ch.chSustain)
    return;
  if (!procins)
    return;

  ch.chVol=ch.chDefVol;
  ch.chFinalVol=ch.chDefVol;
  if (ch.chDefPan!=-1)
  {
    ch.chPan=ch.chDefPan;
    ch.chFinalPan=ch.chDefPan;
  }
  ch.chFadeVol=0x8000;
  ch.chAVibPos=0;
  ch.chAVibSwpPos=0;
  ch.chVolEnvPos=0;
  ch.chPanEnvPos=0;
}

static unsigned short notetab[16]={32768,30929,29193,27554,26008,24548,23170,21870,20643,19484,18390,17358,16384,15464,14596,13777};

static void xmpPlayTick()
{
  if (firstspeed)
  {
    mcpSet(-1, mcpGSpeed, firstspeed);
    firstspeed=0;
  }

  cmdtime=mcpGet(-1, mcpGCmdTimer);
  ReadQue();

  tick0=0;
  int i;
  for (i=0; i<nchan; i++)
  {
    channel &ch=channels[i];
    ch.chFinalVol=ch.chVol;
    ch.chFinalPan=ch.chPan;
    ch.chFinalPitch=ch.chPitch;
    ch.nextstop=0;
    ch.nextsamp=-1;
    ch.nextpos=-1;
  }

  curtick++;
  if (curtick>=curtempo)
    curtick=0;

  if (!curtick&&patdelay)
  {
    if (jumptoord!=-1)
    {
      if (jumptoord!=curord)
        for (i=0; i<nchan; i++)
        {
          channel &ch=channels[i];
          ch.chPatLoopCount=0;
          ch.chPatLoopStart=0;
        }

      if (jumptoord>=nord)
        jumptoord=loopord;
      if ((jumptoord<curord)&&!usersetpos)
        looped=1;
      usersetpos=0;

      curord=jumptoord;
      currow=jumptorow;
      jumptoord=-1;
      patlen=patlens[orders[curord]];
      patptr=patterns[orders[curord]];
    }
  }

  if (!curtick&&patdelay)
  {
    patdelay--;
  }
  else
  if (!curtick)
  {
    tick0=1;

    currow++;
    if ((jumptoord==-1)&&(currow>=patlen))
    {
      jumptoord=curord+1;
      jumptorow=0;
    }
    if (jumptoord!=-1)
    {
      if (jumptoord!=curord)
        for (i=0; i<nchan; i++)
        {
          channel &ch=channels[i];
          ch.chPatLoopCount=0;
          ch.chPatLoopStart=0;
        }

      if (jumptoord>=nord)
        jumptoord=loopord;
      if ((jumptoord<curord)&&!usersetpos)
        looped=1;
      usersetpos=0;

      curord=jumptoord;
      currow=jumptorow;
      jumptoord=-1;
      patlen=patlens[orders[curord]];
      patptr=patterns[orders[curord]];
    }


    for (i=0; i<nchan; i++)
    {
      channel &ch=channels[i];
      procnot=patptr[nchan*currow+i][0];
      procins=patptr[nchan*currow+i][1];
      procvol=patptr[nchan*currow+i][2];
      proccmd=patptr[nchan*currow+i][3];
      procdat=patptr[nchan*currow+i][4];

      if ((procins<=ninst)&&procins)
        ch.chCurIns=procins;

      PlayNote(ch);

      ch.chVCommand=procvol>>4;

      switch (ch.chVCommand)
      {
      case xmpVCmdVol0x: case xmpVCmdVol1x: case xmpVCmdVol2x: case xmpVCmdVol3x:
        if ((proccmd!=xmpCmdDelayNote)||!procdat)
          ch.chFinalVol=ch.chVol=procvol-0x10;
        break;
      case xmpVCmdVol40:
        if ((proccmd!=xmpCmdDelayNote)||!procdat)
          ch.chFinalVol=ch.chVol=0x40;
        break;
      case xmpVCmdVolSlideD: case xmpVCmdVolSlideU: case xmpVCmdPanSlideL: case xmpVCmdPanSlideR:
        ch.chVVolPanSlideVal=procvol&0xF;
        break;
      case xmpVCmdFVolSlideD:
        if ((proccmd!=xmpCmdDelayNote)||!procdat)
          ch.chFinalVol=ch.chVol=volrange(ch.chVol-(procvol&0xF));
        break;
      case xmpVCmdFVolSlideU:
        if ((proccmd!=xmpCmdDelayNote)||!procdat)
          ch.chFinalVol=ch.chVol=volrange(ch.chVol+(procvol&0xF));
        break;
      case xmpVCmdVibRate:
        if (procvol&0xF)
          ch.chVibRate=((procvol&0xF)<<2);
        break;
      case xmpVCmdVibDep:
        if (procvol&0xF)
          ch.chVibDep=((procvol&0xF)<<2);
        break;
      case xmpVCmdPanning:
        if ((proccmd!=xmpCmdDelayNote)||!procdat)
          ch.chFinalPan=ch.chPan=(procvol&0xF)*0x11;
        break;
      case xmpVCmdPortaNote:
        if (procvol&0xF)
          ch.chPortaToVal=(procvol&0xF)<<8;
        break;
      }

      ch.chCommand=proccmd;
      switch (ch.chCommand)
      {
      case xmpCmdArpeggio:
        if (!procdat)
          ch.chCommand=0xFF;
        ch.chArpNotes[0]=0;
        ch.chArpNotes[1]=procdat>>4;
        ch.chArpNotes[2]=procdat&0xF;
        break;
      case xmpCmdPortaU:
        if (procdat)
          ch.chPortaUVal=procdat<<4;
        break;
      case xmpCmdPortaD:
        if (procdat)
          ch.chPortaDVal=procdat<<4;
        break;
      case xmpCmdPortaNote:
        if (procdat)
          ch.chPortaToVal=procdat<<4;
        break;
      case xmpCmdVibrato:
        if (procdat&0xF)
          ch.chVibDep=(procdat&0xF)<<2;
        if (procdat&0xF0)
          ch.chVibRate=(procdat>>4)<<2;
        break;
      case xmpCmdPortaVol: case xmpCmdVibVol: case xmpCmdVolSlide:
        if (procdat)
          ch.chVolSlideVal=procdat;
        break;
      case xmpCmdTremolo:
        if (procdat&0xF)
          ch.chTremDep=(procdat&0xF)<<2;
        if (procdat&0xF0)
          ch.chTremRate=(procdat>>4)<<2;
        break;
      case xmpCmdPanning:
        ch.chFinalPan=ch.chPan=procdat;
        break;
      case xmpCmdJump:
        jumptoord=procdat;
        jumptorow=0;
        break;
      case xmpCmdVolume:
        ch.chFinalVol=ch.chVol=volrange(procdat);
        break;
      case xmpCmdBreak:
        if (jumptoord==-1)
          jumptoord=curord+1;
        jumptorow=(procdat&0xF)+(procdat>>4)*10;
        break;
      case xmpCmdSpeed:
        if (!procdat)
        {
          jumptoord=procdat;
          jumptorow=0;
          break;
        }
        if (procdat>=0x20)
        {
          curbpm=procdat;
          mcpSet(-1, mcpGSpeed, 256*2*curbpm/5);
          putque(queTempo, -1, curbpm);
        }
        else
        {
          curtempo=procdat;
          putque(queSpeed, -1, curtempo);
        }
        break;
      case xmpCmdGVolume:
        globalvol=volrange(procdat);
        putque(queGVol, -1, globalvol);
        break;
      case xmpCmdGVolSlide:
        if (procdat)
          ch.chGVolSlideVal=procdat;
        break;
      case xmpCmdKeyOff: case xmpCmdRetrigger: case xmpCmdNoteCut:
        ch.chActionTick=procdat;
        break;
      case xmpCmdEnvPos:
        ch.chVolEnvPos=ch.chPanEnvPos=procdat;
        if (ch.cursamp->volenv<nenv)
          if (ch.chVolEnvPos>envelopes[ch.cursamp->volenv].len)
            ch.chVolEnvPos=envelopes[ch.cursamp->volenv].len;
        if (ch.cursamp->panenv<nenv)
          if (ch.chPanEnvPos>envelopes[ch.cursamp->panenv].len)
            ch.chPanEnvPos=envelopes[ch.cursamp->panenv].len;
        break;
      case xmpCmdPanSlide:
        if (procdat)
          ch.chPanSlideVal=procdat;
        break;
      case xmpCmdMRetrigger:
        if (procdat)
        {
          ch.chMRetrigLen=procdat&0xF;
          ch.chMRetrigAct=procdat>>4;
        }
        break;
      case xmpCmdSync1: case xmpCmdSync2: case xmpCmdSync3:
        putque(queSync, i, procdat);
        break;
      case xmpCmdTremor:
        if (procdat)
        {
          ch.chTremorLen=(procdat&0xF)+(procdat>>4)+2;
          ch.chTremorOff=(procdat>>4)+1;
          ch.chTremorPos=0;
        }
        break;
      case xmpCmdXPorta:
        if ((procdat>>4)==1)
        {
          if (procdat&0xF)
            ch.chXFinePortaUVal=procdat&0xF;
          ch.chFinalPitch=ch.chPitch=freqrange(ch.chPitch-(ch.chXFinePortaUVal<<2));
        }
        else
        if ((procdat>>4)==2)
        {
          if (procdat&0xF)
            ch.chXFinePortaDVal=procdat&0xF;
          ch.chFinalPitch=ch.chPitch=freqrange(ch.chPitch+(ch.chXFinePortaDVal<<2));
        }
        break;
      case xmpCmdFPortaU:
        if (procdat)
          ch.chFinePortaUVal=procdat;
        ch.chFinalPitch=ch.chPitch=freqrange(ch.chPitch-(ch.chFinePortaUVal<<4));
        break;
      case xmpCmdFPortaD:
        if (procdat)
          ch.chFinePortaDVal=procdat;
        ch.chFinalPitch=ch.chPitch=freqrange(ch.chPitch+(ch.chFinePortaDVal<<4));
        break;
      case xmpCmdGlissando:
        ch.chGlissando=procdat;
        break;
      case xmpCmdVibType:
        ch.chVibType=procdat&3;
        break;
      case xmpCmdPatLoop:
        if (!procdat)
          ch.chPatLoopStart=currow;
        else
        {
          ch.chPatLoopCount++;
          if (ch.chPatLoopCount<=procdat)
          {
            jumptorow=ch.chPatLoopStart;
            jumptoord=curord;
          }
          else
          {
            ch.chPatLoopCount=0;
            ch.chPatLoopStart=currow+1;
          }
        }
        break;
      case xmpCmdTremType:
        ch.chTremType=procdat&3;
        break;
      case xmpCmdSPanning:
        ch.chFinalPan=ch.chPan=procdat*0x11;
        break;
      case xmpCmdFVolSlideU:
        if (procdat)
          ch.chFineVolSlideUVal=procdat;
        ch.chFinalVol=ch.chVol=volrange(ch.chVol+ch.chFineVolSlideUVal);
        break;
      case xmpCmdFVolSlideD:
        if (procdat)
          ch.chFineVolSlideDVal=procdat;
        ch.chFinalVol=ch.chVol=volrange(ch.chVol-ch.chFineVolSlideDVal);
        break;
      case xmpCmdPatDelay:
        patdelay=procdat;
        break;
      case xmpCmdDelayNote:
        if (procnot)
          ch.chDelayNote=procnot;
        ch.chDelayVol=procvol;
        ch.chActionTick=procdat;
        break;
      }
    }
  }

  for (i=0; i<nchan; i++)
  {
    channel &ch=channels[i];

    switch (ch.chVCommand)
    {
    case xmpVCmdVolSlideD:
      if (tick0)
        break;
      ch.chFinalVol=ch.chVol=volrange(ch.chVol-ch.chVVolPanSlideVal);
      break;
    case xmpVCmdVolSlideU:
      if (tick0)
        break;
      ch.chFinalVol=ch.chVol=volrange(ch.chVol+ch.chVVolPanSlideVal);
      break;
    case xmpVCmdVibDep:
      switch (ch.chVibType)
      {
      case 0:
        ch.chFinalPitch=freqrange((( sintab[ch.chVibPos] *ch.chVibDep)>>8)+ch.chPitch);
        break;
      case 1:
        ch.chFinalPitch=freqrange((( (ch.chVibPos-0x80)   *ch.chVibDep)>>4)+ch.chPitch);
        break;
      case 2:
        ch.chFinalPitch=freqrange((( ((ch.chVibPos&0x80)-0x40) *ch.chVibDep)>>3)+ch.chPitch);
        break;
      }
      if (!tick0)
        ch.chVibPos+=ch.chVibRate;
      break;
    case xmpVCmdPanSlideL:
      if (tick0)
        break;
      ch.chFinalPan=ch.chPan=panrange(ch.chPan-ch.chVVolPanSlideVal);
      break;
    case xmpVCmdPanSlideR:
      if (tick0)
        break;
      ch.chFinalPan=ch.chPan=panrange(ch.chPan+ch.chVVolPanSlideVal);
      break;
    case xmpVCmdPortaNote:
      if (!tick0)
        if (ch.chPitch<ch.chPortaToPitch)
        {
          ch.chPitch+=ch.chPortaToVal;
          if (ch.chPitch>ch.chPortaToPitch)
            ch.chPitch=ch.chPortaToPitch;
        }
        else
        {
          ch.chPitch-=ch.chPortaToVal;
          if (ch.chPitch<ch.chPortaToPitch)
            ch.chPitch=ch.chPortaToPitch;
        }
      if (ch.chGlissando)
      {
        if (linearfreq)
          ch.chFinalPitch=((ch.chPitch+ch.chCurNormNote+0x80)&~0xFF)-ch.chCurNormNote;
        else
          ch.chFinalPitch=mcpGetFreq6848(((mcpGetNote6848(ch.chPitch)+ch.chCurNormNote+0x80)&~0xFF)-ch.chCurNormNote);
      }
      else
        ch.chFinalPitch=ch.chPitch;
      break;
    }

    switch (ch.chCommand)
    {
    case xmpCmdArpeggio:
      if (linearfreq)
        ch.chFinalPitch=freqrange(ch.chPitch-(ch.chArpNotes[ch.chArpPos]<<8));
      else
        ch.chFinalPitch=freqrange((ch.chPitch*notetab[ch.chArpNotes[ch.chArpPos]])>>15);
      ch.chArpPos++;
      if (ch.chArpPos==3)
         ch.chArpPos=0;
      break;
    case xmpCmdPortaU:
      if (tick0)
        break;
      ch.chFinalPitch=ch.chPitch=freqrange(ch.chPitch-ch.chPortaUVal);
      break;
    case xmpCmdPortaD:
      if (tick0)
        break;
      ch.chFinalPitch=ch.chPitch=freqrange(ch.chPitch+ch.chPortaDVal);
      break;
    case xmpCmdPortaNote:
      if (!tick0)
        if (ch.chPitch<ch.chPortaToPitch)
        {
          ch.chPitch+=ch.chPortaToVal;
          if (ch.chPitch>ch.chPortaToPitch)
            ch.chPitch=ch.chPortaToPitch;
        }
        else
        {
          ch.chPitch-=ch.chPortaToVal;
          if (ch.chPitch<ch.chPortaToPitch)
            ch.chPitch=ch.chPortaToPitch;
        }
      if (ch.chGlissando)
      {
        if (linearfreq)
          ch.chFinalPitch=((ch.chPitch+ch.chCurNormNote+0x80)&~0xFF)-ch.chCurNormNote;
        else
          ch.chFinalPitch=mcpGetFreq6848(((mcpGetNote6848(ch.chPitch)+ch.chCurNormNote+0x80)&~0xFF)-ch.chCurNormNote);
      }
      else
        ch.chFinalPitch=ch.chPitch;
      break;
    case xmpCmdVibrato:
      switch (ch.chVibType)
      {
      case 0:
        ch.chFinalPitch=freqrange((( sintab[ch.chVibPos] *ch.chVibDep)>>8)+ch.chPitch);
        break;
      case 1:
        ch.chFinalPitch=freqrange((( (ch.chVibPos-0x80)   *ch.chVibDep)>>4)+ch.chPitch);
        break;
      case 2:
        ch.chFinalPitch=freqrange((( ((ch.chVibPos&0x80)-0x40) *ch.chVibDep)>>3)+ch.chPitch);
        break;
      }
      if (!tick0)
        ch.chVibPos+=ch.chVibRate;
      break;
    case xmpCmdPortaVol:
      if (!tick0)
        if (ch.chPitch<ch.chPortaToPitch)
        {
          ch.chPitch+=ch.chPortaToVal;
          if (ch.chPitch>ch.chPortaToPitch)
            ch.chPitch=ch.chPortaToPitch;
        }
        else
        {
          ch.chPitch-=ch.chPortaToVal;
          if (ch.chPitch<ch.chPortaToPitch)
            ch.chPitch=ch.chPortaToPitch;
        }
      if (ch.chGlissando)
      {
        if (linearfreq)
          ch.chFinalPitch=((ch.chPitch+ch.chCurNormNote+0x80)&~0xFF)-ch.chCurNormNote;
        else
          ch.chFinalPitch=mcpGetFreq6848(((mcpGetNote6848(ch.chPitch)+ch.chCurNormNote+0x80)&~0xFF)-ch.chCurNormNote);
      }
      else
        ch.chFinalPitch=ch.chPitch;

      if (tick0)
        break;
      ch.chFinalVol=ch.chVol=volrange(ch.chVol+((ch.chVolSlideVal&0xF0)?(ch.chVolSlideVal>>4):-(ch.chVolSlideVal&0xF)));
      break;
    case xmpCmdVibVol:
      switch (ch.chVibType)
      {
      case 0:
        ch.chFinalPitch=freqrange((( sintab[ch.chVibPos] *ch.chVibDep)>>8)+ch.chPitch);
        break;
      case 1:
        ch.chFinalPitch=freqrange((( (ch.chVibPos-0x80)   *ch.chVibDep)>>4)+ch.chPitch);
        break;
      case 2:
        ch.chFinalPitch=freqrange((( ((ch.chVibPos&0x80)-0x40) *ch.chVibDep)>>3)+ch.chPitch);
        break;
      }
      if (!tick0)
        ch.chVibPos+=ch.chVibRate;

      if (tick0)
        break;
      ch.chFinalVol=ch.chVol=volrange(ch.chVol+((ch.chVolSlideVal&0xF0)?(ch.chVolSlideVal>>4):-(ch.chVolSlideVal&0xF)));
      break;
    case xmpCmdTremolo:
      switch (ch.chTremType)
      {
      case 0:
        ch.chFinalVol+=(( sintab[ch.chTremPos] *ch.chTremDep)>>11);
        break;
      case 1:
        ch.chFinalVol+=(( (ch.chTremPos-0x80)   *ch.chTremDep)>>7);
        break;
      case 2:
        ch.chFinalVol+=(( ((ch.chTremPos&0x80)-0x40) *ch.chTremDep)>>6);
        break;
      }
      ch.chFinalVol=volrange(ch.chFinalVol);
      if (!tick0)
        ch.chTremPos+=ch.chTremRate;
      break;
    case xmpCmdVolSlide:
      if (tick0)
        break;
      ch.chFinalVol=ch.chVol=volrange(ch.chVol+((ch.chVolSlideVal&0xF0)?(ch.chVolSlideVal>>4):-(ch.chVolSlideVal&0xF)));
      break;
    case xmpCmdGVolSlide:
      if (tick0)
        break;
      if (ch.chGVolSlideVal&0xF0)
        globalvol=volrange(globalvol+(ch.chGVolSlideVal>>4));
      else
        globalvol=volrange(globalvol-(ch.chGVolSlideVal&0xF));
      putque(queGVol, -1, globalvol);
      break;
    case xmpCmdKeyOff:
      if (tick0)
        break;
      if (curtick==ch.chActionTick)
      {
        ch.chSustain=0;
        if (ch.cursamp&&(ch.cursamp->volenv>=nenv))
          ch.chFadeVol=0;
      }
      break;
    case xmpCmdPanSlide:
      if (tick0)
        break;
      ch.chFinalPan=ch.chPan=panrange(ch.chPan+((ch.chPanSlideVal&0xF0)?(ch.chPanSlideVal>>4):-(ch.chPanSlideVal&0xF)));
      break;
    case xmpCmdMRetrigger:
      if (ch.chMRetrigPos++!=ch.chMRetrigLen)
        break;
      ch.chMRetrigPos=0;
      ch.nextpos=0;

      switch (ch.chMRetrigAct)
      {
      case 0: case 8: break;
      case 1: case 2: case 3: case 4: case 5:
        ch.chVol=ch.chVol-(1<<(ch.chMRetrigAct-1));
        break;
      case 9: case 10: case 11: case 12: case 13:
        ch.chVol=ch.chVol+(1<<(ch.chMRetrigAct-9));
        break;
      case 6:  ch.chVol=(ch.chVol*5)>>3; break;
      case 14: ch.chVol=(ch.chVol*3)>>1; break;
      case 7:  ch.chVol>>=1; break;
      case 15: ch.chVol<<=1; break;
      }
      ch.chFinalVol=ch.chVol=volrange(ch.chVol);
      break;
    case xmpCmdTremor:
      if (ch.chTremorPos>=ch.chTremorOff)
        ch.chFinalVol=0;
      if (tick0)
        break;
      ch.chTremorPos++;
      if (ch.chTremorPos==ch.chTremorLen)
        ch.chTremorPos=0;
      break;
    case xmpCmdRetrigger:
      if (!ch.chActionTick)
        break;
      if (!(curtick%ch.chActionTick))
        ch.nextpos=0;
      break;
    case xmpCmdNoteCut:
      if (tick0)
        break;
      if (curtick==ch.chActionTick)
        ch.chFinalVol=ch.chVol=0;
      break;
    case xmpCmdDelayNote:
      if (tick0)
        break;
      if (curtick!=ch.chActionTick)
        break;
      procnot=ch.chDelayNote;
      procins=ch.chCurIns;
      proccmd=0;
      procdat=0;
      procvol=0;
      PlayNote(ch);
      switch (ch.chDelayVol>>4)
      {
      case xmpVCmdVol0x: case xmpVCmdVol1x: case xmpVCmdVol2x: case xmpVCmdVol3x:
        ch.chFinalVol=ch.chVol=ch.chDelayVol-0x10;
        break;
      case xmpVCmdVol40:
        ch.chFinalVol=ch.chVol=0x40;
        break;
      case xmpVCmdPanning:
        ch.chFinalPan=ch.chPan=(ch.chDelayVol&0xF)*0x11;
        break;
      }
      break;
    }

    if (!ch.cursamp)
    {
      mcpSet(i, mcpCStatus, 0);
      continue;
    }
    gmdsample &sm=*ch.cursamp;

    int vol=(ch.chFinalVol*globalvol)>>4;
    int pan=ch.chFinalPan-128;
    if (!ch.chSustain)
    {
      vol=(vol*ch.chFadeVol)>>15;
      if (ch.chFadeVol>=sm.volfade)
        ch.chFadeVol-=sm.volfade;
      else
        ch.chFadeVol=0;
    }

    if (sm.volenv<nenv)
    {
      const gmdenvelope &env=envelopes[sm.volenv];
      vol=(env.env[ch.chVolEnvPos]*vol)>>8;

      if (ch.chVolEnvPos<env.len)
        ch.chVolEnvPos++;
      if (ch.chSustain&&(env.type&mpEnvSLoop))
      {
        if (ch.chVolEnvPos==env.sloope)
          ch.chVolEnvPos=env.sloops;
      }
      else
        if (env.type&mpEnvLoop)
        {
          if (ch.chVolEnvPos==env.loope)
            ch.chVolEnvPos=env.loops;
        }
    }

    if (sm.panenv<nenv)
    {
      const gmdenvelope &env=envelopes[sm.panenv];
      pan+=((env.env[ch.chPanEnvPos]-128)*(128-((pan<0)?-pan:pan)))>>7;

      if (ch.chPanEnvPos<env.len)
        ch.chPanEnvPos++;
      if (ch.chSustain&&(env.type&mpEnvSLoop))
      {
        if (ch.chPanEnvPos==env.sloope)
          ch.chPanEnvPos=env.sloops;
      }
      else
      if (env.type&mpEnvLoop)
      {
        if (ch.chPanEnvPos==env.loope)
          ch.chPanEnvPos=env.loops;
      }
    }

    if (sm.vibrate&&sm.vibdepth)
    {
      int dep=0;
      switch (sm.vibtype)
      {
      case 0:
        dep=(sintab[ch.chAVibPos>>8]*sm.vibdepth)>>11;
        break;
      case 1:
        dep=(ch.chAVibPos&0x8000)?-sm.vibdepth:sm.vibdepth;
        break;
      case 2:
        dep=(sm.vibdepth*(32768-ch.chAVibPos))>>14;
        break;
      case 3:
        dep=(sm.vibdepth*(ch.chAVibPos-32768))>>14;
        break;
      }

      ch.chAVibSwpPos+=sm.vibsweep;
      if (ch.chAVibSwpPos>0x10000)
        ch.chAVibSwpPos=0x10000;
      dep=(dep*(int)ch.chAVibSwpPos)>>16;

      ch.chFinalPitch-=dep;

      ch.chAVibPos+=sm.vibrate;
    }

    if (ch.nextstop)
      mcpSet(i, mcpCStatus, 0);
    if (ch.nextsamp!=-1)
      mcpSet(i, mcpCInstrument, ch.nextsamp);
    if (ch.nextpos!=-1)
    {
      mcpSet(i, mcpCPosition, ch.nextpos);
      mcpSet(i, mcpCLoop, 1);
      mcpSet(i, mcpCDirect, 0);
      mcpSet(i, mcpCStatus, 1);
    }
    if (linearfreq)
      mcpSet(i, mcpCPitch, -ch.chFinalPitch);
    else
      mcpSet(i, mcpCPitch6848, ch.chFinalPitch);
    mcpSet(i, mcpCVolume, (looping||!looped)?vol:0);
    mcpSet(i, mcpCPanning, pan);
    mcpSet(i, mcpCMute, mutech[i]);
  }
  putque(quePos, -1, curtick|(curord<<16)|(currow<<8));
}


int xmpGetRealPos()
{
  ReadQue();
  return realpos;
}

int xmpChanActive(int ch)
{
  return mcpGet(ch, mcpCStatus)&&channels[ch].cursamp&&channels[ch].chVol&&channels[ch].chFadeVol;
}

int xmpGetChanIns(int ch)
{
  return channels[ch].chCurIns;
}

int xmpGetChanSamp(int ch)
{
  if (!channels[ch].cursamp)
    return 0xFFFF;
  return channels[ch].cursamp-samples;
}

int xmpGetDotsData(int ch, int &smp, int &frq, int &voll, int &volr, int &sus)
{
  if (!mcpGet(ch, mcpCStatus))
    return 0;
  channel &c=channels[ch];
  if (!c.cursamp||!c.chVol||!c.chFadeVol)
    return 0;
  smp=c.cursamp-samples;
  if (linearfreq)
    frq=60*256+c.cursamp->normnote-freqrange(c.chFinalPitch);
  else
    frq=60*256+c.cursamp->normnote+mcpGetNote8363(6848*8363/freqrange(c.chFinalPitch));
  mcpGetRealVolume(ch, voll, volr);
  sus=c.chSustain;
  return 1;
}

void xmpGetRealVolume(int ch, int &voll, int &volr)
{
  mcpGetRealVolume(ch, voll, volr);
}

unsigned short xmpGetPos()
{
  return (curord<<8)|currow;
}

void xmpSetPos(int ord, int row)
{
  if (row<0)
    ord--;
  if (ord>=nord)
    ord=0;
  if (ord<0)
  {
    ord=0;
    row=0;
  }
  if (row>=patlens[orders[ord]])
  {
    ord++;
    row=0;
  }
  if (ord>=nord)
    ord=0;
  if (row<0)
  {
    row+=patlens[orders[ord]];
    if (row<0)
      row=0;
  }
  int i;
  for (i=0; i<nchan; i++)
    mcpSet(i, mcpCReset, 0);
  jumptoord=ord;
  jumptorow=row;
  curtick=curtempo;
  curord=ord;
  currow=row;
  usersetpos=1;
  querpos=0;
  quewpos=0;
  realpos=(curord<<16)|(currow<<8);
}

int xmpGetLChanSample(int ch, short *b, int len, int rate, int opt)
{
  return mcpGetChanSample(ch, b, len, rate, opt);
}

void xmpMute(int i, int m)
{
  mutech[i]=m;
}

int xmpLoop()
{
  return looped;
}

void xmpSetLoop(int x)
{
  looping=x;
}

int xmpLoadSamples(xmodule &m)
{
  return mcpLoadSamples(m.sampleinfos, m.nsampi);
}

int xmpPlayModule(xmodule &m)
{
  int i;

  memset(channels, 0, sizeof(channels));

  looping=1;
  globalvol=0x40;
  realgvol=0x40;
  jumptorow=0;
  jumptoord=0;
  curord=0;
  currow=0;
  realpos=0;
  ninst=m.ninst;
  nord=m.nord;
  nsamp=m.nsamp;
  instruments=m.instruments;
  envelopes=m.envelopes;
  samples=m.samples;
  patterns=m.patterns;
  orders=m.orders;
  patlens=m.patlens;
  linearfreq=m.linearfreq;
  nchan=m.nchan;
  loopord=m.loopord;
  nenv=m.nenv;
  looped=0;

  curtempo=m.initempo;
  curtick=m.initempo-1;

  for (i=0; i<nchan; i++)
  {
    channels[i].chPan=((i*3)&2)?0xFF:0x00;
    mutech[i]=0;
  }

  quelen=100;
  que=new int [quelen][4];
  if (!que)
    return 0;
  querpos=0;
  quewpos=0;

  curbpm=m.inibpm;
  realtempo=m.inibpm;
  realspeed=m.initempo;
  firstspeed=256*2*curbpm/5;
  if (!mcpOpenPlayer(nchan, xmpPlayTick))
    return 0;

  if (nchan!=mcpNChan)
  {
    mcpClosePlayer();
    return 0;
  }

  return 1;
}

void xmpStopModule()
{
  mcpClosePlayer();
  delete que;
}

void xmpGetGlobInfo(int &tmp, int &bpm, int &gvol)
{
  tmp=realspeed;
  bpm=realtempo;
  gvol=realgvol;
}

int xmpGetTime()
{
  return mcpGet(-1, mcpGTimer);
}

int xmpGetSync(int ch, int &time)
{
  ReadQue();
  if ((ch<0)||(ch>=nchan))
  {
    time=xmpGetTime()-realsynctime;
    return realsync;
  }
  else
  {
    time=xmpGetTime()-channels[ch].chSyncTime;
    return channels[ch].chSync;
  }
}

int xmpGetTickTime()
{
  ReadQue();
  return 65536*5/(2*realtempo);
}

int xmpGetRowTime()
{
  ReadQue();
  return 65536*5*realspeed/(2*realtempo);
}

void xmpSetEvPos(int ch, int pos, int modtype, int mod)
{
  if ((ch<0)||(ch>=nchan))
    return;
  channel &c=channels[ch];
  c.evpos0=pos;
  c.evmodtype=modtype;
  c.evmod=mod;
  c.evmodpos=0;
  c.evpos=-1;
  c.evtime=-1;
}

int xmpGetEvPos(int ch, int &time)
{
  ReadQue();
  if ((ch<0)||(ch>=nchan))
  {
    time=-1;
    return -1;
  }
  time=xmpGetTime()-channels[ch].evtime;
  return channels[ch].evpos;
}

int xmpFindEvPos(int pos, int &time)
{
  ReadQue();
  int i;
  for (i=0; i<nchan; i++)
    if (channels[i].evpos==pos)
      break;
  time=xmpGetTime()-channels[i].evtime;
  return channels[i].evpos;
}
