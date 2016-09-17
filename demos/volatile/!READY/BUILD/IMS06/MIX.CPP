#include <string.h>
#include <dos.h>
#include <conio.h>
#include <stdlib.h>
#include "mcp.h"
#include "mix.h"

void mixGetMasterSample(short *s, int len, int rate, int opt)
{
}

int mixMixChanSamples(int *ch, int n, short *s, int len, int rate, int opt)
{
  return 0;
}

int mixGetChanSample(int ch, short *s, int len, int rate, int opt)
{
  return 0;
}

void mixGetRealVolume(int ch, int &l, int &r)
{
}

void mixGetRealMasterVolume(int &l, int &r)
{
}

void mixSetAmplify(int amp)
{
}

int mixInit(void (*getchan)(int ch, mixchannel &chn, int rate), int masterchan, int chn, int amp)
{
  mcpGetRealVolume=mixGetRealVolume;
  mcpGetChanSample=mixGetChanSample;
  mcpMixChanSamples=mixMixChanSamples;
  if (masterchan)
  {
    mcpGetRealMasterVolume=mixGetRealMasterVolume;
    mcpGetMasterSample=mixGetMasterSample;
  }

  return 1;
}

void mixClose()
{
}
