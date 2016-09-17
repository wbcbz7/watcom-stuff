#include <stdlib.h>
#include <conio.h>
#include <math.h>
#include <stdio.h>

unsigned char  *screen = 0xA0000;

unsigned char  buffer[64000];
unsigned short grid[41 * 26];

FILE *f;

#define sat(a, l) ((a > l) ? l : a)

void bFillGrid() {
    int x, y, i, j, k, gridptr = 0;
    
    for (y = 0; y < 26; y++) {
        for (x = 0; x < 41; x++) {
            grid[gridptr++] = (x % y) << 3;
        }
    }
}

void bDrawGrid() {
    int x, y, k, gridptr = 0;
    long scrptr = (long)&buffer;
    
    for (y = 0; y < 25; y++) {
        for (x = 0; x < 40; x++) {
            k = (y << 11) + (y << 9) + (x << 3);
            *((char*)scrptr+k) = (char)(grid[gridptr++]);
        }
        gridptr++;
    }
}

void bInterpolate() {
    int  x, y, i, j, k, gridptr = 0;
    long scrptr = (long)&buffer;
    int  sy0, sy1, sdy, ey0, ey1, edy, x0, x1, dx;
    int  sy, ey, sx;
    
    
    for (j = 0; j < 25; j++) {
        for (i = 0; i < 40; i++) {
            scrptr = (long)&buffer + (j << 11) + (j << 9) + (i << 3);
            
            sy0 = grid[gridptr];   sy1 = grid[gridptr+41];
            sdy = (sy1 - sy0) << 5;
            sy  = (grid[gridptr] << 8);
        
            ey0 = grid[gridptr+1]; ey1 = grid[gridptr+42];
            edy = (ey1 - ey0) << 5;
            ey  = (grid[gridptr+1] << 8);
            
            for (y = 0; y < 8; y++) {
                x0 = (sy >> 8); x1 = (ey >> 8);
                dx = (x1 - x0) << 5;
                sx = sy;
                for (x = 0; x < 8; x++) {
                    *((char*)scrptr++) = (sx >> 8);
                    sx += dx;
                }
                scrptr += (320 - 8);
                sy += sdy; ey += edy;
            }
            gridptr++;
        }
        gridptr++;
    }
}

int main() { 
    int i, j;
    
    bFillGrid();
    
    f = fopen("aux", "w");
    
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
    
    bInterpolate();
    
    while ((inp(0x3DA) & 8) != 8) {}
    while ((inp(0x3DA) & 8) == 8) {}    
    memcpy(screen, &buffer, 64000);
    while(!kbhit()) {} getch();
    
    _asm {
        mov  ax, 3
        int  10h
    } 
        
}
