
unsigned char  w2texture[131072];
unsigned char  w2buffer[64000];
unsigned short twirl2[64000];
unsigned char  w2pal[1024];

         short w2sintab[256];

void w2buildSinTable() {
    int i, j;
    float r;
    
    for (i = 0; i < 256; i++) {
        r = (32767 * sin(2 * pi * i / 256));
        w2sintab[i] = r;
    }
}

void w2buildTunnel() {
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
            u = a * 128 / pi;
            v = (r);
            twirl2[i++] = (u&0xFF) + ((v&0xFF)<<8);
        }
    
}

void w2buildTexture() {
    unsigned int x, y, i, k;
    
    for (y = 0; y < 512; y++) {
        for (x = 0; x < 256; x++) {
            w2texture[((y << 8) + x)] = (x ^ (y ^ 0xFF)) & 0xFF;
            //texture[((y << 8) + x)] = (x ^ y) & 0xFF;
            //texture[((y << 8) + x)] = (rand() % 0x100) & 0xFF;
        }
    }
    for (k = 0; k < 4; k++)
    for (i = 0; i < 131072; i++) 
        w2texture[i] = (w2texture[(i-1)&0x1FFFF] + w2texture[(i+1)&0x1FFFF] + 
                      w2texture[(i-256)&0x1FFFF] + w2texture[(i+256)&0x1FFFF]) >> 2; 
    
}

void w2drawTwirl (int c) {
    int u1 = w2sintab[((c >> 1) + (c >> 2)) & 255] >> 8;
    int v1 = w2sintab[(c << 0) & 255] >> 8;
    int u2 = w2sintab[(c >> 1) & 255] >> 8, v2 = (c << 1);
    int texofs1 = ((v1 << 8) + u1) & 0xFFFF;
    int texofs2 = ((v2 << 8) + u2) & 0xFFFF;
    int i = 0;
    long scrptr = (long)&w2buffer;
    int k = 0;
    
    for (k = 0; k < 64000; k++)
        *((char*)scrptr++) = (w2texture[(twirl2[k]+texofs1)] + 
                              w2texture[(twirl2[k]+texofs2)]) >> 1 ; 
}

void w2InitPal() {
    int i;

    for (i = 0; i < 256; i++) {
        w2pal[(i<<2) + 0] = (i >> 3) + (i >> 4);
        w2pal[(i<<2) + 1] = (i >> 3) + (i >> 5);
        w2pal[(i<<2) + 2] = (i >> 2);
    }
}

void w2FadeIn(int scale) {
    int i, j;
    
    outp(0x3C8, 0);
    for (i = 0; i < 256; i++) {
        outp(0x3C9, sat((w2pal[(i<<2) + 0] * scale) >> 8, 63));
        outp(0x3C9, sat((w2pal[(i<<2) + 1] * scale) >> 8, 63));
        outp(0x3C9, sat((w2pal[(i<<2) + 2] * scale) >> 8, 63));
    }
}

int  w2Tick = 0;

void w2Timer() {
    w2Tick++;
}

void w2Init() {
    w2buildSinTable();
    w2buildTunnel();
    w2buildTexture();
    w2InitPal();
}

int w2Main() {
    int i;
    int p = 1024;
    
    if (notimer==0) {Timer_Start(&w2Timer,TimerSpeed/60);} 
    while (Order < 0xC) {
        if (notimer==0) {i = w2Tick;} else {i++;}
        
        if (noretrace == 0) {
        while ((inp(0x3DA) & 8) == 8) {}
        while ((inp(0x3DA) & 8) != 8) {} }
        if (p > 256) {w2FadeIn(p); p -= 32;}
        
        _asm {
            mov  esi, offset w2buffer
            mov  edi, screen
            mov  ecx, 16000
            rep  movsd  
        }
        
        //memcpy(screen, &w2buffer, 64000);
        
        w2drawTwirl(i);
    }
    if (notimer==0) {Timer_Stop(&w2Timer);}
}
