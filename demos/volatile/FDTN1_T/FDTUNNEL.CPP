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
#include "fdtunnel.h"

fdtunnel::fdtunnel() {
    fdtab       = new fdgrid[41*26];
    buffer      = new unsigned short[X_RES*Y_RES];
    buffer2     = new unsigned short[X_RES*Y_RES];
    flare       = new unsigned short[flaresize*flaresize*flaretypes*2];
    p           = new vec4f[flarescount];
    pm          = new vec4f[flarescount];
    pt          = new vec4f[flarescount];
    p2d         = new vec2f[flarescount];
    
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
    initflares();
    initvertex();
    
}

fdtunnel::~fdtunnel() {
    delete[] alignbuf;
    delete[] alignbuf2;
    delete[] fdtab;
    delete[] buffer;
    delete[] buffer2;
    delete[] flare;
    delete[] p;
    delete[] pm;
    delete[] pt;
    delete[] p2d;
}

void fdtunnel::interpolate() {
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

void fdtunnel::inittexture() {
    unsigned short *p = texture;
    
    for (int i = 0; i < 65536; i++) {
        *p = (unsigned short)(((*p & 0x1E) >> 1) | ((*p & 0x3C0) >> 2) | ((*p & 0x7800) >> 3));
        p++;
    }
}

void fdtunnel::initltab() {
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

void fdtunnel::initflares() {
    unsigned short *p = flare;
    
    srand(0);
    int rsc, bsc, gsc;
    
    for (int i = 0; i < (flaretypes*2); i++) {
        if (!(i & 1)) {rsc = (rand() % 0xC0) + 0x3f; gsc = (rand() % 0xC0) + 0x3f; bsc = (rand() % 0xC0) + 0x3f;}
        for (int y = -(flaresize/2); y < (flaresize/2); y++) {
            for (int x = -(flaresize/2) - (i&1); x < (flaresize/2) - (i&1); x++) {
                // rgb888
                int b = sat((int)sat(flarescale / (x*x + y*y + ee), 0xFF) * bsc, 0xFFFF) >> 8;
                int g = sat((int)sat(flarescale / (x*x + y*y + ee), 0xFF) * gsc, 0xFFFF) >> 8;
                int r = sat((int)sat(flarescale / (x*x + y*y + ee), 0xFF) * rsc, 0xFFFF) >> 8;
                
                *p++ = ((r >> 3) << 10) | ((g >> 3) << 5) | ((b >> 3) << 0);
            }
        }
    }
}

void fdtunnel::initvertex() {
    vec4f *f = p;
    
    for (int i = 0; i < flaresnum; i++) {
        f->x = (rand() % flaredist) - (flaredist/2);
        f->y = (rand() % flaredist) - (flaredist/2);
        f->z = (rand() % flaredist) - (flaredist/2);
        f->w = 1;
        f++;
    }
}

void fdtunnel::project3d(vec2f *f, vec4f *v) {
    float t;
    
    if (v->z < 0) {
        t = dist / (v->z + ee);
        f->x = (v->x * t) + (X_RES >> 1);
        f->y = (v->y * t) + (Y_RES >> 1);
    } else f->x = -1;
}

void fdtunnel::drawflare(vec2f *f, int type) {
    unsigned short *p;
    unsigned short *v;
    
    static int x_adj, u_adj, fsize, fsize2, spr_index;
    
    int px = f->x - (flaresize/2), py = f->y - (flaresize/2);
    
    // dirty way of clipping :)
    if ((px < 0) || (px >= (X_RES - flaresize)) || (py <= -flaresize) || (py >= Y_RES)) return;
    
    // set static vars
    fsize = flaresize; fsize2 = flaresize/2; u_adj = 0; x_adj = sizeof(unsigned short)*(X_RES - flaresize);
    spr_index = (type << 1) | (px & 1);
    
    v = &flare[flaresize*(flaresize*spr_index + (py < 0 ? -py : 0)) + (px < 0 ? (px & ~1) : 0)];
    
    if (py < 0) {fsize = py + flaresize; py = 0;}
    if (py >= (Y_RES - flaresize)) fsize = Y_RES - py;
    
    px &= ~1; // small fixup
    p = &buffer[(py << 8) + (py << 6) + px];
    
    // saturated rgb555 addition from hugi #18 by BRAD
    // a bit modified by wbcbz7 :)
    _asm {
        mov     esi, v
        mov     edi, p
        mov     ecx, [fsize]
        
        _outer:
        push    ecx
        push    ebp
        mov     ecx, [fsize2]
        
        _inner:
        mov     eax, [edi]          // 1
        mov     ebx, [esi]          // .
        mov     ebp, eax            // 2
        mov     edx, ebx            // .
        and     eax, 0x3DEF3DEF     // 3 <- msb inverted mask
        and     ebx, 0x3DEF3DEF     // .
        and     edx, 0x42104210     // 4 <- msb mask
        add     eax, ebx            // .
        and     ebp, 0x42104210     // 5
        mov     ebx, edx            // .
        or      edx, ebp            // 6 
        and     ebx, ebp            // .
        mov     ebp, edx            // 7
        and     edx, eax            // .
        or      eax, ebp            // 8
        or      edx, ebx            // .
        shr     edx, 4              // 9
        add     edi, 4              // .
        add     edx, 0x3DEF3DEF     // 10 <- add fixup
        add     esi, 4              // .
        xor     edx, 0x3DEF3DEF     // 11 <- xor fixup
        or      eax, edx            // 12 (stall)
        dec     ecx                 // .
        mov     [edi - 4], eax      // 13
        jnz     _inner              // .
        
        pop     ebp                 // 1
        pop     ecx                 // .
        add     esi, [u_adj]        // 2
        add     edi, [x_adj]        // .
        dec     ecx                 // 3
        jnz     _outer              // .
    }
}

void fdtunnel::drawgrid(int ax, int ay, int az, int rx, int ry, int rz, int adj) {
    enum {TunnelSize = 384};
    
    float _1dist = 1.0f / dist;
    int x, y, z, tptr = 0;
    unsigned int u, v;
    
    vec4f origin, direction, intersect;
    float t, l, fx, fy, delta, a, b, c, t1, t2;
    float tsq, _2a, _1pi, qx, qy;
    
    tsq = sqr(TunnelSize);
    _1pi = 1.0f / pi;
    
    origin.x = az;
    origin.y = ay;
    origin.z = ax;
    
    qx = sqr(origin.x);
    qy = sqr(origin.y);
    
    for (y = 0; y < 26; y++) {
        for (x = 0; x < 41; x++) {
            
            direction.x = (float)((x * 8) - 160) * _1dist;
            direction.z = 1;
            direction.y = (float)((y * 8) - 100) * _1dist;
            
            vecnormalize(&direction);
            rotate3dr(rx, ry, rz, &direction);
            
            a = (sqr(direction.x) + sqr(direction.y));
            b = 2 * (origin.x * direction.x + origin.y * direction.y);
            c = (qx + qy - tsq + sqr(256*costabf[((int)(0x5000 * (atan2(direction.y, direction.x))) + adj) & 0xFFFF]));
            
            delta = sqrt(b * b - 4 * a * c);
            //delta = sqrt(- 4 * a * c);
            _2a   = 1.0f / (2 * a + ee);
            
            t1 = (-b + delta) * _2a;
            t2 = (-b - delta) * _2a;

            t = (t1 > 0 ? t1 : t2);
            
            intersect.x = origin.x + (direction.x * t);
            intersect.y = origin.y + (direction.y * t);
            intersect.z = origin.z + (direction.z * t);
            
            u = (unsigned int)((intersect.z * 1 * 256) * _1pi);
            v = (unsigned int)((fabs(atan2(intersect.y, intersect.x)) * 2 * 65536 * _1pi));

            t = ((TunnelSize * 256) * 16) / t;
            z = (int)(sat(t, 4095));
            
            fdtab[tptr].u = u;// & 0xFFFFFFFF;
            fdtab[tptr].v = v;// & 0xFFFFFFFF;
            fdtab[tptr].l = z;// & 0xFFFFFFFF;
            
            if (z < 0x100) fdtab[tptr].i = 1; else fdtab[tptr].i = 0;
            
            tptr++;
        }
    }
}

void fdtunnel::blend() {
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

void fdtunnel::dirblur(int disp) {
    unsigned short *p   = &buffer[disp];
    unsigned short *v   = &buffer[2*disp];
    unsigned int   len  = (X_RES*Y_RES) - 2*disp,
                   _edx = (-1*disp) & ~1;
    _asm {
        mov     esi, p
        mov     ecx, len
        mov     edi, v
        mov     edx, _edx
        push    ebp
        
        _loop:
        mov     ax, [esi]
        add     esi, 2
        
        mov     bx, [edi]
        and     eax, 0x000003C0
        
        mov     bp, [edi + edx]
        and     ebx, 0x000003C0
        
        shr     eax, 1
        and     ebp, 0x00007C1F
        
        shr     ebx, 1
        add     edi, 2
        
        add     eax, ebx
        
        or      eax, ebp
        dec     ecx
        
        mov     [edi + edx - 2], ax
        jnz     _loop

        pop     ebp
    }
}

int fdtunnel::main(int timedemo) {
    int i, j;
    
    i = 0;
    while (timedemo ? (i < 1024) : !kbhit()) {
        if (timedemo) i++; else {i = *tick; ptc_wait();}
        
        ptc_update(buffer2);
        
        int   rot1  = (((i << 7) + (int)(0x4000 * costabf[((i << 6) + (i << 5)) & 0xFFFF])) & 0xFFFF),
              rot2  = (((i << 6) + (int)(0x4000 * sintabf[((i << 6)) & 0xFFFF])) & 0xFFFF),
              rot3  = (((i << 7) + (int)(0x8000 * costabf[((i << 7)) & 0xFFFF])) & 0xFFFF);
        float shx   = 128 * costabf[(i << 7) & 0xFFFF],
              shy   = 128 * costabf[(i << 7) & 0xFFFF];
        
        drawgrid((i << 3),
                 shy,
                 shx,
                 rot1,
                 rot2,
                 rot3,
                 ((i << 9) & 0xFFFF));
        
        interpolate();
        
        float scale = 0.5 * costabf[(i << 6) & 0xFFFF] + 0.8;
        
        for (j = 0; j < flarescount; j++) {
            pm[j] = p[j];
            pt[j] = pm[j];
            rotate3d(rot1 & 0xFFFF, rot2 & 0xFFFF, rot3 & 0xFFFF, &pt[j]);
            //vecmove(shx, shy, 0, &pt[j]);
            project3d(&p2d[j], &pt[j]);
            drawflare(&p2d[j], ((j ^ 0x5A5A) % flaretypes)); 
        }
        dirblur((int)(4 * costabf[(i << 11) & 0xFFFF] + 4));
        blend();
    }
    if (!timedemo) getch();
    
    if (timedemo) printf("timed %d timerticks in %d frames - %f fps", *tick, i, (double)(60 * i) / *tick);
    
    return 0;
}
