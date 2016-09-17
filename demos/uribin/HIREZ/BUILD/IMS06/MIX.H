#ifndef __MIX_H
#define __MIX_H

struct mixchannel
{
  void *samp;
  long length;
  long loopstart;
  long loopend;
  long replen;
  long step;
  long pos;
  unsigned short fpos;
  unsigned short status;
  union
  {
    void *voltabs[2];
    short vols[2];
  };
};

int mixInit(void (*getchan)(int ch, mixchannel &chn, int rate), int resamp, int chan, int amp);
void mixClose();
void mixSetAmplify(int amp);
void mixGetRealVolume(int ch, int &l, int &r);
void mixGetMasterSample(short *s, int len, int rate, int opt);
int mixGetChanSample(int ch, short *s, int len, int rate);
int mixAddChanSample(int ch, short *s, int len, int rate);
void mixGetRealMasterVolume(int &l, int &r);

#define MIX_PLAYING 1
#define MIX_MUTE 2
#define MIX_LOOPED 4
#define MIX_PINGPONGLOOP 8
#define MIX_PLAY16BIT 16
#define MIX_INTERPOLATE 32
#define MIX_MAX 64

#endif
