// (c) 1994-1997 Niklas Beisert, this file is part of "Indoor Music System".
// you must not read, modify, print, compile or copy this file or parts of
// it unless you have accepted the license in the accompanying file IMS.TXT.

#include <string.h>

#include "binfile.h"
#include "mcp.h"
#include "gmdplay.h"

void mpOptimizePatLens(gmdmodule &m)
{
  unsigned char *lastrows=new unsigned char[m.patnum];
  if (!lastrows)
    return;
  memset(lastrows, 0, m.patnum);
  int i;
  for (i=0; i<m.ordnum; i++)
  {
    if (m.orders[i]==0xFFFF)
      continue;
    gmdtrack t=m.tracks[m.patterns[m.orders[i]].gtrack];
    unsigned char first=0;
    while (t.ptr<t.end)
    {
      unsigned char row=*t.ptr;
      t.ptr+=2;
      short newpat=-1;
      unsigned char newrow;
      unsigned char *end;
      for (end=t.ptr+t.ptr[-1]; t.ptr<end; t.ptr+=2)
        switch (t.ptr[0])
        {
        case cmdGoto:
          newpat=t.ptr[1];
          newrow=0;
          break;
        case cmdBreak:
          if (newpat==-1)
            newpat=i+1;
          newrow=t.ptr[1];
          break;
        }
      if (newpat!=-1)
      {
        while ((newpat<m.ordnum)&&(m.orders[newpat]==0xFFFF))
          newpat++;
        if (newpat>=m.ordnum)
        {
          newpat=0;
          newrow=0;
        }
        if ((newrow>=m.patterns[m.orders[newpat]].patlen))
        {
          newpat++;
          newrow=0;
        }
        if (newpat>=m.ordnum)
          newpat=0;
        if (newrow)
          lastrows[m.orders[newpat]]=m.patterns[m.orders[newpat]].patlen-1;
        if (!first)
        {
          first=1;
          if (!lastrows[m.orders[i]])
            lastrows[m.orders[i]]=row;
        }
      }
    }
    if (!first)
      lastrows[m.orders[i]]=m.patterns[m.orders[i]].patlen-1;
  }

  for (i=0; i<m.patnum; i++)
    m.patterns[i].patlen=lastrows[i]+1;
  delete lastrows;
}

void mpReduceInstruments(gmdmodule &m)
{
  int i,j;
  char *inptr;
  for (i=0; i<m.modsampnum; i++)
  {
    gmdsample &smp=m.modsamples[i];
    for (inptr=smp.name; *inptr==' '; inptr++);
    if (!*inptr)
      *smp.name=0;
  }
  for (i=0; i<m.instnum; i++)
  {
    gmdinstrument &ins=m.instruments[i];
    for (inptr=ins.name; *inptr==' '; inptr++);
    if (!*inptr)
      *ins.name=0;
    for (j=0; j<128; j++)
      if ((ins.samples[j]<m.modsampnum)&&(m.modsamples[ins.samples[j]].handle>=m.sampnum))
        ins.samples[j]=0xFFFF;
  }

  for (i=m.instnum-1; i>=0; i--)
  {
    gmdinstrument &ins=m.instruments[i];
    for (j=0; j<128; j++)
      if ((ins.samples[j]<m.modsampnum)&&(m.modsamples[ins.samples[j]].handle<m.sampnum))
        break;
    if ((j!=128)||*m.instruments[i].name)
      break;
    m.instnum--;
  }
}

void mpReduceMessage(gmdmodule &m)
{
  char *mptr;
  for (mptr=m.name; *mptr==' '; mptr++);
  if (!*mptr)
    *m.name=0;
  for (mptr=m.composer; *mptr==' '; mptr++);
  if (!*mptr)
    *m.composer=0;

  if (!m.message)
    return;
  int len,i;
  for (len=0; m.message[len]; len++)
  {
    for (mptr=m.message[len]; *mptr==' '; mptr++);
    if (!*mptr)
      *m.message[len]=0;
  }
  for (i=len-1; i>=0; i--)
  {
    if (*m.message[i])
      break;
    if (i)
      m.message[i]=0;
    else
    {
      delete *m.message;
      delete m.message;
      m.message=0;
    }
  }
}

