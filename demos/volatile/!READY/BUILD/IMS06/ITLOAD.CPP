// (c) 1994-1997 Niklas Beisert, this file is part of "Indoor Music System".
// you must not read, modify, print, compile or copy this file or parts of
// it unless you have accepted the license in the accompanying file IMS.TXT.

#include <stdlib.h>
#include <stdio.h>
#include <conio.h>
#include <string.h>
#include "itplay.h"
#include "mcp.h"
#include "err.h"
#include "binfile.h"

#ifdef USE_STDIO
int itplayerclass::module::load(FILE *file)
#else
int itplayerclass::module::load(binfile &file)
#endif

{
  int i,j,k,l;

  nchan=0;
  ninst=0;
  nsampi=0;
  nsamp=0;
  npat=0;
  nord=0;
  message=0;
  orders=0;
  patlens=0;
  patterns=0;
  samples=0;
  instruments=0;
  sampleinfos=0;

  struct
  {
    unsigned long sig;
    unsigned char name[26];
    unsigned short _d1;
    unsigned short nords;
    unsigned short nins;
    unsigned short nsmps;
    unsigned short npats;
    unsigned short cwtv;
    unsigned short cmwt;
    unsigned short flags;
    unsigned short special;
    unsigned char gvol;
    unsigned char mvol;
    unsigned char ispd;
    unsigned char itmp;
    unsigned char chsep;
    unsigned char _d2;
    unsigned short msglen;
    unsigned long msgoff;
    unsigned long _d3;
    unsigned char pan[64];
    unsigned char vol[64];
  } hdr;

#ifdef USE_STDIO
  fread(&hdr, sizeof(hdr), 1, file);
#else
  file.read(&hdr, sizeof(hdr));
#endif
  
  if (hdr.sig!=0x4D504D49)
    return errFormSig;

  if ((hdr.flags&4)&&(hdr.cmwt<0x200))
    return errFormOldVer;

  int signedsamp=1;
  if (hdr.cwtv<0x202)
    signedsamp=0;

  unsigned char ords[256];
  unsigned long sampoff[100];
  unsigned long insoff[100];
  unsigned long patoff[200];

#ifdef USE_STDIO
  fread(&ords, hdr.nords, 1, file);
  fread(&insoff, hdr.nins*4, 1, file);
  fread(&sampoff, hdr.nsmps*4, 1, file);
  fread(&patoff, hdr.npats*4, 1, file);
#else
  file.read(ords, hdr.nords);
  file.read(insoff, hdr.nins*4);
  file.read(sampoff, hdr.nsmps*4);
  file.read(patoff, hdr.npats*4);
#endif
  
  memcpy(name, hdr.name, 26);
  name[26]=0;
  linearfreq=!!(hdr.flags&8);
  inispeed=hdr.ispd;
  initempo=hdr.itmp;
  inigvol=hdr.gvol;
  chsep=(hdr.flags&1)?hdr.chsep:0;
  instmode=hdr.flags&4;
  linear=hdr.flags&8;
  oldfx=hdr.flags&16;
  geffect=hdr.flags&32;
  instmode=hdr.flags&2;
  memcpy(inipan, hdr.pan, 64);
  memcpy(inivol, hdr.vol, 64);

  if (hdr.special&1)
  {
    char *msg=new char [hdr.msglen+1];
    if (!msg)
      return errAllocMem;
     
#ifdef USE_STDIO     
    fseek(file, hdr.msgoff, SEEK_SET);
    fread(msg, hdr.msglen, 1, file);
#else
    file.seek(hdr.msgoff);
    file.read(msg, hdr.msglen);
#endif
    
    msg[hdr.msglen]=0;
    int linect=1;
    for (i=0; i<hdr.msglen; i++)
    {
      if (msg[i]==0)
        break;
      if (msg[i]==13)
        linect++;
    }
    message=new char *[linect+1];
    if (!message)
      return errAllocMem;
    *message=msg;
    linect=1;
    for (i=0; i<hdr.msglen; i++)
    {
      if (msg[i]==0)
        break;
      if (msg[i]==13)
      {
        msg[i]=0;
        message[linect++]=msg+i+1;
      }
    }
    message[linect]=0;
  }

  npat=hdr.npats+1;
  nord=hdr.nords;
  for (i=nord-1; i>=0; i--)
  {
    if (ords[i]<254)
      break;
    nord--;
  }
  if (!nord)
    return errFormMiss;

  for (i=0; i<nord; i++)
    if (ords[i]==255)
      break;
  for (i=i; i>0; i--)
    if (ords[i-1]!=254)
      break;
  endord=i;

  orders=new unsigned short [nord];
  if (!orders)
    return errAllocMem;
  for (i=0; i<nord; i++)
    orders[i]=(ords[i]==254)?0xFFFF:(ords[i]>=hdr.npats)?hdr.npats:ords[i];

  patlens=new unsigned short [npat];
  patterns=new unsigned char *[npat];
  if (!patlens||!patterns)
    return errAllocMem;
  patlens[npat-1]=64;
  patterns[npat-1]=new unsigned char [64];
  if (!patterns[npat-1])
    return errAllocMem;
  memset(patterns[npat-1], 0, 64);

  int maxchan=0;

  for (k=0; k<hdr.npats; k++)
  {
    if (!patoff[k])
    {
      patlens[k]=64;
      patterns[k]=new unsigned char [patlens[k]];
      if (!patterns[k])
        return errAllocMem;
      memset(patterns[k], 0, patlens[k]);
      continue;
    }
    
#ifdef USE_STDIO
    fseek(file, patoff[k], SEEK_SET);
#else
    file.seek(patoff[k]);
#endif
 
#ifdef USE_STDIO
    int patlen, patrows;
    fread(&patlen, 2, 1, file);
    fread(&patrows, 2, 1, file);
#else 
    int patlen=file.getus();
    int patrows=file.getus();
#endif
    
#ifdef USE_STDIO
    fseek(file, 4, SEEK_CUR);
#else
    file.getl();
#endif
    
    unsigned char *patbuf=new unsigned char [patlen];
    if (!patbuf)
      return errAllocMem;
    
#ifdef USE_STDIO
    fread(patbuf, patlen, 1, file);
#else
    file.read(patbuf, patlen);
#endif

    patlens[k]=patrows;
    patterns[k]=new unsigned char [(6*64+1)*patlens[k]];
    if (!patterns[k])
      return errAllocMem;

    unsigned char lastmask[64];
    unsigned char lastnote[64];
    unsigned char lastins[64];
    unsigned char lastvolpan[64];
    unsigned char lastcmd[64];
    unsigned char lastdata[64];
    unsigned char *pp=patbuf;
    unsigned char *wp=patterns[k];
    for (i=0; i<patlens[k]; i++)
    {
      while (1)
      {
        int chn=*pp++;
        if (!chn)
          break;
        if (((chn&127)-1)>=64)
          return errFormSupp;
        if (chn&128)
          lastmask[(chn&127)-1]=*pp++;
        chn=(chn&127)-1;
        int c=lastmask[chn];
        if (!c)
          continue;
        if (maxchan<=chn)
          maxchan=chn+1;
        if (c&1)
        {
          lastnote[chn]=(*pp<=120)?(*pp+1):*pp;
          pp++;
        }
        if (c&2)
          lastins[chn]=*pp++;
        if (c&4)
        {
          lastvolpan[chn]=1+*pp++;
        }
        if (c&8)
        {
          lastcmd[chn]=*pp++;
          lastdata[chn]=*pp++;
        }
        *wp++=chn+1;
        *wp++=(c&0x11)?lastnote[chn]:0;
        *wp++=(c&0x22)?lastins[chn]:0;
        *wp++=(c&0x44)?lastvolpan[chn]:0;
        *wp++=(c&0x88)?lastcmd[chn]:0;
        *wp++=(c&0x88)?lastdata[chn]:0;
      }
      *wp++=0;
    }
    delete patbuf;
    patterns[k]=(unsigned char*)realloc(patterns[k], wp-patterns[k]);
  }

  
  if (!maxchan)
    return errFormSupp;
  nchan=maxchan;

  nsampi=hdr.nsmps;
  nsamp=hdr.nsmps;

  sampleinfos=new sampleinfo [nsampi];
  samples=new sample [nsamp];
  if (!sampleinfos||!samples)
    return errAllocMem;
  memset(sampleinfos, 0, nsampi*sizeof(*sampleinfos));
  memset(samples, 0, nsamp*sizeof(*samples));

  for (i=0; i<hdr.nsmps; i++)
    samples[i].handle=0xFFFF;

  for (i=0; i<hdr.nsmps; i++)
  {
    struct
    {
      unsigned long sig;
      char dosname[13];
      unsigned char gvl;
      unsigned char flags;
      unsigned char vol;
      char name[26];
      unsigned char cvt;
      unsigned char dfp;
      unsigned long length;
      unsigned long loopstart;
      unsigned long loopend;
      unsigned long c5spd;
      unsigned long sloopstart;
      unsigned long sloopend;
      unsigned long off;
      unsigned char vis;
      unsigned char vid;
      unsigned char vir;
      unsigned char vit;
    } shdr;
   
#ifdef USE_STDIO   
    fseek(file, sampoff[i], SEEK_SET);
    fread(&shdr, sizeof(shdr), 1, file);
#else
    file.seek(sampoff[i]);
    file.read(&shdr, sizeof(shdr));
#endif
    
    sampoff[i]=shdr.off;

    sampleinfo &sip=sampleinfos[i];
    sample &sp=samples[i];

    memcpy(sp.name, shdr.name, 26);
    sp.name[26]=0;

    if (!(shdr.flags&1))
      continue;

    sp.vol=shdr.vol;
    sp.gvl=shdr.gvl;
    sp.vir=shdr.vir;
    sp.vid=shdr.vid;
    sp.vit=shdr.vit;
    sp.vis=shdr.vis;
    sp.dfp=shdr.dfp;
    sp.handle=i;
    sp.normnote=-mcpGetNote8363(shdr.c5spd);
    sip.length=shdr.length;
    sip.loopstart=shdr.loopstart;
    sip.loopend=shdr.loopend;
    sip.sloopstart=shdr.sloopstart;
    sip.sloopend=shdr.sloopend;
    sip.samprate=shdr.c5spd>>((shdr.flags&2)?1:0);
    sip.samprate=8363;
    sip.type=((shdr.flags&2)?mcpSamp16Bit:0)|((shdr.flags&4)?mcpSampStereo:0)|(signedsamp?0:mcpSampUnsigned)|((shdr.flags&0x10)?mcpSampLoop:0)|((shdr.flags&0x40)?mcpSampBiDi:0)|((shdr.flags&0x80)?mcpSampSBiDi:0)|((shdr.flags&0x20)?mcpSampSLoop:0);
    sip.ptr=new char [sip.length<<((sip.type&mcpSamp16Bit)?1:0)];
    if (!sip.ptr)
      return errAllocMem;
  }

  for (i=0; i<hdr.nsmps; i++)
  {
    sampleinfo &sip=sampleinfos[i];
    if (!sip.ptr)
      continue;
      
#ifdef USE_STDIO
    fseek(file, sampoff[i], SEEK_SET);
    fread(sip.ptr, sip.length<<((sip.type&mcpSamp16Bit)?1:0), 1, file);
#else
    file.seek(sampoff[i]);
    file.read(sip.ptr, sip.length<<((sip.type&mcpSamp16Bit)?1:0));
#endif
  }
  
  ninst=(hdr.flags&4)?hdr.nins:hdr.nsmps;
  instruments=new instrument [ninst];
  if (!instruments)
    return errAllocMem;
  memset(instruments, 0, ninst*sizeof(*instruments));

  for (k=0; k<ninst; k++)
  {
    struct envp
    {
      signed char v;
      unsigned short p;
    };
    struct env
    {
      unsigned char flags;
      unsigned char num;
      unsigned char lpb;
      unsigned char lpe;
      unsigned char slb;
      unsigned char sle;
      envp pts[25];
      unsigned char _d1;
    };
    struct
    {
      unsigned long sig;
      char dosname[13];
      unsigned char nna;
      unsigned char dct;
      unsigned char dca;
      unsigned short fadeout;
      unsigned char pps;
      unsigned char ppc;
      unsigned char gbv;
      unsigned char dfp;
      unsigned char rv;
      unsigned char rp;
      unsigned short tver;
      unsigned char nos;
      unsigned char _d3;
      char name[26];
      char _d4[6];
      unsigned char keytab[120][2];
      env envs[3];
    } ihdr;

    if (hdr.flags&4)
    {
#ifdef USE_STDIO
      fseek(file, insoff[k], SEEK_SET);
      fread(&ihdr, sizeof(ihdr), 1, file);
#else
      file.seek(insoff[k]);
      file.read(&ihdr, sizeof(ihdr));
#endif
    }
    else
    {
      memset(&ihdr, 0, sizeof(ihdr));
      for (i=0; i<120; i++)
      {
        ihdr.keytab[i][0]=i;
        ihdr.keytab[i][1]=k+1;
      }
      memcpy(ihdr.name, samples[k].name, 26);
      ihdr.dfp=0x80;
      ihdr.gbv=128;
    }
    
    instrument &ip=instruments[k];
    memcpy(ip.name, ihdr.name, 26);
    ip.name[26]=0;
    ip.fadeout=ihdr.fadeout;
    ip.nna=ihdr.nna;
    ip.dct=ihdr.dct;
    ip.dca=ihdr.dca;
    ip.pps=ihdr.pps;
    ip.ppc=ihdr.ppc;
    ip.gbv=ihdr.gbv;
    ip.dfp=ihdr.dfp;
    ip.rv=ihdr.rv;
    ip.rp=ihdr.rp;
    memcpy(ip.keytab, ihdr.keytab, 120*2);

    for (i=0; i<3; i++)
    {
      env &ie=ihdr.envs[i];
      envelope &e=ip.envs[i];
      e.type=((ie.flags&1)?e.active:0)|((ie.flags&2)?e.looped:0)|((ie.flags&4)?e.slooped:0);
      e.len=ie.num-1;
      e.sloops=ie.slb;
      e.sloope=ie.sle;
      e.loops=ie.lpb;
      e.loope=ie.lpe;

      for (j=0; j<ie.num; j++)
      {
        e.y[j]=ie.pts[j].v;
        e.x[j]=ie.pts[j].p;
      }
    }
  }

  return 0;
};

void itplayerclass::module::free()
{
  int i;
  if (sampleinfos)
    for (i=0; i<nsampi; i++)
      delete sampleinfos[i].ptr;
  delete sampleinfos;
  delete samples;
  delete instruments;
  if (patterns)
    for (i=0; i<npat; i++)
      delete patterns[i];
  delete patterns;
  delete patlens;
  delete orders;
  if (message)
    delete *message;
  delete message;
}
