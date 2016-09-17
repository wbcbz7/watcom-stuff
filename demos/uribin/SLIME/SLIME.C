#include <stdlib.h>
#include <conio.h>
#include <math.h>
#include "rtctimer.h"

unsigned char  *screen = (long)0xA0000;


unsigned char  fdtexture[65536];
unsigned char  fdbuffer[64000], buffer[64000];
unsigned char  fdlightmap[64000], fdblendtab[65536];

unsigned int   fdu[41 * 26], fdv[41 * 26], fdl[41 * 26];

unsigned char  fdpalxlat[65536];

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
#define bb         1.0E+4
#define pi         3.141592653589793

unsigned char fdspr[fdspr_size * fdspr_size];

vec3f fdp[fdcount], fdpt[fdcount];
vec2  fdp2d[fdcount];


void fdFillDots () {
    int i, lptr = 0;  
    
    for (i = 0; i < fdcount; i++) {
        fdp[lptr  ].x = (rand() % fdist_x) - (fdist_x >> 1);
        fdp[lptr  ].y = (rand() % fdist_x) - (fdist_x >> 1);
        fdp[lptr++].z = (rand() % fdist_x) - (fdist_x >> 1); 
    }
}


void fdMakeSprite() {
    int x, y, k;
    long sprptr = (long)&fdspr;
    
    for (y = -(fdspr_size >> 1); y < (fdspr_size >> 1); y++) {
        for (x = -(fdspr_size >> 1); x < (fdspr_size >> 1); x++) {
            *((char*)sprptr++) = sat((int)(0x80 / ((x*x + y*y) + ee)), 128) & 0xFF;
        }
    }
}

void fd3dMove (float ax, float ay, float az) {
    int i;
    
    for (i = 0; i < fdcount; i++) {
        fdpt[i].x += ax;
        fdpt[i].y += ay;
        fdpt[i].z += az;
    }
}

void fd3dProject() {
    int i;
    float t;
    for (i = 0; i < fdcount; i++) if (fdpt[i].z < 0) {
        t = fdDIST / (fdpt[i].z + ee);
        fdp2d[i].x = fdpt[i].x * t + 160;
        fdp2d[i].y = fdpt[i].y * t + 100;
    }
}

void fdDrawPoints () {
    int i, y, x, j;
    int px, py, ofs;
    long scrptr = (long)&fdbuffer;
    long sprptr = (long)&fdspr;
    
    for (i = 0; i < fdcount; i++) {
        sprptr = (long)&fdspr;
        px = fdp2d[i].x - (fdspr_size >> 1); py = fdp2d[i].y - (fdspr_size >> 1);
        
        if ((py<(200 - (fdspr_size)))&&(py>0)&&(px>0)&&(px<(312 - (fdspr_size)))&&(fdpt[i].z < 0)) {
            j = 0;
            scrptr = (long)&fdbuffer + ((py << 8) + (py << 6) + px);
            for (y = 0; y < (fdspr_size); y++) {
                for (x = 0; x < (fdspr_size); x++) {
                    *((char*)scrptr) = sat((*((char*)scrptr) + *((char*)sprptr)), 255);
                    scrptr++; sprptr++;
                }
                scrptr += (320 - (fdspr_size));
            }
        }
    }
}

void fdNormalize(vec3f *v) {
	float l = 1.0 / sqrt(v->x*v->x + v->y*v->y + v->z*v->z);

	v->x *= l;
	v->y *= l;
	v->z *= l;
}

void fdInterpolate() {
    typedef struct {int  sy0, sy1, sdy, ey0, ey1, edy, x0, x1, dx, sy, ey, sx;} _fd;
    
    int  x, y, i, j, k, gridptr = 0;
    long scrptr = (long)&fdbuffer + (320*20);
    _fd u, v, l;
    unsigned int xu, xv, xdu, xdv, xl, xdl;
    
    for (j = 0; j < 20; j++) {
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
                    *((char*)scrptr++) = fdtexture[(((xu >> 8) & 0xFF) | (xv & 0xFF00))];
                    xu += xdu; xv += xdv;
                }
                
                scrptr += (320 - 8);
                u.sy += u.sdy; u.ey += u.edy; v.sy += v.sdy; v.ey += v.edy;// l.sy += l.sdy; l.ey += l.edy;
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
    
    
    for (y = 0; y < 256; y++) {
        for (x = 0; x < 256; x++) {
            //fdtexture[((y << 8) + x)] = sat((x ^ y), 255) & 0xFF;
            fdtexture[((y << 8) + x)] = (x ^ y) & 0xFF;
            //fdtexture[((y << 8) + x)] = (x ^ y) | (rand() % 0x100) & 0xFF;
        }
    }
    
    /*
    for (y = -128; y < 128; y++) {
        for (x = -128; x < 128; x++) {
            //fdtexture[k++] = sat(((int)(0x8000 / ((x*x ^ y*y) + ee))), 255);
            fdtexture[k++] = sat(((int)(0x10000 / ((x*x ^ y*y) + ee))) - ((int)(0x10000 / ((x*x + y*y) + ee))), 255) & 0xFF;
            //texture[((y << 8) + x)] = (x ^ y) & 0xFF;
            //ttexture[((y << 8) + x)] = (rand() % 0x100) & 0xFF;
        }
    }
    */
    
    // blur our texture
    for (k = 0; k < 1; k++)
    for (i = 0; i < 65536; i++) 
        fdtexture[i] = (fdtexture[(i-1)&0xFFFF] + fdtexture[(i+1)&0xFFFF] + 
                        fdtexture[(i-256)&0xFFFF] + fdtexture[(i+256)&0xFFFF]) >> 2; 
    
}

