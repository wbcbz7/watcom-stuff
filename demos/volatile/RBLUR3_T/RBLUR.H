#ifndef RBLUR_H
#define RBLUR_H

// когда что-то тут поменял - ПЕРЕСОБЕРИ MAIN.CPP!!1
#include "matrix.h"
#include "dlist.h"
class rblur {
    private:
        enum {
            dist = 150,         // perspective projection distance
            flaresnum = 1024,     // total num of flares
            flaretypes = 8,     // num of flare types
            flarescale = 0xA00, // flare scale level
            flaredist = 1,     // maximum flare distance
            flaresize = 16      // size of flare sprite, e.g flaresize=8 -> 8x8
        };
        unsigned short *blurtab;             // blur table
        unsigned short *buffer, *buffer2;    // screen buffer
        unsigned short *bgpic, *bgpicbuf;    // bgpic + bgpic screen buffer
        unsigned short *flare;               // flare sprites
        dlist *d;
        vec4f *p, *pt;                       // vertex coords
        vec2f *p2d;                          // projected vertex coords 
        
        // additional procedures
        void initblur();
        void initflares();
        void initvertex(float t);
        
        // main procedures
        void parallelproject(vec2f *v, vec4f *f);
        void drawflare(vec2f *f, int type);
        void blur(int sx, int sy);
        void blend(unsigned short *dst, unsigned short *src);
        void add(unsigned short *dst, unsigned short *src);
        void dirblur(unsigned short *dst, unsigned int len, int disp, unsigned int mask1, unsigned int mask2);
        void filldlist(dlist *d, int vel);
        
    public:
        // constructor\destructor
        rblur();
        ~rblur();
        
        // timer var pointer
        volatile int *tick;
        
        // main entrypoint =)
        int main(int timedemo);
};

#endif