#include <stdlib.h>
#include <conio.h>
#include <math.h>
#include <memory.h>
#include "rtctimer.h"
#include "flexptc\flexptc.c"

#define X_SIZE 512
#define Y_SIZE 256
#define sat(a, l) ((a > l) ? l : a)
#define ee 1.0E-6

unsigned long  *screen = (unsigned long*)0xA0000;

unsigned long  buffer[X_SIZE * Y_SIZE], buffer2[X_SIZE * Y_SIZE];
unsigned char  blendtab[65536];

// background sprite info
#define TILE_SIZE 16
#define X_GRID    (X_SIZE / TILE_SIZE)
#define Y_GRID    (Y_SIZE / TILE_SIZE)
 
unsigned char  bgspr [TILE_SIZE * TILE_SIZE];
unsigned char  bggrid[X_GRID * Y_GRID];

#define lut_size 65536
short sintab [lut_size], costab [lut_size];
float sintabf[lut_size], costabf[lut_size];


#define stardist 128
#define DIST 300
#include "vector.h"

#define flaresnum 256

// orange flares info
vertex   p[flaresnum], pt[flaresnum];
vertex2d p2d[flaresnum];

// cyan flares info
vertex   n[flaresnum], nt[flaresnum];
vertex2d n2d[flaresnum];

#define SPR_SIZE 12
unsigned long  flspr[SPR_SIZE * SPR_SIZE * 2];

double pi = 3.141592653589793;

void vecdraw (vertex2d *v, vertex *f, int param) {
    int i, y, x, j;
    int px, py, ofs;
    long scrptr = (long)&buffer;
    long sprptr = (long)&flspr + (SPR_SIZE * SPR_SIZE * 4 * param);

    px = v->x - (SPR_SIZE >> 1); py = v->y - (SPR_SIZE >> 1);
        
    if ((py<(Y_SIZE - (SPR_SIZE)))&&(py>0)&&(px>0)&&(px<(X_SIZE - (SPR_SIZE)))&&(f->z < 0)) {
            
        scrptr = (long)&buffer + (((py << 9) + px) << 2);
            
        _asm {
            mov    esi, sprptr
            mov    edi, scrptr
            
            mov    ecx, SPR_SIZE
                
            __outer:
            push   ecx
            mov    ecx, SPR_SIZE*4
                
            __inner:
            mov    al, [esi]
            inc    esi
            mov    ah, [edi]
            inc    edi
                
            add    al, ah
            sbb    dl, dl
            or     al, dl
            mov    [edi-1], al
                
            dec    ecx
            jnz    __inner
                
            pop    ecx
            add    edi, (X_SIZE - SPR_SIZE)*4
                
            dec    ecx
            jnz    __outer
        }
    }
}

void vecfill() {
    int rat, max, phi, teta, count2, i, lptr = 0;
    
    for (i = 0; i < flaresnum; i++) {
        p[i].x = rand() % stardist - (stardist/2);
        p[i].y = rand() % stardist - (stardist/2);
        p[i].z = rand() % stardist - (stardist/2);
    }
    
}

void flfill() {
    int rat, max, phi, teta, count2, i, lptr = 0;
    
    for (i = 0; i < flaresnum; i++) {
        n[i].x = rand() % stardist - (stardist/2);
        n[i].y = rand() % stardist - (stardist/2);
        n[i].z = rand() % stardist - (stardist/2);
    }
    
}

void initspr() {
    int r, g, b;
    int x, y, k=0;
    
    for (y = -(SPR_SIZE / 2); y < (SPR_SIZE / 2); y++) {
        for (x = -(SPR_SIZE / 2); x < (SPR_SIZE / 2); x++) {
            //flspr[k++] = sat((int)(0x600 / ((x*x + y*y) + ee)), 255) & 0xFF;
            b = ((int)sat(0x600 / (x*x + y*y + ee), 0xFF) * 0x10) >> 8;
            g = ((int)sat(0x600 / (x*x + y*y + ee), 0xFF) * 0xC0) >> 8;
            r = ((int)sat(0x600 / (x*x + y*y + ee), 0xFF) * 0xA0) >> 8;
            
            //flspr[k++] = sat(0x200 / (x*x + y*y + ee), 0xFF);
            flspr[k++] = b | (g << 8) | (r << 16); 
        }
    }
    
    for (y = -(SPR_SIZE / 2); y < (SPR_SIZE / 2); y++) {
        for (x = -(SPR_SIZE / 2); x < (SPR_SIZE / 2); x++) {
            //flspr[k++] = sat((int)(0x600 / ((x*x + y*y) + ee)), 255) & 0xFF;
            r = ((int)sat(0x600 / (x*x + y*y + ee), 0xFF) * 0x10) >> 8;
            g = ((int)sat(0x600 / (x*x + y*y + ee), 0xFF) * 0xC0) >> 8;
            b = ((int)sat(0x600 / (x*x + y*y + ee), 0xFF) * 0xA0) >> 8;
            
            //flspr[k++] = sat(0x200 / (x*x + y*y + ee), 0xFF);
            flspr[k++] = b | (g << 8) | (r << 16); 
        }
    }
}

