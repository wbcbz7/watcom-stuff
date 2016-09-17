// 15bpp -> cga fakemode conv test
// by wbcbz7 zz.ob.zolb

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <math.h>
#include <i86.h>

#define X_RES 320
#define Y_RES 200

#define sat(a, l) (a < l ? a : l)

unsigned char   *screen = (unsigned char*)0xA0000;

// static vars
unsigned short  buffer[X_RES*Y_RES]; // 320x200x15bpp virtual buffer

void setvgamode(unsigned short);
#pragma aux setvgamode = "int 0x10" parm [ax]

void set60hz() {
    // set 320x200/400 @ 60hz mode (640x480 timings)
    // tested in vga modes, probably also work in vesa modes in vga-compatible cards
    // original by type one \\ (tfl-tdv | puple)
    // modified and fixed by wbcbz7
    outp (0x3D4, 0x11); outp(0x3D5, (inp(0x3D5) & 0x7F)); // unlock registers
    outp (0x3C2, (inp(0x3CC) | 0xC0));  // misc. output
    outpw(0x3D4, 0x0B06); // vertical total
    outpw(0x3D4, 0x3E07); // overflow
    outpw(0x3D4, 0xC310); // vertical start retrace
    outpw(0x3D4, 0x8C11); // vertical end retrace
    outpw(0x3D4, 0x8F12); // vertical display enable end
    outpw(0x3D4, 0xB815); // vertical blank start
    outpw(0x3D4, 0xE216); // vertical blank end
}

void setmodex() {
    // disable chain-4
    outp(0x3C4, 0x4);  outp(0x3C5, (inp(0x3C5) & 0xF7));
    // set crtc byte mode
    outp(0x3D4, 0x14); outp(0x3D5, (inp(0x3D5) & 0xBF));
    outp(0x3D4, 0x17); outp(0x3D5, (inp(0x3D5) | 0x40));
}

// mode 1 - 320x400 ~12bpp (like in tp93 report by xorgraphy) - vga mode-x
class mode1 {
    private:
        unsigned char  *pal;
        unsigned short *xlat;
        void initpal(float bright, float gamma, float intermod);
        void setpal();
        
    public:
        mode1(float bright, float gamma, float intermod);
        ~mode1();
        void setmode();
        void update(unsigned short *p);
};

mode1::mode1(float bright, float gamma, float intermod) {
    pal  = new unsigned char[1024];
    xlat = new unsigned short[(1 << 15)];
    
    unsigned char *p = (unsigned char*)xlat;
    
    initpal(bright, gamma, intermod);
        
    // init xlat table
    for (int r = 0; r < 32; r++)
        for (int g = 0; g < 32; g++)
            for (int b = 0; b < 32; b++) {
                // low byte = r*b
                //*p++ = ((g << 7) | 0xF000) | (sat((int)((float)b*(15/32)), 15) << 4) | (r >> 1);
                *p++ = (((b * 15) >> 5) << 4) | (r >> 1);
                // high byte = g
                *p++ = ((g >> 1) | 0xF0);
            }
}

mode1::~mode1() {
    delete[] pal;
    delete[] xlat;
}

void mode1::initpal(float bright, float gamma, float intermod) {
// palette format: 0x00..0xEF - 16*15 R*B grid
//                 0xF0..0xFF - 16 G shades
    unsigned char *p = pal;
    
    float a = 4 * bright * pow(16, (1 - gamma)); // a = 16 ^ (1-gamma)
    // gamma correction for int x in [0; a):
    // x = a * (x ^ gamma)
    float b=0;
    
    // set R*B palette set
    for (int i = 0; i < 15; i++) {
        for (int r = 0; r < 16; r++) {
            *p++ = sat((int)(a * pow((r*(1-intermod)) + ((r+b)*0.5f*intermod), gamma)), 63);
            *p++ = sat((int)(a * pow(((r+b)*0.5f*intermod), gamma)), 63);
            *p++ = sat((int)(a * pow((b*(1-intermod)) + ((r+b)*0.5f*intermod), gamma)), 63);
            p++;
        }
        b += (float)(16/15);
    }
    
    // set G palette set
    for (int g = 0; g < 16; g++) {
        *p++ = sat((int)(a * pow((g*intermod), gamma)), 63);
        *p++ = sat((int)(a * pow(g, gamma)), 63);
        *p++ = sat((int)(a * pow((g*intermod), gamma)), 63);
        p++;
    }
}

