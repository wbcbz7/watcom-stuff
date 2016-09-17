#ifndef ENVMAP_H
#define ENVMAP_H

// когда что-то тут поменял - ПЕРЕСОБЕРИ MAIN.CPP!!1
#include "matrix.h"
#include "vecdef.h"
#include "dlist.h"
class envmap {
    private:
        
        enum {
            dist = 150,         // perspective projection distance
            flaresnum = 12,      // total num of flares
            flarescount = flaresnum*flaresnum,
            flaredist = 144/flaresnum,     // flare dist
            
            linesnum = 2 * flaresnum * (flaresnum-1),
            facenum = 2 * (flaresnum-1) * (flaresnum-1)
        };
        unsigned short *blurtab;             // blur table
        unsigned short *buffer, *buffer2;    // screen buffer
        unsigned short *bgpic;               // background picture
        unsigned short *texbuf, *texture;    // face texture
        float *pz;                           // vertex z coords
        vec4f *p, *pt;                       // vertex coords
        vec4f *pn;                           // vertex normals
        vec2f *p2d;                          // projected vertex coords 
        uv    *puv;                          // vertex u\v texture coords
        line  *f;                            // line array
        face  *ff, *fz;                      // face array
        dlist *d;                            // display list
        
        // additional procedures
        void initblur();
        void inittexture();
        void initvertex(int color);
        
        // main procedures
        void project3d(vec2f *v, vec4f *f);
        void blur(int sx, int sy);
        void blend();
        void drawbuf(int i, int amp);
        
        void dirblur(unsigned short *dst, unsigned int len, int disp, unsigned int mask1, unsigned int mask2);
        void filldlist(dlist *d);
        
        void adjustbuf(int i);
        
        void facesort(face *f, face *fz);
        
        void linedraw(vec2f *a, vec2f *b, line *f);
        int  lineclip(vec2f *a, vec2f *b);

    public:
        // constructor\destructor
        envmap();
        ~envmap();
        
        // timer var pointer
        volatile int *tick;
        
        // main entrypoint =)
        int main(int timedemo);
};

#endif