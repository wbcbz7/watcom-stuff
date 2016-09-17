#ifndef __GMDINST_H
#define __GMDINST_H

enum
{
  mpEnvLoop=1, mpEnvBiDi=2, mpEnvSLoop=4, mpEnvSBiDi=8,
};

struct gmdenvelope
{
  unsigned char *env;
  unsigned short len;
  unsigned short loops, loope;
  unsigned short sloops, sloope;
  unsigned char type;
  unsigned char speed;
};

struct gmdsample
{
  char name[32];
  unsigned short handle;
  signed short normnote;
  signed short stdvol;
  signed short stdpan;
  unsigned short opt;
#define MP_OFFSETDIV2 1
  unsigned short volfade;
  unsigned char pchint;
  unsigned short volenv;
  unsigned short panenv;
  unsigned short pchenv;
  unsigned char vibspeed;
  unsigned char vibtype;
  unsigned short vibrate;
  unsigned short vibdepth;
  unsigned short vibsweep;
};

struct gmdinstrument
{
  char name[32];
  unsigned short samples[128];
};

struct sampleinfo;

void gmdInstSetup(const gmdinstrument *ins, int nins, const gmdsample *smp, int nsmp, const sampleinfo *smpi, int nsmpi, int type, void (*MarkyBoy)(char *, char *));
void gmdInstClear();

#endif
