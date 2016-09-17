#include <stdlib.h>
#include <conio.h>
#include <math.h>
#include "rtctimer.h"

unsigned char  *screen = (unsigned char*)0xA0000;

unsigned char  fdbuffer[64000], buffer[64000];

unsigned int   fdp[41 * 26];

         short sintab [65536], costab [65536];
         float sintabf[65536], costabf[65536];

typedef struct vec3f {
    float x, y, z;
} vec3f;

typedef struct vec2 {
    int x, y;
} vec2;

#define sat(a, l)  ((a > l) ? l : a)
#define sqr(a)     ((a)*(a))
#define sgn(a)     (((a) > 0) ? 1 : -1)

#define ee         1.0E-6
#define bb         1.0E+4
#define pi         3.141592653589793


void fdInterpolate() {
    typedef struct {int  sy0, sy1, sdy, ey0, ey1, edy, x0, x1, dx, sy, ey, sx;} _fd;
    
    int  x, y, i, j, k, gridptr = 0;
    long scrptr = (long)&fdbuffer;
    _fd p;
    unsigned int xp, xdp;
    
    for (j = 0; j < 25; j++) {
        for (i = 0; i < 40; i++) {
            
            p.sdy = (fdp[gridptr+41] - fdp[gridptr]) >> 3;
            p.sy  = (fdp[gridptr]);
            
            p.edy = (fdp[gridptr+42] - fdp[gridptr+1]) >> 3;
            p.ey  = (fdp[gridptr+1]);
            
            for (y = 0; y < 8; y++) {
                p.dx = (p.ey - p.sy) >> 3;
                p.sx = p.sy;
                
                // prepare vars 
                xp = p.sy;
                xdp = p.dx;
                
                for (x = 0; x < 8; x++) {
                    *((char*)scrptr++) = (xp >> 8);
                    xp += xdp;
                }
                scrptr += (320 - 8);
                p.sy += p.sdy; p.ey += p.edy;
            }
            gridptr++;
            scrptr -= (320 * 8) -8;
        }
        gridptr++;
        scrptr += (320 * 7);
    } 
}

void fdbuildSinTable() {
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

void fd3dRotate (int ax, int ay, int az, vec3f *v) {
    // hehehe, this code is fully ported from my old freebasic demoz ;)
    int i;
    float sinx = sintabf[ax], cosx = costabf[ax];
    float siny = sintabf[ay], cosy = costabf[ay];
    float sinz = sintabf[az], cosz = costabf[az];
    float bx, by, bz, px, py, pz;  // temp var storage

        //pt[i] = p[i];
        
        py = v->y;
        pz = v->z;
        v->y = (py * cosx - pz * sinx);
        v->z = (py * sinx + pz * cosx);
        
        px = v->x;
        pz = v->z;
        v->x = (px * cosy - pz * siny);
        v->z = (px * siny + pz * cosy);
        
        px = v->x;
        py = v->y;
        v->x = (px * cosz - py * sinz);
        v->y = (px * sinz + py * cosz);

} 

void fdCalcPlanes(int i) {
    
    int x, y, z, tptr = 0;
    int p;
    
    for (y = 0; y < 26; y++) {
        for (x = 0; x < 41; x++) {
            
            p = (((sintab[((i << 4) + (x << 10) - (y << 11)) & 0xFFFF]) +
                  (costab[((i << 6) + (y << 10))             & 0xFFFF]) -
                  (sintab[((i << 5) - (x << 10) + (y <<  9)) & 0xFFFF]) +
                  (sintab[((i << 3) + (x << 11) + (i <<  5))  & 0xFFFF])) >> 1) + 32768;
                 
            //p = x ^ y;
            
            fdp[tptr++] = p;
        }
    }
}
int dtick = 0;

/*
void fdBlend();
#pragma aux fdBlend = " mov   esi, offset fdblendtab" \
                      " mov   ebx, offset fdbuffer  " \
                      " mov   edi, offset buffer    " \
                      " mov   ecx, 64000            " \
                      " xor   edx, edx              " \
                      " @inner:                     " \
                      " mov   dh,    [ebx]          " \
                      " mov   dl,    [edi]          " \
                      " inc   ebx                   " \
                      " mov   al,    [esi + edx]    " \
                      " mov   [edi], al             " \
                      " inc   edi                   " \
                      " dec   ecx                   " \
                      " jnz   @inner                " \
                      modify [esi edi ebx ecx edx];
*/
                      
int tick = 0, diftick = 0;

void timer() { tick++; diftick++; }

int main() {
    int i, j, p = 0;
    int atick;
    
    fdbuildSinTable();
    
    memset(&fdbuffer, 0, 64000);

    _asm {
        mov  ax, 13h
        int  10h
    }
     
    _asm {    
        // zpizzheno ;)
        mov ax,13h
        int 10h       // regular mode 13h chained

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
    
    
    outp(0x3C8, 0);
    for (i = 0; i < 256; i++) {
        
        
        outp(0x3C9, (((sintab[((i << 11)) & 0xFFFF] >> 10) + costab[((i <<  9)) & 0xFFFF] >> 10) >> 1) + 32);
        outp(0x3C9, (((sintab[((i << 10)) & 0xFFFF] >> 10) + costab[((i <<  8)) & 0xFFFF] >> 10) >> 1) + 32);
        outp(0x3C9, (((sintab[((i << 10)) & 0xFFFF] >> 10) + costab[((i << 10)) & 0xFFFF] >> 10) >> 1) + 32);
        
        
        //outp(0x3C9, sat(((i >> 2) + (i >> 4)), 63));
        //outp(0x3C9, sat(((i >> 2) + (i >> 3)), 63));
        //outp(0x3C9, sat(((i >> 3) + (i >> 3) + (i >> 4)), 63)); 
        
        /*
        outp(0x3C9, sat((i >> 2), 63));
        outp(0x3C9, sat((i >> 2), 63)); 
        outp(0x3C9, sat((i >> 2), 63));
        */    
    }
    
    rtc_initTimer(3);
    rtc_setTimer(&timer, rtc_timerRate / 60);

    while (!kbhit()) {
        if (i == tick) while(i == atick) {_disable(); atick = tick; _enable();};
        i = tick; dtick = diftick; diftick = 0;
        
        while ((inp(0x3DA) & 8) == 8) {}
        while ((inp(0x3DA) & 8) != 8) {}

        //outp(0x3C8, 0); outp(0x3C9, 63); outp(0x3C9, 0); outp(0x3C9, 0);
        
        memcpy(screen, &fdbuffer, 64000);    
        
        fdCalcPlanes(i << 3);
        
        //outp(0x3C8, 0); outp(0x3C9, 63); outp(0x3C9, 63); outp(0x3C9, 0);
        
        fdInterpolate();
        //outp(0x3C8, 0); outp(0x3C9, 63); outp(0x3C9, 0); outp(0x3C9, 63);
        //fdBlend();
        
        //outp(0x3C8, 0); outp(0x3C9, 0); outp(0x3C9, 0); outp(0x3C9, 0); 
    }
    getch();
    rtc_freeTimer();
    _asm {
        mov  ax, 3
        int  10h
    }
}
