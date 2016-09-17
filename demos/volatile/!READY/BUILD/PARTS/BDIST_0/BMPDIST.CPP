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

#define sat(a, l) (a < l ? a : l)
#define sqr(a)    (a * a)
#define ee         1.0E-6
#define bb        (1.2f * 1.0E+2)
#define pi         3.141592653589793
#define mk15(r, g, b) ((r << 10) | (g << 5) | (b << 0))

// extern sin\cos tables
extern signed int sintab[65536],  costab[65536];
extern float      sintabf[65536], costabf[65536];

// class definition
#include "bmpdist.h"

bmpdist_0::bmpdist_0() {
    fdu         = new unsigned int[41*26];
    fdv         = new unsigned int[41*26];
    buffer      = new unsigned short[X_RES*Y_RES];
    
    // allocate memory for texture and align pointer to 128kb boundary
    texbuf      = new unsigned short[2*65536];
    texture     = (unsigned short*)(((unsigned int)texbuf + 0x1FFFF) & ~0x1FFFF);
    
    FILE *f = fopen("gfx\\bdist_0.tga", "rb");
    if (f) {
        fseek(f, 18, SEEK_SET);
        fread(texture, sizeof(unsigned short), 65536, f);
        fclose(f);
    }
    
    // call init stuff
    //inittexture();
}

bmpdist_0::~bmpdist_0() {
    delete[] fdu;
    delete[] fdv;
    delete[] buffer;
    delete[] texbuf;
}

void bmpdist_0::interpolate() {
    typedef struct {unsigned int sdy, edy, dx, sy, ey, sx;} _fd;
    
    int k, gridptr = 0;
    unsigned short *p = buffer;
    unsigned short *t = texture;
    _fd u, v;
    
    int xu, xv, xdu, xdv;
    
    for (int j = 0; j < 25; j++) {
        for (int i = 0; i < 40; i++) {
            
            u.sdy = (fdu[gridptr+41] - fdu[gridptr]) >> 3;
            u.sy  = (fdu[gridptr]);
            
            u.edy = (fdu[gridptr+42] - fdu[gridptr+1]) >> 3;
            u.ey  = (fdu[gridptr+1]);
            
            v.sdy = (fdv[gridptr+41] - fdv[gridptr]) >> 3;
            v.sy  = (fdv[gridptr]);
        
            v.edy = (fdv[gridptr+42] - fdv[gridptr+1]) >> 3;
            v.ey  = (fdv[gridptr+1]);
            
            for (int y = 0; y < 8; y++) {
                
                u.dx = (u.ey - u.sy) >> 3;
                u.sx = u.sy;
                
                v.dx = (v.ey - v.sy) >> 3;
                v.sx = v.sy;
                
                xu = u.sy;
                xv = v.sy;
                
                xdu = u.dx;
                xdv = v.dx;
                
                
                _asm {
                    xor     eax, eax
                    mov     ebx, xu
                    mov     edx, xv
                    mov     ecx, 8
                    mov     esi, t
                    mov     edi, p
                    
                    _loop:
                    mov     ah, bh              // 1
                    add     edi, 2              // .
                    
                    mov     al, dh              // 2
                    add     ebx, xdu            // .
                    
                    mov     ax, [esi + 2*eax]   // 3
                    add     edx, xdv            // .
                                        
                    mov     [edi - 2], ax       // 4
                    dec     ecx                 // .
                    
                    jnz     _loop               // 5
                }
                
                
                p += X_RES;
                u.sy += u.sdy; u.ey += u.edy; v.sy += v.sdy; v.ey += v.edy;
            }
            gridptr++;
            p -= (X_RES * 8) - 8;
        }
        gridptr++;
        p += (X_RES * 7);
    } 
}

void bmpdist_0::inittexture() {
    int x, y, i, k=0;
    
    
    for (y = 0; y < 256; y++) {
        for (x = 0; x < 256; x++) {
            //texture[((y << 8) + x)] = sat((x ^ y), 255) & 0xFF;
            texture[((y << 8) + x)] = (((x >> 3) & 0x1F) << 0) | (((y >> 3) & 0x1F) << 5) | ((((x ^ y) >> 3) & 0x1F) << 10);
            //texture[((y << 8) + x)] = (x ^ y) | (rand() % 0x100) & 0xFF;
        }
    }
    
}

