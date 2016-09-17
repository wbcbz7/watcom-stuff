
unsigned char  w1texture[131072];
unsigned char  w1buffer[64000];
unsigned char  w1pal[1024];

unsigned short twirl1[4 * 64000];

         short w1sintab[256];


int sat(int a, int l) {return ((a > l) ? l : a); } 

void w1buildSinTable() {
    int i, j;
    float r;
    
    for (i = 0; i < 256; i++) {
        r = (32767 * sin(2 * pi * i / 256));
        w1sintab[i] = r;
    }
}

void w1buildTunnel() {
    long x, y, i, j, u, v;
    double r, a;
    
    const TunnelSize = 4096;
    const LightmapScale = 1.0;

    for (j = 0; j < 4; j++) {
        i = 64000 * j;
        for (y = -100; y < 100; y++) 
            for (x = -160; x < 160; x++) {
                r = sqrt(x*x + y*y);
                if (r < 1) r = 1;
                a = atan2(y, x) + pi;
                u = a * 128 / pi;
                v = (r + (w1sintab[(u<<(4 - (j)))&0xFF] >> (11+j)));
                twirl1[i++] = (u&0xFF) + ((v&0xFF)<<8);
            }
    }
}

void w1buildTexture() {
    unsigned int x, y, i, k;
    
    for (y = 0; y < 512; y++) {
        for (x = 0; x < 256; x++) {
            //texture[((y << 8) + x)] = sat((x ^ y), 255) & 0xFF;
            w1texture[((y << 8) + x)] = (x ^ (y & 0xFF)) & 0xFF;
            //texture[((y << 8) + x)] = (rand() % 0x100) & 0xFF;
        }
    }
    for (k = 0; k < 8; k++)
    for (i = 0; i < 131072; i++) 
        w1texture[i] = (w1texture[(i-1)&0x1FFFF] + w1texture[(i+1)&0x1FFFF] + 
                      w1texture[(i-256)&0x1FFFF] + w1texture[(i+256)&0x1FFFF]) >> 2; 
    
}

void w1drawTwirl (int c, unsigned short *lutptr) {
    int u1 = w1sintab[((c >> 1) + (c >> 2)) & 255] >> 8;
    int v1 = w1sintab[((c << 1) + (w1sintab[(c >> 2) & 255] >>10 )) & 255] >> 8;
    int u2 = w1sintab[(c >> 1) & 255] >> 8;
    int v2 = w1sintab[(c) & 255] >> 9;
    int texofs1 = ((v1 << 8) + u1) &0xFFFF;
    int texofs2 = ((v2 << 8) + u2) &0xFFFF;
    int i = 0;
    long scrptr = (long)&w1buffer;
    int k = 0;
    
    for (k = 0; k < 64000; k++)
        *((char*)scrptr++) = (w1texture[(*((unsigned short*)(lutptr)+k)+texofs1)] + 
                              w1texture[(*((unsigned short*)(lutptr)+k)+texofs2)]) >> 1; 
}

void w1InitPal() {
    int i;

    for (i = 0; i < 256; i++) {
        w1pal[(i<<2) + 0] = (i >> 3) + (i >> 4);
        w1pal[(i<<2) + 1] = (i >> 3) + (i >> 5);
        w1pal[(i<<2) + 2] = (i >> 3) - (i >> 4);
    }
}

void w1FadeIn(int scale) {
    int i, j;
    
    outp(0x3C8, 0);
    for (i = 0; i < 256; i++) {
        outp(0x3C9, ((63 * scale) >> 8) + (w1pal[(i<<2) + 0] * (256 - scale) >> 8));
        outp(0x3C9, ((63 * scale) >> 8) + (w1pal[(i<<2) + 1] * (256 - scale) >> 8));
        outp(0x3C9, ((63 * scale) >> 8) + (w1pal[(i<<2) + 2] * (256 - scale) >> 8));
    }
}

void lFadeOut(int scale) {
    int i, j;
    
    outp(0x3C8, 0);
    for (i = 0; i < 256; i++) {
        outp(0x3C9, sat((w1pal[(i<<2) + 0] * scale) >> 8, 63));
        outp(0x3C9, sat((w1pal[(i<<2) + 1] * scale) >> 8, 63));
        outp(0x3C9, sat((w1pal[(i<<2) + 2] * scale) >> 8, 63));
    }
}

int  w1Tick = 0;

void w1Timer() {
    w1Tick++;
}

void w1Init() {
    w1buildSinTable();
    w1buildTunnel();
    w1buildTexture();
    w1InitPal();
}

void w1Main() {
    int i, e = 256, f = 256;
    int currow = 0;
    int stoporder = 0x1D;
    unsigned short *lutptr = &twirl1;
    
    if (notimer==0) {Timer_Start(&w1Timer,TimerSpeed/60);} 
    
    while (Order < stoporder) {
        if (notimer==0) {i = w1Tick; } else {i++;}
        
        if (noretrace == 0) {
        while ((inp(0x3DA) & 8) == 8) {}
        while ((inp(0x3DA) & 8) != 8) {} }
        
        
        //if ((Order == 0x1C)&&(Row = 0)&&(f < 16)) f = 256;
        if ((Order == 0x1C)&&(e > 0)) {lFadeOut(e); e -= 2;}
        if ((Order != 0x1C)&&(f > 0)) {f -= 8; w1FadeIn(f);}
        //outp(0x3C8, 0); outp(0x3C9, 63);   
        
        _asm {
            mov  esi, offset w1buffer
            mov  edi, screen
            mov  ecx, 16000
            rep  movsd  
        }
                
        w1drawTwirl(i, lutptr);
        
        //outp(0x3C8, 0); outp(0x3C9, 0);
    }
    if (notimer==0) {Timer_Stop(&w1Timer);}
}
