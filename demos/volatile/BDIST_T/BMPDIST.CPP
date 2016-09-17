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
#include "stuff.h"

// class definition
#include "bmpdist.h"

bmpdist::bmpdist() {
    fdu         = new unsigned int[41*26];
    fdv         = new unsigned int[41*26];
    buffer      = new unsigned short[X_RES*Y_RES];
    
    // allocate memory for texture and align pointer to 128kb boundary
    texbuf      = new unsigned short[2*65536];
    texture     = (unsigned short*)(((unsigned int)texbuf + 0x1FFFF) & ~0x1FFFF);
    
    FILE *f = fopen("dutch.tga", "rb");
    if (f) {
        fseek(f, 18, SEEK_SET);
        fread(texture, sizeof(unsigned short), 65536, f);
        fclose(f);
    }
    
    // call init stuff
    //inittexture();
}

bmpdist::~bmpdist() {
    delete[] fdu;
    delete[] fdv;
    delete[] buffer;
    delete[] texbuf;
}

void bmpdist::interpolate() {
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

void bmpdist::inittexture() {
    int x, y, i, k=0;
    
    
    for (y = 0; y < 256; y++) {
        for (x = 0; x < 256; x++) {
            //texture[((y << 8) + x)] = sat((x ^ y), 255) & 0xFF;
            texture[((y << 8) + x)] = (((x >> 3) & 0x1F) << 0) | (((y >> 3) & 0x1F) << 5) | ((((x ^ y) >> 3) & 0x1F) << 10);
            //texture[((y << 8) + x)] = (x ^ y) | (rand() % 0x100) & 0xFF;
        }
    }
    
}

void bmpdist::drawgrid(int ax, int ay, int t) {

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
            
            fu = (x * a) + (64*sintabf[(t << 6) & 0xFFFF] + 32) * sintabf[(((int)r << 8) + (t << 8)) & 0xFFFF] +
                           (64*sintabf[(t << 5) & 0xFFFF]) * sintabf[(t << 7) & 0xFFFF] +
                           (64*costabf[((y << 7) + (y << 6) + (t << 7)) & 0xFFFF]);
            fv = (y * a) + (64*costabf[(t << 6) & 0xFFFF] + 32) * costabf[(((int)r << 8) + (t << 8)) & 0xFFFF] +
                           (64*sintabf[(t << 5) & 0xFFFF]) * costabf[(t << 6) & 0xFFFF] +
                           (64*costabf[((x << 7) + (t << 8) + (t << 7)) & 0xFFFF]);
            
            u = (unsigned int)(fu * 256);
            v = (unsigned int)(fv * 256);
            
            fdu[tptr] = u & 0xFFFFFFFF;
            fdv[tptr] = v & 0xFFFFFFFF;
            
            tptr++;
        }
    }
}

int bmpdist::main(int timedemo) {
    int i, j, p = 0;
    
    i = 0;
    while (timedemo ? (i < 1024) : !kbhit()) {
        if (timedemo) i++; else {i = *tick; ptc_wait();}
        
        //outp(0x3C8, 0); outp(0x3C9, 63); outp(0x3C9, 0); outp(0x3C9, 0);
        
        ptc_update(buffer);
        
        drawgrid(0, 0, i);
        
        //outp(0x3C8, 0); outp(0x3C9, 63); outp(0x3C9, 63); outp(0x3C9, 0);
        
        interpolate();
        
        //outp(0x3C8, 0); outp(0x3C9, 0); outp(0x3C9, 0); outp(0x3C9, 0); 
    }
    if (!timedemo) getch();
    
    if (timedemo) printf("timed %d timerticks in %d frames - %f fps", *tick, i, (double)(60 * i) / *tick);
    
    return 0;
}