void mode1::setpal() {
    unsigned char *p = pal;
    
    outp(0x3C8, 0);
    for (int i = 0; i < 256; i++) {
        outp(0x3C9, *p++);
        outp(0x3C9, *p++);
        outp(0x3C9, *p++);
        p++;
    }
}

void mode1::setmode() {
    setvgamode(0x13);
    setmodex();
    set60hz();
    outp(0x3D4, 0x9);  outp(0x3D5, (inp(0x3D5) & 0x60));
    setpal();
}

void mode1::update(unsigned short *p) {
    unsigned short *t  = xlat;
    unsigned char  *v  = screen;
    unsigned short *pr;
    
    /*
    int x, y, plmask;
    for (int pl = 0; pl < 4; pl++) {
        plmask = 80 * (pl & 1);
        pr = &p[pl]; v = screen;
        outp(0x3C4, 2); outp(0x3C5, (1 << pl)); 
        y = 200;
        while (y--) {
            x = 80;
            while (x--) {
                *(v+plmask)    = (unsigned char)(t[*pr] >> 8);
                *(v+80-plmask) = (unsigned char)(t[*pr] & 0xFF);
                pr += 4; v++;
            }
            v += 80;
        }
    }
    */
    _asm {
        mov     ecx, -4             // trick
        mov     ebx, t
        
        _plane:
        push    ecx                 // 1
        
        // select VGA memory plane
        mov     ah, 1               // .
        and     ecx, 3              // 2
        mov     esi, p              // .
        shl     ah, cl              // 3
        mov     edx, 0x3C4          // . (zero high word of edx also)
        mov     al, 2               // 4
        lea     esi, [esi + 2*ecx]  // . (nice LEA trick btw ;)
        out     dx, ax              // 5
        
        mov     edi, v              // .
        push    ebp                 // 6
        mov     ebp, Y_RES          // .
        
        _y_loop:
        push    ebp
        mov     ebp, X_RES/8
        
        _x_loop:
        mov     dx, [esi]           // 1
        add     esi, 16             // .
        mov     cx, [ebx + 2*edx]   // 2
        mov     dx, [esi - 8]       // .
        mov     al, cl              // 3
        mov     cl, ch              // .
        mov     dx, [ebx + 2*edx]   // 4
        add     edi, 2              // .
        mov     ah, dl              // 5
        mov     ch, dh              // .
        mov     [edi - 2], ax       // 6
        dec     ebp                 // .
        mov     [edi + 78], cx      // 7
        jnz     _x_loop             // .
        
        pop     ebp                 // 1
        add     edi, 80             // .
        dec     ebp                 // 2
        jnz     _y_loop             // .
        
        pop     ebp                 // 1
        pop     ecx                 // .
        inc     ecx                 // 2
        jnz     _plane              // .
    }
}

// mode 2 - 320x200 8bpp grayscale - vga mode0x13
class mode2 {
    private:
        unsigned char *pal;
        unsigned char *xlat;
        void initpal(float bright, float gamma, float intermod);
        void setpal();
        
    public:
        mode2(float bright, float gamma, float intermod);
        ~mode2();
        void setmode();
        void update(unsigned short *p);
};

mode2::mode2(float bright, float gamma, float intermod) {
    pal  = new unsigned char[1024];
    xlat = new unsigned char[(1 << 15)];
    
    unsigned char *p = xlat;
    
    initpal(bright, gamma, intermod);
    
    // init xlat table
    for (int r = 0; r < 32; r++)
        for (int g = 0; g < 32; g++)
            for (int b = 0; b < 32; b++) {
                *p++ = (int)((float)(0.3*r + 0.6*g + 0.1*b) * 8);
            }
}

mode2::~mode2() {
    delete[] pal;
    delete[] xlat;
}

void mode2::initpal(float bright, float gamma, float intermod) {
// palette format: 0x00..0xFF - 256 shades of grey
    unsigned char *p = pal;
    float a = bright * pow(256, (1 - gamma)) / 4; // a = 256 ^ (1-gamma)
    // gamma correction for int x in [0; a):
    // x = a * (x ^ gamma)

    for (int i = 0; i < 256; i++) {
        *p++ = sat((int)(a * pow(i, gamma)), 63);
        *p++ = sat((int)(a * pow(i, gamma)), 63);
        *p++ = sat((int)(a * pow(i, gamma)), 63);
        p++;
    }
}

void mode2::setpal() {
    unsigned char *p = pal;
    
    outp(0x3C8, 0);
    for (int i = 0; i < 256; i++) {
        outp(0x3C9, *p++);
        outp(0x3C9, *p++);
        outp(0x3C9, *p++);
        p++;
    }
}

