// binfile class (c) '96/97 Niklas Beisert

#ifndef __BINFILE_H
#define __BINFILE_H

#ifdef getc
  #undef getc
#endif
#ifdef putc
  #undef putc
#endif

class binfile
{
protected:
  int mode;
  long filepos;
  long filelen;

public:
  enum { canread=1, canwrite=2, canseek=4, canchsize=8 };
  int getmode() { return mode; }

  binfile();
  virtual ~binfile();

  virtual void close();

// abstract functions
  virtual long read(void *buf, long len);
  virtual long write(const void *buf, long len);
  virtual long seek(long pos);
  virtual long chsize(long pos);

// predefined functions
  virtual int eread(void *buf, long len);
  virtual int ewrite(const void *buf, long len);
  virtual long seekcur(long pos);
  virtual long seekend(long pos);
  virtual long tell();
  virtual long length();
  virtual int eof();

// basic types io
  virtual char getc();
  virtual short gets();
  virtual long getl();
  virtual binfile &putc(char c);
  virtual binfile &puts(short s);
  virtual binfile &putl(long l);
  virtual char egetc(int &stat);
  virtual short egets(int &stat);
  virtual long egetl(int &stat);
  virtual int eputc(char c);
  virtual int eputs(short s);
  virtual int eputl(long l);

// operators
  binfile &operator [](long pos) { seek(pos); return *this; }

// derived types io
  signed char getsc() { return getc(); }
  unsigned char getuc() { return getc(); }
  signed short getss() { return gets(); }
  unsigned short getus() { return gets(); }
  signed long getsl() { return getl(); }
  unsigned long getul() { return getl(); }
  binfile &putsc(signed char c) { return putc(c); }
  binfile &putuc(unsigned char c) { return putc(c); }
  binfile &putss(signed short s) { return puts(s); }
  binfile &putus(unsigned short s) { return puts(s); }
  binfile &putsl(signed long l) { return putl(l); }
  binfile &putul(unsigned long l) { return putl(l); }
  signed char egetsc(int &stat) { return egetc(stat); }
  unsigned char egetuc(int &stat) { return egetc(stat); }
  signed short egetss(int &stat) { return egets(stat); }
  unsigned short egetus(int &stat) { return egets(stat); }
  signed long egetsl(int &stat) { return egetl(stat); }
  unsigned long egetul(int &stat) { return egetl(stat); }
  int eputsc(signed char c) { return eputc(c); }
  int eputuc(unsigned char c) { return eputc(c); }
  int eputss(signed short s) { return eputs(s); }
  int eputus(unsigned short s) { return eputs(s); }
  int eputsl(signed long l) { return eputl(l); }
  int eputul(unsigned long l) { return eputl(l); }
};

inline long read(binfile &f, void *buf, long len) { return f.read(buf, len); }
inline long write(binfile &f, void *buf, long len) { return f.write(buf, len); }
inline int eread(binfile &f, void *buf, long len) { return f.eread(buf, len); }
inline int ewrite(binfile &f, void *buf, long len) { return f.ewrite(buf, len); }
inline long seek(binfile &f, long pos) { return f.seek(pos); }
inline long seekcur(binfile &f, long pos) { return f.seekcur(pos); }
inline long seekend(binfile &f, long pos) { return f.seekend(pos); }
inline long chsize(binfile &f, long pos) { return f.chsize(pos); }
inline long tell(binfile &f) { return f.tell(); }
inline long length(binfile &f) { return f.length(); }
inline int eof(binfile &f) { return f.eof(); }

inline char getc(binfile &f) { return f.getc(); }
inline signed char getsc(binfile &f) { return f.getsc(); }
inline unsigned char getuc(binfile &f) { return f.getuc(); }
inline short gets(binfile &f) { return f.gets(); }
inline signed short getss(binfile &f) { return f.getss(); }
inline unsigned short getus(binfile &f) { return f.getus(); }
inline long getl(binfile &f) { return f.getl(); }
inline signed long getsl(binfile &f) { return f.getsl(); }
inline unsigned long getul(binfile &f) { return f.getul(); }
inline binfile &putc(binfile &f, char c) { return f.putc(c); }
inline binfile &putsc(binfile &f, signed char c) { return f.putsc(c); }
inline binfile &putuc(binfile &f, unsigned char c) { return f.putuc(c); }
inline binfile &puts(binfile &f, short s) { return f.puts(s); }
inline binfile &putss(binfile &f, signed short s) { return f.putss(s); }
inline binfile &putus(binfile &f, unsigned short s) { return f.putus(s); }
inline binfile &putl(binfile &f, long l) { return f.putl(l); }
inline binfile &putsl(binfile &f, signed long l) { return f.putsl(l); }
inline binfile &putul(binfile &f, unsigned long l) { return f.putul(l); }
inline char egetc(binfile &f, int &stat) { return f.egetc(stat); }
inline signed char egetsc(binfile &f, int &stat) { return f.egetsc(stat); }
inline unsigned char egetuc(binfile &f, int &stat) { return f.egetuc(stat); }
inline short egets(binfile &f, int &stat) { return f.egets(stat); }
inline signed short egetss(binfile &f, int &stat) { return f.egetss(stat); }
inline unsigned short egetus(binfile &f, int &stat) { return f.egetus(stat); }
inline long egetl(binfile &f, int &stat) { return f.egetl(stat); }
inline signed long egetsl(binfile &f, int &stat) { return f.egetsl(stat); }
inline unsigned long egetul(binfile &f, int &stat) { return f.egetul(stat); }
inline int eputc(binfile &f, char c) { return f.eputc(c); }
inline int eputsc(binfile &f, signed char c) { return f.eputsc(c); }
inline int eputuc(binfile &f, unsigned char c) { return f.eputuc(c); }
inline int eputs(binfile &f, short s) { return f.eputs(s); }
inline int eputss(binfile &f, signed short s) { return f.eputss(s); }
inline int eputus(binfile &f, unsigned short s) { return f.eputus(s); }
inline int eputl(binfile &f, long l) { return f.eputl(l); }
inline int eputsl(binfile &f, signed long l) { return f.eputsl(l); }
inline int eputul(binfile &f, unsigned long l) { return f.eputul(l); }

#endif
