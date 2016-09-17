// (c) 1994-1997 Niklas Beisert, this file is part of "Indoor Music System".
// you must not read, modify, print, compile or copy this file or parts of
// it unless you have accepted the license in the accompanying file IMS.TXT.

#include <stdlib.h>
#include <string.h>
#include "mcp.h"
#include "itplay.h"

itplayerclass *itplayerclass::staticthis;
signed char itplayerclass::sintab[256]=
{
    0,  2,  3,  5,  6,  8,  9, 11, 12, 14, 16, 17, 19, 20, 22, 23,
   24, 26, 27, 29, 30, 32, 33, 34, 36, 37, 38, 39, 41, 42, 43, 44,
   45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 56, 57, 58, 59,
   59, 60, 60, 61, 61, 62, 62, 62, 63, 63, 63, 64, 64, 64, 64, 64,
   64, 64, 64, 64, 64, 64, 63, 63, 63, 62, 62, 62, 61, 61, 60, 60,
   59, 59, 58, 57, 56, 56, 55, 54, 53, 52, 51, 50, 49, 48, 47, 46,
   45, 44, 43, 42, 41, 39, 38, 37, 36, 34, 33, 32, 30, 29, 27, 26,
   24, 23, 22, 20, 19, 17, 16, 14, 12, 11,  9,  8,  6,  5,  3,  2,
    0, -2, -3, -5, -6, -8, -9,-11,-12,-14,-16,-17,-19,-20,-22,-23,
  -24,-26,-27,-29,-30,-32,-33,-34,-36,-37,-38,-39,-41,-42,-43,-44,
  -45,-46,-47,-48,-49,-50,-51,-52,-53,-54,-55,-56,-56,-57,-58,-59,
  -59,-60,-60,-61,-61,-62,-62,-62,-63,-63,-63,-64,-64,-64,-64,-64,
  -64,-64,-64,-64,-64,-64,-63,-63,-63,-62,-62,-62,-61,-61,-60,-60,
  -59,-59,-58,-57,-56,-56,-55,-54,-53,-52,-51,-50,-49,-48,-47,-46,
  -45,-44,-43,-42,-41,-39,-38,-37,-36,-34,-33,-32,-30,-29,-27,-26,
  -24,-23,-22,-20,-19,-17,-16,-14,-12,-11, -9, -8, -6, -5, -3, -2,
};

void itplayerclass::playtickstatic()
{
  staticthis->playtick();
}

void itplayerclass::playnote(logchan &c, const unsigned char *cmd)
{
  if (cmd[0])
    c.lastnote=cmd[0];
  int instchange=0;
  if (cmd[1])
  {
    if (cmd[1]!=c.lastins)
      instchange=1;
    c.lastins=cmd[1];
  }
  if (c.lastnote==cmdNNoteOff)
  {
    if (c.pch)
      c.pch->noteoff=1;
    return;
  }
  if (c.lastnote==cmdNNoteCut)
  {
    if (c.pch)
      c.pch->notecut=1;
    return;
  }
  if (c.lastnote>=cmdNNoteFade)
  {
    if (c.pch)
      c.pch->notefade=1;
    return;
  }
  if ((c.lastins>ninst)||!c.lastins||!c.lastnote||(c.lastnote>=cmdNNoteFade))
    return;
  int porta=(cmd[3]==cmdPortaNote)||(cmd[3]==cmdPortaVol)||((cmd[2]>=cmdVPortaNote)&&(cmd[2]<(cmdVPortaNote+10)));
  if (!c.pch||c.pch->dead)
    porta=0;
  if (instchange||!porta)
  {
    if (porta)
      c.pch->notecut=1;
    int smp=instruments[c.lastins-1].keytab[c.lastnote-cmdNNote][1];
    if (!smp||(smp>nsamp))
      return;
    if (!porta)
      if (c.pch)
      {
        switch (c.nna)
        {
        case 0: c.pch->notecut=1; break;
        case 2: c.pch->noteoff=1; break;
        case 3: c.pch->notefade=1; break;
        }
        c.pch=0;
      }
    if (samples[smp-1].handle==0xFFFF)
      return;
    if (!porta)
      c.pch=&c.newchan;
    c.pch->inst=&instruments[c.lastins-1];
    c.pch->smp=&samples[smp-1];
    c.pch->note=c.lastnote-cmdNNote;
  }
  physchan &p=*c.pch;
  const sample &s=*p.smp;
  const instrument &in=*p.inst;
  if (cmd[3]==cmdOffset)
  {
    if (cmd[4])
      c.offset=(c.offset&0xF00)|cmd[4];
    p.newsamp=s.handle;
    p.newpos=c.offset<<8;
  }
  else
    if (!porta||instchange)
    {
      p.newsamp=s.handle;
      p.newpos=0;
    }
  if (!porta||instchange)
  {
    int i;
    if (in.dct)
    {
      for (i=0; i<npchan; i++)
      {
        physchan &dp=pchannels[i];
        if (&dp==&p)
          continue;
        if (dp.lch!=p.lch)
          continue;
        if (dp.inst!=p.inst)
          continue;
        if ((in.dct!=3)&&(dp.smp!=p.smp))
          continue;
        if ((in.dct==1)&&(dp.note!=p.note))
          continue;
        switch (in.dca)
        {
        case 0: dp.notecut=1; break;
        case 1: dp.noteoff=1; break;
        case 2: dp.notefade=1; break;
        }
      }
    }

    p.fadeval=1024;
    p.fadespd=in.fadeout;
    p.notefade=0;
    p.dead=0;
    p.notecut=0;
    p.noteoff=0;
    p.panenvpos=0;
    p.pitchenvpos=0;
    p.avibpos=0;
    p.avibdep=0;
    p.volenvpos=0;
    p.looptype=0;
    p.volenv=in.envs[0].type&envelope::active;
    p.panenv=in.envs[1].type&envelope::active;
    p.pitchenv=in.envs[2].type&envelope::active;
    c.nna=in.nna;
    c.vol=s.vol;
  // volume swing!!!
    c.fvol=c.vol;
    c.pan=(in.dfp&128)?c.cpan:in.dfp;
    c.pan=(s.dfp&128)?(s.dfp&127):c.pan;
    c.pan=range64(c.pan+(((c.lastnote-cmdNNote-in.ppc)*in.pps)>>8));
  // panning swing!!!
    c.fpan=c.pan;
    c.basenote=c.lastnote-cmdNNote;
    c.realnote=in.keytab[c.basenote][0];
    p.noteoffset=(60-c.realnote+c.basenote)*256+s.normnote;
  // geffect!!!
  }
  c.dpitch=p.noteoffset-256*(c.lastnote-cmdNNote);
  if (!porta)
    c.fpitch=c.pitch=c.dpitch;
  // amiga!!!
}

