// (c) 1994-1997 Niklas Beisert, this file is part of "Indoor Music System".
// you must not read, modify, print, compile or copy this file or parts of
// it unless you have accepted the license in the accompanying file IMS.TXT.
// you must also agree to the license at the bottom of this file to do so.

#include <stdlib.h>
#include <string.h>
#include <conio.h>
#include "dma.h"
#include "imsdev.h"
#include "player.h"
#include "imsrtns.h"

extern "C" extern sounddevice plrProAudioSpectrum;

struct mvstate
{
  unsigned char _sysspkrtmr,_systmrctlr,_sysspkrreg,_joystick;
  unsigned char _lfmaddr,_lfmdata,_rfmaddr,_rfmdata,_dfmaddr,_dfmdata,_RESRVD1;
  unsigned char _paudiomixr,_audiomixr,_intrctlrst,_audiofilt,_intrctlr;
  unsigned char _pcmdata,_RESRVD2,_crosschannel,_RESRVD3;
  unsigned short _samplerate,_samplecnt,_spkrtmr;
  unsigned char _tmrctlr,_mdirqvect,_mdsysctlr,_mdsysstat,_mdirqclr;
  unsigned char _mdgroup1,_mdgroup2,_mdgroup3,_mdgroup4;
};

unsigned short findmvdrv();
#pragma aux findmvdrv modify [ax cx dx] value [bx] = "mov ax,0bc00h" "mov bx,3F3Fh" "xor cx,cx" "xor dx,dx" "int 2fh" "xor bx,cx" "xor bx,dx"

static unsigned short pasPort;
static unsigned char pasDMA;
static unsigned char pas16;
static unsigned char pasStereo;
static unsigned char pas16Bit;
static unsigned short pasSpeed;
static mvstate localtab;
static mvstate *tab;
static long pasBPS;
static long playpos;
static long buflen;
static __segment dmabufsel;

static unsigned char pasinp(unsigned short r)
{
  return inp(pasPort^(r^0x388));
}

static void pasoutp(unsigned short r, unsigned char v)
{
  outp(pasPort^(r^0x388), v);
}

static char pasTestPort(unsigned short p)
{
  pasPort=p;

  unsigned char t,t2;

  t=pasinp(0xB8B);
  if (t==-1)
    return 0;
  pasoutp(0xB8B, t^0xE0);
  for (t2=0; t2<100; t2++);
  t2=pasinp(0xB8B);
  pasoutp(0xB8B, t);

  if (t!=t2)
    return 0;

  pas16=0;
  if (t&0xE0)
  {
    t=pasinp(0xEF8B);
    if (!(t&4))
      return 0;
    if (t&8)
      pas16=1;
  }

  return 1;
}

static void pasSetOptions(int rate, int opt)
{
  pasStereo=!!(opt&PLR_STEREO);
  pas16Bit=!!(opt&PLR_16BIT)&&pas16;
  if (pas16Bit)
    opt|=PLR_SIGNEDOUT;

  if (rate>44100)
    rate=44100;
  if (rate<2000)
    rate=2000;

  pasSpeed=(1193180/rate)>>pasStereo;
  if (pasSpeed>255)
    pasSpeed=255;

  rate=(1193180/pasSpeed)>>pasStereo;

  pasBPS=rate<<(pasStereo+pas16Bit);

  plrRate=rate;
  plrOpt=opt;
}

static void advance(int pos)
{
  playpos+=(pos-(playpos%buflen)+buflen)%buflen;
}

static long gettimer()
{
  return imuldiv(playpos+(dmaGetBufPos()-playpos%buflen+buflen)%buflen, 65536, pasBPS);
}

