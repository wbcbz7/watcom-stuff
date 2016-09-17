
#define fdDIST     150
#define fdspr_size 16
#define fdcount    64
#define fdist_x    256
#define fdist_z    4096

unsigned char  fdtexture[65536];
unsigned char  fdbuffer[64000];
unsigned char  fdlightmap[64000];

unsigned short fdu[64320], fdv[64320];

unsigned char  fdpalxlat[65536];

unsigned char fdspr[fdspr_size * fdspr_size];

vertex    fdp[fdcount], fdpt[fdcount];
vertex2d  fdp2d[fdcount];

unsigned char fd_pal[1024];

void fdFillDots () {
    int i, lptr = 0;  
    
    for (i = 0; i < fdcount; i++) {
        fdp[lptr  ].x = (rand() % fdist_x) - (fdist_x >> 1);
        fdp[lptr  ].y = (rand() % fdist_x) - (fdist_x >> 1);
        fdp[lptr++].z = (rand() % fdist_x) - (fdist_x >> 1); 
    }
}


void fdMakeSprite() {
    int x, y, k;
    long sprptr = (long)&fdspr;
    
    for (y = -(fdspr_size >> 1); y < (fdspr_size >> 1); y++) {
        for (x = -(fdspr_size >> 1); x < (fdspr_size >> 1); x++) {
            *((char*)sprptr++) = sat((int)(0x80 / ((x*x + y*y) + ee)), 128) & 0xFF;
        }
    }
}

void fd3dMove (float ax, float ay, float az) {
    int i;
    
    for (i = 0; i < fdcount; i++) {
        fdpt[i].x += ax;
        fdpt[i].y += ay;
        fdpt[i].z += az;
    }
}

void fd3dProject() {
    int i;
    float t;
    for (i = 0; i < fdcount; i++) if (fdpt[i].z < 0) {
        t = fdDIST / (fdpt[i].z + ee);
        fdp2d[i].x = fdpt[i].x * t + 160;
        fdp2d[i].y = fdpt[i].y * t + 100;
    }
}

void fdDrawPoints () {
    int i, y, x, j;
    int px, py, ofs;
    long scrptr = (long)&fdbuffer;
    long sprptr = (long)&fdspr;
    
    for (i = 0; i < fdcount; i++) {
        sprptr = (long)&fdspr;
        px = fdp2d[i].x - (fdspr_size >> 1); py = fdp2d[i].y - (fdspr_size >> 1);
        
        if ((py<(200 - (fdspr_size)))&&(py>0)&&(px>0)&&(px<(312 - (fdspr_size)))&&(fdpt[i].z < 0)) {
            
            scrptr = (long)&fdbuffer + ((py << 8) + (py << 6) + px);
            
            if ((lowres == 0) | (fakelowres == 1))
            for (y = 0; y < (fdspr_size); y++) {
                for (x = 0; x < (fdspr_size); x++) {
                    *((char*)scrptr++) = sat((*((char*)scrptr) + *((char*)sprptr)), 255);
                    sprptr++;// scrptr++;
                }
                scrptr += (320 - (fx1_spr_size));
            }
            else {
            px = (px >> 1);
            scrptr = (long)&fdbuffer + ((py << 7) + (py << 5) + px);
            for (y = 0; y < (fdspr_size); y++) {
                for (x = 0; x < (fdspr_size >> 1); x++) {
                    *((char*)scrptr) = sat((*((char*)scrptr) + *((char*)sprptr)), 255);
                    scrptr++; sprptr +=2 ;
                }
                scrptr += (160 - (fx1_spr_size >> 1));
            } 
            }
        }
    }
}

void fdNormalize(vertex *v) {
	float l = 1.0 / sqrt(v->x*v->x + v->y*v->y + v->z*v->z);

	v->x *= l;
	v->y *= l;
	v->z *= l;
}

void fdInterpolateVertical() {
    int  x, y, i, j, k;
    long gridptr = 0;
    long scrptr = 0;
    unsigned char udyb, vdyb;
    int  uy0, uy1, udy, vy0, vy1, vdy, uy, vy;
    int  ty0, ty1, tdy, ty;
    
    for (j = 0; j < 25; j++) {
        for (i = 0; i < 40; i++) {
            uy0 = (fdu[gridptr]); uy1 = (fdu[gridptr + (320 * 8)]);
            udy = (uy1 - uy0) << 5 ;
            uy = (uy0 << 8);
            
            vy0 = (fdv[gridptr]); vy1 = (fdv[gridptr + (320 * 8)]);
            vdy = (vy1 - vy0) << 5 ;
            vy = (vy0 << 8);
            
            ty0 = (fdlightmap[gridptr]); ty1 = (fdlightmap[gridptr + (320 * 8)]);
            tdy = (ty1 - ty0) << 5 ;
            ty = (ty0 << 8);
            
            for (k = 0; k < 8; k++) {
                fdu[scrptr] = (uy >> 8);
                fdv[scrptr] = (vy >> 8);
                fdlightmap[scrptr] = (ty >> 8);
                scrptr += 320;
                uy += udy; vy += vdy; ty += tdy;
            }
            gridptr += 8; scrptr -= ((320 * 8) - 8);
        }
        //scrptr += (320 * 3); gridptr += (320 * 3);
        scrptr += (320 * 7); gridptr += (320 * 7);
    }
}


