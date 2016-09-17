
#define fx2_stardist 128
#define fx2_count    729
#define fx2_lut_size 65536
#define fx2_spr_size 16

unsigned char fx2_spr[fx2_spr_size * fx2_spr_size];

vertex   fx2_p[fx2_count], fx2_pt[fx2_count], fx2_pm[fx2_count];
vertex2d fx2_p2d[fx2_count];

unsigned char fx2_buffer[64320];
unsigned char fx2_pal[1024];
unsigned char fx2_greets[32768];

void fx2_3dRotate (int ax, int ay, int az) {
    // hehehe, this code is fully ported from my old freebasic demoz ;)
    int i;
    float sinx = sintabf[ax], cosx = costabf[ax];
    float siny = sintabf[ay], cosy = costabf[ay];
    float sinz = sintabf[az], cosz = costabf[az];
    float bx, by, bz, px, py, pz;  // temp var storage
    for (i = 0; i < fx2_count; i++) {
        fx2_pt[i] = fx2_pm[i];
        
        py = fx2_pt[i].y;
        pz = fx2_pt[i].z;
        fx2_pt[i].y = (py * cosx - pz * sinx);
        fx2_pt[i].z = (py * sinx + pz * cosx);
        
        px = fx2_pt[i].x;
        pz = fx2_pt[i].z;
        fx2_pt[i].x = (px * cosy - pz * siny);
        fx2_pt[i].z = (px * siny + pz * cosy);
        
        px = fx2_pt[i].x;
        py = fx2_pt[i].y;
        fx2_pt[i].x = (px * cosz - py * sinz);
        fx2_pt[i].y = (px * sinz + py * cosz);
    }
} 

void fx2_3dMove (float ax, float ay, float az) {
    int i;
    
    for (i = 0; i < fx2_count; i++) {
        fx2_pt[i].x += ax;
        fx2_pt[i].y += ay;
        fx2_pt[i].z += az;
    }
}

void fx2_3dMovep (float ax, float ay, float az) {
    int i;
    
    for (i = 0; i < fx2_count; i++) {
        fx2_pm[i].x = fx2_p[i].x + ax;
        fx2_pm[i].y = fx2_p[i].y + ay;
        fx2_pm[i].z = fx2_p[i].z + az;
    }
}

void fx2_3dProject () {
    int i;
    float t;
    for (i = 0; i < fx2_count; i++) if (fx2_pt[i].z < 0) {
        t = DIST / (fx2_pt[i].z + ee);
        fx2_p2d[i].x = fx2_pt[i].x * t + (X_SIZE >> 1);
        fx2_p2d[i].y = fx2_pt[i].y * t + (Y_SIZE >> 1);
    }
}

void fx2_DrawPoints () {
    int i, y, x, j;
    int px, py, ofs;
    long scrptr = (long)&fx2_buffer;
    long sprptr = (long)&fx2_spr;
    
    for (i = 0; i < fx2_count; i++) {
        sprptr = (long)&fx2_spr;
        px = fx2_p2d[i].x - (fx2_spr_size >> 1); py = fx2_p2d[i].y - (fx2_spr_size >> 1);
        
        if ((py<(200 - (fx2_spr_size)))&&(py>0)&&(px>0)&&(px<(320 - (fx2_spr_size)))&&(fx2_pt[i].z < 0)) {    
            j = 0;
            scrptr = (long)&fx2_buffer + ((py << 8) + (py << 6) + px);
            
            for (y = 0; y < (fx2_spr_size); y++) {
                for (x = 0; x < (fx2_spr_size); x++) {
                    *((char*)scrptr) = sat((*((char*)scrptr) + *((char*)sprptr)), 255);
                    scrptr++; sprptr++;
                }
                scrptr += (320 - (fx2_spr_size));
            }
        }
    }
}

void fx2_FillDots () {
    int rat, max, phi, teta, count2, i, lptr = 0;
    
    max = sqrt(fx2_count);
    rat = 0xFFFF / max;

    for (teta = 0; teta < max; teta++)
        for (phi = 0; phi < max; phi++) {
        fx2_p[lptr  ].x = ((fx2_stardist<<1) + (fx2_stardist * costabf[phi*rat]))*costabf[teta*rat];
   		fx2_p[lptr  ].y = ((fx2_stardist<<1) + (fx2_stardist * costabf[phi*rat]))*sintabf[teta*rat];
	    fx2_p[lptr++].z = fx2_stardist * sintabf[phi*rat];
    }
    
        
}

void fx2_FillBuffer() {
    int i, j;
    
    for (i = 0; i < 512; i++) {
        j = ((rand() % 31680) << 1) + 320; 
        fx2_buffer[j]   = sat(fx2_buffer[j]   + 128, 255);
        fx2_buffer[j-1] = sat(fx2_buffer[j-1] + 64, 255);
        fx2_buffer[j+1] = sat(fx2_buffer[j+1] + 64, 255);
    }    
    for (i = 0; i < (64000); i++) {
        fx2_buffer[i] >>= 1;
    }
}

void fx2_MakeSprite() {
    int x, y, k;
    long sprptr = (long)&fx2_spr;
    
    for (y = -(fx2_spr_size >> 1); y < (fx2_spr_size >> 1); y++) {
        for (x = -(fx2_spr_size >> 1); x < (fx2_spr_size >> 1); x++) {
            *((char*)sprptr++) = sat((int)(0x100 / ((x*x + y*y) + ee)), 64) & 0xFF;
        }
    }
}

