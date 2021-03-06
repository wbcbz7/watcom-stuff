#ifndef RBLUR_H
#define RBLUR_H

// когда что-то тут поменял - ПЕРЕСОБЕРИ MAIN.CPP!!1
#include "matrix.h"
class rblur {
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
        void dirblur(int disp);

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