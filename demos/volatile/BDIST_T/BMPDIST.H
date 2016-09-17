#ifndef BMPDIST_H
#define BMPDIST_H

class bmpdist {
    private:
        unsigned int   *fdu, *fdv;           // grid u\v coords
        unsigned short *buffer;              // screen buffer
        unsigned short *texture, *texbuf;    // tiled texture
        
        // additional procedures
        void inittexture();
        
        // main procedures
        void drawgrid(int ax, int ay, int t);
        void interpolate();

    public:
        // constructor\destructor
        bmpdist();
        ~bmpdist();
        
        // timer var pointer
        volatile int *tick;
        
        // main entrypoint =)
        int main(int timedemo);
};

#endif