#include <stdio.h>
#include <conio.h>
#include <math.h>

// flexptc includes
#include "flexptc\flexptc.c"

// rtctimer include
#include "rtctimer.h"

// ims includes
#include "ims06\xmplay.h"
#include "ims06\mcp.h"
#include "ims06\ims.h"

#define X_SIZE 640
#define Y_SIZE 350
#define sat(a, l) ((a > l) ? l : a)
#define ee 1.0E-6
double pi = 3.141592653589793;

// globals
ptc_context ptc_8bppmode, ptc_32bppmode;
int notimer = 0, noretrace = 0, rate = 3;

unsigned char  noisetab[X_SIZE * Y_SIZE * 2];

#define lut_size 65536
short sintab [lut_size], costab [lut_size];
float sintabf[lut_size], costabf[lut_size];

void initnoise() {
    int i;
    
    for (i = 0; i < (X_SIZE*Y_SIZE*2); i++) {
        noisetab[i] = (rand() & 0x0F) + 1;
    }
}

// parts includes
namespace wf1 {
#include "parts\wf1\wf1.c"
}
namespace wf2 {
#include "parts\wf2\wf2.c"
}
namespace flat1 {
#include "parts\flat1\flat1.c"
}
namespace wf3 {
#include "parts\wf3\wf3.c"
}
namespace wf4 {
#include "parts\wf4\wf4.c"
}
namespace logo {
#include "parts\logo\logo.c"
}
namespace flare1 {
#include "parts\flare1\flare1.c"
}

void initsintab() {
    int i, j;
    double r, lut_mul;
    lut_mul = (2 * pi / 65536);
    for (i = 0; i < 65536; i++) {
        r = i * lut_mul;
        sintab [i] = 32767 * sin(r);
        costab [i] = 32767 * cos(r);
        sintabf[i] = sin(r);
        costabf[i] = cos(r);
    }
}

xmodule mod;

void uninit() {
    ptc_close();
    xmpStopModule();
    xmpFreeModule(mod);
}

int main(int argc, char *argv[]) {
    int i, j, p, ptc_flags = ptc_OPEN_FILLCONTEXT;
    
    for (p = 1; p < argc; p++) {
        if (strcmp(strupr(argv[p]), "NOTIMER") == 0)     notimer    = 1;
        if (strcmp(strupr(argv[p]), "NORETRACE") == 0)   noretrace  = 1;
        if (strcmp(strupr(argv[p]), "SLOWTIMER") == 0)   rate       = 6;
        if (strcmp(strupr(argv[p]), "FORCEBANKED") == 0) ptc_flags |= ptc_OPEN_FORCEBANKED;
        if (strstr(argv[p], "?")) {
            puts(" short command line parameters info");
            puts("   notimer     - disable timer sync");
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
    
    // flexptc initialisation
    if (ptc_open("", X_SIZE, Y_SIZE, 8, ptc_flags)) {return 0;}
    memcpy(&ptc_8bppmode, &ptc_int, sizeof(ptc_context));
  
    // IMS initialisation
    if (!imsInit(is)) { cputs("could not init\r\n"); return 0; }
    
    FILE *f;
    
    f = fopen("URIBIN.XM", "rb");
    if (!f) {cputs("could not open\r\n"); return 0;}
  
    if (xmpLoadModule(mod, f)) {cputs("could not load\r\n"); return 0;}
  
    fclose(f);

    if (!xmpLoadSamples(mod)) {cputs("could not upload\r\n"); return 0;}
    
    // init parts
    srand(inp(0x40));
    cputs(" downloading win10 update, please wait");
    initsintab();
    cputs(".");
    initnoise();
    cputs(".");
    
    wf1::init();
    cputs(".");
    wf2::init();
    cputs(".");
    flat1::init();
    cputs(".");
    flare1::init();
    cputs(".");
    wf3::init();
    cputs(".");
    wf4::init();
    cputs(".");
    logo::init();
    cputs(".");
    
    // call it before start
    if (!xmpPlayModule(mod))  {cputs("could not play\r\n"); return 0;}
    
    memcpy(&ptc_int, &ptc_8bppmode, sizeof(ptc_context));
    ptc_setcontext();
    
    while (kbhit()) {getch();}

    //xmpSetPos(0xE, 0);
    
    wf1::main();
    wf2::main();
    flat1::main();
    flare1::main();
    wf3::main();
    wf4::main();
    logo::main();
    
    uninit();
    puts(" uribin - b-state - 0x7e0");
    return 0;
}