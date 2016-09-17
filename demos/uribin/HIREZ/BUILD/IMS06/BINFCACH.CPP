// binfile class (c) '96/97 Niklas Beisert

#include <string.h>
#include "binfcach.h"

binfilecache::binfilecache()
{
}

void binfilecache::invalidatebuf()
{
  if (dirty)
  {
    if (fileseekpos!=filebufpos)
      f->seek(filebufpos);
    bufread=f->write(buffer,bufread);
    fileseekpos=filebufpos+bufread;
    dirty=0;
  }
  filebufpos+=bufread;
  bufread=0;
  bufpos=0;
}

int binfilecache::open(binfile &fil, int len)
{
  close();
  if (!fil.getmode())
    return 0;
  f=&fil;
  int fmode=f->getmode();
  if ((fmode&canread)&&(fmode&canwrite)&&!(fmode&canseek))
    return 0;

  buffer=new char[len];
  if (!buffer)
    return 0;
  filelen=f->length();
  buflen=len;
  bufpos=0;
  bufread=0;
  filepos=0;
  dirty=0;
  fileseekpos=f->tell();
  filebufpos=fileseekpos;
  mode=fmode;
  return 1;
}

void binfilecache::close()
{
  if (mode)
  {
    invalidatebuf();
    delete buffer;
  }
  binfile::close();
}

long binfilecache::read(void *buf, long len)
{
  if (!(mode&canread))
    return 0;
  int len0=bufread-bufpos;
  if (len<len0)
    len0=len;
  len-=len0;
  memcpy(buf,buffer+bufpos,len0);
  bufpos+=len0;
  filepos=filebufpos+bufpos;
  if (!len)
    return len0;

  if (len<(buflen-bufread))
  {
    if (fileseekpos!=(filebufpos+bufread))
      f->seek(filebufpos+bufread);
    int l=f->read(buffer,buflen-bufread);
    if (l<len)
      len=l;
    memcpy((char*)buf+len0,buffer+bufread,len);
    bufpos+=len;
    bufread+=l;
  }
  else
  {
    invalidatebuf();
    if (len>=buflen)
    {
      if (fileseekpos!=filebufpos)
        f->seek(filebufpos);
      len=f->read((char*)buf+len0,len);
      filebufpos+=len;
    }
    else
    {
      if (fileseekpos!=filebufpos)
        f->seek(filebufpos);
      bufread=f->read(buffer,buflen);
      if (bufread<len)
        len=bufread;
      memcpy((char*)buf+len0,buffer,len);
      bufpos=len;
    }
  }
  fileseekpos=filebufpos+bufread;
  filepos=filebufpos+bufpos;
  return len0+len;
}

long binfilecache::write(const void *buf, long len)
{
  if (!(mode&canwrite))
    return 0;
  if (!(mode&canchsize))
    if (len>(filelen-filepos))
      len=filelen-filepos;
  int len0=buflen-bufpos;
  if (len<len0)
    len0=len;
  if (len0)
    dirty=1;
  len-=len0;
  memcpy(buffer+bufpos,buf,len0);
  bufpos+=len0;
  if (bufpos>bufread)
    bufread=bufpos;
  filepos=filebufpos+bufpos;
  if (filepos>filelen)
    filelen=filepos;
  if (!len)
    return len0;
  invalidatebuf();

  if (len>=buflen)
  {
    if (fileseekpos!=filebufpos)
      f->seek(filebufpos);
    len=f->write((const char *)buf+len0,len);
    filebufpos+=len;
    fileseekpos=filebufpos;
  }
  else
  {
    memcpy(buffer,(const char *)buf+len0,len);
    bufread=len;
    bufpos=len;
  }
  filepos=filebufpos+bufpos;
  if (filepos>filelen)
    filelen=filepos;
  return len0+len;
}

long binfilecache::seek(long pos)
{
  if (!(mode&canseek))
    return filepos;
  if (pos<0)
    pos=0;
  if (pos>filelen)
    pos=filelen;
  if ((pos>=filebufpos)&&(pos<=(filebufpos+bufread)))
    bufpos=pos-filebufpos;
  else
  {
    invalidatebuf();
    filebufpos=pos;
  }
  filepos=pos;
  return filepos;
}

long binfilecache::chsize(long pos)
{
  if (!(mode&canchsize))
    return filelen;
  invalidatebuf();
  filelen=f->chsize(pos);
  filepos=filebufpos=f->tell();
  return filelen;
}
