unsigned char  ltexture[65536];
unsigned char  lbuffer[64000];
unsigned short lens[64000];
unsigned char  lpal[1024];

         short lsintab[256];

void lbuildSinTable() {
    int i, j;
    float r;
    
    for (i = 0; i < 256; i++) {
        r = (32767 * sin(2 * pi * i / 256));
        lsintab[i] = r;
    }
}

void lbuildTunnel() {
    long x, y, i, u, v, d, m, dr, t;
    double r, a, b, s, z, tx, ty;
    
    i = 0;
    d = 160; m = 60; dr = d >> 1; t = 16;
    s = sqrt(dr*dr - m*m);
    for (y = -100; y < 100; y++) 
        for (x = -160; x < 160; x++) {
            tx = x; ty = y;
            z = sqrt(abs(dr*dr - tx*tx - ty*ty)) + t;
            a = ((tx * m) / z + 0.5);
            b = ((ty * m) / z + 0.5);
            u = (a + dr); v = (b + dr);
            lens[i++] = (u&0xFF) + ((v&0xFF)<<8);
        }
    
}

void lbuildTexture() {
    unsigned int x, y, i, k;
    
    for (y = 0; y < 256; y++) {
        for (x = 0; x < 256; x++) {
            ltexture[((y << 8) + x)] = sat((x ^ y), 255) & 0xFF;
            //texture[((y << 8) + x)] = (x ^ y) & 0xFF;
            //texture[((y << 8) + x)] = (rand() % 0x100) & 0xFF;
        }
    }
    for (k = 0; k < 4; k++)
    for (i = 0; i < 65536; i++) 
        ltexture[i] = (ltexture[(i-1)&0xFFFF] + ltexture[(i+1)&0xFFFF] + 
                      ltexture[(i-256)&0xFFFF] + ltexture[(i+256)&0xFFFF]) >> 2; 
    
}

void ldrawLens (int c) {
    int u1 = lsintab[((c) + (c >> 1)) & 255] >> 8;
    int v1 = lsintab[((c) + (lsintab[(c >> 2) & 255] >>10 )) & 255] >> 8;
    int u2 = lsintab[((c) + (c >> 1) + 16) & 255] >> 8;
    int v2 = lsintab[((c) + (lsintab[(c >> 2) & 255] >>10 ) + 16) & 255] >> 8;

    int texofs1 = ((v1 << 8) + u1) &0xFFFF;
    int texofs2 = ((v2 << 8) + u2) &0xFFFF;
    int i = 0;
    long scrptr = (long)&lbuffer;
    int k = 0;
    
    for (k = 0; k < 64000; k++)
        //*((char*)scrptr++) = texture[(lens[k]+texofs1) & 0xFFFF];
        //*((char*)scrptr++) = palxlat[(texture[(tunnel[k]+texofs1) & 0xFFFF]) + (lightmap[k] << 8)];
        *((char*)scrptr++) = (ltexture[(lens[k]+texofs1) & 0xFFFF] + ltexture[(lens[k]+texofs2) & 0xFFFF]) >> 1;
        //*((char*)scrptr++) = palxlat[((texture[(tunnel[k]+texofs1) & 0xFFFF] +
        //                               texture[(tunnel[k]+texofs2) & 0xFFFF]) >> 1) | ((lightmap[k]) << 8)];
}

void lInitPal() {
    int i;

    for (i = 0; i < 256; i++) {
        lpal[(i<<2) + 0] = (i >> 2);
        lpal[(i<<2) + 1] = (i >> 2);
        lpal[(i<<2) + 2] = (i >> 2);
    }
}

void lFadeIn(int scale) {
    int i, j;
    
    outp(0x3C8, 0);
    for (i = 0; i < 256; i++) {
        outp(0x3C9, sat((lpal[(i<<2) + 0] * scale) >> 8, 63));
        outp(0x3C9, sat((lpal[(i<<2) + 1] * scale) >> 8, 63));
        outp(0x3C9, sat((lpal[(i<<2) + 2] * scale) >> 8, 63));
    }
}

void lShake(int offset) {
    // shake it, baby! ;)
    outp(0x3D4, 0xC); outp(0x3D5, (offset >> 8) & 0xFF);
    outp(0x3D4, 0xD); outp(0x3D5,  offset& 0xFF);
}

int  lTick = 0;

void lTimer() {
    lTick++;
}

void lInit() {
    lbuildSinTable();
    lbuildTunnel();
    lbuildTexture();
    lInitPal();
}

int lMain() {
    int i, j, p = 2176;
    int currow, oldrow;
    
    if (notimer==0) {Timer_Start(&lTimer,TimerSpeed/60); lTick = i;}
    while (Order < 0x10) {
        
        if (notimer==0) {i = lTick;} else {i++;}
        
        if (noretrace == 0) {
        while ((inp(0x3DA) & 8) == 8) {}
        while ((inp(0x3DA) & 8) != 8) {} }
        
        if (p > 256) {p -= 64; lFadeIn(p);}
        if ((Order == 0xF)&&(p > 0)) {lFadeIn(p); p -= 2;}
        currow = Row >> 1;
        if ((ChInstrument[8] == 1)&&(oldrow != currow)) {lShake(((rand() & 1) == 1) ? (65536 - ((rand() & 3) * 80)) : ((rand() & 3) * 80)); ChInstrument[8] = 0;} else lShake(0);
        //outp(0x3C8, 0); outp(0x3C9, 63);
        _asm {
            mov  esi, offset lbuffer
            mov  edi, screen
            mov  ecx, 16000
            rep  movsd  
        }
        //memcpy(screen, &lbuffer, 64000);
        ldrawLens(i);
        //outp(0x3C8, 0); outp(0x3C9, 0);
    }
    if (notimer==0) {Timer_Stop(&lTimer);}
}