void bmpdist_0::drawgrid(int ax, int ay, int t, float mul) {

    int x, y, z, tptr = 0;
    unsigned int u,  v;
    float r, a;
    float fu, fv;
    
    for (y = -(Y_RES/2 + 4); y < (Y_RES/2 + 4); y += 8) {
        for (x = -(X_RES/2 + 4); x < (X_RES/2 + 4); x += 8) {
            
            /*
            //r = sqrt(x*x + y*y) / 64;
            r = ((sqrt((x*x + y*y)) / (x*x + y*y)) * (128 + 64 * sintabf[(t << 8) & 0xFFFF]));
            
            fu = (x / r) + 64*sintabf[(t << 7) & 0xFFFF];
            fv = (y / r) + 64*costabf[(t << 7) & 0xFFFF];
            */
            
            r = (2 * costabf[(t << 7) & 0xFFFF]) * sqrt(x*x + y*y);
            a = (1 * costabf[(t << 6) & 0xFFFF]) + 1;         
            
            fu = (x * a) + (mul*sintabf[(t << 6) & 0xFFFF] + 32) * sintabf[(((int)r << 8) + (t << 9)) & 0xFFFF] +
                           (mul*sintabf[(t << 5) & 0xFFFF]) * sintabf[(t << 8) & 0xFFFF] +
                           (mul*costabf[((y << 7) + (y << 6) + (t << 8)) & 0xFFFF]);
            fv = (y * a) + (mul*costabf[(t << 6) & 0xFFFF] + 32) * costabf[(((int)r << 8) + (t << 9)) & 0xFFFF] +
                           (mul*sintabf[(t << 5) & 0xFFFF]) * costabf[(t << 7) & 0xFFFF] +
                           (mul*costabf[((x << 7) + (t << 9) + (t << 8)) & 0xFFFF]);
            
            u = (unsigned int)(fu * 256);
            v = (unsigned int)(fv * 256);
            
            fdu[tptr] = u & 0xFFFFFFFF;
            fdv[tptr] = v & 0xFFFFFFFF;
            
            tptr++;
        }
    }
}

void bmpdist_0::addval(unsigned short *dst, unsigned short col) {
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

int bmpdist_0::main(int noretrace) {
    int i = 0, di = 0, j;
    
    int rawpos, row, order;
    int glvel = 0, alphaval = 0, alev = 0, mulval = 64;
    
    rawpos = (xmpGetRealPos() & 0x00FFFFFF); row = (rawpos >> 8) & 0xFF; order = (rawpos >> 16) & 0xFF;
    
    while ((order < 0xC) && (!kbhit())) {
        di = i; i = *tick; di = i - di; if (!noretrace) ptc_wait();
        
        ptc_update(buffer);
        if (mcpIdle) {mcpIdle();}
        
        rawpos = (xmpGetRealPos() & 0x00FFFFFF); row = (rawpos >> 8) & 0xFF; order = (rawpos >> 16) & 0xFF;
        
        if (row == 0) mulval = 128;
        
        drawgrid(0, 0, i, mulval);
        
        interpolate();
        
        if (mulval > 64) mulval -= 2*di; else
        if (mulval < 64) mulval += di;
        
        
        if (row == 0) alphaval = 28;
        if (((row & 31) == 0x8) || ((row & 31) == 0x14) || ((row & 31) == 0x1C)) alphaval = 16;
        if ((order & 0x9 == 0x9) && ((row & 31) == 0x2)) {alphaval = 16; mulval = 4;}
        if (alphaval >= 0) {alev = (int)alphaval; addval(buffer, mk15(alev, alev, alev)); alphaval -= di;}
        if (alphaval <= 0) alphaval = 0;
        
    }
    if (kbhit()) {getch(); return 1;}
    
    return 0;
}
