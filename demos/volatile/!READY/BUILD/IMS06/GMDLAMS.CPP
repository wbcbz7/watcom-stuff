// (c) 1994-1997 Niklas Beisert, this file is part of "Indoor Music System".
// you must not read, modify, print, compile or copy this file or parts of
// it unless you have accepted the license in the accompanying file IMS.TXT.

// ENVELOPES & SUSTAIN!!!

#include <string.h>
#include "binfile.h"

#include "mcp.h"
#include "gmdplay.h"
#include "err.h"

static inline void putcmd(unsigned char *&p, unsigned char c, unsigned char d)
{
  *p++=c;
  *p++=d;
}

static const unsigned char envsin[513]=
{
  0,1,2,2,3,4,5,5,6,7,8,8,9,10,11,12,12,13,14,15,16,16,17,18,19,19,20,21,22,22,23,24,25,26,26,27,28,29,30,30,31,32,33,33,34,35,36,36,37,38,39,39,40,41,42,43,43,44,45,46,46,47,48,49,49,50,51,52,52,53,54,55,55,56,57,58,58,59,60,61,61,62,63,64,64,65,66,67,67,68,69,70,70,71,72,73,74,74,75,76,76,77,78,79,79,80,81,82,82,83,84,84,85,86,87,87,88,89,90,90,91,92,93,93,94,95,96,96,97,98,98,99,100,100,101,102,103,103,104,105,106,106,107,108,108,109,110,110,111,112,113,113,114,115,115,116,117,117,118,
  119,119,120,121,121,122,123,123,124,125,126,126,127,127,128,129,130,130,131,132,132,133,134,134,135,136,136,137,138,138,139,139,140,141,141,142,143,143,144,145,145,146,146,147,148,148,149,150,150,151,152,152,153,153,154,155,155,156,156,157,158,158,159,160,160,161,161,162,163,163,164,164,165,166,166,167,167,168,168,169,170,170,171,171,172,173,173,174,174,175,175,176,177,177,178,178,179,179,180,180,181,181,182,182,183,184,184,185,185,186,186,187,187,188,188,189,190,190,191,191,192,192,193,
  193,194,194,195,195,196,196,197,197,198,198,199,199,200,200,201,201,201,202,202,203,203,204,204,205,205,206,206,207,207,207,208,208,209,209,210,210,211,211,211,212,212,213,213,214,214,214,215,215,216,216,216,217,217,218,218,219,219,219,220,220,221,221,221,222,222,223,223,223,224,224,224,225,225,225,226,226,227,227,227,228,228,228,229,229,229,230,230,230,231,231,231,232,232,232,233,233,233,234,234,234,234,235,235,235,236,236,236,237,237,237,237,238,238,238,239,239,239,239,240,240,240,240,
  241,241,241,241,242,242,242,242,243,243,243,243,244,244,244,244,244,245,245,245,245,246,246,246,246,246,247,247,247,247,247,248,248,248,248,248,248,249,249,249,249,249,249,250,250,250,250,250,250,250,251,251,251,251,251,251,251,252,252,252,252,252,252,252,252,253,253,253,253,253,253,253,253,253,253,253,254,254,254,254,254,254,254,254,254,254,254,254,254,254,254,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255
};


