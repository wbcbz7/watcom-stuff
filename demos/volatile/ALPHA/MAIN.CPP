// wrappah нах! :)
// wbcbz7 13.o7.2o16

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <math.h>
#include <i86.h>
#include "..\flexptc\flexptc.h"
#include "rtctimer.h"

#define X_RES 320
#define Y_RES 200

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

// class definition
#include "alpha.h"

volatile int tick = 0;
void timer() {tick++;}

int main(int argc, char *argv[]) {
    int timedemo = 0;
    FILE *f;
    
    if (argc < 3) printf("usage: alpha.exe [pic1.tga] [pic2.tga] \n");
    
    printf(" alpha blending \\ 2d picture processing test by wbcbz7 lb.o8.zolb\n");
    printf(" keys:\n");
    printf("   Q         - swap background pics\n");
    printf("\n");
    printf("   W         - toggle direction blur\n");
    printf("   E         - toggle using display list info for blur\n");
    printf("   Z | X     - inc\\dec blur distance\n");
    printf("   R | G | B - toggle blur color components\n");
    printf("\n");
    printf("   S         - toggle alpha blending\n");
    printf("   O | P     - inc\\dec alpha level\n");
    printf("\n");
    printf("   A         - toggle sprites\n");
    printf("   + | -     - inc\\dec sprite num\n");
    printf("   [ | ]     - inc\\dec sprite rotation velocity\n");
    printf("   < | >     - inc\\dec sprite scale swing level\n");
    printf("   K | L     - inc\\dec sprite scale velocity\n");
    printf(" use terminal connected to AUX for debug info\n");
    printf("\n");
    printf(" press any key ;)\n");
    while (!kbhit()); getch();
    printf("\n");
    
    // init file io stuff
    f = fopen("aux", "rb"); if (!f) {printf("error: can't open aux (pretty weird...)\n"); return 1;}
    
    // init sintables etc
    initsintab();
    
    // init part
    alpha *part = new alpha(argc, argv);
    
    // init ptc
    if (ptc_init()) return 1;
    if (ptc_open("", X_RES, Y_RES, 15, ptc_OPEN_FIND60HZ, &mode)) {ptc_close(); return 1;}
    ptc_setcontext(&mode);
    
    rtc_initTimer(3);
    rtc_setTimer(&timer, rtc_timerRate / 60);
    // call main procedure
    part->tick = &tick;
    part->main(f);
    
    // cleanup before exit
    ptc_close();
    delete part;
    rtc_freeTimer();
    fclose(f);
    
    return 0;
}