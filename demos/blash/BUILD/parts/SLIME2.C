#include <stdio.h>
#include <conio.h>
#include <math.h>

unsigned char *screen = 0xA0000;

unsigned char  pic[64000];
unsigned char  pal[1024];    //alignment

         short sintab[65536];

double pi = 3.141592653589793;

void MakeSinTable() {
    int i, j; double r;
    
    for (i = 0; i < 65536; i++) {
        r = sin(2*pi*i/65536); sintab[i] = 32767 * r;
    }
}

void LoadPicture() {
    int  i;
    char c;
    FILE *f;
    
    f = fopen("picture.img", "rb");
    for (i = 0; i < 64000; i++) {
        c = fgetc(f); pic[i] = c;
    }

    for (i = 0; i < 256; i++) {
        c = fgetc(f); pal[(i << 2)    ] = c;
        c = fgetc(f); pal[(i << 2) + 1] = c;
        c = fgetc(f); pal[(i << 2) + 2] = c;
    }
    fclose(f);
} 

int main() {
    int           i, y, d;
    unsigned char *screen_ptr, *pic_ptr;
    
    MakeSinTable();
    LoadPicture();
    
    _asm {
        mov ax, 13h
        int 10h
    }
    
    outp(0x3C8, 0);
    for (i = 0; i < 256; i++) {
        outp(0x3C9, pal[(i << 2)    ]);
        outp(0x3C9, pal[(i << 2) + 1]);
        outp(0x3C9, pal[(i << 2) + 2]);
    }
    
    while (!kbhit()) {
        i++;

        while ((inp(0x3DA) & 8) == 8) {}
        while ((inp(0x3DA) & 8) != 8) {}

        d = 0; screen_ptr = screen;
        for (y = 0; y < 200; y++) {
            d = y + (sintab[((y << 8) + (i << 8) + (i << 7)) & 0xFFFF] >> 9);
            //d = y + (sintable4p8[((sintable[(y + i) & 255] >> 1) + i + (y << 1)) & 255]);
            if (d >= 200) {d = 200 - abs(199 - d);}
            pic_ptr = &pic[abs((d << 8) + (d << 6)) % 64000];
            memcpy(screen_ptr, pic_ptr, 320);
            screen_ptr += 320;
        }
    }
    getch();
}
