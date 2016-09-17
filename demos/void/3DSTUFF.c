#define fxstardist 512
#define fxcount    4096
#define fxlut_size 65536

vertex   p[fxcount], pt[fxcount];
vertex2d p2d[fxcount];

float fxsintab[fxlut_size], fxcostab[fxlut_size];

unsigned char fxbuffer[64000];
unsigned char fxrnd[64000];
unsigned char fxpal[1024];

void fxMakeSinTable () {
    int i, j;
    double r, lut_mul;
    lut_mul = (2 * pi / fxlut_size);
    for (i = 0; i < fxlut_size; i++) {
        r = i * lut_mul;
        fxsintab[i] = sin(r);
        fxcostab[i] = cos(r);
    }
}

void fxInitRnd() {
    int i, j;
    for (i = 0; i < 64000; i++) {
        fxrnd[i] = (rand() & 0xFF);
    }
}

void fx3dRotate (int ax, int ay, int az) {
    // hehehe, this code is fully ported from my old freebasic demoz ;)
    int i;
    float sinx = fxsintab[ax], cosx = fxcostab[ax];
    float siny = fxsintab[ay], cosy = fxcostab[ay];
    float sinz = fxsintab[az], cosz = fxcostab[az];
    float bx, by, bz, px, py, pz;  // temp var storage
    for (i = 0; i < fxcount; i++) {
        pt[i] = p[i];
        
        py = pt[i].y;
        pz = pt[i].z;
        pt[i].y = (py * cosx - pz * sinx);
        pt[i].z = (py * sinx + pz * cosx);
        
        px = pt[i].x;
        pz = pt[i].z;
        pt[i].x = (px * cosy - pz * siny);
        pt[i].z = (px * siny + pz * cosy);
        
        px = pt[i].x;
        py = pt[i].y;
        pt[i].x = (px * cosz - py * sinz);
        pt[i].y = (px * sinz + py * cosz);
    }
} 

void fx3dProject () {
    int i;
    float t;
    for (i = 0; i < fxcount; i++) if (pt[i].z < 0) {
        t = DIST / (pt[i].z + 0.0001);
        p2d[i].x = pt[i].x * t + (X_SIZE >> 1);
        p2d[i].y = pt[i].y * t + (Y_SIZE >> 1);
    }
}

void fxDrawPoints () {
    int i, y, x, j;
    int px, py, ofs;
    long scrptr = (long)&fxbuffer;
    
    for (i = 0; i < fxcount; i++) {
        px = p2d[i].x; py = p2d[i].y;
        
        if ((py<200)&&(py>0)&&(px>0)&&(px<316)&&(pt[i].z < 0)) {
            j = 0;
            scrptr = (long)&fxbuffer + ((py << 8) + (py << 6) + px);
            _asm {
                mov edi, scrptr
                mov eax, 0x20404020
                
                mov ecx, 2
                @outer:
                add  [edi], eax
                add  edi, 4
                add  edi, (X_SIZE - 4)
                loop @outer
            }
        }
    }
}

void fxFillDots () {
    int rat, max, phi, teta;
    
    max = sqrt(fxcount);
    rat = fxlut_size / max;
    
    for (teta = 0; teta < max; teta++)
        for (phi = 0; phi < max; phi++) {
        p[teta*max+phi].x = (fxstardist * fxsintab[phi*rat]) * fxcostab[teta*rat];
        p[teta*max+phi].y = fxstardist * fxcostab[phi*rat];
        p[teta*max+phi].z = (fxstardist * fxsintab[phi*rat]) * fxsintab[teta*rat];
        
        }
}

void fxFillBuffer(int shift) {
    int i, j;
    
    j = (rand() & 0xFF);
    for (i = 0; i < 64000; i++) {
        fxbuffer[i] = (fxrnd[i] ^ j) >> shift;
    
    }
}

void fxInitPal() {
    int i;

    for (i = 0; i < 256; i++) {
        fxpal[(i<<2) + 0] = (i >> 3) + (i >> 4);
        fxpal[(i<<2) + 1] = (i >> 3) + (i >> 5);
        fxpal[(i<<2) + 2] = (i >> 3) - (i >> 4);
    }
}

void fxFadeIn(int scale) {
    int i, j;
    
    outp(0x3C8, 0);
    for (i = 0; i < 256; i++) {
        outp(0x3C9, sat((fxpal[(i<<2) + 0] * scale) >> 8, 63));
        outp(0x3C9, sat((fxpal[(i<<2) + 1] * scale) >> 8, 63));
        outp(0x3C9, sat((fxpal[(i<<2) + 2] * scale) >> 8, 63));
    }
}

int  fxTick = 0;

void fxTimer() {
    fxTick++;
}

void fxInit () {
    fxMakeSinTable();
    fxFillDots();
    fxInitRnd();
    fxInitPal();
}

void fxMain () {
    int i=0;
    int j=0;
    int shift;
    
    if (notimer==0) {Timer_Start(&fxTimer,TimerSpeed/60);} 
    
    while (Order < 8) {
        if (notimer==0) {i = fxTick; } else {i++;}
        
        if (noretrace == 0) {
        while ((inp(0x3DA) & 8) == 8) {}
        while ((inp(0x3DA) & 8) != 8) {} }
        if (j < 256) {fxFadeIn(j); j++;}
        //outp(0x3C8, 0); outp(0x3C9, 63);   
        
        _asm {
            mov  esi, offset fxbuffer
            mov  edi, screen
            mov  ecx, 16000
            rep  movsd  
        }
        
        //memcpy(screen, &fxbuffer, 64000);   
        
        fx3dRotate(((i << 6) + (i << 5) ) & (fxlut_size-1), (i << 6) & (fxlut_size-1), ((i << 6) - (i << 4)) & (fxlut_size-1));
        fx3dProject();
        //memset(buffer, 0, 64000);
        if (ChInstrument[11] == 3) shift = 2; else shift = 4; 
        fxFillBuffer(shift);
        fxDrawPoints();
         
        //outp(0x3C8, 0); outp(0x3C9, 0);
    }
    if (notimer==0) {Timer_Stop(&fxTimer);}
}
