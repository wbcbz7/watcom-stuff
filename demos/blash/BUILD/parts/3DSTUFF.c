
#define fx1_stardist 64
#define fx1_count    512
#define fx1_lut_size 65536
#define fx1_spr_size 16

unsigned char fx1_spr[fx1_spr_size * fx1_spr_size];

vertex   fx1_p[fx1_count], fx1_pt[fx1_count], fx1_pm[fx1_count];
vertex2d fx1_p2d[fx1_count];

unsigned char fx1_buffer[64320];
unsigned char fx1_pal[1024];

void fx1_3dRotate (int ax, int ay, int az) {
    // hehehe, this code is fully ported from my old freebasic demoz ;)
    int i;
    float sinx = sintabf[ax], cosx = costabf[ax];
    float siny = sintabf[ay], cosy = costabf[ay];
    float sinz = sintabf[az], cosz = costabf[az];
    float bx, by, bz, px, py, pz;  // temp var storage
    for (i = 0; i < fx1_count; i++) {
        fx1_pt[i] = fx1_pm[i];
        
        py = fx1_pt[i].y;
        pz = fx1_pt[i].z;
        fx1_pt[i].y = (py * cosx - pz * sinx);
        fx1_pt[i].z = (py * sinx + pz * cosx);
        
        px = fx1_pt[i].x;
        pz = fx1_pt[i].z;
        fx1_pt[i].x = (px * cosy - pz * siny);
        fx1_pt[i].z = (px * siny + pz * cosy);
        
        px = fx1_pt[i].x;
        py = fx1_pt[i].y;
        fx1_pt[i].x = (px * cosz - py * sinz);
        fx1_pt[i].y = (px * sinz + py * cosz);
    }
} 

void fx1_3dMove (float ax, float ay, float az) {
    int i;
    
    for (i = 0; i < fx1_count; i++) {
        fx1_pt[i].x += ax;
        fx1_pt[i].y += ay;
        fx1_pt[i].z += az;
    }
}

void fx1_3dMovep (float ax, float ay, float az) {
    int i;
    
    for (i = 0; i < fx1_count; i++) {
        fx1_pm[i].x = fx1_p[i].x + ax;
        fx1_pm[i].y = fx1_p[i].y + ay;
        fx1_pm[i].z = fx1_p[i].z + az;
    }
}

void fx1_3dProject () {
    int i;
    float t;
    for (i = 0; i < fx1_count; i++) if (fx1_pt[i].z < 0) {
        t = DIST / (fx1_pt[i].z + ee);
        fx1_p2d[i].x = fx1_pt[i].x * t + (X_SIZE >> 1);
        fx1_p2d[i].y = fx1_pt[i].y * t + (Y_SIZE >> 1);
    }
}

void fx1_DrawPoints () {
    int i, y, x, j;
    int px, py, ofs;
    long scrptr = (long)&fx1_buffer;
    long sprptr = (long)&fx1_spr;
    
    for (i = 0; i < fx1_count; i++) {
        sprptr = (long)&fx1_spr;
        px = fx1_p2d[i].x - (fx1_spr_size >> 1); py = fx1_p2d[i].y - (fx1_spr_size >> 1);
        
        if ((py<(200 - fx1_spr_size))&&(py>0)&&(px>0)&&(px<(320 - fx1_spr_size))&&(fx1_pt[i].z < 0)) {
            
            j = 0;
            scrptr = (long)&fx1_buffer + ((py << 8) + (py << 6) + px);
            
            if ((lowres == 0) | (fakelowres == 1))
            for (y = 0; y < (fx1_spr_size); y++) {
                for (x = 0; x < (fx1_spr_size); x++) {
                    *((char*)scrptr++) = sat((*((char*)scrptr) + *((char*)sprptr)), 255);
                    sprptr++;// scrptr++;
                }
                scrptr += (320 - (fx1_spr_size));
            }
            else {
            px = (px >> 1);
            scrptr = (long)&fx1_buffer + ((py << 7) + (py << 5) + px);
            for (y = 0; y < (fx1_spr_size); y++) {
                for (x = 0; x < (fx1_spr_size >> 1); x++) {
                    *((char*)scrptr) = sat((*((char*)scrptr) + *((char*)sprptr)), 224);
                    scrptr++; sprptr +=2 ;
                }
                scrptr += (160 - (fx1_spr_size >> 1));
            } 
            }
        }
    }
}

void fx1_FillDots () {
    int rat, max, phi, teta, count2, i, lptr = 0;
    

    for (i = 0; i < fx1_count; i++) {
        fx1_p[lptr  ].x = fx1_stardist * sintabf[(i << 9 ) & 0xFFFF];
        fx1_p[lptr  ].y = fx1_stardist * costabf[(i << 9 ) & 0xFFFF];
        fx1_p[lptr++].z = -(fx1_count) + (i) + fx1_stardist * costabf[(i << 12 ) & 0xFFFF];;
    }

}

void fx1_FillBuffer() {
    int i, j;
    
    for (i = 0; i < 512; i++) {
        j = ((rand() % 31680) << 1) + 320; 
        fx1_buffer[j]   = sat(fx1_buffer[j]   + 128, 255);
        fx1_buffer[j-1] = sat(fx1_buffer[j-1] + 64, 255);
        fx1_buffer[j+1] = sat(fx1_buffer[j+1] + 64, 255);
    }    
    for (i = 320; i < (64000); i++) {
        fx1_buffer[i] >>= 1;
    }
}

