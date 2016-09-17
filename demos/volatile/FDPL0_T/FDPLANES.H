#ifndef FDPLANES_H
#define FDPLANES_H

// когда что-то тут поменял - ПЕРЕСОБЕРИ MAIN.CPP!!1

class fdplanes {
    private:
        struct fdgrid {
            unsigned int u, v, l, i;
        };
        fdgrid         *fdtab;               // grid node u\v\l values
        unsigned short *buffer, *buffer2;    // screen buffer
        unsigned short *alignbuf;            // alignment buffer
        unsigned short *texture;             // texture
        unsigned short *alignbuf2;           // alignment buffer #2 :)
        unsigned short *lightmap;            // light table
        
        // additional procedures
        void inittexture();
        void initltab();
        
        // main procedures
        void drawgrid(int ax, int ay, int az, int rx, int ry, int rz);
        void interpolate();
        void blend();

    public:
        // constructor\destructor
        fdplanes();
        ~fdplanes();
        
        // timer var pointer
        volatile int *tick;
        
        // main entrypoint =)
        int main(int timedemo);
};

#endif