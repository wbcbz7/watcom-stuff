#ifndef RBLUR_0_H
#define RBLUR_0_H

// когда что-то тут поменял - ПЕРЕСОБЕРИ MAIN.CPP!!1
#include "..\..\common\matrix.h"
#include "..\..\common\dlist.h"
class rblur_0 {
    private:
        enum {
            dist = 150,         // perspective projection distance
            flaresnum = 8,      // total num of flares
            flarescount = flaresnum*flaresnum*flaresnum,
            flaretypes = 8,     // num of flare types
            flarescale = 0x600, // flare scale level
            flaredist = 64,     // flare dist
            flaresize = 16      // size of flare sprite, e.g flaresize=8 -> 8x8
        };
        unsigned short *blurtab;             // blur table
        unsigned short *buffer, *buffer2;    // screen buffer
        unsigned short *flare;               // flare sprites
        dlist *d;
        vec4f *p, *pt;                       // vertex coords
        vec2f *p2d;                          // projected vertex coords 
        
        // additional procedures
        void initblur();
        void initflares();
        void initvertex();
        
        // main procedures
        void project3d(vec2f *v, vec4f *f);
        void drawflare(vec2f *f, int type);
        void blur(int sx, int sy);
        void blend();
        void drawbuf(int i, int amp);
        void dirblur(unsigned short *dst, unsigned int len, int disp, unsigned int mask1, unsigned int mask2);
        void filldlist(dlist *d, int vel);

    public:
        // constructor\destructor
        rblur_0();
        ~rblur_0();
        
        // timer var pointer
        volatile int *tick;
        
        // main entrypoint =)
        int main(int noretrace);
};

#endif