extern "C" int mpLoadAMS(gmdmodule &m, binfile &file)
{
  mpReset(m);

  unsigned char sig[8];
  file.read(sig, 8);
  if (!memcmp(sig, "Extreme", 7))
    return errFormOldVer;
  if (memcmp(sig, "AMShdr\x1A", 7))
    return errFormSig;

  file.read(m.name, sig[7]);
  m.name[sig[7]]=0;

  unsigned short filever;
  filever=file.gets();
  if ((filever!=0x201)&&(filever!=0x202))
    return errFormOldVer;

  struct
  {
    unsigned char ins;
    unsigned short pat;
    unsigned short pos;
    unsigned short bpm;
    unsigned char speed;
    unsigned char defchn;
    unsigned char defcmd;
    unsigned char defrow;
    unsigned short flags;
  } hdr;

  if (filever==0x201)
  {
    struct
    {
      unsigned char ins;
      unsigned short pat;
      unsigned short pos;
      unsigned char bpm;
      unsigned char speed;
      unsigned char flags;
//          1       Flags:mfsspphh
//                        ³³³³³³ÀÁÄ Pack byte header
//                        ³³³³ÀÁÄÄÄ Pack byte patterns
//                        ³³ÀÁÄÄÄÄÄ Pack byte samples
//                        ÀÄÄÄÄÄÄÄÄ MIDI channels are used in tune.
    } oldhdr;
    file.read(&oldhdr, sizeof(oldhdr));
    hdr.ins=oldhdr.ins;
    hdr.pat=oldhdr.pat;
    hdr.pos=oldhdr.pos;
    hdr.bpm=oldhdr.bpm<<8;
    hdr.speed=oldhdr.speed;
    hdr.flags=(oldhdr.flags&0xC0)|0x20;
  }
  else
    file.read(&hdr, sizeof(hdr));

  m.options=((hdr.flags&0x40)?MOD_EXPOFREQ:0)|MOD_EXPOPITCHENV;

  m.channum=32;
  m.instnum=hdr.ins;
  m.envnum=hdr.ins*3;
  m.patnum=hdr.pat+1;
  m.ordnum=hdr.pos;
  m.endord=hdr.pos;
  m.tracknum=33*hdr.pat+1;
  m.loopord=0;

  unsigned short *ordlist;
  ordlist=new unsigned short[hdr.pos];
  sampleinfo **smps=new sampleinfo *[m.instnum];
  gmdsample **msmps=new gmdsample *[m.instnum];
  int *instsampnum=new int [m.instnum];
  if (!ordlist||!mpAllocInstruments(m, m.instnum)||!mpAllocPatterns(m, m.patnum)||!mpAllocTracks(m, m.tracknum)||!mpAllocEnvelopes(m, m.envnum)||!smps||!msmps||!mpAllocOrders(m, m.ordnum)||!instsampnum)
    return errAllocMem;

  int i,j,t;

  unsigned char namelen;
  unsigned char shadowedby[256];

  m.sampnum=0;
  m.modsampnum=0;
  for (i=0; i<m.instnum; i++)
  {
    gmdinstrument &ip=m.instruments[i];
    smps[i]=0;
    msmps[i]=0;
    shadowedby[i]=0;

    unsigned char smpnum;
    namelen=file.getc();

    file.read(&ip.name, namelen);
    ip.name[namelen]=0;

    smpnum=file.getc();
    instsampnum[i]=smpnum;

    if (!smpnum)
      continue;

    msmps[i]=new gmdsample[smpnum];
    smps[i]=new sampleinfo[smpnum];
    if (!smps[i]||!msmps[i])
      return errAllocMem;

    memset(msmps[i], 0, sizeof(**msmps)*smpnum);
    memset(smps[i], 0, sizeof(**smps)*smpnum);

    unsigned char samptab[120];

    struct
    {
      unsigned char speed;
      unsigned char sustain;
      unsigned char loopstart;
      unsigned char loopend;
      unsigned char points;
      unsigned char data[64][3];
    } envs[3];
    unsigned short envflags;

    file.read(&samptab, 120);
    for (j=0; j<3; j++)
    {
      file.read(&envs[j], 5);
      file.read(&envs[j].data, envs[j].points*3);
    }

    unsigned char vibsweep=0;
    unsigned char shadowinst;
    unsigned short volfade;

    shadowinst=file.getc();
    if (filever==0x201)
    {
      vibsweep=shadowinst;
      shadowinst=0;
    }
    shadowedby[i]=shadowinst;

    volfade=file.gets();
    envflags=file.gets();

    unsigned char pchint=(volfade>>12)&3;
    volfade&=0xFFF;

    for (t=0; t<envs[0].points; t++)
      envs[0].data[t][2]<<=1;

    for (j=0; j<3; j++)
      if (envflags&(4<<(3*j)))
      {
        unsigned int envlen=0;
        for (t=1; t<envs[j].points; t++)
          envlen+=((envs[j].data[t][1]&1)<<8)|envs[j].data[t][0];

        unsigned char *env=new unsigned char [envlen+1];
        if (!env)
          return errAllocMem;

        int k, p=0, h=envs[j].data[0][2];
        for (t=1; t<envs[j].points; t++)
        {
          int l=((envs[j].data[t][1]&1)<<8)|envs[j].data[t][0];
          int dh=envs[j].data[t][2]-h;
          switch (envs[j].data[t][1]&6)
          {
          case 0:
            for (k=0; k<l; k++)
              env[p++]=h+dh*k/l;
            break;
          case 2:
            for (k=0; k<l; k++)
              env[p++]=h+dh*envsin[512*k/l]/256;
            break;
          case 4:
            for (k=0; k<l; k++)
              env[p++]=h+dh*(255-envsin[512-512*k/l])/256;
            break;
          }
          h+=dh;
        }
        env[p]=h;

        signed int sus=-1;
        signed int lst=-1;
        signed int lend=-1;

        if (envflags&(2<<(3*j)))
        {
          sus=0;
          for (t=1; t<envs[j].sustain; t++)
            sus+=((envs[j].data[t][1]&1)<<8)|envs[j].data[t][0];
        }
        if (envflags&(1<<(3*j)))
        {
          lst=0;
          lend=0;
          for (t=1; t<envs[j].loopstart; t++)
            lst+=((envs[j].data[t][1]&1)<<8)|envs[j].data[t][0];
          for (t=1; t<envs[j].loopend; t++)
            lend+=((envs[j].data[t][1]&1)<<8)|envs[j].data[t][0];
        }

        m.envelopes[i*3+j].env=env;
        m.envelopes[i*3+j].len=envlen;
        m.envelopes[i*3+j].type=0;
        m.envelopes[i*3+j].speed=envs[j].speed;
        if (sus!=-1)
        {
          m.envelopes[i*3+j].sloops=sus;
          m.envelopes[i*3+j].sloope=sus+1;
          m.envelopes[i*3+j].type=mpEnvSLoop;
        }
        if (lst!=-1)
        {
          if (envflags&(0x200<<j))
          {
            if (lend<sus)
            {
              m.envelopes[i*3+j].sloops=lst;
              m.envelopes[i*3+j].sloope=lend;
              m.envelopes[i*3+j].type=mpEnvSLoop;
            }
          }
          else
          {
            m.envelopes[i*3+j].loops=lst;
            m.envelopes[i*3+j].loope=lend;
            m.envelopes[i*3+j].type=mpEnvLoop;
          }
        }
      }

    memset(ip.samples, -1, 128*2);

    for (j=0; j<smpnum; j++, m.sampnum++, m.modsampnum++)
    {
      gmdsample &sp=msmps[i][j];
      sampleinfo &sip=smps[i][j];

      int k;
      for (k=0; k<116; k++)
        if (samptab[k]==j)
          ip.samples[k+12]=m.modsampnum;

      sp.handle=0xFFFF;
      sp.volenv=0xFFFF;
      sp.panenv=0xFFFF;
      sp.pchenv=0xFFFF;
      sp.volfade=0xFFFF;

      namelen=file.getc();
      file.read(sp.name, namelen);
      sp.name[namelen]=0;
      sip.length=file.getl();
      if (!sip.length)
        continue;
      struct
      {
        unsigned long loopstart;
        unsigned long loopend;
        unsigned short samprate;
        unsigned char panfine;
        unsigned short rate;
        signed char relnote;
        unsigned char vol;
	unsigned char flags; // bit 6: direction
      } amssmp;
      file.read(&amssmp, sizeof(amssmp));

      sp.stdpan=(amssmp.panfine&0xF0)?((amssmp.panfine>>4)*0x11):-1;
      sp.stdvol=amssmp.vol*2;
      sp.normnote=-amssmp.relnote*256-((signed char)(amssmp.panfine<<4))*2;
      sp.opt=(amssmp.flags&0x04)?MP_OFFSETDIV2:0;

      sp.volfade=volfade;
      sp.pchint=pchint;
      sp.volenv=m.envelopes[3*i+0].env?(3*i+0):-1;
      sp.panenv=m.envelopes[3*i+1].env?(3*i+1):-1;
      sp.pchenv=m.envelopes[3*i+2].env?(3*i+2):-1;

      sip.loopstart=amssmp.loopstart;
      sip.loopend=amssmp.loopend;
      sip.samprate=amssmp.rate;
      sip.type=((amssmp.flags&0x04)?mcpSamp16Bit:0)|((amssmp.flags&0x08)?mcpSampLoop:0)|((amssmp.flags&0x10)?mcpSampBiDi:0);
    }
  }

  namelen=file.getc();
  file.read(&m.composer, namelen);
  m.composer[namelen]=0;
  for (i=0; i<32; i++)
  {
    namelen=file.getc();
    file.seekcur(namelen);
  }

  unsigned long packlen;
  packlen=file.getl();
  file.seekcur(packlen-4);

  file.read(ordlist, 2*hdr.pos);

  for (i=0; i<m.ordnum; i++)
    m.orders[i]=(ordlist[i]<hdr.pat)?ordlist[i]:hdr.pat;

  for (i=0; i<32; i++)
    m.patterns[hdr.pat].tracks[i]=m.tracknum-1;
  m.patterns[hdr.pat].gtrack=m.tracknum-1;
  m.patterns[hdr.pat].patlen=64;

  unsigned char *temptrack=new unsigned char[4000];
  unsigned int buflen=0;
  unsigned char *buffer=0;
  if (!temptrack)
    return errAllocMem;

  m.channum=1;

  for (t=0; t<hdr.pat; t++)
  {
    unsigned long patlen;
    unsigned char maxrow;
    unsigned char chan;
    unsigned char maxcmd;
    patlen=file.getl();
    maxrow=file.getc();
    chan=file.getc();
    namelen=file.getc();
    patlen-=3+namelen;
    char patname[11];
    file.read(patname, namelen);
    patname[namelen]=0;
    maxcmd=chan>>5;
    chan&=0x1F;
    chan++;
    if (chan>m.channum)
      m.channum=chan;

    gmdpattern &pp=m.patterns[t];
    for (i=0; i<32; i++)
      pp.tracks[i]=t*33+i;
    pp.gtrack=t*33+32;
    pp.patlen=maxrow+1;
    strcpy(pp.name, patname);

    if (patlen>buflen)
    {
      buflen=patlen;
      delete buffer;
      buffer=new unsigned char[buflen];
      if (!buffer)
	return errAllocMem;
    }
    file.read(buffer, patlen);

    for (i=0; i<chan; i++)
    {
      unsigned char *tp=temptrack;
      unsigned char *buf=buffer;

      unsigned int row=0;
      unsigned char another=1;
      while (row<=maxrow)
      {
	if (!another||(*buf==0xFF))
	{
	  if (another)
	    buf++;
	  row++;
	  another=1;
          continue;
	}
	another=!(*buf&0x80);
	unsigned char curchan=*buf&0x1F;
	unsigned char anothercmd=1;
	unsigned char nte=0;
	unsigned char ins=0;
        unsigned char noteporta=0;
        short delaynote=-1;
        unsigned char cmds[7][2];
        unsigned char cmdnum=0;
        signed short vol=-1;
        signed short pan=-1;
//        if ((ordlist[0]==t)&&!row)
//          pan=(i&1)?0xC0:0x40;

	if (!(*buf++&0x40))
	{
          anothercmd=*buf&0x80;
          nte=*buf++&0x7F;
          ins=*buf++;
	}
        while (anothercmd)
        {
          anothercmd=*buf&0x80;
          cmds[cmdnum][0]=*buf++&0x7F;
          if (!(cmds[cmdnum][0]&0x40))
            switch (cmds[cmdnum][0])
            {
            case 0x08:
              pan=(*buf++&0x0F)*0x11;
              break;
            case 0x0C:
              vol=*buf++*2;
              break;
            case 0x03: case 0x05: case 0x15:
              noteporta=1;
              cmds[cmdnum++][1]=*buf++;
              break;
            case 0x0E:
              if ((*buf&0xF0)==0xD0)
                delaynote=*buf&0x0F;
              cmds[cmdnum++][1]=*buf++;
              break;
            default:
              cmds[cmdnum++][1]=*buf++;
            }
          else
            vol=(cmds[cmdnum][0]&0x3f)*4;
        }
        if (curchan!=i)
          continue;

	unsigned char *cp=tp+2;

        if ((ordlist[0]==t)&&!row)
          putcmd(cp, cmdPlayNote|cmdPlayPan, (i&1)?0xC0:0x40);

        if (ins||nte||(vol!=-1)||(pan!=-1))
        {
          unsigned char &act=*cp;
          *cp++=cmdPlayNote;
          if (ins)
          {
            act|=cmdPlayIns;
            *cp++=ins-1;
          }
          if (nte>=2)
          {
            act|=cmdPlayNte;
            *cp++=(nte+10)|(noteporta?0x80:0);
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
	  if (delaynote!=-1)
          {
            act|=cmdPlayDelay;
            *cp++=delaynote;
          }

          if (nte==1)
	    putcmd(cp, cmdKeyOff, 0);
        }

        for (j=0; j<cmdnum; j++)
        {
          unsigned char data=cmds[j][1];
          switch (cmds[j][0])
          {
          case 0x0:
            if (data)
              putcmd(cp, cmdArpeggio, data);
            break;
          case 0x1:
            putcmd(cp, cmdPitchSlideUp, data);
            break;
          case 0x2:
            putcmd(cp, cmdPitchSlideDown, data);
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
            else
            if (data&0xF0)
              putcmd(cp, cmdVolSlideUp, (data>>4)<<2);
            else
              putcmd(cp, cmdVolSlideDown, (data&0xF)<<2);
            break;
          case 0x6:
            putcmd(cp, cmdPitchVibrato, 0);
            if (!data)
              putcmd(cp, cmdSpecial, cmdContVolSlide);
            else
            if (data&0xF0)
              putcmd(cp, cmdVolSlideUp, (data>>4)<<2);
            else
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
              putcmd(cp, cmdSpecial, cmdContVolSlide);
            else
            if (data&0xF0)
              putcmd(cp, cmdVolSlideUp, (data>>4)<<2);
            else
              putcmd(cp, cmdVolSlideDown, (data&0xF)<<2);
            break;
          case 0xE:
            cmds[j][0]=data>>4;
            data&=0x0F;
            switch (cmds[j][0])
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
                putcmd(cp, cmdPitchVibratoSetWave, data);
              break;
            case 0x7:
              if (data<4)
                putcmd(cp, cmdVolVibratoSetWave, data);
              break;
            case 0x8:
              if (!(data&0x0F))
                putcmd(cp, cmdSetLoop, 0);
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
            putcmd(cp, cmdRowPitchSlideUp, data<<2);
            break;
          case 0x12:
            putcmd(cp, cmdRowPitchSlideDown, data<<2);
            break;
          case 0x13:
            putcmd(cp, cmdRetrig, data);
            break;
          case 0x15:
            putcmd(cp, cmdPitchSlideToNote, 0);
            if (!data)
              putcmd(cp, cmdSpecial, cmdContVolSlide);
            else
            if (data&0xF0)
              putcmd(cp, cmdVolSlideUp, (data>>4)<<1);
            else
              putcmd(cp, cmdVolSlideDown, (data&0xF)<<1);
            break;
          case 0x16:
            putcmd(cp, cmdPitchVibrato, 0);
            if (!data)
              putcmd(cp, cmdSpecial, cmdContVolSlide);
            else
            if (data&0xF0)
              putcmd(cp, cmdVolSlideUp, (data>>4)<<1);
            else
              putcmd(cp, cmdVolSlideDown, (data&0xF)<<1);
            break;
          case 0x18:
            if (!data)
              putcmd(cp, cmdPanSlide, 0);
            else
            if (data&0xF0)
              putcmd(cp, cmdPanSlide, data>>4);
            else
              putcmd(cp, cmdPanSlide, -(data&0xF));
/*
            if ((data&0x0F)&&(data&0xF0))
              break;
            if (data&0xF0)
              data=-(data>>4)*4;
            else
              data=data*4;
            putcmd(cp, cmdPanSlide, data);
            break;
*/
          case 0x1A:
            if (!data)
              putcmd(cp, cmdSpecial, cmdContVolSlide);
            else
            if (data&0xF0)
              putcmd(cp, cmdVolSlideUp, (data>>4)<<1);
            else
              putcmd(cp, cmdVolSlideDown, (data&0xF)<<1);
            break;
          case 0x1E:
            cmds[j][0]=data>>4;
            data&=0x0F;
            switch (cmds[j][0])
	    {
            case 0x1:
              putcmd(cp, cmdRowPitchSlideUp, data<<4);
              break;
            case 0x2:
              putcmd(cp, cmdRowPitchSlideDown, data<<4);
              break;
            case 0xA:
              putcmd(cp, cmdRowVolSlideUp, data<<1);
              break;
            case 0xB:
              putcmd(cp, cmdRowVolSlideDown, data<<1);
              break;
            }
            break;
          case 0x1C:
            putcmd(cp, cmdChannelVol, (data<=0x7F)?(data<<1):0xFF);
            break;
          case 0x20:
            putcmd(cp, cmdKeyOff, data);
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

      gmdtrack &trk=m.tracks[t*33+i];
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
    unsigned char *buf=buffer;

    unsigned char *cp=tp+2;

    if (ordlist[0]==t)
    {
      if (hdr.bpm&0xFF00)
        putcmd(cp, cmdSpeed, hdr.bpm>>8);
      if (hdr.bpm&0xFF)
        putcmd(cp, cmdFineSpeed, hdr.bpm);
      putcmd(cp, cmdTempo, hdr.speed);
    }

    unsigned short row=0;
    unsigned char another=1;
    while (row<=maxrow)
    {
      if (!another||(*buf==0xFF))
      {
        if (another)
          buf++;
        if (cp!=(tp+2))
        {
          tp[0]=row;
          tp[1]=cp-tp-2;
          tp=cp;
          cp=tp+2;
        }
        row++;
        another=1;
        continue;
      }
      another=!(*buf&0x80);
      unsigned char curchan=*buf&0x1F;
      unsigned char anothercmd=1;
      unsigned char cmds[7][2];
      unsigned char cmdnum=0;
      if (!(*buf++&0x40))
      {
        anothercmd=*buf&0x80;
        buf+=2;
      }
      while (anothercmd)
      {
        anothercmd=*buf&0x80;
        cmds[cmdnum][0]=*buf++&0x7F;
        if (!(cmds[cmdnum][0]&0x40))
          cmds[cmdnum++][1]=*buf++;
      }

      if (curchan>=chan)
        continue;

      for (j=0; j<cmdnum; j++)
      {
        unsigned char data=cmds[j][1];
        switch (cmds[j][0])
        {
        case 0xB:
          putcmd(cp, cmdGoto, data);
          break;
	case 0xD:
          putcmd(cp, cmdBreak, (data&0x0F)+(data>>4)*10);
          break;
	case 0x1D:
          putcmd(cp, cmdBreak, data);
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
        case 0x1F:
          if (data<10)
            putcmd(cp, cmdFineSpeed, data);
          break;
        case 0x2A:
          if ((data&0x0F)&&(data&0xF0))
            break;
          putcmd(cp, cmdSetChan, curchan);
          if (data&0xF0)
            putcmd(cp, cmdGlobVolSlide, (data>>4)<<2);
          else
            putcmd(cp, cmdGlobVolSlide, -(data<<2));
        case 0x2C:
          putcmd(cp, cmdGlobVol, data*2);
          break;
        }
      }
    }

    gmdtrack &trk=m.tracks[t*33+32];
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

  delete ordlist;
  delete temptrack;
  delete buffer;

  if (!mpAllocSamples(m, m.sampnum)||!mpAllocModSamples(m, m.modsampnum))
    return errAllocMem;

  m.sampnum=0;
  m.modsampnum=0;

  for (i=0; i<m.instnum; i++)
  {
    gmdinstrument &ip=m.instruments[i];
    for (j=0; j<instsampnum[i]; j++)
    {
      m.modsamples[m.modsampnum++]=msmps[i][j];
      m.samples[m.sampnum++]=smps[i][j];
    }
    delete msmps[i];
    delete smps[i];
  }

  delete smps;
  delete msmps;

  int sampnum=0;

  for (i=0; i<m.instnum; i++)
  {
    gmdinstrument &ip=m.instruments[i];
    if (shadowedby[i])
    {
      sampnum+=instsampnum[i];
      continue;
    }
    for (j=0; j<instsampnum[i]; j++)
    {
      sampleinfo &sip=m.samples[sampnum++];
      if (!sip.length)
        continue;

      unsigned long packlena,packlenb;
      unsigned char packbyte;

      packlena=file.getl();
      packlenb=file.getl();
      packbyte=file.getc();

      unsigned char *packb=new unsigned char[((packlenb>packlena)?packlenb:packlena)+16];
      unsigned char *smpp=new unsigned char[packlena+16];
      if (!smpp||!packb)
        return errAllocMem;
      file.read(packb, packlenb);
      long p1,p2;
      p1=p2=0;


      while (p2<packlena)
        if (packb[p1]!=packbyte)
          smpp[p2++]=packb[p1++];
        else
          if (!packb[++p1])
            smpp[p2++]=packb[p1++-1];
          else
          {
            memset(smpp+p2, packb[p1+1], packb[p1]);
            p2+=packb[p1];
            p1+=2;
          }
      memset(packb, 0, packlena);

      p1=0;
      unsigned char bitsel=0x80;

      for (p2=0; p2<packlena; p2++)
      {
        unsigned char cur=smpp[p2];

        for (t=0; t<8; t++)
        {
          packb[p1++]|=cur&bitsel;
          cur=(cur<<1)|((cur&0x80)>>7);
          if (p1==packlena)
          {
            p1=0;
            cur=(cur>>1)|((cur&1)<<7);
            bitsel>>=1;
          }
        }
      }

      signed char cursmp=0;
      p1=0;
      for (p2=0; p2<packlena; p2++)
      {
        unsigned char cur=packb[p2];
        cursmp+=-((cur&0x80)?((-cur)|0x80):cur);
        smpp[p1++]=cursmp;
      }

      delete packb;

      sip.ptr=smpp;
      m.modsamples[sampnum-1].handle=sampnum-1;
    }
  }


//  for (i=0; i<m.instnum; i++)
//    if (shadowedby[i])
//    {
//      for (j=0; j<m.instruments[i].sampnum; j++)
//      {
//        m.modsamples[m.instruments[i].samples[j]].handle=m.modsamples[m.instruments[shadowedby[i]-1].samples[j]].handle;
//      }
//    }

  delete instsampnum;

  return errOk;
}
