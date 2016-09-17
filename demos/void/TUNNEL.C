unsigned char  ttexture[65536];
unsigned char  tbuffer[64000];
unsigned short tunnel[64000];
unsigned char  tlightmap[64000];
unsigned char  tpalxlat[65536];
unsigned char  tpal[1024];

         short tsintab[256];

void tbuildSinTable() {
    int i, j;
    float r;
    
    for (i = 0; i < 256; i++) {
        r = (32767 * sin(2 * pi * i / 256));
        tsintab[i] = r;
    }
}

void tbuildTunnel() {
    long x, y, i, u, v, lm;
    double r, a, l;
    
    const TunnelSize = 4096;
    const LightmapScale = 1;

    i = 0;
    for (y = -100; y < 100; y++) 
        for (x = -160; x < 160; x++) {
            r = sqrt(x*x + y*y);
            if (r < 1) r = 1;
            a = atan2(y, x) + pi;
            u = a * 128 / pi;
            v = TunnelSize / (r + (tsintab[(u<<2)&0xFF] >> 13));
            l = LightmapScale * r; lm = l;
            tunnel[i] = (u&0xFF) + ((v&0xFF)<<8);
            tlightmap[i] = sat(lm, 255);
            i++;
        }
    
}

void tbuildTexture() {
    unsigned int x, y, i, k;
    
    for (y = 0; y < 256; y++) {
        for (x = 0; x < 256; x++) {
            ttexture[((y << 8) + x)] = sat((x ^ y), 255) & 0xFF;
            //texture[((y << 8) + x)] = (x ^ y) & 0xFF;
            //texture[((y << 8) + x)] = (rand() % 0x100) & 0xFF;
        }
    }
    for (k = 0; k < 4; k++)
    for (i = 0; i < 65536; i++) 
        ttexture[i] = (ttexture[(i-1)&0xFFFF] + ttexture[(i+1)&0xFFFF] + 
                      ttexture[(i-256)&0xFFFF] + ttexture[(i+256)&0xFFFF]) >> 2; 
    
}

void tdrawTunnel (int c) {
    int u1 = tsintab[((c >> 1) + (c >> 3)) & 255] >> 8, v1 = (c << 2);
    int u2 = tsintab[(c >> 1) & 255] >> 8, v2 = (c << 1);
    int texofs1 = ((v1 << 8) + u1) &0xFFFF;
    int texofs2 = ((v2 << 8) + u2) &0xFFFF;
    int i = 0;
    long scrptr = (long)&tbuffer;
    int k = 0;
    
    for (k = 0; k < 64000; k++)
        *((char*)scrptr++) = (ttexture[(tunnel[k]+texofs1) & 0xFFFF] * tlightmap[k]) >> 8;
        //*((char*)scrptr++) = palxlat[(texture[(tunnel[k]+texofs1) & 0xFFFF]) + (lightmap[k] << 8)];
        //*((char*)scrptr++) = (((texture[(tunnel[k]+texofs1) & 0xFFFF] + texture[(tunnel[k]+texofs2) & 0xFFFF]) >> 1) * lightmap[k]) >> 8;
        //*((char*)scrptr++) = palxlat[((texture[(tunnel[k]+texofs1) & 0xFFFF] +
        //                               texture[(tunnel[k]+texofs2) & 0xFFFF]) >> 1) | ((lightmap[k]) << 8)];
}

void tInitPal() {
    int i;

    for (i = 0; i < 256; i++) {
        tpal[(i<<2) + 0] = (i >> 2);
        tpal[(i<<2) + 1] = (i >> 2);
        tpal[(i<<2) + 2] = (i >> 2);
    }
}

void tFadeIn(int scale) {
    int i, j;
    
    outp(0x3C8, 0);
    for (i = 0; i < 256; i++) {
        outp(0x3C9, ((63 * scale) >> 8) + (tpal[(i<<2) + 0] * (256 - scale) >> 8));
        outp(0x3C9, ((63 * scale) >> 8) + (tpal[(i<<2) + 1] * (256 - scale) >> 8));
        outp(0x3C9, ((63 * scale) >> 8) + (tpal[(i<<2) + 2] * (256 - scale) >> 8));
    }
}

void tInit() {
    tbuildSinTable();
    tbuildTunnel();
    tbuildTexture();
    tInitPal();
}

int  tTick = 0;

void tTimer() {
    tTick++;
}

int tMain() {
    int i, j, p = 0, e = 256;
    
    for (j = 0; j < 256; j++) for (i = 0; i < 256; i++) tpalxlat[p++] = (i * j) >> 8;
    
    if (notimer==0) {Timer_Start(&tTimer,TimerSpeed/60);} 
    
    while (Order < 0x18) {
        if (notimer==0) {i = tTick; } else {i++;}
        
        if (noretrace == 0) {
        while ((inp(0x3DA) & 8) == 8) {}
        while ((inp(0x3DA) & 8) != 8) {} }
        if (e > 0) {tFadeIn(e); e -= 8;}
        //outp(0x3C8, 0); outp(0x3C9, 63);
        
        _asm {
            mov  esi, offset tbuffer
            mov  edi, screen
            mov  ecx, 16000
            rep  movsd  
        }
        //memcpy(screen, &tbuffer, 64000);
        
        tdrawTunnel(i);
        //outp(0x3C8, 0); outp(0x3C9, 0);
    }
    if (notimer==0) {Timer_Stop(&tTimer);}
}
