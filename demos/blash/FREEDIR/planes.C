#include <stdlib.h>
#include <conio.h>
#include <math.h>

unsigned char  *screen = 0xA0000;


unsigned char  fd2texture[65536];
unsigned char  fd2buffer[64000];
unsigned char  fd2lightmap[64000];

unsigned short fd2u[64320], fd2v[64320];

unsigned char  fd2palxlat[65536];

         short sintab [65536];
         float sintabf[65536], costabf[65536];

typedef struct vec3f {
    float x, y, z;
} vec3f;

#define sat(a, l) ((a > l) ? l : a)
#define sqr(a)    ((a)*(a))
#define sgn(a)    (((a) > 0) ? 1 : -1)

#define ee        1.0E-6
#define bb        1.0E+6 
#define pi        3.141592653589793

void fd2Normalize(vec3f *v) {
	float l = 1.0 / sqrt(v->x*v->x + v->y*v->y + v->z*v->z);

	v->x *= l;
	v->y *= l;
	v->z *= l;
}

//void fd2Interpolate() {

//}

void fd2InterpolateVertical() {
    int  x, y, i, j, k;
    long gridptr = 0;
    long scrptr = 0;
    unsigned char udyb, vdyb;
    int  uy0, uy1, udy, vy0, vy1, vdy, uy, vy;
    int  ty0, ty1, tdy, ty;
    
    for (j = 0; j < 25; j++) {
        for (i = 0; i < 40; i++) {
            uy0 = (fd2u[gridptr]); uy1 = (fd2u[gridptr + (320 * 8)]);
            udy = (uy1 - uy0) << 5 ;
            uy = (uy0 << 8);
            
            vy0 = (fd2v[gridptr]); vy1 = (fd2v[gridptr + (320 * 8)]);
            vdy = (vy1 - vy0) << 5 ;
            vy = (vy0 << 8);
            
            ty0 = (fd2lightmap[gridptr]); ty1 = (fd2lightmap[gridptr + (320 * 8)]);
            tdy = (ty1 - ty0) << 5 ;
            ty = (ty0 << 8);
            
            for (k = 0; k < 8; k++) {
                fd2u[scrptr] = (uy >> 8);
                fd2v[scrptr] = (vy >> 8);
                fd2lightmap[scrptr] = (ty >> 8);
                scrptr += 320;
                uy += udy; vy += vdy; ty += tdy;  
            }
            gridptr += 8; scrptr -= ((320 * 8) - 8);
        }
        //scrptr += (320 * 3); gridptr += (320 * 3);
        scrptr += (320 * 7); gridptr += (320 * 7);
    }
}


void fd2InterpolateHorizontal() {
    int  x, y, i, j, k;
    long scrptr = 0;
    long sptr = (long)&fd2buffer;
    
    unsigned char udxb, vdxb, tdxb;
    int  ux0, ux1, udx, vx0, vx1, vdx, ux, vx;
    int  tx0, tx1, tdx, tx;
    
    for (j = 0; j < 200; j++) {
        for (i = 0; i < 39; i++) {
            ux0 = (fd2u[scrptr]); ux1 = (fd2u[scrptr + 8]);
            udx = (ux1 - ux0) << 5 ;
            ux = (ux0 << 8);
            
            vx0 = (fd2v[scrptr]); vx1 = (fd2v[scrptr + 8]);
            vdx = (vx1 - vx0) << 5 ;
            vx = (vx0 << 8);
            
            tx0 = (fd2lightmap[scrptr]); tx1 = (fd2lightmap[scrptr + 8]);
            tdx = (tx1 - tx0) << 5 ;
            tx = (tx0 << 8);
            
            for (k = 0; k < 8; k++) {
                *((char*)sptr++) = fd2palxlat[(fd2texture[(((vx >> 8) & 0xFF) | (ux & 0xFF00))] << 8) + (tx >> 8)];
                ux += udx; vx += vdx; tx += tdx; scrptr++;
            }
        }
        scrptr += 8; sptr += 8;
    }
}


