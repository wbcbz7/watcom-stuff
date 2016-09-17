// (c) 1994-1997 Niklas Beisert, this file is part of "Indoor Music System".
// you must not read, modify, print, compile or copy this file or parts of
// it unless you have accepted the license in the accompanying file IMS.TXT.

//#include <string.h>
#include <stdlib.h>
#include "mcp.h"

#define SAMPEND 8

static unsigned short abstab[0x200];

static int sampsizefac(int type)
{
  return ((type&mcpSamp16Bit)?1:0)+((type&mcpSampStereo)?1:0);
}

static int stereosizefac(int type)
{
  return (type&mcpSampStereo)?1:0;
}

static void sampto8(sampleinfo &s)
{
  s.type&=~mcpSamp16Bit;
  s.type|=mcpSampRedBits;
  int l=(s.length+SAMPEND)<<sampsizefac(s.type);
  int i;
  for (i=0; i<l; i++)
    ((char *)s.ptr)[i]=((char *)s.ptr)[2*i+1];
  s.ptr=realloc(s.ptr,(s.length+SAMPEND)<<sampsizefac(s.type));
}

static void samptomono(sampleinfo &s)
{
  s.type&=~mcpSampStereo;
  s.type|=mcpSampRedStereo;
  int i;
  if (s.type&mcpSamp16Bit)
    for (i=0; i<(s.length+SAMPEND); i++)
      ((short *)s.ptr)[i]=(((short *)s.ptr)[2*i]+((short *)s.ptr)[2*i+1])>>1;
  else
    for (i=0; i<(s.length+SAMPEND); i++)
      ((signed char *)s.ptr)[i]=(((signed char *)s.ptr)[2*i]+((signed char *)s.ptr)[2*i+1])>>1;
  s.ptr=realloc(s.ptr,(s.length+SAMPEND)<<sampsizefac(s.type));
}

static void repairloop(sampleinfo &s)
{
  if (s.type&mcpSampLoop)
  {
    if (s.loopend<=s.loopstart)
      s.type&=~mcpSampLoop;
    if (s.loopstart<0)
      s.loopstart=0;
    if (s.loopend>s.length)
      s.loopend=s.length;
    if (s.loopend==s.loopstart)
      s.type&=~mcpSampLoop;
  }
  if (s.type&mcpSampSLoop)
  {
    if (s.sloopend<=s.sloopstart)
      s.type&=~mcpSampSLoop;
    if (s.sloopstart<0)
      s.sloopstart=0;
    if (s.sloopend>s.length)
      s.sloopend=s.length;
    if (s.sloopend==s.sloopstart)
      s.type&=~mcpSampSLoop;
  }
  if ((s.type&mcpSampLoop)&&(s.type&mcpSampSLoop)&&((!(s.type&mcpSampBiDi))==(!(s.type&mcpSampSBiDi)))&&(s.loopstart==s.sloopstart)&&(s.loopend==s.sloopend))
    s.type&=~mcpSampSLoop;
}

