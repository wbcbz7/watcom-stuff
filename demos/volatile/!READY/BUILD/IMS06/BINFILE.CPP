// binfile class (c) '96/97 Niklas Beisert

#include "binfile.h"

binfile::binfile()
{
  binfile::close();
}

binfile::~binfile()
{
  close();
}

void binfile::close()
{
  filepos=filelen=0;
  mode=0;
}

long binfile::read(void *, long)
{
  return 0;
}

long binfile::write(const void *, long)
{
  return 0;
}

long binfile::seek(long)
{
  return filepos;
}

long binfile::chsize(long)
{
  return filelen;
}

long binfile::tell()
{
  return filepos;
}

long binfile::length()
{
  return filelen;
}

int binfile::eof()
{
  return filepos==filelen;
}

long binfile::seekcur(long pos)
{
  return seek(filepos+pos);
}

long binfile::seekend(long pos)
{
  return seek(filelen+pos);
}

int binfile::eread(void *buf, long len)
{
  return read(buf,len)==len;
}

int binfile::ewrite(const void *buf, long len)
{
  return write(buf,len)==len;
}

char binfile::getc()
{
  char c=0;
  read(&c, 1);
  return c;
}

short binfile::gets()
{
  short s=0;
  read(&s, 2);
  return s;
}

long binfile::getl()
{
  long l=0;
  read(&l, 4);
  return l;
}

binfile &binfile::putc(char c)
{
  write(&c, 1);
  return *this;
}

binfile &binfile::puts(short s)
{
  write(&s, 2);
  return *this;
}

binfile &binfile::putl(long l)
{
  write(&l, 4);
  return *this;
}

char binfile::egetc(int &stat)
{
  char c=0;
  stat=eread(&c, 1);
  return c;
}

short binfile::egets(int &stat)
{
  short s=0;
  stat=eread(&s, 2);
  return s;
}

long binfile::egetl(int &stat)
{
  long l=0;
  stat=eread(&l, 4);
  return l;
}

int binfile::eputc(char c)
{
  return ewrite(&c, 1);
}

int binfile::eputs(short s)
{
  return ewrite(&s, 2);
}

int binfile::eputl(long l)
{
  return ewrite(&l, 4);
}
