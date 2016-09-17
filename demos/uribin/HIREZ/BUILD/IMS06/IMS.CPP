// (c) 1994-1997 Niklas Beisert, this file is part of "Indoor Music System".
// you must not read, modify, print, compile or copy this file or parts of
// it unless you have accepted the license in the accompanying file IMS.TXT.

//do not uncomment unless you have accepted the license at the bottom of devppas.cpp
//#define USEPAS

#include <stdlib.h>
#include "imssetup.h"
#include "imsdev.h"
#include "mcp.h"
#include "player.h"
#include "ims.h"

extern "C" { extern sounddevice mcpMixer; };
extern "C" { extern sounddevice mcpUltraSound; };
extern "C" { extern sounddevice mcpDoubleGUS; };
extern "C" { extern sounddevice mcpInterWave; };
extern "C" { extern sounddevice mcpSoundBlaster32; };
extern "C" { extern sounddevice mcpNone; };
extern "C" { extern sounddevice plrSoundBlaster; };
extern "C" { extern sounddevice plrWinSoundSys; };
extern "C" { extern sounddevice plrUltraSound; };
extern "C" { extern sounddevice plrProAudioSpectrum; };
extern "C" { extern sounddevice plrESSAudioDrive; };

static int sb32ports[]={0x620,0x640,0x660,0x680};
static int stdports[]={0x220,0x240,0x260,0x280};
static int gmxports[]={0x32C,0x34C,0x36C,0x38C};
static int pasports[]={0x288,0x384,0x388,0x38C};
static int wssports[]={0x530,0x604,0xE80,0xF40};
static int sb1ports[]={0x210,0x220,0x230,0x240,0x250,0x260,0x280};
static int sb2ports[]={0x220,0x240,0x250,0x260};
static int lowdmas[]={0,1,3};
static int hghdmas[]={5,6,7};
static int alldmas[]={0,1,3,5,6,7};
static int sbirqs[]={2,3,5,7,10};
static int sb16irqs[]={2,5,7,10};
static int gusirqs[]={2,3,5,7,11,12,15};
static int sbrates[]={11111,15151,22222,33333,43478};
static int sbprates[]={11111,15151,21739};
static int sb16rates[]={11025,16537,22050,33075,44100};
static int wssrates[]={11025,16000,22050,32000,44100,48000};
static int pasrates[]={11025,16537,22050,33075,44100};
static int gusrates[]={19293,22050,32494,44100};
static int essrates[]={11025,16537,22050,33075,44100};
static int essirqs[]={5,7,9,10};
static int essports[]={0x220,0x230,0x240,0x250};

