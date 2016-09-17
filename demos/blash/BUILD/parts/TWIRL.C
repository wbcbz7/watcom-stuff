
unsigned char  w1_texture[65536];
unsigned char  w1_buffer[64000];
unsigned short w1_tunnel[64000];
unsigned char  w1_pal[1024];

void w1_buildTunnel() {
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
            v = (r) + (sintab[((int)r << 10) & 0xFFFF] >> 13);
            w1_tunnel[i++] = (u&0xFF) + ((v&0xFF)<<8);
        }
    
}

void w1_buildTexture() {
    int x, y, i, k=0;

    for (y = -128; y < 127; y++) {
        for (x = -128; x < 127; x++) {
            w1_texture[k++] = sat((int)(0x80000 / ((x*x | y*y) + 0.0001)), 255) & 0xFF;
        }
    }
    
    for (k = 0; k < 3; k++)
    for (i = 0; i < 65536; i++) 
        w1_texture[i] = (w1_texture[(i-1)&0xFFFF] + w1_texture[(i+1)&0xFFFF] + 
                      w1_texture[(i-256)&0xFFFF] + w1_texture[(i+256)&0xFFFF]) >> 2; 
    
}

void w1_drawTunnel (int c) {
    int u1 = (c << 0) + (sintab[((c << 8)) & 0xFFFF] >> 8) + (sintab[((c << 7)) & 0xFFFF] >> 9);
    int v1 = (c << 1) + (sintab[((c << 7)) & 0xFFFF] >> 10);
    
    int texofs1 = ((v1 << 8) + u1) &0xFFFF;
    int i = 0;
    long scrptr = (long)&w1_buffer;
    int k = 0;
    
    for (k = 0; k < 64000; k++)
        *((char*)scrptr++) = w1_texture[(w1_tunnel[k]+texofs1) & 0xFFFF];

}

void w1_initPal() {
    int i;
    
    for (i = 0; i < 256; i++) {
        w1_pal[(i << 2)    ] = sat(((sintab[(i << 9) & 0xFFFF] >> 13) + (i >> 2) + (i >> 3)), 63);
        w1_pal[(i << 2) + 1] = sat(((i >> 2) + (i >> 5)), 63); 
        w1_pal[(i << 2) + 2] = sat(((i >> 2) - (i >> 4)), 63);
    }
}

void w1_FadeIn(int scale) {
    int i, j;
    
    outp(0x3C8, 0);
    for (i = 0; i < 256; i++) {
        outp(0x3C9, ((63 * scale) >> 8) + (w1_pal[(i<<2) + 0] * (256 - scale) >> 8));
        outp(0x3C9, ((63 * scale) >> 8) + (w1_pal[(i<<2) + 1] * (256 - scale) >> 8));
        outp(0x3C9, ((63 * scale) >> 8) + (w1_pal[(i<<2) + 2] * (256 - scale) >> 8));
    }
}

void w1_FadeOut(int scale) {
    int i, j;
    
    outp(0x3C8, 0);
    for (i = 0; i < 256; i++) {
        outp(0x3C9, sat((w1_pal[(i << 2) + 0] * scale) >> 8, 63));
        outp(0x3C9, sat((w1_pal[(i << 2) + 1] * scale) >> 8, 63));
        outp(0x3C9, sat((w1_pal[(i << 2) + 2] * scale) >> 8, 63));
    }
}

int w1_tick = 0, w1_inc = 0;

void w1_timer() {
    w1_tick++; w1_inc++;
}

void w1_init() {
    w1_buildTexture();
    w1_buildTunnel();
    w1_initPal();
}

void w1_main() {
    int i;
    int e = 256;
    
    if (notimer==0) {rtc_setTimer(&w1_timer, rtc_timerRate / 60);}
    
    while (Order < 0xF) {
        //i++;
          
        if (notimer==0) {i = w1_tick;} else {i++;}
        
        if (noretrace == 0 ) {
            while ((inp(0x3DA) & 8) == 8) {}
            while ((inp(0x3DA) & 8) != 8) {}
        }
        
        if (e > 0) {e -= 16; e = (e < 0 ? 0 : e); w1_FadeIn(e);}
        w1_inc = ((notimer == 0) ? 0 : 1);
        
        memcpy(screen, &w1_buffer, 64000);
        
        w1_drawTunnel(i);
        
        if (kbhit()) {if (getch() == 27) {KillAll();}}
    }
}
