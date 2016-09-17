#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <math.h>
#include <i86.h>
#include "..\flexptc\flexptc.h"
#include "tmap.h"

#define X_RES 320
#define Y_RES 200

#define sgn(a)    (a < 0 ? -1 : 1)
#define sat(a, l) (a < l ? a : l)
#define sqr(a)    (a * a)
#define ee         1.0E-6
#define bb        (1.2f * 1.0E+2)
#define pi         3.141592653589793

// extern sin\cos tables
extern signed int sintab[65536],  costab[65536];
extern float      sintabf[65536], costabf[65536];
#include "stuff.h"

// class definition
#include "fdplanes.h"

fdplanes::fdplanes() {
    fdtab       = new fdgrid[41*26];
    buffer      = new unsigned short[X_RES*Y_RES];
    buffer2     = new unsigned short[X_RES*Y_RES];
    
    // allocate memory for texture and align pointer to 128kb boundary
    alignbuf    = new unsigned short[4*65536];
    alignbuf2   = new unsigned short[4*65536];
    texture     = (unsigned short*)((((unsigned int)alignbuf)  + 0x1FFFF) & ~0x1FFFF);
    lightmap    = (unsigned short*)((((unsigned int)alignbuf2) + 0x1FFFF) & ~0x1FFFF);
    
    FILE *f = fopen("dutch.tga", "rb");
    if (f) {
        fseek(f, 18, SEEK_SET);
        fread(texture, sizeof(unsigned short), 65536, f);
        fclose(f);
    }
    
    // call init stuff
    inittexture();
    initltab();
    
}

fdplanes::~fdplanes() {
    delete[] alignbuf;
    delete[] alignbuf2;
    delete[] fdtab;
    delete[] buffer;
}

void fdplanes::interpolate() {
    typedef struct {int sdy, edy, dx, sy, ey, sx;} _fd;
    
    int k, gridptr = 0;
    unsigned short *p = buffer;
    unsigned short *t = (unsigned short*)((unsigned int)texture  >> 1);
    unsigned short *b = (unsigned short*)((unsigned int)lightmap >> 1);
    _fd u, v, l;
    
    static int xu, xv, xdu, xdv, xl, xdl;
    
    for (int j = 0; j < 25; j++) {
        for (int i = 0; i < 40; i++) {
            if ((fdtab[gridptr].i)   && (fdtab[gridptr+41].i) &&
                (fdtab[gridptr+1].i) && (fdtab[gridptr+42].i)) {
                    gridptr++; p += 8;
                    continue;
            }
            
            u.sdy = (fdtab[gridptr+41].u - fdtab[gridptr].u) >> 3;
            u.sy  = (fdtab[gridptr].u);
            
            u.edy = (fdtab[gridptr+42].u - fdtab[gridptr+1].u) >> 3;
            u.ey  = (fdtab[gridptr+1].u);
            
            v.sdy = (fdtab[gridptr+41].v - fdtab[gridptr].v) >> 3;
            v.sy  = (fdtab[gridptr].v);
        
            v.edy = (fdtab[gridptr+42].v - fdtab[gridptr+1].v) >> 3;
            v.ey  = (fdtab[gridptr+1].v);
            
            l.sdy = (fdtab[gridptr+41].l - fdtab[gridptr].l) >> 3;
            l.sy  = (fdtab[gridptr].l);
            
            l.edy = (fdtab[gridptr+42].l - fdtab[gridptr+1].l) >> 3;
            l.ey  = (fdtab[gridptr+1].l);
            
            for (int y = 0; y < 8; y++) {
                
                xdu = (u.ey - u.sy) >> 3;
                xu = u.sy;
                
                xdv = (v.ey - v.sy) >> 3;
                xv = v.sy;
                
                xdl = (l.ey - l.sy) << 1; // fixedpoint 4:12
                xl = l.sy << 4;
                
                _asm {
                    mov     ebx, xu
                    mov     eax, t
                    mov     ecx, b
                    mov     esi, xl
                    mov     edi, p
                    mov     edx, xv
                    add     edi, 16
                    push    ebp
                    mov     ebp, -16
                    
                    _loop:
                    mov     ah, bh              // 1
                    add     ebx, [xdu]          // .
                    
                    mov     al, dh              // 2
                    add     edx, [xdv]          // .
                    
                    mov     ax, [2 * eax]       // 3
                    
                    mov     cx, si              // 4
                    add     esi, [xdl]          // .
                    
                    and     ecx, 0xFFFFF000     // 5
                    
                    or      cx, ax              // 6
                    add     ebp, 2              // .
                    
                    mov     ax, [2 * ecx]       // 7
                    
                    mov     [edi + ebp - 2], ax // 8
                    jnz     _loop               // .
                    
                    pop     ebp
                }
                
                p += X_RES;
                u.sy += u.sdy; u.ey += u.edy;
                v.sy += v.sdy; v.ey += v.edy;
                l.sy += l.sdy; l.ey += l.edy;
            }
            gridptr++;
            p -= (X_RES * 8) - 8;
        }
        gridptr++;
        p += (X_RES * 7);
    } 
}

