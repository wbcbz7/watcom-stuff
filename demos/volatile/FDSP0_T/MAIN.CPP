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
#include "fdsphere.h"

volatile int tick = 0;
void timer() {tick++;}

int main(int argc, char *argv[]) {
    int timedemo = 0;
    
    // init sintables etc
    initsintab();
    
    // init part
    fdsphere *part = new fdsphere();
    
    // init ptc
    if (ptc_init()) return 1;
    if (ptc_open("", X_RES, Y_RES, 15, ptc_OPEN_FIND60HZ | ptc_OPEN_SETMODE, &mode)) {ptc_close(); return 1;}
    //ptc_setcontext(&mode);
    
    if (strstr(strupr(argv[1]), "T")) timedemo = 1;
    
    rtc_initTimer(3);
    rtc_setTimer(&timer, rtc_timerRate / 60);
    // call main procedure
    part->tick = &tick;
    part->main(timedemo);
    
    // cleanup before exit
    ptc_close();
    delete part;
    rtc_freeTimer();
    
    return 0;
}