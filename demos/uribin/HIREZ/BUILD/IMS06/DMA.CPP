#include <stdlib.h>
#include <conio.h>
#include <i86.h>
#include "imsrtns.h"

static unsigned char dmaCh;
static int dmaLen;
static unsigned char *dmaPort;
static unsigned char dmaPorts[8][6]=
  {{0x00, 0x01, 0x0A, 0x0B, 0x0C, 0x87},
   {0x02, 0x03, 0x0A, 0x0B, 0x0C, 0x83},
   {0x04, 0x05, 0x0A, 0x0B, 0x0C, 0x81},
   {0x06, 0x07, 0x0A, 0x0B, 0x0C, 0x82},
   {0xC0, 0xC2, 0xD4, 0xD6, 0xD8, 0x8F},
   {0xC4, 0xC6, 0xD4, 0xD6, 0xD8, 0x8B},
   {0xC8, 0xCA, 0xD4, 0xD6, 0xD8, 0x89},
   {0xCC, 0xCE, 0xD4, 0xD6, 0xD8, 0x8A}};

static void *dosmalloc(unsigned long len, void __far16 *&rmptr, __segment &pmsel)
{
  len=(len+15)>>4;
  REGS r;
  r.w.ax=0x100;
  r.w.bx=len;
  int386(0x31, &r, &r);
  if (r.x.cflag)
    return 0;
  pmsel=r.w.dx;
  rmptr=(void __far16 *)((unsigned long)r.w.ax<<16);
  return (void *)((unsigned long)r.w.ax<<4);
}

static void dosfree(__segment pmsel)
{
  REGS r;
  r.w.ax=0x101;
  r.w.dx=pmsel;
  int386(0x31, &r, &r);
}

void dmaStart(int ch, void *buf, int buflen, int autoinit)
{
  dmaCh=ch&7;
  dmaPort=dmaPorts[dmaCh];
  unsigned long realadr=(unsigned long)buf;
  if (dmaCh&4)
  {
    realadr>>=1;
    buflen=(buflen+1)>>1;
  }
  dmaLen=buflen-1;
  int is=_disableint();
  outp(dmaPort[2],dmaCh|4);
//  outp(dmaPort[3],(autoinit?0x18:0x08)|(ch&3));
  outp(dmaPort[3],autoinit|(ch&3));
  outp(dmaPort[4],0);
  outp(dmaPort[0],realadr);
  outp(dmaPort[0],(realadr>>8));
  outp(dmaPort[5],(((unsigned long)buf)>>16));
  outp(dmaPort[5],(((unsigned long)buf)>>16));
  outp(dmaPort[1],dmaLen);
  outp(dmaPort[1],(dmaLen>>8));
  outp(dmaPort[2],dmaCh&3);
  _restoreint(is);
}

void dmaStop()
{
//  int is=_disableint();
  outp(dmaPort[4],0);
  outp(dmaPort[2],dmaCh|4);
//  outp(dmaPort[2],dmaCh&3);
//  _restoreint(is);
}

int dmaGetBufPos()
{
  unsigned int a,b;
  int is=_disableint();
  while(1)
  {
    outp(dmaPort[4], 0xFF);
    a=inp(dmaPort[1]);
    a+=inp(dmaPort[1])<<8;
    b=inp(dmaPort[1]);
    b+=inp(dmaPort[1])<<8;
    if (abs(a-b)<=64)
      break;
  }
  _restoreint(is);
  int p=dmaLen-b;
  if (p<0)
    return 0;
  if (p>=dmaLen)
    return 0;
  if (dmaCh&4)
    p<<=1;
  return p;
}

void *dmaAlloc(int &buflen, __segment &pmsel)
{
  if (buflen>0x20000)
    buflen=0x20000;
  buflen=(buflen+15)&~15;
  void __far16 *rmptr;
  void *ptr=dosmalloc(buflen, rmptr, pmsel);
  if (!ptr)
    return 0;
  unsigned long a=0x10000-((unsigned long)ptr&0xFFFF);
  if (a<buflen)
    if (a<(buflen>>1))
    {
      buflen-=a;
      *(unsigned long*)&ptr+=a; // gotcha!
    }
    else
      buflen=a;
  if (buflen>0xFF00)
    buflen=0xFF00;
  return ptr;
}

void dmaFree(__segment pmseg)
{
  dosfree(pmseg);
}
