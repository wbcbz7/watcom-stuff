#ifndef CUBE_H
#define CUBE_H

// когда что-то тут поменял - ПЕРЕСОБЕРИ MAIN.CPP!!1
#include "..\..\common\dlist.h"
#include "..\..\common\alpha.h"
class cube {
    private:
    
        unsigned short *buffer;             // screen buffer
        unsigned short *bgpic;    // pictures
        dlist          *d;
        
        // main procedures
        void add(unsigned short *dst, unsigned short *src);
        void dirblur(unsigned short *dst, unsigned int len, int disp, unsigned int mask1, unsigned int mask2);
        void filldlist(dlist *d, int vel);
        void addval(unsigned short *dst, unsigned short col);
        
    public:
        // constructor\destructor
        cube();
        ~cube();
        
        // timer var pointer
        volatile int *tick;
        
        // main entrypoint =)
        int main(int noretrace);
};

#endif