void initbg() {
    int x, y, k=0;
    
    for (y = -(TILE_SIZE / 2); y < (TILE_SIZE / 2); y++) {
        for (x = -(TILE_SIZE / 2); x < (TILE_SIZE / 2); x++) {
            bgspr[k++] = (((0x280 * TILE_SIZE) / (x*x + y*y + ee)) <= (0x10 * TILE_SIZE) ? 0 : 0xFF);
            //bgspr[k++] = sat((0x8000 / (x*x + y*y + ee)), 255);
        }
    }
}

void initsintab() {
    int i, j;
    double r, lut_mul;
    lut_mul = (2 * pi / 65536);
    for (i = 0; i < 65536; i++) {
        r = i * lut_mul;
        sintab [i] = 32767 * sin(r);
        costab [i] = 32767 * cos(r);
        sintabf[i] = sin(r);
        costabf[i] = cos(r);
    }
}

void adjustbuf(int j, int dir) {
    int x, y, i, k=0;
    
    if (dir) for (y = 0; y < Y_GRID; y++) {
        for (x = 0; x < X_GRID-1 ; x++) {
            bggrid[k] = bggrid[k+1];
            k++;
        }
        bggrid[k++] = 0;
    } else for (y = 0; y < Y_GRID; y++) {
        k = (X_GRID * (y+1) - 1);
        for (x = X_GRID; x > 1; x--) {
            bggrid[k] = bggrid[k-1];
            k--;
        }
        bggrid[k--] = 0;
    }
    
    // draw random grid nodes
    for (i = 0; i < 1; i++) {
        k = ((rand() % Y_GRID) * X_GRID) + (dir ? X_GRID-1 : 0);
        bggrid[k] = ((rand() & 0x0F) + 8);
    }
    
    //if (!(rand() & 0x3F)) j ^= 0x42;
    
    k = abs((j % ((Y_GRID*2) + 1)) - Y_GRID) * X_GRID + (dir ? X_GRID-1 : 0);
    bggrid[k] = 0x0F;
}

void drawbuf(int i) {
    unsigned int  k=0, c, l, b, x, y, col, d;
    unsigned long *buf = &buffer;
    unsigned char *spr = &bgspr;
    
    /*
    // fill upper gap
    _asm {
        cld
        mov    eax, 0x01010101
        mov    ecx, (X_SIZE * (Y_SIZE - (TILE_SIZE * (Y_SIZE / TILE_SIZE)))) / 8
        mov    edi, buf
        rep    stosd
    }
    
    // adjust pointer
    buf += (X_SIZE * (Y_SIZE - (TILE_SIZE * (Y_SIZE / TILE_SIZE)))) / 2;
    
    // draw background sprites (tiles in fact ;)
    for (y = 0; y < Y_GRID; y++) {
        for (x = 0; x < X_GRID; x++) {
            col = bggrid[k++] * 0x01010101;
            spr = &bgspr;
            _asm {
                cld
                mov    edx, col
                mov    esi, spr
                mov    edi, buf
                mov    ebx, TILE_SIZE
                
                __y_loop:
                mov    ecx, (TILE_SIZE / 4)
                
                __x_loop:
                mov    eax, [esi]
                and    eax, edx
                add    esi, 4
                add    eax, 0x01010101
                mov    [edi], eax
                add    edi, 4
                dec    ecx
                jnz    __x_loop
                
                add    edi, (X_SIZE - TILE_SIZE);
                dec    ebx
                jnz    __y_loop
            }
            buf += TILE_SIZE;
        }
        buf += (X_SIZE * (TILE_SIZE-1));
    }
    
    // fill lower gap
    _asm {
        cld
        mov    eax, 0x01010101
        mov    ecx, (X_SIZE * (Y_SIZE - (TILE_SIZE * (Y_SIZE / TILE_SIZE)))) / 8
        mov    edi, buf
        rep    stosd
    }
    */
    
    
    _asm {
        cld
        mov    eax, 0x01010101
        mov    ecx, (X_SIZE * Y_SIZE)
        mov    edi, buf
        rep    stosd
    }
    
    // fill random buffer lines
    for (k = 0; k < 40; k++) {
        c = (rand() & 0x3F) * 0x01010101;
        l = (rand() % Y_SIZE) * X_SIZE;
        buf = &buffer[l];
        _asm {
            cld
            mov    eax, c
            mov    ecx, X_SIZE
            mov    edi, buf
            
            __loop:
            add    [edi], eax
            add    edi, 4
            dec    ecx
            jnz    __loop            
        }
    }
}

