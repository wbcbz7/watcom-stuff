#ifndef BMPDIST_0_H
#define BMPDIST_0_H

class bmpdist_0 {
    private:
        unsigned int   *fdu, *fdv;           // grid u\v coords
        unsigned short *buffer;              // screen buffer
        unsigned short *texture, *texbuf;    // tiled texture
        
        // additional procedures
        void inittexture();
        
        // main procedures
        void drawgrid(int ax, int ay, int t, float mul);
        void interpolate();
        void addval(unsigned short *dst, unsigned short col);

    public:
        // constructor\destructor
        bmpdist_0();
        ~bmpdist_0();
        
        // timer var pointer
        volatile int *tick;
        
        // main entrypoint =)
        int main(int noretrace);
};

#endif