void itplayerclass::playvcmd(logchan &c, int vcmd)
{
  c.vcmd=vcmd;
  if ((vcmd>=cmdVVolume)&&(vcmd<=(cmdVVolume+64)))
    c.fvol=c.vol=vcmd-cmdVVolume;
  else
  if ((vcmd>=cmdVPanning)&&(vcmd<=(cmdVPanning+64)))
  {
    c.fpan=c.pan=c.cpan=vcmd-cmdVPanning;
    c.srnd=0;
  }
  else
  if ((vcmd>=cmdVFVolSlU)&&(vcmd<(cmdVFVolSlU+10)))
  {
    if (vcmd!=cmdVFVolSlU)
      c.vvolslide=vcmd-cmdVFVolSlU;
    c.fvol=c.vol=range64(c.vol+c.vvolslide);
  }
  else
  if ((vcmd>=cmdVFVolSlD)&&(vcmd<(cmdVFVolSlD+10)))
  {
    if (vcmd!=cmdVFVolSlD)
      c.vvolslide=vcmd-cmdVFVolSlD;
    c.fvol=c.vol=range64(c.vol-c.vvolslide);
  }
  else
  if ((vcmd>=cmdVVolSlU)&&(vcmd<(cmdVVolSlU+10)))
  {
    if (vcmd!=cmdVVolSlU)
      c.vvolslide=vcmd-cmdVVolSlU;
  }
  else
  if ((vcmd>=cmdVVolSlD)&&(vcmd<(cmdVVolSlD+10)))
  {
    if (vcmd!=cmdVVolSlD)
      c.vvolslide=vcmd-cmdVVolSlD;
  }
  else
  if ((vcmd>=cmdVPortaD)&&(vcmd<(cmdVPortaD+10)))
  {
    if (vcmd!=cmdVPortaD)
      c.porta=4*(vcmd-cmdVPortaD);
  }
  else
  if ((vcmd>=cmdVPortaU)&&(vcmd<(cmdVPortaU+10)))
  {
    if (vcmd!=cmdVPortaU)
      c.porta=4*(vcmd-cmdVPortaU);
  }
  else
  if ((vcmd>=cmdVPortaNote)&&(vcmd<(cmdVPortaNote+10)))
  {
    if (vcmd!=cmdVPortaNote)
    {
      c.portanote="\x00\x01\x04\x08\x10\x20\x40\x60\x80\xFF"[vcmd-cmdVPortaNote];
      if (!geffect)
        c.porta=c.portanote;
    }
  }
  else
  if ((vcmd>=cmdVVibrato)&&(vcmd<(cmdVVibrato+10)))
  {
    if (vcmd!=cmdVVibrato)
      c.vibdep=(vcmd-cmdVVibrato)*4;
    dovibrato(c);
  }
}

int itplayerclass::range64(int v)
{
  return (v<0)?0:(v>64)?64:v;
}

int itplayerclass::range128(int v)
{
  return (v<0)?0:(v>128)?128:v;
}

int itplayerclass::rowslide(int data)
{
  if ((data&0x0F)==0x0F)
    return data>>4;
  else
  if ((data&0xF0)==0xF0)
    return -(data&0xF);
  return 0;
}

int itplayerclass::rowudslide(int data)
{
  if (data>=0xF0)
    return (data&0xF)*4;
  else
  if (data>=0xE0)
    return data&0xF;
  return 0;
}

int itplayerclass::rowvolslide(int data)
{
  if (data==0xF0)
    return 0xF;
  else
  if (data==0x0F)
    return -0xF;
  else
    return rowslide(data);
}

int itplayerclass::tickslide(int data)
{
  if (!(data&0x0F))
    return data>>4;
  else
  if (!(data&0xF0))
    return -(data&0xF);
  return 0;
}

