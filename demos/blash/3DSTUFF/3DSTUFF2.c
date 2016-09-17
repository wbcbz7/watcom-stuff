#include <math.h>
#include <strings.h>
#include <stdlib.h>
#include <conio.h>

typedef struct {
    float x, y, z, d;
} vertex;

typedef struct { float x, y; } vertex2d;

#define X_SIZE 320
#define Y_SIZE 200
#define DIST   300

#define stardist 128
#define count    1024
#define lut_size 65536
#define spr_size 16
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

short fxsintab[lut_size];
float fxsintabf[lut_size], fxcostabf[lut_size];

unsigned char  *screen = 0xA0000;
unsigned char fxbuffer[64320];
unsigned char fxrnd[64000];

void fxMakeSinTable () {
    int i, j;
    double r, lut_mul;
    lut_mul = (2 * pi / lut_size);
    for (i = 0; i < lut_size; i++) {
        r = i * lut_mul;
        fxsintab[i] = 32767 * r;
        fxsintabf[i] = sin(r);
        fxcostabf[i] = cos(r);
    }
}

void fxInitRnd() {
    int i, j;
    for (i = 0; i < 64000; i++) {
        fxrnd[i] = (rand() & 0xFF);
    }
}

void fx3dRotate (int ax, int ay, int az) {
    // hehehe, this code is fully ported from my old freebasic demoz ;)
    int i;
    float sinx = fxsintabf[ax], cosx = fxcostabf[ax];
    float siny = fxsintabf[ay], cosy = fxcostabf[ay];
    float sinz = fxsintabf[az], cosz = fxcostabf[az];
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
        
        if ((py<(200 - (spr_size)))&&(py>0)&&(px>0)&&(px<(320 - (spr_size)))&&(pt[i].z < 0)) {
            
            j = 0;
            scrptr = (long)&fxbuffer + ((py << 8) + (py << 6) + px);
            
            /*
            *((char*)scrptr) = sat((*((char*)scrptr) + 64), 255);
            *((char*)scrptr+1) = sat((*((char*)scrptr+1) + 32), 255);
            *((char*)scrptr-1) = sat((*((char*)scrptr-1) + 32), 255);
            */
            for (y = 0; y < (spr_size); y++) {
                for (x = 0; x < (spr_size); x++) {
                    *((char*)scrptr) = sat((*((char*)scrptr) + *((char*)sprptr)), 255);
                    scrptr++; sprptr++;
                }
                scrptr += (320 - (spr_size));
            }
            /*
            _asm {
                mov edi, scrptr
                mov eax, 0x20404020
                
                mov ecx, 2
                @outer:
                add  [edi], eax
                add  edi, 4
                add  edi, (X_SIZE - 4)
                loop @outer
            }
            */
        }
    }
}

void fxFillDots () {
    int rat, max, phi, teta, count2, i, lptr = 0;
    
    max = sqrt(count);
    rat = lut_size / max;
    /*
    for (teta = 0; teta < max; teta++)
        for (phi = 0; phi < max; phi++) {
        p[lptr].x = (stardist * fxsintabf[phi*rat]) * fxcostabf[teta*rat];
        p[lptr].y = stardist * fxcostabf[phi*rat];
        p[lptr++].z = (stardist * fxsintabf[phi*rat]) * fxsintabf[teta*rat];
    }
    */
    for (teta = 0; teta < max; teta++)
        for (phi = 0; phi < max; phi++) {
        p[lptr  ].x = ((stardist<<1) + (stardist * fxcostabf[phi*rat]))*fxcostabf[teta*rat];
   		p[lptr  ].y = ((stardist<<1) + (stardist * fxcostabf[phi*rat]))*fxsintabf[teta*rat];
	    p[lptr++].z = stardist * fxsintabf[phi*rat];
    }
    
        
}

void fxFillBuffer() {
    int i, j;
    
    for (i = 0; i < 512; i++) {
        j = ((rand() % 31680) << 1) + 320; 
        fxbuffer[j]   = sat(fxbuffer[j]   + 128, 255);
        fxbuffer[j-1] = sat(fxbuffer[j-1] + 64, 255);
        fxbuffer[j+1] = sat(fxbuffer[j+1] + 64, 255);
    }    
    for (i = 320; i < (64000); i++) {
        //fxbuffer[i] = (fxbuffer[i-1] + fxbuffer[i+1] + fxbuffer[i-320] + fxbuffer[i+320]) >> 2;
        fxbuffer[i] >>= 1;
    }
}

void fxMakeSprite() {
    int x, y, k;
    long sprptr = (long)&spr;
    
    for (y = -(spr_size >> 1); y < (spr_size >> 1); y++) {
        for (x = -(spr_size >> 1); x < (spr_size >> 1); x++) {
            *((char*)sprptr++) = sat((int)(0x100 / ((x*x + y*y) + ee)), 64) & 0xFF;
        }
    }
}

int main () {
    int i;

    fxMakeSinTable();
    
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
        outp(0x3C9, (i >> 3) + (i >> 4));
        outp(0x3C9, (i >> 2) - (i >> 5)); 
        outp(0x3C9, (i >> 3) + (i >> 4));
    }
    
    fxMakeSprite();
    fxFillDots();
    fxInitRnd();
    while (!kbhit()) {
        i++;
        
        while ((inp(0x3DA) & 8) != 8) {}
        outp(0x3C8, 0); outp(0x3C9, 63);   
        while ((inp(0x3DA) & 8) == 8) {}
        
        memcpy(screen, fxbuffer, 64000);   

        //fx3dMovep(0,0,0);
        
        //fx3dRotate((i << 9) & (lut_size - 1),lut_size >> 2,0);
        //fx3dRotate((i << 9) & (lut_size - 1),lut_size >> 2,lut_size >> 2);
        
        fx3dMovep(0,
                 (stardist<<1) * fxsintabf[(i << 9) & (lut_size - 1)],
                 (stardist<<1) * fxcostabf[(i << 9) & (lut_size - 1)]);
        
        //fx3dRotate(4096 * fxcostabf[(i << 8) & (lut_size - 1)] + (lut_size >> 1),
        //           4096 * fxsintabf[(i << 8) & (lut_size - 1)] + (lut_size >> 1),
        //           0);
        
        
        
        fx3dRotate(((i << 6) + (i << 7) ) & (lut_size-1),
                    0,
                   ((i << 5) + (i << 8)) & (lut_size-1));
        
        
        
        fx3dMove  (0,0,64 * fxcostabf[(i << 8) & (lut_size - 1)] + -DIST);
        
        fx3dProject();
        //memset(fxbuffer, 0, 64000);
        fxFillBuffer();
        fxDrawPoints();

        outp(0x3C8, 0); outp(0x3C9, 0);
    }
    getch();

}