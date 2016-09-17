#include <stdlib.h>
#include <conio.h>
#include <math.h>
#include "rtctimer.h"

unsigned char  *screen = (unsigned char*)0xA0000;

#define LIGHT_SIZE 512
#define FLARE_SIZE 192
#define FLARE_NUM  2

unsigned char  lightmap[LIGHT_SIZE*LIGHT_SIZE];
unsigned char  flarespr[FLARE_SIZE*FLARE_SIZE];
unsigned char  buffer[64000], buffer2[64000];
unsigned char  envmap[64000];

         float sintabf[65536], costabf[65536];

#define sat(a, l)  ((a > l) ? l : a)
#define pi         3.141592653589793
#define ee         1.0E-6

void initsintab() {
    int i, j;
    double r, lut_mul;
    lut_mul = (2 * pi / 65536);
    for (i = 0; i < 65536; i++) {
        r = i * lut_mul;
        //sintab [i] = 32767 * sin(r);
        //costab [i] = 32767 * cos(r);
        sintabf[i] = sin(r);
        costabf[i] = cos(r);
    }
}

void initpal() {
    int i;
    
    outp(0x3C8, 0);
    for (i = 0; i < 256; i++) {
        /*
        outp(0x3C9, sat(8  + (i >> 2) + (i >> 3), 63));
        outp(0x3C9, sat(10 + (i >> 2) + (i >> 3), 63));
        outp(0x3C9, sat(12 + (i >> 2) + (i >> 3) + (i >> 4), 63));
        */
        
        outp(0x3C9, sat((i >> 2), 63));
        outp(0x3C9, sat((i >> 2), 63));
        outp(0x3C9, sat((i >> 2), 63));
        
    }
}

/*
void initlight() {
    unsigned char *p = (unsigned char*)&lightmap;
    int x, y;
    float nx, ny, nz;
    
    float size = 128.0f, gamma = 0.8f;
    
    for (y = -LIGHT_SIZE/2; y < LIGHT_SIZE/2; y++) {
        for (x = -LIGHT_SIZE/2; x < LIGHT_SIZE/2; x++) {
            nx = ((float)x / size); ny = ((float)y / size); 
            nz = 1 - sqrt(nx*nx + ny*ny);
            nz = (nz < 0 ? 0 : sat(pow(nz, gamma) * 255, 255));
            *p++ = (unsigned char)nz;
        }
    }
}
*/

void initflare() {
    unsigned char *p = (unsigned char*)&flarespr;
    int x, y;
    
    for (y = -FLARE_SIZE/2; y < FLARE_SIZE/2; y++) {
        for (x = -FLARE_SIZE/2; x < FLARE_SIZE/2; x++) {
            *p++ = sat(0x10000/(x*x + y*y + ee), 255);
        }
    }
}

void initenv() {
    unsigned char *p = (unsigned char*)&envmap;
    int x, y, i, k;
    
    
    for (y = 0; y < 200; y++) {
        for (x = 0; x < 320; x++) *p++ = (x ^ y) & 0x7F;
    }
    
    
    // fill with random values
    //for (i = 0; i < 64000; i++) *p++ = (rand() & 0x1F);
    
    // blur it
    for (k = 0; k < 1; k++) {
        p = (unsigned char*)&envmap;
        for (i = 0; i < 320; i++) {
            *p = (*(p+1) + *(p-1) + *p + *(p+320)) >> 2;
            p++;
        }
        for (i = 320; i < 64000-320; i++) {
            *p = (*(p+1) + *(p-1) + *(p+320) + *(p-320)) >> 2;
            p++;
        }
        for (i = 64000-320; i < 64000; i++) {
            *p = (*(p+1) + *(p-1) + *p + *(p-320)) >> 2;
            p++;
        }
    }
}

void drawflare(int i) {
    unsigned char *p = (unsigned char*)&lightmap;
    unsigned char *v;
    int x, y, j;
    
    // clear old map
    _asm {
        mov    ecx, (512*512)/4
        mov    edi, p
        xor    eax, eax
        cld
        rep    stosd
    }
    
    for (j = 0; j < FLARE_NUM; j++) {
        x = (96 * sintabf[((i << 8) + (i << (3 + j)) + (0x800 * j)) & 0xFFFF]) + 256 - (FLARE_SIZE/2);
        y = (96 * sintabf[((i << 7) + (i << (4 + j)) + (0x1800 * j)) & 0xFFFF]) + 256 - (FLARE_SIZE/2);
        
        p = (unsigned char*)&lightmap[(y << 9) + x];
        v = (unsigned char*)&flarespr;
        _asm {
            mov    esi, v
            mov    edi, p
                
            mov    ecx, FLARE_SIZE
                
            @outer:
            push   ecx
            mov    ecx, FLARE_SIZE
                
            @inner:
            mov    al, [esi]
            mov    bl, [edi]
                
            add    al, bl
            sbb    dl, dl
            inc    esi
            or     al, dl
            inc    edi
            mov    [edi-1], al
                
            dec    ecx
            jnz    @inner
                
            pop    ecx
            add    edi, (512 - FLARE_SIZE)
                
            dec    ecx
            jnz    @outer
        }
    }
}

