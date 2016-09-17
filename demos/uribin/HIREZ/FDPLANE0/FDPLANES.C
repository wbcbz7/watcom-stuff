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
unsigned char  buffer[X_SIZE * Y_SIZE], fdbuffer[X_SIZE * Y_SIZE];

unsigned int   fdu[41 * 26], fdv[41 * 26], fdl[41 * 26];

unsigned char  palxlat[65536], blendtab[65536];

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
#define bb         (1.2f * 1.0E+3)
#define pi         3.141592653589793

void normalize(vec3f *v) {
	float l = 1.0 / sqrt(v->x*v->x + v->y*v->y + v->z*v->z);

	v->x *= l;
	v->y *= l;
	v->z *= l;
}

void interpolate(int c) {
    typedef struct {int  sy0, sy1, sdy, ey0, ey1, edy, x0, x1, dx, sy, ey, sx;} _fd;
    
    int  x, y, i, j, k, gridptr = 0;
    unsigned char* scrptr = (unsigned char*)&fdbuffer + (X_SIZE * (c & 1));// + (320*20);
    _fd u, v, l;
    unsigned int xu, xv, xdu, xdv, xl, xdl;
    
    for (j = 0; j < 21; j++) {
        //scrptr = (long)&fdbuffer + (j << 11) + (j << 9);
        for (i = 0; i < 40; i++) {
            //scrptr = (long)&fdbuffer + (j << 11) + (j << 9) + (i << 3);
            
            u.sdy = (fdu[gridptr+41] - fdu[gridptr]) >> 3;
            u.sy  = (fdu[gridptr]);
            
            u.edy = (fdu[gridptr+42] - fdu[gridptr+1]) >> 3;
            u.ey  = (fdu[gridptr+1]);
            
            v.sdy = (fdv[gridptr+41] - fdv[gridptr]) >> 3;
            v.sy  = (fdv[gridptr]);
        
            v.edy = (fdv[gridptr+42] - fdv[gridptr+1]) >> 3;
            v.ey  = (fdv[gridptr+1]);
            
            l.sdy = (fdl[gridptr+41] - fdl[gridptr]) >> 3;
            l.sy  = (fdl[gridptr]);
        
            l.edy = (fdl[gridptr+42] - fdl[gridptr+1]) >> 3;
            l.ey  = (fdl[gridptr+1]);
            
            for (y = 0; y < 8; y++) {
                u.dx = (u.ey - u.sy) >> 4;
                u.sx = u.sy;
                
                v.dx = (v.ey - v.sy) >> 4;
                v.sx = v.sy;
                
                l.dx = (l.ey - l.sy) >> 4;
                l.sx = l.sy;
                
                
                // prepare vars 
                xu = u.sy;
                xv = v.sy;
                xl = l.sy;
                
                xdu = u.dx;
                xdv = v.dx;
                xdl = l.dx;
                
                for (x = 0; x < 16; x++) {
                    //*scrptr++ = texture[(((xu >> 8) & 0xFF) | (xv & 0xFF00))];
                    *scrptr++ = palxlat[(texture[(((xu >> 8) & 0xFF) | (xv & 0xFF00))]) | (xl & 0xFF00)];
                    xu += xdu; xv += xdv; xl += xdl;
                }
                
                scrptr += (X_SIZE*2 - 16);
                u.sy += u.sdy; u.ey += u.edy; v.sy += v.sdy; v.ey += v.edy; l.sy += l.sdy; l.ey += l.edy;
            }
            gridptr++;
            scrptr -= (X_SIZE * 16) - 16;
        }
        gridptr++;
        scrptr += (X_SIZE * 15);
    } 
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
    
    /*
    for (y = 0; y < 256; y++) {
        for (x = 0; x < 256; x++) {
            texture[k++] = (x ^ y) & 0xFF;
        }
    }
    */
    
    for (y = -128; y < 128; y++) {
        for (x = -128; x < 128; x++) {
            texture[k++] = sat(((int)(0x10000 / ((x*x ^ y*y) + ee))) - ((int)(0x10000 / ((x*x + y*y) + ee))), 255) & 0xFF;
        }
    }
    
    // blur our texture
    for (k = 0; k < 6; k++)
    for (i = 0; i < 65536; i++) 
        texture[i] = (texture[(i-1)&0xFFFF] + texture[(i+1)&0xFFFF] + 
                      texture[(i-256)&0xFFFF] + texture[(i+256)&0xFFFF]) >> 2; 
    
}