static const int ndevs=
#ifdef USEPAS
15
#else
14
#endif
;
static imssetupdevicepropstruct devprops[]=
{
  {"quiet",                     0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
  {"AMD InterWave",             4,0,0,0,0,0,0,0,0,0,2,1,0,0,stdports,0,0,0,0,0,0},
  {"Gravis UltraSound",         4,0,0,0,0,0,0,0,0,0,2,0,0,0,stdports,0,0,0,0,0,0},
  {"GUS Software Mixing",       4,0,6,0,7,0,4,1,1,1,1,0,0,1,stdports,0,alldmas,0,gusirqs,0,gusrates},
  {"GUS MAX",                   4,4,6,6,0,0,6,1,1,1,1,0,0,1,gmxports,stdports,alldmas,alldmas,0,0,wssrates},
  {"GUS DaughterBoard",         4,0,3,0,0,0,6,1,1,1,1,0,0,1,wssports,0,lowdmas,0,0,0,wssrates},
  {"Double GUS 2D",             4,4,0,0,0,0,0,0,0,0,2,0,0,1,stdports,stdports,0,0,0,0,0},
  {"SoundBlaster 32",           4,0,0,0,0,0,0,0,0,0,2,1,1,0,sb32ports,0,0,0,0,0,0},
  {"SoundBlaster 16",           4,0,3,3,4,0,5,1,1,1,1,0,0,1,stdports,0,lowdmas,hghdmas,sb16irqs,0,sb16rates},
  {"SoundBlaster Pro stereo",   4,0,3,0,5,0,3,1,2,0,1,0,0,1,stdports,0,lowdmas,0,sbirqs,0,sbprates},
  {"SoundBlaster 2.x/Pro mono", 4,0,3,0,5,0,5,1,0,0,1,0,0,1,sb2ports,0,lowdmas,0,sbirqs,0,sbrates},
  {"SoundBlaster 1.x",          7,0,3,0,5,0,3,1,0,0,1,0,0,1,sb1ports,0,lowdmas,0,sbirqs,0,sbrates},
  {"Windows Sound System",      4,0,3,0,0,0,6,1,1,1,1,0,0,1,wssports,0,lowdmas,0,0,0,wssrates},
  {"ESS Audio Drive 688",       4,0,3,0,4,0,5,1,1,1,1,0,0,1,essports,0,lowdmas,0,essirqs,0,essrates},
  {"Pro Audio Spectrum",        4,0,3,0,0,0,5,1,1,1,1,0,0,1,pasports,0,lowdmas,0,0,0,pasrates},
};
static sounddevice *snddevs[]=
{
  &mcpNone, &mcpInterWave, &mcpUltraSound, &plrUltraSound, &plrWinSoundSys,
  &plrWinSoundSys, &mcpDoubleGUS, &mcpSoundBlaster32, &plrSoundBlaster,
  &plrSoundBlaster, &plrSoundBlaster, &plrSoundBlaster, &plrWinSoundSys,
  &plrESSAudioDrive
#ifdef USEPAS
  , &plrProAudioSpectrum
#endif
};
static int subdevs[]={-1,-1,-1,-1,2,1,0,-1,4,3,2,1,0,-1,-1};
static int devopts[]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
static int detectorder[]={6,0,11,10,9,13,
#ifdef USEPAS
14,
#endif
12,5,3,4,8,7,2,1};

static deviceinfo curdev;
static deviceinfo mixdev;

int imsInit(imsinitstruct &is)
{
  deviceinfo devs[ndevs];

  int i;
  for (i=0; i<ndevs; i++)
  {
    devs[i].port=-1;
    devs[i].port2=-1;
    devs[i].dma=-1;
    devs[i].dma2=-1;
    devs[i].irq=-1;
    devs[i].irq2=-1;
    devs[i].dev=snddevs[i];
    devs[i].opt=devopts[i];
    devs[i].subtype=subdevs[i];
  }

  int devno=0;
  if (!is.usequiet)
    for (i=0; i<ndevs; i++)
      if (devs[detectorder[i]].dev->Detect(devs[detectorder[i]]))
        devno=detectorder[i];

  for (i=0; i<ndevs; i++)
  {
    devprops[i].port=devs[i].port;
    devprops[i].port2=devs[i].port2;
    devprops[i].dma=devs[i].dma;
    devprops[i].dma2=devs[i].dma2;
    devprops[i].irq=devs[i].irq;
    devprops[i].irq2=devs[i].irq2;
  }
  imssetupresultstruct res;
  res.device=devno;
  res.rate=is.rate;
  res.stereo=is.stereo;
  res.bit16=is.bit16;
  res.intrplt=is.interpolate;
  res.amplify=is.amplify;
  res.panning=is.panning;
  res.reverb=is.reverb;
  res.chorus=is.chorus;
  res.surround=is.surround;

  if (!is.usequiet&&(is.usersetup||getenv("IMSSETUP")))
    if (!imsSetup(res, devprops, ndevs))
      return 0;

  curdev=devs[res.device];
  curdev.port=devprops[res.device].port;
  curdev.port2=devprops[res.device].port2;
  curdev.dma=devprops[res.device].dma;
  curdev.dma2=devprops[res.device].dma2;
  curdev.irq=devprops[res.device].irq;
  curdev.irq2=devprops[res.device].irq2;
  mixdev.dev=0;
  if (devprops[res.device].mixer)
  {
    mixdev.opt=0;
    mixdev.subtype=0;
    mcpMixer.Detect(mixdev);
  }

  if (!curdev.dev->Init(curdev))
    return 0;
  if (mixdev.dev)
  {
    if (!mixdev.dev->Init(mixdev))
    {
      curdev.dev->Close();
      return 0;
    }
    mcpMixMaxRate=res.rate;
    mcpMixProcRate=res.rate*64;
    mcpMixOpt=(res.bit16?PLR_16BIT:0)|(res.stereo?PLR_STEREO:0);
    mcpMixBufSize=mcpMixMax=is.bufsize;
    mcpMixPoll=is.bufsize-is.pollmin;
  }
  mcpSet(-1, mcpMasterAmplify, res.amplify*16384/100);
  mcpSet(-1, mcpMasterPanning, res.panning*64/100);
  mcpSet(-1, mcpMasterReverb, res.reverb*64/100);
  mcpSet(-1, mcpMasterChorus, res.chorus*64/100);
  mcpSet(-1, mcpMasterFilter, res.intrplt?1:0);
  mcpSet(-1, mcpMasterSurround, res.surround?1:0);

  return 1;
}

void imsClose()
{
  if (mixdev.dev)
    mixdev.dev->Close();
  curdev.dev->Close();
}

void imsFillDefaults(imsinitstruct &is)
{
  is.bufsize=8192;
  is.pollmin=0;
  is.usequiet=0;
  is.usersetup=1;
  is.rate=44100;
  is.stereo=1;
  is.bit16=1;
  is.interpolate=1;
  is.amplify=100;
  is.panning=100;
  is.reverb=0;
  is.chorus=0;
  is.surround=0;
}
