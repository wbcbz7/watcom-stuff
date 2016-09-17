#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
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

// extern sin\cos tables
extern signed int sintab[65536],  costab[65536];
extern float      sintabf[65536], costabf[65536];
#include "matrix.h"

// class definition
#include "alpha.h"

alpha::alpha(int argc, char *argv[]) {
    FILE *f;
    
    pic1        = new unsigned short[X_RES*Y_RES];
    pic2        = new unsigned short[X_RES*Y_RES];
    buffer      = new unsigned short[X_RES*Y_RES];
    buffer2     = new unsigned short[X_RES*Y_RES];
    flare       = new unsigned short[flaresize*flaresize*flaretypes*2];
    p           = new vec4f[flaresnum];
    pt          = new vec4f[flaresnum];
    p2d         = new vec2f[flaresnum];
    d           = new dlist[Y_RES];
    
    // allocate memory for alphatab and align pointer to 128kb boundary
    alignbuf    = new unsigned short[65536 + 33792];
    alphatab    = (unsigned short*)((((unsigned int)alignbuf)  + 0x1FFFF) & ~0x1FFFF);
    
    // call init stuff
    initflares();
    initvertex();
    initalphatab();
    
    f = fopen(argv[1], "rb"); if (f) {fseek(f, 18, SEEK_SET); fread(pic1, sizeof(unsigned short), 65536, f); fclose(f);} else fillpic(pic1);
    f = fopen(argv[2], "rb"); if (f) {fseek(f, 18, SEEK_SET); fread(pic2, sizeof(unsigned short), 65536, f); fclose(f);} else fillpic(pic2);
    
    memset(buffer,  0, sizeof(unsigned short)*X_RES*Y_RES);
    memset(buffer2, 0, sizeof(unsigned short)*X_RES*Y_RES);
}

alpha::~alpha() {
    delete[] alignbuf;
    delete[] buffer;
    delete[] buffer2;
    delete[] pic1;
    delete[] pic2;
    delete[] flare;
    delete[] p;
    delete[] pt;
    delete[] p2d;
    delete[] d;
}

void alpha::fillpic(unsigned short *f) {
    unsigned short *p = f;
    
    float m[12];
    
    srand(inp(0x40));
    for (int i = 0; i < 12; i++) {
        m[i] = rand() % 384;
    }
    
    for (int y = 0; y < Y_RES; y++) {
        for (int x = 0; x < X_RES; x++) {
            // rgb888
            
            int b = 63 * (sintabf[(int)((x * m[0]) + (y * m[1])) & 0xFFFF] + costabf[(int)((x * m[6]) + (y * m[7])) & 0xFFFF]) + 128;
            int g = 63 * (sintabf[(int)((x * m[2]) + (y * m[3])) & 0xFFFF] + costabf[(int)((x * m[8]) + (y * m[9])) & 0xFFFF]) + 128;
            int r = 63 * (sintabf[(int)((x * m[4]) + (y * m[5])) & 0xFFFF] + costabf[(int)((x * m[10]) + (y * m[11])) & 0xFFFF]) + 128;
            
            *p++ = ((r >> 3) << 10) | ((g >> 3) << 5) | ((b >> 3) << 0);
        }
    }
}

void alpha::initalphatab() {
    unsigned short *p = alphatab;
    
    // R*G*A 15bpp table
    for (int r = 0; r < 32; r++) {
        for (int g = 0; g < 32; g++) {
            for (int a = 0; a < 32; a++) {
                int ar = (r * a) >> 5;
                int ag = (g * a) >> 5;
                
                *p++ = (ar << 10) | (ag << 5);
            }
        }
    }
    
    // A*B 15bpp table
    for (int a = 0; a < 32; a++) {
        for (int b = 0; b < 32; b++) {
            *p++ = (b * a) >> 5;
        }
    }
}

void alpha::initflares() {
    unsigned short *p = flare;
    
    for (int i = 0; i < (flaretypes*2); i++) {
        for (int y = -(flaresize/2); y < (flaresize/2); y++) {
            for (int x = -(flaresize/2) - (i&1); x < (flaresize/2) - (i&1); x++) {
                // rgb888
                
                int b = sat((int)sat(flarescale / (x*x + y*y + ee), 0xFF) * (0x20 + (0x70 * (i >> 1))), 0xFFFF) >> 8;
                int g = sat((int)sat(flarescale / (x*x + y*y + ee), 0xFF) * (0xC0 - (0x20 * (i >> 1))), 0xFFFF) >> 8;
                int r = sat((int)sat(flarescale / (x*x + y*y + ee), 0xFF) * (0xA0 - (0x30 * (i >> 1))), 0xFFFF) >> 8;
                
                *p++ = ((r >> 3) << 10) | ((g >> 3) << 5) | ((b >> 3) << 0);
            }
        }
    }
}

