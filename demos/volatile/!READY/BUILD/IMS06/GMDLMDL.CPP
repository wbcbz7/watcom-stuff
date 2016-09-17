// (c) 1994-1997 Niklas Beisert, this file is part of "Indoor Music System".
// you must not read, modify, print, compile or copy this file or parts of
// it unless you have accepted the license in the accompanying file IMS.TXT.

#include <conio.h>
#include <string.h>
#include "binfile.h"

#include "mcp.h"
#include "gmdplay.h"
#include "err.h"

static const unsigned char *ibuf;
static unsigned long bitbuf;
static char bitnum;

static unsigned short readbits(char n)
{
  unsigned short v=bitbuf&((1L<<n)-1);
  bitbuf>>=n;
  bitnum-=n;
  if (bitnum<=24)
  {
    bitbuf|=(long)*ibuf++<<bitnum;
    bitnum+=8;
  }
  return v;
}


/*
static short vibsintab[256]=
 {    0,    50,   100,   151,   201,   251,   301,   350,
    400,   449,   498,   546,   595,   642,   690,   737,
    784,   830,   876,   921,   965,  1009,  1053,  1096,
   1138,  1179,  1220,  1260,  1299,  1338,  1375,  1412,
   1448,  1483,  1517,  1551,  1583,  1615,  1645,  1674,
   1703,  1730,  1757,  1782,  1806,  1829,  1851,  1872,
   1892,  1911,  1928,  1945,  1960,  1974,  1987,  1998,
   2009,  2018,  2026,  2033,  2038,  2042,  2046,  2047,
   2048,  2047,  2046,  2042,  2038,  2033,  2026,  2018,
   2009,  1998,  1987,  1974,  1960,  1945,  1928,  1911,
   1892,  1872,  1851,  1829,  1806,  1782,  1757,  1730,
   1703,  1674,  1645,  1615,  1583,  1551,  1517,  1483,
   1448,  1412,  1375,  1338,  1299,  1260,  1220,  1179,
   1138,  1096,  1053,  1009,   965,   921,   876,   830,
    784,   737,   690,   642,   595,   546,   498,   449,
    400,   350,   301,   251,   201,   151,   100,    50,
      0,   -50,  -100,  -151,  -201,  -251,  -301,  -350,
   -400,  -449,  -498,  -546,  -595,  -642,  -690,  -737,
   -784,  -830,  -876,  -921,  -965, -1009, -1053, -1096,
  -1138, -1179, -1220, -1260, -1299, -1338, -1375, -1412,
  -1448, -1483, -1517, -1551, -1583, -1615, -1645, -1674,
  -1703, -1730, -1757, -1782, -1806, -1829, -1851, -1872,
  -1892, -1911, -1928, -1945, -1960, -1974, -1987, -1998,
  -2009, -2018, -2026, -2033, -2038, -2042, -2046, -2047,
  -2048, -2047, -2046, -2042, -2038, -2033, -2026, -2018,
  -2009, -1998, -1987, -1974, -1960, -1945, -1928, -1911,
  -1892, -1872, -1851, -1829, -1806, -1782, -1757, -1730,
  -1703, -1674, -1645, -1615, -1583, -1551, -1517, -1483,
  -1448, -1412, -1375, -1338, -1299, -1260, -1220, -1179,
  -1138, -1096, -1053, -1009,  -965,  -921,  -876,  -830,
   -784,  -737,  -690,  -642,  -595,  -546,  -498,  -449,
   -400,  -350,  -301,  -251,  -201,  -151,  -100,   -50};
*/

static inline void putcmd(unsigned char *&p, unsigned char c, unsigned char d)
{
  *p++=c;
  *p++=d;
}

