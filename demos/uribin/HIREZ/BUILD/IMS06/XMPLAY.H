#define USE_STDIO

#ifndef USE_STDIO
class binfile;
#endif
struct gmdenvelope;
struct gmdinstrument;
struct gmdsample;
struct sampleinfo;

struct xmodule
{
  char name[21];
  int linearfreq;
  int nchan;
  int ninst;
  int nenv;
  int npat;
  int nord;
  int nsamp;
  int nsampi;
  int loopord;
  unsigned char initempo;
  unsigned char inibpm;

  gmdenvelope *envelopes;
  gmdsample *samples;
  gmdinstrument *instruments;
  sampleinfo *sampleinfos;
  unsigned short *patlens;
  unsigned char (**patterns)[5];
  unsigned short *orders;
};

enum
{
  xmpFXIns=0,
  xmpFXNote=1,
  xmpFXVol=2,
  xmpFXCmd=3,
  xmpFXData=4
};

int xmpLoadSamples(xmodule &m);


#ifdef USE_STDIO
int xmpLoadMOD(xmodule &m, FILE *file);
int xmpLoadModule(xmodule &m, FILE *file);
#else
int xmpLoadMOD(xmodule &m, binfile &file);
int xmpLoadModule(xmodule &m, binfile &file);
#endif

void xmpFreeModule(xmodule &m);

int xmpPlayModule(xmodule &m);
void xmpStopModule();
void xmpSetPos(int ord, int row);

void xmpGetRealVolume(int i, int &l, int &r);
void xmpMute(int i, int m);
int xmpGetLChanSample(int ch, short *b, int len, int rate, int opt);
unsigned short xmpGetPos();
int xmpGetRealPos();
int xmpGetDotsData(int ch, int &smp, int &frq, int &l, int &r, int &sus);
int xmpPrecalcTime(xmodule &m, int startpos, int (*calc)[2], int n, int ite);
int xmpLoop();
void xmpSetLoop(int);
int xmpGetChanIns(int);
int xmpGetChanSamp(int);
int xmpChanActive(int);
void xmpGetGlobInfo(int &tmp, int &bpm, int &gvol);
void xmpOptimizePatLens(xmodule &m);
int xmpGetSync(int ch, int &time);
int xmpGetTime();

int xmpGetTickTime();
int xmpGetRowTime();
void xmpSetEvPos(int ch, int pos, int modtype, int mod);
int xmpGetEvPos(int ch, int &time);
int xmpFindEvPos(int pos, int &time);

enum
{
  xmpCmdArpeggio=0,xmpCmdPortaU=1,xmpCmdPortaD=2,xmpCmdPortaNote=3,
  xmpCmdVibrato=4,xmpCmdPortaVol=5,xmpCmdVibVol=6,xmpCmdTremolo=7,
  xmpCmdPanning=8,xmpCmdOffset=9,xmpCmdVolSlide=10,xmpCmdJump=11,
  xmpCmdVolume=12,xmpCmdBreak=13,xmpCmdSpeed=15,xmpCmdGVolume=16,
  xmpCmdGVolSlide=17,xmpCmdKeyOff=20,xmpCmdEnvPos=21,xmpCmdPanSlide=25,
  xmpCmdMRetrigger=27,xmpCmdTremor=29,xmpCmdXPorta=33,xmpCmdFPortaU=37,
  xmpCmdFPortaD=38,xmpCmdGlissando=39,xmpCmdVibType=40,xmpCmdPatLoop=42,
  xmpCmdTremType=43,xmpCmdSPanning=44,xmpCmdRetrigger=45,xmpCmdFVolSlideU=46,
  xmpCmdFVolSlideD=47,xmpCmdNoteCut=48,xmpCmdDelayNote=49,xmpCmdPatDelay=50,
  xmpCmdSync1=28,xmpCmdSync2=32,xmpCmdSync3=51,
  xmpVCmdVol0x=1,xmpVCmdVol1x=2,xmpVCmdVol2x=3,xmpVCmdVol3x=4,xmpVCmdVol40=5,
  xmpVCmdVolSlideD=6,xmpVCmdVolSlideU=7,xmpVCmdFVolSlideD=8,
  xmpVCmdFVolSlideU=9,xmpVCmdVibRate=10,xmpVCmdVibDep=11,xmpVCmdPanning=12,
  xmpVCmdPanSlideL=13,xmpVCmdPanSlideR=14,xmpVCmdPortaNote=15,
};
