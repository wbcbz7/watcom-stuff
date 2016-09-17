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
#include "matrix.h"

// class definition
#include "fdsphere.h"

fdsphere::fdsphere() {
    fdtab       = new fdgrid[41*26];
    buffer      = new unsigned short[X_RES*Y_RES];
    buffer2     = new unsigned short[X_RES*Y_RES];
    
    // allocate memory for texture and align pointer to 128kb boundary
    alignbuf    = new unsigned short[4*65536];
    texture     = (unsigned short*)((((unsigned int)alignbuf)  + 0x1FFFF) & ~0x1FFFF);
    
    FILE *f = fopen("dutch.tga", "rb");
    if (f) {
        fseek(f, 18, SEEK_SET);
        fread(texture, sizeof(unsigned short), 65536, f);
        fclose(f);
    }
    
    // call init stuff
    
}

fdsphere::~fdsphere() {
    delete[] alignbuf;
    delete[] fdtab;
    delete[] buffer;
}

void fdsphere::interpolate() {
    typedef struct {int sdy, edy, dx, sy, ey, sx;} _fd;
    
    int k, gridptr = 0;
    unsigned short *p = buffer;
    unsigned short *t = (unsigned short*)((unsigned int)texture  >> 1);
    _fd u, v;
    
    static int xu, xv, xdu, xdv;
    
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
            
            for (int y = 0; y < 8; y++) {
                
                xdu = (u.ey - u.sy) >> 3;
                xu = u.sy;
                
                xdv = (v.ey - v.sy) >> 3;
                xv = v.sy;
                
                
                _asm {
                    mov     ebx, xu
                    mov     eax, t
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
                    add     ebp, 2              // .
                    
                    mov     [edi + ebp - 2], ax // 4
                    jnz     _loop               // .
                    
                    pop     ebp
                }
                
                p += X_RES;
                u.sy += u.sdy; u.ey += u.edy;
                v.sy += v.sdy; v.ey += v.edy;
            }
            gridptr++;
            p -= (X_RES * 8) - 8;
        }
        gridptr++;
        p += (X_RES * 7);
    } 
}

void fdsphere::drawgrid(vec4f *o, vec4f *ot, int rx, int ry, int rz) {

    float FOV = (1.0f / 90);
    
    int x, y, z, tptr = 0;
    unsigned int u, v;
    
    vec4f step = *ot, origin = *o, direction, intersect;
    float t, l, a, b, c;
    float tsq = sqr(SphereSize), _2a, delta, t1, t2;
    float _1pi = 1.0f / pi;
    float sqox = sqr(origin.x), sqoy = sqr(origin.y), sqoz = sqr(origin.z);
    
    //vecmatmulf(&step, &rot);
    rotate3d(rx, ry, rz, &step);
    
    for (y = 0; y < 26; y++) {
        for (x = 0; x < 41; x++) {
            
            direction.x = (float)((x * 8) - 160) * FOV;
            direction.z = 1;
            direction.y = (float)((y * 8) - 100) * FOV;
            
            vecnormalize(&direction);
            rotate3d(rx, ry, rz, &direction);
            
            a = sqr(direction.x) + sqr(direction.y) + sqr(direction.z);
            b = 2*(origin.x*direction.x + origin.y*direction.y + origin.z*direction.z);
            c = sqox + sqoy + sqoz - tsq;
            
            delta = sqrt(b * b - 4 * a * c);
            _2a   = 1.0f / (2 * a + ee);
            
            t1 = (-b + delta) * _2a;
            t2 = (-b - delta) * _2a;

            t = (t1 > 0 ? t1 : t2);
            
            intersect.x = origin.x + step.x + (direction.x * t);
            intersect.y = origin.y + step.y + (direction.y * t);
            intersect.z = origin.z + step.z + (direction.z * t);
            
            u = (unsigned int)((0.5f - (asin(intersect.y / SphereSize) *_1pi)) * 65536);
            v = (unsigned int)(fabs(((atan2(intersect.z, intersect.x) * 0.5f * _1pi)) * 2 * 65536));
            
            fdtab[tptr].u = u;// & 0xFFFFFF00;
            fdtab[tptr].v = v;// & 0xFFFFFF00;
            
            fdtab[tptr].i = 0;
            
            tptr++;
        }
    }
}

void fdsphere::blend() {
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

int fdsphere::main(int timedemo) {
    int i, j, p = 0;
    
    i = 0;
    while (timedemo ? (i < 1024) : !kbhit()) {
        if (timedemo) i++; else {i = *tick; ptc_wait();}
        
        ptc_update(buffer);
        
        vec4f o, ot;
        
        o.x = o.y = 0; o.z = 0;
        
        ot.z = 0;
        ot.x = 0;
        ot.y = 0;
        
        float rot1 = (i << 6) & 0xFFFF,
              rot2 = (i << 6) & 0xFFFF,
              rot3 = (i << 6) & 0xFFFF;
        
        drawgrid(&o, &ot, rot1, rot2, rot3);
        
        interpolate();
        //blend();
    }
    if (!timedemo) getch();
    
    if (timedemo) printf("timed %d timerticks in %d frames - %f fps", *tick, i, (double)(60 * i) / *tick);
    
    return 0;
}