void alpha::initvertex() {
    vec4f *f = p;
    
    for (int i = 0; i < flaresnum; i++) {
        float rx = (rand() % flaredist) - (flaredist/2), ry = (rand() % flaredist) - (flaredist/2);
        f->x = rx;
        f->y = ry;
        f->z = 0; // ignored for parallel projection
        f->w = 1;
        f++;
    }
}

void alpha::parallelproject(vec2f *v, vec4f *f) {
    v->x = f->x + (X_RES/2); v->y = f->y + (Y_RES/2);
}

void alpha::drawflare(vec2f *f, int type) {
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

void alpha::blend(unsigned short *src, unsigned short *dst) {
    unsigned short *p = src;
    unsigned short *v = dst;
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

void alpha::ablend(unsigned short *src, unsigned short *dst, unsigned int a) {
    unsigned short *p = src;
    unsigned short *v = dst;
    unsigned short *t = (unsigned short*)((unsigned int)alphatab >> 1);
    
    unsigned int alpha = sat(a, 31);
    static unsigned int sa1, sa2, da1, da2;
    
    /*
    sa1   = alpha * 0x00010001;        sa2 = ((alpha << 5) | 0x8000) * 0x00010001;
    da1   = (31 - alpha) * 0x00010001; da2 = (((31 - alpha) << 5) | 0x8000) * 0x00010001;
    */
    
    sa1   = alpha;        sa2 = ((alpha << 5) | 0x8000);
    da1   = (31 - alpha); da2 = (((31 - alpha) << 5) | 0x8000);
    
    // res = src*alpha + dst*(31-alpha)
    
    _asm {
        mov     esi, p          // 1
        mov     edi, v          // .
        mov     ecx, t          // 2
        push    ebp             // .
        mov     ebp, 64000      // 3
        
        _loop:
        mov     ax, [esi]       // 1
        add     esi, 2          // .
        mov     bx, [edi]       // 2
        mov     edx, eax        // .
        and     eax, 0x00007FE0 // 3
        and     edx, 0x0000001F // .
        or      eax, [sa1]      // 4
        or      edx, [sa2]      // .
        mov     cx, ax          // 5
        add     edi, 2          // .
        mov     ax, [2*ecx]     // 6
        mov     cx, dx          // .
        mov     edx, ebx        // 7
        and     ebx, 0x00007FE0 // .
        or      ax, [2*ecx]     // 8
        or      ebx, [da1]      // .
        mov     cx,  bx         // 9
        and     edx, 0x0000001F // .
        mov     bx, [2*ecx]     // 10
        or      edx, [da2]      // .
        mov     cx, dx          // 11
        add     eax, ebx        // .
        mov     dx, [2*ecx]     // 12
        add     eax, edx        // 13 (stall)
        dec     ebp             // .
        mov     [edi-2], ax     // 14
        jnz     _loop           // .
        pop     ebp
    }
    
}

void alpha::dirblur(unsigned short *dst, unsigned int len, int disp, unsigned int mask1, unsigned int mask2) {
    unsigned short *p = dst;
    unsigned short *v = &dst[2*disp];
    unsigned int   l  = len - 2*disp,
                   _edx = -2*disp;               
    static unsigned int m1, m2;
    
    m1 = mask1; m2 = mask2;
    
    _asm {
        mov     esi, p
        mov     ecx, l
        mov     edi, v
        mov     edx, _edx
        push    ebp
        
        _loop:
        mov     ax, [esi]
        add     esi, 2
        
        mov     bx, [edi]
        and     eax, [m2]
        
        mov     bp, [edi + edx]
        and     ebx, [m2]
        
        shr     eax, 1
        and     ebp, [m1]
        
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

void alpha::filldlist(dlist *d) {
    dlist *p = d;
    
    for (int i = 0; i < Y_RES; i++) {
        p->active = rand() & 1;
        p->disp   = (rand() & 3);
        p->mask1  = 0x7FFF; p->mask2  = 0;
        
        int flags = rand() % 7 + 1;
        if (flags & 1) {p->mask2 ^= 0x7800; p->mask1 ^= 0x7C00;} flags >>= 1;
        if (flags & 1) {p->mask2 ^= 0x3C0; p->mask1 ^= 0x3E0;} flags >>= 1;
        if (flags & 1) {p->mask2 ^= 0x1E; p->mask1 ^= 0x1F;} flags >>= 1;
        
        p++;
    }
}

int alpha::main(FILE *f) {
    int i, j;
    
    // tweakable vars
    char  ch = 0; // keyboard char
    int   f_visible = 0, f_num = flaresnum; // flare flags
    int   b_alphaen = 0, b_alpha = 0;
    int   bl_en = 0, bl_mask1 = 0x7FFF, bl_mask2 = 0, bl_disp = 0, bl_dlist = 0;
    float rot_velocity = 64, rot_swing = 4096; // rotation velocity
    float scale_swing = 0.5, scale_shift = 0.8, scale_velocity = 64;
    
    
    i = 0;
    do {
        i = *tick;
        ptc_wait();
        ptc_update(buffer);
        
        // process keyboard
        ch = (kbhit() ? toupper(getch()) : 0);
        
        switch(ch) {
            case '-' : f_num = (f_num <= 0 ? 0 : f_num - 1); break;
            case '+' : f_num = (f_num >= flaresnum ? flaresnum : f_num + 1); break;
            
            case 'Q' : {unsigned short *tmp = pic1; pic1 = pic2; pic2 = tmp;} break;
            
            case 'W' : bl_en ^= -1; break;
            case 'E' : bl_dlist ^= -1; break;
            case 'Z' : bl_disp = (bl_disp <= 0 ? 0 : bl_disp - 1); break;
            case 'X' : bl_disp = (bl_disp >= 16 ? 16 : bl_disp + 1); break;
            case 'R' : bl_mask2 ^= 0x7800; bl_mask1 ^= 0x7C00; break;
            case 'G' : bl_mask2 ^= 0x3C0; bl_mask1 ^= 0x3E0; break;
            case 'B' : bl_mask2 ^= 0x1E; bl_mask1 ^= 0x1F; break;
            
            case 'A' : f_visible ^= -1; break;
            case 'S' : b_alphaen ^= -1; break;
            
            case 'O' : b_alpha = (b_alpha <= 0 ? 0 : b_alpha - 1); break;
            case 'P' : b_alpha = (b_alpha >= 31 ? 31 : b_alpha + 1); break;
            
            case '[' :
            case '{' : rot_velocity -= 2; break;
            case ']' :
            case '}' : rot_velocity += 2; break;
            
            case ',' :
            case '<' : scale_swing -= 0.05; break;
            case '.' :
            case '>' : scale_swing += 0.05; break;
            
            case 'K' : scale_velocity -= 2; break;
            case 'L' : scale_velocity += 2; break;
            
            default: break;
        }
        
        memcpy(buffer, pic1, sizeof(unsigned short)*X_RES*Y_RES);
        ///*
        // processing stuff
        if (b_alphaen) ablend(pic2, buffer, b_alpha);
        if (bl_en) if (bl_dlist) {
            unsigned short *pd = buffer;
            dlist *dd = d;
            filldlist(d);
            dlist de;
            for (int k = 0; k < Y_RES; k++) {
                if (dd->active) memcpy(&de, dd, sizeof(dlist));
                dirblur(pd, X_RES, de.disp, de.mask1, de.mask2);
                dd++; pd += X_RES;
            }
        } else dirblur(buffer, X_RES*Y_RES, bl_disp, bl_mask1, bl_mask2);
        // -------
        //*/
        float scale = scale_swing * costabf[(int)(i * scale_velocity) & 0xFFFF] + scale_shift;
        int   rot   = (i * rot_velocity) + (int)(rot_swing * costabf[((i << 6) + (i << 5)) & 0xFFFF]);
        
        
        if (f_visible) for (j = 0; j < f_num; j++) {
            pt[j] = p[j];
            rotate3d(0, 0, rot & 0xFFFF, &pt[j]);

            vecmul(scale, scale, scale, &pt[j]);
            parallelproject(&p2d[j], &pt[j]);
            drawflare(&p2d[j], j % flaretypes); 
        }
        /*
        // processing stuff
        if (b_alphaen) ablend(pic2, buffer, b_alpha);
        if (bl_en) if (bl_dlist) {
            unsigned short *pd = buffer;
            dlist *dd = d;
            filldlist(d);
            dlist de;
            for (int k = 0; k < Y_RES; k++) {
                if (dd->active) memcpy(&de, dd, sizeof(dlist));
                dirblur(pd, X_RES, de.disp, de.mask1, de.mask2);
                dd++; pd += X_RES;
            }
        } else dirblur(buffer, X_RES*Y_RES, bl_disp, bl_mask1, bl_mask2);
        // -------
        */
    } while (ch != 0x1B);
    
    return 0;
}
