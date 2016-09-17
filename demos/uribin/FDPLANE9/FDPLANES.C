#include <stdlib.h>
#include <conio.h>
#include <math.h>
#include <memory.h>
#include "rtctimer.h"

unsigned char  *screen = (unsigned char*)0xA0000;


unsigned char  fdtexture[65536];
unsigned char  buffer[65536], fdbuffer[65536];

unsigned int   fdu[41 * 26], fdv[41 * 26], fdl[41 * 26];

unsigned char  fdpalxlat[65536], fdblendtab[65536];

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

#define fdDIST     150
#define fdspr_size 16
#define fdcount    64
#define fdist_x    256
#define fdist_z    4096

#define ee         1.0E-6
#define bb         (1.2f * 1.0E+3)
#define pi         3.141592653589793

void fdNormalize(vec3f *v) {
	float l = 1.0 / sqrt(v->x*v->x + v->y*v->y + v->z*v->z);

	v->x *= l;
	v->y *= l;
	v->z *= l;
}

void fdInterpolate() {
    typedef struct {int  sy0, sy1, sdy, ey0, ey1, edy, x0, x1, dx, sy, ey, sx;} _fd;
    
    int  x, y, i, j, k, gridptr = 0;
    long scrptr = (long)&fdbuffer;// + (320*20);
    _fd u, v, l;
    unsigned int xu, xv, xdu, xdv, xl, xdl;
    
    for (j = 0; j < 25; j++) {
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
                u.dx = (u.ey - u.sy) >> 3;
                u.sx = u.sy;
                
                v.dx = (v.ey - v.sy) >> 3;
                v.sx = v.sy;
                
                l.dx = (l.ey - l.sy) >> 3;
                l.sx = l.sy;
                
                
                // prepare vars 
                xu = u.sy;
                xv = v.sy;
                xl = l.sy;
                
                xdu = u.dx;
                xdv = v.dx;
                xdl = l.dx;
                
                for (x = 0; x < 8; x++) {
                    //*((char*)scrptr++) = fdtexture[(((xu >> 8) & 0xFF) | (xv & 0xFF00))];
                    *((char*)scrptr++) = fdpalxlat[(fdtexture[(((xu >> 8) & 0xFF) | (xv & 0xFF00))]) | (xl & 0xFF00)];
                    xu += xdu; xv += xdv; xl += xdl;
                }
                
                scrptr += (320 - 8);
                u.sy += u.sdy; u.ey += u.edy; v.sy += v.sdy; v.ey += v.edy; l.sy += l.sdy; l.ey += l.edy;
            }
            gridptr++;
            scrptr -= (320 * 8) -8;
        }
        gridptr++;
        scrptr += (320 * 7);
    } 
}

                //*((char*)sptr++) = (fdtexture[(((vx >> 8) & 0xFF) | (ux & 0xFF00))] * (tx >> 8)) >> 8;

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


void fdbuildTexture() {
    int x, y, i, k=0;
    
    /*
    for (y = 0; y < 256; y++) {
        for (x = 0; x < 256; x++) {
            //fdtexture[((y << 8) + x)] = sat((x ^ y), 255) & 0xFF;
            fdtexture[((y << 8) + x)] = (x ^ y) & 0xFF;
            //fdtexture[((y << 8) + x)] = (x ^ y) | (rand() % 0x100) & 0xFF;
        }
    }
    */
    
    
    for (y = -128; y < 128; y++) {
        for (x = -128; x < 128; x++) {
            //fdtexture[k++] = sat(((int)(0x10000 / ((x*x ^ y*y) + ee))) + ((int)(0x10000 / ((x*x + y*y) + ee))), 255);
            fdtexture[k++] = sat(((int)(0x10000 / ((x*x ^ y*y) + ee))) - ((int)(0x10000 / ((x*x + y*y) + ee))), 255) & 0xFF;
            //texture[((y << 8) + x)] = (x ^ y) & 0xFF;
            //ttexture[((y << 8) + x)] = (rand() % 0x100) & 0xFF;
        }
    }
    
    
    // blur our texture
    for (k = 0; k < 2; k++)
    for (i = 0; i < 65536; i++) 
        fdtexture[i] = (fdtexture[(i-1)&0xFFFF] + fdtexture[(i+1)&0xFFFF] + 
                        fdtexture[(i-256)&0xFFFF] + fdtexture[(i+256)&0xFFFF]) >> 2; 
    
}