void fdplanes::inittexture() {
    unsigned short *p = texture;
    
    for (int i = 0; i < 65536; i++) {
        *p = (unsigned short)(((*p & 0x1E) >> 1) | ((*p & 0x3C0) >> 2) | ((*p & 0x7800) >> 3));
        p++;
    }
}

void fdplanes::initltab() {
    unsigned short *p = lightmap;
    
    for (int i = 0; i < 16; i++) {
        for (int r = 0; r < 16; r++) {
            for (int g = 0; g < 16; g++) {
                for (int b = 0; b < 16; b++) {
                    *p++ = (unsigned short)((((i * r) >> 3) << 10) | (((i * g) >> 3) << 5) | (((i * b) >> 3) << 0));
                }
            }
        }
    }
}

void fdplanes::drawgrid(int ax, int ay, int az, int rx, int ry, int rz) {

    float FOV = (1.0f / 50);
    enum {PlaneSize = 96};
    
    int x, y, z, tptr = 0;
    unsigned int u, v;
    
    vec3f origin, direction, intersect;
    float t, l;
    float tsq, _2a;

    // ASSUMING origin.y = 0!

    for (y = 0; y < 26; y++) {
        for (x = 0; x < 41; x++) {
        
            origin.x = az;// + (sintab[((x << 11) + (ax << 2)) & 0xFFFF] >> 10);
            origin.y = ay;// + (64 * sintabf[(ax << 5) & 0xFFFF]);  
            origin.z = ax;// + (costab[((y << 11) + (ax << 2)) & 0xFFFF] >> 10);
            
            direction.x = (float)((x * 8) - 160) * FOV;
            direction.z = 1;
            direction.y = (float)((y * 8) - 100) * FOV;
            
            normalize(&direction);
            rotate3d(rx, ry, rz, &direction);
            
            t = (PlaneSize - origin.y) / fabs(direction.y);
            //t = ((sgn(direction.y)*PlaneSize) - origin.y) / direction.y;

            intersect.x = origin.x + (direction.x * t);
            intersect.z = origin.z + (direction.z * t);

            u = (unsigned int)((intersect.x) * 256);
            v = (unsigned int)((intersect.z) * 256);
            

            t = ((PlaneSize * 256) * 16) / t;
            z = (int)(sat(t, 4095));
            
            fdtab[tptr].u = u;// & 0xFFFFFFFF;
            fdtab[tptr].v = v;// & 0xFFFFFFFF;
            fdtab[tptr].l = z;// & 0xFFFFFFFF;
            
            if (z < 0x100) fdtab[tptr].i = 1; else fdtab[tptr].i = 0;
            
            tptr++;
        }
    }
}

void fdplanes::blend() {
    unsigned short *p = buffer;
    unsigned short *v = buffer2;
    _asm {
        mov     esi, p
        mov     edi, v
        mov     ecx, (X_RES*Y_RES) / 2
        
        _loop:
        mov     eax, [esi]      // 1
        mov     ebx, [edi]      // .
        
        and     eax, 0x7BDE7BDE // 2
        and     ebx, 0x7BDE7BDE // .
        
        shr     eax, 1          // 3
        add     esi, 4          // .
        
        shr     ebx, 1          // 4
        add     edi, 4          // .
        
        add     eax, ebx        // 5
        dec     ecx             // .
        
        mov     [edi - 4], eax  // 6
        jnz     _loop           // .
    }
}

int fdplanes::main(int timedemo) {
    int i, j, p = 0;
    
    i = 0;
    while (timedemo ? (i < 1024) : !kbhit()) {
        if (timedemo) i++; else {i = *tick; ptc_wait();}
        
        ptc_update(buffer2);
        
        drawgrid((i << 2),
                 64 * costabf[(i << 7) & 0xFFFF], 0, 
                (((i << 8) + (int)(0x8000 * costabf[(i << 7) & 0xFFFF])) & 0xFFFF),
                (((i << 8) + (int)(0x0000 * sintabf[(i << 6) & 0xFFFF])) & 0xFFFF),
                (((i << 8) + (int)(0x8000 * sintabf[(i << 7) & 0xFFFF])) & 0xFFFF)
                );
        
        interpolate();
        blend();
    }
    if (!timedemo) getch();
    
    if (timedemo) printf("timed %d timerticks in %d frames - %f fps", *tick, i, (double)(60 * i) / *tick);
    
    return 0;
}