void itplayerclass::doretrigger(logchan &c)
{
  c.retrigpos++;
  if (c.retrigpos<c.retrigspd)
    return;
  c.retrigpos=0;
  int x=c.vol;
  switch (c.retrigvol)
  {
  case 1: case 2: case 3: case 4: case 5: x-=1<<(c.retrigvol-1); break;
  case 6: x=(5*x)>>3; break;
  case 7: x=x>>1; break;
  case 9: case 10: case 11: case 12: case 13: x+=1<<(c.retrigvol-9); break;
  case 14: x=(3*x)>>1; break;
  case 15: x=2*x; break;
  }
  c.fvol=c.vol=range64(x);
  if (!c.pch)
    return;
  physchan &p=*c.pch;
  p.newpos=0;
  p.dead=0;
}

void itplayerclass::dotremor(logchan &c)
{
  if (c.tremorpos>=c.tremoroff)
    c.fvol=0;
  c.tremorpos++;
  if (c.tremorpos>=c.tremorlen)
    c.tremorpos=0;
}

void itplayerclass::playcmd(logchan &c, int cmd, int data)
{
  int i;
  c.command=cmd;
  switch (cmd)
  {
  case cmdSpeed:
    if (data)
      speed=data;
    putque(queSpeed, -1, speed);
    break;
  case cmdJump:
    gotorow=0;
    gotoord=data;
    break;
  case cmdBreak:
    if (gotoord==-1)
      gotoord=curord+1;
    gotorow=data;
    break;
  case cmdVolSlide:
    if (data)
      c.volslide=data;
    c.fvol=c.vol=range64(c.vol+rowvolslide(c.volslide));
    break;
  case cmdPortaD:
    if (data)
      c.porta=data;
    c.fpitch=c.pitch=c.pitch+rowudslide(c.porta);
    break;
  case cmdPortaU:
    if (data)
      c.porta=data;
    c.fpitch=c.pitch=c.pitch-rowudslide(c.porta);
    break;
  case cmdPortaNote:
    if (data)
    {
      c.portanote=data;
      if (!geffect)
        c.porta=c.portanote;
    }
    break;
  case cmdVibrato:
    if (data&0xF)
      c.vibdep=(data&0xF)*4;
    if (data>>4)
      c.vibspd=data>>4;
    dovibrato(c);
    break;
  case cmdTremor:
    if (data)
    {
      c.tremoroff=(data>>4)+1;
      c.tremorlen=c.tremoroff+(data&0xF)+1;
    }
    dotremor(c);
    break;
  case cmdArpeggio:
    if (data)
    {
      c.arpeggio1=data>>4;
      c.arpeggio2=data&0xF;
    }
    break;
  case cmdPortaVol:
    if (data)
      c.volslide=data;
    c.fvol=c.vol=range64(c.vol+rowvolslide(c.volslide));
    break;
  case cmdVibVol:
    if (data)
      c.volslide=data;
    c.fvol=c.vol=range64(c.vol+rowvolslide(c.volslide));
    dovibrato(c);
    break;
  case cmdChanVol:
    if (data<=64)
      c.cvol=data;
    break;
  case cmdChanVolSlide:
    if (data)
      c.cvolslide=data;
    c.cvol=range64(c.cvol+rowslide(c.cvolslide));
    break;
  case cmdOffset:
    break;
  case cmdPanSlide:
    if (data)
      c.panslide=data;
    c.fpan=c.cpan=c.pan=range64(c.pan-rowslide(c.panslide));
    break;
  case cmdRetrigger:
    if (data)
    {
      c.retrigspd=data&0xF;
      c.retrigvol=data>>4;
    }
    doretrigger(c);
    break;
  case cmdTremolo:
    if (data&0xF)
      c.tremdep=data&0xF;
    if (data>>4)
      c.tremspd=data>>4;
    dotremolo(c);
    break;
  case cmdSpecial:
    switch (c.specialcmd)
    {
    case cmdSVibType:
      if (c.specialdata<4)
        c.vibtype=c.specialdata;
      break;
    case cmdSTremType:
      if (c.specialdata<4)
        c.tremtype=c.specialdata;
      break;
    case cmdSPanbrType:
      if (c.specialdata<4)
        c.panbrtype=c.specialdata;
      break;
    case cmdSPatDelayTick:
      patdelaytick=c.specialdata;
      break;
    case cmdSInstFX:
      if (!instmode)
        break;
      switch (c.specialdata)
      {
      case cmdSIPastCut:
        for (i=0; i<npchan; i++)
          if ((c.newchan.lch==pchannels[i].lch)&&(c.pch!=&pchannels[i]))
            pchannels[i].notecut=1;
        break;
      case cmdSIPastOff:
        for (i=0; i<npchan; i++)
          if ((c.newchan.lch==pchannels[i].lch)&&(c.pch!=&pchannels[i]))
            pchannels[i].noteoff=1;
        break;
      case cmdSIPastFade:
        for (i=0; i<npchan; i++)
          if ((c.newchan.lch==pchannels[i].lch)&&(c.pch!=&pchannels[i]))
            pchannels[i].notefade=1;
        break;
      case cmdSINNACut:
        c.nna=0;
        break;
      case cmdSINNACont:
        c.nna=1;
        break;
      case cmdSINNAOff:
        c.nna=2;
        break;
      case cmdSINNAFade:
        c.nna=3;
        break;
      case cmdSIVolEnvOff:
        if (c.pch)
          c.pch->volenv=0;
        break;
      case cmdSIVolEnvOn:
        if (c.pch)
          c.pch->volenv=1;
        break;
      case cmdSIPanEnvOff:
        if (c.pch)
          c.pch->panenv=0;
        break;
      case cmdSIPanEnvOn:
        if (c.pch)
          c.pch->panenv=1;
        break;
      case cmdSIPitchEnvOff:
        if (c.pch)
          c.pch->pitchenv=0;
        break;
      case cmdSIPitchEnvOn:
        if (c.pch)
          c.pch->pitchenv=1;
        break;
      }
      break;
    case cmdSPanning:
      c.srnd=0;
      c.fpan=c.pan=c.cpan=((c.specialdata*0x11)+1)>>2;
      break;
    case cmdSSurround:
      if (c.specialdata==1)
        c.srnd=1;
      break;
    case cmdSOffsetHigh:
      c.offset=(c.offset&0xFF)|(c.specialdata<<8);
      break;
    case cmdSPatLoop:
      if (!c.specialdata)
        c.patloopstart=currow;
      else
      {
        c.patloopcount++;
        if (c.patloopcount<=c.specialdata)
        {
          gotorow=c.patloopstart;
          gotoord=curord;
        }
        else
        {
          c.patloopcount=0;
          c.patloopstart=currow+1;
        }
      }
      break;
    case cmdSNoteCut:
      break;
    case cmdSNoteDelay:
      dodelay(c);
      break;
    case cmdSPatDelayRow:
      patdelayrow=c.specialdata;
      break;
    }
    break;
  case cmdTempo:
    if (data)
      c.tempo=data;
    if (c.tempo>=0x20)
    {
      tempo=c.tempo;
      putque(queTempo, -1, tempo);
    }
    break;
  case cmdFineVib:
    if (data&0xF)
      c.vibdep=data&0xF;
    if (data>>4)
      c.vibspd=data>>4;
    dovibrato(c);
    break;
  case cmdGVolume:
    if (data<=128)
      gvol=data;
    putque(queGVol, -1, gvol);
    break;
  case cmdGVolSlide:
    if (data)
      c.gvolslide=data;
    gvol=range128(gvol+rowslide(c.gvolslide));
    putque(queGVol, -1, gvol);
    break;
  case cmdPanning:
    c.srnd=0;
    c.fpan=c.cpan=c.pan=(data+1)>>2;
    break;
  case cmdPanbrello:
    if (data&0xF)
      c.panbrdep=data&0xF;
    if (data>>4)
      c.panbrspd=data>>4;
    dopanbrello(c);
    break;
  case cmdSync:
    putque(queSync, c.newchan.lch, data);
    break;
  }
}

