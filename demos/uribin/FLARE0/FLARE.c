#include <math.h>
#include <strings.h>
#include <stdlib.h>
#include <stdio.h>

#include "rtctimer.h"

typedef struct {
    float x, y, z, d;
} vertex;

typedef struct { float x, y; } vertex2d;

//#define X_SIZE 256
//#define Y_SIZE 192

#define X_SIZE 320
#define Y_SIZE 200
#define DIST   200

#define stardist 64
#define count    1024
#define lut_size 65536
#define spr_size 8
#define sat(a, l) ((a > l) ? l : a)
#define ee        1.0E-6

int random(int a) {
    int i, j; float r;
    
    r = rand();
    r /= RAND_MAX;
    i = a * r;
    return a; // 22:42 на часах...я не хочу еще спать :)
}

double pi = 3.141592653589793;

unsigned char spr[spr_size * spr_size];

vertex   p[count], pt[count], pm[count];
vertex2d p2d[count];

unsigned char pal[256 * 4];

short sintab[lut_size];
float sintabf[lut_size], costabf[lut_size];

unsigned char  *screen = 0xA0000;
unsigned char  fxbuffer[64000], buffer[64000];
unsigned char  fxblendtab[65536];

void fxMakeSinTable () {
    int i, j;
    double r, lut_mul;
    lut_mul = (2 * pi / lut_size);
    for (i = 0; i < lut_size; i++) {
        r = i * lut_mul;
        sintab[i] = 32767 * sin(r);
        sintabf[i] = sin(r);
        costabf[i] = cos(r);
    }
}

void fx3dRotate (int ax, int ay, int az) {
    // hehehe, this code is fully ported from my old freebasic demoz ;)
    int i;
    float sinx = sintabf[ax], cosx = costabf[ax];
    float siny = sintabf[ay], cosy = costabf[ay];
    float sinz = sintabf[az], cosz = costabf[az];
    float bx, by, bz, px, py, pz;  // temp var storage
    for (i = 0; i < count; i++) {
        pt[i] = pm[i];
        
        py = pt[i].y;
        pz = pt[i].z;
        pt[i].y = (py * cosx - pz * sinx);
        pt[i].z = (py * sinx + pz * cosx);
        
        px = pt[i].x;
        pz = pt[i].z;
        pt[i].x = (px * cosy - pz * siny);
        pt[i].z = (px * siny + pz * cosy);
        
        px = pt[i].x;
        py = pt[i].y;
        pt[i].x = (px * cosz - py * sinz);
        pt[i].y = (px * sinz + py * cosz);
    }
} 

void fx3dMove (float ax, float ay, float az) {
    int i;
    
    for (i = 0; i < count; i++) {
        pt[i].x += ax;
        pt[i].y += ay;
        pt[i].z += az;
    }
}

void fx3dMovep (float ax, float ay, float az) {
    int i;
    
    for (i = 0; i < count; i++) {
        pm[i].x = p[i].x + ax;
        pm[i].y = p[i].y + ay;
        pm[i].z = p[i].z + az;
    }
}

void fx3dProject () {
    int i;
    float t;
    for (i = 0; i < count; i++) if (pt[i].z < 0) {
        t = DIST / (pt[i].z + ee);
        p2d[i].x = pt[i].x * t + (X_SIZE >> 1);
        p2d[i].y = pt[i].y * t + (Y_SIZE >> 1);
    }
}