static int expandsmp(sampleinfo &s, int nopingpongloops)
{
  int newlen=s.length;
  int replen=s.loopend-s.loopstart;
  int sreplen=s.sloopend-s.sloopstart;
  char restricted=0;
  char toforward=0;
  char stoforward=0;
  int expandloop=0;
  int sexpandloop=0;
  int c;

  if ((s.type&mcpSampLoop)&&(s.type&mcpSampSLoop)&&(s.loopend>s.sloopstart)&&(s.sloopend>s.loopstart))
    restricted=1;

  if ((s.type&mcpSampLoop)&&(s.type&mcpSampBiDi)&&nopingpongloops&&!restricted)
  {
    toforward=1;
    expandloop=replen*toforward;
    replen+=replen*toforward;
  }
  if ((s.type&mcpSampLoop)&&(replen<256)&&!restricted)
  {
    int ln=255/replen;
    if ((s.type&mcpSampBiDi)&&!toforward)
      ln=(ln+1)&~1;
    expandloop+=ln*replen;
  }

  if ((s.type&mcpSampSLoop)&&(s.type&mcpSampSBiDi)&&nopingpongloops&&!restricted)
  {
    stoforward=1;
    sexpandloop=sreplen*stoforward;
    sreplen+=sreplen*stoforward;
  }
  if ((s.type&mcpSampSLoop)&&(sreplen<256)&&!restricted)
  {
    int ln=sexpandloop=255/sreplen;
    if ((s.type&mcpSampSBiDi)&&!stoforward)
      ln=(ln+1)&~1;
    sexpandloop+=ln*sreplen;
  }


  replen=s.loopend-s.loopstart;
  sreplen=s.sloopend-s.sloopstart;

  newlen+=expandloop+sexpandloop;
  if (newlen<2)
    newlen=2;

  s.ptr=realloc(s.ptr, (newlen+SAMPEND)<<sampsizefac(s.type));
  if (!s.ptr)
    return 0;
  if (expandloop)
  {
    if (sampsizefac(s.type)==2)
    {
      long *p=(long *)s.ptr+s.loopend;
      for (c=s.length-s.loopend-1; c>=0; c--)
        p[c+expandloop]=p[c];
      if (!(s.type&mcpSampBiDi))
        for (c=0; c<replen; c++)
          p[c]=p[c-replen];
      else
        for (c=0; c<replen; c++)
          p[c]=p[-1-c];
      for (c=replen; c<expandloop; c++)
        p[c]=p[c-(replen<<1)];
    }
    else
    if (sampsizefac(s.type)==1)
    {
      short *p=(short *)s.ptr+s.loopend;
      for (c=s.length-s.loopend-1; c>=0; c--)
        p[c+expandloop]=p[c];
      if (!(s.type&mcpSampBiDi))
        for (c=0; c<replen; c++)
          p[c]=p[c-replen];
      else
        for (c=0; c<replen; c++)
          p[c]=p[-1-c];
      for (c=replen; c<expandloop; c++)
        p[c]=p[c-(replen<<1)];
    }
    else
    {
      char *p=(char *)s.ptr+s.loopend;
      for (c=s.length-s.loopend-1; c>=0; c--)
        p[c+expandloop]=p[c];
      if (!(s.type&mcpSampBiDi))
        for (c=0; c<replen; c++)
          p[c]=p[c-replen];
      else
        for (c=0; c<replen; c++)
          p[c]=p[-1-c];
      for (c=replen; c<expandloop; c++)
        p[c]=p[c-(replen<<1)];
    }

    if (s.sloopstart>=s.loopend)
      s.sloopstart+=expandloop;
    if (s.sloopend>=s.loopend)
      s.sloopend+=expandloop;
    s.length+=expandloop;
    s.loopend+=expandloop;
    if (toforward)
      s.type&=~mcpSampBiDi;
    if (toforward==2)
      s.loopstart+=replen;
  }

  if (sexpandloop)
  {
    if (sampsizefac(s.type)==2)
    {
      long *p=(long *)s.ptr+s.sloopend;
      for (c=0; c<(s.length-s.sloopend); c++)
        p[c+sexpandloop]=p[c];
      if (!(s.type&mcpSampSBiDi))
        for (c=0; c<sreplen; c++)
          p[c]=p[c-sreplen];
      else
        for (c=0; c<sreplen; c++)
          p[c]=p[-1-c];
      for (c=sreplen; c<sexpandloop; c++)
        p[c]=p[c-(sreplen<<1)];
    }
    else
    if (sampsizefac(s.type)==1)
    {
      short *p=(short *)s.ptr+s.sloopend;
      for (c=0; c<(s.length-s.sloopend); c++)
        p[c+sexpandloop]=p[c];
      if (!(s.type&mcpSampSBiDi))
        for (c=0; c<sreplen; c++)
          p[c]=p[c-sreplen];
      else
        for (c=0; c<sreplen; c++)
          p[c]=p[-1-c];
      for (c=sreplen; c<sexpandloop; c++)
        p[c]=p[c-(sreplen<<1)];
    }
    else
    {
      char *p=(char *)s.ptr+s.sloopend;
      for (c=0; c<(s.length-s.sloopend); c++)
        p[c+sexpandloop]=p[c];
      if (!(s.type&mcpSampSBiDi))
        for (c=0; c<sreplen; c++)
          p[c]=p[c-sreplen];
      else
        for (c=0; c<sreplen; c++)
          p[c]=p[-1-c];
      for (c=sreplen; c<sexpandloop; c++)
        p[c]=p[c-(sreplen<<1)];
    }

    if (s.loopstart>=s.sloopend)
      s.loopstart+=sexpandloop;
    if (s.loopend>=s.sloopend)
      s.loopend+=sexpandloop;
    s.length+=sexpandloop;
    s.sloopend+=sexpandloop;
    if (stoforward)
      s.type&=~mcpSampSBiDi;
    if (stoforward==2)
      s.sloopstart+=sreplen;
  }

  if (s.length<2)
  {
    if (!s.length)
      if (sampsizefac(s.type)==2)
        *(long*)s.ptr=0;
      else
      if (sampsizefac(s.type)==1)
        *(short*)s.ptr=0;
      else
        *(char*)s.ptr=0;
    if (sampsizefac(s.type)==2)
      ((long*)s.ptr)[1]=*(long*)s.ptr;
    else
    if (sampsizefac(s.type)==1)
      ((short*)s.ptr)[1]=*(short*)s.ptr;
    else
      ((char*)s.ptr)[1]=*(char*)s.ptr;
    s.length=2;
  }

  return 1;
}

