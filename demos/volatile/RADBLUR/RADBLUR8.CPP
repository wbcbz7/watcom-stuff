#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <math.h>
#include <i86.h>
#include "rtctimer.h"
#include "..\flexptc\flexptc.h"

#define X_RES 320
#define Y_RES 200

#define sat(a, l) (a < l ? a : l)
#define ee         1.0E-6

unsigned char   *screen = (unsigned char*)0xA0000;
ptc_context mode;

// static vars
unsigned short blurtab[X_RES*Y_RES*2];
unsigned char  buffer[X_RES*Y_RES], pic[X_RES*Y_RES];

void setvgamode(unsigned short);
#pragma aux setvgamode = "int 0x10" parm [ax]

void set60hz() {
    // set 320x200/400 @ 60hz mode (640x480 timings)
    // tested in vga modes, probably also work in vesa modes in vga-compatible cards
    // original by type one \\ (tfl-tdv | puple)
    // modified and fixed by wbcbz7
    outp (0x3D4, 0x11); outp(0x3D5, (inp(0x3D5) & 0x7F)); // unlock registers
    outp (0x3C2, (inp(0x3CC) | 0xC0));  // misc. output
    outpw(0x3D4, 0x0B06); // vertical total
    outpw(0x3D4, 0x3E07); // overflow
    outpw(0x3D4, 0xC310); // vertical start retrace
    outpw(0x3D4, 0x8C11); // vertical end retrace
    outpw(0x3D4, 0x8F12); // vertical display enable end
    outpw(0x3D4, 0xB815); // vertical blank start
    outpw(0x3D4, 0xE216); // vertical blank end
}

void initblurtab() {
    unsigned short *p = (unsigned short*)&blurtab;
    float n, nx, ny;
    //float scale = 0.8;
    float scale = 0x20000;
    
    for (int y = 0; y < Y_RES; y++) {
        for (int x = 0; x < X_RES; x++) {
            n = scale / (x*x + y*y);
            nx = (x / n);
            ny = (y / n);
            *p++ = (unsigned short)(nx > X_RES ? X_RES : nx);
            *p++ = (unsigned short)(ny > Y_RES ? Y_RES : ny);
        }
    }
}

void drawblur(int cx, int cy) {
    unsigned char  *p = (unsigned char *)&buffer;
    unsigned char  *v = (unsigned char *)&buffer;
    unsigned short *t = (unsigned short*)&blurtab;
    
    // upper left corner
    t = (unsigned short*)&blurtab;
    p = (unsigned char *)&buffer[(cx-1) + ((cy-1) << 6) + ((cy-1) << 8)];
    for (int y = cy-1; y >= 0; y--) {
        for (int x = cx-1; x >= 0; x--) {
            int sx = *t, sy = *(t + 1);
            *p = (*p + *(p + sx + (sy << 6) + (sy << 8))) >> 1;
            p--; t += 2;
        }
        t += 2*(X_RES - cx); p -= (X_RES - cx);
    }
    // upper right corner
    t = (unsigned short*)&blurtab;
    p = (unsigned char *)&buffer[cx + ((cy-1) << 6) + ((cy-1) << 8)];
    for (int y = cy-1; y >= 0; y--) {
        for (int x = cx; x < X_RES; x++) {
            int sx = *t, sy = *(t + 1);
            *p = (*p + *(p - sx + (sy << 6) + (sy << 8))) >> 1;
            p++; t += 2;
        }
        t += 2*cx; p -= (2*X_RES - cx);
    }
    // lower left corner
    t = (unsigned short*)&blurtab;
    p = (unsigned char *)&buffer[(cx-1) + (cy << 6) + (cy << 8)];
    for (int y = cy; y < Y_RES; y++) {
        for (int x = cx-1; x >= 0; x--) {
            int sx = *t, sy = *(t + 1);
            *p = (*p + *(p + sx - (sy << 6) - (sy << 8))) >> 1;
            p--; t += 2;
        }
        t += 2*(X_RES - cx); p += (X_RES + cx);
    }
    // lower right corner
    t = (unsigned short*)&blurtab;
    p = (unsigned char *)&buffer[cx + (cy << 6) + (cy << 8)];
    for (int y = cy; y < Y_RES; y++) {
        for (int x = cx; x < X_RES; x++) {
            int sx = *t, sy = *(t + 1);
            *p = (*p + *(p - sx - (sy << 6) - (sy << 8))) >> 1;
            p++; t += 2;
        }
        t += 2*cx; p += cx;
    }

}

void initpal() {
    outp(0x3C8, 0);
    for (int i = 0; i < 256; i++) {
        outp(0x3C9, sat((i >> 2), 63));
        outp(0x3C9, sat((i >> 2), 63));
        outp(0x3C9, sat((i >> 2), 63));
    }
}

volatile int tick;
void timer() {tick++;}

volatile unsigned short sx, sy;

unsigned short initmouse();
#pragma aux initmouse = "xor ax, ax" "int 0x33" value [ax] modify [ax];
void showmouse();
#pragma aux showmouse = "mov ax, 0x1" "int 0x33" modify [ax];
void getmouse() {
    _asm {
        mov     ax, 0x3
        int     0x33
        
        mov     [sx], cx
        mov     [sy], dx
    }
}

int main(int argc, char *argv[]) {
    int i;
    
    initblurtab();
    if (ptc_init()) return 1;
    if (ptc_open("", X_RES, Y_RES, 8, ptc_OPEN_FIND60HZ, &mode)) {ptc_close(); return 1;}
    
    
    FILE *f = fopen(argv[1], "rb");
    if (!f) return 1;
    fread(&pic, sizeof(unsigned char), 64000, f);
    fclose(f);
    
    //setvgamode(0x13);
    //set60hz();
    ptc_setcontext(&mode);
    
    initpal();
    
    if (!initmouse()) {printf("where is your mouse, dude? :) \n"); return 1;}
    
    rtc_initTimer(3);
    rtc_setTimer(&timer, rtc_timerRate / 60);
    
    //memcpy(screen, &pic, 64000);
    ptc_update(&pic);
    while(!kbhit()); getch();
    
    while (!kbhit()) {
        i = tick;
        
        /*
        while ((inp(0x3DA) & 8) == 8) {}
        while ((inp(0x3DA) & 8) != 8) {}
        */
        ptc_wait();
        
        outp(0x3C8, 0); outp(0x3C9, 63); outp(0x3C9, 0); outp(0x3C9, 0);
        ptc_update(&buffer);
        //memcpy(screen, &buffer, 64000);
        outp(0x3C8, 0); outp(0x3C9, 63); outp(0x3C9, 0); outp(0x3C9, 63);
        memcpy(&buffer, &pic, 64000);
        getmouse();
        drawblur(sx/2, sy);
        
        outp(0x3C8, 0); outp(0x3C9, 0); outp(0x3C9, 0); outp(0x3C9, 0);
    }
    getch();
    
    rtc_freeTimer();
    
    /*
    _asm {
        mov  ax, 3
        int  10h
    }
    */
    ptc_close();
    return 0;
}