#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <math.h>
#include <i86.h>

#include "..\..\flexptc\flexptc.h"
#include "..\..\ims06\xmplay.h"
#include "..\..\ims06\mcp.h"

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

// class definition
#include "flat.h"

flat_0::flat_0() {
    bgpic       = new unsigned short[X_RES*Y_RES];
    greets      = new unsigned short[16*1280];
    buffer      = new unsigned short[X_RES*Y_RES];
    buffer2     = new unsigned short[X_RES*Y_RES];
    d           = new dlist[Y_RES];
    pz          = new float[vertexcount];
    p           = new vec4f[vertexcount];
    pt          = new vec4f[vertexcount];
    pn          = new vec4f[vertexcount];
    p2d         = new vec2f[vertexcount];
    puv         = new uv   [vertexcount];
    f           = new line [linesnum];
    ff          = new face [facenum];
    fz          = new face [facenum];
    
    FILE *f = fopen("gfx\\flat_bg.tga", "rb");
    if (f) {
        fseek(f, 18, SEEK_SET);
        fread(bgpic, sizeof(unsigned short), X_RES*Y_RES, f);
        fclose(f);
    }
    
    f = fopen("gfx\\greets.tga", "rb");
    if (f) {
        fseek(f, 18, SEEK_SET);
        fread(greets, sizeof(unsigned short), 16*1280, f);
        fclose(f);
    }
    
    // call init stuff
    initvertex(mk15(2,2,2));
    
    memset(pz,      0, sizeof(float)*vertexcount);
    memset(buffer,  0, sizeof(unsigned short)*X_RES*Y_RES);
    memset(buffer2, 0, sizeof(unsigned short)*X_RES*Y_RES);
}

flat_0::~flat_0() {
    delete[] buffer;
    delete[] greets;
    delete[] buffer2;
    delete[] bgpic;
    delete[] p;
    delete[] d;
    delete[] pt;
    delete[] pn;
    delete[] p2d;
    delete[] puv;
    delete[] f;
    delete[] pz;
    delete[] ff;
    delete[] fz;
}

void flat_0::initvertex(int color) {
    uv    *tuv = puv;
    vec4f *t   = p;
    line  *v   = f;
    face  *z   = ff;
    
    // init vertices
    int max = vertexnum;
    float scale = 0.5 * ((float)(max + 2) / (float)max);
    float rat = (65536 / max);
    for (int teta = 0; teta < max; teta++) {
        for (int phi = 0; phi < max; phi++) {
            t->x = vertexdist * sintabf[(int)(phi * rat*scale)] * costabf[(int)(teta * rat)];
            t->y = vertexdist * sintabf[(int)(phi * rat*scale)] * sintabf[(int)(teta * rat)];
            t->z = vertexdist * costabf[(int)(phi * rat*scale)];
            t->w = 1;
            t++;
        }
    }
    memset(v, 0, sizeof(line)*linesnum);
    // init lines
    for (int teta = 0; teta < (max-1); teta++) {
        for (int phi = 0; phi < max; phi++) {
            v->a   = (phi*max + (teta % max));
            v->b   = (phi*max + ((teta + 1) % max));
            v->col = color;
            v++;
        }
    }
    
    for (int teta = 0; teta < max; teta++) {
        for (int phi = 0; phi < max; phi++) {
            v->a   = ((phi % max)*max + teta);
            v->b   = (((phi + 1) % max)*max + teta);
            v->col = color;
            v++;
        }
    }
    
    
    memset(z, 0, sizeof(face)*facenum);
    // init faces
    for (int teta = 0; teta < max-1; teta++) {
        for (int phi = 0; phi < max; phi++) {
            z->a   = (((phi  ) % max)*max + (teta % max));
            z->b   = (((phi+1) % max)*max + ((teta + 1) % max));
            z->c   = (((phi  ) % max)*max + ((teta + 1) % max));
            z++;
        }
        //lptr++;
    }
    
    for (int teta = 0; teta < max-1; teta++) {
        for (int phi = 0; phi < max; phi++) {
            z->a   = ((phi % max)*max + teta);
            z->b   = (((phi + 1) % max)*max + teta);
            z->c   = (((phi + 1) % max)*max + ((teta + 1) % max));
            z++;
        }
    }
    
}

