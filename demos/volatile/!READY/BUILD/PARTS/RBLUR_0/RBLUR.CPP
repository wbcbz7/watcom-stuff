#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <math.h>
#include <i86.h>

#include "..\..\flexptc\flexptc.h"
#include "..\..\ims06\xmplay.h"
#include "..\..\ims06\mcp.h"
#include "..\..\common\alpha.h"

#define X_RES 320
#define Y_RES 200

#define sat(a, l)      (a < l ? a : l)
#define sqr(a)         (a * a)
#define ee              1.0E-6
#define bb             (1.2f * 1.0E+2)
#define pi              3.141592653589793
#define mk15(r, g, b)  (((r >> 3) << 10) | ((g >> 3) << 5) | ((b >> 3) << 0))

// extern sin\cos tables
extern signed int sintab[65536],  costab[65536];
extern float      sintabf[65536], costabf[65536];

// class definition
#include "rblur.h"

rblur_0::rblur_0() {
    blurtab     = new unsigned short[X_RES*Y_RES*2];
    buffer      = new unsigned short[X_RES*Y_RES];
    buffer2     = new unsigned short[X_RES*Y_RES];
    flare       = new unsigned short[flaresize*flaresize*flaretypes*2];
    d           = new dlist[Y_RES];
    p           = new vec4f[flaresnum*flaresnum*flaresnum];
    pt          = new vec4f[flaresnum*flaresnum*flaresnum];
    p2d         = new vec2f[flaresnum*flaresnum*flaresnum];
    
    // call init stuff
    initblur();
    initflares();
    initvertex();
    
    memset(buffer,  0, sizeof(unsigned short)*X_RES*Y_RES);
    memset(buffer2, 0, sizeof(unsigned short)*X_RES*Y_RES);
}

rblur_0::~rblur_0() {
    delete[] blurtab;
    delete[] buffer;
    delete[] buffer2;
    delete[] flare;
    delete[] p;
    delete[] d;
    delete[] pt;
    delete[] p2d;
}

void rblur_0::initflares() {
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

void rblur_0::initvertex() {
    vec4f *f = p;
    
    for (int i = 0; i < flaresnum; i++) {
        for (int j = 0; j < flaresnum; j++) {
            for (int k = 0; k < flaresnum; k++) {
                f->x = (flaredist * k) - (flaredist * (flaresnum - 1) * 0.5);
                f->y = (flaredist * j) - (flaredist * (flaresnum - 1) * 0.5);
                f->z = (flaredist * i) - (flaredist * (flaresnum - 1) * 0.5);
                f->w = 1;
                f++;
            }
        }
    }
}

void rblur_0::initblur() {
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

void rblur_0::blur(int sx, int sy) {
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

void rblur_0::project3d(vec2f *f, vec4f *v) {
    float t;
    
    if (v->z < 0) {
        t = dist / (v->z + ee);
        f->x = (v->x * t) + (X_RES >> 1);
        f->y = (v->y * t) + (Y_RES >> 1);
    } else f->x = -1;
}

void rblur_0::drawflare(vec2f *f, int type) {
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

void rblur_0::blend() {
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

void rblur_0::filldlist(dlist *d, int vel) {
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

void rblur_0::dirblur(unsigned short *dst, unsigned int len, int disp, unsigned int mask1, unsigned int mask2) {
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

void rblur_0::drawbuf(int i, int amp) {
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

int rblur_0::main(int noretrace) {
    int i = 0, di = 0, j;
    
    int rawpos, row, order;
    int glvel = 0, alphaval = 0;
    
    rawpos = (xmpGetRealPos() & 0x00FFFFFF); row = (rawpos >> 8) & 0xFF; order = (rawpos >> 16) & 0xFF;
    
    while ((rawpos < 0x032000) && (!kbhit())) {
        di = i; i = *tick; di = i - di; if (!noretrace) ptc_wait();
        ptc_update(buffer2);
        if (mcpIdle) {mcpIdle();}
        
        rawpos = (xmpGetRealPos() & 0x00FFFFFF); row = (rawpos >> 8) & 0xFF; order = (rawpos >> 16) & 0xFF;
        
        memset(buffer,  0, sizeof(unsigned short)*X_RES*Y_RES);
        drawbuf(i, 4);
        
        float scale = 0.5 * costabf[(i << 6) & 0xFFFF] + 0.8;
        int   rot1  = (i << 6) + (int)(0x1000 * costabf[((i << 6) + (i << 5)) & 0xFFFF]),
              rot2  = (i << 6) + (i << 5) + (int)(0x2000 * sintabf[((i << 6)) & 0xFFFF]),
              rot3  = (i << 7) - (i << 6) + (int)(0x1000 * sintabf[((i << 6)) & 0xFFFF]),
              movedist = -150 * costabf[(i << 7) & 0xFFFF] - 200;
        
        
        for (j = 0; j < flarescount; j++) {
            pt[j] = p[j];
            rotate3d(rot1 & 0xFFFF, rot2 & 0xFFFF, rot1 & 0xFFFF, &pt[j]);
            vecmove(0, 0, movedist, &pt[j]);
            project3d(&p2d[j], &pt[j]);
            drawflare(&p2d[j], ((j ^ 0x5A5A) % flaretypes)); 
        }
        blend();
        blur(X_RES/2, Y_RES/2);
        
        if ((((rawpos & 0x00FFFF) >= 0x003000) && ((rawpos & 0x00FFFF) <= 0x003F00)) ||
            (((rawpos & 0x02FFFF) >= 0x021000) && ((rawpos & 0x02FFFF) <= 0x021F00))) glvel = 8;
        else if ((order == 1) & (((row & 31) == 0xB) || ((row & 31) == 0x1B) || ((row & 31) == 0x1E))) glvel = 4; else glvel = 0;
        
        // fill and draw dlist
        if (glvel) {
            unsigned short *pd = buffer2;
            dlist *dd = d;
            filldlist(d, glvel);
            dlist de;
            memcpy(&de, dd, sizeof(dlist));
            for (int k = 0; k < Y_RES; k++) {
                if (dd->active) {memcpy(&de, dd, sizeof(dlist));}
                dirblur(pd, X_RES, de.disp, de.mask1, de.mask2);
                dd++; pd += X_RES;
            }
        }
        
        if ((order == 0) && (order < 0x20)) alphaval = (i >> 3);
        
        if ((alphaval > 15) && ((row & (order >= 0x2 ? 31 : 63)) == 0)) alphaval = 23;
        if (alphaval < 31) {alphafade(buffer2, 0, alphaval); alphaval += di; alphaval = sat(alphaval, 31);}
    }
    if (kbhit()) {getch(); return 1;}
    
    return 0;
}
