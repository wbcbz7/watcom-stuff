#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <ctype.h>
#include <math.h>
#include <i86.h>

#include "..\..\flexptc\flexptc.h"
#include "..\..\ims06\xmplay.h"
#include "..\..\ims06\mcp.h"

#define X_RES 320
#define Y_RES 200

#define sgn(a)    (a < 0 ? -1 : 1)
#define sat(a, l) (a < l ? a : l)
#define sqr(a)    (a * a)
#define ee         1.0E-6
#define bb        (1.2f * 1.0E+2)
#define pi         3.141592653589793
#define mk15(r, g, b) ((b << 0) | (g << 5) | (r << 10))

// extern sin\cos tables
extern signed int sintab[65536],  costab[65536];
extern float      sintabf[65536], costabf[65536];

// class definition
#include "fdsphere.h"

fdsphere::fdsphere() {
    fdtab       = new fdgrid[41*26];
    buffer      = new unsigned short[X_RES*Y_RES];
    buffer2     = new unsigned short[X_RES*Y_RES];
    flare       = new unsigned short[flaresize*flaresize*flaretypes*2];
    p           = new vec4f[vertexcount];
    pt          = new vec4f[vertexcount];
    p2d         = new vec2f[vertexcount];
    n           = new vec4f[flarescount];
    nt          = new vec4f[flarescount];
    n2d         = new vec2f[flarescount];
    f           = new line [linesnum];
    d           = new dlist[Y_RES];
    
    // allocate memory for texture and align pointer to 128kb boundary
    alignbuf    = new unsigned short[2*65536];
    texture     = (unsigned short*)((((unsigned int)alignbuf)  + 0x1FFFF) & ~0x1FFFF);
    
    FILE *f = fopen("gfx\\fdsphere.tga", "rb");
    if (f) {
        fseek(f, 18, SEEK_SET);
        fread(texture, sizeof(unsigned short), 65536, f);
        fclose(f);
    }
    
    // call init stuff
    initflares();
    initvertex(mk15(4,4,4));
}

fdsphere::~fdsphere() {
    delete[] alignbuf;
    delete[] fdtab;
    delete[] buffer;
    delete[] flare;
    delete[] p;
    delete[] pt;
    delete[] p2d;
    delete[] n;
    delete[] nt;
    delete[] n2d;
    delete[] f;
    delete[] d;
}