static int pasPlay(void *&buf, int &len)
{
  buf=(unsigned char*)dmaAlloc(len, dmabufsel);
  if (!buf)
    return 0;
  memsetd(buf, (plrOpt&PLR_SIGNEDOUT)?0:(plrOpt&PLR_16BIT)?0x80008000:0x80808080, len>>2);

  pasoutp(0xB89, 0);

  tab->_samplerate=pasSpeed;

//  cli
  pasoutp(0x138B, tab->_tmrctlr=0x36);

  pasoutp(0x1388, tab->_samplerate);
  pasoutp(0x1388, (tab->_samplerate>>8));
//  sti

  pasoutp(0xF8A, tab->_crosschannel|=0x80);

  dmaStart(pasDMA, buf, len, 0x58);

  if (pas16Bit)
    pasoutp(0x8389, (pasinp(0x8389)&~0x08)|0x04);

  tab->_crosschannel=(tab->_crosschannel&0x8F)|(pasStereo?0x50:0x70);
  pasoutp(0xF8A, tab->_crosschannel&~0x40);
  short i;
  for (i=0; i<100; i++);
  pasoutp(0xF8A, tab->_crosschannel);

  pasoutp(0xB8A, tab->_audiofilt|=0xC0);

  pasoutp(0xF8A, tab->_crosschannel|=0x80);

  buflen=len;
  playpos=-buflen;

  plrGetBufPos=dmaGetBufPos;
  plrGetPlayPos=dmaGetBufPos;
  plrAdvanceTo=advance;
  plrGetTimer=gettimer;

  return 1;
}

static void pasStop()
{
//  cli
  pasoutp(0xB8A, tab->_audiofilt&=~0xC0);
//  sti

  if (pas16Bit)
    pasoutp(0x8389, pasinp(0x8389)&~12);

  pasoutp(0xB8B, tab->_intrctlr=pasinp(0xB8B)&~0x08);

  pasoutp(0xF8A, tab->_crosschannel=(tab->_crosschannel&~0xC0)|0x10);

  dmaStop();
  dmaFree(dmabufsel);
}

mvstate *gettab();
#pragma aux gettab modify [ax cx] value [ecx] = "mov ax,0bc02h" "int 2fh" "xor ecx,ecx" "cmp ax,'MV'" "jne err" "movzx ecx,bx" "movzx edx,dx" "shl edx,4" "add ecx,edx" "err:"

static int pasInit(const deviceinfo &card)
{
  pasPort=card.port;
  pasDMA=card.dma;

  if (!pasTestPort(pasPort))
    return 0;

  tab=&localtab;

  tab->_audiomixr=0x31;
  tab->_crosschannel=0x09;

  if (findmvdrv()==0x4D56)
  {
    tab=gettab();
    if (!tab)
      tab=&localtab;
  }

  plrSetOptions=pasSetOptions;
  plrPlay=pasPlay;
  plrStop=pasStop;
  return 1;
}

static void pasClose()
{
  plrPlay=0;
}

unsigned char getdma();
#pragma aux getdma modify [ax cx] value [bl] = "mov ax,0bc04h" "xor cx,cx" "xor dx,dx" "int 2fh"

static int pasDetect(deviceinfo &card)
{
  int port=card.port;
  int dma=card.dma;
  if (port==-1)
  {
    if (findmvdrv()!=0x4D56)
      return 0;

    int i;
    unsigned short ports[]={0x388, 0x384, 0x38C, 0x288};
    for (i=0; i<4; i++)
      if (pasTestPort(ports[i]))
        break;
    if (i==4)
      return 0;
    port=ports[i];
  }
  else
    if (!pasTestPort(port))
      return 0;

  if (dma==-1)
  {
    if (findmvdrv()!=0x4D56)
      return 0;
    dma=getdma();
  }

  card.dev=&plrProAudioSpectrum;
  card.port=port;
  card.dma=dma;
  card.port2=-1;
  card.irq=-1;
  card.irq2=-1;
  card.dma2=-1;
  card.subtype=-1;
  card.mem=0;
  card.chan=2;
  return 1;
}

extern "C" sounddevice plrProAudioSpectrum={SS_PLAYER, "Pro Audio Spectrum", pasDetect, pasInit, pasClose};

