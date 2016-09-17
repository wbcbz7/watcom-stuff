#include <stdlib.h>
#include <conio.h>
#include <math.h>
#include "rtctimer.h"

unsigned char  *screen = (unsigned char*)0xA0000;

unsigned char  buffer[65536];

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

#define DIST     300
#define count    4
#define dist_x   64
#define dist_z   4096

#define ee         1.0E-6
#define bb         1.0E+4
#define pi         3.141592653589793

vec3f p[count];
vec3f pt[count];
vec2  p2d[count];


void FillDots () {
    int i, lptr = 0;  
    
    for (i = 0; i < count; i++) {
        p[lptr  ].x = (rand() % dist_x) - (dist_x >> 1);
        p[lptr  ].y = (rand() % dist_x) - (dist_x >> 1);
        p[lptr++].z = 0;
    }
}


void _3dMove (float ax, float ay, float az) {
    int i;
    
    for (i = 0; i < count; i++) {
        pt[i].x += ax;
        pt[i].y += ay;
        pt[i].z += az;
    }
}

void _3dProject() {
    int i;
    float t;
    for (i = 0; i < count; i++) if (pt[i].z < 0) {
        t = DIST / (pt[i].z + ee);
        p2d[i].x = pt[i].x * t + 160;
        p2d[i].y = pt[i].y * t + 100;
    }
}

void DrawPoints () {
    int i, y, x, j;
    int px, py, ofs;
    long scrptr = (long)&buffer;
    
    for (i = 0; i < count; i++) {
        px = p2d[i].x; py = p2d[i].y;
        
        if ((py < 199) && (py > 1) && (px > 0) && (px < 320) && (pt[i].z < 0)) {
            scrptr = (long)&buffer + ((py << 8) + (py << 6) + px);
            *((char*)scrptr) = 255;
        }
    }
}

void buildSinTable() {
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

void _3dRotate (int ax, int ay, int az) {
    // hehehe, this code is fully ported from my old freebasic demoz ;)
    int i;
    float sinx = sintabf[ax], cosx = costabf[ax];
    float siny = sintabf[ay], cosy = costabf[ay];
    float sinz = sintabf[az], cosz = costabf[az];
    float bx, by, bz, px, py, pz;  // temp var storage
    for (i = 0; i < count; i++) {
        pt[i] = p[i];
        
        py = pt[i].y;
        pz = pt[i].z;
        pt[i].y = (py * cosx - pz * sinx);
        pt[i].z = (py * sinx + pz * cosx);
        
        px = pt[i].x;
        pz = pt[i].z;
        pt[i].x = (px * cosy - pz * siny);
        pt[i].z = (px * siny + pz * cosy);
        
        px = pt[i].x;
        py = pt[i].y;
        pt[i].x = (px * cosz - py * sinz);
        pt[i].y = (px * sinz + py * cosz);
    }
} 


void Blend() {
    char *scrptr = (char*)&buffer;
    char *bufptr = (char*)&buffer;
    int i, j;
    
    for (i = 320; i < (64000); i++) {
        scrptr[i] = (scrptr[i-1] + scrptr[i+1] + scrptr[i-320] + scrptr[i+320]) >> 2;
    }
}

int tick = 0, diftick = 0;

void timer() { tick++; diftick++; }

int main() {
    int i, j, p = 0;
    int atick;
    
    buildSinTable();
    FillDots();
    
    memset(&buffer, 0, 64000);

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
        
        outp(0x3C9, sat(((i >> 2) + (i >> 4)), 63));
        outp(0x3C9, sat(((i >> 4) + (i >> 3) + (i >> 5)), 63));
        outp(0x3C9, sat(((i >> 4) + (i >> 3)), 63)); 
        
    }
    
    rtc_initTimer(3);
    rtc_setTimer(&timer, rtc_timerRate / 60);

    while (!kbhit()) {
        if (i == tick) while(i == atick) {_disable(); atick = tick; _enable();};
        i = tick;
        
        while ((inp(0x3DA) & 8) == 8) {}
        while ((inp(0x3DA) & 8) != 8) {}

        //outp(0x3C8, 0); outp(0x3C9, 63); outp(0x3C9, 0); outp(0x3C9, 0);
        
        memcpy(screen, &buffer, 64000);    
        
        //outp(0x3C8, 0); outp(0x3C9, 63); outp(0x3C9, 63); outp(0x3C9, 0);
        
        //outp(0x3C8, 0); outp(0x3C9, 63); outp(0x3C9, 0); outp(0x3C9, 63);
        
        _3dRotate(0, 0, ((i << 8) & 0xFFFF));
        //_3dMove(0, 0, -(64 + 64 * sintabf[(i << 9) & 0xFFFF]));
        _3dMove(0, 0, -DIST);
        _3dProject();
        DrawPoints();
        Blend();
        
        outp(0x3C8, 0); outp(0x3C9, 0); outp(0x3C9, 0); outp(0x3C9, 0); 
    }
    getch();
    rtc_freeTimer();
    _asm {
        mov  ax, 3
        int  10h
    }
}
