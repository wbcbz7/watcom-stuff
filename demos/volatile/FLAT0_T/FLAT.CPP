#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <math.h>
#include <i86.h>
#include "..\flexptc\flexptc.h"

#define X_RES 320
#define Y_RES 200

#define sat(a, l) (a < l ? a : l)
#define sqr(a)    (a * a)
#define ee         1.0E-6
#define bb        (1.2f * 1.0E+2)
#define pi         3.141592653589793
#define mk15(r, g, b) ((b << 0) | (g << 5) | (r << 10))

// extern sin\cos tables
extern signed int sintab[65536],  costab[65536];
extern float      sintabf[65536], costabf[65536];
#include "matrix.h"
#include "vecdef.h"
#include "tmap.h"

// class definition
#include "flat.h"

flat::flat() {
    blurtab     = new unsigned short[X_RES*Y_RES*2];
    buffer      = new unsigned short[X_RES*Y_RES];
    buffer2     = new unsigned short[X_RES*Y_RES];
    pz          = new float[flarescount];
    p           = new vec4f[flarescount];
    pt          = new vec4f[flarescount];
    pn          = new vec4f[flarescount];
    p2d         = new vec2f[flarescount];
    puv         = new uv   [flarescount];
    f           = new line [linesnum];
    ff          = new face [facenum];
    fz          = new face [facenum];
    
    // allocate memory for texture and align pointer to 128kb boundary
    texbuf      = new unsigned short[2*65536];
    texture     = (unsigned short*)(((unsigned int)texbuf + 0x1FFFF) & ~0x1FFFF);
    
    FILE *f = fopen("dutch.tga", "rb");
    if (f) {
        fseek(f, 18, SEEK_SET);
        fread(texture, sizeof(unsigned short), 65536, f);
        fclose(f);
    }
    
    // call init stuff
    initblur();
    //inittexture();
    initvertex(mk15(4,4,4));
    
    memset(pz,      0, sizeof(float)*flarescount);
    memset(buffer,  0, sizeof(unsigned short)*X_RES*Y_RES);
    memset(buffer2, 0, sizeof(unsigned short)*X_RES*Y_RES);
}

flat::~flat() {
    delete[] blurtab;
    delete[] buffer;
    delete[] buffer2;
    delete[] p;
    delete[] pt;
    delete[] pn;
    delete[] p2d;
    delete[] puv;
    delete[] f;
    delete[] pz;
    delete[] ff;
    delete[] fz;
    
    delete[] texbuf;
}

void flat::inittexture() {
    unsigned short *p = texture;
    
    for (int y = -128; y < 128; y++) {
        for (int x = -128; x < 128; x++) {
            float dist = 2*(127 - sqrt(x*x + y*y)); if (dist < 0) dist = 0;
            
            int r = (dist * 0xFF);
            int g = (dist * 0xFF);
            int b = (dist * 0xFF);
            
            *p++ = ((b >> 11) << 0) | ((g >> 11) << 5) | ((r >> 11) << 10);
        }
    }
}

void flat::initvertex(int color) {
    uv    *tuv = puv;
    vec4f *t   = p;
    line  *v   = f;
    face  *z   = ff;
    
    // init vertices
    for (int j = 0; j < flaresnum; j++) {
        for (int k = 0; k < flaresnum; k++) {
            t->x   = (flaredist * k) - (flaredist * (flaresnum - 1) * 0.5);
            t->y   = (flaredist * j) - (flaredist * (flaresnum - 1) * 0.5);
            t->z   = 0;
            t->w   = 1;
            tuv->u = (256.0/flaresnum) * j;
            tuv->v = (256.0/flaresnum) * k;
            t++; tuv++;
        }
    }
    
    unsigned int l = 0;
    
    // init lines
    for (int i = 0; i < flaresnum; i++) {
        for (int j = 0; j < flaresnum-1; j++) {
            v->a = (flaresnum * i) + j;
            v->b = (flaresnum * i) + j + 1;
            v->col = color;
            v++;
        }
    }
    for (int i = 0; i < flaresnum-1; i++) {
        for (int j = 0; j < flaresnum; j++) {
            v->a = (flaresnum * i) + j;
            v->b = (flaresnum * (i + 1)) + j;
            v->col = color;
            v++;
        }
    }
    
    memset(z, 0, sizeof(face)*facenum);
    // init faces
    for (int i = 0; i < flaresnum-1; i++) {
        for (int j = 0; j < flaresnum-1; j++) {
            z->a = (flaresnum * i) + j;
            z->b = (flaresnum * (i+1)) + j + 1;
            z->c = (flaresnum * i) + j + 1;
            z++;
        }
    }
    for (int i = 0; i < flaresnum-1; i++) {
        for (int j = 0; j < flaresnum-1; j++) {
            z->a = (flaresnum * i) + j;
            z->b = (flaresnum * (i + 1)) + j;
            z->c = (flaresnum * (i + 1)) + j + 1;
            z++;
        }
    }
}