void fdInterpolateHorizontal() {
    int  x, y, i, j, k;
    long scrptr = 0;
    long sptr = (long)&fdbuffer;
    
    unsigned char udxb, vdxb, tdxb;
    int  ux0, ux1, udx, vx0, vx1, vdx, ux, vx;
    int  tx0, tx1, tdx, tx;
    
    for (j = 0; j < 200; j++) {
        for (i = 0; i < 39; i++) {
            ux0 = (fdu[scrptr]); ux1 = (fdu[scrptr + 8]);
            udx = (ux1 - ux0) << 5 ;
            ux = (ux0 << 8);
            
            vx0 = (fdv[scrptr]); vx1 = (fdv[scrptr + 8]);
            vdx = (vx1 - vx0) << 5 ;
            vx = (vx0 << 8);
            
            tx0 = (fdlightmap[scrptr]); tx1 = (fdlightmap[scrptr + 8]);
            tdx = (tx1 - tx0) << 5 ;
            tx = (tx0 << 8);
            
            for (k = 0; k < 8; k++) {
                *((char*)sptr++) = fdpalxlat[(fdtexture[(((vx >> 8) & 0xFF) | (ux & 0xFF00))] << 8) + (tx >> 8)];
                ux += udx; vx += vdx; tx += tdx;
            }
            scrptr += 8;
        }
        scrptr += 8; sptr += 8;
    }
}

void fdInterpolateHorizontall() { // for 160x200 mode (да, бля, отдельная процедура!)
    int  x, y, i, j, k;
    long scrptr = 0;
    long sptr = (long)&fdbuffer;
    
    unsigned char udxb, vdxb, tdxb;
    int  ux0, ux1, udx, vx0, vx1, vdx, ux, vx;
    int  tx0, tx1, tdx, tx;
    
    for (j = 0; j < 200; j++) {
        for (i = 0; i < 39; i++) {
            ux0 = (fdu[scrptr]); ux1 = (fdu[scrptr + 8]);
            udx = (ux1 - ux0) << 5 ;
            ux = (ux0 << 8);
            
            vx0 = (fdv[scrptr]); vx1 = (fdv[scrptr + 8]);
            vdx = (vx1 - vx0) << 5 ;
            vx = (vx0 << 8);
            
            tx0 = (fdlightmap[scrptr]); tx1 = (fdlightmap[scrptr + 8]);
            tdx = (tx1 - tx0) << 5 ;
            tx = (tx0 << 8);
            
            for (k = 0; k < 4; k++) {
                *((char*)sptr++) = fdpalxlat[(fdtexture[(((vx >> 8) & 0xFF) | (ux & 0xFF00))] << 8) + (tx >> 8)];
                ux += udx; vx += vdx; tx += tdx; ux += udx; vx += vdx; tx += tdx;
            }
            scrptr += 8;
        }
        scrptr += 8; sptr += 4;
    }
}

void fdInterpolateHorizontalf() { // for 160x200 fakemode
    int  x, y, i, j, k;
    long scrptr = 0;
    long sptr = (long)&fdbuffer;
    
    unsigned char udxb, vdxb, tdxb;
    int  ux0, ux1, udx, vx0, vx1, vdx, ux, vx;
    int  tx0, tx1, tdx, tx;
    
    for (j = 0; j < 200; j++) {
        for (i = 0; i < 39; i++) {
            ux0 = (fdu[scrptr]); ux1 = (fdu[scrptr + 8]);
            udx = (ux1 - ux0) << 5 ;
            ux = (ux0 << 8);
            
            vx0 = (fdv[scrptr]); vx1 = (fdv[scrptr + 8]);
            vdx = (vx1 - vx0) << 5 ;
            vx = (vx0 << 8);
            
            tx0 = (fdlightmap[scrptr]); tx1 = (fdlightmap[scrptr + 8]);
            tdx = (tx1 - tx0) << 5 ;
            tx = (tx0 << 8);
            
            for (k = 0; k < 4; k++) {
                *((char*)sptr  ) = fdpalxlat[(fdtexture[(((vx >> 8) & 0xFF) | (ux & 0xFF00))] << 8) + (tx >> 8)];
                *((char*)sptr+1) = *((char*)sptr  ); sptr+=2;
                ux += udx; vx += vdx; tx += tdx; ux += udx; vx += vdx; tx += tdx;
            }
            scrptr += 8;
        }
        scrptr += 8; sptr += 8;
    }
}

