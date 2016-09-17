#ifndef TMAP_H
#define TMAP_H

typedef struct {
    float x, y, u, v, c;
    long visible;
} vecf;

typedef struct {
    long x, y, u, v, c, visible;
} veci;

inline long ceil(long x){
    x +=  0xffff;
    return (x >> 16);
}

long imul16(long x, long y);        // (x * y) >> 16
#pragma aux imul16 = \
    " imul  edx        "\
    " shrd  eax,edx,16 "\
    parm [eax] [edx] value [eax]


long imul14(long x, long y);        // (x * y) >> 14
#pragma aux imul14 = \
    " imul  edx        "\
    " shrd  eax,edx,14 "\
    parm [eax] [edx] value [eax]


long idiv16(long x, long y);        // (x << 16) / y
#pragma aux idiv16 = \
    " mov   edx,eax    "\
    " sar   edx,16     "\
    " shl   eax,16     "\
    " idiv  ebx        "\
    parm [eax] [ebx] modify exact [eax edx] value [eax]

#define VISIBLE     0
#define TOP         1
#define BOT         2
#define LEFT        4
#define RIGHT       8

enum {
    FLAT_OPAQUE, FLAT_HALFTRANS
};

void tmap_init(void *buf, int xres, int yres, int pitch);

void facedraw_flat(vecf *v, int style);
void facedraw_tex (vecf *v, void *t);

#endif