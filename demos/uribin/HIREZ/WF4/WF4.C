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
unsigned char  noisetab[X_SIZE * Y_SIZE * 2];

// background sprite info
#define TILE_NUM  16
#define TILE_SIZE 16
#define X_GRID    (X_SIZE / TILE_SIZE)
#define Y_GRID    (Y_SIZE / TILE_SIZE)
 
unsigned char  bgspr [TILE_SIZE * TILE_SIZE * TILE_NUM];
unsigned char  bggrid[X_GRID * Y_GRID];

#define lut_size 65536
short sintab [lut_size], costab [lut_size];
float sintabf[lut_size], costabf[lut_size];

// 3d info
#define stardist 256
float DIST = 150;
#include "vector.h"

//flares info
#define  flaredist 32
#define  flares    256
vertex   p[flares], pt[flares];
vertex2d p2d[flares];

// wireframe node\line info
#define  facenum   6
#define  nodesize  8
#define  nodes     (nodesize * nodesize * facenum)
vertex   n[nodes], nt[nodes];
vertex2d n2d[nodes];

// lines info
#define  lines nodes*2
line     f[lines];

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
            c->y += (a->y - b->y) * (0 - c->x) / (a->x - b->x + ee);
            c->x = 0;
        } else if (code & 2) {
            c->y += (a->y - b->y) * (X_SIZE - c->x) / (a->x - b->x + ee);
            c->x = X_SIZE - 1;
        }

        if (code & 4) {
            c->x += (a->x - b->x) * (0 - c->y) / (a->y - b->y + ee);
            c->y = 0;
        } else if (code & 8) {
            c->x += (a->x - b->x) * (Y_SIZE - c->y) / (a->y - b->y + ee);
            c->y = Y_SIZE - 1;
        }
 
        if (code == code_a)
            code_a = vcode(a);
        else
            code_b = vcode(b);
    }
 
    return 0;
}


void vecload() {
    int phi, teta, count2, i, j, lptr = 0, dptr = 0;
    float max, rat, scale;
    
    float dx, dy, dz, rx, ry, rz;
    
    max = nodesize;
    rat = (stardist / max);
    for (j = 0; j < facenum; j++) {
        for (teta = 0; teta < max; teta++) {
            for (phi = 0; phi < max; phi++) {
                n[lptr  ].x = (rat * teta) - (stardist/2) + (rat/2);
                n[lptr  ].y = (rat * phi ) - (stardist/2) + (rat/2);
                n[lptr++].z = 0;
            }
        }
        
        // switch and rotate
        switch(j) {
            case 0: {
                rx = 0; ry = 0; rz = 0;
                dx = 0; dy = 0; dz = ((stardist-rat)/2);
                break;
            }
            case 1: {
                rx = 0; ry = 0; rz = 0;
                dx = 0; dy = 0; dz = -((stardist-rat)/2);
                break;
            }
            case 2: {
                rx = 0; ry = 0x4000; rz = 0;
                dx = ((stardist-rat)/2); dy = 0; dz = 0;
                break;
            }
            case 3: {
                rx = 0; ry = 0x4000; rz = 0;
                dx = -((stardist-rat)/2); dy = 0; dz = 0;
                break;
            }
            case 4: {
                rx = 0x4000; ry = 0; rz = 0;
                dx = 0; dy = ((stardist-rat)/2); dz = 0;
                break;
            }
            case 5: {
                rx = 0x4000; ry = 0; rz = 0;
                dx = 0; dy = -((stardist-rat)/2); dz = 0;
                break;
            }
        }
        
        
        for (i = 0; i < (nodesize * nodesize); i++) {
            vecrotate(rx, ry, rz, &n[dptr  ]);
            vecmove  (dx, dy, dz, &n[dptr++]);
        }
        
    }
}