void itplayerclass::dovibrato(logchan &c)
{
  int x;
  switch (c.vibtype)
  {
  case 0: x=sintab[4*(c.vibpos&63)]>>1; break;
  case 1: x=32-(c.vibpos&63); break;
  case 2: x=32-(c.vibpos&32); break;
  case 3: x=(random()&63)-32;
  }
  c.fpitch-=(c.vibdep*x)>>3;
  c.vibpos+=c.vibspd;
}

void itplayerclass::dotremolo(logchan &c)
{
  int x;
  switch (c.tremtype)
  {
  case 0: x=sintab[4*(c.trempos&63)]>>1; break;
  case 1: x=32-(c.trempos&63); break;
  case 2: x=32-(c.trempos&32); break;
  case 3: x=(random()&63)-32;
  }
  c.fvol=range64(c.fvol+((c.tremdep*x)>>4));
  c.trempos+=c.tremspd;
}

void itplayerclass::dopanbrello(logchan &c)
{
  if (c.panbrtype==3)
  {
    if (c.panbrpos>=c.panbrspd)
    {
      c.panbrpos=0;
      c.panbrrnd=random();
    }
    c.fpan=range64(c.fpan+((c.panbrdep*((c.panbrrnd&255)-128))>>6));
    return;
  }
  int x;
  switch (c.panbrtype)
  {
  case 0: x=sintab[c.panbrpos&255]*2; break;
  case 1: x=128-(c.panbrpos&255); break;
  case 2: x=128-2*(c.panbrpos&128); break;
  }
  c.fpan=range64(c.fpan+((c.panbrdep*x)>>6));
  c.panbrpos+=c.panbrspd;
}

void itplayerclass::doportanote(logchan &c)
{
  if (c.pitch<c.dpitch)
  {
    c.pitch+=(geffect?c.portanote:c.porta)*16;
    if (c.pitch>c.dpitch)
      c.pitch=c.dpitch;
  }
  else
  {
    c.pitch-=(geffect?c.portanote:c.porta)*16;
    if (c.pitch<c.dpitch)
      c.pitch=c.dpitch;
  }
  c.fpitch=c.pitch;
}

void itplayerclass::dodelay(logchan &c)
{
  if (curtick==c.specialdata)
  {
    if (c.delayed[0]||c.delayed[1])
      playnote(c, c.delayed);
    if (c.delayed[2])
      playvcmd(c, c.delayed[2]);
  }
}

int itplayerclass::rangepitch(int p)
{
  return (p<pitchhigh)?pitchhigh:(p>pitchlow)?pitchlow:p;
}

