#ifndef FDSPHERE_H
#define FDSPHERE_H

// когда что-то тут поменял - ПЕРЕСОБЕРИ MAIN.CPP!!1
#include "matrix.h"
class fdsphere {
    private:
        struct fdgrid {
            unsigned int u, v, l, i;
        };
        
        enum {
            SphereSize = 128
        };
        
        fdgrid         *fdtab;               // grid node u\v\l values
        unsigned short *buffer, *buffer2;    // screen buffer
        unsigned short *alignbuf;            // alignment buffer
        unsigned short *texture;             // texture
        
        // additional procedures
        
        // main procedures
        void drawgrid(vec4f *o, vec4f *ot, int rx, int ry, int rz);
        void interpolate();
        void blend();

    public:
        // constructor\destructor
        fdsphere();
        ~fdsphere();
        
        // timer var pointer
        volatile int *tick;
        
        // main entrypoint =)
        int main(int timedemo);
};

#endif