void triload() {
    int max, phi, teta, count2, i, j, lptr = 0;
    float rat;
    
    
    for (j = 0; j < facenum; j++) {
        // fill line values
        
        max = nodesize;
        for (teta = 0; teta < max-1; teta++) {
            for (phi = 0; phi < max; phi++) {
                f[lptr  ].a   = (j * (nodesize * nodesize)) + (phi*max + (teta % max));
                f[lptr  ].b   = (j * (nodesize * nodesize)) + (phi*max + ((teta + 1) % max));
                f[lptr++].col = 32;
            }
            lptr++;
        }
    
        for (teta = 0; teta < max; teta++) {
            for (phi = 0; phi < max-1; phi++) {
                f[lptr  ].a   = (j * (nodesize * nodesize)) + ((phi % max)*max + teta);
                f[lptr  ].b   = (j * (nodesize * nodesize)) + (((phi + 1) % max)*max + teta);
                f[lptr++].col = 32;
            }
            lptr++;
        }
    }
}

void flfill() {
    int rat, max, phi, teta, count2, i, lptr = 0;
    
    for (i = 0; i < flares; i++) {
        p[i].x = flaredist * (sintabf[((i << 9)) & 0xFFFF]);
        p[i].y = flaredist * (costabf[((i << 9)) & 0xFFFF]);
        p[i].z = flaredist * (costabf[((i << 8) + (i << 9)) & 0xFFFF]);
    }
}

void initnoise() {
    int i;
    
    for (i = 0; i < (X_SIZE*Y_SIZE*2); i++) {
        noisetab[i] = rand() & 0x0F;
    }
}

void initpal() {  
    int i;
    
    pal[0].r = pal[0].g = pal[0].b = 0;
    for (i = 1; i < 256; i++) {
        pal[i].r = sat(8  + (i >> 2) + (i >> 3), 63);
        pal[i].g = sat(10 + (i >> 1) + (i >> 3), 63); 
        pal[i].b = sat(12 + (i >> 1) + (i >> 3) + (i >> 4), 63);
    }
}

void initspr() {
    int x, y, k=0;
    
    for (y = -(SPR_SIZE / 2); y < (SPR_SIZE / 2); y++) {
        for (x = -(SPR_SIZE / 2); x < (SPR_SIZE / 2); x++) {
            //flspr[k++] = sat((int)(0x600 / ((x*x + y*y) + ee)), 255) & 0xFF;
            flspr[k++] = sat((int)(0x100 / ((x*x + y*y) + ee)), 128) & 0xFF;
        }
    }
}

void initbg() {
    int x, y, i, k=0;
    
    for (i = 0; i < TILE_NUM; i++)
    for (y = -(TILE_SIZE / 2); y < (TILE_SIZE / 2); y++) {
        for (x = -(TILE_SIZE / 2); x < (TILE_SIZE / 2); x++) {
            bgspr[k++] = (((0x20 * TILE_SIZE * i) / (x*x + y*y + ee)) <= (0x10 * TILE_SIZE) ? 1 : 0x8);
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
            bggrid[k] = (bggrid[k] >= (TILE_NUM-1) ? (TILE_NUM-1) : bggrid[k] + 1);
            k++;
        }
    }

    // draw random grid nodes
    for (i = 0; i < 40; i++) {
        k = (rand() % (Y_GRID * X_GRID));
        //bggrid[k] = ((rand() & 0x1F) + 8);
        bggrid[k] = 0;
    }
}


