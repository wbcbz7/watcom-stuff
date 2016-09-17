// binfile class (c) '96/97 Niklas Beisert

#include <io.h>
#include <fcntl.h>
#include <sys\stat.h>
#include "binfstd.h"

sbinfile::sbinfile()
{
}

sbinfile::~sbinfile()
{
}

int sbinfile::open(const char *name, int type)
{
  close();
  int fmode=canread|canseek;
  switch (type)
  {
  case openro: handle=::open(name, O_RDONLY|O_BINARY); break;
  case openrw: handle=::open(name, O_RDWR|O_BINARY); fmode|=canwrite|canchsize; break;
  case opencr: handle=::open(name, O_RDWR|O_BINARY|O_CREAT|O_TRUNC, S_IREAD|S_IWRITE); fmode|=canwrite|canchsize; break;
  case opencrn: handle=::open(name, O_RDWR|O_BINARY|O_CREAT|O_TRUNC|O_EXCL, S_IREAD|S_IWRITE); fmode|=canwrite|canchsize; break;
  default: return 0;
  }
  if (handle<0)
    return 0;
  filepos=0;
  filelen=::filelength(handle);
  mode=fmode;
  return 1;
}

void sbinfile::close()
{
  if (mode)
    ::close(handle);
  binfile::close();
}

long sbinfile::read(void *buf, long len)
{
  if (!(mode&canread))
    return 0;
  len=::read(handle, buf, len);
  filepos+=len;
  return len;
}

long sbinfile::write(const void *buf, long len)
{
  if (!(mode&canwrite))
    return 0;
  if (!(mode&canchsize))
    if (len>(filelen-filepos))
      len=filelen-filepos;

  len=::write(handle, buf, len);
  filepos+=len;
  if (filepos>filelen)
    filelen=filepos;
  return len;
}

long sbinfile::seek(long pos)
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
  ::lseek(handle, pos, SEEK_SET);
  return filepos;
}

long sbinfile::chsize(long pos)
{
  if (!(mode&canchsize))
    return filelen;
  ::chsize(handle, pos);
  filelen=::filelength(handle);
  filepos=::tell(handle);
  return filelen;
}
