// (c) 1994-1997 Niklas Beisert, this file is part of "Indoor Music System".
// you must not read, modify, print, compile or copy this file or parts of
// it unless you have accepted the license in the accompanying file IMS.TXT.

#include <string.h>
#include <stdio.h>
#include "mcp.h"
#include "binfile.h"
#include "gmdinst.h"
#include "xmplay.h"
#include "err.h"

#ifdef USE_STDIO
int xmpLoadModule(xmodule &m, FILE *file)
#else
int xmpLoadModule(xmodule &m, binfile &file)
#endif
{
  m.envelopes=0;
  m.samples=0;
  m.instruments=0;
  m.sampleinfos=0;
  m.patlens=0;
  m.patterns=0;
  m.orders=0;

  struct
  {
    char sig[17];
    char name[20];
    char eof;
    char tracker[20];
    unsigned short ver;
    unsigned long hdrsize;
  } head1;

  struct
  {
    unsigned short nord;
    unsigned short loopord;
    unsigned short nchan;
    unsigned short npat;
    unsigned short ninst;
    unsigned short freqtab;
    unsigned short tempo;
    unsigned short bpm;
    unsigned char ord[256];
  } head2;

#ifdef USE_STDIO
  fread(&head1, sizeof(head1), 1, file);
#else
  file.read(&head1, sizeof(head1));
#endif

  if (memcmp(head1.sig, "Extended Module: ", 17))
    return errFormStruc;
  if (head1.eof!=26)
    return errFormStruc;
  if (head1.ver<0x104)
    return errFormOldVer;
    
#ifdef USE_STDIO
  fread(&head2, sizeof(head2), 1, file);
  fseek(file, head1.hdrsize-4-sizeof(head2), SEEK_CUR);
#else    
  file.read(&head2, sizeof(head2));
  file.seekcur(head1.hdrsize-4-sizeof(head2));
#endif
  
  if (!head2.ninst)
    return errFormMiss;

  memcpy(m.name, head1.name, 20);
  m.name[20]=0;

  m.linearfreq=!!(head2.freqtab&1);
  m.nchan=head2.nchan;
  m.ninst=head2.ninst;
  m.nenv=head2.ninst*2;
  m.npat=head2.npat+1;
  m.nord=head2.nord;
  m.loopord=head2.loopord;
  m.inibpm=head2.bpm;
  m.initempo=head2.tempo;

  m.orders=new unsigned short [head2.nord];
  m.patterns=(unsigned char (**)[5])new void *[head2.npat+1];
  m.patlens=new unsigned short [head2.npat+1];
  m.instruments=new gmdinstrument [head2.ninst];
  m.envelopes=new gmdenvelope [head2.ninst*2];
  sampleinfo **smps=new sampleinfo *[head2.ninst];
  gmdsample **msmps=new gmdsample *[head2.ninst];
  int *instsmpnum=new int [head2.ninst];

  if (!smps||!msmps||!instsmpnum||!m.instruments||!m.envelopes||!m.patterns||!m.orders||!m.patlens)
    return errAllocMem;

  memset(m.patterns, 0, (head2.npat+1)*sizeof(void*));
  memset(m.envelopes, 0, (head2.ninst*2)*sizeof(gmdenvelope));

  int i,j,k;

  for (i=0; i<head2.nord; i++)
    m.orders[i]=(head2.ord[i]<head2.npat)?head2.ord[i]:head2.npat;

  m.patlens[head2.npat]=64;
  m.patterns[head2.npat]=new unsigned char [64*head2.nchan][5];
  if (!m.patterns[head2.npat])
    return errAllocMem;
  memset(m.patterns[head2.npat], 0, 64*5*head2.nchan);

  for (i=0; i<head2.npat; i++)
  {
    struct
    {
      unsigned long len;
      unsigned char ptype;
      unsigned short rows;
      unsigned short patdata;
    } pathead;

#ifdef USE_STDIO
    fread(&pathead, sizeof(pathead), 1, file);
    fseek(file, pathead.len-sizeof(pathead), SEEK_CUR);
#else
    file.read(&pathead, sizeof(pathead));
    file.seekcur(pathead.len-sizeof(pathead));
#endif  
  
    m.patlens[i]=pathead.rows;
    m.patterns[i]=new unsigned char [pathead.rows*head2.nchan][5];
    if (!m.patterns[i])
      return errAllocMem;
    memset(m.patterns[i], 0, pathead.rows*head2.nchan*5);
    if (!pathead.patdata)
      continue;
    unsigned char *pbuf=new unsigned char [pathead.patdata];
    if (!pbuf)
      return errAllocMem;
      
#ifdef USE_STDIO
    fread(pbuf, pathead.patdata, 1, file);
#else
    file.read(pbuf, pathead.patdata);
#endif  

    unsigned char *pbp=pbuf;
    unsigned char *cur=(unsigned char*)(m.patterns[i]);
    for (j=0; j<(pathead.rows*head2.nchan); j++)
    {
      unsigned char pack=(*pbp&0x80)?(*pbp++):0x1F;
      for (k=0; k<5; k++)
      {
        *cur++=(pack&1)?*pbp++:0;
        pack>>=1;
      }
      if (cur[-2]==0xE)
      {
        cur[-2]=36+(cur[-1]>>4);
        cur[-1]&=0xF;
      }
    }
    delete pbuf;
  }

  m.nsampi=0;
  m.nsamp=0;
  for (i=0; i<m.ninst; i++)
  {
    gmdinstrument &ip=m.instruments[i];
    gmdenvelope *env=m.envelopes+2*i;
    smps[i]=0;
    msmps[i]=0;
    struct
    {
      unsigned long size;
      char name[22];
      char type;
      unsigned short samp;
    } ins1;
    
#ifdef USE_STDIO
    fread(&ins1, sizeof(ins1), 1, file);
#else    
    file.read(&ins1, sizeof(ins1));
#endif
    
    memcpy(ip.name, ins1.name, 22);
    ip.name[22]=0;
    memset(ip.samples, -1, 256);
    instsmpnum[i]=ins1.samp;
    if (!ins1.samp)
    {
#ifdef USE_STDIO
      fseek(file, ins1.size-sizeof(ins1), SEEK_CUR);
#else    
      file.seekcur(ins1.size-sizeof(ins1));
#endif

      continue;
    }
    struct
    {
      unsigned long shsize;
      unsigned char snum[96];
      unsigned short venv[12][2];
      unsigned short penv[12][2];
      unsigned char vnum, pnum;
      unsigned char vsustain, vloops, vloope, psustain, ploops, ploope;
      unsigned char vtype, ptype;
      unsigned char vibtype, vibsweep, vibdepth, vibrate;
      unsigned short volfade;
      unsigned short res;
    } ins2;
    
#ifdef USE_STDIO
    fread(&ins2, sizeof(ins2), 1, file);
    fseek(file, ins1.size-sizeof(ins1)-sizeof(ins2), SEEK_CUR);
#else 
    file.read(&ins2, sizeof(ins2));
    file.seekcur(ins1.size-sizeof(ins1)-sizeof(ins2));
#endif

    smps[i]=new sampleinfo[ins1.samp];
    msmps[i]=new gmdsample[ins1.samp];
    if (!smps[i]||!msmps[i])
      return errAllocMem;
    memset(msmps[i], 0, sizeof(**msmps)*ins1.samp);
    memset(smps[i], 0, sizeof(**smps)*ins1.samp);

    memset(ip.samples, 0xFF, 128*2);
    for (j=0; j<96; j++)
      if (ins2.snum[j]<ins1.samp)
        ip.samples[j]=m.nsamp+ins2.snum[j];
    unsigned short volfade=0xFFFF;
    volfade=ins2.volfade;
    if (ins2.vtype&1)
    {
      env[0].speed=0;
      env[0].type=0;
      env[0].env=new unsigned char[ins2.venv[ins2.vnum-1][0]+1];
      if (!env[0].env)
        return errAllocMem;
      short k, p=0, h=ins2.venv[0][1]*4;
      for (j=1; j<ins2.vnum; j++)
      {
        short l=ins2.venv[j][0]-p;
        short dh=ins2.venv[j][1]*4-h;
        for (k=0; k<l; k++)
        {
          short cv=h+dh*k/l;
          env[0].env[p++]=(cv>255)?255:cv;
        }
        h+=dh;
      }
      env[0].len=p;
      env[0].env[p]=(h>255)?255:h;
      if (ins2.vtype&2)
      {
        env[0].type|=mpEnvSLoop;
        if (!(ins2.vtype&4)||(ins2.vsustain<=ins2.vloope))
        {
          env[0].sloops=ins2.venv[ins2.vsustain][0];
          env[0].sloope=ins2.venv[ins2.vsustain][0]+1;
        }
        else
        {
          env[0].sloops=ins2.venv[ins2.vloops][0];
          env[0].sloope=ins2.venv[ins2.vloope][0];
        }
      }
      if (ins2.vtype&4)
      {
        if ((ins2.vtype&2)&&(ins2.vsustain==ins2.vloope))
        {
          env[0].type|=mpEnvSLoop;
          env[0].sloops=ins2.venv[ins2.vloops][0];
          env[0].sloope=ins2.venv[ins2.vloope][0];
        }
        else
        {
          env[0].type|=mpEnvLoop;
          env[0].loops=ins2.venv[ins2.vloops][0];
          env[0].loope=ins2.venv[ins2.vloope][0];
        }
      }
    }
    if (ins2.ptype&1)
    {
      env[1].speed=0;
      env[1].type=0;
      env[1].env=new unsigned char[ins2.penv[ins2.pnum-1][0]+1];
      if (!env[1].env)
        return errAllocMem;
      short k, p=0, h=ins2.penv[0][1]*4;
      for (j=1; j<ins2.pnum; j++)
      {
        short l=ins2.penv[j][0]-p;
        short dh=ins2.penv[j][1]*4-h;
        for (k=0; k<l; k++)
        {
          short cv=h+dh*k/l;
          env[1].env[p++]=(cv>255)?255:cv;
        }
        h+=dh;
      }
      env[1].len=p;
      env[1].env[p]=(h>255)?255:h;
      if (ins2.ptype&2)
      {
        env[1].type|=mpEnvSLoop;
        if (!(ins2.ptype&4)||(ins2.psustain<=ins2.ploope))
        {
          env[1].sloops=ins2.penv[ins2.psustain][0];
          env[1].sloope=ins2.penv[ins2.psustain][0]+1;
        }
        else
        {
          env[1].sloops=ins2.penv[ins2.ploops][0];
          env[1].sloope=ins2.penv[ins2.ploope][0];
        }
      }
      if (ins2.ptype&4)
      {
        if ((ins2.ptype&2)&&(ins2.psustain==ins2.ploope))
        {
          env[1].type|=mpEnvSLoop;
          env[1].sloops=ins2.penv[ins2.ploops][0];
          env[1].sloope=ins2.penv[ins2.ploope][0];
        }
        else
        {
          env[1].type|=mpEnvLoop;
          env[1].loops=ins2.penv[ins2.ploops][0];
          env[1].loope=ins2.penv[ins2.ploope][0];
        }
      }
    }
    for (j=0; j<ins1.samp; j++)
    {
      struct
      {
        unsigned long samplen;
        unsigned long loopstart;
        unsigned long looplen;
        unsigned char vol;
        signed char finetune;
        unsigned char type;
        unsigned char pan;
        signed char relnote;
        unsigned char res;
        unsigned char name[22];
      } samp;
     
#ifdef USE_STDIO     
      fread(&samp, sizeof(samp), 1, file);
      fseek(file, ins2.shsize-sizeof(samp), SEEK_CUR);
#else
      file.read(&samp, sizeof (samp));
      file.seekcur(ins2.shsize-sizeof(samp));
#endif
      
      if (samp.type&16)
      {
        samp.samplen>>=1;
        samp.loopstart>>=1;
        samp.looplen>>=1;
      }

      gmdsample &sp=msmps[i][j];
      memcpy(sp.name, samp.name, 22);
      sp.name[22]=0;
      sp.handle=0xFFFF;
      sp.normnote=-samp.relnote*256-samp.finetune*2;
      sp.stdvol=(samp.vol>0x3F)?0xFF:(samp.vol<<2);
      sp.stdpan=samp.pan;
      sp.opt=0;
      sp.volfade=volfade;
      sp.vibtype=ins2.vibtype;
      sp.vibdepth=ins2.vibdepth<<2;
      sp.vibspeed=0;
      sp.vibrate=ins2.vibrate<<8;
      sp.vibsweep=0xFFFF/(ins2.vibsweep+1);
      sp.volenv=env[0].env?(2*i+0):0xFFFF;
      sp.panenv=env[1].env?(2*i+1):0xFFFF;
      sp.pchenv=0xFFFF;

      sampleinfo &sip=smps[i][j];
      sip.length=samp.samplen;
      sip.loopstart=samp.loopstart;
      sip.loopend=samp.loopstart+samp.looplen;
      sip.samprate=8363;
      sip.type=mcpSampDelta|((samp.type&16)?mcpSamp16Bit:0)|((samp.type&3)?(((samp.type&3)==2)?(mcpSampLoop|mcpSampBiDi):mcpSampLoop):0);
    }
    for (j=0; j<ins1.samp; j++)
    {
      gmdsample &sp=msmps[i][j];
      sampleinfo &sip=smps[i][j];
      unsigned long l=sip.length<<(!!(sip.type&mcpSamp16Bit));
      if (!l)
        continue;
      sip.ptr=new char [l+16];
      if (!sip.ptr)
        return errAllocMem;
     
#ifdef USE_STDIO
      fread(sip.ptr, l, 1, file);
#else      
      file.read(sip.ptr, l);
#endif
      
      sp.handle=m.nsampi++;
    }
    m.nsamp+=ins1.samp;
  }

  m.samples=new gmdsample [m.nsamp];
  m.sampleinfos=new sampleinfo [m.nsampi];
  if (!m.samples||!m.sampleinfos)
    return errAllocMem;

  m.nsampi=0;
  m.nsamp=0;
  for (i=0; i<m.ninst; i++)
  {
    for (j=0; j<instsmpnum[i]; j++)
    {
      m.samples[m.nsamp++]=msmps[i][j];
      if (smps[i][j].ptr)
        m.sampleinfos[m.nsampi++]=smps[i][j];
    }
    delete smps[i];
    delete msmps[i];
  }
  delete smps;
  delete msmps;
  delete instsmpnum;

  return errOk;
}