void fdsphere::initflares() {
    unsigned short *p = flare;
    
    srand(0);
    int rsc, bsc, gsc;
    
    for (int i = 0; i < (flaretypes*2); i++) {
        if (!(i & 1)) {rsc = (rand() % 0x80) + 0x3f; gsc = (rand() % 0x80) + 0x3f; bsc = (rand() % 0x80) + 0x3f;}
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

void fdsphere::initvertex(int color) {
    vec4f *t   = p;
    vec4f *d   = n;
    line  *v   = f;
    
    // init vertices
    int max = vertexnum;
    float scale = 0.5 * ((float)(max + 1) / max);
    float rat = (65535 / max);
    for (int teta = 0; teta < max; teta++) {
        for (int phi = 0; phi < max; phi++) {
            t->x = vertexdist * sintabf[((int)(phi * rat*scale) + 0x0000) & 0xFFFF] * costabf[((int)(teta * rat) + 0x0000) & 0xFFFF];
            t->y = vertexdist * sintabf[((int)(phi * rat*scale) + 0x0000) & 0xFFFF] * sintabf[((int)(teta * rat) + 0x0000) & 0xFFFF];
            t->z = vertexdist * costabf[((int)(phi * rat*scale) + 0x0000) & 0xFFFF];
            t->w = 1;
            t++;
        }
    }
    
    memset(v, 0, sizeof(line)*linesnum);
    // init lines
    for (int teta = 0; teta < (max-1); teta++) {
        for (int phi = 0; phi < max; phi++) {
            v->a   = (phi*max + (teta % max));
            v->b   = (phi*max + ((teta + 1) % max));
            v->col = color;
            v++;
        }
    }
    
    for (int teta = 0; teta < max; teta++) {
        for (int phi = 0; phi < max; phi++) {
            v->a   = ((phi % max)*max + teta);
            v->b   = (((phi + 1) % max)*max + teta);
            v->col = color;
            v++;
        }
    }
     
    for (int i = 0; i < flarescount; i++) {
        d->x = flaresdist*sintabf[((i << 9) + (i << 10)) & 0xFFFF];
        d->y = flaresdist*costabf[(i << 9) & 0xFFFF];
        d->z = flaresdist*sintabf[(i << 10) & 0xFFFF];
        d->w = 1;
        d++;
    }
}

void fdsphere::project3d(vec2f *f, vec4f *v) {
    float t;
    
    if (v->z > 0) {
        t = dist / (v->z + ee);
        f->x = (v->x * t) + (X_RES >> 1);
        f->y = (v->y * t) + (Y_RES >> 1);
    } else f->x = -X_RES;
}

void fdsphere::drawflare(vec2f *f, int type) {
    unsigned short *p;
    unsigned short *v;
    
    static int x_adj, u_adj, fsize, fsize2, spr_index;
    
    int px = f->x - (flaresize/2), py = f->y - (flaresize/2);
    
    // dirty way of clipping :)
    if ((px <= -(flaresize-1)) || (px >= X_RES) || (py <= -flaresize) || (py >= Y_RES)) return;
    
    // set static vars
    fsize = flaresize; fsize2 = flaresize/2; u_adj = 0; x_adj = sizeof(unsigned short)*(X_RES - flaresize);
    spr_index = (type << 1) | (px & 1);
    
    px &= ~1; // small fixup
        
    v = &flare[flaresize*(flaresize*spr_index + (py < 0 ? -py : 0)) + (px < 0 ? -px : 0)];
        
    if (px < 0) {fsize2 = (px + flaresize)/2; u_adj = sizeof(unsigned short)*(-px); x_adj += sizeof(unsigned short)*(-px); px = 0;}
    if (px >= (X_RES - flaresize)) {
        fsize2 = (X_RES - px); u_adj = sizeof(unsigned short)*(flaresize - fsize2);
        x_adj += sizeof(unsigned short)*(flaresize - fsize2);
        fsize2 >>= 1;
    }
    
    if (py < 0) {fsize = py + flaresize; py = 0;}
    if (py >= (Y_RES - flaresize)) fsize = Y_RES - py;
    
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

void fdsphere::linedraw(vec2f *a, vec2f *b, line *f) {

    int delta_x, delta_y, dx, dy, t, d;
    int xerr = 0, yerr = 0;
    
    int sx, sy;
    unsigned short *p = buffer;
    long col;
    
    // determine dx and dy
    delta_x = b->x - a->x;
    delta_y = b->y - a->y;
    // determine steps by x and y axes (it will be +1 if we move in forward
    // direction and -1 if we move in backward direction
    if (delta_x > 0) dx = 1; else if (delta_x == 0) dx = 0; else dx = -1;
    if (delta_y > 0) dy = 1; else if (delta_y == 0) dy = 0; else dy = -1;
    delta_x = abs(delta_x);
    delta_y = abs(delta_y);
    // select largest from deltas and use it as a main distance
    if (delta_x > delta_y) d = delta_x; else d = delta_y;
    
    sx = a->x; sy = a->y;
    for (t = 0; t <= d; t++)	{	
        p = &buffer[(sy << 8) + (sy << 6) + (sx & ~1)];
        //col = 64;
        col = f->col << (sx & 1 ? 16 : 0);
        
        // saturated rgb555 addition from hugi #18 by BRAD
        // a bit modified by wbcbz7 :)
        _asm {
            mov     edi, p

            mov     eax, [edi]          // 1
            mov     ebx, col            // .
            mov     ecx, eax            // 2
            mov     edx, ebx            // .
            and     eax, 0x3DEF3DEF     // 3 <- msb inverted mask
            and     ebx, 0x3DEF3DEF     // .
            and     edx, 0x42104210     // 4 <- msb mask
            add     eax, ebx            // .
            and     ecx, 0x42104210     // 5
            mov     ebx, edx            // .
            or      edx, ecx            // 6 
            and     ebx, ecx            // .
            mov     ecx, edx            // 7
            and     edx, eax            // .
            or      eax, ecx            // 8
            or      edx, ebx            // .
            shr     edx, 4              // 9
            add     edi, 4              // .
            add     edx, 0x3DEF3DEF     // 10 <- add fixup
            xor     edx, 0x3DEF3DEF     // 11 <- xor fixup
            or      eax, edx            // 12 (stall)
            mov     [edi - 4], eax      // 13 (another stall)
        }
        
        // increasing error
        xerr += delta_x;
        yerr += delta_y;
        // if error is too big then we should decrease it by changing
        // coordinates of the next plotting point to make it closer
        // to the true line
        if (xerr > d) {	
            xerr -= d;
            sx += dx;
        }	
        if (yerr > d) {	
            yerr -= d;
            sy += dy;
        }	
    }
}

#define vcode(p) (((p->x < 0) ? 1 : 0) | ((p->x >= X_RES) ? 2 : 0) | ((p->y < 0) ? 4 : 0) | ((p->y >= Y_RES) ? 8 : 0))    
 
int fdsphere::lineclip(vec2f *a, vec2f *b) {
    int code_a, code_b, code;
    vec2f *c;
 
    code_a = vcode(a);
    code_b = vcode(b);
 
    while (code_a || code_b) {
        if (code_a & code_b)
            return -1;
 
        if (code_a) {
            code = code_a;
            c = a;
        } else {
            code = code_b;
            c = b;
        }
 
        if (code & 1) {
            c->y += (a->y - b->y) * (0 - c->x) / (a->x - b->x + ee);
            c->x = 0;
        } else if (code & 2) {
            c->y += (a->y - b->y) * (X_RES - c->x) / (a->x - b->x + ee);
            c->x = X_RES - 1;
        }

        if (code & 4) {
            c->x += (a->x - b->x) * (0 - c->y) / (a->y - b->y + ee);
            c->y = 0;
        } else if (code & 8) {
            c->x += (a->x - b->x) * (Y_RES - c->y) / (a->y - b->y + ee);
            c->y = Y_RES - 1;
        }
 
        if (code == code_a)
            code_a = vcode(a);
        else
            code_b = vcode(b);
    }
 
    return 0;
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

    float FOV = (1.0f / dist);
    
    int x, y, z, tptr = 0;
    unsigned int u, v;
    
    vec4f step = *ot, origin = *o, direction, intersect;
    float t, l, a, b, c;
    float tsq = sqr(SphereSize), _2a, delta, t1, t2;
    float _1pi = 1.0f / pi;
    
    rotate3dr(rx, ry, rz, &step);
    origin = vecadd(&origin, &step);
    float sqox = sqr(origin.x), sqoy = sqr(origin.y), sqoz = sqr(origin.z);

    for (y = 0; y < 26; y++) {
        for (x = 0; x < 41; x++) {
            
            direction.x = (float)(-(X_RES/2) + (x * 8)) * FOV;
            direction.y = (float)(-(Y_RES/2) + (y * 8)) * FOV;
            direction.z = 1;
            
            vecnormalize(&direction);
            rotate3dr(rx, ry, rz, &direction);
            
            a = sqr(direction.x) + sqr(direction.y) + sqr(direction.z);
            b = 2*(origin.x*direction.x + origin.y*direction.y + origin.z*direction.z);
            c = sqox + sqoy + sqoz - tsq;
            
            delta = b * b - 4 * a * c;
            if (delta >= 0) {
                delta = sqrt(delta);
                _2a   = 1.0f / (2 * a + ee);
                
                t1 = (-b + delta) * _2a;
                t2 = (-b - delta) * _2a;
    
                if ((t1 > 0) && (t2 > 0)) t = (t1 < t2 ? t1 : t2); else t = (t1 > 0 ? t1 : t2);
                
                intersect.x = origin.x + (direction.x * t);
                intersect.y = origin.y + (direction.y * t);
                intersect.z = origin.z + (direction.z * t);
                
                u = (unsigned int)((0.5f + (asin(intersect.z / SphereSize) *_1pi)) * 65536);
                v = (unsigned int)(fabs(((atan2(intersect.y, intersect.x) * 0.5f * _1pi)) * 2 * 65536));
            
                fdtab[tptr].u = u;// & 0xFFFFFF00;
                fdtab[tptr].v = v;// & 0xFFFFFF00;
            
                fdtab[tptr].i = 0;
            } else {
                fdtab[tptr].u = 0;// & 0xFFFFFF00;
                fdtab[tptr].v = 0;// & 0xFFFFFF00;
                fdtab[tptr].i = 0;
            }
            
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
        mov     edx, (X_RES*Y_RES) / 2
        
        _loop:
        mov     eax, [esi]      // 1
        mov     ebx, [edi]      // .
        
        mov     ecx, eax        // 2
        and     eax, 0x7BDE7BDE // .
        
        shr     eax, 1          // 3
        and     ecx, ebx        // .
        
        shr     ebx, 1          // 4
        and     ecx, 0x04210421 // .
        
        and     ebx, 0x3DEF3DEF // 5
        add     eax, ecx        // .
        
        add     eax, ebx        // 6
        add     esi, 4          // .
        
        mov     [edi], eax      // 7
        add     edi, 4          // .
        
        dec     edx             // 8
        jnz     _loop           // .
        
    }
}

void fdsphere::filldlist(dlist *d, int vel) {
    dlist *p = d;
    
    for (int i = 0; i < Y_RES; i++) {
        p->active = rand() & 1;
        p->disp   = (rand() % vel) & ~1; // two pixels boundary
        p->mask1  = 0x7FFF7FFF; p->mask2  = 0;
        
        int flags = rand() % 7 + 1;
        if (flags & 1) {p->mask2 ^= 0x78007800; p->mask1 ^= 0x7C007C00;} flags >>= 1;
        if (flags & 1) {p->mask2 ^= 0x03C003C0; p->mask1 ^= 0x03E003E0;} flags >>= 1;
        if (flags & 1) {p->mask2 ^= 0x001E001E; p->mask1 ^= 0x001F001F;} flags >>= 1;
        
        p++;
    }
}

void fdsphere::dirblur(unsigned short *dst, unsigned int len, int disp, unsigned int mask1, unsigned int mask2) {
    unsigned short *p = dst;
    unsigned short *v = &dst[2*disp];
    unsigned int   l  = (len - 2*disp),
                   _edx = -4*disp;               
    static unsigned int m1, m2;
    
    m1 = mask1; m2 = mask2;
    
    _asm {
        mov     esi, p
        mov     ecx, l
        mov     edi, v
        mov     edx, _edx
        push    ebp
        
        _loop:
        mov     eax, [esi]
        add     esi, 4
        
        mov     ebx, [edi]
        and     eax, [m2]
        
        mov     ebp, [edi + edx]
        and     ebx, [m2]
        
        shr     eax, 1
        and     ebp, [m1]
        
        shr     ebx, 1
        add     edi, 4
        
        add     eax, ebx
        
        or      eax, ebp
        sub     ecx, 2
        
        mov     [edi + edx - 4], eax
        jnz     _loop
    
        pop     ebp
    }
}

void fdsphere::invert(unsigned short *dst) {
    unsigned short *p = dst;
    
    _asm {
        mov     esi, p
        mov     ecx, X_RES*Y_RES/2
        
        _loop:
        mov     eax, [esi]
        add     esi, 4
        
        xor     eax, 0x7FFF7FFF
        dec     ecx
        
        mov     [esi - 4], eax
        jnz     _loop
    }
}

int fdsphere::main(int noretrace) {
    int i = 0, j, di = 0;
    
    int dofd = 0, waitdofd = 0;
    
    int rawpos, order, row;
    
    int avel = 0;
    float alphaval = 0;
    
    int dirtybuf = 1, glvel = 0;
    
    int rotx = 0, roty = 0, rotz = 0;
    int rotnx = 0, rotny = 0, rotnz = 0;
    
    rawpos = (xmpGetRealPos() & 0x00FFFFFF); row = (rawpos >> 8) & 0xFF; order = (rawpos >> 16) & 0xFF;
    
    while ((order < 0x19) && (!kbhit())) {
        di = i; i = *tick; di = i - di; if (!noretrace) ptc_wait();
        
        if (!dirtybuf) ptc_update(buffer2); dirtybuf = 0;
        if (mcpIdle) {mcpIdle();}
        
        rawpos = (xmpGetRealPos() & 0x00FFFFFF); row = (rawpos >> 8) & 0xFF; order = (rawpos >> 16) & 0xFF;
        
        vec4f o, ot;
        
        o.x = (SphereSize*0.2f) * sintabf[(i << 8) & 0xFFFF];
        o.y = (SphereSize*0.2f) * sintabf[(i << 7) & 0xFFFF];
        o.z = (SphereSize*0.2f) * sintabf[(i << 8) & 0xFFFF];
        
        ot.x = 0;
        ot.y = 0;
        ot.z = (SphereSize*0.3f) * costabf[(i << 7) & 0xFFFF] - (SphereSize*0.3f);
        
        
        rotx = (int)(((i << 4) + (i << 5) + (int)(0x2000 * costabf[((i << 5) + (i << 6)) & 0xFFFF])) & 0xFFFF);
        roty = (int)(((i << 5) + (0 << 6) + (int)(0x2000 * sintabf[((i << 5) + (i << 5)) & 0xFFFF])) & 0xFFFF);
        rotz = (int)(((i << 4) + (i << 6) + (int)(0x2000 * costabf[((i << 5) + (i << 4)) & 0xFFFF])) & 0xFFFF);
        
        rotnx = 0x100*di;
        rotny = 0xC0*di;
        rotnz = 0x120*di;
        
        if ((order == 0x18) && (!(row & 63)) && (!waitdofd)) {dofd ^= 1; waitdofd = 1;} else waitdofd = 0;
        
        if (dofd) {
            drawgrid(&o, &ot, rotx, roty, rotz);
            interpolate();
        } else memset(buffer, 0, sizeof(unsigned short)*X_RES*Y_RES);
        
        for (j = 0; j < vertexcount; j++) {
            pt[j] = p[j];
            pt[j] = vecsub(&o, &pt[j]);
            rotate3d(rotx, roty, rotz, &pt[j]);
            pt[j] = vecsub(&ot, &pt[j]); 
            project3d(&p2d[j], &pt[j]);
            drawflare(&p2d[j], ((j ^ 0x5A5A) % flaretypes)); 
        }
        
        for (j = 0; j < flarescount; j++) {
            rotate3d(rotnx, rotny, rotnz, &n[j]);
            nt[j] = n[j];
            nt[j] = vecsub(&o, &nt[j]);
            rotate3d(rotx, roty, rotz, &nt[j]);
            nt[j] = vecsub(&ot, &nt[j]); 
            project3d(&n2d[j], &nt[j]);
            drawflare(&n2d[j], 0); 
        }
        
        // draw lines
        
        for (j = 0; j < linesnum; j++) {
            vec4f fa, fb, fd;
            vec2f ca, cb, cd;
            line cf;
            
            cf = f[j];
            fa = pt[cf.a]; fb = pt[cf.b];
            
            if ((fb.z <= 0) && (fa.z <= 0)) continue;
            if (fb.z < fa.z) {fd = fa; fa = fb; fb = fd;}
            
            if (fa.z <= 0){
                vec4f pa = fa;
                
                pa.y = fa.y + (fa.y + fb.y) * (-fb.z-dist) / (fa.z - fb.z);
                pa.x = fa.x + (fa.x + fb.x) * (-fb.z-dist) / (fa.z - fb.z);
                pa.z = dist;
                
                fa = pa;
            }
            
            project3d(&ca, &fa);
            project3d(&cb, &fb);
            
            if (ca.y > cb.y) {cd = cb; cb = ca; ca = cd;} 
            
            if (!lineclip(&ca, &cb)) {linedraw(&ca, &cb, &cf); }
        }
        
        blend();
        
        if (row % ((rand() % 3) + 1)) glvel = (rand() & 7) & ~1; else glvel = 0;
        
        // fill and draw dlist
        if (glvel) {
            unsigned short *pd = buffer2;
            dlist *dd = d;
            filldlist(d, glvel);
            dlist de;
            memcpy(&de, dd, sizeof(dlist));
            for (int k = 0; k < Y_RES; k++) {
                if (dd->active) {memcpy(&de, dd, sizeof(dlist));
                dirblur(pd, X_RES, de.disp, de.mask1, de.mask2);}
                dd++; pd += X_RES;
            }
        }
        
        if ((order >= 0x16) && (order <= 0x17)) {
           avel = (int)alphaval; alphafade(buffer2, 0, avel); alphaval += 0.05f*di; alphaval = sat(alphaval, 31);
        }
        
        if ((order == 0x18) && ((row & 63) < 2)) invert(buffer2);
    }
    if (kbhit()) {getch(); return 1;}
    
    return 0;
}