static int repairsmp(sampleinfo &s)
{
  int i;
  s.ptr=realloc(s.ptr, (s.length+SAMPEND)<<sampsizefac(s.type));
  if (!s.ptr)
    return 0;

  repairloop(s);

/*
  if ((s.type&mcpSampLoop)&&!(s.type&mcpSampSLoop))
  {
    s.type&=~mcpSampSBiDi;
    s.type|=mcpSampSLoop;
    if (s.type&mcpSampBiDi)
      s.type|=mcpSampSBiDi;
    s.sloopstart=s.loopstart;
    s.sloopend=s.loopend;
  }
*/

  if (sampsizefac(s.type)==2)
  {
    long *p=(long*)s.ptr;
    for (i=0; i<SAMPEND; i++)
      p[s.length+i]=p[s.length-1];
    if ((s.type&mcpSampSLoop)&&!(s.type&mcpSampSBiDi))
    {
      p[s.sloopend]=p[s.sloopstart];
      p[s.sloopend+1]=p[s.sloopstart+1];
    }
    if ((s.type&mcpSampLoop)&&!(s.type&mcpSampBiDi))
    {
      p[s.loopend]=p[s.loopstart];
      p[s.loopend+1]=p[s.loopstart+1];
    }
  }
  else
  if (sampsizefac(s.type)==1)
  {
    short *p=(short*)s.ptr;
    for (i=0; i<SAMPEND; i++)
      p[s.length+i]=p[s.length-1];
    if ((s.type&mcpSampSLoop)&&!(s.type&mcpSampSBiDi))
    {
      p[s.sloopend]=p[s.sloopstart];
      p[s.sloopend+1]=p[s.sloopstart+1];
    }
    if ((s.type&mcpSampLoop)&&!(s.type&mcpSampBiDi))
    {
      p[s.loopend]=p[s.loopstart];
      p[s.loopend+1]=p[s.loopstart+1];
    }
  }
  else
  {
    char *p=(char*)s.ptr;
    for (i=0; i<SAMPEND; i++)
      p[s.length+i]=p[s.length-1];
    if ((s.type&mcpSampSLoop)&&!(s.type&mcpSampSBiDi))
    {
      p[s.sloopend]=p[s.sloopstart];
      p[s.sloopend+1]=p[s.sloopstart+1];
    }
    if ((s.type&mcpSampLoop)&&!(s.type&mcpSampBiDi))
    {
      p[s.loopend]=p[s.loopstart];
      p[s.loopend+1]=p[s.loopstart+1];
    }
  }

  return 1;
}

unsigned long getpitch(const void *ptr, unsigned long len);
#pragma aux getpitch parm [esi] [edi] value [edx] modify [eax ebx] = \
  "xor eax,eax" \
  "xor ebx,ebx" \
  "xor edx,edx" \
  "lp:"\
    "mov al,[esi]" \
    "mov ah,[esi+1]" \
    "xor eax,8080h" \
    "sub al,ah" \
    "sbb ah,ah" \
    "inc ah" \
    "mov bx,abstab[eax+eax]" \
    "add edx,ebx" \
    "inc esi" \
  "dec edi" \
  "jnz lp"

unsigned long getpitch16(const void *ptr, unsigned long len);
#pragma aux getpitch16 parm [esi] [edi] value [edx] modify [eax ebx] = \
  "xor eax,eax" \
  "xor ebx,ebx" \
  "xor edx,edx" \
  "lp:"\
    "mov al,[esi+1]" \
    "mov ah,[esi+3]" \
    "xor eax,8080h" \
    "sub al,ah" \
    "sbb ah,ah" \
    "inc ah" \
    "mov bx,abstab[eax+eax]" \
    "add edx,ebx" \
    "add esi,2" \
  "dec edi" \
  "jnz lp"

