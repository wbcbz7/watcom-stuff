#include <stdlib.h>
#include <math.h>
#include "tmap.h"

static void *dst;
static float TopClip, BotClip, LeftClip, RightClip;
static int dst_pitch;

void tmap_init(void *buf, int xres, int yres, int pitch) {
    dst = buf;
    TopClip = 0; BotClip = yres; LeftClip = 0; RightClip = xres;
    dst_pitch = pitch;
}

// zpizzheno from fatmap2.zip by MRI\Doomsday just because
// I didn't have ANY time to write my own tmapper :)

inline int cliptestx(vecf * v)
{
    int bits = 0;
    if(v->y < LeftClip)     bits |= LEFT;
    if(v->y > RightClip)    bits |= RIGHT;
    return bits;
}

inline int cliptesty(vecf * v)
{
    int bits = 0;
    if(v->y < TopClip)      bits |= TOP;
    if(v->y > BotClip)      bits |= BOT;
    return bits;
}

/*
 *      vbp is a pointer to a vertex array. The first 3 vertices in that
 *      array is our source vertices. The rest of the array will be used
 *      to hold new vertices created during clipping.
 *
 *      you can then access the new vertices using the *final variable
 *
 *      function returns the number of vertices in the resulting polygon
 */
 
int ClipUV(vecf *** final, vecf * vbp)
{
    int max, n, dsti;
    static vecf * vp1[12], * vp2[12];     // vertex ptr buffers
    vecf ** src = vp1;
    vecf ** dst = vp2;
    vecf ** tmp;

    vp1[0] = vbp + 0;
    vp1[1] = vbp + 1;
    vp1[2] = vbp + 2;
    vp1[3] = vbp + 0;

    vbp += 3;       // Next free vertex

    dsti = 0;
    max = 3;

    // left clip

    for(n=0; n<max; n++) {
        vecf * src1 = src[n];             // current vertex
        vecf * src2 = src[n+1];           // next vertex      
        if((src1->visible & LEFT) == VISIBLE) {
            dst[dsti++] = src1;                 // add visible vertex to list
            if((src2->visible & LEFT) == VISIBLE)
                continue;
        } else
            if((src2->visible & LEFT) != VISIBLE)
                continue;
        float a = (LeftClip - src1->x) / (src2->x - src1->x);
        float ima = 1.0-a;
        dst[dsti] = vbp++;                      // create new vertex
        dst[dsti]->y = src1->y*ima + src2->y*a;
        dst[dsti]->x = LeftClip;
        dst[dsti]->u = src1->u*ima + src2->u*a;
        dst[dsti]->v = src1->v*ima + src2->v*a;
        dst[dsti]->visible = cliptesty(dst[dsti]);
        dsti++;
    }
    dst[dsti] = dst[0];
    tmp = src; src = dst; dst = tmp;            // Swap src - dst buffers
    max = dsti;
    dsti = 0;

    // right clip

    for(n=0; n<max; n++) {
        vecf * src1 = src[n];             // current vertex
        vecf * src2 = src[n+1];           // next vertex      
        if((src1->visible & RIGHT) == VISIBLE) {
            dst[dsti++] = src1;                 // add visible vertex to list
            if((src2->visible & RIGHT) == VISIBLE)
                continue;
        } else
            if((src2->visible & RIGHT) != VISIBLE)
                continue;
        float a = (RightClip - src1->x) / (src2->x - src1->x);
        float ima = 1.0-a;
        dst[dsti] = vbp++;                      // create new vertex
        dst[dsti]->y = src1->y*ima + src2->y*a;
        dst[dsti]->x = RightClip;
        dst[dsti]->u = src1->u*ima + src2->u*a;
        dst[dsti]->v = src1->v*ima + src2->v*a;
        dst[dsti]->visible = cliptesty(dst[dsti]);
        dsti++;
    }
    dst[dsti] = dst[0];
    tmp = src; src = dst; dst = tmp;            // swap src - dst buffers
    max = dsti;
    dsti = 0;

    // top clip

    for(n=0; n<max; n++) {
        vecf * src1 = src[n];             // current vertex
        vecf * src2 = src[n+1];           // next vertex      
        if((src1->visible & TOP) == VISIBLE) {
            dst[dsti++] = src1;                 // add visible vertex to list
            if((src2->visible & TOP) == VISIBLE)
                continue;
        } else
            if((src2->visible & TOP) != VISIBLE)
                continue;
        float a = (TopClip - src1->y) / (src2->y - src1->y);
        float ima = 1.0-a;
        dst[dsti] = vbp++;                      // create new vertex
        dst[dsti]->x = src1->x*ima + src2->x*a;
        dst[dsti]->y = TopClip;
        dst[dsti]->u = src1->u*ima + src2->u*a;
        dst[dsti]->v = src1->v*ima + src2->v*a;
        dst[dsti]->visible = cliptestx(dst[dsti]);
        dsti++;
    }
    dst[dsti] = dst[0];
    tmp = src; src = dst; dst = tmp;            // swap src - dst buffers
    max = dsti;
    dsti = 0;

    // bot clip

    for(n=0; n<max; n++) {
        vecf * src1 = src[n];             // current vertex
        vecf * src2 = src[n+1];           // next vertex      
        if((src1->visible & BOT) == VISIBLE) {
            dst[dsti++] = src1;                 // add visible vertex to list
            if((src2->visible & BOT) == VISIBLE)
                continue;
        } else
            if((src2->visible & BOT) != VISIBLE)
                continue;
        float a = (BotClip - src1->y) / (src2->y - src1->y);
        float ima = 1.0-a;
        dst[dsti] = vbp++;                      // create new vertex
        dst[dsti]->x = src1->x*ima + src2->x*a;
        dst[dsti]->y = BotClip;
        dst[dsti]->u = src1->u*ima + src2->u*a;
        dst[dsti]->v = src1->v*ima + src2->v*a;
        dsti++;
    }

    *final = dst;
    
    return dsti;
}

