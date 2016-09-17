#ifndef TUNNEL_H
#define TUNNEL_H

#include "tgaload.h"
class tunnel {
    private:
        unsigned short *table;               // tunnel table
        unsigned short *buffer;              // screen buffer
        unsigned short *texture, *texbuf;    // tiled texture
        
        // additional procedures
        void inittexture();
        void inittunnel();
        
        // main procedures
        void draw(int t);

    public:
        // constructor\destructor
        tunnel();
        ~tunnel();
        
        // timer var pointer
        volatile int *tick;
        
        // main entrypoint =)
        int main(int timedemo);
};

#endif