void fxDrawPoints () {
    int i, y, x, j;
    int px, py, ofs;
    long scrptr = (long)&fxbuffer;
    long sprptr = (long)&spr;
    
    for (i = 0; i < count; i++) {
        sprptr = (long)&spr;
        px = p2d[i].x - (spr_size >> 1); py = p2d[i].y - (spr_size >> 1);
        
        if ((py<(Y_SIZE - (spr_size)))&&(py>0)&&(px>0)&&(px<(X_SIZE - (spr_size)))&&(pt[i].z < 0)) {
            
            scrptr = (long)&fxbuffer + ((py << 8) + (py << 6) + px);
            
            _asm {
                mov    esi, sprptr
                mov    edi, scrptr
                
                mov    ecx, spr_size
                
                @outer:
                push   ecx
                mov    ecx, spr_size
                
                @inner:
                mov    al, [esi]
                mov    ah, [edi]
                
                add    al, ah
                inc    esi
                sbb    dl, dl
                inc    edi
                or     al, dl
                mov    [edi-1], al
                
                dec    ecx
                jnz    @inner
                
                pop    ecx
                add    edi, (X_SIZE - spr_size)
                
                dec    ecx
                jnz    @outer
            }
            
            /*
            for (y = 0; y < (spr_size); y++) {
                for (x = 0; x < (spr_size); x++) {
                    *((char*)scrptr) = sat((*((char*)scrptr) + *((char*)sprptr)), 255);
                    scrptr++; sprptr++;
                }
                scrptr += (320 - (spr_size));
            }
            */
        }
    }
}

#define sd stardist

void fxFillDots() {
    int rat, max, phi, teta, count2, i, lptr = 0;
    
    /*
    for (i = 0; i < count; i++) {
        p[i].x = stardist * (costabf[(i <<  8) & 0xFFFF] + sintabf[((i << 11) + (i << 9)) & 0xFFFF]);
        p[i].y = stardist * (costabf[(i <<  8) & 0xFFFF] + sintabf[((i << 8) + (i << 6)) & 0xFFFF]);
        p[i].z = stardist * (sintabf[((i << 9) + (i << 8)) & 0xFFFF]);
    }
    */
    for (i = 0; i < count; i++) {
        p[i].x = stardist * (sintabf[(i << 8) & 0xFFFF]);
        p[i].y = stardist * (sintabf[(i << 7) & 0xFFFF]);
        p[i].z = stardist * (sintabf[((i << 7) + (i << 6)) & 0xFFFF]);
    }
    
    //*/
    /*
    for (i = 0; i < count; i++) {
        p[lptr  ].x = 0;
        p[lptr  ].y = 0;
        p[lptr++].z = 0;
    }
    */
}

void fxMakeSprite() {
    int x, y, k;
    long sprptr = (long)&spr;
    
    for (y = -(spr_size >> 1); y < (spr_size >> 1); y++) {
        for (x = -(spr_size >> 1); x < (spr_size >> 1); x++) {
            *((char*)sprptr++) = sat((int)(0xA0 / ((x*x + y*y) + ee)), 224) & 0xFF;
        }
    }
}

volatile int fxtick=0;

void initpal() {
    int i, j, k=0;
    int r, g, b;
    unsigned short d;
    
    for (i = 0; i < 256; i++) {
        pal[(i << 2)    ] = sat((i >> 2) + (i >> 4), 63);
        pal[(i << 2) + 1] = sat((i >> 1) - (i >> 4), 63); 
        pal[(i << 2) + 2] = sat((i >> 3) + (i >> 3), 63);
    }
}

void fxResize() {
    _asm {
        mov    esi, offset buffer
        mov    edi, offset fxbuffer
        
        mov    ecx, 2
        
        @outer:
        push   ecx
        mov    ecx, 100
        
        @y_loop:
        push   ecx
        push   edi
        
        in     al, 0x40
        and    eax, 0x7
        sub    eax, 0x4
        
        add    edi, eax
        mov    ecx, 160
        
        @x_loop:
        mov    al, [esi]   // 0
        add    esi, 2      // .
        shr    al, 2       // 1
        inc    edi         // .
        
        mov    ah, [esi]   // 2
        add    esi, 2      // .
        shr    ah, 2       // 3
        inc    edi         // .
        
        mov    [edi-2], ax // 4
        
        dec    ecx         // 5
        jnz    @x_loop     // .
        
        pop    edi
        pop    ecx
        add    edi, 320
        dec    ecx
        jnz    @y_loop
        
        pop    ecx
        mov    esi, offset buffer
        dec    ecx
        jnz    @outer
    }
}