void flat::linedraw(vec2f *a, vec2f *b, line *f) {

    int delta_x, delta_y, dx, dy, t, d;
    int xerr = 0, yerr = 0;
    
    int sx, sy;
    unsigned short *p = buffer;
    long col;
    
    // determine dx and dy
    delta_x = b->x - a->x;
    delta_y = b->y - a->y;
    // determine steps by x and y axes (it will be +1 if we move in forward
    // direction and -1 if we move in backward direction
    if (delta_x > 0) dx = 1; else if (delta_x == 0) dx = 0; else dx = -1;
    if (delta_y > 0) dy = 1; else if (delta_y == 0) dy = 0; else dy = -1;
    delta_x = abs(delta_x);
    delta_y = abs(delta_y);
    // select largest from deltas and use it as a main distance
    if (delta_x > delta_y) d = delta_x; else d = delta_y;
    
    sx = a->x; sy = a->y;
    for (t = 0; t <= d; t++)	{	
        p = &buffer[(sy << 8) + (sy << 6) + (sx & ~1)];
        //col = 64;
        col = f->col << (sx & 1 ? 16 : 0);
        
        // saturated rgb555 addition from hugi #18 by BRAD
        // a bit modified by wbcbz7 :)
        _asm {
            mov     edi, p

            mov     eax, [edi]          // 1
            mov     ebx, col            // .
            mov     ecx, eax            // 2
            mov     edx, ebx            // .
            and     eax, 0x3DEF3DEF     // 3 <- msb inverted mask
            and     ebx, 0x3DEF3DEF     // .
            and     edx, 0x42104210     // 4 <- msb mask
            add     eax, ebx            // .
            and     ecx, 0x42104210     // 5
            mov     ebx, edx            // .
            or      edx, ecx            // 6 
            and     ebx, ecx            // .
            mov     ecx, edx            // 7
            and     edx, eax            // .
            or      eax, ecx            // 8
            or      edx, ebx            // .
            shr     edx, 4              // 9
            add     edi, 4              // .
            add     edx, 0x3DEF3DEF     // 10 <- add fixup
            xor     edx, 0x3DEF3DEF     // 11 <- xor fixup
            or      eax, edx            // 12 (stall)
            mov     [edi - 4], eax      // 13 (another stall)
        }
        
        // increasing error
        xerr += delta_x;
        yerr += delta_y;
        // if error is too big then we should decrease it by changing
        // coordinates of the next plotting point to make it closer
        // to the true line
        if (xerr > d) {	
            xerr -= d;
            sx += dx;
        }	
        if (yerr > d) {	
            yerr -= d;
            sy += dy;
        }	
    }
}

#define vcode(p) (((p->x < 0) ? 1 : 0) | ((p->x >= X_RES) ? 2 : 0) | ((p->y < 0) ? 4 : 0) | ((p->y >= Y_RES) ? 8 : 0))    
 
