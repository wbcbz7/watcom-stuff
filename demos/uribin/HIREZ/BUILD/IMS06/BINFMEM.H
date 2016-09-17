// binfile class (c) '96/97 Niklas Beisert

#ifndef __MBINFILE_H
#define __MBINFILE_H

#include "binfile.h"

class mbinfile : public binfile
{
protected:
  char *filebuf;

  long *flen;
  void **fbuf;
  long fbuflen;
  int fleninc;
  int freemem;

public:
  mbinfile();

  enum { openro=0, openrw=1, openfree=2 };

  int open(void *buf, long len, int type);
  int opencs(void *&buf, long &len, int inc);
  virtual void close();

  virtual long read(void *buf, long len);
  virtual long write(const void *buf, long len);
  virtual long seek(long pos);
  virtual long chsize(long pos);
};

#endif