void fx1_MakeSprite() {
    int x, y, k;
    long sprptr = (long)&fx1_spr;
    
    for (y = -(fx1_spr_size >> 1); y < (fx1_spr_size >> 1); y++) {
        for (x = -(fx1_spr_size >> 1); x < (fx1_spr_size >> 1); x++) {
            *((char*)sprptr++) = sat((int)(0x200 / ((x*x + y*y) + ee)), 128) & 0xFF;
        }
    }
}

void fx1_initPal() {
    int i;

    for (i = 0; i < 256; i++) {
        fx1_pal[(i<<2) + 0] = (i >> 3) + (i >> 4);
        fx1_pal[(i<<2) + 1] = (i >> 3) + (i >> 5);
        fx1_pal[(i<<2) + 2] = (i >> 3) + (i >> 4);
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

void fx1_FadeOut(int scale) {
    int i, j;
    
    outp(0x3C8, 0);
    for (i = 0; i < 256; i++) {
        while ((inp(0x3DA) & 1) != 1) {}
        //outp(0x3C8, i);
        outp(0x3C9, sat((fx1_pal[(i << 2) + 0] * scale) >> 8, 63));
        outp(0x3C9, sat((fx1_pal[(i << 2) + 1] * scale) >> 8, 63));
        outp(0x3C9, sat((fx1_pal[(i << 2) + 2] * scale) >> 8, 63));
    }
}

void fx1_FadeIn(int scale, int start, int stop) {
    int i, j;
    
    outp(0x3C8, 0);
    for (i = start; i <= stop; i++) {
        while ((inp(0x3DA) & 1) != 1) {}
        //outp(0x3C8, i);
        outp(0x3C9, sat(((63 * scale) >> 8) + (fx1_pal[(i<<2) + 0] * (256 - scale) >> 8), 63));
        outp(0x3C9, sat(((63 * scale) >> 8) + (fx1_pal[(i<<2) + 1] * (256 - scale) >> 8), 63));
        outp(0x3C9, sat(((63 * scale) >> 8) + (fx1_pal[(i<<2) + 2] * (256 - scale) >> 8), 63));
    }
}

int fx1_tick = 0, fx1_inc = 0;

void fx1_timer() {
    fx1_tick++; fx1_inc++;
}

void fx1_init() {
    fx1_MakeSprite();
    fx1_FillDots();
    fx1_initPal();
}

void fx1_main () {
    int i = 0;
    int e = 256;
    int f = 256;
    
    if (notimer==0) {rtc_setTimer(&fx1_timer, rtc_timerRate / 60);}    
    //outp(0x3C8, 0xFF); outp(0x3C9, 0); outp(0x3C9, 0); outp(0x3C9, 0);

    while (Order < 0xD) {
        //i++;
        if (notimer==0) {i = fx1_tick;} else {i++;}
        
        if (noretrace == 0 ) {
            while ((inp(0x3DA) & 8) == 8) {}
            while ((inp(0x3DA) & 8) != 8) {}
        }
        
        if ((Order == 9) && (e >  0)) {e -= (16 * fx1_inc); e = (e < 0 ? 0 : e); fx1_FadeIn(e, 0, 255);}
        if ((Order != 0xC) && (e == 0)) {fx1_FadeOut(256 + (sintab[(i << 12) & 0xFFFF] >> 11));}
        if ((Order == 0xC) && (Row >= 48) && (f > 0)) {f -= (32 * fx1_inc); f = (f < 0 ? 0 : f); fx1_FadeOut(f);}
        
        fx1_inc = ((notimer == 0) ? 0 : 1);
        
        memcpy(screen, fx1_buffer, (((lowres == 1) && (fakelowres == 0)) ? 32000 : 64000));   
        
        fx1_3dMovep(0, 0, 0);
        
        if (i < (fx1_count >> 1)) {
            fx1_3dRotate(0,0,0);
            fx1_3dMove((int)(32 * sintabf[(i << 10) & 0xFFFF]),
                       (int)(32 * costabf[(i << 10) & 0xFFFF]),
                       (int)(i << 2) - (fx1_count));
        }
        else if (i < (fx1_count + fx1_count >> 1)) {
            fx1_3dRotate(0,0,0);
            fx1_3dMove((int)(32 * sintabf[(i << 10) & 0xFFFF]),
                       (int)(32 * costabf[(i << 10) & 0xFFFF]),
                       (int)-((i - ((fx1_count << 1) - (fx1_count >> 1)) << 1) + fx1_count));
        } else {
        
        fx1_3dMovep(0, 0, (fx1_count >> 1));
        fx1_3dRotate(((i << 6) + (i << 8) ) & 0xFFFF, (i << 8) & 0xFFFF, ((i << 6) + (i << 7)) & 0xFFFF);
        
        fx1_3dMove  ((int)(32 * sintabf[(i << 10) & 0xFFFF]),
                    (int)(32 * costabf[(i << 10) & 0xFFFF]),
                    (int) -DIST);
        }   
        
        fx1_3dProject();
        //memset(fxbuffer, 0, 64000);
        fx1_FillBuffer();
        fx1_DrawPoints();

        if (kbhit()) {if (getch() == 27) {KillAll();}}
    }

}