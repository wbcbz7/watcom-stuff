#define gstardist 100
#define gcount    4096
#define glut_size 65536
#define gdist     800

vertex   gp[gcount], gpt[gcount];
vertex2d gp2d[gcount];

float gsintab[glut_size], gcostab[glut_size];

unsigned char gbuffer[64000];
unsigned char grnd[64000];
unsigned char gpal[1024];

void gMakeSinTable () {
    int i, j;
    double r, lut_mul;
    lut_mul = (2 * pi / glut_size);
    for (i = 0; i < glut_size; i++) {
        r = i * lut_mul;
        gsintab[i] = sin(r);
        gcostab[i] = cos(r);
    }
}

void gInitRnd() {
    int i, j;
    for (i = 0; i < 64000; i++) {
        grnd[i] = (rand() & 0xFF);
    }
}

void g3dRotate (int ax, int ay, int az) {
    // hehehe, this code is fully ported from my old freebasic demoz ;)
    int i;
    float sinx = gsintab[ax], cosx = gcostab[ax];
    float siny = gsintab[ay], cosy = gcostab[ay];
    float sinz = gsintab[az], cosz = gcostab[az];
    float bx, by, bz, px, py, pz;  // temp var storage
    for (i = 0; i < gcount; i++) {
        gpt[i] = gp[i];
        
        py = gpt[i].y;
        pz = gpt[i].z;
        gpt[i].y = (py * cosx - pz * sinx);
        gpt[i].z = (py * sinx + pz * cosx);
        
        px = gpt[i].x;
        pz = gpt[i].z;
        gpt[i].x = (px * cosy - pz * siny);
        gpt[i].z = (px * siny + pz * cosy);
        
        px = gpt[i].x;
        py = gpt[i].y;
        gpt[i].x = (px * cosz - py * sinz);
        gpt[i].y = (px * sinz + py * cosz);
    }
} 

void g3dProject () {
    int i;
    float t;
    for (i = 0; i < gcount; i++) if (gpt[i].z < gdist) {
        t = DIST / (-gpt[i].z + gdist);
        gp2d[i].x = gpt[i].x * t + (X_SIZE >> 1);
        gp2d[i].y = gpt[i].y * t + (Y_SIZE >> 1);
    }
}

void gDrawPoints () {
    int i, y, x, j;
    int px, py, ofs;
    long scrptr = (long)&gbuffer;
    
    for (i = 0; i < gcount; i++) {
        px = gp2d[i].x; py = gp2d[i].y;
        
        if ((py<200)&&(py>0)&&(px>0)&&(px<316)&&(gpt[i].z < gdist)) {
            j = 0;
            scrptr = (long)&gbuffer + ((py << 8) + (py << 6) + px);
            _asm {
                mov edi, scrptr
                mov eax, 0x08101008
                
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

void gFillDots () {
    int rat, max, phi, teta;
    
    max = sqrt(gcount);
    rat = glut_size / max;
    
    for (teta = 0; teta < max; teta++)
        for (phi = 0; phi < max; phi++) {
        //gp[teta*max+phi].x = (gstardist * gsintab[phi*rat]) * gcostab[teta*rat];
        //gp[teta*max+phi].y = gstardist * gcostab[phi*rat];
        //gp[teta*max+phi].z = (gstardist * gsintab[phi*rat]) * gsintab[teta*rat];
        
        gp[max*teta+phi].x = ((gstardist<<1) + (gstardist * gcostab[phi*rat]))*gcostab[teta*rat];
   		gp[max*teta+phi].y = ((gstardist<<1) + (gstardist * gcostab[phi*rat]))*gsintab[teta*rat];
	    gp[max*teta+phi].z = gstardist * gsintab[phi*rat];
        }
}

void gFillBuffer(int shift) {
    int i, j;
    
    j = (rand() & 0xFF);
    for (i = 0; i < 64000; i++) {
        gbuffer[i] = (grnd[i] ^ j) >> shift;
    
    }
}

void gInitPal() {
    int i;

    for (i = 0; i < 256; i++) {
        gpal[(i<<2) + 0] = (i >> 3) + (i >> 4);
        gpal[(i<<2) + 1] = (i >> 3) + (i >> 5);
        gpal[(i<<2) + 2] = (i >> 3) - (i >> 4);
    }
}

void gFadeIn(int scale) {
    int i, j;
    
    outp(0x3C8, 0);
    for (i = 0; i < 256; i++) {
        outp(0x3C9, sat((gpal[(i<<2) + 0] * scale) >> 8, 63));
        outp(0x3C9, sat((gpal[(i<<2) + 1] * scale) >> 8, 63));
        outp(0x3C9, sat((gpal[(i<<2) + 2] * scale) >> 8, 63));
    }
}

void gInit () {
    gMakeSinTable();
    gFillDots();
    gInitRnd();
    gInitPal();
}

int  gTick = 0;

void gTimer() {
    gTick++;
}

void gMain () {
    int i = 0;
    int j = 0;
    int shift;
    
    if (notimer==0) {Timer_Start(&gTimer,TimerSpeed/60);}
    
    while (Order < 0x14) {
        if (notimer==0) {i = gTick; } else {i++;}
        
        if (noretrace == 0) {
        while ((inp(0x3DA) & 8) == 8) {}
        while ((inp(0x3DA) & 8) != 8) {} }
        if (j < 256) {gFadeIn(j); j++;}
        //outp(0x3C8, 0); outp(0x3C9, 63);   
        

        _asm {
            mov  esi, offset gbuffer
            mov  edi, screen
            mov  ecx, 16000
            rep  movsd  
        }
        //memcpy(screen, &gbuffer, 64000);   
        
        g3dRotate(((i << 6) + (i << 5) ) & (glut_size-1), (i << 6) & (glut_size-1), ((i << 6) - (i << 4)) & (glut_size-1));
        g3dProject();
        //memset(buffer, 0, 64000);
        if (ChInstrument[11] == 3) shift = 2; else shift = 3; 
        gFillBuffer(shift);
        gDrawPoints();

        //outp(0x3C8, 0); outp(0x3C9, 0);
    }
    if (notimer==0) {Timer_Stop(&gTimer);}
}