static void dividefrq(sampleinfo &s)
{
  int i;
  if (sampsizefac(s.type)==2)
    for (i=0; i<(s.length>>1); i++)
      ((long*)s.ptr)[i]=((long*)s.ptr)[2*i];
  else
  if (sampsizefac(s.type)==1)
    for (i=0; i<(s.length>>1); i++)
      ((short*)s.ptr)[i]=((short*)s.ptr)[2*i];
  else
    for (i=0; i<(s.length>>1); i++)
      ((char*)s.ptr)[i]=((char*)s.ptr)[2*i];

  s.length>>=1;
  s.loopstart>>=1;
  s.loopend>>=1;
  s.sloopstart>>=1;
  s.sloopend>>=1;
  s.samprate>>=1;
  s.type|=(s.type&mcpSampRedRate2)?mcpSampRedRate4:mcpSampRedRate2;

  s.ptr=realloc(s.ptr, (s.length+SAMPEND)<<sampsizefac(s.type));
}

static int totalsmpsize(sampleinfo *samples, int nsamp, int always16bit)
{
  int i;
  int curdif=0;
  if (always16bit)
    for (i=0; i<nsamp; i++)
      curdif+=(samples[i].length+SAMPEND)<<stereosizefac(samples[i].type);
  else
    for (i=0; i<nsamp; i++)
      curdif+=(samples[i].length+SAMPEND)<<sampsizefac(samples[i].type);
  return curdif;
}

static int reduce16(sampleinfo *samples, int samplenum, unsigned long *redpars, int memmax)
{
  int i;
  signed long curdif=-memmax;
  signed long totdif=0;
  for (i=0; i<samplenum; i++)
  {
    sampleinfo &s=samples[i];
    if (s.type&mcpSamp16Bit)
      redpars[i]=(s.length+SAMPEND)<<stereosizefac(s.type);
    else
      redpars[i]=0;
    totdif+=redpars[i];
    curdif+=(s.length+SAMPEND)<<sampsizefac(s.type);
  }

  if (curdif>totdif)
  {
    for (i=0; i<samplenum; i++)
      if (samples[i].type&mcpSamp16Bit)
        sampto8(samples[i]);
    return 0;
  }

  while (curdif>0)
  {
    int fit=0;
    int best;
    long bestdif=0;
    for (i=0; i<samplenum; i++)
      if (curdif<=redpars[i])
      {
        if (!fit||(bestdif>redpars[i]))
        {
          fit=1;
          bestdif=redpars[i];
          best=i;
        }
      }
      else
      {
        if (!fit&&(bestdif<redpars[i]))
        {
          bestdif=redpars[i];
          best=i;
        }
      }
    sampto8(samples[best]);
    curdif-=redpars[best];
    redpars[best]=0;
  }
  return 1;
}

static int reducestereo(sampleinfo *samples, int samplenum, unsigned long *redpars, int memmax)
{
  int i;
  signed long curdif=-memmax;
  signed long totdif=0;
  for (i=0; i<samplenum; i++)
  {
    sampleinfo &s=samples[i];
    if (s.type&mcpSampStereo)
      redpars[i]=s.length+SAMPEND;
    else
      redpars[i]=0;
    totdif+=redpars[i];
    curdif+=(s.length+SAMPEND)<<stereosizefac(s.type);
  }

  if (curdif>totdif)
  {
    for (i=0; i<samplenum; i++)
      if (samples[i].type&mcpSampStereo)
        samptomono(samples[i]);
    return 0;
  }

  while (curdif>0)
  {
    int fit=0;
    int best;
    long bestdif=0;
    for (i=0; i<samplenum; i++)
      if (curdif<=redpars[i])
      {
        if (!fit||(bestdif>redpars[i]))
        {
          fit=1;
          bestdif=redpars[i];
          best=i;
        }
      }
      else
      {
        if (!fit&&(bestdif<redpars[i]))
        {
          bestdif=redpars[i];
          best=i;
        }
      }
    samptomono(samples[best]);
    curdif-=redpars[best];
    redpars[best]=0;
  }
  return 1;
}