void mode2::setmode() {
    setvgamode(0x13);
    set60hz();
    setpal();
}

void mode2::update(unsigned short *p) {
    unsigned char *t = xlat;
    unsigned char *v = screen;
    
    _asm {
        push    ebp
        mov     esi, p
        mov     edi, v
        mov     ebp, t
        xor     ebx, ebx
        mov     ecx, 16000 // process 4 pixels in parallel
        
        inner:
        mov     edx, [esi + 4]     // 1
        add     esi, 8             // .
        mov     bx, dx             // 2
        shr     edx, 16            // .
        mov     al, [ebp + ebx]    // 3
        mov     bx, dx             // .
        mov     ah, [ebp + ebx]    // 4
        mov     edx, [esi - 8]     // .
        shl     eax, 16            // 5
        mov     bx, dx             // .
        shr     edx, 16            // 6
        mov     al, [ebp + ebx]    // .
        mov     bx, dx             // 7
        mov     ah, [ebp + ebx]    // .
        add     edi, 4             // 8
        mov     [edi - 4], eax     // .
        dec     ecx                // 9
        jnz     inner              // .
        
        pop     ebp
    }
}

// mode 3 - 320x200 ~7.75bpp web safe colors + 32 grays - vga mode0x13
class mode3 {
    private:
        unsigned char *pal;
        unsigned char *xlat;
        void initpal(float bright, float gamma, float intermod);
        void setpal();
        
    public:
        mode3(float bright, float gamma, float intermod);
        ~mode3();
        void setmode();
        void update(unsigned short *p);
};

mode3::mode3(float bright, float gamma, float intermod) {
    pal  = new unsigned char[1024];
    xlat = new unsigned char[(1 << 15)];
    
    unsigned char *p = xlat;
    
    initpal(bright, gamma, intermod);
    
    // init xlat table
    for (int r = 0; r < 32; r++)
        for (int g = 0; g < 32; g++)
            for (int b = 0; b < 32; b++) {
                *p++ = (((r == g) && (r == b)) ? (r | 0xE0) : ((36 * ((r * 6) / 32)) + (6 * ((g * 6) / 32)) + ((b * 6) / 32)));
            }
}

mode3::~mode3() {
    delete[] pal;
    delete[] xlat;
}

void mode3::initpal(float bright, float gamma, float intermod) {
// palette format: 0x00..0xD7 - 6x6x6 RGB color space
//                 0xE0..0xFF - 32 shades of gray
    unsigned char *p = pal;
    float a = bright * pow(256, (1 - gamma)) / 4; // a = 256 ^ (1-gamma)
    // gamma correction for int x in [0; a):
    // x = a * (x ^ gamma)

    for (int r = 0; r < 6; r++) {
        for (int g = 0; g < 6; g++) {
            for (int b = 0; b < 6; b++) {
                *p++ = sat((int)(a * pow(r * 0x33, gamma)), 63);
                *p++ = sat((int)(a * pow(g * 0x33, gamma)), 63);
                *p++ = sat((int)(a * pow(b * 0x33, gamma)), 63);
                *p++;
            }
        }
    }
    
   p = &pal[(0xE0 << 2)];
   
   for (int i = 0; i < 32; i++) {
        *p++ = sat((int)(a * pow((i << 3), gamma)), 63);
        *p++ = sat((int)(a * pow((i << 3), gamma)), 63);
        *p++ = sat((int)(a * pow((i << 3), gamma)), 63);
        *p++;
    } 
}

void mode3::setpal() {
    unsigned char *p = pal;
    
    outp(0x3C8, 0);
    
    for (int i = 0; i < 256; i++) {
        outp(0x3C9, *p++);
        outp(0x3C9, *p++);
        outp(0x3C9, *p++);
        p++;
    }
}

void mode3::setmode() {
    setvgamode(0x13);
    set60hz();
    setpal();
}

void mode3::update(unsigned short *p) {
    unsigned char *t = xlat;
    unsigned char *v = screen;
    
    _asm {
        push    ebp
        mov     esi, p
        mov     edi, v
        mov     ebp, t
        xor     ebx, ebx
        mov     ecx, 16000 // process 4 pixels in parallel
        
        inner:
        mov     edx, [esi + 4]     // 1
        add     esi, 8             // .
        mov     bx, dx             // 2
        shr     edx, 16            // .
        mov     al, [ebp + ebx]    // 3
        mov     bx, dx             // .
        mov     ah, [ebp + ebx]    // 4
        mov     edx, [esi - 8]     // .
        shl     eax, 16            // 5
        mov     bx, dx             // .
        shr     edx, 16            // 6
        mov     al, [ebp + ebx]    // .
        mov     bx, dx             // 7
        mov     ah, [ebp + ebx]    // .
        add     edi, 4             // 8
        mov     [edi - 4], eax     // .
        dec     ecx                // 9
        jnz     inner              // .
        
        pop     ebp
    }
}

