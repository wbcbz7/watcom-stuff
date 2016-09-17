#ifndef ALPHA_H
#define ALPHA_H

// когда что-то тут поменял - ПЕРЕСОБЕРИ MAIN.CPP!!1
#include "matrix.h"
class alpha {
    private:
        typedef struct {
            int active, disp, mask1, mask2;
        } dlist;
        
        enum {
            dist = 150,         // perspective projection distance
            flaresnum = 512,     // total num of flares
            flaretypes = 3,     // num of flare types
            flarescale = 0x600, // flare scale level
            flaredist = 960,     // maximum flare distance
            flaresize = 24      // size of flare sprite, e.g flaresize=8 -> 8x8
        };
        unsigned short *buffer, *buffer2;    // screen buffer
        unsigned short *pic1, *pic2;         // picture buffer
        unsigned short *alignbuf;            // alignment buffer
        unsigned short *alphatab;            // alpha blending table
        unsigned short *flare;               // flare sprites
        dlist *d;                            // display list
        vec4f *p, *pt;                       // vertex coords
        vec2f *p2d;                          // projected vertex coords 
        
        // additional procedures
        void fillpic(unsigned short *f);
        void initflares();
        void initvertex();
        void initalphatab();
        
        // main procedures
        void parallelproject(vec2f *v, vec4f *f);
        void drawflare(vec2f *f, int type);
        void filldlist(dlist *d);
        
        // blending procedures
        void blend(unsigned short *src, unsigned short *dst);
        void ablend(unsigned short *src, unsigned short *dst, unsigned int a);
        void dirblur(unsigned short *dst, unsigned int len, int disp, unsigned int mask1, unsigned int mask2);

    public:
        // constructor\destructor
        alpha(int argc, char *argv[]);
        ~alpha();
        
        // timer var pointer
        volatile int *tick;
        
        // main entrypoint =)
        int main(FILE *f);
};

#endif