void drawbuf(int i) {
    unsigned int  k=0, c, l, x, y,col;
    unsigned char *buf   = &buffer;
    unsigned char *spr   = &bgspr;
    unsigned char *noise = &noisetab;
    
    // затереть ВСЕ!
    l = ((rand() % (Y_SIZE - 1)) * X_SIZE);
    noise = &noisetab[l];
    
    _asm {
        cld
        mov    eax, 0x01010101
        mov    ecx, (X_SIZE * Y_SIZE) / 4
        mov    esi, noise
        mov    edi, buf
        rep    movsd
        
        /*
        __loop:
        mov    eax, [esi]
        add    esi, 4
        and    eax, 0x1f1f1f1f
        mov    [edi], eax
        add    edi, 4
        dec    ecx
        jnz    __loop;*/
    }
    
    // fill random buffer lines
    for (k = 0; k < 40; k++) {
        c = (rand() & 0x1F) * 0x01010101;
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
    int oi, di;
    int objdist = 0;
    
    int rx = 0, ry = 0, rz = 0;
    float dx = 0, dy = 0, dz = 0;
    int rxdx, rydx, rzdx;
    
    vertex dv, nv;
    
    srand(inp(0x40));
    initsintab();
    initpal();
    initbg();
    initspr();
    initnoise();
    
    triload();
    vecload();
    flfill();
 
    if (ptc_open("", X_SIZE, Y_SIZE, 8, 0)) {return 0;}
    
    for (j = 0; j < 256; j++) for (i = 0; i < 256; i++) blendtab[k++] = sat(((i * 64) >> 8) + ((j * 192) >> 8), 255);
    
    ptc_setpal(&pal[0]);
    
    rtc_initTimer(3);
    rtc_setTimer(&timer, rtc_timerRate / 60);
    
    while (!kbhit()) {
        oi = i; i = tick; di = (i - oi); if (di == 0) di = 1;
        ptc_wait();
           
        /*
        dv.x = ((float)stardist/4) * costabf[((i << 6) + (i << 5)) & 0xFFFF];
        dv.y = ((float)stardist/4) * sintabf[((i << 8) + (i << 4)) & 0xFFFF];
        dv.z = ((float)stardist/4) * costabf[((i << 5) + (i << 7)) & 0xFFFF];
              
        nv = dv;
        vecnormalize(&nv);
        
        rx = (int)(nv.x * 32768);
        ry = (int)(nv.y * 32768);
        rz = (int)(nv.z * 32768);
        
        */
        
        dv.x = 0;
        dv.y = 0;
        
        dv.z = -(stardist/4);
        rx = (i << 7) + (sintab[(i << 8) & 0xFFFF] >> 4);
        ry = (i << 7);
        rz = (i << 8) + (sintab[(i << 7) & 0xFFFF] >> 1);
        
        //DIST = 200 + 50 * sintabf[(i << 9) & 0xFFFF];
        
        DIST = 150;
        
        ptc_update(&buffer);
        
        drawbuf(i);
        
        
        // hujak flares
        for (j = 0; j < flares; j++) {
            
            pt[j] = p[j];
            vecmove    (0,
                        0,
                        0, &pt[j]);
                        
            vecrotate  ((rx) & (lut_size-1),
                        ((ry)) & (lut_size - 1),
                        ((rz)) & (lut_size - 1),
                        &pt[j]);
            vecmove    (dv.x,
                        dv.y,
                        dv.z + objdist, &pt[j]);
            
            vecproject2d(&pt[j], &p2d[j]);
            vecdraw     (&p2d[j], &pt[j]);
        }
        
        
        // hujak nodes
        for (j = 0; j < nodes; j++) {
            nt[j] = n[j];
            vecrotate  ( (rx) & (lut_size-1),
                        ((ry)) & (lut_size - 1),
                        ((rz)) & (lut_size - 1),
                        &nt[j]);
            vecmove     (dv.x,
                         dv.y,
                         dv.z + objdist, &nt[j]);
            
            //vecproject2d(&nt[j], &n2d[j]);
            //vecdraw     (&n2d[j], &nt[j]);
        }
        
        
        // hujak lines
        for (j = 0; j < lines; j++) {
            vertex   fa, fb, fd;
            vertex2d ca, cb, cd;
            line cf;
            
            cf = f[j];
            fa = nt[cf.a]; fb = nt[cf.b];
            if ((fb.z >= 0) && (fa.z >= 0)) continue;
            if (fb.z > fa.z) {fd = fa; fa = fb; fb = fd;}            
            
            
            if (fa.z >= 0){
                vertex pa = fa;
                
                //c->y += (a->y - b->y) * (0 - c->x) / (a->x - b->x + ee);
                
                pa.y = fa.y + (fa.y + fb.y) * (-fb.z+DIST) / (fa.z - fb.z);
                pa.x = fa.x + (fa.x + fb.x) * (-fb.z+DIST) / (fa.z - fb.z);
                pa.z = -DIST;
                
                fa = pa;
            }
            
            vecproject2d(&fa, &ca);
            vecproject2d(&fb, &cb);
            
            if (ca.y > cb.y) {cd = cb; cb = ca; ca = cd;} 
            
            if (lineclip(&ca, &cb) == 0) {linedraw(&ca, &cb, &cf); }
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