void itplayerclass::processfx(logchan &c)
{
  if ((c.vcmd>=cmdVVolSlD)&&(c.vcmd<(cmdVVolSlD+10)))
    c.fvol=c.vol=range64(c.vol-c.vvolslide);
  else
  if ((c.vcmd>=cmdVVolSlU)&&(c.vcmd<(cmdVVolSlU+10)))
    c.fvol=c.vol=range64(c.vol+c.vvolslide);
  else
  if ((c.vcmd>=cmdVVibrato)&&(c.vcmd<(cmdVVibrato+10)))
    dovibrato(c);
  else
  if ((c.vcmd>=cmdVPortaNote)&&(c.vcmd<(cmdVPortaNote+10)))
    doportanote(c);
  else
  if ((c.vcmd>=cmdVPortaU)&&(c.vcmd<(cmdVPortaU+10)))
  {
    c.fpitch=c.pitch=rangepitch(c.pitch-c.porta*16);
    if ((c.pitch==pitchhigh)&&c.pch)
      c.pch->notecut=1;
  }
  else
  if ((c.vcmd>=cmdVPortaD)&&(c.vcmd<(cmdVPortaD+10)))
    c.fpitch=c.pitch=rangepitch(c.pitch+c.porta*16);

  switch (c.command)
  {
  case cmdSpeed:
    break;
  case cmdJump:
    break;
  case cmdBreak:
    break;
  case cmdVolSlide:
    c.vol=range64(c.vol+tickslide(c.volslide));
    break;
  case cmdPortaD:
    if (c.porta>=0xE0)
      break;
    c.fpitch=c.pitch=rangepitch(c.pitch+c.porta*16);
    break;
  case cmdPortaU:
    if (c.porta>=0xE0)
      break;
    c.fpitch=c.pitch=rangepitch(c.pitch-c.porta*16);
    if ((c.pitch==pitchhigh)&&c.pch)
      c.pch->notecut=1;
    break;
  case cmdPortaNote:
    doportanote(c);
    break;
  case cmdVibrato:
    dovibrato(c);
    break;
  case cmdTremor:
    dotremor(c);
    break;
  case cmdArpeggio:
    switch (curtick%3)
    {
    case 1:
      c.fpitch-=c.arpeggio1<<8;
      break;
    case 2:
      c.fpitch-=c.arpeggio2<<8;
      break;
    }
    break; //amiga!!!
  case cmdPortaVol:
    doportanote(c);
    c.fvol=c.vol=range64(c.vol+tickslide(c.volslide));
    break;
  case cmdVibVol:
    dovibrato(c);
    c.fvol=c.vol=range64(c.vol+tickslide(c.volslide));
    break;
  case cmdChanVol:
    break;
  case cmdChanVolSlide:
    c.cvol=range64(c.cvol+tickslide(c.cvolslide));
    break;
  case cmdOffset:
    break;
  case cmdPanSlide:
    c.fpan=c.cpan=c.pan=range64(c.pan-tickslide(c.panslide));
    break;
  case cmdRetrigger:
    doretrigger(c);
    break;
  case cmdTremolo:
    dotremolo(c);
    break;
  case cmdSpecial:
    switch (c.specialcmd)
    {
    case cmdSVibType:
      break;
    case cmdSTremType:
      break;
    case cmdSPanbrType:
      break;
    case cmdSPatDelayTick:
      break;
    case cmdSInstFX:
      break;
    case cmdSPanning:
      break;
    case cmdSSurround:
      break;
    case cmdSOffsetHigh:
      break;
    case cmdSPatLoop:
      break;
    case cmdSNoteCut:
      if ((curtick>=c.specialdata)&&c.pch)
        c.pch->notecut=1;
      break;
    case cmdSNoteDelay:
      dodelay(c);
      break;
    case cmdSPatDelayRow:
      break;
    }
    break;
  case cmdTempo:
    if (c.tempo<0x20)
    {
      tempo+=(c.tempo<0x10)?-c.tempo:(c.tempo&0xF);
      tempo=(tempo<0x20)?0x20:(tempo>0xFF)?0xFF:tempo;
      putque(queTempo, -1, tempo);
    }
    break;
  case cmdFineVib:
    dovibrato(c);
    break;
  case cmdGVolume:
    break;
  case cmdGVolSlide:
    gvol=range128(gvol+tickslide(c.gvolslide));
    putque(queGVol, -1, gvol);
    break;
  case cmdPanning:
    break;
  case cmdPanbrello:
    dopanbrello(c);
    break;
  }
}

void itplayerclass::inittickchan(physchan &p)
{
  p.fvol=p.vol;
  p.fpan=p.pan;
  p.fpitch=p.pitch;
}

void itplayerclass::inittick(logchan &c)
{
  c.fvol=c.vol;
  c.fpan=c.pan;
  c.fpitch=c.pitch;
}

void itplayerclass::initrow(logchan &c)
{
  c.command=0;
  c.vcmd=0;
}

void itplayerclass::updatechan(logchan &c)
{
  if (!c.pch)
    return;
  physchan &p=*c.pch;
  p.vol=(c.vol*c.cvol)>>4;
  p.fvol=(c.fvol*c.cvol)>>4;
  p.pan=c.pan*4-128;
  p.fpan=c.fpan*4-128;
// amiga!!!
  p.pitch=-c.pitch;
  p.fpitch=-c.fpitch;
  p.srnd=c.srnd;
}

int itplayerclass::processenvelope(const envelope &env, int &pos, int noteoff, int active)
{
  int i,x;
  for (i=0; i<env.len; i++)
    if (pos<env.x[i+1])
      break;

  x=256*env.y[i]+((pos==env.x[i])?0:(256*(env.y[i+1]-env.y[i])*(pos-env.x[i])/(env.x[i+1]-env.x[i])));

  if (active)
    pos++;
  if (!noteoff&&(env.type&env.slooped))
  {
    if (pos==(env.x[env.sloope]+((env.sloope==env.sloops)?1:0)))
      pos=env.x[env.sloops];
  }
  else
    if (env.type&env.looped)
    {
      if (pos==(env.x[env.loope]+((env.loope==env.loops)?1:0)))
        pos=env.x[env.loops];
    }
  if (pos>env.x[env.len])
    pos=env.x[env.len];

  return x;
}