void fx2_DrawGreets(int pos) {
    int i, y, x, j;
    int px, py, ofs;
    long scrptr = (long)&fx2_buffer + (184 * 320) + 192;
    long sprptr = (long)&fx2_greets + (pos << 11);
    
    for (y = 0; y < 16; y++) {
        for (x = 0; x < 128; x++) {
            *((char*)scrptr) = sat((*((char*)scrptr) + *((char*)sprptr)), 255);
            scrptr++; sprptr++;
        }
        scrptr += (320 - 128);
    }
}

void fx2_initPal() {
    int i;

    for (i = 0; i < 256; i++) {
        fx2_pal[(i<<2) + 0] = (i >> 3) + (i >> 4);
        fx2_pal[(i<<2) + 1] = (i >> 2) - (i >> 5);
        fx2_pal[(i<<2) + 2] = (i >> 3) + (i >> 4);
    }
}
/*
void fx1_FadeInv(int scale) {
    int i, j;
    
    outp(0x3C8, 0);
    for (i = 0; i < 256; i++) {
        outp(0x3C9, (((63 - fx1_pal[(i<<2) + 0]) * scale) >> 8) + (fx1_pal[(i<<2) + 0] * (256 - scale) >> 8));
        outp(0x3C9, (((63 - fx1_pal[(i<<2) + 1]) * scale) >> 8) + (fx1_pal[(i<<2) + 1] * (256 - scale) >> 8));
        outp(0x3C9, (((63 - fx1_pal[(i<<2) + 2]) * scale) >> 8) + (fx1_pal[(i<<2) + 2] * (256 - scale) >> 8));
    }
}
*/

void fx2_FadeOut(int scale) {
    int i, j;
    
    outp(0x3C8, 0);
    for (i = 0; i < 256; i++) {
        outp(0x3C9, sat((fx2_pal[(i << 2) + 0] * scale) >> 8, 63));
        outp(0x3C9, sat((fx2_pal[(i << 2) + 1] * scale) >> 8, 63));
        outp(0x3C9, sat((fx2_pal[(i << 2) + 2] * scale) >> 8, 63));
    }
}

void fx2_FadeIn(int scale, int start, int stop) {
    int i, j;
    
    outp(0x3C8, 0);
    for (i = start; i <= stop; i++) {
        outp(0x3C9, sat(((63 * scale) >> 8) + (fx2_pal[(i<<2) + 0] * (256 - scale) >> 8), 63));
        outp(0x3C9, sat(((63 * scale) >> 8) + (fx2_pal[(i<<2) + 1] * (256 - scale) >> 8), 63));
        outp(0x3C9, sat(((63 * scale) >> 8) + (fx2_pal[(i<<2) + 2] * (256 - scale) >> 8), 63));
    }
}

void fx2_init() {
    FILE *f;
    int i, c;
    
    f = fopen("some.dat", "rb");
    for (i = 0; i < 32768; i++) {
        c = fgetc(f); fx2_greets[i] = c;
    }
    close(f);
    
    fx2_MakeSprite();
    fx2_FillDots();
    fx2_initPal();
}

int fx2_tick = 0, fx2_inc = 0;

void fx2_timer() {
    fx2_tick++; fx2_inc++;
}

void fx2_main () {
    int i = 0;
    int e = 256;
    int f = 256;
    
    if (notimer==0) {rtc_setTimer(&fx2_timer, rtc_timerRate / 60);}  

    while (1) {
        //i++;
        if (notimer==0) {i = fx2_tick;} else {i++;}
        
        if (noretrace == 0 ) {
            while ((inp(0x3DA) & 8) == 8) {}
            while ((inp(0x3DA) & 8) != 8) {}
        }
        
        if ((Order == 0x15) && (e >  0)) {e -= (16 * fx2_inc); e = (e < 0 ? 0 : e); fx2_FadeIn(e, 0, 255);}
        if ((Order != 0x1D) && (e == 0)) {fx2_FadeOut(256 + (sintab[(i << 12) & 0xFFFF] >> 11));}
        if ((Order == 0x1D) && (f > 0)) {f -= (2 * fx2_inc); f = (f < 0 ? 0 : f); fx2_FadeOut(f);}
        
        fx2_inc = ((notimer == 0) ? 0 : 1);
        
        memcpy(screen, &fx2_buffer, 64000);   
        fx2_DrawGreets((i >> 6) & 15);
        
        fx2_3dMovep(0,
                 (fx2_stardist<<1) * sintabf[(i << 9) & 0xFFFF],
                 (fx2_stardist<<1) * costabf[(i << 9) & 0xFFFF]);
        
        
        fx2_3dRotate(((i << 6) + (i << 7) ) & 0xFFFF,
                    0,
                   ((i << 5) + (i << 8)) & 0xFFFF);
        
        
        
        fx2_3dMove  (0,0,64 * costabf[(i << 8) & 0xFFFF] + -DIST);
        
        fx2_3dProject();
        fx2_FillBuffer();
        fx2_DrawPoints();
        if ((Order == 0x1D) && (Row > 32)) break;
        
        if (kbhit()) {if (getch() == 27) {KillAll();}}
    }

}