#ifndef FDPLANES_H
#define FDPLANES_H

class fdplanes {
    private:
        unsigned int  *fdu, *fdv;           // grid u\v coords
        unsigned char *buffer;              // screen buffer
        unsigned char *texture, *texbuf;    // tiled texture
        unsigned char *palxlat, *blendtab;  // additional stuff
        ptc_palette   *pal;                 // palette
        
        // additional procedures
        void initpal();
        void inittexture();
        
        // main procedures
        void drawgrid(int ax, int ay, int az, int rx, int ry, int rz);
        void interpolate();

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