void fd2buildSinTable() {
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

void fd23dRotate (int ax, int ay, int az, vec3f *v) {
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


void fd2buildTexture() {
    int x, y, i, k=0;
    
    
    for (y = 0; y < 256; y++) {
        for (x = 0; x < 256; x++) {
            fd2texture[((y << 8) + x)] = sat((x ^ y), 255) & 0xFF;
        }
    }
    
    
    // blur our texture
    for (k = 0; k < 20; k++)
    for (i = 0; i < 65536; i++) 
        fd2texture[i] = (fd2texture[(i-1)&0xFFFF] + fd2texture[(i+1)&0xFFFF] + 
                        fd2texture[(i-256)&0xFFFF] + fd2texture[(i+256)&0xFFFF]) >> 2; 
    
}

void fd2CalcPlanes(int ax, int ay, int az, int rx, int ry, int rz) {

    float FOV = (1.0f / 120);
    const PlaneSize = 32;
    
    int x, y, z, tptr = 0;
    unsigned short u, v;
    
	vec3f origin, direction, intersect;
	float t, l, fx, fy;

    // ASSUMING origin.y = 0!
    
    for (y = 0; y < 26; y++) {
        for (x = 0; x < 40; x++) {
        
            origin.x = ax;
            origin.y = 0;  
            origin.z = az;
            
            
            direction.x = (float)((x << 3) - 160) * FOV;
            direction.z = 1;
            direction.y = (float)((y << 3) - 100) * FOV;
            
            
            fd2Normalize(&direction);
            fd23dRotate(rx, ry, rz, &direction);
            
            //t = ((sgn(direction.y) * PlaneSize) - origin.y) / direction.y;
            t = (PlaneSize) / fabs(direction.y);

            
            intersect.x = origin.x + (direction.x * t);
            intersect.y = origin.y + (direction.y * t);
            intersect.z = origin.z + (direction.z * t);

            u = (unsigned short)((intersect.x));
            v = (unsigned short)((intersect.z));
            
            
            /*
            intersect.x = origin.x + (direction.x * t);
            u = (unsigned short)((intersect.x));
            
            intersect.z = origin.z + (direction.z * t);
            v = (unsigned short)((intersect.z)); 
            */
            
            fd2u[tptr] = u;
            fd2v[tptr] = v;
            //fd2table[tptr] = (u << 8) + v;
            
            
            t = sqr(intersect.x - origin.x) + sqr(intersect.z - origin.z);
	        if (t >= bb) {
                z = 0;
                fd2u[tptr] = 0;
                fd2v[tptr] = 0;
                }
            else {
                t = (PlaneSize * 256) / sqrt(t);
                z = (char)sat(t, 255);
            }
            fd2lightmap[tptr] = z;
            
            tptr += 8;
        }
        tptr += (320 * 7);
    }
}


int main() {
    int i, j, p = 0;
    
    fd2buildSinTable();
    fd2buildTexture();
    
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
        outp(0x3C9, sat(((i >> 2) + (i >> 3)), 63)); 
        outp(0x3C9, sat(((i >> 2)), 63));
   
    }
    for (j = 0; j < 256; j++) for (i = 0; i < 256; i++) fd2palxlat[p++] = (i * j) >> 8;
    
    while (!kbhit()) {
        i++;
        
        while ((inp(0x3DA) & 8) == 8) {}
        while ((inp(0x3DA) & 8) != 8) {}

        //outp(0x3C8, 0); outp(0x3C9, 63);
        
        _asm {
            mov  esi, offset fd2buffer
            mov  edi, screen
            mov  ecx, 16000
            rep  movsd  
        }
        
        
        fd2CalcPlanes((i << 2), 0, 64, 
                    ((i << 6) & 0xFFFF), ((i << 7) & 0xFFFF), ((i << 8) & 0xFFFF));           
        fd2InterpolateVertical();
        fd2InterpolateHorizontal();
        
        outp(0x3C8, 0); outp(0x3C9, 0); outp(0x3C9, 0); outp(0x3C9, 0); 
    }
    getch();
    
    _asm {
        mov  ax, 3
        int  10h
    }
}
