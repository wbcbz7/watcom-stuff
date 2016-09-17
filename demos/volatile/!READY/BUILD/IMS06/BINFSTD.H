// binfile class (c) '96/97 Niklas Beisert

#ifndef __SBINFILE_H
#define __SBINFILE_H

#include "binfile.h"

class sbinfile : public binfile
{
protected:
  int handle;

public:
  sbinfile();
  ~sbinfile();

  enum { openro=0, openrw, opencr, opencrn };
  int open(const char *name, int type);

  virtual void close();
  virtual long read(void *buf, long len);
  virtual long write(const void *buf, long len);
  virtual long seek(long pos);
  virtual long chsize(long pos);
};

#endif