int flat::lineclip(vec2f *a, vec2f *b) {
    int code_a, code_b, code;
    vec2f *c;
 
    code_a = vcode(a);
    code_b = vcode(b);
 
    while (code_a || code_b) {
        if (code_a & code_b)
            return -1;
 
        if (code_a) {
            code = code_a;
            c = a;
        } else {
            code = code_b;
            c = b;
        }
 
        if (code & 1) {
            c->y += (a->y - b->y) * (0 - c->x) / (a->x - b->x + ee);
            c->x = 0;
        } else if (code & 2) {
            c->y += (a->y - b->y) * (X_RES - c->x) / (a->x - b->x + ee);
            c->x = X_RES - 1;
        }

        if (code & 4) {
            c->x += (a->x - b->x) * (0 - c->y) / (a->y - b->y + ee);
            c->y = 0;
        } else if (code & 8) {
            c->x += (a->x - b->x) * (Y_RES - c->y) / (a->y - b->y + ee);
            c->y = Y_RES - 1;
        }
 
        if (code == code_a)
            code_a = vcode(a);
        else
            code_b = vcode(b);
    }
 
    return 0;
}

void flat::initblur() {
    unsigned short *p = blurtab;
    float n, nx, ny;
    //float scale = 0.8;
    //float scale = 0x20000;
    float scale = 0x10000;
    
    for (int y = 0; y < Y_RES; y++) {
        for (int x = 0; x < X_RES; x++) {
            //n = scale / (x*x + y*y);
            //n = sqrt(x*x + y*y) / 8 + 8;
            n = 6;
            nx = (x / n);
            ny = (y / n);
            *p++ = (unsigned short)ny * X_RES;
            *p++ = (unsigned short)nx;
            //*p++ = (unsigned short)(ny > Y_RES ? Y_RES : (int)ny * X_RES);
            //*p++ = (unsigned short)(nx > X_RES ? X_RES : nx);
        }
    }
}