/*
 *  Watcom float -> long conversion is very slow.     
 *  Therefore we do this with inline FPU assembler.
 *  No fadd trick here though, just simple fistp.
 */

static float Scale16 = 65536.0;

// dst[0] = (long) (src[0] * 65536.0);
// dst[1] = (long) (src[1] * 65536.0);

void fist16two(long * dst, float * src);
#pragma aux fist16two = \
    "   fld    dword ptr [ebx]          "\
    "   fmul   dword ptr [Scale16]      "\
    "   fld    dword ptr [ebx+4]        "\
    "   fmul   dword ptr [Scale16]      "\
    "   fxch   st(1)                "\
    "   fistp  dword ptr [eax]          "\
    "   fistp  dword ptr [eax+4]        "\
    parm [eax] [ebx] modify [8087]


// *dst = (long) src;

void fist(long * dst, double src);
#pragma aux fist = \
    "   fistp  dword ptr [eax]  "\
    parm [eax] [8087] modify [8087]


inline int cliptest(vecf * v)
{
    int bits = 0;
    if(v->y < TopClip)   bits |= TOP;
    if(v->y > BotClip)   bits |= BOT;
    if(v->x < LeftClip)  bits |= LEFT;
    if(v->x > RightClip) bits |= RIGHT;
    return bits;
}

static veci * max_vtx;                  // Max y veci (ending veci)
static veci * start_vtx, * end_vtx;     // First and last veci in array
static veci * right_vtx, * left_vtx;    // Current right and left veci

static long right_height, left_height;
static long right_x, right_dxdy, left_x, left_dxdy;
static long left_u, left_dudy, left_v, left_dvdy;
static long _dudx, _dvdx;

static void RightSection(void)
{
    // Walk backwards trough the veci array

    veci * v2, * v1 = right_vtx;
    if(right_vtx > start_vtx) v2 = right_vtx-1;     
    else                      v2 = end_vtx;         // Wrap to end of array
    right_vtx = v2;

    // v1 = top veci
    // v2 = bottom veci 

    // Calculate number of scanlines in this section

    right_height = ceil(v2->y) - ceil(v1->y);
    if(right_height <= 0) return;

    // Guard against possible div overflows

    if(right_height > 1) {
        // OK, no worries, we have a section that is at least
        // one pixel high. Calculate slope as usual.

        long height = v2->y - v1->y;
        right_dxdy  = idiv16(v2->x - v1->x, height);
    }
    else {
        // Height is less or equal to one pixel.
        // Calculate slope = width * 1/height
        // using 18:14 bit precision to avoid overflows.

        long inv_height = (0x10000 << 14) / (v2->y - v1->y);  
        right_dxdy = imul14(v2->x - v1->x, inv_height);
    }

    // Prestep initial values

    long prestep = (ceil(v1->y) << 16) - v1->y;
    right_x = v1->x + imul16(prestep, right_dxdy);
}

