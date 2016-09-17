#include <stdlib.h>
#include <conio.h>
#include <math.h>

unsigned char  *screen = 0xA0000;

unsigned char  buffer[64000];
unsigned char  grid[41 * 26];

#define sat(a, l) ((a > l) ? l : a)

void bFillGrid() {
    int x, y, i, j, k, gridptr = 0;
    
    for (y = 0; y < 26; y++) {
        for (x = 0; x < 41; x++) {
            grid[gridptr++] = x * y;
        }
    }
}

void bDrawGrid() {
    int x, y, k, gridptr = 0;
    long scrptr = (long)&buffer;
    
    for (y = 0; y < 25; y++) {
        for (x = 0; x < 41; x++) {
            k = (y << 11) + (y << 9) + (x << 3);
            *((char*)scrptr+k) = grid[gridptr++];
        }
    }
}

void bInterpolateVertical() {
    int  x, y, i, j, k, gridptr = 0;
    long scrptr = (long)&buffer;
    int  y0, y1, dy;
    
    for (j = 0; j < 25; j++) {
        for (i = 0; i < 40; i++) {
            y0 = grid[gridptr]; y1 = grid[gridptr + 41];
            dy = (y1 - y0) << 5 ;
            y = (y0 << 8);
            for (k = 0; k < 8; k++) {
                *((char*)scrptr) = (y >> 8);
                scrptr += 320;
                y += dy;
            }
            gridptr++; scrptr -= ((320 * 8) - 8);
        }
        scrptr += (320 * 7); gridptr++;
    }
}

void bInterpolateHoriontal() {
    int  x, y, i, j, k;
    long scrptr = (long)&buffer;
    int  x0, x1, dx;
    
    for (j = 0; j < 200; j++) {
        for (i = 0; i < 40; i++) {
            x0 = *((char*)scrptr); x1 = *((char*)(scrptr + 8));
            dx = (x1 - x0) << 5 ;
            x = (x0 << 8);
            for (k = 0; k < 8; k++) {
                *((char*)scrptr++) = (x >> 8);
                x += dx;
            }
        }
    }
}

int main() { 
    int i, j;
    
    bFillGrid();
    
    _asm {
        mov  ax, 13h
        int  10h
    } 
    
    outp(0x3C8, 0);
    for (i = 0; i < 256; i++) {
        /*
        outp(0x3C9, sat(((i >> 3) + (i >> 4)), 63));
        outp(0x3C9, sat(((i >> 2) + (i >> 3)), 63)); 
        outp(0x3C9, sat(((i >> 2)), 63));
        */
        outp(0x3C9, sat((i >> 2), 63));
        outp(0x3C9, sat((i >> 2), 63)); 
        outp(0x3C9, sat((i >> 2), 63));
            
    }
    
    bDrawGrid(); 
    
    while ((inp(0x3DA) & 8) != 8) {}
    while ((inp(0x3DA) & 8) == 8) {}    
    memcpy(screen, &buffer, 64000);
    while(!kbhit()) {} getch();
    
    bInterpolateVertical();
    
    while ((inp(0x3DA) & 8) != 8) {}
    while ((inp(0x3DA) & 8) == 8) {}    
    memcpy(screen, &buffer, 64000);
    while(!kbhit()) {} getch();
    
    bInterpolateHoriontal();
    
    while ((inp(0x3DA) & 8) != 8) {}
    while ((inp(0x3DA) & 8) == 8) {}    
    memcpy(screen, &buffer, 64000);
    while(!kbhit()) {} getch();
    
    _asm {
        mov  ax, 3
        int  10h
    } 
        
}