void flat::blur(int sx, int sy) {
    unsigned short *p = buffer;
    unsigned short *v = buffer2;
    unsigned short *t = blurtab;
    
    const unsigned int BLENDMASK = 0x7BDE;
    int t_add, p_add, x_count, y_count;
    
    // upper left corner
    t = blurtab;
    p = &v[(sx-1) + ((sy-1) << 6) + ((sy-1) << 8)];
    /*
    for (int y = sy-1; y >= 0; y--) {
        for (int x = sx-1; x >= 0; x--) {
            int ty = *t, tx = *(t + 1);
            *p = (((*p & BLENDMASK) >> 1) + ((*(p + tx + ty) & BLENDMASK) >> 1));
            p--; t += 2;
        }
        t += 2*(X_RES - sx); p -= (X_RES - sx);
    }
    */
    t_add = 4*(X_RES - sx); p_add = -2*(X_RES - sx); x_count = sx; y_count = sy;
    _asm {
        mov     esi, t
        mov     edi, p
        mov     ecx, y_count
        
        _y_loop:
        push    ecx
        mov     ecx, x_count
        xor     eax, eax
        xor     ebx, ebx
        xor     edx, edx
        
        _x_loop:
        mov     ebx, [esi]              // 1
        add     esi, 4                  // .
        mov     dx, bx                  // 2
        mov     ax, [edi]               // .
        shr     ebx, 16                 // 3
        add     edi, -2                 // .
        add     edx, ebx                // 4
        mov     dx, [edi + 2*edx + 2]   // 5
        and     eax, 0x7BDE7BDE         // .
        shr     eax, 1                  // 6
        and     edx, 0x7BDE7BDE         // .
        shr     edx, 1                  // 7
        add     eax, edx                // 8
        dec     ecx                     // .
        mov     [edi + 2], ax           // 9
        jnz     _x_loop                 // .

        pop     ecx
        add     edi, p_add
        add     esi, t_add
        dec     ecx
        jnz     _y_loop
    } 
    
    // upper right corner
    t = blurtab;
    p = &v[sx + ((sy-1) << 6) + ((sy-1) << 8)];
    /*
    for (int y = sy-1; y >= 0; y--) {
        for (int x = sx; x < X_RES; x++) {
            int ty = *t, tx = *(t + 1);
            *p = (((*p & BLENDMASK) >> 1) + ((*(p - tx + ty) & BLENDMASK) >> 1));
            p++; t += 2;
        }
        t += 2*sx; p -= (2*X_RES - sx);
    }
    */
    t_add = 4*sx; p_add = -2*(2*X_RES - sx); x_count = X_RES - sx; y_count = sy;
    _asm {
        mov     esi, t
        mov     edi, p
        mov     ecx, y_count
        
        _y_loop:
        push    ecx
        mov     ecx, x_count
        xor     eax, eax
        xor     ebx, ebx
        xor     edx, edx
        
        _x_loop:
        mov     ax, [edi]               // 1
        mov     ebx, [esi]              // .
        mov     dx, bx                  // 2
        add     esi, 4                  // .
        shr     ebx, 16                 // 3 
        and     eax, 0x7BDE7BDE         // .
        shr     eax, 1                  // 4
        sub     edx, ebx                // .
        mov     dx, [edi + 2*edx]       // 5
        add     edi, 2                  // .
        and     edx, 0x00007BDE         // 6
        shr     edx, 1                  // 7
        add     eax, edx                // 8
        dec     ecx                     // .
        mov     [edi - 2], ax           // 9
        jnz     _x_loop                 // .
        
        pop     ecx
        add     edi, p_add
        add     esi, t_add
        dec     ecx
        jnz     _y_loop
    }    
    
    // lower left corner
    t = blurtab;
    p = &v[(sx-1) + (sy << 6) + (sy << 8)];
    /*
    for (int y = sy; y < Y_RES; y++) {
        for (int x = sx-1; x >= 0; x--) {
            int ty = *t, tx = *(t + 1);
            *p = (((*p & BLENDMASK) >> 1) + ((*(p + tx - ty) & BLENDMASK) >> 1));
            p--; t += 2;
        }
        t += 2*(X_RES - sx); p += (X_RES + sx);
    }
    */
    t_add = 4*(X_RES - sx); p_add = 2*(X_RES + sx); x_count = sx; y_count = Y_RES - sy;
    _asm {
        mov     esi, t
        mov     edi, p
        mov     ecx, y_count
        
        _y_loop:
        push    ecx
        mov     ecx, x_count
        xor     eax, eax
        xor     ebx, ebx
        xor     edx, edx
        
        _x_loop:
        mov     ax, [edi]               // 1
        mov     ebx, [esi]              // .        
        mov     dx, bx                  // 2
        add     esi, 4                  // .        
        shr     ebx, 16                 // 3 
        and     eax, 0x7BDE7BDE         // .        
        shr     eax, 1                  // 4
        sub     edx, ebx                // .       
        xor     edx, 0xFFFFFFFF         // 5
        add     edi, -2                 // .       
        mov     dx, [edi + 2*edx + 4]   // 6        
        and     edx, 0x00007BDE         // 7
        shr     edx, 1                  // 8        
        add     eax, edx                // 9
        dec     ecx                     // .        
        mov     [edi + 2], ax           // 10
        jnz     _x_loop                 // .
        
        pop     ecx
        add     edi, p_add
        add     esi, t_add
        dec     ecx
        jnz     _y_loop
    }    
    
    // lower right corner
    t = blurtab;
    p = &v[sx + (sy << 6) + (sy << 8)];
    /*
    for (int y = sy; y < Y_RES; y++) {
        for (int x = sx; x < X_RES; x++) {
            int ty = *t, tx = *(t + 1);
            *p = (((*p & BLENDMASK) >> 1) + ((*(p - tx - ty) & BLENDMASK) >> 1));
            p++; t += 2;
        }
        t += 2*sx; p += sx;
    }
    */
    t_add = 4*sx; p_add = 2*sx; x_count = X_RES - sx; y_count = Y_RES - sy;
    _asm {
        mov     esi, t
        mov     edi, p
        mov     ecx, y_count
        
        _y_loop:
        push    ecx
        mov     ecx, x_count
        xor     eax, eax
        xor     ebx, ebx
        xor     edx, edx
        
        _x_loop:
        mov     ax, [edi]               // 1
        mov     ebx, [esi]              // .        
        mov     dx, bx                  // 2
        add     esi, 4                  // .        
        shr     ebx, 16                 // 3 
        and     eax, 0x7BDE7BDE         // .       
        shr     eax, 1                  // 4
        add     edx, ebx                // .        
        xor     edx, 0xFFFFFFFF         // 5
        add     edi, 2                  // .        
        mov     dx, [edi + 2*edx]       // 6        
        and     edx, 0x00007BDE         // 7
        shr     edx, 1                  // 8        
        add     eax, edx                // 9
        dec     ecx                     // .        
        mov     [edi - 2], ax           // 10
        jnz     _x_loop                 // .
        
        pop     ecx
        add     edi, p_add
        add     esi, t_add
        dec     ecx
        jnz     _y_loop
    }
}