static int reducefrq(sampleinfo *samples, int samplenum, unsigned long *redpars, int memmax)
{
  int i;

  for (i=-0x100; i<0x100; i++)
    abstab[i+0x100]=i*i/16;

  signed long curdif=-memmax;
  for (i=0; i<samplenum; i++)
  {
    sampleinfo &s=samples[i];
    curdif+=s.length+SAMPEND;
    if (s.length<1024)
      redpars[i]=0xFFFFFFFF;
    else
      if (s.type&mcpSamp16Bit)
        redpars[i]=getpitch16(s.ptr, s.length)/s.length;
      else
        redpars[i]=getpitch(s.ptr, s.length)/s.length;
  }

  while (curdif>0)
  {
    int best=-1;
    unsigned long bestpitch=0xFFFFFFFF;
    for (i=0; i<samplenum; i++)
      if (redpars[i]<bestpitch)
      {
        bestpitch=redpars[i];
        best=i;
      }
    if (best==-1)
      return 0;
    sampleinfo &s=samples[best];
    curdif-=s.length+SAMPEND;
    dividefrq(s);
    curdif+=s.length+SAMPEND;

    if ((s.length<1024)||(s.type&mcpSampRedRate4))
      redpars[best]=0xFFFFFFFF;
    else
      if (s.type&mcpSamp16Bit)
        redpars[best]=(getpitch16(s.ptr, s.length)/s.length)<<1;
      else
        redpars[best]=(getpitch(s.ptr, s.length)/s.length)<<1;
  }

  return 1;
}

static int convertsample(sampleinfo &s)
{
  if (s.loopstart>=s.loopend)
    s.type&=~mcpSampLoop;

  int i;

  if ((s.type&(mcpSampBigEndian|mcpSamp16Bit))==(mcpSampBigEndian|mcpSamp16Bit))
  {
    int l=s.length<<sampsizefac(s.type);
    for (i=0; i<l; i+=2)
    {
      char tmp=((char*)s.ptr)[i];
      ((char*)s.ptr)[i]=((char*)s.ptr)[i+1];
      ((char*)s.ptr)[i+1]=tmp;
    }
    s.type&=~mcpSampBigEndian;
  }

  if (s.type&mcpSampDelta)
  {
    if (s.type&mcpSampStereo)
    {
      if (s.type&mcpSamp16Bit)
      {
        short oldl=0;
        short oldr=0;
        for (i=0; i<s.length; i++)
        {
          oldl=(((short*)s.ptr)[2*i]+=oldl);
          oldr=(((short*)s.ptr)[2*i+1]+=oldr);
        }
      }
      else
      {
        char oldl=0;
        char oldr=0;
        for (i=0; i<s.length; i++)
        {
          oldl=(((char*)s.ptr)[2*i]+=oldl);
          oldr=(((char*)s.ptr)[2*i+1]+=oldr);
        }
      }
    }
    else
    {
      if (s.type&mcpSamp16Bit)
      {
        short old=0;
        for (i=0; i<s.length; i++)
          old=(((short*)s.ptr)[i]+=old);
      }
      else
      {
        char old=0;
        for (i=0; i<s.length; i++)
          old=(((char*)s.ptr)[i]+=old);
      }
    }
    s.type&=~(mcpSampDelta|mcpSampUnsigned);
  }
  if (s.type&mcpSampUnsigned)
  {
    int l=s.length<<stereosizefac(s.type);
    if (s.type&mcpSamp16Bit)
    {
      for (i=0; i<l; i++)
        ((short *)s.ptr)[i]^=0x8000;
    }
    else
    {
      for (i=0; i<l; i++)
        ((char *)s.ptr)[i]^=0x80;
    }
    s.type&=~mcpSampUnsigned;
  }

  return 1;
}

int mcpReduceSamples(sampleinfo *si, int n, long mem, int opt)
{
  sampleinfo *samples=si;
  unsigned long memmax=mem;
  int samplenum=n;

  int i;
  for (i=0; i<samplenum; i++)
  {
    if (!convertsample(samples[i]))
      return 0;
    repairloop(samples[i]);
    if (!expandsmp(samples[i], opt&mcpRedNoPingPong))
      return 0;
  }

  if (opt&mcpRedToMono)
    for (i=0; i<samplenum; i++)
      if (samples[i].type&mcpSampStereo)
        samptomono(samples[i]);

  if (opt&(mcpRedGUS|mcpRedTo8Bit))
    for (i=0; i<samplenum; i++)
      if ((samples[i].type&mcpSamp16Bit)&&((opt&mcpRedTo8Bit)||((samples[i].length+SAMPEND)>(128*1024))))
        sampto8(samples[i]);

  if (totalsmpsize(samples, samplenum, opt&mcpRedAlways16Bit)>memmax)
  {
    unsigned long *redpars=new unsigned long [samplenum];
    if (!redpars)
      return 0;
    if ((opt&mcpRedAlways16Bit)||!reduce16(samples, samplenum, redpars, memmax))
      if (!reducestereo(samples, samplenum, redpars, memmax))
        if (!reducefrq(samples, samplenum, redpars, memmax))
          return 0;
    delete redpars;
  }

  for (i=0; i<samplenum; i++)
    if (!repairsmp(samples[i]))
      return 0;

  return 1;
}