void itplayerclass::processchan(physchan &p)
{
  if (p.volenvpos||p.volenv)
    p.fvol=(processenvelope(p.inst->envs[0], p.volenvpos, p.noteoff, p.volenv)*p.fvol)>>14;

  if (p.volenv)
  {
    const envelope &env=p.inst->envs[0];
    if (p.noteoff&&(p.inst->envs[0].type&envelope::looped))
      p.notefade=1;
    if ((p.volenvpos==env.x[env.len])&&!(p.inst->envs[0].type&envelope::looped)&&(!(p.inst->envs[0].type&envelope::slooped)||p.noteoff))
    {
      if (!env.y[env.len])
        p.notecut=1;
      else
        p.notefade=1;
    }
  }
  else
    if (p.noteoff)
      p.notefade=1;

  p.fvol=(p.fvol*p.fadeval)>>10;
  p.fadeval-=p.notefade?(p.fadeval>p.fadespd)?p.fadespd:p.fadeval:0;
  if (!p.fadeval)
    p.notecut=1;

  p.fvol=(gvol*p.fvol)>>7;
  p.fvol=(p.smp->gvl*p.fvol)>>6;
  p.fvol=(p.inst->gbv*p.fvol)>>7;


  if (p.panenvpos||p.panenv)
    p.fpan+=processenvelope(p.inst->envs[1], p.panenvpos, p.noteoff, p.panenv)>>6;

  if (p.srnd)
    p.fpan=0;

  p.fpan=(chsep*p.fpan)>>7;


  if (p.pitchenvpos||p.pitchenv)
    p.fpitch+=processenvelope(p.inst->envs[2], p.pitchenvpos, p.noteoff, p.pitchenv)>>1;

  int x;
  switch (p.smp->vit)
  {
  case 0: x=sintab[p.avibpos&255]*2; break;
  case 1: x=128-(p.avibpos&255); break;
  case 2: x=128-(p.avibpos&128); break;
  case 3: x=(random()&255)-128;
  }
  p.fpitch+=(x*p.avibdep)>>13;
  p.avibpos+=p.smp->vis;
  p.avibdep+=p.smp->vir;
  if (p.avibdep>(p.smp->vid*256))
    p.avibdep=p.smp->vid*256;
}

void itplayerclass::putchandata(physchan &p)
{
  if (p.newsamp!=-1)
  {
    mcpSet(p.no, mcpCReset, 0);
    mcpSet(p.no, mcpCInstrument, p.newsamp);
    p.newsamp=-1;
  }
  if (p.newpos!=-1)
  {
    mcpSet(p.no, mcpCPosition, p.newpos);
    mcpSet(p.no, mcpCLoop, 1);
    mcpSet(p.no, mcpCDirect, 0);
    mcpSet(p.no, mcpCStatus, 1);
    p.newpos=-1;
    p.dead=0;
  }
  if (p.noteoff&&!p.looptype)
  {
    mcpSet(p.no, mcpCLoop, 2);
    p.looptype=1;
  }
  mcpSet(p.no, mcpCPitch, p.fpitch);
  mcpSet(p.no, mcpCVolume, p.fvol);
  mcpSet(p.no, mcpCPanning, p.fpan);
  mcpSet(p.no, mcpCSurround, p.srnd);
  mcpSet(p.no, mcpCMute, channels[p.lch].mute);
}

void itplayerclass::mutechan(int c, int m)
{
  if ((c>=0)||(c<nchan))
    channels[c].mute=m;
}

void itplayerclass::allocatechan(logchan &c)
{
  if (c.disabled)
  {
    c.pch=0;
    return;
  }
  int i;
  for (i=0; i<npchan; i++)
    if (pchannels[i].lch==-1)
      break;
  if (i==npchan)
    for (i=0; i<npchan; i++)
      if (pchannels[i].dead)
        break;
//!!!
  if (i==npchan)
    i=random()%npchan;
  if (pchannels[i].lch!=-1)
  {
    if (channels[pchannels[i].lch].pch==&pchannels[i])
      channels[pchannels[i].lch].pch=0;
  }
  pchannels[i]=c.newchan;
  c.pch=&pchannels[i];
  c.pch->no=i;
}

void itplayerclass::getproctime()
{
  proctime=mcpGet(-1, mcpGCmdTimer);
}

void itplayerclass::putglobdata()
{
  mcpSet(-1, mcpGSpeed, 256*2*tempo/5);
}

void itplayerclass::putque(int type, int val1, int val2)
{
  if (((quewpos+1)%quelen)==querpos)
    return;
  que[quewpos][0]=proctime;
  que[quewpos][1]=type;
  que[quewpos][2]=val1;
  que[quewpos][3]=val2;
  quewpos=(quewpos+1)%quelen;
}

