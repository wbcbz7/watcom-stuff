#include <stdlib.h>
#include <conio.h>
#include <math.h>
#include <memory.h>
#include "rtctimer.h"
#include "..\..\flexptc\flexptc.c"

#define X_SIZE 640
#define Y_SIZE 350
#define sat(a, l) ((a > l) ? l : a)
#define ee 1.0E-6

unsigned char  *screen = (unsigned char*)0xA0000;
ptc_palette    pal[256];

unsigned char  buffer[X_SIZE * Y_SIZE], buffer2[X_SIZE * Y_SIZE];
unsigned char  blendtab[65536];

// background sprite info
#define TILE_SIZE 32
#define X_GRID    (X_SIZE / TILE_SIZE)
#define Y_GRID    (Y_SIZE / TILE_SIZE)
 
unsigned char  bgspr [TILE_SIZE * TILE_SIZE];
unsigned char  bggrid[X_GRID * Y_GRID];

#define lut_size 65536
short sintab [lut_size], costab [lut_size];
float sintabf[lut_size], costabf[lut_size];

// 3d info
#define stardist 288
#define DIST 300
#include "vector.h"

// lines info
#define  faces 8
face     f[faces], fz[faces];

// wireframe node\line info
#define  nodes         (faces + 2)
vertex   n[nodes], nt[nodes];
vertex2d n2d[nodes];

#define SPR_SIZE 12
unsigned char  flspr[SPR_SIZE * SPR_SIZE];


double pi = 3.141592653589793;

void vecdraw (vertex2d *v, vertex *f) {
    int i, y, x, j;
    int px, py, ofs;
    long scrptr = (long)&buffer;
    long sprptr = (long)&flspr;

    px = v->x - (SPR_SIZE >> 1); py = v->y - (SPR_SIZE >> 1);
        
    if ((py<(Y_SIZE - (SPR_SIZE)))&&(py>0)&&(px>0)&&(px<(X_SIZE - (SPR_SIZE)))&&(f->z < 0)) {
            
        scrptr = (long)&buffer + ((py << 9) + (py << 7) + px);
            
        _asm {
            mov    esi, sprptr
            mov    edi, scrptr
            
            mov    ecx, SPR_SIZE
                
            __outer:
            push   ecx
            mov    ecx, SPR_SIZE
                
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
            add    edi, (X_SIZE - SPR_SIZE)
                
            dec    ecx
            jnz    __outer
        }
    }
}

void linedraw(vertex2d *a, vertex2d *b, line *f) {
    //vertex2d a, b;

    int delta_x, delta_y, dx, dy, t, d;
    int xerr = 0, yerr = 0;
    
    int sx, sy;
    unsigned char *p = (unsigned char*)&buffer;
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
        p = &buffer[((sy << 9) + (sy << 7) + sx)];
        //col = 64;
        col = f->col;
        
        _asm {
            mov    esi, p
            mov    ebx, col
            
            mov    al, [esi]   // 1
            inc    esi         // .
            add    al, bl      // 2
            sbb    dl, dl      // .
            or     al, dl      // 3
            mov    [esi-1], al // .
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


#define vcode(p) (((p->x < 0) ? 1 : 0) | ((p->x >= X_SIZE) ? 2 : 0) | ((p->y < 0) ? 4 : 0) | ((p->y >= Y_SIZE) ? 8 : 0))    
 
int lineclip(vertex2d *a, vertex2d *b) {
    int code_a, code_b, code;
    vertex2d *c;
 
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
            c->y = c->y + (float)(a->y - b->y) * (0 - c->x) / (a->x - b->x + ee);
            c->x = 0;
        } else if (code & 2) {
            c->y = c->y + (float)(a->y - b->y) * (X_SIZE - c->x) / (a->x - b->x + ee);
            c->x = X_SIZE - 1;
        }

        if (code & 4) {
            c->x = c->x + (float)(a->x - b->x) * (0 - c->y) / (a->y - b->y + ee);
            c->y = 0;
        } else if (code & 8) {
            c->x = c->x + (float)(a->x - b->x) * (Y_SIZE - c->y) / (a->y - b->y + ee);
            c->y = Y_SIZE - 1;
        }
 
        if (code == code_a)
            code_a = vcode(a);
        else
            code_b = vcode(b);
    }
 
    return 0;
}


void vecfill() {
    int rat = (lut_size / faces), max, i, lptr = 0;
    int radius = (X_SIZE / 2) * sqrt(2);
    //int radius = 200;
    
    n[0].x = n[0].y = n[0].z = 0;
    
    for (i = 1; i < nodes; i++) {
        n[i].x = radius * costabf[(rat * i) & 0xFFFF];
        n[i].y = radius * sintabf[(rat * i) & 0xFFFF];
        n[i].z = 0;
    }
}

