#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <math.h>
#include <i86.h>

// flexptc includes
#include "flexptc\flexptc.h"

// rtctimer include
#include "common\rtctimer.h"

// ims includes
#include "ims06\xmplay.h"
#include "ims06\mcp.h"
#include "ims06\ims.h"

#include "common\alpha.h"

#define X_RES 320
#define Y_RES 200

// globals
ptc_context mode;

signed int sintab[65536],  costab[65536];
float      sintabf[65536], costabf[65536];

#define pi         3.141592653589793
void initsintab() {
    int i, j;
    float r;
    
    for (i = 0; i < 65536; i++) {
        r = (sin(2 * pi * i / 65536));
        sintab[i] = 32767 * r;
        sintabf[i] = r;
        r = (cos(2 * pi * i / 65536));
        costab[i] = 32767 * r;
        costabf[i] = r;
    }
}

// parts includes
#include "parts\rblur_0\rblur.h"
rblur_0 *_rblur_0;
#include "parts\logo\logo.h"
logo    *_logo;
#include "parts\bdist_0\bmpdist.h"
bmpdist_0 *_bdist_0;
#include "parts\flat_0\flat.h"
flat_0 *_flat_0;
#include "parts\wf\wf.h"
wf *_wf;
#include "parts\cube\cube.h"
cube *_cube;
#include "parts\fdsphere\fdsphere.h"
fdsphere *_fdsphere;
#include "parts\rblur_1\rblur.h"
rblur_1 *_rblur_1;
#include "parts\fdtunnel\fdtunnel.h"
fdtunnel *_fdtunnel;
#include "parts\credits\credits.h"
credits *_credits;

char *aaa = "как же я ненавижу эту дему :)";

volatile int tick = 0;
void timer() {tick++;}

xmodule mod;

void uninit() {
    ptc_close();
    xmpStopModule();
    xmpFreeModule(mod);
    
    delete _rblur_0;
    delete _logo;
    delete _bdist_0;
    delete _flat_0;
    delete _wf;
    delete _cube;
    delete _fdsphere;
    delete _rblur_1;
    delete _fdtunnel;
    delete _credits;
    
    donealpha();
    
    rtc_freeTimer();
}

int noretrace = 0, rate = 3;

int main(int argc, char *argv[]) {
    int i, j, p, ptc_flags = 0;
    
    
    for (p = 1; p < argc; p++) {
        if (strcmp(strupr(argv[p]), "NORETRACE") == 0)   noretrace  = 1;
        if (strcmp(strupr(argv[p]), "SLOWTIMER") == 0)   rate       = 6;
        if (strcmp(strupr(argv[p]), "FORCEBANKED") == 0) ptc_flags |= ptc_OPEN_FORCEBANKED;
        if (strstr(argv[p], "?")) {
            puts(" short command line parameters info");
            puts("   noretrace   - disable vertical retrace sync");
            puts("   slowtimer   - use slow timer mode");
            puts("   forcebanked - force banked VESA modes usage");
            return 0;
        }
    }
    
    
    imsinitstruct is;
    imsFillDefaults(is);
    is.bufsize = 65535; // 1sec buffer
    is.pollmin = 1092;
    is.interpolate = 0;
    
    printf(" choose 320x200 15bpp 60hz tweak mode if possible \n");
    printf(" if some border mess occurs, pick 320x240 mode \n");
    printf(" 16bpp modes for nvidia lamers, 640x4x0 hdouble modes for matroxoids :)\n");
    printf(" ----------------------------------------\n");
    
    // flexptc initialisation
    if (ptc_init()) return 1;
    if (ptc_open("", X_RES, Y_RES, 15, ptc_OPEN_FIND60HZ | ptc_flags, &mode)) {ptc_close(); return 1;}
  
    // IMS initialisation
    if (!imsInit(is)) { cputs("could not init\r\n"); return 0; }
    
    FILE *f;
   
    // global init   
    srand(inp(0x40));
    cputs("init ");
    
    // load module
    f = fopen("WUFFLER.XM", "rb");
    if (!f) {cputs("could not open\r\n"); return 0;}
    cputs(".");
  
    if (xmpLoadModule(mod, f)) {cputs("could not load\r\n"); return 0;}
    fclose(f);
    cputs(".");

    if (!xmpLoadSamples(mod)) {cputs("could not upload\r\n"); return 0;}
    cputs(".");
    
    // init stuff
    initsintab();
    cputs(".");
    initalpha();
    cputs(".");
    
    // init parts
    _rblur_0 = new rblur_0();
    cputs(".");
    _logo = new logo();
    cputs(".");
    _bdist_0 = new bmpdist_0();
    cputs(".");
    _flat_0 = new flat_0();
    cputs(".");
    _wf = new wf();
    cputs(".");
    _cube = new cube();
    cputs(".");
    _fdsphere = new fdsphere();
    cputs(".");
    _rblur_1 = new rblur_1();
    cputs(".");
    _fdtunnel = new fdtunnel();
    cputs(".");
    _credits = new credits();
    cputs(".");
    
    // set mode and start music
    if (!xmpPlayModule(mod))  {cputs("could not play\r\n"); return 0;}
    ptc_setcontext(&mode);
    
    while (kbhit()) {getch();}

    //xmpSetPos(0x1D, 0);
    
    // init timer
    rtc_initTimer(6);
    rtc_setTimer(&timer, rtc_timerRate / 60);
    
    // run parts
    _rblur_0->tick      = &tick;
    if (_rblur_0->main(noretrace)) {uninit(); return 0;}

    tick = 0;    
    _logo->tick         = &tick;
    if (_logo->main(noretrace)) {uninit(); return 0;}
    
    tick = 0;    
    _bdist_0->tick      = &tick;
    if (_bdist_0->main(noretrace)) {uninit(); return 0;}
    
    tick = 0;    
    _flat_0->tick       = &tick;
    if (_flat_0->main(noretrace)) {uninit(); return 0;}
    
    tick = 0;    
    _wf->tick           = &tick;
    if (_wf->main(noretrace)) {uninit(); return 0;}
    
    tick = 0;    
    _cube->tick         = &tick;
    if (_cube->main(noretrace)) {uninit(); return 0;}
    
    tick = 0;    
    _fdsphere->tick     = &tick;
    if (_fdsphere->main(noretrace)) {uninit(); return 0;}
    
    tick = 0;    
    _rblur_1->tick      = &tick;
    if (_rblur_1->main(noretrace)) {uninit(); return 0;}
    
    tick = 0;    
    _fdtunnel->tick     = &tick;
    if (_fdtunnel->main(noretrace)) {uninit(); return 0;}
    
    tick = 0;    
    _credits->tick      = &tick;
    if (_credits->main(noretrace)) {uninit(); return 0;}

    // uninit
    uninit();
    
    puts(" volatile - (b-state && light future) - 0x7e0");
    puts(" everybody hates dos4gw :)");
    return 0;
}