static void LeftSection(void)
{
    // Walk forward trough the veci array

    veci * v2, * v1 = left_vtx;
    if(left_vtx < end_vtx) v2 = left_vtx+1;
    else                   v2 = start_vtx;      // Wrap to start of array
    left_vtx = v2;

    // v1 = top veci
    // v2 = bottom veci 

    // Calculate number of scanlines in this section

    left_height = ceil(v2->y) - ceil(v1->y);
    if(left_height <= 0) return;

    // Guard against possible div overflows

    if(left_height > 1) {
        // OK, no worries, we have a section that is at least
        // one pixel high. Calculate slope as usual.

        long height = v2->y - v1->y;
        left_dxdy = idiv16(v2->x - v1->x, height);
        left_dudy = idiv16(v2->u - v1->u, height);
        left_dvdy = idiv16(v2->v - v1->v, height);
    }
    else {
        // Height is less or equal to one pixel.
        // Calculate slope = width * 1/height
        // using 18:14 bit precision to avoid overflows.

        long inv_height = (0x10000 << 14) / (v2->y - v1->y);
        left_dxdy = imul14(v2->x - v1->x, inv_height);
        left_dudy = imul14(v2->u - v1->u, inv_height);
        left_dvdy = imul14(v2->v - v1->v, inv_height);
    }

    // Prestep initial values

    long prestep = (ceil(v1->y) << 16) - v1->y;
    left_x = v1->x + imul16(prestep, left_dxdy);
    left_u = v1->u + imul16(prestep, left_dudy);
    left_v = v1->v + imul16(prestep, left_dvdy);
}

// ---------------------------------------------

void DrawFlatPoly(veci * vtx, int vertices, int color, int style)
{
    start_vtx = vtx;        // First vertex in array

    // Search trough the vtx array to find min y, max y
    // and the location of these structures.

    veci * min_vtx = vtx;
    max_vtx = vtx;

    long min_y = vtx->y;
    long max_y = vtx->y;

    vtx++;

    for(int n=1; n<vertices; n++) {
        if(vtx->y < min_y) {
            min_y = vtx->y;
            min_vtx = vtx;
        }
        else
        if(vtx->y > max_y) {
            max_y = vtx->y;
            max_vtx = vtx;
        }
        vtx++;
    }

    // OK, now we know where in the array we should start and
    // where to end while scanning the edges of the polygon

    left_vtx  = min_vtx;    // Left side starting vertex
    right_vtx = min_vtx;    // Right side starting vertex
    end_vtx   = vtx-1;      // Last vertex in array

    // Search for the first usable right section

    do {
        if(right_vtx == max_vtx) return;
        RightSection();
    } while(right_height <= 0);

    // Search for the first usable left section

    do {
        if(left_vtx == max_vtx) return;
        LeftSection();
    } while(left_height <= 0);

    unsigned char *destptr = (unsigned char*)dst + (ceil(min_y) * dst_pitch);

    for(;;)
    {
        long x1 = ceil(left_x);
        long width = ceil(right_x) - x1;
        
        unsigned char *p = destptr + (x1 << 1);
        
        if (width > 0)
            switch(style) {
                case FLAT_OPAQUE:
                    _asm {
                        mov     eax, color
                        mov     edi, p
                        mov     ecx, width
                        rep     stosw
                    }
                    break;
                case FLAT_HALFTRANS:
                    _asm {
                        mov     eax, color
                        mov     edi, p
                        and     eax, 0x7BDE
                        mov     ecx, width
                        shr     eax, 1
                        
                        _loop:
                        mov     bx, [edi]       // 1
                        add     edi, 2          // .
                            
                        and     ebx, 0x7BDE     // 2
                        shr     ebx, 1          // 3
                            
                        add     ebx, eax        // 4
                        dec     ecx             // .
                            
                        mov     [edi - 2], bx   // 5
                        jnz     _loop           // .
                    }
                    break;
                default: break;
            }

        destptr += dst_pitch;

        // Scan the right side

        if(--right_height <= 0) {               // End of this section?
            do {
                if(right_vtx == max_vtx) return;
                RightSection();
            } while(right_height <= 0);
        }
        else 
            right_x += right_dxdy;

        // Scan the left side

        if(--left_height <= 0) {                // End of this section?
            do {
                if(left_vtx == max_vtx) return;
                LeftSection();
            } while(left_height <= 0);
        }
        else 
            left_x += left_dxdy;
    }
}

