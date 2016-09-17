// (c) 1994-1997 Niklas Beisert, this file is part of "Indoor Music System".
// you must not read, modify, print, compile or copy this file or parts of
// it unless you have accepted the license in the accompanying file IMS.TXT.

#include <string.h>
#include "mcp.h"
#include "binfile.h"
#include "gmdinst.h"
#include "xmplay.h"
#include "err.h"

void xmpFreeModule(xmodule &m)
{
  int i;
  if (m.sampleinfos)
    for (i=0; i<m.nsampi; i++)
      delete m.sampleinfos[i].ptr;
  delete m.sampleinfos;
  delete m.samples;
  if (m.envelopes)
    for (i=0; i<m.nenv; i++)
      delete m.envelopes[i].env;
  delete m.envelopes;
  delete m.instruments;
  if (m.patterns)
    for (i=0; i<m.npat; i++)
      delete m.patterns[i];
  delete m.patterns;
  delete m.patlens;
  delete m.orders;
}

void xmpOptimizePatLens(xmodule &m)
{
  unsigned char *lastrows=new unsigned char[m.npat];
  if (!lastrows)
    return;
  memset(lastrows, 0, m.npat);
  int i,j,k;
  for (i=0; i<m.nord; i++)
  {
    if (m.orders[i]==0xFFFF)
      continue;
    int first=0;
    for (j=0; j<m.patlens[m.orders[i]]; j++)
    {
      int neword=-1;
      int newrow;
      for (k=0; k<m.nchan; k++)
        switch (m.patterns[m.orders[i]][m.nchan*j+k][3])
        {
        case xmpCmdJump:
          neword=m.patterns[m.orders[i]][m.nchan*j+k][4];
          newrow=0;
          break;
        case xmpCmdBreak:
          if (neword==-1)
            neword=i+1;
          newrow=m.patterns[m.orders[i]][m.nchan*j+k][4];
          break;
        }
      if (neword!=-1)
      {
        while ((neword<m.nord)&&(m.orders[neword]==0xFFFF))
          neword++;
        if (neword>=m.nord)
        {
          neword=0;
          newrow=0;
        }
        if ((newrow>=m.patlens[m.orders[neword]]))
        {
          neword++;
          newrow=0;
        }
        if (neword>=m.nord)
          neword=0;
        if (newrow)
          lastrows[m.orders[neword]]=m.patlens[m.orders[neword]]-1;
        if (!first)
        {
          first=1;
          if (!lastrows[m.orders[i]])
            lastrows[m.orders[i]]=j;
        }
      }
    }
    if (!first)
      lastrows[m.orders[i]]=m.patlens[m.orders[i]]-1;
  }

  for (i=0; i<m.npat; i++)
    m.patlens[i]=lastrows[i]+1;
  delete lastrows;
}