void itplayerclass::readque()
{
  int i;
  int time=gettime();
  while (1)
  {
    if (querpos==quewpos)
      break;
    int t=que[querpos][0];
    if (time<t)
      break;
    int val1=que[querpos][2];
    int val2=que[querpos][3];
    switch (que[querpos][1])
    {
    case queSync:
      realsync=val2;
      realsynctime=t;
      channels[val1].realsync=val2;
      channels[val1].realsynctime=t;
      break;
    case quePos:
      realpos=val2;
      for (i=0; i<nchan; i++)
      {
        logchan &c=channels[i];
        if (c.evpos==-1)
        {
          if (c.evpos0==realpos)
          {
            c.evpos=realpos;
            c.evtime=t;
          }
        }
        else
        {
          switch (c.evmodtype)
          {
          case 1:
            c.evmodpos++;
            break;
          case 2:
            if (!(realpos&0xFF))
              c.evmodpos++;
            break;
          case 3:
            if (!(realpos&0xFFFF))
              c.evmodpos++;
            break;
          }
          if ((c.evmodpos==c.evmod)&&c.evmod)
          {
            c.evmodpos=0;
            c.evpos=realpos;
            c.evtime=t;
          }
        }
      }
      break;
    case queGVol: realgvol=val2; break;
    case queTempo: realtempo=val2; break;
    case queSpeed: realspeed=val2; break;
    };
    querpos=(querpos+1)%quelen;
  }
}

void itplayerclass::checkchan(physchan &p)
{
  if (!mcpGet(p.no, mcpCStatus))
    p.dead=1;
  if (p.dead&&(channels[p.lch].pch!=&p))
    p.notecut=1;
  if (p.notecut)
  {
    if (channels[p.lch].pch==&p)
      channels[p.lch].pch=0;
    p.lch=-1;
    mcpSet(p.no, mcpCReset, 0);
    return;
  }
}

int itplayerclass::random()
{
  randseed=randseed*0x15A4E35+1;
  return (randseed>>16)&32767;
}

void itplayerclass::playtick()
{
  getproctime();
  readque();

  int i,j;
  for (i=0; i<nchan; i++)
    inittick(channels[i]);

  manualgoto=0;
  curtick++;
  if ((curtick==(speed+patdelaytick))&&patdelayrow)
  {
    curtick=0;
    patdelayrow--;
  }

  if (curtick==(speed+patdelaytick))
  {
    patdelaytick=0;
    curtick=0;
    currow++;
    if ((gotoord==-1)&&(currow==patlens[orders[curord]]))
    {
      gotoord=curord+1;
      gotorow=0;
    }
    if (gotoord!=-1)
    {
      if (gotoord!=curord)
        for (i=0; i<nchan; i++)
        {
          logchan &c=channels[i];
          c.patloopcount=0;
          c.patloopstart=0;
        }

      if (gotoord>=endord)
        gotoord=0;
      while (orders[gotoord]==0xFFFF)
        gotoord++;
      if (gotoord==endord)
        gotoord=0;
      if (gotorow>=patlens[orders[gotoord]])
      {
        gotoord++;
        gotorow=0;
        while (orders[gotoord]==0xFFFF)
          gotoord++;
        if (gotoord==endord)
          gotoord=0;
      }
      curord=gotoord;
      patptr=patterns[orders[curord]];
      for (currow=0; currow<gotorow; currow++)
      {
        while (*patptr)
          patptr+=6;
        patptr++;
      }
      gotoord=-1;
    }

    for (i=0; i<nchan; i++)
      initrow(channels[i]);
    while (*patptr)
    {
      logchan &c=channels[*patptr++-1];
      if ((patptr[3]==cmdSpecial)&&patptr[4])
      {
        c.specialcmd=patptr[4]>>4;
        c.specialdata=patptr[4]&0xF;
      }
      if ((patptr[3]==cmdSpecial)&&(c.specialcmd==cmdSNoteDelay))
        memcpy(c.delayed, patptr, sizeof(c.delayed));
      else
      {
        if (patptr[0]||patptr[1])
          playnote(c, patptr);
        if (patptr[2])
          playvcmd(c, patptr[2]);
      }
      if (patptr[3])
        playcmd(c, patptr[3], patptr[4]);
      patptr+=5;
    }
    patptr++;
  }
  else
    for (i=0; i<nchan; i++)
      processfx(channels[i]);
  for (i=0; i<npchan; i++)
    inittickchan(pchannels[i]);
  for (i=0; i<nchan; i++)
    updatechan(channels[i]);
  for (i=0; i<npchan; i++)
    if (pchannels[i].lch!=-1)
      checkchan(pchannels[i]);
  for (i=0; i<npchan; i++)
    if (pchannels[i].lch!=-1)
      processchan(pchannels[i]);
  for (i=0; i<nchan; i++)
    if (channels[i].pch==&channels[i].newchan)
    {
      processchan(*channels[i].pch);
      allocatechan(channels[i]);
    }
  for (i=0; i<npchan; i++)
    if (pchannels[i].lch!=-1)
      putchandata(pchannels[i]);
  putglobdata();
  putque(quePos, -1, (curtick&0xFF)|(currow<<8)|(curord<<16));
}

int itplayerclass::loadsamples(module &m)
{
  return mcpLoadSamples(m.sampleinfos, m.nsampi);
}