void flat_0::linedraw(vec2f *a, vec2f *b, line *f) {

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
 
int flat_0::lineclip(vec2f *a, vec2f *b) {
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

void flat_0::addval(unsigned short *dst, unsigned short col) {
    unsigned short *p = dst;
    unsigned int val = col * 0x00010001;
    // saturated rgb555 addition from hugi #18 by BRAD
    // a bit modified by wbcbz7 :)
    _asm {
        mov     esi, val
        mov     edi, p
        mov     ecx, (X_RES*Y_RES) / 2
        push    ebp
        
        _inner:
        mov     eax, [edi]          // 1
        mov     ebx, esi            // .
        mov     ebp, eax            // 2
        mov     edx, ebx            // .
        and     eax, 0x3DEF3DEF     // 3 <- msb inverted mask
        and     ebx, 0x3DEF3DEF     // .
        and     edx, 0x42104210     // 4 <- msb mask
        add     eax, ebx            // .
        and     ebp, 0x42104210     // 5
        mov     ebx, edx            // .
        or      edx, ebp            // 6 
        and     ebx, ebp            // .
        mov     ebp, edx            // 7
        and     edx, eax            // .
        or      eax, ebp            // 8
        or      edx, ebx            // .
        shr     edx, 4              // 9
        add     edi, 4              // .
        add     edx, 0x3DEF3DEF     // 10 <- add fixup
        xor     edx, 0x3DEF3DEF     // 11 <- xor fixup
        or      eax, edx            // 12 (stall)
        dec     ecx                 // .
        mov     [edi - 4], eax      // 13
        jnz     _inner              // .
        
        pop     ebp                 // 1
    }
}

void flat_0::project3d(vec2f *f, vec4f *v) {
    float t;
    
    if (v->z < 0) {
        t = dist / (v->z + ee);
        f->x = (v->x * t);
        f->y = (v->y * t);
    } else f->x = -1;
}

typedef struct {
    void *p;
    float avg_z;
} sindex;

static int sortcmp(const void *f, const void *v) {
    return (((sindex*)f)->avg_z - ((sindex*)v)->avg_z);
}

void flat_0::facesort(face *f, face *fz) {
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

void flat_0::filldlist(dlist *d) {
    dlist *p = d;
    
    for (int i = 0; i < Y_RES; i++) {
        p->active = rand() & 1;
        p->disp   = (rand() & 6); // two pixels boundary
        p->mask1  = 0x7FFF7FFF; p->mask2  = 0;
        
        int flags = rand() % 7 + 1;
        if (flags & 1) {p->mask2 ^= 0x78007800; p->mask1 ^= 0x7C007C00;} flags >>= 1;
        if (flags & 1) {p->mask2 ^= 0x03C003C0; p->mask1 ^= 0x03E003E0;} flags >>= 1;
        if (flags & 1) {p->mask2 ^= 0x001E001E; p->mask1 ^= 0x001F001F;} flags >>= 1;
        
        p++;
    }
}

void flat_0::dirblur(unsigned short *dst, unsigned int len, int disp, unsigned int mask1, unsigned int mask2) {
    unsigned short *p = dst;
    unsigned short *v = &dst[2*disp];
    unsigned int   l  = (len - 2*disp),
                   _edx = -4*disp;               
    static unsigned int m1, m2;
    
    m1 = mask1; m2 = mask2;
    
    _asm {
        mov     esi, p
        mov     ecx, l
        mov     edi, v
        mov     edx, _edx
        push    ebp
        
        _loop:
        mov     eax, [esi]
        add     esi, 4
        
        mov     ebx, [edi]
        and     eax, [m2]
        
        mov     ebp, [edi + edx]
        and     ebx, [m2]
        
        shr     eax, 1
        and     ebp, [m1]
        
        shr     ebx, 1
        add     edi, 4
        
        add     eax, ebx
        
        or      eax, ebp
        sub     ecx, 2
        
        mov     [edi + edx - 4], eax
        jnz     _loop

        pop     ebp
    }
}

void flat_0::drawbuf(int i, int amp) {
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
            sub    [edi], eax
            add    edi, 4
            dec    ecx
            jnz    __loop            
        }
    }
}

void flat_0::transblit(unsigned short *dst, unsigned short *src, unsigned int xres, unsigned int yres) {
    // blit with transparency
    unsigned short *p = src;
    unsigned short *v = dst;
    
    static int pitch_add, xr, yr;
    xr = xres/2; yr = yres; pitch_add = (X_RES - xres)*2;
    if ((!xr) || (!yr)) return;
    
    _asm {
        mov     esi, p
        mov     edi, v
        mov     ecx, [yr]
        push    ebp
        
        _outer:
        push    ecx
        mov     ecx, [xr]
        
        _inner:
        mov     eax, [esi]
        mov     ebx, [edi]
        
        mov     edx, eax
        add     esi, 4
        
        and     edx, 0x80008000
        and     eax, 0x7FFF7FFF
        
        shr     edx, 15
        add     edi, 4
        
        add     edx, 0x7FFF7FFF
        mov     ebp, edx
        
        xor     edx, 0x7FFF7FFF 
        xor     ebp, 0x80008000 
        
        and     ebx, edx
        and     eax, ebp
        
        xor     eax, ebx
        dec     ecx
        
        mov     [edi - 4], eax
        jnz     _inner
        
        pop     ecx
        add     edi, [pitch_add]
        dec     ecx
        jnz     _outer
        
        pop     ebp
    }
}

int flat_0::main(int noretrace) {
    int i = 0, di = 0, j;
    
    tmap_init(buffer, X_RES, Y_RES, (X_RES*2));
    
    int rawpos, row, order;
    int glvel = 0, alphaval = 31, alev = 0, mulval = 64;
    
    float scale = 0;
    
    rawpos = (xmpGetRealPos() & 0x00FFFFFF); row = (rawpos >> 8) & 0xFF; order = (rawpos >> 16) & 0xFF;
    
    while ((order < 0x10) && (!kbhit())) {
        di = i; i = *tick; di = i - di; if (!noretrace) ptc_wait();
        
        ptc_update(buffer);
        if (mcpIdle) {mcpIdle();}
                
        rawpos = (xmpGetRealPos() & 0x00FFFFFF); row = (rawpos >> 8) & 0xFF; order = (rawpos >> 16) & 0xFF;
        
        memcpy(buffer, bgpic, sizeof(unsigned short)*X_RES*Y_RES);
        transblit(&buffer[X_RES-16], &greets[16*(i >> 0)], 16, Y_RES);
        
        //memset(buffer,  0, sizeof(unsigned short)*X_RES*Y_RES);
        drawbuf(i, 1);
        
        if (((row & 31) == 0x0) || ((row & 31)== 0xC) || ((row & 31) == 0x18)) scale = 0.3;
        
        int   rot1  = (i << 7) + (int)(0x1C00 * sintabf[((i << 6) + (i << 5)) & 0xFFFF]),
              rot2  = (i << 6) + (i << 4) + (int)(0x1800 * sintabf[((i << 4)) & 0xFFFF]),
              rot3  = (i << 7) + (int)(0x2000 * sintabf[((i << 7)) & 0xFFFF]),
              movedist = -250;
        
        // draw vertex
        for (j = 0; j < vertexcount; j++) {
            pt[j] = p[j];
            float mul = 1 - (0.8f *(((1-cos((atan2(pt[j].x,pt[j].y) * 4) + ((float)i * 0.04f)))+
                               (1-cos((atan2(pt[j].x,pt[j].z) * 2) + ((float)i * 0.05f)))+
                               (1-cos((atan2(pt[j].y,pt[j].z) * 2) + ((float)i * 0.02f)))) / 8));
            
            vecmul(mul, mul, mul, &pt[j]);
            vecmul(scale + 1, scale + 1, scale + 1, &pt[j]);
            //pt[j] = pm[j];
            rotate3d(rot1 & 0xFFFF, rot2 & 0xFFFF, rot1 & 0xFFFF, &pt[j]);
            vecmove(0, 0, movedist, &pt[j]);
            project3d(&p2d[j], &pt[j]);
        }
        
        for (j = 0; j < vertexcount; j++) pn[j].x = pn[j].y = pn[j].z = 0;
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
        for (j = 0; j < vertexcount; j++) vecnormalize(&pn[j]);
        
        int x_shift = -80;
        // draw faaaaces
        facesort(ff, fz);
        for (int k = 0; k < 3; k++) {
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
            fcol = vecdot(&cf.n, &lv); if (fcol > 0) continue;
            
            int col = (int)(-fcol * 256);// + 16;
            cf.col = (((col * 240) >> 11) << 0) | (((col * 192) >> 11) << 5) | (((col * 128) >> 11) << 10);
            
            // draw face (erm...wait)
            vecf f_pipe[3];
            
            f_pipe[0].x = p2d[cf.a].x*(1 + 0.3f*k) + x_shift + X_RES/2;  f_pipe[0].y = p2d[cf.a].y*(1 + 0.3f*k) + Y_RES/2;
            f_pipe[1].x = p2d[cf.b].x*(1 + 0.3f*k) + x_shift + X_RES/2;  f_pipe[1].y = p2d[cf.b].y*(1 + 0.3f*k) + Y_RES/2;
            f_pipe[2].x = p2d[cf.c].x*(1 + 0.3f*k) + x_shift + X_RES/2;  f_pipe[2].y = p2d[cf.c].y*(1 + 0.3f*k) + Y_RES/2;
            f_pipe[0].c = f_pipe[1].c = f_pipe[2].c = cf.col;
            
            facedraw_flat(&f_pipe[0], FLAT_HALFTRANS);
        }
        }
        
        // draw lines
        
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
            
            ca.x += X_RES/2 + x_shift; cb.x += X_RES/2 + x_shift;
            ca.y += Y_RES/2; cb.y += Y_RES/2;
            
            if (!lineclip(&ca, &cb)) {linedraw(&ca, &cb, &cf); }
        }
        
        if (((row & 31) == 0x8) || ((row & 31) == 0x14) || ((row & 31) == 0x1C)) glvel = 8;
        
        // fill and draw dlist
        if (glvel) {
            unsigned short *pd = buffer;
            dlist *dd = d;
            filldlist(d);
            dlist de;
            memcpy(&de, dd, sizeof(dlist));
            for (int k = 0; k < Y_RES; k++) {
                if (dd->active) {memcpy(&de, dd, sizeof(dlist));}
                dirblur(pd, X_RES, de.disp, de.mask1, de.mask2);
                dd++; pd += X_RES;
            }
        }
        
        glvel -= di; if (glvel <= 0) glvel = 0;
        scale -= 0.03f*di; if (scale <= 0) scale = 0;
        
        if (alphaval >= 0) {alev = (int)alphaval; addval(buffer, mk15(alev, alev, alev)); alphaval -= di;}
        if (alphaval <= 0) alphaval = 0;
        
    }
    if (kbhit()) {getch(); return 1;}
    
    return 0;
}
