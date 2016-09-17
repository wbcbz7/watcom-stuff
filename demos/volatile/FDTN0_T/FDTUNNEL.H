#ifndef FDTUNNEL_H
#define FDTUNNEL_H

// когда что-то тут поменял - ПЕРЕСОБЕРИ MAIN.CPP!!1

#include "matrix.h"
class fdtunnel {
    private:
        enum {
            dist = 90,
            TunnelSize = 384,
            flaresnum = 64,      // total num of flares
            flarescount = flaresnum,
            flaretypes = 4,     // num of flare types
            flarezshades = 8,
            flarescale = 0x600, // flare scale level
            flarezdist = 64,
            flaredif = 16,
            flaredist = 256,     // flare dist
            flaresize = 16      // size of flare sprite, e.g flaresize=8 -> 8x8
        };
    
        struct fdgrid {
            unsigned int u, v, l, i;
        };
        fdgrid         *fdtab;               // grid node u\v\l values
        unsigned short *buffer, *buffer2;    // screen buffer
        unsigned short *alignbuf;            // alignment buffer
        unsigned short *texture;             // texture
        unsigned short *alignbuf2;           // alignment buffer #2 :)
        unsigned short *lightmap;            // light table
        unsigned short *flare;               // flare sprite
        vec4f *p, *pm, *pt;                  // vertex coords
        vec2f *p2d;                          // projected vertex coords 
        
        // additional procedures
        void inittexture();
        void initltab();
        void initflares();
        void initvertex();
        
        // main procedures
        void drawgrid(int ax, int ay, int az, int rx, int ry, int rz, int adj);
        void interpolate();
        void blend();
        void project3d(vec2f *v, vec4f *f);
        void drawflare(vec2f *f, int type);
        void adjustvertex(int t, vec4f *f, int dif);

    public:
        // constructor\destructor
        fdtunnel();
        ~fdtunnel();
        
        // timer var pointer
        volatile int *tick;
        
        // main entrypoint =)
        int main(int timedemo);
};

#endif