void fd3dRotate (int ax, int ay, int az, vertex *v) {
    // hehehe, this code is fully ported from my old freebasic demoz ;)
    int i;
    float sinx = sintabf[ax], cosx = costabf[ax];
    float siny = sintabf[ay], cosy = costabf[ay];
    float sinz = sintabf[az], cosz = costabf[az];
    float bx, by, bz, px, py, pz;  // temp var storage

        //pt[i] = p[i];
        
        py = v->y;
        pz = v->z;
        v->y = (py * cosx - pz * sinx);
        v->z = (py * sinx + pz * cosx);
        
        px = v->x;
        pz = v->z;
        v->x = (px * cosy - pz * siny);
        v->z = (px * siny + pz * cosy);
        
        px = v->x;
        py = v->y;
        v->x = (px * cosz - py * sinz);
        v->y = (px * sinz + py * cosz);

} 


void fdbuildTexture() {
    int x, y, i, k=0;
    
    
    for (y = 0; y < 256; y++) {
        for (x = 0; x < 256; x++) {
            fdtexture[((y << 8) + x)] = (x ^ y) | (rand() % 0x100) & 0xFF;
        }
    }
    
    
    // blur our texture
    for (k = 0; k < 4; k++)
    for (i = 0; i < 65536; i++) 
        fdtexture[i] = (fdtexture[(i-1)&0xFFFF] + fdtexture[(i+1)&0xFFFF] + 
                        fdtexture[(i-256)&0xFFFF] + fdtexture[(i+256)&0xFFFF]) >> 2; 
    
}

void fdCalcPlanes(int ax, int ay, int az, int rx, int ry, int rz) {

    float FOV = (1.0f / 50);
    const TunnelSize = 128;
    
    int x, y, z, tptr = 0;
    unsigned short u, v;
    
	vertex origin, direction, intersect;
	float t, l, fx, fy, delta, a, b, c, t1, t2;

    // ASSUMING origin.y = 0!
    
    for (y = 0; y < 26; y++) {
        for (x = 0; x < 40; x++) {
        
            origin.x = az;
            origin.y = 0;  
            origin.z = ax;
            
            
            direction.x = (float)((x << 3) - 160) * FOV;
            direction.z = 1;
            direction.y = (float)((y << 3) - 100) * FOV;
            
            
            fdNormalize(&direction);
            fd3dRotate(rx, ry, rz, &direction);
            
            a = sqr(direction.x) + sqr(direction.y);
	        b = 2 * (origin.x * direction.x + origin.y * direction.y);
	        c = sqr(origin.x) + sqr(origin.y) - sqr(TunnelSize);

	        delta = sqrt(b * b - 4 * a * c);

	        t1 = (-b + delta) / (2 * a + ee);
	        t2 = (-b - delta) / (2 * a + ee);

	        t = (t1 > 0 ? t1 : t2);
            
            intersect.x = origin.x + (direction.x * t);
            intersect.y = origin.y + (direction.y * t);
            intersect.z = origin.z + (direction.z * t);
            
            u = (unsigned short)(fabs(intersect.z)/pi);
	        v = (unsigned short)((fabs(atan2(intersect.y, intersect.x))*256/pi));
            
            fdu[tptr] = u;
            fdv[tptr] = v;
            
            if (t > bb) {
                fdu[tptr] = 0;
                fdv[tptr] = 0;
                fdlightmap[tptr] = 0;
            } else {
                t = (TunnelSize * 96) / t;
	            z = sat(t, 255);
                fdlightmap[tptr] = z;
            }
            
            
            tptr += 8;
        }
        tptr += (320 * 7);
    }
}

void fd_initPal() {
    int i;

    for (i = 0; i < 256; i++) {
        fd_pal[(i<<2) + 0] = sat(((i >> 2) + (i >> 4)), 63);
        fd_pal[(i<<2) + 1] = sat(((i >> 2) + (i >> 3)), 63);
        fd_pal[(i<<2) + 2] = sat(((i >> 2) + (i >> 3) + (i >> 4)), 63);
    }
}