// mode 4 - 320x200 8bpp RGB332 - vga mode0x13
class mode4 {
    private:
        unsigned char *pal;
        unsigned char *xlat;
        void initpal(float bright, float gamma, float intermod);
        void setpal();
        
    public:
        mode4(float bright, float gamma, float intermod);
        ~mode4();
        void setmode();
        void update(unsigned short *p);
};

mode4::mode4(float bright, float gamma, float intermod) {
    pal  = new unsigned char[1024];
    xlat = new unsigned char[(1 << 15)];
    
    unsigned char *p = xlat;
    
    initpal(bright, gamma, intermod);
    
    // init xlat table
    for (int r = 0; r < 32; r++)
        for (int g = 0; g < 32; g++)
            for (int b = 0; b < 32; b++) {
                *p++ = ((r & 0x1C) << 3) | ((g & 0x1C) << 0) | ((b & 0x18) >> 3);
            }
}

mode4::~mode4() {
    delete[] pal;
    delete[] xlat;
}

void mode4::initpal(float bright, float gamma, float intermod) {
// palette format: 0x00..0xFF - RGB332
    unsigned char *p = pal;
    float a = bright * pow(256, (1 - gamma)) / 4; // a = 256 ^ (1-gamma)
    // gamma correction for int x in [0; a):
    // x = a * (x ^ gamma)

    for (int r = 0; r < 8; r++) {
        for (int g = 0; g < 8; g++) {
            for (int b = 0; b < 4; b++) {
                *p++ = sat((int)(a * pow((r * 36), gamma)), 63);
                *p++ = sat((int)(a * pow((g * 36), gamma)), 63);
                *p++ = sat((int)(a * pow((b * 85), gamma)), 63);
                *p++;
            }
        }
    }
}

void mode4::setpal() {
    unsigned char *p = pal;
    
    outp(0x3C8, 0);
    
    for (int i = 0; i < 256; i++) {
        outp(0x3C9, *p++);
        outp(0x3C9, *p++);
        outp(0x3C9, *p++);
        p++;
    }
}

void mode4::setmode() {
    setvgamode(0x13);
    set60hz();
    setpal();
}

void mode4::update(unsigned short *p) {
    unsigned char *t = xlat;
    unsigned char *v = screen;
    
    _asm {
        push    ebp
        mov     esi, p
        mov     edi, v
        mov     ebp, t
        xor     ebx, ebx
        mov     ecx, 16000 // process 4 pixels in parallel
        
        inner:
        mov     edx, [esi + 4]     // 1
        add     esi, 8             // .
        mov     bx, dx             // 2
        shr     edx, 16            // .
        mov     al, [ebp + ebx]    // 3
        mov     bx, dx             // .
        mov     ah, [ebp + ebx]    // 4
        mov     edx, [esi - 8]     // .
        shl     eax, 16            // 5
        mov     bx, dx             // .
        shr     edx, 16            // 6
        mov     al, [ebp + ebx]    // .
        mov     bx, dx             // 7
        mov     ah, [ebp + ebx]    // .
        add     edi, 4             // 8
        mov     [edi - 4], eax     // .
        dec     ecx                // 9
        jnz     inner              // .
        
        pop     ebp
    }
}

// main code
int main(int argc, char *argv[]) {
    
    unsigned short *p = &buffer[0];
    char ch;
    
    
    // load picture
    FILE *f = fopen(argv[1], "rb");
    if (!f) return 1;
    fread(p, sizeof(unsigned short), 64000, f);
    fclose(f);
    
    
    mode4 *mode = new mode4(1, 1, 0);
    
    /*
    // draw pattern
    for (int y = 0; y < Y_RES; y++) 
        for (int x = 0; x < X_RES; x++)
            *p++ = (((x >> 3) & 0x1F) << 0) | (((x >> 3) & 0x1F) << 5) | (((x >> 3) & 0x1F) << 10);
            //*p++ = ((x & 0x1F) << 0) | ((y & 0x1F) << 5);
    
    //for (int i = 0; i < 64000; i++) *p++ = (i & 0x7FFF);
    */
    
    mode->setmode();
    mode->update(&buffer[0]);
    while(!kbhit()); getch();
    
    setvgamode(0x3);
    
    delete mode;
    return 0;
}