void fxBlend();
#pragma aux fxBlend = " mov   esi, offset fxblendtab" \
                      " mov   ebx, offset fxbuffer  " \
                      " mov   edi, offset buffer    " \
                      " mov   ecx, 64000            " \
                      " xor   edx, edx              " \
                      " @inner:                     " \
                      " mov   dh,    [ebx]          " \
                      " inc   ebx                   " \
                      " mov   dl,    [edi]          " \
                      " mov   al,    [esi + edx]    " \
                      " mov   [edi], al             " \
                      " inc   edi                   " \
                      " dec   ecx                   " \
                      " jnz   @inner                " \
                      modify [esi edi ebx ecx edx];

void fxTimer() { fxtick++; }

int main () {
    int i, j, p=0;

    fxMakeSinTable();
    initpal();
    
    _asm {
        mov  ax, 13h
        int  10h
         
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

    
    outp(0x3C8, 0);
    for (i = 0; i < 256; i++) {
        outp(0x3C9, pal[(i << 2) + 2]);
        outp(0x3C9, pal[(i << 2) + 1]); 
        outp(0x3C9, pal[(i << 2)    ]);
    }
    
    
    fxMakeSprite();
    for (j = 0; j < 256; j++) for (i = 0; i < 256; i++) fxblendtab[p++] = sat(((i * 240) >> 8) + ((j * 32) >> 8), 255);
    //for (j = 0; j < 256; j++) for (i = 0; i < 256; i++) fxblendtab[p++] = sat(((i * 192) >> 8) + ((j * 96) >> 8), 255);
    //while (!kbhit()) {} getch();
    
    rtc_initTimer(4);
    rtc_setTimer(&fxTimer, rtc_timerRate / 60);
    //rtc_setTimer(&fxTimer, 137);    

    fxFillDots();
    
    i = 0;
    
    while (!kbhit()) {
    //while (i < 60*16) {
        i = fxtick;
        //i++;
        
        while ((inp(0x3DA) & 8) == 8) {}
        while ((inp(0x3DA) & 8) != 8) {}
        
        outp(0x3C8, 0); outp(0x3C9, 16); outp(0x3C9, 16);  outp(0x3C9, 16);   
        
        memcpy(screen, buffer, 64000);  
        fxResize();

        fx3dMovep(0,0,0);
        
        /*
        fx3dRotate(((i << 6) + (i << 7)) & (lut_size-1),
                   ((i << 5) + (i << 8)) & (lut_size-1),
                   0);
        */
        
        fx3dRotate((i << 8) & (lut_size-1), (i << 7) & (lut_size-1), (i << 6) & (lut_size-1));
        fx3dMove  ((stardist >> 1) * costabf[(i << 8) & (lut_size - 1)], 
                   (stardist >> 1) * sintabf[(i << 7) & (lut_size - 1)],
                   i < 300 ? (300 - i) : 0);
        /*
        fx3dMove  ((stardist >> 1) * costabf[(i << 8) & (lut_size - 1)], 
                   (stardist >> 1) * sintabf[(i << 7) & (lut_size - 1)],
                   (stardist << 0) * costabf[(i << 9) & (lut_size - 1)]);
        */
        fx3dMove  (0,0,-DIST);        

        outp(0x3C8, 0); outp(0x3C9, 32); outp(0x3C9, 16); outp(0x3C9, 0);
        fx3dProject();
        
        outp(0x3C8, 0); outp(0x3C9, 16); outp(0x3C9, 16); outp(0x3C9, 0);
        fxDrawPoints();
        
        outp(0x3C8, 0); outp(0x3C9, 16); outp(0x3C9, 32); outp(0x3C9, 0);
        fxBlend();
        
        outp(0x3C8, 0); outp(0x3C9, 0); outp(0x3C9, 0); outp(0x3C9, 0);
    }
    getch();
    rtc_freeTimer();

    _asm {
        mov  ax, 3
        int  10h
    }
    
    printf("timed %i timerticks in %i realticks = %f fps\n", fxtick, i, ((float)(60 * i) / fxtick));
}