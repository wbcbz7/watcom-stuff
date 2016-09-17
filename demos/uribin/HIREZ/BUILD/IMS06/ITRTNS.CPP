// (c) 1994-1997 Niklas Beisert, this file is part of "Indoor Music System".
// you must not read, modify, print, compile or copy this file or parts of
// it unless you have accepted the license in the accompanying file IMS.TXT.

#include <string.h>
#include "itplay.h"

void itplayerclass::module::optimizepatlens()
{
  unsigned char *lastrows=new unsigned char[npat];
  if (!lastrows)
    return;
  memset(lastrows, 0, npat);
  int i;
  for (i=0; i<nord; i++)
  {
    if (orders[i]==0xFFFF)
      continue;
    unsigned char *t=patterns[orders[i]];
    unsigned char first=0;
    int row=0;
    int newpat=-1;
    int newrow;
    while (row<patlens[orders[i]])
    {
      if (!*t++)
      {
        if (newpat!=-1)
        {
          while ((newpat<nord)&&(orders[newpat]==0xFFFF))
            newpat++;
          if (newpat>=nord)
          {
            newpat=0;
            newrow=0;
          }
          if ((newrow>=patlens[orders[newpat]]))
          {
            newpat++;
            newrow=0;
          }
          if (newpat>=nord)
            newpat=0;
          if (newrow)
            lastrows[orders[newpat]]=patlens[orders[newpat]]-1;
          if (!first)
          {
            first=1;
            if (!lastrows[orders[i]])
               lastrows[orders[i]]=row;
          }
        }
        row++;
        newpat=-1;
      }
      else
      {
        switch (t[3])
        {
        case cmdJump:
          newpat=t[4];
          newrow=0;
          break;
        case cmdBreak:
          if (newpat==-1)
            newpat=i+1;
          newrow=t[4];
          break;
        }
        t+=5;
      }
    }
    if (!first)
      lastrows[orders[i]]=patlens[orders[i]]-1;
  }

  for (i=0; i<npat; i++)
    patlens[i]=lastrows[i]+1;
  delete lastrows;
}

