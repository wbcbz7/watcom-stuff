#include <stdlib.h>
#include <stdio.h>
#include <conio.h>
#include <math.h>

unsigned char  *screen = 0xA0000;

unsigned char  texture[65536];
unsigned char  buffer[64000];


unsigned short tunnel[64000];

         short sintab[65536];
         float sintabf[65536];

double pi = 3.141592653589793;

#define sat(a, l) ((a > l) ? l : a)

void buildSinTable() {
    int i, j;
    float r;
    
    for (i = 0; i < 65536; i++) {
        r = (sin(2 * pi * i / 65536));
        sintab[i] = 32767 * r;
        sintabf[i] = r;
    }
}

void buildTunnel() {
    long x, y, i, u, v, lm;
    double r, a, l;
    
    const TunnelSize = 4096;
    const LightmapScale = 1.0;

    i = 0;
    for (y = -100; y < 100; y++) 
        for (x = -160; x < 160; x++) {
            r = sqrt(x*x + y*y);
            if (r < 1) r = 1;
            a = atan2(y, x) + pi;
            u = (a * 128 / pi) + (sintab[((int)r << 9) & 0xFFFF] >> 10);
            v = (r) + (sintab[((int)r << 10) & 0xFFFF] >> 11); 
            tunnel[i++] = (u&0xFF) + ((v&0xFF)<<8);
        }
    
}

void buildTexture() {
    int x, y, i, k=0;
    
    /*
    for (y = 0; y < 256; y++) {
        for (x = 0; x < 256; x++) {
            texture[k++] = sat((x ^ y), 255) & 0xFF;
            //texture[((y << 8) + x)] = (x ^ y) & 0xFF;
            //texture[((y << 8) + x)] = (rand() % 0x100) & 0xFF;
        }
    }
    */

    for (y = -128; y < 127; y++) {
        for (x = -128; x < 127; x++) {
            texture[k++] = sat((int)(0x80000 / ((x*x + y*y) + 0.0001)), 255) & 0xFF;
            //texture[((y << 8) + x)] = (x ^ y) & 0xFF;
            //ttexture[((y << 8) + x)] = (rand() % 0x100) & 0xFF;
        }
    }
    
    for (k = 0; k < 3; k++)
    for (i = 0; i < 65536; i++) 
        texture[i] = (texture[(i-1)&0xFFFF] + texture[(i+1)&0xFFFF] + 
                      texture[(i-256)&0xFFFF] + texture[(i+256)&0xFFFF]) >> 2; 
    
}

void drawTunnel (int c) {
    int u1 = (c << 1) - (sintab[((c << 9)) & 0xFFFF] >> 8) + (sintab[((c << 8)) & 0xFFFF] >> 9);
    int v1 = (c << 2) + (sintab[((c << 8)) & 0xFFFF] >> 10);
    
    //int u2 = sintab[(c << 5) & 0xFFFF] >> 8;
    //int v2 = sintab[(c << 3) & 0xFFFF] >> 9;
    int texofs1 = ((v1 << 8) + u1) &0xFFFF;
    //int texofs2 = ((v2 << 8) + u2) &0xFFFF;
    int i = 0;
    long scrptr = (long)&buffer;
    int k = 0;
    
    for (k = 0; k < 64000; k++)
        *((char*)scrptr++) = texture[(tunnel[k]+texofs1) & 0xFFFF];
    /*    
    _asm {
        xor    eax, eax
        xor    edx, edx
        mov    ecx, 64000
        mov    edi, offset buffer
        mov    esi, offset texture
        mov    ebx, offset tunnel
        mov    edx, texofs1

        @@inner:
        mov    ax, [ebx]
        add    eax, edx
        and    eax, 0xFFFF
        mov    al, [esi + eax]
        stosb
        add    ebx, 2
        loop   @@inner        
    }
    */
}

void dump() {
    FILE *f;
    int i;
    
    f = fopen("texture.dmp", "wb");
    fwrite(&texture, sizeof(unsigned char), sizeof(texture), f);
    
    for (i = 0; i < 256; i++) {
        fputc(sat(((sintab[(i << 9) & 0xFFFF] >> 13) + (i >> 2) + (i >> 4)), 63), f);
        fputc(sat(((i >> 3) + (i >> 5)), 63), f); 
        fputc(sat(((i >> 2) - (i >> 4)), 63), f);
    }    
    fclose(f);
}

int main() {
    int i;
    unsigned short *lutptr;
    
    buildSinTable();
    buildTunnel();
    buildTexture();
    
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
        
        outp(0x3C9, (i >> 2));
        outp(0x3C9, (i >> 2)); 
        outp(0x3C9, (i >> 2));
    }
    for (i = 0; i < 64000; i++) {
        *((char*)screen+i) = (char)(tunnel[i] >> 8);
    }
    while (!kbhit()) {} getch();
    
    for (i = 0; i < 64000; i++) {
        *((char*)screen+i) = (char)(tunnel[i] & 0xFF);
    }
    while (!kbhit()) {} getch();
    
    outpw(0x3D4, 0x2013);
    memcpy(screen, texture, 65536);
    while (!kbhit()) {} getch();
    outpw(0x3D4, 0x2813);
    
    dump();
    
    outp(0x3C8, 0);
    for (i = 0; i < 256; i++) {
        outp(0x3C9, sat(((sintab[(i << 9) & 0xFFFF] >> 13) + (i >> 2) + (i >> 4)), 63));
        outp(0x3C9, sat(((i >> 3) + (i >> 5)), 63)); 
        outp(0x3C9, sat(((i >> 2) - (i >> 4)), 63));
    }
    
    while (!kbhit()) {
        i++;
        
        while ((inp(0x3DA) & 8) != 8) {}
        outp(0x3C8, 0); outp(0x3C9, 63);   
        while ((inp(0x3DA) & 8) == 8) {}
        
        memcpy(screen, buffer, 64000);
        
        drawTunnel(i);
        
        
        outp(0x3C8, 0); outp(0x3C9, 0);
    }
    getch();
}