void trifill() {
    int i, j, k;
    
    for (i = 0; i < faces; i++) {
        f[i].a = 0;
        f[i].b = ((i + 1) % nodes);
        f[i].c = ((i + 1) % nodes) + 1;
        
        f[i].col = (rand() & 0x1F) + 32;
    }
}

void initpal() {  
    int i;
    
    pal[0].r = pal[0].g = pal[0].b = 0;
    for (i = 1; i < 256; i++) {
        pal[i].r = sat(8  + (i >> 2) + (i >> 3), 63);
        pal[i].g = sat(12 + (i >> 1) + (i >> 4), 63); 
        pal[i].b = sat(10 + (i >> 1) + (i >> 3) + (i >> 4), 63);
    }
}

void initspr() {
    int x, y, k=0;
    
    for (y = -(SPR_SIZE / 2); y < (SPR_SIZE / 2); y++) {
        for (x = -(SPR_SIZE / 2); x < (SPR_SIZE / 2); x++) {
            //flspr[k++] = sat((int)(0x600 / ((x*x + y*y) + ee)), 255) & 0xFF;
            flspr[k++] = sat((int)(0x200 / ((x*x + y*y) + ee)), 128) & 0xFF;
        }
    }
}

void initbg() {
    int x, y, k=0;
    
    for (y = -(TILE_SIZE / 2); y < (TILE_SIZE / 2); y++) {
        for (x = -(TILE_SIZE / 2); x < (TILE_SIZE / 2); x++) {
            bgspr[k++] = (((0x800 * TILE_SIZE) / (x*x + y*y + ee)) <= (0x10 * TILE_SIZE) ? 0 : 0xFF);
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

void adjustbuf() {
    int x, y, i, k=0;
    
    for (y = 0; y < Y_GRID; y++) {
        for (x = 0; x < X_GRID; x++) {
            bggrid[k] = (bggrid[k] <= 4 ? 4 : bggrid[k] - 1);
            k++;
        }
    }

    // draw random grid nodes
    for (i = 0; i < 2; i++) {
        k = (rand() % (Y_GRID * X_GRID));
        //bggrid[k] = ((rand() & 0x1F) + 8);
        bggrid[k] = 0;
    }
}


void drawbuf(int i) {
    unsigned int  k=0, c, l, x, y,col;
    unsigned char *buf = &buffer;
    unsigned char *spr = &bgspr;
    
    
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
            /*
            _asm {
                cld
                mov    esi, spr
                mov    edi, buf
                mov    ebx, TILE_SIZE
                
                __y_loop:
                mov    ecx, (TILE_SIZE / 4)
                rep    movsd
                
                add    edi, (X_SIZE - TILE_SIZE);
                dec    ebx
                jnz    __y_loop
            }
            */
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
    
    
    // fill random buffer lines
    for (k = 0; k < 40; k++) {
        c = (rand() & 0xF) * 0x01010101;
        l = ((rand() % (Y_SIZE - 1)) * X_SIZE);
        buf = &buffer[l];
        _asm {
            cld
            mov    eax, c
            mov    ecx, (X_SIZE / 4)
            mov    edi, buf
            
            __loop:
            add    [edi], eax
            add    edi, 4
            dec    ecx
            jnz    __loop            
        }
    }
}

void satline (void *dst, int color, int len) {
    _asm {
        mov    edi, dst
        mov    ebx, color
        mov    ecx, len
        
        __inner:
        mov    al, [edi]
        inc    edi
                
        add    al, bl
        sbb    dl, dl
        or     al, dl
        mov    [edi-1], al
                
        dec    ecx
        jnz    __inner
    }
}

void facedraw(face *f) {
    vertex2d a, b, c, d;
    int fa, fb, fc;
    int sx1, sx2, sx3, dx1, dx2, sy, tmp_dx;
    int k_ab, k_bc, k_ac;
    int i, col, len;
    unsigned char *scrptr;
    
    col = f->col * 0x01010101;
    
    a = n2d[f->a]; b = n2d[f->b]; c = n2d[f->c];
    
    if (a.y > b.y) {d = a; a = b; b = d;}
    if (a.y > c.y) {d = a; a = c; c = d;}
    if (b.y > c.y) {d = b; b = c; c = d;}
    
    // clip polygon if ((a.y < 0) || (b.y << 0)
    /*
    if ((a.y < 0) && (b.y < 0)) {
        // c->x += (a->x - b->x) * (0 - c->y) / (a->y - b->y + ee);
        float slope_a = (0 - c.y) / (c.y - a.y);
        float slope_b = (0 - c.y) / (c.y - b.y);
        
        a.x += (float)((c.x - a.x) * slope_a);
        b.x += (float)((c.x - b.x) * slope_b);
        
        
        a.y = 0;
        b.y = 0;
    }
    */
    
    if ((a.y < 0) && (b.y < 0)) {
        lineclip(&a, &c);
        lineclip(&b, &c);
    }
    
    if (b.y == c.y) {k_bc = 0;} else {k_bc = (((c.x - b.x) << 16)) / (c.y - b.y);}
    
    if (a.y == c.y) {return;  } else {k_ac = (((c.x - a.x) << 16)) / (c.y - a.y);}
    
    if (b.y == a.y) {k_ab = 0;} else {k_ab = (((b.x - a.x) << 16)) / (b.y - a.y);}
    
    dx1 = a.x << 16; if (a.y == b.y) dx2 = b.x << 16; else dx2 = dx1;
    
    if (a.y != b.y) for (sy = a.y; sy < b.y; sy++) {
        sx1 = (dx1 >> 16) + ((dx1 & 0x8000) >> 15); sx2 = (dx2 >> 16) + ((dx2 & 0x8000) >> 15);
        
        if (sx1 > sx2) {sx3 = sx1; sx1 = sx2; sx2 = sx3;}
        
        if (sy >= Y_SIZE) return;
        if ((sy < 0) || (sx1 >= X_SIZE) || (sx2 < 0)) {dx1 += k_ac; dx2 += k_ab; continue;}
        if (sx1 < 0) sx1 = 0; if (sx2 >= X_SIZE) sx2 = (X_SIZE - 1);
        
        scrptr = &buffer[((sy << 9) + (sy << 7) + sx1)];
        len = sx2 - sx1;
        
        if (len != 0) satline(scrptr, col, len);
        //if (len != 0) memset(scrptr, col, len);
        
        dx1 += k_ac; dx2 += k_ab;
    }
    
    if (b.y == c.y) return;
    for (sy = b.y; sy < c.y; sy++) {
        sx1 = (dx1 >> 16) + ((dx1 & 0x8000) >> 15); sx2 = (dx2 >> 16) + ((dx2 & 0x8000) >> 15);
        
        if (sx1 > sx2) {sx3 = sx1; sx1 = sx2; sx2 = sx3;}
        
        if (sy >= Y_SIZE) return;
        if ((sy < 0) || (sx1 >= X_SIZE) || (sx2 < 0)) {dx1 += k_ac; dx2 += k_ab; continue;}
        if (sx1 < 0) sx1 = 0; if (sx2 >= X_SIZE) sx2 = (X_SIZE - 1);
        
        scrptr = &buffer[((sy << 9) + (sy << 7) + sx1)];
        len = sx2 - sx1;
        
        if (len != 0) satline(scrptr, col, len);
        //if (len != 0) memset(scrptr, col, len);
        
        dx1 += k_ac; dx2 += k_bc;
    }
}

void blend();
#pragma aux blend = " mov   esi, offset blendtab  " \
                    " mov   ebx, offset buffer    " \
                    " mov   edi, offset buffer2   " \
                    " mov   ecx, 224000" \
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
    int i, j, k = 0;
    int atick;
    
    int rx = 0, ry = 0, rz = 0;
    int rxdx = 0, rydx = 0, rzdx = 0;
    
    srand(inp(0x40));
    initsintab();
    initpal();
    initbg();
    initspr();
    
    trifill();
    vecfill();
 
    if (ptc_open("", X_SIZE, Y_SIZE, 8, 0)) {return 0;}
    
    for (j = 1; j < 256; j++) for (i = 1; i < 256; i++) blendtab[k++] = sat(((i * 128) >> 8) + ((j * 128) >> 8), 255);
    
    ptc_setpal(&pal[0]);
    
    rtc_initTimer(3);
    rtc_setTimer(&timer, rtc_timerRate / 60);
    
    while (!kbhit()); getch();
    
    while (!kbhit()) {
        i = tick;
        ptc_wait();
        
        if (!((i ^ 0x5A5A) % 60)) {
            rxdx = (((rand() & 0xF) - 8) * 0x20);
            rydx = (((rand() & 0xF) - 8) * 0x20);
            rzdx = (((rand() & 0xF) - 8) * 0x20);
        }
        
        rx += rxdx; ry += rydx; rz += rzdx;
        
        
        ptc_update(&buffer);
        adjustbuf();
        drawbuf(i);
        
        // hujak nodes
        for (j = 0; j < nodes; j++) {
            nt[j] = n[j];
            vecrotate    (0,
                          0,
                          (i << 6) & (lut_size - 1),
                          &nt[j]);
            vecmove      (0,
                          0,
                          -DIST, &nt[j]);
            
            vecproject2dp(&nt[j], &n2d[j]);
            vecdraw      (&n2d[j], &nt[j]);
        }
        
        // hujak lines
        for (j = 0; j < faces; j++) {
            facedraw(&f[j]);
        }
        //blend();
        //outp(0x3C8, 0); outp(0x3C9, 0); outp(0x3C9, 0); outp(0x3C9, 0); 
    }
    
    getch();
    ptc_close();
    rtc_freeTimer();
    _asm {
        mov  ax, 3
        int  10h
    }
    
    return 0;
}
