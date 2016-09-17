// binfile class (c) '96/97 Niklas Beisert

#ifndef __BFILEARC_H
#define __BFILEARC_H

#include "binfile.h"

class abinfile : public binfile
{
protected:
  binfile *f;
  long fofs;

public:
  abinfile();

  int open(binfile &fil, long ofs, long len);

  virtual long read(void *buf, long len);
  virtual long write(const void *buf, long len);
  virtual long seek(long pos);
};

#endif
