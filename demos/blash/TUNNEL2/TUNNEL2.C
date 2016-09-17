#include <stdlib.h>
#include <conio.h>
#include <math.h>

unsigned char  *screen = 0xA0000;

unsigned char  ttexture[65536];
unsigned char  tbuffer[64000];
unsigned short tunnel[512*512];
unsigned char  tlightmap[512*512];
unsigned char  tpalxlat[65536];

         short tsintab [65536];
         float tsintabf[65536];

double pi = 3.141592653589793;

int sat(int a, int l) {return ((a > l) ? l : a); } 

void tbuildSinTable() {
    int i, j;
    float r;
    
    for (i = 0; i < 65536; i++) {
        r = (sin(2 * pi * i / 65536));
        tsintab[i] = 32767 * r;
        tsintabf[i] = r;
    }
}

void tbuildTunnel() {
    long x, y, i, u, v, lm;
    double r, a, l;
    
    const TunnelSize = 8192;
    const LightmapScale = 3;

    i = 0;
    for (y = -256; y < 256; y++) 
        for (x = -256; x < 256; x++) {
            r = sqrt(x*x + y*y);
            if (r < 1) r = 1;
            a = atan2(y, x) + pi;
            u = (a * 256 / pi);// + (tsintab[((int)(TunnelSize / (r)) <<2)&0xFF] >> 12);
            //v = TunnelSize / (r + (tsintab[(u<<10)&0xFFFF] >> 13));
            v = TunnelSize / (r);
            
            l = LightmapScale * r; lm = l;
            
            if (r < 5) {u = 0; v = 0; lm = 0;}
            
            //l = 3 * r; lm = l;
            tunnel[i] = ((u)&0xFF) | (((v)&0xFF)<<8);
            tlightmap[i] = sat(lm, 128);
            i++;
        }
    
}

void tbuildTexture() {
    int x, y, i, k=0;
    /*
    for (y = 0; y < 256; y++) {
        for (x = 0; x < 256; x++) {
            ttexture[((y << 8) + x)] = sat((x ^ y), 255) & 0xFF;
            //texture[((y << 8) + x)] = (x ^ y) & 0xFF;
            //ttexture[((y << 8) + x)] = (rand() % 0x100) & 0xFF;
        }
    }
    */
    for (i = 0; i < 2; i++)
    for (y = -128; y < 127; y++) {
        for (x = -128; x < 127; x++) {
            ttexture[k++] = sat((int)(0x80000 / ((x*x | y*y) + 0.0001)), 255) & 0xFF;
            //texture[((y << 8) + x)] = (x ^ y) & 0xFF;
            //ttexture[((y << 8) + x)] = (rand() % 0x100) & 0xFF;
        }
    }
    
    for (k = 0; k < 2; k++)
    for (i = 0; i < 65536; i++) 
        ttexture[i] = (ttexture[(i-1)&0xFFFF] + ttexture[(i+1)&0xFFFF] + 
                      ttexture[(i-256)&0xFFFF] + ttexture[(i+256)&0xFFFF]) >> 2; 
    
}

void tdrawTunnel (int c) {
    int u1 = tsintab[((c << 5) + (c << 8) + ((tsintab[(c << 10) & 0xFFFF]) >> 4)) & 0xFFFF] >> 8, v1 = (c << 2);
    int u2 = (96 * (tsintabf[(c << 5) & 0xFFFF])) * (tsintabf[((c << 8) + (c << 6)) & 0xFFFF]) + 96, v2 = (tsintab[((c << 6) + (c << 5) + 0x4000) & 0xFFFF] + 32768) / (65536/(512-200));
    int texofs1 = ((v1 << 8) + u1) &0x1FFFF;
    int texofs2 = ((v2 << 9) + u2) &0x3FFFF;
    int i = 0;
    long scrptr = (long)&tbuffer;
    int x = 0, y = 0, k = texofs2;
    
    for (y = 0; y < 200; y++) {
        for (x = 0; x < 320; x++) {
            //*((char*)scrptr++) = ttexture[(tunnel[k++]+texofs1) & 0xFFFF];
            *((char*)scrptr++) = tpalxlat[(ttexture[(tunnel[k]+texofs1) & 0xFFFF]) | (tlightmap[k++] << 8)];
            //*((char*)scrptr++) = (ttexture[(tunnel[k]+texofs1) & 0xFFFF] * tlightmap[k++]) >> 8;
            //*((char*)scrptr++) = (ttexture[(tunnel[k]+texofs1) & 0xFFFF] * tlightmap[k++]) >> 8;
            //*((char*)scrptr++) = (ttexture[(tunnel[k]+texofs1) & 0xFFFF] * tlightmap[k++]) >> 8;
            //k++;
            }
        k += (512-320);
    }
    
    //for (k = 0; k < 64000; k++)
        //*((char*)scrptr++) = (ttexture[(tunnel[k]+texofs1) & 0xFFFF] * tlightmap[k]) >> 8;
        //*((char*)scrptr++) = ttexture[(tunnel[k]+texofs1) & 0xFFFF];
        
        //*((char*)scrptr++) = palxlat[(texture[(tunnel[k]+texofs1) & 0xFFFF]) + (lightmap[k] << 8)];
        //*((char*)scrptr++) = (((texture[(tunnel[k]+texofs1) & 0xFFFF] + texture[(tunnel[k]+texofs2) & 0xFFFF]) >> 1) * lightmap[k]) >> 8;
        //*((char*)scrptr++) = palxlat[((texture[(tunnel[k]+texofs1) & 0xFFFF] +
        //                               texture[(tunnel[k]+texofs2) & 0xFFFF]) >> 1) | ((lightmap[k]) << 8)];
}

int main() {
    int i, j, p = 0;
    
    tbuildSinTable();
    tbuildTunnel();
    tbuildTexture();
    
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
    for (j = 0; j < 256; j++) for (i = 0; i < 256; i++) tpalxlat[p++] = (i * j) >> 8;
    
    while (!kbhit()) {
        i++;
        
        while ((inp(0x3DA) & 8) != 8) {}
        outp(0x3C8, 0); outp(0x3C9, 63);
        while ((inp(0x3DA) & 8) == 8) {}
        
        memcpy(screen, &tbuffer, 64000);
        
        tdrawTunnel(i);
        outp(0x3C8, 0); outp(0x3C9, 0);
    }
    getch();
}
