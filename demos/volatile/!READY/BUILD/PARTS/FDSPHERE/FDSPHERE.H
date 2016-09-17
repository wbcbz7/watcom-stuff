#ifndef FDSPHERE_H
#define FDSPHERE_H

// когда что-то тут поменял - ПЕРЕСОБЕРИ MAIN.CPP!!1
#include "..\..\common\matrix.h"
#include "..\..\common\dlist.h"
#include "..\..\common\alpha.h"
class fdsphere {
    private:
        struct fdgrid {
            unsigned int u, v, l, i;
        };
        typedef struct {
            int a, b, col;
        } line;
        
        enum {
            SphereSize = 128,
            dist       = 90,
            
            flaresnum = 256,      // total num of flares
            flarescount = flaresnum,
            flaretypes = 2,     // num of flare types
            flarescale = 0x300, // flare scale level
            flaresize = 16,     // size of flare sprite, e.g flaresize=8 -> 8x8
            flaresdist = (int)(SphereSize*0.2),
            
            vertexnum = 12,
            vertexcount = vertexnum*vertexnum,
            vertexdist = SphereSize,
            linesnum = (vertexcount * 2) - vertexnum
        };
        
        fdgrid         *fdtab;               // grid node u\v\l values
        unsigned short *buffer, *buffer2;    // screen buffer
        unsigned short *alignbuf;            // alignment buffer
        unsigned short *texture;             // texture
        unsigned short *flare;               // flare sprite
        vec4f *p, *pt;                       // vertex coords
        vec2f *p2d;                          // projected vertex coords 
        vec4f *n, *nt;                       // vertex coords
        vec2f *n2d;                          // projected vertex coords 
        line  *f;                            // line array
        dlist *d;
        
        // additional procedures
        void initflares();
        void initvertex(int color);
        
        // main procedures
        void drawgrid(vec4f *o, vec4f *ot, int rx, int ry, int rz);
        void interpolate();
        void blend();
        void project3d(vec2f *v, vec4f *f);
        void drawflare(vec2f *f, int type);
        void dirblur(unsigned short *dst, unsigned int len, int disp, unsigned int mask1, unsigned int mask2);
        void filldlist(dlist *d, int vel);
        void invert(unsigned short *dst);

        void linedraw(vec2f *a, vec2f *b, line *f);
        int  lineclip(vec2f *a, vec2f *b);
        
    public:
        // constructor\destructor
        fdsphere();
        ~fdsphere();
        
        // timer var pointer
        volatile int *tick;
        
        // main entrypoint =)
        int main(int noretrace);
};

#endif