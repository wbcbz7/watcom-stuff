unsigned char  t2_texture[65536];
unsigned char  t2_buffer[64000];

unsigned short t2_tunnel[512*512];
unsigned char  t2_lightmap[512*512];
unsigned char  t2_palxlat[65536];
unsigned char  t2_pal[1024];

void t2_buildTunnel() {
    long x, y, i, u, v, lm;
    double r, a, l;
    
    const TunnelSize = 8192;
    const LightmapScale = 3;

    i = 0;
    for (y = -256; y < 256; y++) 
        for (x = -256; x < 256; x++) {
            r = sqrt(x*x + y*y);
            if (r < 1) r = 1;
            a = atan2(y, x) + pi;
            u = (a * 256 / pi);
            v = TunnelSize / (r);
            l = LightmapScale * r; lm = l;
            if (r < 5) {u = 0; v = 0; lm = 0;}
            t2_tunnel[i] = ((u)&0xFF) | (((v)&0xFF)<<8);
            t2_lightmap[i] = sat(lm, 128);
            i++;
        }
    
}

void t2_buildTexture() {
    int x, y, i, k=0;

    for (i = 0; i < 2; i++)
    for (y = -128; y < 127; y++) {
        for (x = -128; x < 127; x++) {
            t2_texture[k++] = sat((int)(0x80000 / ((x*x | y*y) + ee)), 255) & 0xFF;
        }
    }
    
    for (k = 0; k < 2; k++)
    for (i = 0; i < 65536; i++) 
        t2_texture[i] = (t2_texture[(i-1)&0xFFFF] + t2_texture[(i+1)&0xFFFF] + 
                      t2_texture[(i-256)&0xFFFF] + t2_texture[(i+256)&0xFFFF]) >> 2; 
    
}

void t2_drawTunnel (int c) {
    int u1 = sintab[((c << 5) + (c << 8) + ((sintab[(c << 10) & 0xFFFF]) >> 4)) & 0xFFFF] >> 8, v1 = (c << 2);
    int u2 = (96 * (sintabf[(c << 5) & 0xFFFF])) * (sintabf[((c << 8) + (c << 6)) & 0xFFFF]) + 96, v2 = (sintab[((c << 6) + (c << 5) + 0x4000) & 0xFFFF] + 32768) / (65536/(512-200));
    int texofs1 = ((v1 << 8) + u1) &0x1FFFF;
    int texofs2 = ((v2 << 9) + u2) &0x3FFFF;
    int i = 0;
    long scrptr = (long)&t2_buffer;
    int x = 0, y = 0, k = texofs2;
    
    if (lowres == 0)
    for (y = 0; y < 200; y++) {
        for (x = 0; x < 320; x++) {
            *((char*)scrptr++) = t2_palxlat[(t2_texture[(t2_tunnel[k]+texofs1) & 0xFFFF]) | (t2_lightmap[k++] << 8)];
            //k++;
            }
        k += (512-320);
    } else if (fakelowres == 0)
    for (y = 0; y < 200; y++) {
        for (x = 0; x < 160; x++) {
            *((char*)scrptr++) = t2_palxlat[(t2_texture[(t2_tunnel[k]+texofs1) & 0xFFFF]) | (t2_lightmap[k++] << 8)];
            k++;
            }
        k += (512-320);
    }
    else
    for (y = 0; y < 200; y++) {
        for (x = 0; x < 160; x++) {
            *((char*)scrptr++) = t2_palxlat[(t2_texture[(t2_tunnel[k]+texofs1) & 0xFFFF]) | (t2_lightmap[k++] << 8)];
            *((char*)scrptr)   = *((char*)scrptr - 1);
            scrptr++;
            k++;
            }
        k += (512-320);
    }
}

void t2_initPal() {
    int i;

    for (i = 0; i < 256; i++) {
        t2_pal[(i<<2) + 0] = sat(((i >> 3) + (i >> 4)), 63);
        t2_pal[(i<<2) + 1] = sat(((i >> 2) + (i >> 3)), 63);
        t2_pal[(i<<2) + 2] = sat(((i >> 2)), 63);
    }
}

void t2_FadeInv(int scale) {
    int i, j;
    
    outp(0x3C8, 0);
    for (i = 0; i < 256; i++) {
        outp(0x3C9, (((63 - t2_pal[(i<<2) + 0]) * scale) >> 8) + (t2_pal[(i<<2) + 0] * (256 - scale) >> 8));
        outp(0x3C9, (((63 - t2_pal[(i<<2) + 1]) * scale) >> 8) + (t2_pal[(i<<2) + 1] * (256 - scale) >> 8));
        outp(0x3C9, (((63 - t2_pal[(i<<2) + 2]) * scale) >> 8) + (t2_pal[(i<<2) + 2] * (256 - scale) >> 8));
        inp (0x3C6);
    }
}

int t2_tick = 0, t2_inc = 0;

void t2_timer() {
    t2_tick++; t2_inc++;
}

void t2_init() {
    int i, j, p = 0;
    t2_buildTexture();
    t2_buildTunnel();
    t2_initPal();
    for (j = 0; j < 256; j++) for (i = 0; i < 256; i++) t2_palxlat[p++] = (i * j) >> 8;
}

void t2_main() {
    int i;
    int e = 256;
    
    if (notimer==0) {rtc_setTimer(&t2_timer, rtc_timerRate / 60);}
    
    while (Order < 9) {
        //i++;
        if (notimer==0) {i = t2_tick;} else {i++;}
        
        if (noretrace == 0 ) {
            while ((inp(0x3DA) & 8) == 8) {}
            while ((inp(0x3DA) & 8) != 8) {}
        }
        
        if (e > 0) {e -= (8 * t2_inc); e = (e < 0 ? 0 : e); t2_FadeInv(e);}
        if ((Order == 7) && (Row < 2)) {e = 256;}
        
        t2_inc = ((notimer == 0) ? 0 : 1);
        
        memcpy(screen, &t2_buffer, (((lowres == 1) && (fakelowres == 0)) ? 32000 : 64000));
        
        t2_drawTunnel(i);
        
        if (kbhit()) {if (getch() == 27) {KillAll();}}
    }
}