void facedraw_flat(vecf *v, int style)
{
    vecf * v0 = v+0;
    vecf * v1 = v+1;
    vecf * v2 = v+2;

    v0->visible = cliptest(v0);
    v1->visible = cliptest(v1);
    v2->visible = cliptest(v2);

    int andbits = v0->visible & v1->visible & v2->visible;
    if(andbits != 0) return; 

    int orbits  = v0->visible | v1->visible | v2->visible;

    if((orbits & (TOP | BOT | LEFT | RIGHT)) == VISIBLE) {
        static veci v[3];
        fist16two(&v[0].x, &v0->x);
        fist16two(&v[1].x, &v1->x);
        fist16two(&v[2].x, &v2->x);
        DrawFlatPoly(v, 3, v0->c, style);
    }
    else {
        static vecf vc[12], ** dst;
        static veci v[12];
        vc[0].x = v0->x;
        vc[0].y = v0->y;
        vc[0].u = 0.0;                      // We don't need u,v
        vc[0].v = 0.0;
        vc[0].visible = v0->visible;
        vc[1].x = v1->x;
        vc[1].y = v1->y;
        vc[1].u = 0.0;
        vc[1].v = 0.0;
        vc[1].visible = v1->visible;
        vc[2].x = v2->x;
        vc[2].y = v2->y;
        vc[2].u = 0.0;
        vc[2].v = 0.0;
        vc[2].visible = v2->visible;
        int max = ClipUV(&dst, vc);
        if(max < 3) return;

        for(int n=0; n<max; n++) {
            fist16two(&v[n].x, &dst[n]->x);
        }
        DrawFlatPoly(v, max, v0->c, style);
    }
}

void inner(void * dst, int width, void * src, long u, long v);
#pragma aux inner = \
    "    push  ebp                                      "\
    "    rol   eax, 16                                  "\
    "    rol   edx, 16                                  "\
    "    mov   bl, al                                   "\
    "    mov   bh, dl                                   "\
    "    mov   esi, [_dudx]                             "\
    "    mov   ebp, [_dvdx]                             "\
    "    shl   esi, 16                                  "\
    "    mov   dl, byte ptr [_dudx+2]                   "\
    "    shl   ebp, 16                                  "\
    "    mov   dh, byte ptr [_dvdx+2]                   "\
    " next:                                             "\
    "    mov   ax, [2*ebx]                              "\
    "    add   edx, ebp                                 "\
    "    adc   bh, dh                                   "\
    "    add   eax, esi                                 "\
    "    mov   [edi], ax                          "\
    "    adc   bl, dl                                   "\
    "    add   edi, 2                                   "\
    "    dec   ecx                                   "\
    "    jnz   next                                     "\
    "    pop   ebp                                      "\
    parm [edi] [ecx] [ebx] [eax] [edx] modify [eax ebx ecx edx esi edi]

