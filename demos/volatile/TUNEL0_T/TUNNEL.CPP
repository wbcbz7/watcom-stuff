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

// extern sin\cos tables
extern signed int sintab[65536],  costab[65536];
extern float      sintabf[65536], costabf[65536];
#include "stuff.h"
#include "tgaload.h"

// class definition
#include "tunnel.h"

tunnel::tunnel() {
    table       = new unsigned short[X_RES*Y_RES];
    buffer      = new unsigned short[X_RES*Y_RES];
    
    // allocate memory for texture and align pointer to 128kb boundary
    texbuf      = new unsigned short[2*65536];
    texture     = (unsigned short*)(((unsigned int)texbuf + 0x1FFFF) & ~0x1FFFF);
    
    t_image *img;
    
    img_loadfile(img, "dutch.tga");
    memcpy(texture, img->ptr, img->pitch*img->height);
    //img_free(img);
    
    // call init stuff
    inittunnel();
}

tunnel::~tunnel() {
    delete[] table;
    delete[] buffer;
    delete[] texbuf;
}


void tunnel::inittexture() {
    int x, y, i, k=0;
    
    
    for (y = 0; y < 256; y++) {
        for (x = 0; x < 256; x++) {
            //texture[((y << 8) + x)] = sat((x ^ y), 255) & 0xFF;
            texture[((y << 8) + x)] = (((x >> 3) & 0x1F) << 0) | (((y >> 3) & 0x1F) << 5) | ((((x ^ y) >> 3) & 0x1F) << 10);
            //texture[((y << 8) + x)] = (x ^ y) | (rand() % 0x100) & 0xFF;
        }
    }
    
}

void tunnel::inittunnel() {
    long x, y, i, u, v, lm;
    double r, a, l;
    double size = 3072;
    
    i = 0;
    for (y = -100; y < 100; y++) 
        for (x = -160; x < 160; x++) {
            r = sqrt(x*x + y*y);
            if (r < 1) r = 1;
            a = atan2(y, x) + pi;
            u = (a * 256 / pi);
            v = (size / r);
            table[i++] = (u&0xFF) + ((v&0xFF)<<8);
        }
}

void tunnel::draw(int t) {
    int u1 = (t << 0) + (sintab[((t << 8)) & 0xFFFF] >> 8) + (sintab[((t << 7)) & 0xFFFF] >> 9);
    int v1 = (t << 1) + (sintab[((t << 7)) & 0xFFFF] >> 10);
    
    int texofs1 = ((v1 << 8) + u1) &0xFFFF;
    unsigned short *p = buffer;
    unsigned short *v = (unsigned short *)((unsigned int)texture >> 1);
    unsigned short *r = table;
    
    //for (int k = 0; k < 64000; k++) *p++ = texture[(table[k]+texofs1) & 0xFFFF];
        
    _asm {
        mov     ecx, 64000
        mov     esi, v
        mov     edi, p
        mov     ebx, r
        mov     edx, texofs1
        
        _loop:
        mov     si, [ebx]       // 1
        add     ebx, 2          // .
        
        add     si, dx          // 2
        add     edi, 2          // .
        
        mov     ax, [2 * esi]   // 3
        dec     ecx             // .
        
        mov     [edi - 2], ax   // 4
        jnz     _loop           // .
    }
    
}

int tunnel::main(int timedemo) {
    int i, j, p = 0;
    
    i = 0;
    while (timedemo ? (i < 1024) : !kbhit()) {
        if (timedemo) i++; else {i = *tick; ptc_wait();}
        ptc_update(buffer);
        draw(i);
    }
    if (!timedemo) getch();
    
    if (timedemo) printf("timed %d timerticks in %d frames - %f fps", *tick, i, (double)(60 * i) / *tick);
    
    return 0;
}
