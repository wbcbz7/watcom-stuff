unsigned char  t1_heap[8192];
unsigned char  t1_lightmap[64000];
unsigned char  t1_texture[65536];
unsigned char  t1_buffer[64000];
unsigned short t1_tunnel[64000];
unsigned char  t1_pal[1024];

void t1_buildTunnel() {
    long x, y, i, u, v, lm;
    double r, a, l;
    
    const TunnelSize = 6100;
    float LightmapScale = 1.1;

    i = 0;
    for (y = -100; y < 100; y++) 
        for (x = -160; x < 160; x++) {
            r = sqrt(x*x + y*y);
            if (r < 1) r = 1;
            a = atan2(y, x) + pi;
            u = (a * 128 / pi) + (sintab[((int)r << 7) & 0xFFFF] >> 11);
            v = TunnelSize / (r) ;//+ (sintab[((int)r << 10) & 0xFFFF] >> 11); 
            if (r < 16) {u = 0; v = 0; lm = 0;}
            t1_tunnel[i] = (u&0xFF) + ((v&0xFF)<<8);
            t1_lightmap[i++] = sat(LightmapScale * r, 255);
        }
    
}
void t1_buildTexture() {
    unsigned int x, y, i, k;
    
    for (y = 0; y < 256; y++) {
        for (x = 0; x < 256; x++) {
            t1_texture[((y << 8) + x)] = ((x ^ y) | (rand() % 0x100) & 0xFF);
        }
    }
    
    for (k = 0; k < 4; k++)
    for (i = 0; i < 65536; i++) 
        t1_texture[i] = (t1_texture[(i-1)&0xFFFF] + t1_texture[(i+1)&0xFFFF] + 
                      t1_texture[(i-256)&0xFFFF] + t1_texture[(i+256)&0xFFFF]) >> 2; 
    
}

void t1_drawTunnel (int c) {
    int u1 = (sintab[((c << 7) + (c << 6)) & 0xFFFF] >> 8) + (sintab[(c << 8) & 0xFFFF] >> 8);
    int v1 = (c << 2) + (sintab[(c << 7) & 0xFFFF] >> 8);

    
    int texofs1 = ((v1 << 8) + u1) &0xFFFF;

    int i = 0;
    long scrptr = (long)&t1_buffer;
    int k = 0;
    
    if (lowres == 0) 
    for (k = 0; k < 64000; k++)
        *((char*)scrptr++) = ((long)t1_texture[(t1_tunnel[k]+texofs1) & 0xFFFF] * (long)t1_lightmap[k]) >> 8; 
    else if (fakelowres == 0)  
    for (k = 0; k < 64000; k++) {
        *((char*)scrptr++) = (t1_texture[(t1_tunnel[k]+texofs1) & 0xFFFF] * t1_lightmap[k]) >> 8; k++;}
    else
    for (k = 0; k < 64000; k++) {
        *((char*)scrptr++) = (t1_texture[(t1_tunnel[k]+texofs1) & 0xFFFF] * t1_lightmap[k]) >> 8; *((char*)scrptr) = *((char*)scrptr - 1); scrptr++; k++;}
}

void t1_initPal() {
    int i;

    for (i = 0; i < 256; i++) {
        t1_pal[(i<<2) + 0] = sat(abs(64 - ((i >> 3) + (i >> 4))), 63);
        t1_pal[(i<<2) + 1] = sat(abs(64 - ((i >> 2) + (i >> 3))), 63);
        t1_pal[(i<<2) + 2] = sat(abs(64 - ((i >> 2))), 63);
    }
}

void t1FadeIn(int scale) {
    int i, j;
    
    outp(0x3C8, 0);
    for (i = 0; i < 255; i++) {
        while ((inp(0x3DA) & 1) != 1) {}
        //outp(0x3C8, i);
        outp(0x3C9, ((63 * scale) >> 8) + (t1_pal[(i<<2) + 0] * (256 - scale) >> 8));
        outp(0x3C9, ((63 * scale) >> 8) + (t1_pal[(i<<2) + 1] * (256 - scale) >> 8));
        outp(0x3C9, ((63 * scale) >> 8) + (t1_pal[(i<<2) + 2] * (256 - scale) >> 8));
    }
}

void t1FadeOut(int scale) {
    int i, j;
    
    outp(0x3C8, 0);
    for (i = 0; i < 255; i++) {
        while ((inp(0x3DA) & 1) != 1) {}
        //outp(0x3C8, i);
        outp(0x3C9, sat((t1_pal[(i<<2) + 0] * scale) >> 8, 63));
        outp(0x3C9, sat((t1_pal[(i<<2) + 1] * scale) >> 8, 63));
        outp(0x3C9, sat((t1_pal[(i<<2) + 2] * scale) >> 8, 63));
    }
}

int t1_tick = 0, t1_inc = 0;

void t1_timer() {
    t1_tick++; t1_inc++;
}

void t1_init() {
    t1_initPal();
    t1_buildTunnel();
    t1_buildTexture();
}

void t1_main() {
    int i;
    int e = 0;
    
    if (notimer==0) {rtc_setTimer(&t1_timer, rtc_timerRate / 60);}
    
    outp(0x3C8, 0xFF); outp(0x3C9, 0); outp(0x3C9, 0); outp(0x3C9, 0);

    while (Order < 5) {
        //i++;
        if (notimer==0) {i = t1_tick;} else {i++;}
        
        if (noretrace == 0 ) {
            while ((inp(0x3DA) & 8) == 8) {}
            while ((inp(0x3DA) & 8) != 8) {}
        }
        //outp(0x3C8, 0); outp(0x3C9, 63);

        if ((Order == 0) && (e < 256)) {e += (4 * t1_inc); e = (e < 0 ? 0 : e); t1FadeOut(e);}
        if ((Order == 4) && (e > 0)) {e -= (2 * t1_inc); e = (e < 0 ? 0 : e); t1FadeOut(e);} 
        
        t1_inc = ((notimer == 0) ? 0 : 1);
          
        memcpy(screen, &t1_buffer, (((lowres == 1) && (fakelowres == 0)) ? 32000 : 64000));

        t1_drawTunnel(i);

        //outp(0x3C8, 0); outp(0x3C9, 0);
        if (kbhit()) {if (getch() == 27) {KillAll();}}
    }
}
