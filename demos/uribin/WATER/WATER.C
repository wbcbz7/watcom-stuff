// water in 640x480x8bpp VESA mode
// by...guess who :) okay, by wbcbz7 15.o3.2o16
#include <stdlib.h>
#include <conio.h>
#include <math.h>
#include "vbe.c" 

#define SCREEN_X  640
#define SCREEN_Y  480

#define pi        3.141592653589793
#define sat(a, l) ((a > l) ? l : a)

unsigned char  *screen;

unsigned char  buffer1[SCREEN_X * SCREEN_Y], buffer2[SCREEN_X * SCREEN_Y];
unsigned char  *pbuf1 = (unsigned char*)&buffer1, *pbuf2 = (unsigned char*)&buffer2, *pbuf3;

         float sintab[65536], costab[65536];

vbe_VbeInfoBlock  vbeinfo;
vbe_ModeInfoBlock modeinfo;
         
void drop(int x, int y) {
    int ptr;
    
    ptr = (y * SCREEN_X) + x;
    pbuf1[ptr] = 255;
}

void buildSinTable() {
    int i, j;
    float r;
    
    for (i = 0; i < 65536; i++) {
        r = (sin(2 * pi * i / 65536));
        sintab[i] = r;
        r = (cos(2 * pi * i / 65536));
        costab[i] = r;
    }
}

void water();
#pragma aux water = " mov   esi, pbuf1                                  " \
                    " mov   edi, pbuf2                                  " \
                    " add   esi, 640 + 1                           " \
                    " add   edi, 640 + 1                           " \
                    " mov   ecx, (640*480) - ((640*2)+2) " \
                    " xor   eax, eax                                    " \
                    " xor   ebx, ebx                                    " \
                    " @inner:                                           " \
                    " mov   al,  [esi - 640]                       " \
                    " mov   bl,  [esi + 640]                       " \
                    " add   eax, ebx                                    " \
                    " mov   bl,  [esi - 1]                              " \
                    " add   eax, ebx                                    " \
                    " mov   bl,  [esi + 1]                              " \
                    " add   eax, ebx                                    " \
                    " shr   eax, 1                                      " \
                    " mov   bl,  [edi]                                  " \
                    " sub   eax, ebx                                    " \
                    " jnc   @skip                                       " \
                    " xor   eax, eax                                    " \
                    " @skip:                                            " \
                    " mov   [edi], al                                   " \
                    " inc   esi                                         " \
                    " inc   edi                                         " \
                    " dec   ecx                                         " \
                    " jnz   @inner                                      " \
                    modify [esi edi eax ebx ecx];


// 8-pixel version
/*
void water();
#pragma aux water = " mov   esi, pbuf1       " \
                    " mov   edi, pbuf2       " \
                    " add   esi, 641         " \
                    " add   edi, 641         " \
                    " mov   ecx, 64000-1281  " \
                    " xor   eax, eax         " \
                    " xor   ebx, ebx         " \
                    " @inner:                " \
                    " mov   al,  [esi - 320] " \
                    " mov   bl,  [esi + 320] " \
                    " add   eax, ebx         " \
                    " mov   bl,  [esi - 1]   " \
                    " add   eax, ebx         " \
                    " mov   bl,  [esi + 1]   " \
                    " add   eax, ebx         " \
                    " mov   bl,  [esi - 319] " \
                    " add   eax, ebx         " \
                    " mov   bl,  [esi + 319] " \
                    " add   eax, ebx         " \
                    " mov   bl,  [esi - 321] " \
                    " add   eax, ebx         " \
                    " mov   bl,  [esi + 321] " \
                    " add   eax, ebx         " \
                    " shr   eax, 3           " \
                    " mov   bl,  [edi]       " \
                    " sub   eax, ebx         " \
                    " jnc   @skip            " \
                    " xor   eax, eax         " \
                    " @skip:                 " \
                    " mov   [edi], al        " \
                    " inc   esi              " \
                    " inc   edi              " \
                    " dec   ecx              " \
                    " jnz   @inner           " \
                    modify [esi edi eax ebx ecx];


void water() {
    int i, j, k;
    
    for (i = 320; i < (64000-320); i++) {
        
        k = (pbuf1[i - 1] + pbuf1[i + 1] + pbuf1[i-320] + pbuf1[i+320]) >> 1;
        k -= pbuf2[i]; if (k < 0) k = 0;
        pbuf2[i] = k;        
    }
}
*/

int main() {
    int i, j, mode, p = 0;   

    buildSinTable();
    
    if (vbe_Init()) { puts("can't init vbe interface \n"); return 1; }
    
    mode = vbe_FindMode(SCREEN_X, SCREEN_Y, 8, vbe_ATTR_LFB_Support);
    if (mode == -1) {printf("can't find %ix%ix8bpp LFB mode", SCREEN_X, SCREEN_Y); vbe_Done(); return 1;}
    
    vbe_ModeInfo(mode, &modeinfo);
    if (vbe_status != 0x4f) {printf("can't get info for mode 0x%x", mode); vbe_Done(); return 1;}
    vbe_SetMode(mode | vbe_MODE_LINEAR);
    if (vbe_status != 0x4f) {printf("can't set mode 0x%x", mode); vbe_Done(); return 1;}
    
    screen = (unsigned char*)vbe_GetVideoPtr();
    if (screen == NULL) {vbe_SetMode(0x3);  puts("can't get pointer to framebuffer (fuck NTVDM!)"); vbe_Done(); return 1;}
    
    // assume that controller is VGA compatible!
    
    outp(0x3C8, 0);
    for (i = 0; i < 256; i++) {
        outp(0x3C9, sat(8  + ((i >> 4) + (i >> 4)), 63));
        outp(0x3C9, sat(12 + ((i >> 3) + (i >> 4)), 63));
        outp(0x3C9, sat(24 + ((i >> 2) + (i >> 3)), 63));  
    }

    while (!kbhit()) {} getch();
    
    while (!kbhit()) {
        i++; 
        
        while ((inp(0x3DA) & 8) == 8) {}
        while ((inp(0x3DA) & 8) != 8) {}

        outp(0x3C8, 0); outp(0x3C9, 63); outp(0x3C9, 0); outp(0x3C9, 0);
        
        memcpy(screen, pbuf2, SCREEN_X * SCREEN_Y); // no pitch correction
        
        outp(0x3C8, 0); outp(0x3C9, 63); outp(0x3C9, 63); outp(0x3C9, 0);
        
        if (pbuf1 == &buffer1) drop((rand() % (SCREEN_X - 2)) + 1, (rand() % (SCREEN_Y - 2)) + 1);
        drop(SCREEN_X/2 + (SCREEN_X * 0.45f) * sintab[(i << 8) & 0xFFFF], SCREEN_Y/2 + (SCREEN_Y/2 - 1) * sintab[(i << 9) & 0xFFFF]);
        drop(SCREEN_X/2 + (SCREEN_X * 0.45f) * costab[(i << 7) & 0xFFFF], SCREEN_Y/2 + (SCREEN_Y/2 - 1) * sintab[(i << 8) & 0xFFFF]);
        
        water();
        outp(0x3C8, 0); outp(0x3C9, 8); outp(0x3C9, 12); outp(0x3C9, 24); 
        
        pbuf3 = pbuf1; pbuf1 = pbuf2; pbuf2 = pbuf3;
    }
    getch();

    vbe_SetMode(0x3);
    vbe_Done();
}