/*
ported from MIDAS: (actually an earlier version, but who cares)

;*      PAS.ASM
;*
;* Pro Audio Spectrum Sound Device
;*
;* $Id: pas.asm,v 1.3 1997/01/16 18:41:59 pekangas Exp $
;*
;* Copyright 1996,1997 Housemarque Inc.
;*
;* This file is part of the MIDAS Sound System, and may only be
;* used, modified and distributed under the terms of the MIDAS
;* Sound System license, LICENSE.TXT. By continuing to use,
;* modify or distribute this file you indicate that you have
;* read the license and understand and accept it fully.
;*

;* NOTE! A lot of this code is ripped more or less directly from the PAS
;* SDK and might therefore seem messy.
;* I really do not understand some parts of this code... Perhaps I'll clear
;* it up some day when I have time.
;* (PK)

;* Update: The above probably won't ever happen though. The statement has been
;* there over a year now, through three major revisions of the Sound Device..

LICENSE.TXT

                 MIDAS Sound System LICENSE
                 --------------------------
    Copyright 1996,1997 Housemarque Inc.

0. Throughout this license, "the original MIDAS Sound System archive"
or "the original archive" refers to the set of files originally
distributed by the authors (Petteri Kangaslampi and Jarno Paananen) as
the MIDAS Sound System.

"You" refers to the licensee, or person using the MIDAS Sound System.
"Using" the MIDAS Sound System includes compiling the MIDAS Sound
System source code and linking it to other parts of MIDAS Sound System
or your own code to form a "program". This program is referred to as "a
program using the MIDAS Sound System".

This license applies to all files distributed in the original MIDAS
Sound System archive, including all source code, binaries and
documentation, unless otherwise stated in the file in its original,
unmodified form as distributed in the original archive. If you are
unsure whether or not a particular file is covered by this license, you
must contact us to verify this.

MIDAS SOUND SYSTEM IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND,
EITHER EXPRESSED OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT
WILL ANY OF THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY DAMAGES
CAUSED BY THE USE OR INABILITY TO USE, OF MIDAS SOUND SYSTEM.

As you have not signed this license, you are not required to accept it.
However, as MIDAS Sound System is copyrighted material, only this
license grants you the right to use, distribute and modify it.
Therefore, by using, distributing or modifying MIDAS Sound System, you
indicate that you understand and accept all the terms of this license.


1. MIDAS Sound System may freely be copied and distributed in its
original, unmodified form as long as no fee is charged for the MIDAS
Sound System itself. A fee, not exceeding 10USD, may be charged for the
physical act of copying, as long as it is clearly stated, that the
MIDAS Sound System itself is free and may always be distributed free of
charge. When ever the MIDAS Sound System or portions of it are
distributed, this license must be included in unmodified form, and all
files must contain their original copyright notices. If only a portion
of a file is being distributed, the appropriate copyright notice must
be copied into it from the beginning of the file, and this license must
still be included in unmodified form.


2. The MIDAS Sound System files may freely be modified, and these
modified files distributed under the terms of Section 1 above, provided
that the following conditions are met:

        2.1. All the modified files must carry their original
             copyright notice, in addition to a list of modifications
             made and their respective authors.

        2.2. Modified versions files belonging under this
             license still do so, no matter what kind of modifications
             have been made.


3. MIDAS Sound System may not be used in commercial programs, including
shareware. No program using MIDAS Sound System may be sold for any
price, and neither may any fee be charged for the right to use the
program. Programs using MIDAS Sound System may always be freely
distributed if no fee is charged for the distribution and may freely be
used by anyone. The Program and its documentation must clearly state
that the program uses MIDAS Sound System and that it may freely be used
and distributed under the terms of this Section 3.


4. MIDAS Sound System, or portions of it, may not be included as parts
of a commercial software package, in source code, object module or any
other form. Neither may MIDAS Sound System, or portions of it, be
printed in a commercial publication, including, but not limited to,
magazines and books. Any software package or publication including
portions of MIDAS Sound System may freely be used and distributed, as
stated in Section 3.

MIDAS Sound System may, however, be included in free software packages
as long as the whole software package can freely be distributed under
the terms of section 3 and all conditions of this license are met.
MIDAS Sound System, or portions of it, may also be printed for personal
or educational usage, provided that no attempt is made to restrict the
copying and usage of any parts of the printout or the publication in
which portions of MIDAS are included.
*/