extern "C" int mpLoadMDL(gmdmodule &m, binfile &file)
{
  mpReset(m);

  if (file.getul()!=0x4C444D44)
    return errFormSig;

  if ((file.getuc()&0x10)!=0x10)
    return errFormOldVer;

  unsigned long blklen;

  if (file.getus()!=0x4E49)
    return errFormStruc;

  blklen=file.getul();
  struct
  {
    char name[32];
    char composer[20];
    unsigned short ordnum;
    unsigned short repstart;
    unsigned char mainvol;
    unsigned char speed;
    unsigned char bpm;
    unsigned char pan[32];
  } mdlhead;

  file.read(&mdlhead, 91);
  int i,j,k;
  for (i=0; i<32; i++)
    if (mdlhead.pan[i]&0x80)
      break;
  m.channum=i;
  memcpy(m.name, mdlhead.name, 31);
  m.name[32]=0;
  memcpy(m.composer, mdlhead.composer, 20);
  m.composer[20]=0;
  m.ordnum=mdlhead.ordnum;
  m.endord=m.ordnum;
  m.loopord=mdlhead.repstart;
  m.options=MOD_EXPOFREQ|MP_OFFSETDIV2;

  unsigned char ordtab[256];
  if (mdlhead.ordnum>256)
    return errFormSupp;

  file.read(ordtab, mdlhead.ordnum);
  file.seekcur(8*m.channum); // channames
  file.seekcur(blklen-8*m.channum-91-mdlhead.ordnum);

  unsigned short blktype;
  blktype=file.getus();
  blklen=file.getul();

  if (blktype==0x454D)
  {
    file.seekcur(blklen);
    blktype=file.getus();
    blklen=file.getul();
//songmessage; every line is closed with the CR-char (13). A
//0-byte stands at the end of the whole text.
  }

  if (blktype!=0x4150)
    return errFormStruc;
  unsigned char patnum=file.getuc();

  m.patnum=patnum+1;
  m.tracknum=patnum*(m.channum+1)+1;

  if (!mpAllocPatterns(m, m.patnum)||!mpAllocOrders(m, m.ordnum)||!mpAllocTracks(m, m.tracknum))
    return errAllocMem;

  for (j=0; j<patnum; j++)
  {
    unsigned char chnn=file.getc();
    m.patterns[j].patlen=file.getc()+1;
    file.read(m.patterns[j].name, 16);
    m.patterns[j].name[16]=0;
    memset(m.patterns[j].tracks, 0, 32*2);
    file.read(m.patterns[j].tracks, 2*chnn);
  }

  if (file.getus()!=0x5254)
    return errFormStruc;
  blklen=file.getul();

  unsigned short ntracks=file.getus();

  unsigned char **trackends=new unsigned char *[ntracks+1];
  unsigned char **trackptrs=new unsigned char *[ntracks+1];
  unsigned char *trackbuf=new unsigned char [blklen-2-2*ntracks];
  unsigned char (*patdata)[256][6]=new unsigned char [m.channum][256][6];
  unsigned char *temptrack=new unsigned char [3000];

  if (!trackends||!trackptrs||!trackbuf||!patdata||!temptrack)
    return errAllocMem;

  trackptrs[0]=trackbuf;
  trackends[0]=trackbuf;
  int tpos=0;
  for (i=0; i<ntracks; i++)
  {
    int l=file.getus();
    trackptrs[1+i]=trackbuf+tpos;
    file.read(trackbuf+tpos, l);
    tpos+=l;
    trackends[1+i]=trackbuf+tpos;
  }

  for (i=0; i<m.ordnum; i++)
    m.orders[i]=(ordtab[i]<patnum)?ordtab[i]:patnum;

  for (i=0; i<32; i++)
    m.patterns[patnum].tracks[i]=m.tracknum-1;
  m.patterns[patnum].gtrack=m.tracknum-1;
  m.patterns[patnum].patlen=64;

  for (j=0; j<patnum; j++)
  {
    memset(patdata, 0, m.channum*256*6);
    for (i=0; i<m.channum; i++)
    {
      unsigned char *trkptr=trackptrs[m.patterns[j].tracks[i]];
      unsigned char *endptr=trackends[m.patterns[j].tracks[i]];
      int row=0;
      while (trkptr<endptr)
      {
        unsigned char p=*trkptr++;
        switch (p&3)
        {
        case 0:
          row+=p>>2;
          break;
        case 1:
          for (k=0; k<((p>>2)+1); k++)
            memcpy(patdata[i][row+k], patdata[i][row-1], 6);
          row+=p>>2;
          break;
        case 2:
          memcpy(patdata[i][row], patdata[i][p>>2], 6);
          break;
        case 3:
          for (k=0; k<6; k++)
            if (p&(4<<k))
              patdata[i][row][k]=*trkptr++;
          break;
        }
        row++;
      }
    }

    for (i=0; i<m.channum; i++)
      m.patterns[j].tracks[i]=j*(m.channum+1)+i;
    m.patterns[j].gtrack=j*(m.channum+1)+m.channum;

    for (i=0; i<m.channum; i++)
    {
      unsigned char *tp=temptrack;
      unsigned char *buf=patdata[i][0];

      int row;
      for (row=0; row<m.patterns[j].patlen; row++, buf+=6)
      {
        unsigned char *cp=tp+2;

        unsigned char command1=buf[3]&0xF;
        unsigned char command2=buf[3]>>4;
        unsigned char data1=buf[4];
        unsigned char data2=buf[5];
        signed short ins=buf[1]-1;
        signed short nte=buf[0];
        signed short pan=-1;
        signed short vol=buf[2];
        short ofs;
        if (!vol)
          vol=-1;

        if (command1==0xE)
        {
          command1=(data1&0xF0)|0xE;
          data1&=0xF;
          if (command1==0xFE)
            ofs=(data1<<8)|data2;
        }
        if (command2==0xE)
        {
          command2=(data2&0xF0)|0xE;
          data2&=0xF;
        }

        if (!row&&(j==ordtab[0]))
          putcmd(cp, cmdPlayNote|cmdPlayPan, mdlhead.pan[i]*2);

        if (command1==0x8)
          pan=data1*2;

        if (command2==0x8)
          pan=data2*2;

        if ((command1==0x3)&&nte&&(nte!=255))
          nte|=128;

        if ((ins!=-1)||nte||(vol!=-1)||(pan!=-1))
        {
          unsigned char &act=*cp;
          *cp++=cmdPlayNote;
          if (ins!=-1)
          {
            act|=cmdPlayIns;
            *cp++=ins;
          }
          if (nte&&(nte!=255))
          {
            act|=cmdPlayNte;
            *cp++=nte+11;
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
          if (command1==0xDE)
          {
            act|=cmdPlayDelay;
            *cp++=data1;
          }
          else if (command2==0xDE)
          {
            act|=cmdPlayDelay;
            *cp++=data2;
          }

          if (nte==255)
            putcmd(cp, cmdKeyOff, 0);
        }

//E8x - Set Sample Status
//x=0 no loop, x=1 loop, unidirectional, x=3 loop, bidirectional
        switch (command1)
        {
        case 0x1:
          if (!data1)
            putcmd(cp, cmdSpecial, cmdContMixPitchSlideUp);
          else
          if (data1<0xE0)
            putcmd(cp, cmdPitchSlideUp, data1);
          else
          if (data1<0xF0)
            putcmd(cp, cmdRowPitchSlideUp, (data1&0xF)<<1);
          else
            putcmd(cp, cmdRowPitchSlideUp, (data1&0xF)<<4);
          break;
        case 0x2:
          if (!data1)
            putcmd(cp, cmdSpecial, cmdContMixPitchSlideDown);
          else
          if (data1<0xE0)
            putcmd(cp, cmdPitchSlideDown, data1);
          else
          if (data1<0xF0)
            putcmd(cp, cmdRowPitchSlideDown, (data1&0xF)<<1);
          else
            putcmd(cp, cmdRowPitchSlideDown, (data1&0xF)<<4);
          break;
        case 0x3:
          putcmd(cp, cmdPitchSlideToNote, data1);
          break;
        case 0x4:
          putcmd(cp, cmdPitchVibrato, data1);
          break;
        case 0x5:
          putcmd(cp, cmdArpeggio, data1);
          break;
        case 0x1E:
          putcmd(cp, cmdRowPanSlide, -data1*2);
          break;
        case 0x2E:
          putcmd(cp, cmdRowPanSlide, data1*2);
          break;
        case 0x3E:
          putcmd(cp, cmdSpecial, data1?cmdGlissOn:cmdGlissOff);
          break;
        case 0x4E:
          if (data1<4)
            putcmd(cp, cmdPitchVibratoSetWave, data1);
          break;
        case 0x7E:
          if (data1<4)
            putcmd(cp, cmdVolVibratoSetWave, data1);
          break;
        case 0x9E:
          putcmd(cp, cmdRetrig, data1);
          break;
        case 0xCE:
          putcmd(cp, cmdNoteCut, data1);
          break;
        case 0xFE:
          if (ofs&0xF00)
            putcmd(cp, cmdOffsetHigh, ofs>>8);
          putcmd(cp, cmdOffset, ofs);
          break;
        }

        switch (command2)
        {
        case 0x1:
          if (data2<0xE0)
            putcmd(cp, cmdVolSlideUp, data2);
          else
          if (data2<0xF0)
            putcmd(cp, cmdRowVolSlideUp, data2&0xF);
          else
            putcmd(cp, cmdRowVolSlideUp, (data2&0xF)<<2);
          break;
        case 0x2:
          if (data2<0xE0)
            putcmd(cp, cmdVolSlideDown, data2);
          else
          if (data2<0xF0)
            putcmd(cp, cmdRowVolSlideDown, data2&0xF);
          else
            putcmd(cp, cmdRowVolSlideDown, (data2&0xF)<<2);
          break;
        case 0x4:
          putcmd(cp, cmdVolVibrato, data2);
          break;
        case 0x5:
          putcmd(cp, cmdTremor, data2);
          break;
        case 0x1E:
          putcmd(cp, cmdRowPanSlide, -data2*2);
          break;
        case 0x2E:
          putcmd(cp, cmdRowPanSlide, data2*2);
          break;
        case 0x3E:
          putcmd(cp, cmdSpecial, data1?cmdGlissOn:cmdGlissOff);
          break;
        case 0x4E:
          if (data2<4)
            putcmd(cp, cmdPitchVibratoSetWave, data2);
          break;
        case 0x7E:
          if (data2<4)
            putcmd(cp, cmdVolVibratoSetWave, data2);
          break;
        case 0x9E:
          putcmd(cp, cmdRetrig, data2);
          break;
        case 0xCE:
          putcmd(cp, cmdNoteCut, data2);
          break;
        }

        if (cp!=(tp+2))
        {
          tp[0]=row;
          tp[1]=cp-tp-2;
          tp=cp;
        }
      }

      gmdtrack &trk=m.tracks[j*(m.channum+1)+i];
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
    unsigned char *buf=**patdata;

    int row;
    for (row=0; row<m.patterns[j].patlen; row++, buf+=6)
    {
      unsigned char *cp=tp+2;
      if (!row&&(j==ordtab[0]))
      {
        if (mdlhead.speed!=6)
          putcmd(cp, cmdTempo, mdlhead.speed);
        if (mdlhead.bpm!=125)
          putcmd(cp, cmdSpeed, mdlhead.bpm);
        if (mdlhead.mainvol!=255)
          putcmd(cp, cmdGlobVol, mdlhead.mainvol);
      }

      int q;
      for (q=0; q<m.channum; q++)
      {
        unsigned char command1=buf[256*6*q+3]&0xF;
        unsigned char command2=buf[256*6*q+3]>>4;
        unsigned char data1=buf[256*6*q+4];
        unsigned char data2=buf[256*6*q+5];

        switch (command1)
        {
        case 0x7:
          if (data1)
            putcmd(cp, cmdSpeed, data1);
          break;
        case 0xB:
          putcmd(cp, cmdGoto, data1);
          break;
        case 0xD:
          putcmd(cp, cmdBreak, (data1&0x0F)+(data1>>4)*10);
          break;
        case 0xE:
          switch (data1>>4)
          {
          case 0x6:
            putcmd(cp, cmdSetChan, q);
            putcmd(cp, cmdPatLoop, data1&0xF);
            break;
          case 0xE:
            putcmd(cp, cmdPatDelay, data1&0xF);
            break;
          case 0xA:
            putcmd(cp, cmdGlobVolSlide, data1&0xF);
            break;
          case 0xB:
            putcmd(cp, cmdSetChan, q);
            putcmd(cp, cmdGlobVolSlide, -(data1&0xF));
            break;
          }
          break;
        case 0xF:
          if (data1)
            putcmd(cp, cmdTempo, data1);
          break;
        case 0xC:
          putcmd(cp, cmdGlobVol, data1);
          break;
        }
        switch (command2)
        {
        case 0x7:
          if (data2)
            putcmd(cp, cmdSpeed, data2);
          break;
        case 0xB:
          putcmd(cp, cmdGoto, data2);
          break;
        case 0xD:
          putcmd(cp, cmdBreak, (data2&0x0F)+(data2>>4)*10);
          break;
        case 0xE:
          switch (data2>>4)
          {
          case 0x6:
            putcmd(cp, cmdSetChan, q);
            putcmd(cp, cmdPatLoop, data2&0xF);
            break;
          case 0xE:
            putcmd(cp, cmdPatDelay, data2&0xF);
            break;
          case 0xA:
            putcmd(cp, cmdGlobVolSlide, data2&0xF);
            break;
          case 0xB:
            putcmd(cp, cmdSetChan, q);
            putcmd(cp, cmdGlobVolSlide, -(data2&0xF));
            break;
          }
          break;
        case 0xF:
          if (data2)
            putcmd(cp, cmdTempo, data2);
          break;
        case 0xC:
          putcmd(cp, cmdGlobVol, data2);
          break;
        }
      }

      if (cp!=(tp+2))
      {
        tp[0]=row;
        tp[1]=cp-tp-2;
        tp=cp;
      }
    }

    gmdtrack &trk=m.tracks[j*(m.channum+1)+m.channum];
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
  delete temptrack;
  delete trackends;
  delete trackptrs;
  delete trackbuf;
  delete patdata;

  if (file.getus()!=0x4949)
    return errFormStruc;
  blklen=file.getul();

  int inssav=file.getuc();

  m.instnum=255;
  m.modsampnum=0;
  m.envnum=192;

//  envelope **envs=new envelope *[255];
  gmdsample **msmps=new gmdsample *[255];
  int *inssampnum=new int [255];
//  int *insenvnum=new int [255];
   if (/*!envs||!insenvnum||*/!inssampnum||!msmps||!mpAllocInstruments(m, m.instnum))
    return errAllocMem;

  int maxins=0;

  memset(msmps, 0, 4*255);
//  memset(envs, 0, 4*255);
  memset(inssampnum, 0, 4*255);
//  memset(insenvnum, 0, 4*255);

  for (j=0; j<inssav; j++)
  {
    unsigned char insnum=file.getuc()-1;
    gmdinstrument &ip=m.instruments[insnum];

    inssampnum[j]=file.getuc();
    file.read(ip.name, 32);
    ip.name[31]=0;
    msmps[j]=new gmdsample [inssampnum[j]];
//    envs[insnum]=new envelope [inssampnum[j]];
    if (!msmps[j]/*||!envs[insnum]*/)
      return errAllocMem;

    int note=0;
    for (i=0; i<inssampnum[j]; i++)
    {
      struct
      {
        unsigned char smp;
        unsigned char highnote;
        unsigned char vol;
        unsigned char volenv;
        unsigned char pan;
        unsigned char panenv;
        unsigned short fadeout;
        unsigned char vibspd;
        unsigned char vibdep;
        unsigned char vibswp;
        unsigned char vibfrm;
        unsigned char res1;
        unsigned char pchenv;
      } mdlmsmp;
      file.read(&mdlmsmp, sizeof(mdlmsmp));
      if ((mdlmsmp.highnote+12)>128)
        mdlmsmp.highnote=128-12;
      while (note<(mdlmsmp.highnote+12))
        ip.samples[note++]=m.modsampnum;
      m.modsampnum++;

      gmdsample &sp=msmps[j][i];
      *sp.name=0;
      sp.handle=mdlmsmp.smp-1;
      sp.normnote=0;
      sp.stdvol=(mdlmsmp.volenv&0x40)?mdlmsmp.vol:-1;
      sp.stdpan=(mdlmsmp.panenv&0x40)?mdlmsmp.pan*2:-1;
      sp.opt=0;
      sp.volfade=mdlmsmp.fadeout;
      sp.vibspeed=0;
      sp.vibdepth=mdlmsmp.vibdep*4;
      sp.vibrate=mdlmsmp.vibspd<<7;
      sp.vibsweep=0xFFFF/(mdlmsmp.vibswp+1);
      sp.vibtype=mdlmsmp.vibfrm;
      sp.pchint=4;
      sp.volenv=(mdlmsmp.volenv&0x80)?(mdlmsmp.volenv&0x3F):0xFFFF;
      sp.panenv=(mdlmsmp.panenv&0x80)?(64+(mdlmsmp.panenv&0x3F)):0xFFFF;
      sp.pchenv=(mdlmsmp.pchenv&0x80)?(128+(mdlmsmp.pchenv&0x3F)):0xFFFF;;
/*
      if (mdlmsmp.vibdep&&mdlmsmp.vibspd)
      {
        sp.vibenv=m.envnum++;

        envelope &ep=envs[insnum][insenvnum[j]++];
        ep.speed=0;
        ep.opt=0;
        ep.len=512;
        ep.sustain=-1;
        ep.loops=0;
        ep.loope=512;
        ep.env=new unsigned char [512];
        if (!ep.env)
          return errAllocMem;
        unsigned char ph=0;
        for (k=0; k<512; k++)
        {
          ph=k*mdlmsmp.vibspd/2;
          switch (mdlmsmp.vibfrm)
          {
          case 0:
            ep.env[k]=128+((mdlmsmp.vibdep*vibsintab[ph])>>10);
            break;
          case 1:
            ep.env[k]=128+((mdlmsmp.vibdep*(64-(ph&128)))>>5);
            break;
          case 2:
            ep.env[k]=128+((mdlmsmp.vibdep*(128-ph))>>6);
            break;
          case 3:
            ep.env[k]=128+((mdlmsmp.vibdep*(ph-128))>>6);
            break;
          }
        }
      }
*/
    }
  }

  m.sampnum=255;
  if (!mpAllocModSamples(m, m.modsampnum)||!mpAllocEnvelopes(m, m.envnum)||!mpAllocSamples(m, m.sampnum))
    return errAllocMem;

  int smpnum=0;
//  int envnum=192;
  for (j=0; j<255; j++)
  {
    memcpy(m.modsamples+smpnum, msmps[j], sizeof (*m.modsamples)*inssampnum[j]);
    smpnum+=inssampnum[j];
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//    for (i=0; i<insenvnum[j]; i++)
//      memcpy(&m.envelopes[envnum++], &envs[j][i], sizeof (*m.envelopes));
    delete msmps[j];
//    delete envs[j];
  }
  delete msmps;
//  delete envs;
  delete inssampnum;
//  delete insenvnum;

  blktype=file.getus();
  blklen=file.getul();
  if (blktype==0x4556)
  {
    unsigned char envnum=file.getuc();
    for (i=0; i<envnum; i++)
    {
      struct
      {
        unsigned char num;
        unsigned char env[15][2];
        unsigned char sus;
        unsigned char loop;
      } env;
      file.read(&env, sizeof(env));
      if (env.env[0][0]!=1)
        continue;
      gmdenvelope &e=m.envelopes[env.num];

      e.type=((env.sus&16)?mpEnvSLoop:0)|((env.sus&32)?mpEnvLoop:0);
      e.speed=0;
      int k,l;
      l=-1;
      for (j=0; j<15; j++)
      {
        if (!env.env[j][0])
          break;
        l+=env.env[j][0];
        if ((env.loop&15)==j)
          e.loops=l;
        if ((env.loop>>4)==j)
          e.loope=l;
        if ((env.sus&15)==j)
        {
          e.sloops=l;
          e.sloope=l+1;
        }
      }
      if ((e.type&mpEnvSLoop)&&(e.type&mpEnvLoop)&&(e.sloope>e.loope))
      {
        e.sloops=e.loops;
        e.sloope=e.loope;
      }
      e.len=l;
      e.env=new unsigned char [l+1];
      if (!e.env)
        return errAllocMem;
      l=1;
      e.env[0]=env.env[0][1]<<2;
      for (j=0; j<15; j++)
      {
        if (!env.env[j+1][0])
          break;
        for (k=1; k<=env.env[j+1][0]; k++)
          e.env[l++]=4*env.env[j][1]+4*k*(env.env[j+1][1]-env.env[j][1])/env.env[j+1][0];
      }
    }

    blktype=file.getus();
    blklen=file.getul();
  }

  if (blktype==0x4550)
  {
    unsigned char envnum=file.getuc();
    for (i=0; i<envnum; i++)
    {
      struct
      {
        unsigned char num;
        unsigned char env[15][2];
        unsigned char sus;
        unsigned char loop;
      } env;
      file.read(&env, sizeof(env));
      if (env.env[0][0]!=1)
        continue;
      gmdenvelope &e=m.envelopes[64+env.num];

      e.type=((env.sus&16)?mpEnvSLoop:0)|((env.sus&32)?mpEnvLoop:0);
      e.speed=0;
      int k,l;
      l=-1;
      for (j=0; j<15; j++)
      {
        if (!env.env[j][0])
          break;
        l+=env.env[j][0];
        if ((env.loop&15)==j)
          e.loops=l;
        if ((env.loop>>4)==j)
          e.loope=l;
        if ((env.sus&15)==j)
        {
          e.sloops=l;
          e.sloope=l+1;
        }
      }
      if ((e.type&mpEnvSLoop)&&(e.type&mpEnvLoop)&&(e.sloope>e.loope))
      {
        e.sloops=e.loops;
        e.sloope=e.loope;
      }
      e.len=l;
      e.env=new unsigned char [l+1];
      if (!e.env)
        return errAllocMem;
      l=1;
      e.env[0]=env.env[0][1]<<2;
      for (j=0; j<15; j++)
      {
        if (!env.env[j+1][0])
          break;
        for (k=1; k<=env.env[j+1][0]; k++)
          e.env[l++]=4*env.env[j][1]+4*k*(env.env[j+1][1]-env.env[j][1])/env.env[j+1][0];
      }
    }

    blktype=file.getus();
    blklen=file.getul();
  }

  if (blktype==0x4546)
  {
    unsigned char envnum=file.getuc();
    for (i=0; i<envnum; i++)
    {
      struct
      {
        unsigned char num;
        unsigned char env[15][2];
        unsigned char sus;
        unsigned char loop;
      } env;
      file.read(&env, sizeof(env));
      if (env.env[0][0]!=1)
        continue;
      gmdenvelope &e=m.envelopes[128+env.num];

      e.type=((env.sus&32)?mpEnvLoop:0)|((env.sus&16)?mpEnvSLoop:0);
      e.speed=0;
      int k,l;
      l=-1;
      for (j=0; j<15; j++)
      {
        if (!env.env[j][0])
          break;
        l+=env.env[j][0];
        if ((env.loop&15)==j)
          e.loops=l;
        if ((env.loop>>4)==j)
          e.loope=l;
        if ((env.sus&15)==j)
        {
          e.sloops=l;
          e.sloope=l+1;
        }
      }
      if ((e.type&mpEnvSLoop)&&(e.type&mpEnvLoop)&&(e.sloope>e.loope))
      {
        e.sloops=e.loops;
        e.sloope=e.loope;
      }
      e.len=l;
      e.env=new unsigned char [l+1];
      if (!e.env)
        return errAllocMem;
      l=1;
      e.env[0]=env.env[0][1]<<2;
      for (j=0; j<15; j++)
      {
        if (!env.env[j+1][0])
          break;
        for (k=1; k<=env.env[j+1][0]; k++)
          e.env[l++]=4*env.env[j][1]+4*k*(env.env[j+1][1]-env.env[j][1])/env.env[j+1][0];
      }
    }

    blktype=file.getus();
    blklen=file.getul();
  }

  if (blktype!=0x5349)
    return errFormStruc;

  int smpsav=file.getuc();

  unsigned char packtype[255];
  memset(packtype, 0xFF, 255);

  for (i=0; i<smpsav; i++)
  {
    struct
    {
      unsigned char num;
      char name[32];
      char filename[8];
      unsigned long rate;
      unsigned long len;
      unsigned long loopstart;
      unsigned long replen;
      unsigned char vol;
      unsigned char opt;
    } mdlsmp;
    file.read(&mdlsmp, sizeof(mdlsmp));

    mdlsmp.name[31]=0;
    if (mdlsmp.opt&1)
    {
      mdlsmp.len>>=1;
      mdlsmp.loopstart>>=1;
      mdlsmp.replen>>=1;
    }

    for (j=0; j<m.modsampnum; j++)
      if (m.modsamples[j].handle==(mdlsmp.num-1))
        strcpy(m.modsamples[j].name, mdlsmp.name);

    sampleinfo &sip=m.samples[mdlsmp.num-1];

    sip.ptr=0;
    sip.length=mdlsmp.len;
    sip.loopstart=mdlsmp.loopstart;
    sip.loopend=mdlsmp.loopstart+mdlsmp.replen;
    sip.samprate=mdlsmp.rate;
    sip.type=((mdlsmp.opt&1)?mcpSamp16Bit:0)|(mdlsmp.replen?mcpSampLoop:0)|((mdlsmp.opt&2)?mcpSampBiDi:0);

    packtype[mdlsmp.num-1]=(mdlsmp.opt>>2)&3;
  }

  if (file.getus()!=0x4153)
    return errFormStruc;
  blklen=file.getul();

  for (i=0; i<255; i++)
  {
    if (packtype[i]==255)
      continue;

    sampleinfo &sip=m.samples[i];
    int bit16=!!(sip.type&mcpSamp16Bit);

    sip.ptr=new unsigned char [(sip.length+8)<<bit16];
    if (!sip.ptr)
      return errAllocMem;

    if (packtype[i]==0)
    {
      file.read(sip.ptr, sip.length<<bit16);
      continue;
    }

    unsigned long packlen=file.getul();
    unsigned char *packbuf=new unsigned char [packlen+4];

    if (!packbuf)
      return errAllocMem;
    file.read(packbuf, packlen);

    bitbuf=*(unsigned long*)packbuf;
    bitnum=32;
    ibuf=packbuf+4;

    unsigned char dlt=0;
    bit16=packtype[i]==2;
    for (j=0; j<sip.length; j++)
    {
      unsigned char lowbyte;
      if (bit16)
        lowbyte=readbits(8);

      unsigned char byte;
      unsigned char sign=readbits(1);
      if (readbits(1))
        byte=readbits(3);
      else
      {
        byte=8;
        while (!readbits(1))
          byte+=16;
        byte+=readbits(4);
      }
      if (sign)
        byte=~byte;
      dlt+=byte;
      if (!bit16)
        ((unsigned char*)sip.ptr)[j]=dlt;
      else
        ((unsigned short*)sip.ptr)[j]=(dlt<<8)|lowbyte;
    }

    delete packbuf;
  }

  return errOk;
}
