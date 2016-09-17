#include <string.h>
#include <conio.h>
#include <i86.h>
#include "imsrtns.h"

static void (__far __interrupt *tmOldTimer)();
static void (*tmTimerRoutine)();
static unsigned long tmTimerRate;
static unsigned long tmTicker;
static unsigned long tmIntCount;
static char __far *stack;
static unsigned long stacksize;
static unsigned char stackused;
static void __far *oldssesp;

static void far *getvect(unsigned char intno)
{
  REGS r;
  SREGS sr;
  r.h.ah=0x35;
  r.h.al=intno;
  sr.ds=sr.es=0;
  int386x(0x21, &r, &r, &sr);
  return MK_FP(sr.es, r.x.ebx);
}

static void setvect(unsigned char intno, void far *vect)
{
  REGS r;
  SREGS sr;
  r.h.ah=0x25;
  r.h.al=intno;
  r.x.edx=FP_OFF(vect);
  sr.ds=FP_SEG(vect);
  sr.es=0;
  int386x(0x21, &r, &r, &sr);
}


void stackcall(void *);
#pragma aux stackcall parm [eax] = \
  "mov word ptr oldssesp+4,ss" \
  "mov dword ptr oldssesp+0,esp" \
  "lss esp,stack" \
  "sti" \
  "call eax" \
  "cli" \
  "lss esp,oldssesp"
void loades();
#pragma aux loades = "push ds" "pop es"

static void __interrupt tmTimerHandler()
{
  loades();

  outp(0x43,0x34);
  outp(0x40,tmTimerRate);
  outp(0x40,tmTimerRate>>8);

  tmTicker+=tmTimerRate;

  tmIntCount+=tmTimerRate;
  if (tmIntCount&0xFFFF0000)
  {
    tmIntCount&=0xFFFF;
    tmOldTimer();
  }

  outp(0x20,0x20);

  if (stackused)
    return;
  stackused++;
  stackcall(tmTimerRoutine);
  stackused--;
}

int tmInit(void (*rout)(), int timerval, int stk)
{
  stacksize=stk;
  stack=new char [stacksize];
  if (!stack)
    return 0;
  stack+=stacksize;

  tmTimerRoutine=rout;
  tmIntCount=0;
  tmTicker=-timerval;
  tmTimerRate=timerval;

  tmOldTimer=(void (__far __interrupt *)())getvect(0x08);
  setvect(0x08, tmTimerHandler);

  outp(0x43, 0x34);
  outp(0x40, tmTimerRate);
  outp(0x40, (tmTimerRate>>8));

  return 1;
}

void tmSetNewRate(int val)
{
  tmTimerRate=val;
  outp(0x43,0x34);
  outp(0x40,tmTimerRate);
  outp(0x40,tmTimerRate>>8);
}

int tmGetTicker()
{
  return tmTicker;
}

void tmSetTicker(int t)
{
  tmTicker+=t-tmGetTicker();
}

int tmGetTimer()
{
  unsigned short f=_disableint();

  unsigned long tm=tmTimerRate+tmTicker;
  outp(0x43,0);
  tm-=inp(0x40);
  tm-=inp(0x40)<<8;

  _restoreint(f);

  return umulshr16(tm, 3600);
}

void tmClose()
{
  setvect(0x08, tmOldTimer);
  outp(0x43, 0x34);
  outp(0x40, 0x00);
  outp(0x40, 0x00);
  delete (char near *)(stack-stacksize);
}
