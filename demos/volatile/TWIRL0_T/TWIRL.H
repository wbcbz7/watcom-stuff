#ifndef TWIRL_H
#define TWIRL_H

#include "tgaload.h"
class twirl {
    private:
        unsigned short *table;               // twirl table
        unsigned short *buffer;              // screen buffer
        unsigned short *texture, *texbuf;    // tiled texture
        
        // additional procedures
        void inittexture();
        void inittwirl();
        
        // main procedures
        void draw(int t);

    public:
        // constructor\destructor
        twirl();
        ~twirl();
        
        // timer var pointer
        volatile int *tick;
        
        // main entrypoint =)
        int main(int timedemo);
};

#endif