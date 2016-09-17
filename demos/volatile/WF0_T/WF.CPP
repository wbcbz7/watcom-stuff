#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <math.h>
#include <i86.h>
#include "..\flexptc\flexptc.h"

#define X_RES 320
#define Y_RES 200

#define sat(a, l) (a < l ? a : l)
#define sqr(a)    (a * a)
#define ee         1.0E-6
#define bb        (1.2f * 1.0E+2)
#define pi         3.141592653589793
#define mk15(r, g, b) ((b << 0) | (g << 5) | (r << 10))

// extern sin\cos tables
extern signed int sintab[65536],  costab[65536];
extern float      sintabf[65536], costabf[65536];
#include "matrix.h"

// class definition
#include "wf.h"

wf::wf() {
    blurtab     = new unsigned short[X_RES*Y_RES*2];
    buffer      = new unsigned short[X_RES*Y_RES];
    buffer2     = new unsigned short[X_RES*Y_RES];
    flare       = new unsigned short[flaresize*flaresize*flaretypes*2];
    p           = new vec4f[flarescount];
    pt          = new vec4f[flarescount];
    p2d         = new vec2f[flarescount];
    f           = new line [linesnum];
    
    // call init stuff
    initblur();
    initflares();
    initvertex(mk15(16,16,16));
    
    memset(buffer,  0, sizeof(unsigned short)*X_RES*Y_RES);
    memset(buffer2, 0, sizeof(unsigned short)*X_RES*Y_RES);
}

wf::~wf() {
    delete[] blurtab;
    delete[] buffer;
    delete[] buffer2;
    delete[] flare;
    delete[] p;
    delete[] pt;
    delete[] p2d;
    delete[] f;
}

