
ptc_palette    int_pal[256], pal[256];

unsigned char  buffer[X_SIZE * Y_SIZE];
unsigned char  logo[320 * 64];

void initpal() {  
    int i;
    
    int_pal[0].r = int_pal[0].g = int_pal[0].b = 0;
    for (i = 1; i < 256; i++) {
        int_pal[i].r = sat(8  + (i >> 2) + (i >> 3), 63);
        int_pal[i].g = sat(10 + (i >> 1) + (i >> 3), 63); 
        int_pal[i].b = sat(12 + (i >> 1) + (i >> 3) + (i >> 4), 63);
    }
}

void drawbuf(int i) {
    unsigned int  k=0, c, l, x, y,col;
    unsigned char *buf = (unsigned char*)&buffer;
    unsigned char *spr = (unsigned char*)&logo;
    
    
    // fill upper gap
    _asm {
        cld
        mov    eax, 0x01010101
        mov    ecx, (X_SIZE * Y_SIZE) / 4
        mov    edi, buf
        rep    stosd
    }

    // write greets text
    
    l = X_SIZE * ((Y_SIZE - 64) / 2) + ((X_SIZE - 320) / 2);
    buf = (unsigned char*)&buffer[l];
    spr = (unsigned char*)&logo;
    
    _asm {
        cld
        mov    eax, 0x01010101
        mov    ecx, 64
        mov    esi, spr
        mov    edi, buf
        
        __outer:
        push   ecx
        mov    ecx, (320 / 4)
        
        __loop:
        mov    eax, [esi]
        add    esi, 4
        add    [edi], eax
        add    edi, 4
        dec    ecx
        jnz    __loop
        
        add    edi, (X_SIZE - 320)
        pop    ecx
        dec    ecx
        jnz    __outer
    }
    
    
    // fill random buffer lines
    for (k = 0; k < 20; k++) {
        c = (rand() & 0xF) * 0x01010101;
        l = ((rand() % (Y_SIZE - 1)) * X_SIZE);
        buf = (unsigned char*)&buffer[l];
        _asm {
            cld
            mov    eax, c
            mov    ecx, (X_SIZE / 4)
            mov    edi, buf
            
            __loop:
            add    [edi], eax
            add    edi, 4
            dec    ecx
            jnz    __loop            
        }
    }
}

volatile int tick = 0;
void timer() { tick++;}

void init() {
    FILE *f;
    
    initpal();
    
    f = fopen("logo.bin", "rb");
    fread(&logo, 1, sizeof(logo), f);
    fclose(f);
}

void fadein(int scale) {
    int i, j;
    
    for (i = 1; i < 256; i++) {
        pal[i].r = sat(((63 * scale) >> 8) + ((int_pal[i].r * (255 - scale)) >> 8), 63);
        pal[i].g = sat(((63 * scale) >> 8) + ((int_pal[i].g * (255 - scale)) >> 8), 63);
        pal[i].b = sat(((63 * scale) >> 8) + ((int_pal[i].b * (255 - scale)) >> 8), 63);
    }
}

void fadeout(int scale) {
    int i, j;
    
    for (i = 1; i < 256; i++) {
        pal[i].r = (int_pal[i].r * sat(scale, 255)) >> 8;
        pal[i].g = (int_pal[i].g * sat(scale, 255)) >> 8;
        pal[i].b = (int_pal[i].b * sat(scale, 255)) >> 8;
    }
}

void fadeshift(int scale) {
    int i, j;
    
    for (i = 1; i < 256; i++) {
        pal[i].r = sat((scale) + int_pal[i].r, 63);
        pal[i].g = sat((scale) + int_pal[i].g, 63);
        pal[i].b = sat((scale) + int_pal[i].b, 63);
    }
}

int main() {
    int i, j, k = 0;
    int oi, di = 0;
    
    int rawpos, row, order;
    int e = 224, g = 255, e_bound = 128;
    
    float mul;
    
    int rx = 0, ry = 0, rz = 0;
    float dx = 0, dy = 0, dz = 0;
    int rxdx, rydx, rzdx;
    
    srand(inp(0x40));
    
    fadein(255);
    ptc_setpal(&pal[0]);
    
    if (!notimer) {
        rtc_initTimer(rate);
        rtc_setTimer(&timer, rtc_timerRate / 60);
    }
    
    rawpos = xmpGetRealPos(); row = (rawpos >> 8) & 0xFF; order = (rawpos >> 16) & 0xFF;
    
    while ((order < 0x14) && (!kbhit())) {
        rawpos = xmpGetRealPos(); row = (rawpos >> 8) & 0xFF; order = (rawpos >> 16) & 0xFF;
        
        if (!notimer) {oi = i; _disable(); i = tick; _enable(); di = (i - oi); if (di == 0) di = 1;} else {i++; di = 1;}
        if (!noretrace) ptc_wait();
                       
        if (order == 0x12) {e = (e <= 0 ? 0 : e - di); fadein(e);}
        if (order == 0x13) {g = (g <= 0 ? 0 : g - di); fadeout(g);}
       
        ptc_setpal(&pal[0]);
        
        ptc_update(&buffer);
        if (mcpIdle) mcpIdle();

        drawbuf(i);
    }
    
    if (kbhit()) getch();
    if (!notimer) rtc_freeTimer();
    
    return 0;
}