void DrawTexturePoly(veci * vtx, int vertices,
                     void * bitmap, long dudx, long dvdx)
{
    start_vtx = vtx;        // First vertex in array

    // Search trough the vtx array to find min y, max y
    // and the location of these structures.

    veci * min_vtx = vtx;
    max_vtx = vtx;

    long min_y = vtx->y;
    long max_y = vtx->y;

    vtx++;

    for(int n=1; n<vertices; n++) {
        if(vtx->y < min_y) {
            min_y = vtx->y;
            min_vtx = vtx;
        }
        else
        if(vtx->y > max_y) {
            max_y = vtx->y;
            max_vtx = vtx;
        }
        vtx++;
    }

    // OK, now we know where in the array we should start and
    // where to end while scanning the edges of the polygon

    left_vtx  = min_vtx;    // Left side starting vertex
    right_vtx = min_vtx;    // Right side starting vertex
    end_vtx   = vtx-1;      // Last vertex in array

    // Search for the first usable right section

    do {
        if(right_vtx == max_vtx) return;
        RightSection();
    } while(right_height <= 0);

    // Search for the first usable left section

    do {
        if(left_vtx == max_vtx) return;
        LeftSection();
    } while(left_height <= 0);

    unsigned char *destptr = (unsigned char*)dst + (ceil(min_y) * dst_pitch);

    _dudx = dudx;
    _dvdx = dvdx;

    for(;;)
    {
        long x1 = ceil(left_x);
        long width = ceil(right_x) - x1;
        
        unsigned char *p = destptr + (x1 << 1);
            
        if(width > 0) {

            // Prestep initial texture u,v
    
            long prestep = (x1 << 16) - left_x;
            long u = left_u + imul16(prestep, dudx);
            long v = left_v + imul16(prestep, dvdx);

            inner(p, width, bitmap, u, v);
        }

        destptr += dst_pitch;

        // Scan the right side

        if(--right_height <= 0) {               // End of this section?
            do {
                if(right_vtx == max_vtx) return;
                RightSection();
            } while(right_height <= 0);
        }
        else 
            right_x += right_dxdy;

        // Scan the left side

        if(--left_height <= 0) {                // End of this section?
            do {
                if(left_vtx == max_vtx) return;
                LeftSection();
            } while(left_height <= 0);
        }
        else {
            left_x += left_dxdy;
            left_u += left_dudy;
            left_v += left_dvdy;
        }
    }
}

void facedraw_tex(vecf *v, void *t)
{
    vecf * v0 = v+0;
    vecf * v1 = v+1;
    vecf * v2 = v+2;

    v0->visible = cliptest(v0);
    v1->visible = cliptest(v1);
    v2->visible = cliptest(v2);

    int andbits = v0->visible & v1->visible & v2->visible;
    if(andbits != 0) return; 

    double denom = (v0->x - v2->x) * (v1->y - v2->y) -
                   (v1->x - v2->x) * (v0->y - v2->y);
    if(denom == 0.0) return;
    double id = 1.0/denom * 65536.0;
    long dudx, dvdx;
    fist(&dudx, ((v0->u - v2->u) * (v1->y - v2->y) -
                 (v1->u - v2->u) * (v0->y - v2->y)) * id);
    fist(&dvdx, ((v0->v - v2->v) * (v1->y - v2->y) -
                 (v1->v - v2->v) * (v0->y - v2->y)) * id);

    int orbits  = v0->visible | v1->visible | v2->visible;

    if((orbits & (TOP | BOT | LEFT | RIGHT)) == VISIBLE) {
        static veci v[3];
        fist16two(&v[0].x, &v0->x);
        fist16two(&v[0].u, &v0->u);
        fist16two(&v[1].x, &v1->x);
        fist16two(&v[1].u, &v1->u);
        fist16two(&v[2].x, &v2->x);
        fist16two(&v[2].u, &v2->u);
        DrawTexturePoly(v, 3, t, dudx, dvdx);
    }
    else {
        static vecf vc[12], ** dst;
        static veci v[12];
        vc[0].x = v0->x;
        vc[0].y = v0->y;
        vc[0].u = v0->u;
        vc[0].v = v0->v;
        vc[0].visible = v0->visible;
        vc[1].x = v1->x;
        vc[1].y = v1->y;
        vc[1].u = v1->u;
        vc[1].v = v1->v;
        vc[1].visible = v1->visible;
        vc[2].x = v2->x;
        vc[2].y = v2->y;
        vc[2].u = v2->u;
        vc[2].v = v2->v;
        vc[2].visible = v2->visible;
        int max = ClipUV(&dst, vc);
        if(max < 3) return;

        for(int n=0; n<max; n++) {
            fist16two(&v[n].x, &dst[n]->x);
            fist16two(&v[n].u, &dst[n]->u);
        }
        DrawTexturePoly(v, max, t, dudx, dvdx);
    }
}