void fdCalcPlanes(int ax, int ay, int az, int rx, int ry, int rz) {

    float FOV = (1.0f / 60);
    enum {PlaneSize = 192};
    
    int x, y, z, tptr = 0;
    unsigned int u, v;
    
    vec3f origin, direction, intersect;
    float t, l, fx, fy, delta, a, b, c, t1, t2;
    float tsq, _2a;

    // ASSUMING origin.y = 0!
    

    for (y = 0; y < 26; y++) {
        for (x = 0; x < 41; x++) {
        
            origin.x = az + (sintab[((x << 11) + (ax << 2)) & 0xFFFF] >> 10);
            origin.y = ay;// + (64 * sintabf[(ax << 5) & 0xFFFF]);  
            origin.z = ax + (costab[((y << 11) + (ax << 2)) & 0xFFFF] >> 10);
            
            /*
            direction.x = (float)((x * 8) - 160 + (16 * costabf[((x << 10) + (ax << 3)) & 0xFFFF])) * FOV;
            direction.z = 1 + 0.5f * sintabf[((ax << 5)) & 0xFFFF];
            direction.y = (float)((y * 8) - 100 + (16 * sintabf[((y << 10) + (ax << 3)) & 0xFFFF])) * FOV;
            */
            
            
            direction.x = (float)((x << 3) - 160) * FOV;
            direction.z = 1;
            direction.y = (float)((y << 3) - 100) * FOV;
            
            
            fdNormalize(&direction);
            fd3dRotate(rx, ry, rz, &direction);
            
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
                                            
void fdBlur(unsigned long);
#pragma aux fdBlur =  " mov   esi, offset buffer[4]    " \
                      " mov   edi, edx              " \
                      " not   edi                   " \
                      " mov   ecx, 64000-8           " \
                      " xor   eax, eax              " \
                      " xor   ebx, ebx              " \
                      " @inner:                     " \
                      " mov   al,  [esi + edi]        " \
                      " mov   bl,  [esi + edx]        " \
                      " add   eax, ebx              " \
                      " shr   eax, 1                " \
                      " mov   [esi], al             " \
                      " inc   esi                   " \
                      " dec   ecx                   " \
                      " jnz   @inner                " \
                      parm [edx] modify [esi eax ebx ecx];

volatile int tick = 0, diftick = 0;

void timer() { tick++; diftick++; }

int main() {
    int i, j, p = 0;
    int atick;
    
    fdbuildSinTable();
    fdbuildTexture();
    
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
        
        outp(0x3C9, sat(((i >> 3) + (i >> 4)), 63));
        outp(0x3C9, sat(((i >> 3) + (i >> 3) + (i >> 4)), 63));
        outp(0x3C9, sat(((i >> 2) + (i >> 3)), 63)); 
        
    }
    for (j = 0; j < 16; j++) for (i = 0; i < 256; i++) fdpalxlat[p++] = sat(((i * j)) >> 4, 255); p = 0;
    for (j = 0; j < 256; j++) for (i = 0; i < 256; i++) fdblendtab[p++] = sat(((i * 128) >> 8) + ((j * 192) >> 8), 255);
    
    rtc_initTimer(3);
    rtc_setTimer(&timer, rtc_timerRate / 60);

    while (!kbhit()) {
        if (i == tick) while(i == atick) {_disable(); atick = tick; _enable();};
        i = tick; dtick = diftick; diftick = 0;
        
        while ((inp(0x3DA) & 8) == 8) {}
        while ((inp(0x3DA) & 8) != 8) {}

        //outp(0x3C8, 0); outp(0x3C9, 63); outp(0x3C9, 0); outp(0x3C9, 0);
        
        memcpy(screen, &buffer, 64000);    
        
        /*
        fdCalcPlanes((i << 4), 0, 0, 
                    ((i << 8) & 0xFFFF), ((i << 7) & 0xFFFF), (((i << 7) + (i << 5)) & 0xFFFF));
        */            
        
        //fdCalcPlanes((i << 3), 0, 0, 0, 0, 0);

        
        fdCalcPlanes((i << 2), 0, 0, 
                    (((i << 8) + (int)(0x8000 * costabf[(i << 7) & 0xFFFF])) & 0xFFFF),
                    (((i << 8) + (int)(0x0000 * sintabf[(i << 6) & 0xFFFF])) & 0xFFFF),
                    (((i << 8) + (int)(0x8000 * sintabf[(i << 7) & 0xFFFF])) & 0xFFFF)
                    );
                

        //outp(0x3C8, 0); outp(0x3C9, 63); outp(0x3C9, 63); outp(0x3C9, 0);
        
        fdInterpolate();
        //outp(0x3C8, 0); outp(0x3C9, 63); outp(0x3C9, 0); outp(0x3C9, 63);
        fdBlend();
        p = abs(sintab[(i << 8) & 0xFFFF] >> 12);
        /*for (i = 0; i < p; i++)*/ fdBlur(p);
        
        outp(0x3C8, 0); outp(0x3C9, 0); outp(0x3C9, 0); outp(0x3C9, 0); 
    }
    getch();
    rtc_freeTimer();
    _asm {
        mov  ax, 3
        int  10h
    }
}
