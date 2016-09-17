// (c) 1994-1997 Niklas Beisert, this file is part of "Indoor Music System".
// you must not read, modify, print, compile or copy this file or parts of
// it unless you have accepted the license in the accompanying file IMS.TXT.

#include <string.h>
#include <stdlib.h>
#include "binfile.h"

#include "mcp.h"
#include "gmdplay.h"
#include "err.h"

static inline void putcmd(unsigned char *&p, unsigned char c, unsigned char d)
{
  *p++=c;
  *p++=d;
}

extern "C" int mpLoadPTM(gmdmodule &m, binfile &file)
{
  mpReset(m);

  struct
  {
    char name[28];
    unsigned char end;
    unsigned short type;
    unsigned char d1;
    unsigned short orders,ins,pats,chan,flags,d2;
    char magic[4];
    char d3[16];
    unsigned char channels[32];
  } hdr;

  file.read(&hdr, sizeof(hdr));
  if (memcmp(hdr.magic, "PTMF", 4))
    return errFormSig;

  memcpy(m.name, hdr.name, 28);
  m.name[28]=0;

  short t;
  m.channum=hdr.chan;
  m.modsampnum=m.sampnum=m.instnum=hdr.ins;
  m.patnum=hdr.orders;
  m.ordnum=hdr.orders;
  m.endord=m.patnum;
  m.tracknum=hdr.pats*(m.channum+1)+1;
  m.options=MOD_S3M;
  m.loopord=0;

  unsigned char orders[256];

  file.read(orders, 256);

  if (!m.patnum)
    return errFormMiss;

  unsigned short patpara[129];
  file.read(patpara, 256);

  if (!mpAllocInstruments(m, m.instnum)||!mpAllocTracks(m, m.tracknum)||!mpAllocPatterns(m, m.patnum)||!mpAllocSamples(m, m.sampnum)||!mpAllocModSamples(m, m.modsampnum)||!mpAllocOrders(m, m.ordnum))
    return errAllocMem;

  int i,j;

  for (i=0; i<m.ordnum; i++)
    m.orders[i]=i;

  gmdpattern *pp;
  for (pp=m.patterns, t=0; t<m.patnum; pp++, t++)
  {
    pp->patlen=64;
    if ((orders[t]!=255)&&(orders[t]<hdr.pats))
    {
      for (i=0; i<m.channum; i++)
        pp->tracks[i]=orders[t]*(m.channum+1)+i;
      pp->gtrack=orders[t]*(m.channum+1)+m.channum;
    }
    else
    {
      for (i=0; i<m.channum; i++)
        pp->tracks[i]=m.tracknum-1;
      pp->gtrack=m.tracknum-1;
    }
  }

  unsigned long inspos[256];

  for (i=0; i<m.instnum; i++)
  {
    struct
    {
      unsigned char type; // 0:not used, 1:sample, 2:opl, 3:midi 4:loop, 8:pingpong, 10:16bit
      char dosname[12];
      unsigned char volume;
      unsigned short samprate;
      unsigned short d1;
      unsigned long offset;
      unsigned long length;
      unsigned long loopstart;
      unsigned long loopend;
      unsigned long d2;
      unsigned long d3;
      unsigned long d4;
      unsigned char d5;
      unsigned char d7;
      char name[28];
      long magic;
    } sins;

    file.read(&sins, sizeof(sins));
    if ((sins.magic!=0x534D5450)&&(sins.magic!=0))
      return errFormStruc;
    if (!i)
      patpara[hdr.pats]=sins.offset>>4;
    inspos[i]=sins.offset;

    if (sins.type&0x10)
    {
      sins.length>>=1;
      sins.loopstart>>=1;
      sins.loopend>>=1;
    }
    gmdinstrument &ip=m.instruments[i];
    gmdsample &sp=m.modsamples[i];
    sampleinfo &sip=m.samples[i];

    memcpy(ip.name, sins.name, 28);
    ip.name[28]=0;
    if (!(sins.type&3))
      continue;
    if ((sins.type&3)!=1)
      continue;

    for (j=0; j<128; j++)
      ip.samples[j]=i;

    memcpy(sp.name, sins.dosname, 12);
    sp.name[13]=0;
    sp.handle=i;
    sp.normnote=-mcpGetNote8363(sins.samprate);
    sp.stdvol=(sins.volume>0x3F)?0xFF:(sins.volume<<2);
    sp.stdpan=-1;
    sp.opt=(sins.type&0x10)?MP_OFFSETDIV2:0;

    sip.length=sins.length;
    sip.loopstart=sins.loopstart;
    sip.loopend=sins.loopend;
    sip.samprate=8363;
    sip.type=((sins.type&4)?mcpSampLoop:0)|((sins.type&8)?mcpSampBiDi:0)|((sins.type&0x10)?mcpSamp16Bit:0);
  }

  unsigned short bufSize=1024;
  unsigned char *buffer=new unsigned char[bufSize];
  unsigned char *temptrack=new unsigned char [2000];
  if (!temptrack||!buffer)
    return errAllocMem;

  for (t=0; t<hdr.pats; t++)
  {
    file.seek(patpara[t]*16);
    unsigned short patSize=(patpara[t+1]-patpara[t])*16;
    if (patSize>bufSize)
    {
      bufSize=patSize;
      delete buffer;
      buffer=new unsigned char[bufSize];
      if (!buffer)
        return errAllocMem;
    }
    file.read(buffer, patSize);

    for (j=0; j<m.channum; j++)
    {
      unsigned char *bp=buffer;
      unsigned char *tp=temptrack;

      unsigned char *cp=tp+2;
      char setorgpan=t==orders[0];
      char setorgvwav=t==orders[0];
      char setorgpwav=t==orders[0];
      short row=0;

      while (row<64)
      {
	unsigned char c=*bp++;
	if (!c)
        {
          if (setorgpan)
          {
            putcmd(cp, cmdPlayNote|cmdPlayPan, hdr.channels[j]*0x11);
            putcmd(cp, cmdVolVibratoSetWave, 0x10);
            putcmd(cp, cmdPitchVibratoSetWave, 0x10);
            setorgpan=0;
          }

          if (cp!=(tp+2))
          {
            tp[0]=row;
            tp[1]=cp-tp-2;
            tp=cp;
            cp=tp+2;
          }
          row++;
          continue;
        }
	if ((c&0x1F)!=j)
        {
          bp+=((c&0x20)>>4)+((c&0x40)>>5)+((c&0x80)>>7);
          continue;
        }
        signed short nte=-1;
        signed short ins=-1;
        signed short vol=-1;
        unsigned char command=0;
        unsigned char data=0;
        signed int pan=-1;
        unsigned char pansrnd=0;

        if (!row&&(t==orders[0]))
        {
          setorgpan=0;
          pan=hdr.channels[j]*0x11;
          putcmd(cp, cmdVolVibratoSetWave, 0x10);
          putcmd(cp, cmdPitchVibratoSetWave, 0x10);
        }

        if (c&0x20)
        {
          nte=*bp++;
          ins=*bp++-1;
          if ((nte<=120)||!nte)
            nte=nte+11;
          else
          {
            if (nte==254)
              putcmd(cp, cmdNoteCut, 0);
            nte=-1;
          }
        }
        if (c&0x40)
        {
          command=*bp++;
          data=*bp++;
        }
        if (c&0x80)
        {
          vol=*bp++;
          vol=(vol>0x3F)?0xFF:(vol<<2);
        }

        if (command==0xC)
          vol=(data>0x3F)?0xFF:(data<<2);

        if ((command==0xE)&&((data>>4)==0x8))
          pan=(data&0xF)*0x11;

        if (((command==0x3)||(command==0x5))&&(nte!=-1))
          nte|=128;

        if ((ins!=-1)||(nte!=-1)||(vol!=-1)||(pan!=-1))
        {
          unsigned char &act=*cp;
          *cp++=cmdPlayNote;
          if (ins!=-1)
          {
            act|=cmdPlayIns;
            *cp++=ins;
          }
          if (nte!=-1)
          {
            act|=cmdPlayNte;
            *cp++=nte;
          }
          if (vol!=-1)
          {
            act|=cmdPlayVol;
            *cp++=vol;
          }
          if (pan!=-1)
          {
            act|=cmdPlayPan;
            *cp++=pan;
          }
          if ((command==0xE)&&((data>>4)==0xD))
          {
            act|=cmdPlayDelay;
            *cp++=data&0xF;
          }
        }

//        if (pansrnd)
//          putcmd(cp, cmdPanSurround, 0);

        switch (command)
	{
        case 0x0:
          if (data)
            putcmd(cp, cmdArpeggio, data);
          break;
        case 0x1:
          if (!data)
            putcmd(cp, cmdSpecial, cmdContMixPitchSlideUp);
          else
          if (data<0xE0)
            putcmd(cp, cmdPitchSlideUp, data);
          else
          if (data<0xF0)
            putcmd(cp, cmdRowPitchSlideUp, (data&0xF)<<2);
          else
            putcmd(cp, cmdRowPitchSlideUp, (data&0xF)<<4);
          break;
        case 0x2:
          if (!data)
            putcmd(cp, cmdSpecial, cmdContMixPitchSlideDown);
          else
          if (data<0xE0)
            putcmd(cp, cmdPitchSlideDown, data);
          else
          if (data<0xF0)
            putcmd(cp, cmdRowPitchSlideDown, (data&0xF)<<2);
          else
            putcmd(cp, cmdRowPitchSlideDown, (data&0xF)<<4);
          break;
        case 0x3:
          putcmd(cp, cmdPitchSlideToNote, data);
          break;
        case 0x4:
          putcmd(cp, cmdPitchVibrato, data);
          break;
        case 0x5:
          putcmd(cp, cmdPitchSlideToNote, 0);
          if (!data)
            putcmd(cp, cmdSpecial, cmdContVolSlide);
          if ((data&0x0F)&&(data&0xF0))
            data=0;
          if (data&0xF0)
            putcmd(cp, cmdVolSlideUp, (data>>4)<<2);
          else
          if (data&0x0F)
            putcmd(cp, cmdVolSlideDown, (data&0xF)<<2);
          break;
        case 0x6:
          putcmd(cp, cmdPitchVibrato, 0);
          if (!data)
            putcmd(cp, cmdSpecial, cmdContVolSlide);
          if ((data&0x0F)&&(data&0xF0))
            data=0;
          if (data&0xF0)
            putcmd(cp, cmdVolSlideUp, (data>>4)<<2);
          else
          if (data&0x0F)
            putcmd(cp, cmdVolSlideDown, (data&0xF)<<2);
         break;
        case 0x7:
          putcmd(cp, cmdVolVibrato, data);
          break;
        case 0x9:
          putcmd(cp, cmdOffset, data);
          break;
        case 0xA:
          if (!data)
            putcmd(cp, cmdSpecial, cmdContMixVolSlide);
          else
          if ((data&0x0F)==0x00)
            putcmd(cp, cmdVolSlideUp, (data>>4)<<2);
          else
          if ((data&0xF0)==0x00)
            putcmd(cp, cmdVolSlideDown, (data&0xF)<<2);
          else
          if ((data&0x0F)==0x0F)
            putcmd(cp, cmdRowVolSlideUp, (data>>4)<<2);
          else
          if ((data&0xF0)==0xF0)
            putcmd(cp, cmdRowVolSlideDown, (data&0xF)<<2);
          break;
        case 0xE:
          command=data>>4;
          data&=0x0F;
          switch (command)
          {
          case 0x1:
            putcmd(cp, cmdRowPitchSlideUp, data<<4);
            break;
          case 0x2:
            putcmd(cp, cmdRowPitchSlideDown, data<<4);
            break;
          case 0x3:
            putcmd(cp, cmdSpecial, data?cmdGlissOn:cmdGlissOff);
            break;
          case 0x4:
            if (data<4)
              putcmd(cp, cmdPitchVibratoSetWave, (data&3)+0x10);
            break;
          case 0x5: // finetune
            break;
          case 0x7:
            if (data<4)
              putcmd(cp, cmdVolVibratoSetWave, (data&3)+0x10);
            break;
          case 0x9:
            if (data)
              putcmd(cp, cmdRetrig, data);
	    break;
          case 0xA:
	    putcmd(cp, cmdRowVolSlideUp, data<<2);
            break;
          case 0xB:
            putcmd(cp, cmdRowVolSlideDown, data<<2);
            break;
          case 0xC:
            putcmd(cp, cmdNoteCut, data);
	    break;
          }
          break;
        case 0x11:
          putcmd(cp, cmdRetrig, data);
	  break;
        case 0x12:
          putcmd(cp, cmdPitchVibratoFine, data);
          break;
        case 0x13: // note slide down  xy x speed, y notecount
          break;
        case 0x14: // note slide up
          break;
        case 0x15: // note slide down + retrigger
          break;
        case 0x16: // note slide up + retrigger
          break;
        case 0x17:
          putcmd(cp, cmdOffsetEnd, data);
          break;
        }
      }

      gmdtrack &trk=m.tracks[t*(m.channum+1)+j];
      unsigned short len=tp-temptrack;

      if (!len)
        trk.ptr=trk.end=0;
      else
      {
        trk.ptr=new unsigned char[len];
        trk.end=trk.ptr+len;
        if (!trk.ptr)
          return errAllocMem;
        memcpy(trk.ptr, temptrack, len);
      }
    }

    unsigned char *tp=temptrack;
    unsigned char *bp=buffer;
    unsigned char *cp=tp+2;

    if (t==orders[0])
    {
//      if (hdr.it!=6)
//        putcmd(cp, cmdTempo, hdr.it);
//      if (hdr.is!=125)
//        putcmd(cp, cmdSpeed, hdr.is);
    }

    unsigned char row=0;
    while (row<64)
    {
      unsigned char c=*bp++;
      if (!c)
      {
        if (cp!=(tp+2))
        {
          tp[0]=row;
          tp[1]=cp-tp-2;
          tp=cp;
          cp=tp+2;
        }
        row++;
        continue;
      }
      unsigned char command=0;
      unsigned char data=0;
      if (c&0x20)
        bp+=2;
      if (c&0x40)
      {
        command=*bp++;
        data=*bp++;
      }
      if (c&0x80)
        bp++;

      int curchan=c&0x1F;
      if (curchan>=m.channum)
        continue;

      switch (command)
      {
      case 0xB:
        putcmd(cp, cmdGoto, data);
        break;
      case 0xD:
        putcmd(cp, cmdBreak, (data&0x0F)+(data>>4)*10);
        break;
      case 0xE:
        switch (data>>4)
        {
        case 0x6:
          putcmd(cp, cmdSetChan, curchan);
          putcmd(cp, cmdPatLoop, data&0xF);
          break;
        case 0xE:
          putcmd(cp, cmdPatDelay, data&0xF);
          break;
        }
        break;
      case 0xF:
        if (data)
          if (data<0x20)
            putcmd(cp, cmdTempo, data);
          else
            putcmd(cp, cmdSpeed, data);
        break;
      case 0x10:
        data=(data>0x3F)?0xFF:(data<<2);
        putcmd(cp, cmdGlobVol, data);
        break;
      }
    }

    gmdtrack &trk=m.tracks[t*(m.channum+1)+m.channum];
    unsigned short len=tp-temptrack;

    if (!len)
      trk.ptr=trk.end=0;
    else
    {
      trk.ptr=new unsigned char[len];
      trk.end=trk.ptr+len;
      if (!trk.ptr)
        return errAllocMem;
      memcpy(trk.ptr, temptrack, len);
    }
  }
  delete buffer;
  delete temptrack;

  for (i=0; i<m.instnum; i++)
  {
    gmdinstrument &ip=m.instruments[i];
    gmdsample &sp=m.modsamples[i];
    sampleinfo &sip=m.samples[i];

    if (sp.handle==0xFFFF)
      continue;
    char bit16=!!(sip.type&mcpSamp16Bit);

    unsigned long slen=sip.length<<bit16;
    file.seek(inspos[i]);
    sip.ptr=new unsigned char[slen+16];
    if (!sip.ptr)
      return errAllocMem;
    file.read(sip.ptr, slen);
    signed char x=0;
    for (j=0; j<slen; j++)
      ((unsigned char*)sip.ptr)[j]=x+=((unsigned char*)sip.ptr)[j];
  }

  return errOk;
}