void flat::project3d(vec2f *f, vec4f *v) {
    float t;
    
    if (v->z < 0) {
        t = dist / (v->z + ee);
        f->x = (v->x * t) + (X_RES >> 1);
        f->y = (v->y * t) + (Y_RES >> 1);
    } else f->x = -1;
}

typedef struct {
    void *p;
    float avg_z;
} sindex;

static int sortcmp(const void *f, const void *v) {
    return (((sindex*)f)->avg_z - ((sindex*)v)->avg_z);
}

void flat::facesort(face *f, face *fz) {
    face tmpbuf;

    // дети, НИКОГДА не используйте сортировку пузырьком, если не хотите замедлить свой
    // код в ШЕСТЬ(!) раз. юзайте qsort, а лучше - radix :)
    
    // конкретно здесь заюзан костыль с использованием qsort
    
    sindex *s = new sindex[facenum];
    
    // calculate avg_z value
    for (int i = 0; i < facenum; i++) {
        s[i].p     = &f[i];
        s[i].avg_z = (pt[f[i].a].z + pt[f[i].b].z + pt[f[i].c].z) / 3;
    }
    
    // sort sindex array
    qsort(s, facenum, sizeof(sindex), &sortcmp);
    
    //  we got a sorted array, expand it then
    face *vz = fz; // local z-sorted face array pointer
    for (int i = 0; i < facenum; i++) {
        memcpy(fz++, s[i].p, sizeof(face));
    }
    
    delete[] s;
}

void flat::blend() {
    unsigned short *p = buffer;
    unsigned short *v = buffer2;
    _asm {
        mov     esi, p
        mov     edi, v
        mov     ecx, (X_RES*Y_RES) / 2
        
        _loop:
        mov     eax, [esi]      // 1
        mov     ebx, [edi]      // .
        
        and     eax, 0x7BDE7BDE // 2
        and     ebx, 0x7BDE7BDE // .
        
        shr     eax, 1          // 3
        add     esi, 4          // .
        
        shr     ebx, 1          // 4
        add     edi, 4          // .
        
        add     eax, ebx        // 5
        dec     ecx             // .
        
        mov     [edi - 4], eax  // 6
        jnz     _loop           // .
    }
}

void flat::dirblur(int disp) {
    unsigned short *p   = &buffer2[0];
    unsigned short *v   = &buffer2[2*disp];
    unsigned int   len  = (X_RES*Y_RES) - 2*disp,
                   _edx = (-disp) & ~1;
    _asm {
        mov     esi, p
        mov     ecx, len
        mov     edi, v
        mov     edx, _edx
        push    ebp
        
        _loop:
        mov     ax, [esi]
        add     esi, 2
        
        mov     bx, [edi]
        and     eax, 0x0000781E
        
        mov     bp, [edi + edx]
        and     ebx, 0x0000781E
        
        shr     eax, 1
        and     ebp, 0x000003E0
        
        shr     ebx, 1
        add     edi, 2
        
        add     eax, ebx
        
        or      eax, ebp
        dec     ecx
        
        mov     [edi + edx - 2], ax
        jnz     _loop

        pop     ebp
    }
}

