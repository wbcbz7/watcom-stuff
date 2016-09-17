unsigned char  hp_pic[64000];
unsigned char  hp_pal[1024];    //alignment

void hp_LoadPicture() {
    int  i;
    char c;
    FILE *f;
    
    f = fopen("musik.xm", "rb");
    fseek(f, 0x71EC5, SEEK_SET); 
    for (i = 0; i < 64000; i++) {
        c = fgetc(f); hp_pic[i] = c;
    }

    for (i = 0; i < 256; i++) {
        c = fgetc(f); hp_pal[(i << 2)    ] = c;
        c = fgetc(f); hp_pal[(i << 2) + 1] = c;
        c = fgetc(f); hp_pal[(i << 2) + 2] = c;
    }
    fclose(f);
} 

int hp_main() {
    int           i, y, d;
    unsigned char *screen_ptr, *pic_ptr;
    
    hp_LoadPicture();
    setvidmode(0x13);
    
    outp(0x3C8, 0);
    for (i = 0; i < 256; i++) {
        outp(0x3C9, hp_pal[(i << 2)    ]);
        outp(0x3C9, hp_pal[(i << 2) + 1]);
        outp(0x3C9, hp_pal[(i << 2) + 2]);
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
            pic_ptr = &hp_pic[abs((d << 8) + (d << 6)) % 64000];
            memcpy(screen_ptr, pic_ptr, 320);
            screen_ptr += 320;
        }
    }
    getch(); setvidmode(3);
}