int itplayerclass::play(const module &m, int ch)
{
  int i;
  staticthis=this;

  randseed=1;
  patdelayrow=0;
  patdelaytick=0;
  gotoord=0;
  gotorow=0;
  endord=m.endord;
  nord=m.nord;
  nchan=m.nchan;
  orders=m.orders;
  patlens=m.patlens;
  patterns=m.patterns;
  ninst=m.ninst;
  instruments=m.instruments;
  nsamp=m.nsamp;
  samples=m.samples;
  sampleinfos=m.sampleinfos;
  nsampi=m.nsampi;
  speed=m.inispeed;
  tempo=m.initempo;
  gvol=m.inigvol;
  chsep=m.chsep;
  linear=m.linear;
  oldfx=m.oldfx;
  instmode=m.instmode;
  geffect=m.geffect;
  curtick=speed-1;
  currow=0;
  curord=0;
  realpos=0;
  pitchhigh=-0x6000;
  pitchlow=0x6000;
  realsynctime=0;
  realsync=0;
  realtempo=tempo;
  realspeed=speed;
  realgvol=gvol;

  if (orders[0]==0xFFFF)
    return 0;

  channels=new logchan[nchan];
  pchannels=new physchan[ch];
  quelen=500;
  que=new int [quelen][4];
  if (!channels||!pchannels||!que)
  {
    delete channels;
    delete pchannels;
    return 0;
  }
  querpos=quewpos=0;
  memset(channels, 0, sizeof(*channels)*nchan);
  memset(pchannels, 0, sizeof(*pchannels)*ch);
  for (i=0; i<ch; i++)
    pchannels[i].lch=-1;
  for (i=0; i<nchan; i++)
  {
    logchan &c=channels[i];
    c.newchan.lch=i;
    c.cvol=m.inivol[i];
    c.cpan=m.inipan[i]&127;
    c.srnd=c.cpan==100;
    c.disabled=m.inipan[i]&128;
  }

  if (!mcpOpenPlayer(ch, playtickstatic))
    return 0;

  npchan=mcpNChan;

  return 1;
}

void itplayerclass::stop()
{
  mcpClosePlayer();
  delete channels;
  delete pchannels;
  delete que;
}

int itplayerclass::getpos()
{
  if (manualgoto)
    return (gotorow<<8)|(gotoord<<16);
  return (curtick&0xFF)|(currow<<8)|(curord<<16);
}

int itplayerclass::getrealpos()
{
  readque();
  return realpos;
}

int itplayerclass::getchansample(int ch, short *buf, int len, int rate, int opt)
{
  int i,n;
  int chn[64];
  n=0;
  for (i=0; i<npchan; i++)
    if (pchannels[i].lch==ch)
      chn[n++]=i;
  mcpMixChanSamples(chn, n, buf, len, rate, opt);
  return 1;
}

void itplayerclass::getrealvol(int ch, int &l, int &r)
{
  int i,voll,volr;
  l=r=0;
  for (i=0; i<npchan; i++)
    if (pchannels[i].lch==ch)
    {
      mcpGetRealVolume(i, voll, volr);
      l+=voll;
      r+=volr;
    }
}

void itplayerclass::setpos(int ord, int row)
{
  int i;
  if (curord!=ord)
    for (i=0; i<npchan; i++)
      pchannels[i].notecut=1;
  curtick=speed-1;
  patdelaytick=0;
  patdelayrow=0;
  if ((ord==curord)&&(row>patlens[orders[curord]]))
  {
    row=0;
    ord++;
  }
  gotorow=(row>0xFF)?0xFF:(row<0)?0:row;
  gotoord=((ord>=nord)||(ord<0))?0:ord;
  manualgoto=1;
  querpos=quewpos=0;
  realpos=(gotorow<<8)|(gotoord<<16);
}

int itplayerclass::getdotsdata(int ch, int pch, int &smp, int &note, int &voll, int &volr, int &sus)
{
  for (pch=pch; pch<npchan; pch++)
    if ((pchannels[pch].lch==ch)&&!pchannels[pch].dead)
      break;
  if (pch>=npchan)
    return -1;
  physchan &p=pchannels[pch];
  smp=p.smp->handle;
  note=p.noteoffset+p.fpitch;
  mcpGetRealVolume(p.no, voll, volr);
  sus=!(p.noteoff||p.notefade);
  return pch+1;
}

void itplayerclass::getglobinfo(int &tmp, int &bp, int &gv)
{
  readque();
  tmp=realspeed;
  bp=realtempo;
  gv=realgvol;
}

int itplayerclass::getsync(int ch, int &time)
{
  readque();
  if ((ch<0)||(ch>=nchan))
  {
    time=gettime()-realsynctime;
    return realsync;
  }
  else
  {
    time=gettime()-channels[ch].realsynctime;
    return channels[ch].realsync;
  }
}

int itplayerclass::gettime()
{
  return mcpGet(-1, mcpGTimer);
}

int itplayerclass::getticktime()
{
  readque();
  return 65536*5/(2*realtempo);
}

int itplayerclass::getrowtime()
{
  readque();
  return 65536*5*realspeed/(2*realtempo);
}

void itplayerclass::setevpos(int ch, int pos, int modtype, int mod)
{
  if ((ch<0)||(ch>=nchan))
    return;
  logchan &c=channels[ch];
  c.evpos0=pos;
  c.evmodtype=modtype;
  c.evmod=mod;
  c.evmodpos=0;
  c.evpos=-1;
  c.evtime=-1;
}

int itplayerclass::getevpos(int ch, int &time)
{
  readque();
  if ((ch<0)||(ch>=nchan))
  {
    time=-1;
    return -1;
  }
  time=gettime()-channels[ch].evtime;
  return channels[ch].evpos;
}

int itplayerclass::findevpos(int pos, int &time)
{
  readque();
  int i;
  for (i=0; i<nchan; i++)
    if (channels[i].evpos==pos)
      break;
  time=gettime()-channels[i].evtime;
  return channels[i].evpos;
}
