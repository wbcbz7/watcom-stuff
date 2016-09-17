#ifndef LOGO_H
#define LOGO_H

// когда что-то тут поменял - ПЕРЕСОБЕРИ MAIN.CPP!!1
#include "..\..\common\dlist.h"
class logo {
    private:
    
        unsigned short *buffer;             // screen buffer
        unsigned short *bgpic, *namepic;    // pictures
        dlist          *d;
        
        // main procedures
        void transblit(unsigned short *dst, unsigned short *src, unsigned int xres, unsigned int yres);
        void blur(unsigned short *dst);
        void add(unsigned short *dst, unsigned short *src);
        void dirblur(unsigned short *dst, unsigned int len, int disp, unsigned int mask1, unsigned int mask2);
        void filldlist(dlist *d, int vel);
        void addval(unsigned short *dst, unsigned short col);
        
    public:
        // constructor\destructor
        logo();
        ~logo();
        
        // timer var pointer
        volatile int *tick;
        
        // main entrypoint =)
        int main(int noretrace);
};

#endif