int mpReduceSamples(gmdmodule &m)
{
  unsigned short *rellist=new unsigned short [m.sampnum];
  if (!rellist)
    return 0;

  int i,j,n;
  n=0;
  for (i=0; i<m.sampnum; i++)
  {
    if (!m.samples[i].ptr)
    {
      rellist[i]=0xFFFF;
      continue;
    }
    m.samples[n]=m.samples[i];
    rellist[i]=n++;
  }

  for (i=0; i<m.modsampnum; i++)
    if (m.modsamples[i].handle<m.sampnum)
      m.modsamples[i].handle=rellist[m.modsamples[i].handle];

  m.sampnum=n;
  delete rellist;

  return 1;
}

void mpRemoveText(gmdmodule &m)
{
  *m.name=0;
  *m.composer=0;
  if (m.message)
    delete *m.message;
  m.message=0;
  delete m.message;
  int i;
  for (i=0; i<m.patnum; i++)
    *m.patterns[i].name=0;
  for (i=0; i<m.instnum; i++)
    *m.instruments[i].name=0;
  for (i=0; i<m.modsampnum; i++)
    *m.modsamples[i].name=0;
}

void mpReset(gmdmodule &m)
{
  m.instruments=0;
  m.tracks=0;
  m.patterns=0;
  m.message=0;
  m.samples=0;
  m.modsamples=0;
  m.envelopes=0;
  m.orders=0;
  *m.composer=0;
  *m.name=0;
}

void mpFree(gmdmodule &m)
{
  int i,j;
  if (m.envelopes)
    for (i=0; i<m.envnum; i++)
      delete m.envelopes[i].env;
  if (m.tracks)
    for (i=0; i<m.tracknum; i++)
      delete m.tracks[i].ptr;
  if (m.message)
    delete *m.message;
  if (m.samples)
    for (i=0; i<m.sampnum; i++)
      delete m.samples[i].ptr;

  delete m.tracks;
  delete m.patterns;
  delete m.message;
  delete m.samples;
  delete m.envelopes;
  delete m.instruments;
  delete m.modsamples;
  delete m.orders;

  mpReset(m);
}

int mpAllocInstruments(gmdmodule &m, int n)
{
  m.instnum=n;
  m.instruments=new gmdinstrument[m.instnum];
  if (!m.instruments)
    return 0;
  memset(m.instruments, 0, m.instnum*sizeof(gmdinstrument));
  int i;
  for (i=0; i<m.instnum; i++)
    memset(m.instruments[i].samples, -1, 2*128);
  return 1;
}

int mpAllocTracks(gmdmodule &m, int n)
{
  m.tracknum=n;
  m.tracks=new gmdtrack[m.tracknum];
  if (!m.tracks)
    return 0;
  memset(m.tracks, 0, m.tracknum*sizeof(gmdtrack));
  return 1;
}

int mpAllocPatterns(gmdmodule &m, int n)
{
  m.patnum=n;
  m.patterns=new gmdpattern[m.patnum];
  if (!m.patterns)
    return 0;
  memset(m.patterns, 0, m.patnum*sizeof(gmdpattern));
  return 1;
}

int mpAllocSamples(gmdmodule &m, int n)
{
  m.sampnum=n;
  m.samples=new sampleinfo[m.sampnum];
  if (!m.samples)
    return 0;
  memset(m.samples, 0, m.sampnum*sizeof(sampleinfo));
  return 1;
}

int mpAllocEnvelopes(gmdmodule &m, int n)
{
  m.envnum=n;
  m.envelopes=new gmdenvelope[m.envnum];
  if (!m.envelopes)
    return 0;
  memset(m.envelopes, 0, m.envnum*sizeof(gmdenvelope));
  return 1;
}

int mpAllocOrders(gmdmodule &m, int n)
{
  m.ordnum=n;
  m.orders=new unsigned short [m.ordnum];
  if (!m.orders)
    return 0;
  memset(m.orders, 0, m.ordnum*2);
  return 1;
}

int mpAllocModSamples(gmdmodule &m, int n)
{
  m.modsampnum=n;
  m.modsamples=new gmdsample[m.modsampnum];
  if (!m.modsamples)
    return 0;
  memset(m.modsamples, 0, m.modsampnum*sizeof(gmdsample));
  int i;
  for (i=0; i<m.modsampnum; i++)
  {
    m.modsamples[i].volfade=0xFFFF;
    m.modsamples[i].volenv=0xFFFF;
    m.modsamples[i].panenv=0xFFFF;
    m.modsamples[i].pchenv=0xFFFF;
    m.modsamples[i].handle=0xFFFF;
  }
  return 1;
}