void drawbump(int lx, int ly) {
    unsigned char *p = (unsigned char*)&envmap[320];
    unsigned char *s = (unsigned char*)&buffer[320];
    unsigned char *v = (unsigned char*)&lightmap;
    int x, y, dx, dy;
    int a_lx0 = -lx + 96, a_lx = -lx, a_ly = -ly + 156 + 1;       

    ///*    

    _asm {
        mov   esi, v
        mov   edx, p
        mov   edi, s
        mov   ecx, 198
        
        @outer:
        push  ecx
        mov   ecx, 320
        mov   eax, [a_lx0]
        mov   [a_lx], eax
        
        @inner:
        // calculate dx/dy
        mov   al, [edx + 1]    // 1 (cache hit assumed)
        mov   bl, [edx + 320]  // .
        sub   al, [edx - 1]    // 2-3
        sub   bl, [edx - 320]  // .
        // al = (*(p+1) - *(p-1)), bl = (*(p+320) - *(p-320))
        shl   eax, 24          // 4
        inc   edx              // .
        sar   eax, 24          // 5
        shl   ebx, 24          // 8
        inc   edi              // .
        sar   ebx, 24          // 9
        add   eax, [a_lx]      // 6-7
        add   ebx, [a_ly]      // 10-11
        // eax = dx = (*(p+1) - *(p-1)) - lx + x; edx = dy = (*(p+320) - *(p-320)) - ly + y;
        inc   [a_lx]           // 12-14
        shl   ebx, 9           // 15
        add   eax, ebx         // 16
        and   eax, (LIGHT_SIZE*LIGHT_SIZE)-1 // можно убрать, но возможны глюки
        // eax = (dy << 9) + dx
        mov   al, [esi + eax]  // 17
        dec   ecx              // .
        mov   [edi - 1], al    // 18
        jnz   @inner           // .
        
        pop   ecx 
        inc   [a_ly]
        dec   ecx
        jnz   @outer
    }
    //*/
    /*
    for (y = 156+1; y < 199+156; y++) {
        for (x = 96; x < 320+96; x++) {
            dx = (*(p+1) - *(p-1)) - lx + x; dy = (*(p+320) - *(p-320)) - ly + y;
            *s++ = v[(dy << 9) + dx];
            p++;
        }
    }
    */
}

int tick = 0;

void timer() { tick++; }

int main() {
    int i;
    
    initsintab();
    initenv();
    //initlight();
    initflare();
    
    _asm {    
        // zpizzheno ;)
        mov ax,13h
        int 10h       // regular mode 13h chained

        mov dx,3d4h   // remove protection
        mov al,11h
        out dx,al
        inc dl
        in  al,dx
        and al,7fh
        out dx,al

        mov dx,3c2h   // misc output
        mov al,0e3h   // clock
        out dx,al

        mov dx,3d4h
        mov ax,00B06h // Vertical Total
        out dx,ax
        mov ax,03E07h // Overflow
        out dx,ax
        mov ax,0C310h // Vertical start retrace
        out dx,ax
        mov ax,08C11h // Vertical end retrace
        out dx,ax
        mov ax,08F12h // Vertical display enable end
        out dx,ax
        mov ax,09015h // Vertical blank start
        out dx,ax
        mov ax,00B16h // Vertical blank end
        out dx,ax
    }
    initpal();

    rtc_initTimer(3);
    rtc_setTimer(&timer, rtc_timerRate / 60);
    
    while (!kbhit()) {
        i = tick;
        
        while ((inp(0x3DA) & 8) == 8) {}
        while ((inp(0x3DA) & 8) != 8) {}

        //outp(0x3C8, 0); outp(0x3C9, 63); outp(0x3C9, 0); outp(0x3C9, 0);
        
        memcpy(screen, &buffer, 64000);

        //outp(0x3C8, 0); outp(0x3C9, 63); outp(0x3C9, 0); outp(0x3C9, 63);
        drawflare(i);
        //drawbump(96 * sintabf[((i << 5) + (i << 6)) & 0xFFFF], 96 * sintabf[((i << 6) + (i << 6)) & 0xFFFF]);
        //outp(0x3C8, 0); outp(0x3C9, 32); outp(0x3C9, 0); outp(0x3C9, 63);
        drawbump(0, 0);
        //outp(0x3C8, 0); outp(0x3C9, 0); outp(0x3C9, 0); outp(0x3C9, 0);
    }
    getch();
    
    rtc_freeTimer();
    
    _asm {
        mov  ax, 3
        int  10h
    }
    return 0;
}
