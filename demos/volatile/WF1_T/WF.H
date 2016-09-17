#ifndef WF_H
#define WF_H

// когда что-то тут поменял - ПЕРЕСОБЕРИ MAIN.CPP!!1
#include "matrix.h"
class wf {
    private:
        typedef struct {
            int a, b, col;
        } line;
    
        enum {
            dist = 150,         // perspective projection distance
            flaresnum = 18,      // total num of flares
            flarescount = flaresnum*flaresnum,
            flaretypes = 8,     // num of flare types
            flarescale = 0x300, // flare scale level
            flaredist = 256,     // flare dist
            flaresize = 12,     // size of flare sprite, e.g flaresize=8 -> 8x8
            
            linesnum = (2 * flarescount) - flaresnum
        };
        unsigned short *blurtab;             // blur table
        unsigned short *buffer, *buffer2;    // screen buffer
        unsigned short *flare;               // flare sprites
        vec4f *p, *pt;                       // vertex coords
        vec2f *p2d;                          // projected vertex coords 
        line  *f;                            // line array
        
        // additional procedures
        void initblur();
        void initflares();
        void initvertex(int color);
        
        // main procedures
        void project3d(vec2f *v, vec4f *f);
        void drawflare(vec2f *f, int type);
        void blur(int sx, int sy);
        void blend();
        void drawbuf(int i, int amp);
        void dirblur(int disp);
        
        void linedraw(vec2f *a, vec2f *b, line *f);
        int  lineclip(vec2f *a, vec2f *b);

    public:
        // constructor\destructor
        wf();
        ~wf();
        
        // timer var pointer
        volatile int *tick;
        
        // main entrypoint =)
        int main(int timedemo);
};

#endif