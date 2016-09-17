// binfile class (c) '96/97 Niklas Beisert

#ifndef __CBINFILE_H
#define __CBINFILE_H

#include "binfile.h"

class binfilecache : public binfile
{
protected:
  binfile *f;
  char *buffer;
  int buflen;
  int bufpos;
  int bufread;
  long filebufpos;
  int dirty;
  long fileseekpos;
  void invalidatebuf();

public:
  binfilecache();

  int open(binfile &fil, int len);
  virtual void close();
  virtual long read(void *buf, long len);
  virtual long write(const void *buf, long len);
  virtual long seek(long pos);
  virtual long chsize(long pos);
};

#endif