void fdCalcPlanes(int ax, int ay, int az, float scale, int rx, int ry, int rz) {

    float FOV = (1.0f / 120);
    const PlaneSize = 2;
    
    int x, y, z, tptr = 0;
    unsigned int u, v;
    
    vec3f origin, direction, intersect;
    float t, l, fx, fy, delta, a, b, c, t1, t2;
    float tsq, _2a;

    for (y = 0; y < 21; y++) {
        for (x = 0; x < 41; x++) {
            
            origin.x = ax;// + (sintab[((x << 11) + (az << 4)) & 0xFFFF] >> 8);
            origin.y = ay;// + (costab[((y << 10) + (az << 4)) & 0xFFFF] >> 8);
            origin.z = 0;
            
            direction.x = (float)((x << 3) - 160) * PlaneSize;
            direction.y = (float)((y << 3) - 100) * PlaneSize * 1.2;
            direction.z = 0;
            
            fd3dRotate(rx, ry, rz, &direction);
            
            t = scale;
            
            intersect.x = (origin.x + direction.x) * t;
            intersect.y = (origin.y + direction.y) * t;
            intersect.z = (origin.z + direction.z) * t;

            u = (unsigned int)((intersect.x) * 256);
            v = (unsigned int)((intersect.y) * 256);
            
            fdu[tptr] = u & 0xFFFFFFFF;
            fdv[tptr] = v & 0xFFFFFFFF;
            
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
                      " inc   ebx                   " \
                      " mov   dl,    [edi]          " \
                      " mov   al,    [esi + edx]    " \
                      " mov   [edi], al             " \
                      " inc   edi                   " \
                      " dec   ecx                   " \
                      " jnz   @inner                " \
                      modify [esi edi ebx ecx edx];

int tick = 0, diftick = 0;

void timer() { tick++; diftick++; }

int main() {
    int i, j, p = 0;
    int atick;
    
    fdbuildSinTable();
    fdbuildTexture();
    fdFillDots();
    fdMakeSprite();
    
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
        
        outp(0x3C9, sat(((i >> 2) + (i >> 4)), 63));
        outp(0x3C9, sat(((i >> 2) + (i >> 4)), 63));
        outp(0x3C9, sat(((i >> 3) + (i >> 3) + (i >> 4)), 63)); 
        
        
        /*
        outp(0x3C9, sat((i >> 2), 63));
        outp(0x3C9, sat((i >> 2), 63)); 
        outp(0x3C9, sat((i >> 2), 63));
        */    
    }
    for (j = 0; j < 32; j++) for (i = 0; i < 256; i++) fdpalxlat[p++] = (i * j) >> 4; p = 0;
    //for (j = 0; j < 256; j++) for (i = 0; i < 256; i++) fdblendtab[p++] = sat(((i * 224) >> 8) + ((j * 32) >> 8), 255);
    for (j = 0; j < 256; j++) for (i = 0; i < 256; i++) fdblendtab[p++] = sat(((i * 128) >> 8) + ((j * 128) >> 8), 255);
    
    rtc_initTimer(3);
    rtc_setTimer(&timer, rtc_timerRate / 60);

    while (!kbhit()) {
        if (i == tick) while(i == atick) {_disable(); atick = tick; _enable();};
        i = tick; dtick = diftick; diftick = 0;
        //i++;        

        while ((inp(0x3DA) & 8) == 8) {}
        while ((inp(0x3DA) & 8) != 8) {}

        //outp(0x3C8, 0); outp(0x3C9, 63); outp(0x3C9, 0); outp(0x3C9, 0);
        
        memcpy(screen, &buffer, 64000);    
        
        fdCalcPlanes((int)(0x200 * sintabf[(i << 6) & 0xFFFF]),
                     (int)(0x200 * costabf[(i << 7) & 0xFFFF]),
                     (i << 3), 
                     (0.5f + 0.4f * sintabf[(i << 8) & 0xFFFF]),
                     0,
                     0,
                     ((i << 7) + (i << 6) + (int)(0x8000 * costabf[(i << 7) & 0xFFFF])) & 0xFFFF);
        
        //outp(0x3C8, 0); outp(0x3C9, 63); outp(0x3C9, 63); outp(0x3C9, 0);
        
        fdInterpolate();
        //outp(0x3C8, 0); outp(0x3C9, 63); outp(0x3C9, 0); outp(0x3C9, 63);
        fdBlend();
        
        //outp(0x3C8, 0); outp(0x3C9, 0); outp(0x3C9, 0); outp(0x3C9, 0); 
    }
    getch();
    rtc_freeTimer();
    _asm {
        mov  ax, 3
        int  10h
    }
}
