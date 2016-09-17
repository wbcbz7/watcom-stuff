#include <math.h>
#include <strings.h>
#include <stdlib.h>
#include <conio.h>
#include <stdio.h>
#include "usmplay.h"

#define X_SIZE 320
#define Y_SIZE 200
#define DIST   300

typedef struct {
    float x, y, z, d;
} vertex;

typedef struct { float x, y; } vertex2d;

short sintab[65536];
float sintabf[65536], costabf[65536];

unsigned char  *screen = 0xA0000;

#define pi        3.141592653589793
#define ee        1.0E-6
#define bb        2 * 1.0E+4
#define bb2       1.0E+6

#define sat(a, l) ((a > l) ? l : a)
#define sqr(a)    ((a)*(a))
#define sgn(a)    (((a) > 0) ? 1 : -1)

USM *module;
int notimer = 0, noretrace = 0, mode13h = 0;

//--------------------------------------------------

void initsintab() {
    int i, j;
    float r;
    
    for (i = 0; i < 65536; i++) {
        r = (sin(2 * pi * i / 65536));
        sintab[i] = 32767 * r;
        sintabf[i] = r;
        r = (cos(2 * pi * i / 65536));
        costabf[i] = r;
    }
}

void setvidmode(char mode) { 
    _asm {
        mov ah, 0
        mov al, mode
        int 10h 
    } 
}

int normal = 0;

void KillAll() {
    USMP_StopPlay();
    USMP_FreeModule(module);
    
    setvidmode(3);
    if (Error_Number!=0) { Display_Error(Error_Number); exit(0); }
    if (normal == 0) {puts("hey, why you pressed that escape key?!\0"); exit(0);}
}

//------------------------------------------------

#include "parts\\tunnel.c"
#include "parts\\tunnel2.c"
#include "parts\\3dstuff.c"
#include "parts\\twirl.c"
#include "parts\\twirl2.c"
#include "parts\\twirl3.c"
#include "parts\\twirl4.c"
#include "parts\\3dstuff2.c"
#include "parts\\fdtunnel.c"
#include "parts\\fdplanes.c"

int main(int argc, char *argv[]) {
    int p;
    
    for (p = 1; p < argc; p++) {
        if (strcmp(strupr(argv[p]), "NOTIMER") == 0)   notimer   = 1;
        if (strcmp(strupr(argv[p]), "NORETRACE") == 0) noretrace = 1;
        if (strcmp(strupr(argv[p]), "70HZ") == 0)      mode13h   = 1;
    }
    
    HardwareInit(_psp);
    USS_Setup();
    if (Error_Number!=0) { Display_Error(Error_Number); exit(0); }

    if (notimer   == 1) {puts("timer sync disabled\0");}
    if (noretrace == 1) {puts("vertical retrace sync disabled\0");}
    if (mode13h   == 1) {puts("320x200 70Hz mode used\0");}
    
    cputs("init .");

    module = XM_Load(LM_File, 0x020202020, "musik.xm");
    if (Error_Number!=0) { puts(".\0"); Display_Error(Error_Number); exit(0); }
    
    cputs(".");
    initsintab();
    cputs(".");
    t1_init();
    cputs(".");
    t2_init();
    cputs(".");
    fx1_init();
    cputs(".");
    w1_init();
    cputs(".");
    w2_init();
    cputs(".");
    w3_init();
    cputs(".");
    w4_init();
    cputs(".");
    fx2_init();
    cputs(".");
    fd1_init();
    cputs(".");
    fd2_init();
    puts(".");
    
    
    puts("get down! ;)\0");
    
    setvidmode(0x13);
    
    if (mode13h == 0) _asm {
        // zpizzheno ;)

        mov dx,3d4h   // remove protection
        mov al,11h
        out dx,al
        inc dl
        in  al,dx
        and al,7fh
        out dx,al

        mov dx,3c2h   // misc output
        mov al,0e3h   // clock
        out dx,al

        mov dx,3d4h
        mov ax,00B06h // Vertical Total
        out dx,ax
        mov ax,03E07h // Overflow
        out dx,ax
        mov ax,0C310h // Vertical start retrace
        out dx,ax
        mov ax,08C11h // Vertical end retrace
        out dx,ax
        mov ax,08F12h // Vertical display enable end
        out dx,ax
        mov ax,09015h // Vertical blank start
        out dx,ax
        mov ax,00B16h // Vertical blank end
        out dx,ax 
    }
    
    USS_SetAmpli(100); // to prevent clipping on sbcovoxpcspeaker                 
    USMP_StartPlay(module);
    USMP_SetOrder(0);
    
    if (Error_Number!=0) { setvidmode(3); Display_Error(Error_Number); exit(0); }
    
    while (Row < 32) {}
    
    t1_main();
    t2_main();
    fx1_main();
    w1_main();
    w2_main();
    w3_main();
    w4_main();
    fx2_main();
    
    fd1_main();
    fd2_main();
    
    normal = 1;
    KillAll();

    
    puts("blash - b-state - 2015\0");
    
}
