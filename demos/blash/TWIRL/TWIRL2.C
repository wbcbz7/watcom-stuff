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
            v = (r) ;//+ (sintab[((int)r << 10) & 0xFFFF] >> 11); 
            tunnel[i++] = (u&0xFF) + ((v&0xFF)<<8);
        }
    
}
void buildTexture() {
    unsigned int x, y, i, k;
    
    for (y = 0; y < 256; y++) {
        for (x = 0; x < 256; x++) {
            texture[((y << 8) + x)] = sat((x ^ y), 255) & 0xFF;
            //texture[((y << 8) + x)] = (x ^ y) & 0xFF;
            //texture[((y << 8) + x)] = (rand() % 0x100) & 0xFF;
        }
    }
    for (k = 0; k < 8; k++)
    for (i = 0; i < 65536; i++) 
        texture[i] = (texture[(i-1)&0xFFFF] + texture[(i+1)&0xFFFF] + 
                      texture[(i-256)&0xFFFF] + texture[(i+256)&0xFFFF]) >> 2; 
    
}

void drawTunnel (int c) {
    int u1 = sintab[((c << 7) + (c << 6)) & 0xFFFF] >> 8;
    int v1 = (c << 2) + (sintab[(c << 7) & 0xFFFF] >> 8);
    int u2 = sintab[(c << 8) & 0xFFFF] >> 8;
    int v2 = (c << 1);
    
    int texofs1 = ((v1 << 8) + u1) &0xFFFF;
    int texofs2 = ((v2 << 8) + u2) &0xFFFF;
    int i = 0;
    long scrptr = (long)&buffer;
    int k = 0;
    
    for (k = 0; k < 64000; k++)
        *((char*)scrptr++) = (texture[(tunnel[k]+texofs1) & 0xFFFF] + texture[(tunnel[k]+texofs2) & 0xFFFF]) >> 1 ; 
}

void dump() {
    FILE *f;
    int i;
    
    f = fopen("texture.dmp", "wb");
    fwrite(&texture, sizeof(unsigned char), sizeof(texture), f);
    
    for (i = 0; i < 256; i++) {
        fputc((i >> 3) + (i >> 4), f);
        fputc((i >> 3) + (i >> 5), f); 
        fputc(i >> 2, f);
    }    
    fclose(f);
}

int main() {
    int i;
    
    buildSinTable();
    buildTunnel();
    buildTexture();
    
    _asm {
        mov  ax, 13h
        int  10h
    }
    
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
        outp(0x3C9, (i >> 3) + (i >> 4));
        outp(0x3C9, (i >> 3) + (i >> 5)); 
        outp(0x3C9, i >> 2);
    }
    
    while (!kbhit()) {
        i++;
        
        while ((inp(0x3DA) & 8) == 8) {}
        while ((inp(0x3DA) & 8) != 8) {}
        outp(0x3C8, 0); outp(0x3C9, 63); 
        
        
        memcpy(screen, &buffer, 64000);
        
        drawTunnel(i);
        
        outp(0x3C8, 0); outp(0x3C9, 0);
    }
    getch();
}
