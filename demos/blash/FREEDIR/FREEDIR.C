#include <stdlib.h>
#include <conio.h>
#include <math.h>

unsigned char  *screen = 0xA0000;

unsigned char  fdtexture[65536];
unsigned char  fdbuffer[64000];
unsigned char  fdlightmap[64000];

unsigned short fdtable[64000];
//unsigned short fdgrid_u[41 * 26];
//unsigned short fdgrid_v[41 * 26];

         short fdsintab [65536];
         float fdsintabf[65536];

typedef struct vec3f {
    float x, y, z;
} vec3f;

#define sat(a, l) ((a > l) ? l : a)
#define sqr(a)    ((a)*(a))
#define sgn(a)    (((a) > 0) ? 1 : -1)

#define ee        1.0E-6 
#define pi        3.141592653589793

void fdNormalize(vec3f *v) {
	float l = 1.0 / sqrt(v->x*v->x + v->y*v->y + v->z*v->z);

	v->x *= l;
	v->y *= l;
	v->z *= l;
}

void fdInterpolate() {
    int x, y, z, i, j, k, u, v;
    int udx, udy, ux0, uy0, ux1, uv1;
    int vdx, vdy, vx0, vy0, vx1, vy1;
    int gridptr = 0, tableptr = 0; 

    for (j = 0; y < 40; y++) {
        for (i = 0; x < 25; x++) {
            uy0 = fdgrid_u[gridptr];      vy0 = fdgrid_v[gridptr];
            uy1 = fdgrid_u[gridptr + 41]; vy1 = fdgrid_v[gridptr + 41];
            udy = (uy1 - uy0) >> 3;       vdy = (vy1 - vy0) >> 3;
            
            for (y = 0; y < 8; y++) {
                ux0 = fdgrid_u[gridptr];     vx0 = fdgrid_v[gridptr];
                ux1 = fdgrid_u[gridptr + 1]; vx1 = fdgrid_v[gridptr + 1];
                udx = (ux1 - ux0) >> 3;      vdx = (vx1 - vx0) >> 3;
            }
        gridptr++;
        }
    }
}


void fdbuildSinTable() {
    int i, j;
    float r;
    
    for (i = 0; i < 65536; i++) {
        r = (sin(2 * pi * i / 65536));
        fdsintab[i] = 32767 * r;
        fdsintabf[i] = r;
    }
}


void fdbuildTexture() {
    int x, y, i, k=0;
    
    for (y = 0; y < 256; y++) {
        for (x = 0; x < 256; x++) {
            fdtexture[((y << 8) + x)] = sat((x ^ y), 255) & 0xFF;
            //texture[((y << 8) + x)] = (x ^ y) & 0xFF;
            //ttexture[((y << 8) + x)] = (rand() % 0x100) & 0xFF;
        }
    }
    
    /*
    for (i = 0; i < 2; i++)
    for (y = -128; y < 127; y++) {
        for (x = -128; x < 127; x++) {
            fdtexture[k++] = sat((int)(0x80000 / ((x*x | y*y) + ee)), 255) & 0xFF;
            //texture[((y << 8) + x)] = (x ^ y) & 0xFF;
            //ttexture[((y << 8) + x)] = (rand() % 0x100) & 0xFF;
        }
    }
    */
    
    // blur our texture
    for (k = 0; k < 2; k++)
    for (i = 0; i < 65536; i++) 
        fdtexture[i] = (fdtexture[(i-1)&0xFFFF] + fdtexture[(i+1)&0xFFFF] + 
                        fdtexture[(i-256)&0xFFFF] + fdtexture[(i+256)&0xFFFF]) >> 2; 
    
}

void fdCalcPlanes(int ax, int ay, int az) {

    const FOV = 120;
    const PlaneSize = 128;
    
    int x, y, z, tptr = 0;
    unsigned char u, v;
    
	vec3f origin, direction, intersect;
	float t, l;

    for (y = 0; y < 200; y++) {
        for (x = 0; x < 320; x++) {
        
            origin.x = ax;
            origin.y = ay;
            origin.z = az;
        
            direction.x = (float)(x - 160) / FOV;
            direction.y = (float)(y - 100) / FOV;
            direction.z = 1;
            
            // Normalize our Direction Vector
            fdNormalize(&direction);
            
            // Find t
            t = ((sgn(direction.y) * PlaneSize) - origin.y) / direction.y;

            // Calculate Intersect Point (O + D*t)
            intersect.x = origin.x + (direction.x * t);
            intersect.y = origin.y + (direction.y * t);
            intersect.z = origin.z + (direction.z * t);

            // Calculate Mapping Coordinates ( Coordiantes)
            u = (char)((intersect.x));
            v = (char)((intersect.z));
        
            fdtable[tptr] = (u << 8) + v;
            
            t = sqr(intersect.x - origin.x) + sqr(intersect.z - origin.z);
	        if (t <= ee) z = 0;
            else {
                t = (PlaneSize * 64) / sqrt(t);
                z = (char)sat(t, 255);
            }
            fdlightmap[tptr++] = z;
        }   
    }
}

void fdDrawPlanes() {
    int i, j, k;
    long scrptr = (long)&fdbuffer;
    /*
    for (i = 0; i < 64000; i++) {
        *((char*)scrptr++) = (fdtexture[fdtable[i]] * fdlightmap[i]) >> 8;
    }
    */
    _asm {
        xor    eax, eax
        xor    edx, edx
        mov    ecx, 64000
        mov    edi, offset fdbuffer
        mov    esi, offset fdtexture
        mov    ebx, offset fdtable
        mov    edx, offset fdlightmap

        @@inner:
        mov    ax, [ebx]
        mov    al, [esi + eax]
        mov    ah, [edx]
        mul    ah  // result in ax
        shr    eax, 8
        stosb
        add    ebx, 2
        inc    edx
        loop   @@inner        
    }
}

int main() {
    int i, j, p = 0;
    
    fdbuildSinTable();
    fdbuildTexture();
    
    _asm {
        mov  ax, 13h
        int  10h
    } 
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
    
    outp(0x3C8, 0);
    for (i = 0; i < 256; i++) {
        
        outp(0x3C9, sat(((i >> 3) + (i >> 4)), 63));
        outp(0x3C9, sat(((i >> 2) + (i >> 3)), 63)); 
        outp(0x3C9, sat(((i >> 2)), 63));
        /*
        outp(0x3C9, sat((i >> 2), 63));
        outp(0x3C9, sat((i >> 2), 63)); 
        outp(0x3C9, sat((i >> 2), 63));
        */    
    }
    //for (j = 0; j < 256; j++) for (i = 0; i < 256; i++) tpalxlat[p++] = (i * j) >> 8;
    
    while (!kbhit()) {
        i++;
        
        while ((inp(0x3DA) & 8) == 8) {}
        while ((inp(0x3DA) & 8) != 8) {}

        //outp(0x3C8, 0); outp(0x3C9, 63);
        
        _asm {
            mov  esi, offset fdbuffer
            mov  edi, screen
            mov  ecx, 16000
            rep  movsd  
        }
        
        fdCalcPlanes(i, 0, i);
        fdDrawPlanes();
        
        //outp(0x3C8, 0); outp(0x3C9, 0);
    }
    getch();
    
    _asm {
        mov  ax, 3
        int  10h
    }
}
