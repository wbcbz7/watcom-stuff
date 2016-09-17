#include <stdlib.h>
#include <conio.h>
#include <math.h>

unsigned char  *screen = 0xA0000;


unsigned char  fdtexture[65536];
unsigned char  fdbuffer[64000];
unsigned char  fdlightmap[64000];

unsigned short fdu[64320], fdv[64320];

unsigned char  fdpalxlat[65536];

         short sintab [65536];
         float sintabf[65536], costabf[65536];

typedef struct vec3f {
    float x, y, z;
} vec3f;

typedef struct vec2 {
    int x, y;
} vec2;

#define sat(a, l)  ((a > l) ? l : a)
#define sqr(a)     ((a)*(a))
#define sgn(a)     (((a) > 0) ? 1 : -1)

#define fdDIST     150
#define fdspr_size 16
#define fdcount    64
#define fdist_x    256
#define fdist_z    4096

#define ee         1.0E-6
#define bb         2 * 1.0E+4
#define pi         3.141592653589793

unsigned char fdspr[fdspr_size * fdspr_size];

vec3f fdp[fdcount], fdpt[fdcount];
vec2  fdp2d[fdcount];


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
            j = 0;
            scrptr = (long)&fdbuffer + ((py << 8) + (py << 6) + px);
            for (y = 0; y < (fdspr_size); y++) {
                for (x = 0; x < (fdspr_size); x++) {
                    *((char*)scrptr) = sat((*((char*)scrptr) + *((char*)sprptr)), 255);
                    scrptr++; sprptr++;
                }
                scrptr += (320 - (fdspr_size));
            }
        }
    }
}

void fdNormalize(vec3f *v) {
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
                *((char*)sptr++) = fdtexture[(((vx >> 8) & 0xFF) | (ux & 0xFF00))];
                //*((char*)sptr++) = (fdtexture[(((vx >> 8) & 0xFF) | (ux & 0xFF00))] * (tx >> 8)) >> 8;
                ux += udx; vx += vdx; tx += tdx; scrptr++;
                
            }
        }
        scrptr += 8; sptr += 8;
    }
}


void fdbuildSinTable() {
    int i, j;
    float r;
    
    for (i = 0; i < 65536; i++) {
        r = (sin(2 * pi * i / 65536));
        sintab[i] = 32767 * r;
        sintabf[i] = r;
        r = (cos(2 * pi * i / 65536));
        costabf[i] = r;
    }
}

void fd3dRotate (int ax, int ay, int az, vec3f *v) {
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
            //fdtexture[((y << 8) + x)] = sat((x ^ y), 255) & 0xFF;
            //texture[((y << 8) + x)] = (x ^ y) & 0xFF;
            fdtexture[((y << 8) + x)] = (x ^ y) | (rand() % 0x100) & 0xFF;
        }
    }
    
    /*
    for (i = 0; i < 2; i++)
    for (y = -128; y < 127; y++) {
        for (x = -128; x < 127; x++) {
            fdtexture[k++] = sat((int)(0x80000 / ((x*x | y*y) + ee)), 255) & 0xFF;
            //texture[((y << 8) + x)] = (x ^ y) & 0xFF;
            //ttexture[((y << 8) + x)] = (rand() % 0x100) & 0xFF;
        }
    }
    */
    
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
    
	vec3f origin, direction, intersect;
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

	        t = t1 > 0 ? t1 : t2;
            
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

/*
void fdDrawPlanes() {
    int i, j, k;
    long scrptr = (long)&fdbuffer;
    
    
    for (i = 0; i < 64000; i++) {
        //*((char*)scrptr++) = fdtexture[fdtable[i]];
        *((char*)scrptr++) = fdpalxlat[(fdtexture[fdtable[i]] << 8) + fdlightmap[i]];
        //*((char*)scrptr++) = (fdtexture[fdtable[i]] * fdlightmap[i]) >> 8;
    }
    
    
    
    _asm {
        xor    eax, eax
        xor    edx, edx
        mov    ecx, 64000
        mov    edi, offset fdbuffer
        mov    esi, offset fdtexture
        mov    ebx, offset fdtable
        mov    edx, offset fdlightmap

        @@inner:
        mov    ax, [ebx]
        mov    al, [esi + eax]
        mov    ah, [edx]
        mul    ah  // result in ax
        shr    eax, 8
        stosb
        add    ebx, 2
        inc    edx
        dec    ecx
        jnz    @@inner  
    }
    
}
*/

int main() {
    int i, j, p = 0;
    
    fdbuildSinTable();
    fdbuildTexture();
    fdFillDots();
    fdMakeSprite();
    
    _asm {
        mov  ax, 13h
        int  10h
    } 
    _asm {    
        // zpizzheno ;)
        mov ax,13h
        int 10h       // regular mode 13h chained

        mov dx,3d4h   // remove protection
        mov al,11h
        out dx,al
        inc dl
        in  al,dx
        and al,7fh
        out dx,al

        mov dx,3c2h   // misc output
        mov al,0e3h   // clock
        out dx,al

        mov dx,3d4h
        mov ax,00B06h // Vertical Total
        out dx,ax
        mov ax,03E07h // Overflow
        out dx,ax
        mov ax,0C310h // Vertical start retrace
        out dx,ax
        mov ax,08C11h // Vertical end retrace
        out dx,ax
        mov ax,08F12h // Vertical display enable end
        out dx,ax
        mov ax,09015h // Vertical blank start
        out dx,ax
        mov ax,00B16h // Vertical blank end
        out dx,ax
        
        mov ax,0FF0Dh
        out dx,ax
        mov ax,0FF0Ch
        out dx,ax
    }
    
    outp(0x3C8, 0);
    for (i = 0; i < 256; i++) {
        
        outp(0x3C9, sat(((i >> 3) + (i >> 4)), 63));
        outp(0x3C9, sat(((i >> 3) + (i >> 3)), 63));
        outp(0x3C9, sat(((i >> 3) + (i >> 3) + (i >> 4)), 63)); 
        
        /*
        outp(0x3C9, sat((i >> 2), 63));
        outp(0x3C9, sat((i >> 2), 63)); 
        outp(0x3C9, sat((i >> 2), 63));
        */    
    }
    for (j = 0; j < 256; j++) for (i = 0; i < 256; i++) fdpalxlat[p++] = (i * j) >> 8;
    
    while (!kbhit()) {
        i++;
        
        while ((inp(0x3DA) & 8) == 8) {}
        while ((inp(0x3DA) & 8) != 8) {}

        outp(0x3C8, 0); outp(0x3C9, 63);
        
        memcpy(screen, &fdbuffer, 64000);
        
        
        fdCalcPlanes((i << 4), 0, 0, 
                    ((i << 6) & 0xFFFF), ((i << 7) & 0xFFFF), ((i << 8) & 0xFFFF));
       
        fdInterpolateVertical();
        fdInterpolateHorizontal();
        
        /*
        for (j = 0; j < (fdcount); j++) {
            fdpt[j] = fdp[j];
            fd3dRotate(((i << 6) & 0xFFFF), ((i << 7) & 0xFFFF), ((i << 8) & 0xFFFF), &fdpt[j]);
        }
        fd3dMove(0, 0, 0);
        fd3dProject();
        fdDrawPoints();
        */
        
        outp(0x3C8, 0); outp(0x3C9, 0); outp(0x3C9, 0); outp(0x3C9, 0); 
    }
    getch();
    
    _asm {
        mov  ax, 3
        int  10h
    }
}
