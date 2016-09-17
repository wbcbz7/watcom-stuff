#include <stdlib.h>
#include <conio.h>
#include <math.h>
#include <memory.h>
#include "rtctimer.h"
#include "..\..\flexptc\flexptc.c"

#define X_SIZE 640
#define Y_SIZE 350

unsigned char  *screen = (unsigned char*)0xA0000;

unsigned char  texture[65536];
unsigned char  buffer[X_SIZE * Y_SIZE];

unsigned short tunnel[X_SIZE * Y_SIZE];

         short sintab [65536], costab [65536];
         float sintabf[65536], costabf[65536];
         
ptc_palette    pal[256];

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
#define bb         (1.2f * 1.0E+3)
#define pi         3.141592653589793

void normalize(vec3f *v) {
	float l = 1.0 / sqrt(v->x*v->x + v->y*v->y + v->z*v->z);

	v->x *= l;
	v->y *= l;
	v->z *= l;
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

void rotate3d (int ax, int ay, int az, vec3f *v) {
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


void buildtexture() {
    int x, y, i, k=0;
    int r, g, b;
    
    /*
    for (y = 0; y < 256; y++) {
        for (x = 0; x < 256; x++) {
            texture[k++] = (x ^ y) & 0xFF;
        }
    }
    */
    
    for (y = -128; y < 128; y++) {
        for (x = -128; x < 128; x++) {
            //texture[k++] = sat(((int)(0x10000 / ((x*x ^ y*y) + ee))) - ((int)(0x10000 / ((x*x + y*y) + ee))), 255) & 0xFF;
            texture[k++] = sat((int)(0xA0000 / ((x*x ^ y*y) + 0.0001)), 255) & 0xFF;
        }
    }
    
    
    
    // blur our texture
    for (k = 0; k < 4; k++)
    for (i = 0; i < 65536; i++) 
        texture[i] = (texture[(i-1)&0xFFFF] + texture[(i+1)&0xFFFF] + 
                      texture[(i-256)&0xFFFF] + texture[(i+256)&0xFFFF]) >> 2; 
    
}

void buildtunnel() {
    long x, y, i, u, v, lm;
    double r, a, l;
    
    const TunnelSize = 4096;
    const LightmapScale = 1.0;

    i = 0;
    for (y = -(Y_SIZE/2); y < (Y_SIZE/2); y++) 
        for (x = -(X_SIZE/2); x < (X_SIZE/2); x++) {
            r = sqrt(x*x + y*y);
            if (r < 1) r = 1;
            a = atan2(y, x) + pi;
            u = (a * 128 / pi);// + (sintab[((int)r << 9) & 0xFFFF] >> 10);
            v = (r);// + (sintab[(u << 10) & 0xFFFF] >> 12);
            tunnel[i++] = (u&0xFF) + ((v&0xFF)<<8);
        }
    
}
int dtick = 0;

void drawtunnel (int c) {
    int u1 = (sintab[((c << 8)) & 0xFFFF] >> 8) + (sintab[((c << 8)) & 0xFFFF] >> 9);
    int v1 = (c << 2) + (sintab[((c << 7)) & 0xFFFF] >> 9);
    
    //int u2 = sintab[(c << 5) & 0xFFFF] >> 8;
    //int v2 = sintab[(c << 3) & 0xFFFF] >> 9;
    int texofs1 = ((v1 << 8) + u1) &0xFFFF;
    //int texofs2 = ((v2 << 8) + u2) &0xFFFF;
    int i = 0;
    unsigned char *scrptr = (unsigned char*)&buffer;
    int k = 0;
    
    for (k = 0; k < X_SIZE*Y_SIZE; k++)
        *scrptr++ = texture[(tunnel[k]+texofs1) & 0xFFFF];

}
                                            
void blur();
#pragma aux blur =  " mov   esi, offset buffer    " \
                    " mov   ecx, 224000           " \
                    " xor   eax, eax              " \
                    " xor   ebx, ebx              " \
                    " __inner:                     " \
                    " mov   al,  [esi - 1]        " \
                    " mov   bl,  [esi + 1]        " \
                    " add   eax, ebx              " \
                    " shr   eax, 1                " \
                    " mov   [esi], al             " \
                    " inc   esi                   " \
                    " dec   ecx                   " \
                    " jnz   __inner                " \
                    modify [esi eax ebx ecx];

volatile int tick = 0, diftick = 0;

void timer() { tick++; diftick++; }

void initpal() {  
    int i;

    for (i = 0; i < 256; i++) {
        pal[i].b = sat(((i >> 3) + (i >> 4)), 63);
        pal[i].g = sat(((i >> 3) + (i >> 3) + (i >> 4)), 63);
        pal[i].r = sat(((i >> 2) + (i >> 3)), 63);
    }
}

int main() {
    int i, si = 0, j, p = 0;
    int atick;
    
    initpal();
    initsintab();
    buildtexture();
    buildtunnel();

    if (ptc_open("", X_SIZE, Y_SIZE, 8, 0)) {return 0;}
    
    /*
    outp(0x3C8, 0);
    for (i = 0; i < 256; i++) {
        
        outp(0x3C9, sat(((i >> 3) + (i >> 4)), 63));
        outp(0x3C9, sat(((i >> 3) + (i >> 3) + (i >> 4)), 63));
        outp(0x3C9, sat(((i >> 2) + (i >> 3)), 63)); 
        
    }
    */
    
    ptc_setpal(&pal[0]);
    
    rtc_initTimer(3);
    rtc_setTimer(&timer, rtc_timerRate / 60);

    i = 0;
    while (!kbhit()) {
        _disable(); i = tick; _enable();
        si++;
        
        ptc_wait();

        outp(0x3C8, 0); outp(0x3C9, 63); outp(0x3C9, 0); outp(0x3C9, 0);
        
        ptc_update(&buffer);

        drawtunnel(i);
        
        outp(0x3C8, 0); outp(0x3C9, 0); outp(0x3C9, 0); outp(0x3C9, 0); 
    }
    getch();
    rtc_freeTimer();
    _asm {
        mov  ax, 3
        int  10h
    }
}
