// binfile class (c) '96/97 Niklas Beisert

#include <string.h>
#include <stdlib.h>
#include "binfmem.h"

mbinfile::mbinfile()
{
}

int mbinfile::open(void *buf, long len, int type)
{
  close();
  filebuf=(char*)buf;
  mode=canread|canseek|((type&openrw)?canwrite:0);
  filepos=0;
  filelen=len;
  freemem=type&openfree;
  return 1;
}

int mbinfile::opencs(void *&buf, long &len, int inc)
{
  close();
  fbuf=&buf;
  flen=&len;
  fbuflen=*flen;
  fleninc=inc;
  mode=canread|canwrite|canseek|canchsize;
  filelen=*flen;
  filebuf=(char*)*fbuf;
  filepos=0;
  return 1;
}

void mbinfile::close()
{
  if (mode&canchsize)
    *fbuf=realloc(*fbuf,*flen);
  else
    if (mode&&freemem)
      delete *fbuf;
  binfile::close();
}

long mbinfile::read(void *buf, long len)
{
  if (!(mode&canread))
    return 0;
  if ((filepos+len)>filelen)
    len=filelen-filepos;
  memcpy(buf, filebuf+filepos, len);
  filepos+=len;
  return len;
}

long mbinfile::write(const void *buf, long len)
{
  if (!(mode&canwrite))
    return 0;
  if (len>(filelen-filepos))
  {
    chsize(filepos+len);
    if (len>(filelen-filepos))
      len=filelen-filepos;
  }
  memcpy(filebuf+filepos, buf, len);
  filepos+=len;
  return len;
}

long mbinfile::seek(long pos)
{
  if (!(mode&canseek))
    return filepos;
  if (pos<0)
    pos=0;
  if (pos>filelen)
    pos=filelen;
  if (pos==filepos)
    return filepos;
  filepos=pos;
  return filepos;
}

long mbinfile::chsize(long len)
{
  if (!(mode&canchsize))
    return filelen;
  if (len<0)
    len=0;
  if (len<=fbuflen)
  {
    *flen=len;
    filelen=*flen;
    return filelen;
  }
  void *n=realloc(*fbuf,len+fleninc);
  if (!n)
    return filelen;
  *fbuf=n;
  *flen=len;
  fbuflen=len+fleninc;
  filebuf=(char*)*fbuf;
  memset(filebuf+filelen, 0, fbuflen-filelen);
  filelen=*flen;
  if (filepos>filelen)
    filepos=filelen;
  return filelen;
}
