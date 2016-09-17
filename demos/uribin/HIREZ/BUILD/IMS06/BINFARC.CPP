// binfile class (c) '96/97 Niklas Beisert

#include "binfarc.h"

abinfile::abinfile()
{
}

int abinfile::open(binfile &fil, long ofs, long len)
{
  close();
  int fmode=fil.getmode()&~canchsize;
  if (!fmode||!(fmode&canseek))
    return 0;
  f=&fil;
  fofs=ofs;
  filelen=len;
  filepos=0;
  long l=f->length();
  if (fofs>l)
    fofs=l;
  if ((fofs+filelen)>l)
    filelen=l-fofs;
  mode=fmode;
  return 1;
}

long abinfile::read(void *buf, long len)
{
  if (!(mode&canread))
    return 0;
  if ((filepos+len)>filelen)
    len=filelen-filepos;
  f->seek(fofs+filepos);
  len=f->read(buf, len);
  filepos+=len;
  return len;
}

long abinfile::write(const void *buf, long len)
{
  if (!(mode&canwrite))
    return 0;
  if ((filepos+len)>filelen)
    len=filelen-filepos;
  f->seek(fofs+filepos);
  len=f->write(buf, len);
  filepos+=len;
  return len;
}

long abinfile::seek(long pos)
{
  if (!(mode&canseek))
    return filepos;
  if (pos<0)
    pos=0;
  if (pos>filelen)
    pos=filelen;
  filepos=pos;
  return filepos;
}