void blend();
#pragma aux blend = " mov   esi, offset blendtab  " \
                    " mov   ebx, offset buffer    " \
                    " mov   edi, offset buffer2   " \
                    " mov   ecx, 131072*4" \
                    " xor   edx, edx              " \
                    " nop                         " \
                    " __inner:                     " \
                    " mov   dh,    [ebx]          " \
                    " mov   dl,    [edi]          " \
                    " inc   ebx                   " \
                    " mov   al,    [esi + edx]    " \
                    " mov   [edi], al             " \
                    " inc   edi                   " \
                    " dec   ecx                   " \
                    " jnz   __inner                " \
                    modify [esi edi ebx ecx edx];

volatile int tick = 0;
void timer() { tick++;}

int main() {
    int i = 0, j, k = 0, e;
    int rx, ry, rz;
    vertex dv, nv;
    int oi, di;
    const objdist = -32;    

    srand(inp(0x40));
    initsintab();
    initbg();
    initspr();
    vecfill();
    flfill();
 
    if (ptc_open("", X_SIZE, Y_SIZE, 32, 0)) {return 0;}
    
    for (j = 0; j < 256; j++) for (i = 0; i < 256; i++) blendtab[k++] = sat(((i * 192) >> 8) + ((j * 128) >> 8), 255);
    
    rtc_initTimer(3);
    rtc_setTimer(&timer, rtc_timerRate / 60);

    i = 0;
    while (!kbhit()) {
        oi = i; _disable(); i = tick; _enable();
        di = (i - oi); if (di == 0) di = 1;
        ptc_wait();
        
        ptc_update(&buffer2);
        
        drawbuf(i);
        
        dv.x = ((float)stardist/3) * sintabf[((i << 6)) & 0xFFFF];
        dv.y = ((float)stardist/3) * sintabf[((i << 5)) & 0xFFFF];
        dv.z = ((float)stardist/3) * costabf[((i << 6) + (i << 5)) & 0xFFFF];
        
        
        nv = dv;
        vecnormalize(&nv);
        
        /*
        rx = ((i << 7) + 0x8000) & 0xFFFF;
        ry = ((i << 6) + 0x2000) & 0xFFFF;
        rz = ((i << 7) + (int)(nv.x * 32768)) & 0xFFFF;
        */
        
        
        rx = (int)(nv.x * 32768);
        ry = (int)(nv.y * 32768);
        rz = (int)(nv.z * 32768);
        
        
        // hujak sprites ;)
        for (j = 0; j < flaresnum; j++) {
            pt[j] = p[j];
            
            vecrotate   ((rx) & (lut_size - 1),
                         (ry) & (lut_size - 1),
                         (rz) & (lut_size - 1),
                        &pt[j]);
            
            vecmove     (dv.x,
                         dv.y,
                         dv.z + objdist, &pt[j]);
            
            vecproject2d(&pt[j], &p2d[j]);
            vecdraw     (&p2d[j], &pt[j], 0);
        }
        
        for (j = 0; j < flaresnum; j++) {
            nt[j] = n[j];
            
            vecrotate   ((rx) & (lut_size - 1),
                         (ry) & (lut_size - 1),
                         (rz) & (lut_size - 1),
                        &nt[j]);
            
            vecmove     (dv.x,
                         dv.y,
                         dv.z + objdist, &nt[j]);
            
            vecproject2d(&nt[j], &n2d[j]);
            vecdraw     (&n2d[j], &nt[j], 1);
        }
        
        blend();
    }
    
    getch();
    ptc_close();
    rtc_freeTimer();
    _asm {
        mov  ax, 3
        int  10h
    }
}
