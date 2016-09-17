#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <math.h>
#include <i86.h>

#include "..\..\flexptc\flexptc.h"
#include "..\..\ims06\xmplay.h"
#include "..\..\ims06\mcp.h"

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

// class definition
#include "cube.h"

cube::cube() {
    bgpic       = new unsigned short[X_RES*Y_RES];
    buffer      = new unsigned short[X_RES*Y_RES];
    d           = new dlist[Y_RES];
    
    FILE *f = fopen(".\\gfx\\cube.tga", "rb");
    if (f) {
        fseek(f, 18, SEEK_SET);
        fread(bgpic, sizeof(unsigned short), 65536, f);
        fclose(f);
    }
}

cube::~cube() {
    delete[] bgpic;
    delete[] buffer;
    delete[] d;
}

void cube::add(unsigned short *dst, unsigned short *src) {
    unsigned short *p = dst;
    unsigned short *v = src;
    // saturated rgb555 addition from hugi #18 by BRAD
    // a bit modified by wbcbz7 :)
    _asm {
        mov     esi, v
        mov     edi, p
        mov     ecx, (X_RES*Y_RES) / 2
        push    ebp
        
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
    }
}

void cube::addval(unsigned short *dst, unsigned short col) {
    unsigned short *p = dst;
    unsigned int val = col * 0x00010001;
    // saturated rgb555 addition from hugi #18 by BRAD
    // a bit modified by wbcbz7 :)
    _asm {
        mov     esi, val
        mov     edi, p
        mov     ecx, (X_RES*Y_RES) / 2
        push    ebp
        
        _inner:
        mov     eax, [edi]          // 1
        mov     ebx, esi            // .
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
        xor     edx, 0x3DEF3DEF     // 11 <- xor fixup
        or      eax, edx            // 12 (stall)
        dec     ecx                 // .
        mov     [edi - 4], eax      // 13
        jnz     _inner              // .
        
        pop     ebp                 // 1
    }
}

void cube::filldlist(dlist *d, int vel) {
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

void cube::dirblur(unsigned short *dst, unsigned int len, int disp, unsigned int mask1, unsigned int mask2) {
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

int cube::main(int noretrace) {
    int i, j;

    int flval = 31, glvel = 0, avel = 0;
    float alphaval = 31;
    int dt = 0;
    
    int rawpos, row, order;
    
    i = 0;
    
    rawpos = (xmpGetRealPos() & 0x00FFFFFF); row = (rawpos >> 8) & 0xFF; order = (rawpos >> 16) & 0xFF;
    while ((order < 0x16) && (!kbhit())) {
        dt = i; i = *tick; dt = i - dt; if (!noretrace) ptc_wait();
        
        ptc_update(buffer);
        if (mcpIdle) {mcpIdle();}
        
        rawpos = (xmpGetRealPos() & 0x00FFFFFF); row = (rawpos >> 8) & 0xFF; order = (rawpos >> 16) & 0xFF;
        
        memcpy(buffer, bgpic, sizeof(unsigned short)*X_RES*Y_RES);
        
        glvel = rand() & 4;
        
        // fill and draw dlist
        if (glvel) {
            unsigned short *pd = buffer;
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
        
        if (order == 0x14) {
            flval    -= 1*dt; if (flval    <= 0) flval = 0; flval = sat(flval, 31);
            addval(buffer, mk15(flval,flval,flval));
        }
        if ((order == 0x15) && (row > 32)) {
           avel = (int)alphaval; alphafade(buffer, 0, avel); alphaval -= 0.3f*dt; if (alphaval < 0) alphaval = 0;
        }
    }
    if (kbhit()) {getch(); return 1;}
    
    return 0;
}