void wf::initflares() {
    unsigned short *p = flare;
    
    srand(120);
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

void wf::initvertex(int color) {
    vec4f *t = p;
    line  *v = f;
    
    // init vertices
    for (int i = 0; i < flaresnum; i++) {
        for (int j = 0; j < flaresnum; j++) {
            for (int k = 0; k < flaresnum; k++) {
                t->x = (flaredist * k) - (flaredist * (flaresnum - 1) * 0.5);
                t->y = (flaredist * j) - (flaredist * (flaresnum - 1) * 0.5);
                t->z = (flaredist * i) - (flaredist * (flaresnum - 1) * 0.5);
                t->w = 1;
                t++;
            }
        }
    }
    
    unsigned int l = 0;
    
    // init lines
    // 1st side
    for (int i = 0; i < flaresnum; i++) {
        for (int j = 0; j < flaresnum-1; j++) {
            v->a = (flaresnum*flaresnum*0) + (flaresnum * i) + j;
            v->b = (flaresnum*flaresnum*0) + (flaresnum * i) + j + 1;
            v->col = color;
            v++;
        }
    }
    for (int i = 0; i < flaresnum-1; i++) {
        for (int j = 0; j < flaresnum; j++) {
            v->a = (flaresnum*flaresnum*0) + (flaresnum * i) + j;
            v->b = (flaresnum*flaresnum*0) + (flaresnum * (i + 1)) + j;
            v->col = color;
            v++;
        }
    }
    
    // 2nd side
    for (int i = 0; i < flaresnum; i++) {
        for (int j = 0; j < flaresnum-1; j++) {
            v->a = (flaresnum*flaresnum*(flaresnum-1)) + (flaresnum * i) + j;
            v->b = (flaresnum*flaresnum*(flaresnum-1)) + (flaresnum * i) + j + 1;
            v->col = color;
            v++;
        }
    }
    for (int i = 0; i < flaresnum-1; i++) {
        for (int j = 0; j < flaresnum; j++) {
            v->a = (flaresnum*flaresnum*(flaresnum-1)) + (flaresnum * i) + j;
            v->b = (flaresnum*flaresnum*(flaresnum-1)) + (flaresnum * (i + 1)) + j;
            v->col = color;
            v++;
        }
    }
    
    // 3rd side
    for (int i = 0; i < flaresnum; i++) {
        for (int j = 0; j < flaresnum-1; j++) {
            v->a = (flaresnum*flaresnum*i) + (flaresnum * j);
            v->b = (flaresnum*flaresnum*i) + (flaresnum * (j+1));
            v->col = color;
            v++;
        }
    }
    
    for (int i = 0; i < flaresnum-1; i++) {
        for (int j = 0; j < flaresnum; j++) {
            v->a = (flaresnum*flaresnum*i) + (flaresnum * j);
            v->b = (flaresnum*flaresnum*(i+1)) + (flaresnum * j);
            v->col = color;
            v++;
        }
    }
    
    // 4th side
    for (int i = 0; i < flaresnum; i++) {
        for (int j = 0; j < flaresnum-1; j++) {
            v->a = (flaresnum*flaresnum*i) + (flaresnum * j) + (flaresnum-1);
            v->b = (flaresnum*flaresnum*i) + (flaresnum * (j+1) + (flaresnum-1));
            v->col = color;
            v++;
        }
    }
    
    for (int i = 0; i < flaresnum-1; i++) {
        for (int j = 0; j < flaresnum; j++) {
            v->a = (flaresnum*flaresnum*i) + (flaresnum * j) + (flaresnum-1);
            v->b = (flaresnum*flaresnum*(i+1)) + (flaresnum * j) + (flaresnum-1);
            v->col = color;
            v++;
        }
    }
    
    // 5th side
    for (int i = 0; i < flaresnum; i++) {
        for (int j = 0; j < flaresnum-1; j++) {
            v->a = (flaresnum*flaresnum*i) + j;
            v->b = (flaresnum*flaresnum*i) + j+1;
            v->col = color;
            v++;
        }
    }
    
    for (int i = 0; i < flaresnum-1; i++) {
        for (int j = 0; j < flaresnum; j++) {
            v->a = (flaresnum*flaresnum*i) + j;
            v->b = (flaresnum*flaresnum*(i+1)) + j;
            v->col = color;
            v++;
        }
    }
    
    // 6th side
    for (int i = 0; i < flaresnum; i++) {
        for (int j = 0; j < flaresnum-1; j++) {
            v->a = (flaresnum*flaresnum*i) + j + (flaresnum * (flaresnum-1));
            v->b = (flaresnum*flaresnum*i) + j+1 + (flaresnum * (flaresnum-1));
            v->col = color;
            v++;
        }
    }
    
    for (int i = 0; i < flaresnum-1; i++) {
        for (int j = 0; j < flaresnum; j++) {
            v->a = (flaresnum*flaresnum*i) + j + (flaresnum * (flaresnum-1));
            v->b = (flaresnum*flaresnum*(i+1)) + j + (flaresnum * (flaresnum-1));
            v->col = color;
            v++;
        }
    }
    
}

void wf::linedraw(vec2f *a, vec2f *b, line *f) {

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
 
int wf::lineclip(vec2f *a, vec2f *b) {
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

void wf::initblur() {
    unsigned short *p = blurtab;
    float n, nx, ny;
    //float scale = 0.8;
    //float scale = 0x20000;
    float scale = 0x10000;
    
    for (int y = 0; y < Y_RES; y++) {
        for (int x = 0; x < X_RES; x++) {
            //n = scale / (x*x + y*y);
            //n = sqrt(x*x + y*y) / 8 + 8;
            n = 6;
            nx = (x / n);
            ny = (y / n);
            *p++ = (unsigned short)ny * X_RES;
            *p++ = (unsigned short)nx;
            //*p++ = (unsigned short)(ny > Y_RES ? Y_RES : (int)ny * X_RES);
            //*p++ = (unsigned short)(nx > X_RES ? X_RES : nx);
        }
    }
}

void wf::blur(int sx, int sy) {
    unsigned short *p = buffer;
    unsigned short *v = buffer2;
    unsigned short *t = blurtab;
    
    const unsigned int BLENDMASK = 0x7BDE;
    int t_add, p_add, x_count, y_count;
    
    // upper left corner
    t = blurtab;
    p = &v[(sx-1) + ((sy-1) << 6) + ((sy-1) << 8)];
    /*
    for (int y = sy-1; y >= 0; y--) {
        for (int x = sx-1; x >= 0; x--) {
            int ty = *t, tx = *(t + 1);
            *p = (((*p & BLENDMASK) >> 1) + ((*(p + tx + ty) & BLENDMASK) >> 1));
            p--; t += 2;
        }
        t += 2*(X_RES - sx); p -= (X_RES - sx);
    }
    */
    t_add = 4*(X_RES - sx); p_add = -2*(X_RES - sx); x_count = sx; y_count = sy;
    _asm {
        mov     esi, t
        mov     edi, p
        mov     ecx, y_count
        
        _y_loop:
        push    ecx
        mov     ecx, x_count
        xor     eax, eax
        xor     ebx, ebx
        xor     edx, edx
        
        _x_loop:
        mov     ebx, [esi]              // 1
        add     esi, 4                  // .
        mov     dx, bx                  // 2
        mov     ax, [edi]               // .
        shr     ebx, 16                 // 3
        add     edi, -2                 // .
        add     edx, ebx                // 4
        mov     dx, [edi + 2*edx + 2]   // 5
        and     eax, 0x7BDE7BDE         // .
        shr     eax, 1                  // 6
        and     edx, 0x7BDE7BDE         // .
        shr     edx, 1                  // 7
        add     eax, edx                // 8
        dec     ecx                     // .
        mov     [edi + 2], ax           // 9
        jnz     _x_loop                 // .

        pop     ecx
        add     edi, p_add
        add     esi, t_add
        dec     ecx
        jnz     _y_loop
    } 
    
    // upper right corner
    t = blurtab;
    p = &v[sx + ((sy-1) << 6) + ((sy-1) << 8)];
    /*
    for (int y = sy-1; y >= 0; y--) {
        for (int x = sx; x < X_RES; x++) {
            int ty = *t, tx = *(t + 1);
            *p = (((*p & BLENDMASK) >> 1) + ((*(p - tx + ty) & BLENDMASK) >> 1));
            p++; t += 2;
        }
        t += 2*sx; p -= (2*X_RES - sx);
    }
    */
    t_add = 4*sx; p_add = -2*(2*X_RES - sx); x_count = X_RES - sx; y_count = sy;
    _asm {
        mov     esi, t
        mov     edi, p
        mov     ecx, y_count
        
        _y_loop:
        push    ecx
        mov     ecx, x_count
        xor     eax, eax
        xor     ebx, ebx
        xor     edx, edx
        
        _x_loop:
        mov     ax, [edi]               // 1
        mov     ebx, [esi]              // .
        mov     dx, bx                  // 2
        add     esi, 4                  // .
        shr     ebx, 16                 // 3 
        and     eax, 0x7BDE7BDE         // .
        shr     eax, 1                  // 4
        sub     edx, ebx                // .
        mov     dx, [edi + 2*edx]       // 5
        add     edi, 2                  // .
        and     edx, 0x00007BDE         // 6
        shr     edx, 1                  // 7
        add     eax, edx                // 8
        dec     ecx                     // .
        mov     [edi - 2], ax           // 9
        jnz     _x_loop                 // .
        
        pop     ecx
        add     edi, p_add
        add     esi, t_add
        dec     ecx
        jnz     _y_loop
    }    
    
    // lower left corner
    t = blurtab;
    p = &v[(sx-1) + (sy << 6) + (sy << 8)];
    /*
    for (int y = sy; y < Y_RES; y++) {
        for (int x = sx-1; x >= 0; x--) {
            int ty = *t, tx = *(t + 1);
            *p = (((*p & BLENDMASK) >> 1) + ((*(p + tx - ty) & BLENDMASK) >> 1));
            p--; t += 2;
        }
        t += 2*(X_RES - sx); p += (X_RES + sx);
    }
    */
    t_add = 4*(X_RES - sx); p_add = 2*(X_RES + sx); x_count = sx; y_count = Y_RES - sy;
    _asm {
        mov     esi, t
        mov     edi, p
        mov     ecx, y_count
        
        _y_loop:
        push    ecx
        mov     ecx, x_count
        xor     eax, eax
        xor     ebx, ebx
        xor     edx, edx
        
        _x_loop:
        mov     ax, [edi]               // 1
        mov     ebx, [esi]              // .        
        mov     dx, bx                  // 2
        add     esi, 4                  // .        
        shr     ebx, 16                 // 3 
        and     eax, 0x7BDE7BDE         // .        
        shr     eax, 1                  // 4
        sub     edx, ebx                // .       
        xor     edx, 0xFFFFFFFF         // 5
        add     edi, -2                 // .       
        mov     dx, [edi + 2*edx + 4]   // 6        
        and     edx, 0x00007BDE         // 7
        shr     edx, 1                  // 8        
        add     eax, edx                // 9
        dec     ecx                     // .        
        mov     [edi + 2], ax           // 10
        jnz     _x_loop                 // .
        
        pop     ecx
        add     edi, p_add
        add     esi, t_add
        dec     ecx
        jnz     _y_loop
    }    
    
    // lower right corner
    t = blurtab;
    p = &v[sx + (sy << 6) + (sy << 8)];
    /*
    for (int y = sy; y < Y_RES; y++) {
        for (int x = sx; x < X_RES; x++) {
            int ty = *t, tx = *(t + 1);
            *p = (((*p & BLENDMASK) >> 1) + ((*(p - tx - ty) & BLENDMASK) >> 1));
            p++; t += 2;
        }
        t += 2*sx; p += sx;
    }
    */
    t_add = 4*sx; p_add = 2*sx; x_count = X_RES - sx; y_count = Y_RES - sy;
    _asm {
        mov     esi, t
        mov     edi, p
        mov     ecx, y_count
        
        _y_loop:
        push    ecx
        mov     ecx, x_count
        xor     eax, eax
        xor     ebx, ebx
        xor     edx, edx
        
        _x_loop:
        mov     ax, [edi]               // 1
        mov     ebx, [esi]              // .        
        mov     dx, bx                  // 2
        add     esi, 4                  // .        
        shr     ebx, 16                 // 3 
        and     eax, 0x7BDE7BDE         // .       
        shr     eax, 1                  // 4
        add     edx, ebx                // .        
        xor     edx, 0xFFFFFFFF         // 5
        add     edi, 2                  // .        
        mov     dx, [edi + 2*edx]       // 6        
        and     edx, 0x00007BDE         // 7
        shr     edx, 1                  // 8        
        add     eax, edx                // 9
        dec     ecx                     // .        
        mov     [edi - 2], ax           // 10
        jnz     _x_loop                 // .
        
        pop     ecx
        add     edi, p_add
        add     esi, t_add
        dec     ecx
        jnz     _y_loop
    }
}

void wf::project3d(vec2f *f, vec4f *v) {
    float t;
    
    if (v->z < 0) {
        t = dist / (v->z + ee);
        f->x = (v->x * t) + (X_RES >> 1);
        f->y = (v->y * t) + (Y_RES >> 1);
    } else f->x = -1;
}

void wf::drawflare(vec2f *f, int type) {
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


void wf::blend() {
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

void wf::dirblur(int disp) {
    unsigned short *p   = &buffer2[0];
    unsigned short *v   = &buffer2[2*disp];
    unsigned int   len  = (X_RES*Y_RES) - 2*disp,
                   _edx = (-disp) & ~1;
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
        and     eax, 0x0000781E
        
        mov     bp, [edi + edx]
        and     ebx, 0x0000781E
        
        shr     eax, 1
        and     ebp, 0x000003E0
        
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

void wf::drawbuf(int i, int amp) {
    // fill random buffer lines
    for (int k = 0; k < 40; k++) {
        int c = (rand() % amp) * 0x04210421;
        int l = (rand() % Y_RES) * X_RES;
        unsigned short *buf = &buffer[l];
        _asm {
            cld
            mov    eax, c
            mov    ecx, X_RES / 2
            mov    edi, buf
            
            __loop:
            add    [edi], eax
            add    edi, 4
            dec    ecx
            jnz    __loop            
        }
    }
}

int wf::main(int timedemo) {
    int i, j;
    
    i = 0;
    while (timedemo ? (i < 1024) : !kbhit()) {
        if (timedemo) i++; else {i = *tick; ptc_wait();}
        
        ptc_update(buffer2);
        
        memset(buffer,  0, sizeof(unsigned short)*X_RES*Y_RES);
        drawbuf(i, 4);
        
        float scale = 0.5 * costabf[(i << 6) & 0xFFFF] + 0.8;
        int   rot1  = (i << 7),
              rot2  = (i << 6) + (i << 5),
              rot3  = 0,//(i << 3) - (i << 6) + (int)(0x2000 * sintabf[((i << 7)) & 0xFFFF]),
              movedist = -100 * costabf[(i << 7) & 0xFFFF] - 500;
        
        // draw flares
        for (j = 0; j < flarescount; j++) {
            pt[j] = p[j];
            float mul = 1 - (0.8f *(((1-cos((atan2(pt[j].x,pt[j].y) * 8) + ((float)i * 0.05f)))+
                               (1-cos((atan2(pt[j].x,pt[j].z) * 8) + ((float)i * 0.05f)))+
                               (1-cos((atan2(pt[j].y,pt[j].z) * 8) + ((float)i * 0.05f)))) / 18));
            
            vecmul(mul, mul, mul, &pt[j]);
            //pt[j] = pm[j];
            rotate3d(rot1 & 0xFFFF, rot2 & 0xFFFF, rot1 & 0xFFFF, &pt[j]);
            vecmove(0, 0, movedist, &pt[j]);
            project3d(&p2d[j], &pt[j]);
            drawflare(&p2d[j], ((j ^ 0x5A5A) % flaretypes)); 
        }
        
        // draw lines
        for (j = 0; j < linesnum; j++) {
            vec4f fa, fb, fd;
            vec2f ca, cb, cd;
            line cf;
            
            cf = f[j];
            fa = pt[cf.a]; fb = pt[cf.b];
            
            if ((fb.z >= 0) && (fa.z >= 0)) continue;
            if (fb.z > fa.z) {fd = fa; fa = fb; fb = fd;}
            
            if (fa.z >= 0){
                vec4f pa = fa;
                
                pa.y = fa.y + (fa.y + fb.y) * (-fb.z+dist) / (fa.z - fb.z);
                pa.x = fa.x + (fa.x + fb.x) * (-fb.z+dist) / (fa.z - fb.z);
                pa.z = -dist;
                
                fa = pa;
            }
            
            project3d(&ca, &fa);
            project3d(&cb, &fb);
            
            if (ca.y > cb.y) {cd = cb; cb = ca; ca = cd;} 
            
            if (!lineclip(&ca, &cb)) {linedraw(&ca, &cb, &cf); }
        }
        
        blend();
        blur(X_RES/2, Y_RES/2);
        //dirblur((int)(2 * costabf[(i << 11) & 0xFFFF] + 2));
    }
    if (!timedemo) getch();
    
    if (timedemo) printf("timed %d timerticks in %d frames - %f fps", *tick, i, (double)(60 * i) / *tick);
    
    return 0;
}