void fd_FadeInv(int scale) {
    int i, j;
    
    outp(0x3C8, 0);
    for (i = 0; i < 256; i++) {
        outp(0x3C9, (((63 - fd_pal[(i<<2) + 0]) * scale) >> 8) + (fd_pal[(i<<2) + 0] * (256 - scale) >> 8));
        outp(0x3C9, (((63 - fd_pal[(i<<2) + 1]) * scale) >> 8) + (fd_pal[(i<<2) + 1] * (256 - scale) >> 8));
        outp(0x3C9, (((63 - fd_pal[(i<<2) + 2]) * scale) >> 8) + (fd_pal[(i<<2) + 2] * (256 - scale) >> 8));
    }
}


void fd_FadeOut(int scale) {
    int i, j;
    
    outp(0x3C8, 0);
    for (i = 0; i < 256; i++) {
        outp(0x3C9, sat((fd_pal[(i << 2) + 0] * scale) >> 8, 63));
        outp(0x3C9, sat((fd_pal[(i << 2) + 1] * scale) >> 8, 63));
        outp(0x3C9, sat((fd_pal[(i << 2) + 2] * scale) >> 8, 63));
    }
}

void fd_FadeIn(int scale, int start, int stop) {
    int i, j;
    
    outp(0x3C8, 0);
    for (i = start; i <= stop; i++) {
        outp(0x3C9, sat(((63 * scale) >> 8) + (fd_pal[(i<<2) + 0] * (256 - scale) >> 8), 63));
        outp(0x3C9, sat(((63 * scale) >> 8) + (fd_pal[(i<<2) + 1] * (256 - scale) >> 8), 63));
        outp(0x3C9, sat(((63 * scale) >> 8) + (fd_pal[(i<<2) + 2] * (256 - scale) >> 8), 63));
    }
}

void fd1_init() {
    int i,j,p=0;
    
    fdbuildTexture();
    fdFillDots();
    fdMakeSprite();
    fd_initPal();
    for (j = 0; j < 256; j++) for (i = 0; i < 256; i++) fdpalxlat[p++] = (i * j) >> 8;
}

int fd1_tick, fd1_inc=0;

void fd1_timer() {
    fd1_tick++; fd1_inc++;
}

void fd1_main() {
    int i, j, p = 0;
    int e = 0;
    int f = 0;
    
    if ((lowres ==0) || (fakelowres == 1)) {
        outpw(0x3D4, 0xFF0D);
        outpw(0x3D4, 0xFF0C);
    }
    
    // это дерьмо надо разгребать :)
    
    if (notimer==0) {rtc_setTimer(&fd1_timer, rtc_timerRate / 60);}
    
    fd_FadeOut(e);
    while (Order < 0x26) {
        if (notimer==0) {i = fd1_tick;} else {i++;}
        
        if (noretrace==0) {
            while ((inp(0x3DA) & 8) == 8) {}
            while ((inp(0x3DA) & 8) != 8) {}
        }
        
        if ((Order == 0x1D) && (e < 256)) {e += (4 * fd1_inc); e = (e < 0 ? 0 : e); fd_FadeOut(e); }
        if ((Order != 0x1D) && (Row < 2)) {f = 256;}
        if (f > 0) {f -= (8 * fd1_inc); f = (f < 0 ? 0 : f); fd_FadeInv(f);}
        if ((f <= 0) && (Order < 0x26) && (Order > 0x21)) {fd_FadeOut(224 + (rand() & 63));}
        fd1_inc = ((notimer == 0) ? 0 : 1);
        
        memcpy(screen, &fdbuffer, (((lowres == 1) && (fakelowres == 0)) ? 32000 : 64000));
        
        fdCalcPlanes((i << 4), 0, 0, 
                    ((i << 6) & 0xFFFF), ((i << 7) & 0xFFFF), ((i << 8) & 0xFFFF));
       
        fdInterpolateVertical();
        if (lowres == 0) fdInterpolateHorizontal(); else {if (fakelowres == 0) fdInterpolateHorizontall(); else fdInterpolateHorizontalf();}
        
        for (j = 0; j < (fdcount); j++) {
            fdpt[j] = fdp[j];
            fd3dRotate(((i << 6) & 0xFFFF), ((i << 7) & 0xFFFF), ((i << 8) & 0xFFFF), &fdpt[j]);
        }
        fd3dMove(0, 0, 0);
        fd3dProject();
        fdDrawPoints();
        
        if (kbhit()) {if (getch() == 27) {KillAll();}}
    }
    if (notimer==0) {Timer_Stop(&fd1_timer);}
}
