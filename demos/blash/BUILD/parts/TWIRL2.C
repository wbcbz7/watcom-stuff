
unsigned char  w2_texture[65536];
unsigned char  w2_buffer[64000];
unsigned short w2_tunnel[64000];
unsigned char  w2_pal[1024];

void w2_buildTunnel() {
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
            v = (r);
            w2_tunnel[i++] = (u&0xFF) + ((v&0xFF)<<8);
        }
    
}
void w2_buildTexture() {
    unsigned int x, y, i, k;
    
    for (y = 0; y < 256; y++) {
        for (x = 0; x < 256; x++) {
            w2_texture[((y << 8) + x)] = sat((x ^ y), 255) & 0xFF;
        }
    }
    for (k = 0; k < 8; k++)
    for (i = 0; i < 65536; i++) 
        w2_texture[i] = (w2_texture[(i-1)&0xFFFF] + w2_texture[(i+1)&0xFFFF] + 
                      w2_texture[(i-256)&0xFFFF] + w2_texture[(i+256)&0xFFFF]) >> 2; 
    
}

void w2_drawTunnel (int c) {
    int u1 = sintab[((c << 7) + (c << 6)) & 0xFFFF] >> 8;
    int v1 = (c << 2) + (sintab[(c << 7) & 0xFFFF] >> 8);
    int u2 = sintab[(c << 8) & 0xFFFF] >> 8;
    int v2 = (c << 1);
    
    int texofs1 = ((v1 << 8) + u1) &0xFFFF;
    int texofs2 = ((v2 << 8) + u2) &0xFFFF;
    int i = 0;
    long scrptr = (long)&w2_buffer;
    int k = 0;
    
    for (k = 0; k < 64000; k++)
        *((char*)scrptr++) = (w2_texture[(w2_tunnel[k]+texofs1) & 0xFFFF] + w2_texture[(w2_tunnel[k]+texofs2) & 0xFFFF]) >> 1 ; 
}

void w2_initPal() {
    int i;
    
    for (i = 0; i < 256; i++) {
        w2_pal[(i << 2)    ] = (i >> 3) + (i >> 4);
        w2_pal[(i << 2) + 1] = (i >> 3) + (i >> 5); 
        w2_pal[(i << 2) + 2] = (i >> 2);
    }
}

void w2_FadeIn(int scale) {
    int i, j;
    
    outp(0x3C8, 0);
    for (i = 0; i < 256; i++) {
        outp(0x3C9, ((63 * scale) >> 8) + (w2_pal[(i<<2) + 0] * (256 - scale) >> 8));
        outp(0x3C9, ((63 * scale) >> 8) + (w2_pal[(i<<2) + 1] * (256 - scale) >> 8));
        outp(0x3C9, ((63 * scale) >> 8) + (w2_pal[(i<<2) + 2] * (256 - scale) >> 8));
    }
}

int w2_tick = 0, w2_inc = 0;

void w2_timer() {
    w2_tick++; w2_inc++;
}

void w2_init() {
    w2_buildTexture();
    w2_buildTunnel();
    w2_initPal();
}

void w2_main() {
    int i;
    int e = 256;
    
    if (notimer==0) {rtc_setTimer(&w2_timer, rtc_timerRate / 60);}
    
    while (Order < 0x11) {
        //i++;
        if (notimer==0) {i = w2_tick;} else {i++;}
          
        if (noretrace == 0 ) {
            while ((inp(0x3DA) & 8) == 8) {}
            while ((inp(0x3DA) & 8) != 8) {}
        }
        
        if (e > 0) {e -= 16; e = (e < 0 ? 0 : e); w2_FadeIn(e);}
        w2_inc = ((notimer == 0) ? 0 : 1);
        
        memcpy(screen, &w2_buffer, 64000);
        
        w2_drawTunnel(i);
        
        if (kbhit()) {if (getch() == 27) {KillAll();}}
    }
}