void flat::drawbuf(int i, int amp) {
    // fill random buffer lines
    for (int k = 0; k < 40; k++) {
        int c = (rand() % amp) * 0x04210421;
        int l = (rand() % Y_RES) * X_RES;
        unsigned short *buf = &buffer[l];
        _asm {
            cld
            mov    eax, c
            mov    ecx, X_RES / 2
            mov    edi, buf
            
            __loop:
            add    [edi], eax
            add    edi, 4
            dec    ecx
            jnz    __loop            
        }
    }
}

void flat::adjustbuf(int i) {
    float *f = pz;

    for (int y = 0; y < flaresnum; y++) {
        for (int x = 0; x < flaresnum; x++) {
            
            *f = 32*(sintabf[((i << 9) + (x << 12) + (x << 11)) & 0xFFFF] + 
                     costabf[((i << 7) + (y << 13)) & 0xFFFF] + 
                     costabf[((i << 9) + (y << 14)) & 0xFFFF] +
                     sintabf[((i << 8) + (x << 14)) & 0xFFFF]);
            /*
            *f = 16*(sintabf[((i << 7) + (x << 11) + (x << 10)) & 0xFFFF] + 
                     costabf[((i << 7) + (y << 12)) & 0xFFFF] + 
                     costabf[((i << 8) + (y << 13) + (y << 11)) & 0xFFFF] +
                     sintabf[((i << 8) + (x << 13)) & 0xFFFF]);*/
            f++;
        }
    }
}


