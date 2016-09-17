#include <string.h>
#include <conio.h>
#include <stdio.h>
//#include "binfstd.h"
#include "xmplay.h"
#include "gmdplay.h"
#include "mcp.h"
#include "ims.h"
#include "itplay.h"
#include <iostream.h>

void main(int argc, char **argv)
{
  imsinitstruct is;
  imsFillDefaults(is);
  is.bufsize=8192; // 1sec buffer
//  is.pollmin=61440; // use polling only after 0.9375sec
//  is.usersetup=0;
  int i;
  for (i=1; i<argc; i++)
    if (!stricmp(argv[i], "nosound"))
      is.usequiet=1;
  if (!imsInit(is))
  {
    cputs("could not init\r\n");
    return;
  }

  //sbinfile fil;
  FILE *f;

  
  xmodule mod1;
  //if (!fil.open(".\\SAMPLE\\BLITZKRI.MOD", fil.openro)) {cputs("could not open\r\n"); return;}
  f = fopen(".\\SAMPLE\\BLITZKRI.MOD", "rb");
  if (!f) {cputs("could not open\r\n"); return;}
  
  //if (xmpLoadMOD(mod1, fil)) {cputs("could not load\r\n"); return;}
  if (xmpLoadMOD(mod1, f)) {cputs("could not load\r\n"); return;}
  //fil.close();
  fclose(f);

  if (!xmpLoadSamples(mod1)) {cputs("could not upload\r\n"); return;}
  if (!xmpPlayModule(mod1))  {cputs("could not play\r\n"); return;}

  xmpSetEvPos(0, 0x20000, 2, 8);
  xmpSetEvPos(1, 0x20400, 2, 8);
  while (!kbhit())
  {
    int time1,time2,time3;
    if (mcpIdle)
      mcpIdle();
    while (inp(0x3da)&8);
    while (!(inp(0x3da)&8));
    cerr << "\r" << hex << xmpGetRealPos();
    xmpGetEvPos(0, time1);
    xmpGetEvPos(1, time2);
    outp(0x3c8,0);
    outp(0x3c9,(time1<4096)?(63-(time1/64)):0);
    outp(0x3c9,0);
    outp(0x3c9,(time2<8192)?(63-(time2/128)):0);
  }
  while (kbhit())
    getch();

  xmpStopModule();
  xmpFreeModule(mod1);
  
  //if (!fil.open(".\\SAMPLE\\BEYOND.XM", fil.openro)) {cputs("could not open\r\n"); return;}
  f = fopen(".\\SAMPLE\\BEYOND.XM", "rb");
  if (!f) {cputs("could not open\r\n"); return;}
  
  //if (xmpLoadModule(mod1, fil)) {cputs("could not load\r\n"); return;}
  if (xmpLoadModule(mod1, f)) {cputs("could not load\r\n"); return;}
  
  //fil.close();
  fclose(f);

  if (!xmpLoadSamples(mod1)) {cputs("could not upload\r\n"); return;}
  if (!xmpPlayModule(mod1))  {cputs("could not play\r\n"); return;}

  xmpSetEvPos(0, 0x20000, 2, 8);
  xmpSetEvPos(1, 0x20400, 2, 8);
  while (!kbhit())
  {
    int time1,time2,time3;
    if (mcpIdle)
      mcpIdle();
    while (inp(0x3da)&8);
    while (!(inp(0x3da)&8));
    cerr << "\r" << hex << xmpGetRealPos();
    xmpGetEvPos(0, time1);
    xmpGetEvPos(1, time2);
    outp(0x3c8,0);
    outp(0x3c9,(time1<4096)?(63-(time1/64)):0);
    outp(0x3c9,0);
    outp(0x3c9,(time2<8192)?(63-(time2/128)):0);
  }
  while (kbhit())
    getch();

  xmpStopModule();
  xmpFreeModule(mod1);
  
  
  //------------------------------
  
  /*
  gmdmodule mod2;
  if (!fil.open(".\\SAMPLE\\MECH8.S3M", fil.openro)) {cputs("could not open\r\n"); return;}
  cputs(".");
  mpLoadS3M(mod2, fil);
  cputs(".");
  fil.close();
  
  mpReduceSamples(mod2);
  cputs(".");
  mpLoadSamples(mod2);
  cputs(".");
  mpPlayModule(mod2);
  cputs(".");

  while (!kbhit())
  {
    if (mcpIdle)
      mcpIdle();
    while (inp(0x3da)&8);
    while (!(inp(0x3da)&8));
    cerr << "\r" << hex << mpGetRealPos();
  }
  while (kbhit())
    getch();

  mpStopModule();
  mpFree(mod2);
  */
  //------------------------------

  
  static itplayerclass itplayer;
  itplayerclass::module mod3;

  /*
  if (!fil.open(".\\SAMPLE\\CHRIS31B.IT", fil.openro))
  {
    cputs("could not open\r\n");
    return;
  }
  
  
  if (mod3.load(fil))
  {
    cputs("could not load\r\n");
    return;
  }
  fil.close();
  */
  
  f = fopen(".\\SAMPLE\\CHRIS31B.IT", "rb");
  if (!f) {cputs("could not open\r\n"); return;}
  
  if (mod3.load(f))
  {
    cputs("could not load\r\n");
    return;
  }
  
  fclose(f);
  
  if (!itplayer.loadsamples(mod3))
  {
    cputs("could not upload\r\n");
    return;
  }
  if (!itplayer.play(mod3, 64))
  {
    cputs("could not play\r\n");
    return;
  }

  itplayer.setevpos(0, 0, 2, 4);
  while (!kbhit())
  {
    int time;
    cerr << "\r" << hex << itplayer.getevpos(0, time) << "\t" << hex << time;
  }

  while (kbhit())
    getch();

  itplayer.stop();
  mod3.free();
  

  imsClose();
}