void fillgrid(int ax, int ay, int az, int rx, int ry, int rz, int fov) {

    float FOV = 0.008f + 0.004f * (sintabf[fov & 0xFFFF]);
    enum {PlaneSize = 192};
    
    int x, y, z, tptr = 0;
    unsigned int u, v;
    
    vec3f origin, direction, intersect;
    float t, l, fx, fy, delta, a, b, c, t1, t2;
    float tsq, _2a;

    // ASSUMING origin.y = 0!
    

    for (y = 0; y < 22; y++) {
        for (x = 0; x < 41; x++) {
        
            origin.x = az + (32 * costabf[((x << 11) + (ax << 5)) & 0xFFFF]);
            origin.y = ay;// + (64 * sintabf[(ax << 5) & 0xFFFF]);  
            origin.z = ax + (32 * sintabf[((y << 11) + (ax << 5)) & 0xFFFF]);
            
            
            
            direction.x = (float)((x * 16) - (X_SIZE/2) + (64 * costabf[((x << 10) + (ax << 4)) & 0xFFFF])) * FOV;
            direction.z = 1 + 0.5f * sintabf[((ax << 5)) & 0xFFFF];
            direction.y = (float)((y * 16) - (Y_SIZE/2) + (64 * sintabf[((y << 10) + (ax << 4)) & 0xFFFF])) * FOV;
            
            
            /*
            direction.x = (float)((x << 4) - (X_SIZE/2)) * FOV;
            direction.z = 1;
            direction.y = (float)((y << 4) - (Y_SIZE/2)) * FOV;
            */
            
            normalize(&direction);
            rotate3d(rx, ry, rz, &direction);
            
            t = (PlaneSize) / fabs(direction.y);

            intersect.x = origin.x + (direction.x * t);
            //intersect.y = origin.y + (direction.y * t);
            intersect.z = origin.z + (direction.z * t);

            u = (unsigned int)((intersect.x) * 256);
            v = (unsigned int)((intersect.z) * 256);
            
            fdu[tptr] = u;// & 0xFFFFFFFF;
            fdv[tptr] = v;// & 0xFFFFFFFF;
            
            t = sqrt(sqr(intersect.x - origin.x) + sqr(intersect.z - origin.z));	    
    
            if (t >= bb) {
                z = 0;
                fdu[tptr] = 0;
                fdv[tptr] = 0;
                }
            else {
                t = ((PlaneSize * 512) * 256) / t;
                z = ((unsigned short)sat(t, 65535)) >> 4;
            }
            fdl[tptr] = z;// & 0xFFFFFFFF;
            
            tptr++;
        }
    }
}
int dtick = 0;


void blend();
#pragma aux blend = " mov   esi, offset blendtab  " \
                    " mov   ebx, offset fdbuffer  " \
                    " mov   edi, offset buffer    " \
                    " mov   ecx, 224000           " \
                    " xor   edx, edx              " \
                    " __inner:                     " \
                    " mov   dh,    [ebx]          " \
                    " mov   dl,    [edi]          " \
                    " inc   ebx                   " \
                    " mov   al,    [esi + edx]    " \
                    " mov   [edi], al             " \
                    " inc   edi                   " \
                    " dec   ecx                   " \
                    " jnz   __inner                " \
                    modify [esi edi ebx ecx edx];
                                            
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

int main() {
    int i, si = 0, j, p = 0;
    int atick;
    
    initsintab();
    buildtexture();
    
    memset(&fdbuffer, 0, 64000);

    if (ptc_open("", X_SIZE, Y_SIZE, 8, 0)) {return 0;}
    
    outp(0x3C8, 0);
    for (i = 0; i < 256; i++) {
        
        outp(0x3C9, sat(((i >> 3) + (i >> 4)), 63));
        outp(0x3C9, sat(((i >> 3) + (i >> 3) + (i >> 4)), 63));
        outp(0x3C9, sat(((i >> 2) + (i >> 3)), 63)); 
        
    }
    for (j = 0; j < 16; j++) for (i = 0; i < 256; i++) palxlat[p++] = sat(((i * j)) >> 4, 255); p = 0;
    for (j = 0; j < 256; j++) for (i = 0; i < 256; i++) blendtab[p++] = sat(((i * 192) >> 8) + ((j * 128) >> 8), 255);
    
    rtc_initTimer(3);
    rtc_setTimer(&timer, rtc_timerRate / 60);

    i = 0;
    while (!kbhit()) {
        _disable(); i = tick; _enable();
        si++;
        
        ptc_wait();

        //outp(0x3C8, 0); outp(0x3C9, 63); outp(0x3C9, 0); outp(0x3C9, 0);
        
        ptc_update(&buffer);
        
        /*
        fdCalcPlanes((i << 4), 0, 0, 
                    ((i << 8) & 0xFFFF), ((i << 7) & 0xFFFF), (((i << 7) + (i << 5)) & 0xFFFF));
        */            
        
        //fdCalcPlanes((i << 3), 0, 0, 0, 0, 0);

        
        fillgrid((i << 3), 0, 0, 
                 (((i << 7) + (int)(0x8000 * costabf[(i << 5) & 0xFFFF])) & 0xFFFF),
                 (((i << 8) + (int)(0x0000 * sintabf[(i << 6) & 0xFFFF])) & 0xFFFF),
                 (((i << 7) + (int)(0x8000 * sintabf[(i << 5) & 0xFFFF])) & 0xFFFF),
                 (i << 7)
                );
                

        //outp(0x3C8, 0); outp(0x3C9, 63); outp(0x3C9, 63); outp(0x3C9, 0);
        
        interpolate(si);
        //outp(0x3C8, 0); outp(0x3C9, 63); outp(0x3C9, 0); outp(0x3C9, 63);
        blend();
        
        //outp(0x3C8, 0); outp(0x3C9, 0); outp(0x3C9, 0); outp(0x3C9, 0); 
    }
    getch();
    rtc_freeTimer();
    _asm {
        mov  ax, 3
        int  10h
    }
}