int flat::main(int timedemo) {
    int i, j;
    
    tmap_init(buffer, X_RES, Y_RES, (X_RES*2));
    
    i = 0;
    while (timedemo ? (i < 1024) : !kbhit()) {
        if (timedemo) i++; else {i = *tick; ptc_wait();}
        
        ptc_update(buffer2);
        
        memset(buffer,  0, sizeof(unsigned short)*X_RES*Y_RES);
        drawbuf(i, 4);
        
        float scale = 0.5 * costabf[(i << 6) & 0xFFFF] + 0.8;
        int   rot1  = (int)(0x2000 * sintabf[((i << 6) + (i << 7)) & 0xFFFF]),
              rot2  = (int)(0x2000 * sintabf[((i << 7)) & 0xFFFF]),
              rot3  = (i << 7) + (int)(0x2000 * sintabf[((i << 7)) & 0xFFFF]),
              movedist = -50 * costabf[(i << 7) & 0xFFFF] - 150;
        
        adjustbuf(i);
        
        // draw flares
        for (j = 0; j < flarescount; j++) {
            pt[j] = p[j];
            pt[j].z = pz[j];
            //float mul = 1 - (0.8f *(((1-cos((atan2(pt[j].x,pt[j].y) * 8) + ((float)i * 0.05f)))+
            //                   (1-cos((atan2(pt[j].x,pt[j].z) * 8) + ((float)i * 0.05f)))+
            //                   (1-cos((atan2(pt[j].y,pt[j].z) * 8) + ((float)i * 0.05f)))) / 16));
            
            //vecmul(mul, mul, mul, &pt[j]);
            //pt[j] = pm[j];
            rotate3d(rot1 & 0xFFFF, rot2 & 0xFFFF, rot1 & 0xFFFF, &pt[j]);
            vecmove(0, 0, movedist, &pt[j]);
            project3d(&p2d[j], &pt[j]);
        }
        for (j = 0; j < flaresnum; j++) pn[j].x = pn[j].y = pn[j].z = 0;
        for (j = 0; j < facenum; j++) {
            face cf;      
            
            vec4f  cfa, cfb, cfc; // some vertices
            vec4f  rfa, rfb;      // vecsub result
            vec4f  fn;            // face normal
            
            cf = ff[j];
            
            // calculate normal
            cfa = pt[cf.a]; cfb = pt[cf.b]; cfc = pt[cf.c];
            
            rfa = vecsub(&cfb, &cfa); rfb = vecsub(&cfc, &cfa);
            fn = veccross(&rfa, &rfb);
            vecnormalize(&fn);
            
            ff[j].n = fn;
            
            pn[cf.a].x = fn.x; pn[cf.a].y = fn.y; pn[cf.a].z = fn.z;
            pn[cf.b].x = fn.x; pn[cf.b].y = fn.y; pn[cf.b].z = fn.z;
            pn[cf.c].x = fn.x; pn[cf.c].y = fn.y; pn[cf.c].z = fn.z;
        }
        for (j = 0; j < flaresnum; j++) vecnormalize(&pn[j]);
        
        
        // draw faaaaces
        //memcpy(fz, ff, sizeof(face)*facenum);
        facesort(ff, fz);
        unsigned short *t = (unsigned short*)(((unsigned int)texture) >> 1);
        for (j = 0; j < facenum; j++) {
            face cf;      
            
            float  fcol;          // face color
            vec4f  lv;            // light vertex
            vec4f  cfa, cfb, cfc; // some vertices
            vec4f  rfa, rfb;      // vecsub result
            vec4f  fn;            // face normal
            
            cf = fz[j];
            
            // calculate face color
            lv.x = lv.y = 0; lv.z = 1;
            fcol = vecdot(&cf.n, &lv);// if (fcol > 0) continue;
            
            int col = (int)(-fcol * 128) + 128;
            cf.col = (((col * 64) >> 11) << 0) | (((col * 128) >> 11) << 5) | (((col * 192) >> 11) << 10);
            
            // draw face (erm...wait)
            vecf f_pipe[3];
            
            f_pipe[0].x = p2d[cf.a].x;  f_pipe[0].y = p2d[cf.a].y; f_pipe[0].u = (1-pn[cf.a].x)*128; f_pipe[0].v = (1-pn[cf.a].y)*128;
            f_pipe[1].x = p2d[cf.b].x;  f_pipe[1].y = p2d[cf.b].y; f_pipe[1].u = (1-pn[cf.b].x)*128; f_pipe[1].v = (1-pn[cf.b].y)*128;
            f_pipe[2].x = p2d[cf.c].x;  f_pipe[2].y = p2d[cf.c].y; f_pipe[2].u = (1-pn[cf.c].x)*128; f_pipe[2].v = (1-pn[cf.c].y)*128;
            f_pipe[0].c = f_pipe[1].c = f_pipe[2].c = cf.col;
            
            facedraw_flat(&f_pipe[0]);
            //facedraw_tex(&f_pipe[0], t);
        }
        
        // draw lines
        /*
        for (j = 0; j < linesnum; j++) {
            vec4f fa, fb, fd;
            vec2f ca, cb, cd;
            line cf;
            
            cf = f[j];
            fa = pt[cf.a]; fb = pt[cf.b];
            
            if ((fb.z >= 0) && (fa.z >= 0)) continue;
            if (fb.z > fa.z) {fd = fa; fa = fb; fb = fd;}
            
            if (fa.z >= 0){
                vec4f pa = fa;
                
                pa.y = fa.y + (fa.y + fb.y) * (-fb.z+dist) / (fa.z - fb.z);
                pa.x = fa.x + (fa.x + fb.x) * (-fb.z+dist) / (fa.z - fb.z);
                pa.z = -dist;
                
                fa = pa;
            }
            
            project3d(&ca, &fa);
            project3d(&cb, &fb);
            
            if (ca.y > cb.y) {cd = cb; cb = ca; ca = cd;} 
            
            if (!lineclip(&ca, &cb)) {linedraw(&ca, &cb, &cf); }
        }
        */
        blend();
        //blur(X_RES/2, Y_RES/2);
        //dirblur((int)(4 * costabf[(i << 11) & 0xFFFF] + 4));
    }
    if (!timedemo) getch();
    
    if (timedemo) printf("timed %d timerticks in %d frames - %f fps", *tick, i, (double)(60 * i) / *tick);
    
    return 0;
}
