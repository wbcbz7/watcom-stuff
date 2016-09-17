#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <math.h>
#include <i86.h>
#include "rtctimer.h"
#include "..\flexptc\flexptc.h"
#include "tgaload.cpp"

#define X_RES 320
#define Y_RES 200

#define sat(a, l) (a < l ? a : l)
#define ee         1.0E-6

#define pi         3.141592653589793

unsigned char   *screen = (unsigned char*)0xA0000;
ptc_context mode;

// static vars
unsigned short blurtab[X_RES*Y_RES*2];
unsigned short buffer[X_RES*Y_RES];

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

void initblurtab() {
    unsigned short *p = (unsigned short*)&blurtab;
    float n, nx, ny;
    //float scale = 0.8;
    //float scale = 0x20000;
    float scale = 0x18000;
    
    for (int y = 0; y < Y_RES; y++) {
        for (int x = 0; x < X_RES; x++) {
            //n = scale / (x*x + y*y);
            //n = sqrt(x*x + y*y) / 8 + 8;
            n = 10;
            nx = (x / n);
            ny = (y / n);
            *p++ = (unsigned short)ny * X_RES;
            *p++ = (unsigned short)nx;
            //*p++ = (unsigned short)(ny > Y_RES ? Y_RES : (int)ny * X_RES);
            //*p++ = (unsigned short)(nx > X_RES ? X_RES : nx);
        }
    }
}

void drawblur(int sx, int sy) {
    unsigned short *p = (unsigned short*)&buffer;
    unsigned short *v = (unsigned short*)&buffer;
    unsigned short *t = (unsigned short*)&blurtab;
    
    const unsigned int BLENDMASK = 0x7BDE;
    int t_add, p_add, x_count, y_count;
    
    // upper left corner
    t = (unsigned short*)&blurtab;
    p = (unsigned short*)&buffer[(sx-1) + ((sy-1) << 6) + ((sy-1) << 8)];
    /*
    for (int y = sy-1; y >= 0; y--) {
        for (int x = sx-1; x >= 0; x--) {
            int ty = *t, tx = *(t + 1);
            *p = (((*p & BLENDMASK) >> 1) + ((*(p + tx + ty) & BLENDMASK) >> 1));
            p--; t += 2;
        }
        t += 2*(X_RES - sx); p -= (X_RES - sx);
    }
    */
    t_add = 4*(X_RES - sx); p_add = -2*(X_RES - sx); x_count = sx; y_count = sy;
    _asm {
        mov     esi, t
        mov     edi, p
        mov     ecx, y_count
        
        _y_loop:
        push    ecx
        mov     ecx, x_count
        xor     eax, eax
        xor     ebx, ebx
        xor     edx, edx
        
        _x_loop:
        mov     ebx, [esi]              // 1
        add     esi, 4                  // .
        
        mov     dx, bx                  // 2
        mov     ax, [edi]               // .
        
        shr     ebx, 16                 // 3
        add     edi, -2                 // .
        
        add     edx, ebx                // 4
        
        mov     dx, [edi + 2*edx + 2]   // 5
        and     eax, 0x7BDE7BDE         // .
        
        shr     eax, 1                  // 6
        and     edx, 0x7BDE7BDE         // .
        
        shr     edx, 1                  // 7
        
        add     eax, edx                // 8
        dec     ecx                     // .
        
        mov     [edi + 2], ax           // 9
        jnz     _x_loop                 // .

        pop     ecx
        add     edi, p_add
        add     esi, t_add
        dec     ecx
        jnz     _y_loop
    } 
    
    // upper right corner
    t = (unsigned short*)&blurtab;
    p = (unsigned short*)&buffer[sx + ((sy-1) << 6) + ((sy-1) << 8)];
    /*
    for (int y = sy-1; y >= 0; y--) {
        for (int x = sx; x < X_RES; x++) {
            int ty = *t, tx = *(t + 1);
            *p = (((*p & BLENDMASK) >> 1) + ((*(p - tx + ty) & BLENDMASK) >> 1));
            p++; t += 2;
        }
        t += 2*sx; p -= (2*X_RES - sx);
    }
    */
    t_add = 4*sx; p_add = -2*(2*X_RES - sx); x_count = X_RES - sx; y_count = sy;
    _asm {
        mov     esi, t
        mov     edi, p
        mov     ecx, y_count
        
        _y_loop:
        push    ecx
        mov     ecx, x_count
        xor     eax, eax
        xor     ebx, ebx
        xor     edx, edx
        
        _x_loop:
        mov     ax, [edi]               // 1
        mov     ebx, [esi]              // .
        
        mov     dx, bx                  // 2
        add     esi, 4                  // .
        
        shr     ebx, 16                 // 3 
        and     eax, 0x7BDE7BDE         // .
        
        shr     eax, 1                  // 4
        sub     edx, ebx                // .
        
        mov     dx, [edi + 2*edx]       // 5
        add     edi, 2                  // .
        
        and     edx, 0x00007BDE         // 6
        shr     edx, 1                  // 7
        
        add     eax, edx                // 8
        dec     ecx                     // .
        
        mov     [edi - 2], ax           // 9
        jnz     _x_loop                 // .

        pop     ecx
        add     edi, p_add
        add     esi, t_add
        dec     ecx
        jnz     _y_loop
    }    
    
    // lower left corner
    t = (unsigned short*)&blurtab;
    p = (unsigned short*)&buffer[(sx-1) + (sy << 6) + (sy << 8)];
    /*
    for (int y = sy; y < Y_RES; y++) {
        for (int x = sx-1; x >= 0; x--) {
            int ty = *t, tx = *(t + 1);
            *p = (((*p & BLENDMASK) >> 1) + ((*(p + tx - ty) & BLENDMASK) >> 1));
            p--; t += 2;
        }
        t += 2*(X_RES - sx); p += (X_RES + sx);
    }
    */
    t_add = 4*(X_RES - sx); p_add = 2*(X_RES + sx); x_count = sx; y_count = Y_RES - sy;
    _asm {
        mov     esi, t
        mov     edi, p
        mov     ecx, y_count
        
        _y_loop:
        push    ecx
        mov     ecx, x_count
        xor     eax, eax
        xor     ebx, ebx
        xor     edx, edx
        
        _x_loop:
        mov     ax, [edi]               // 1
        mov     ebx, [esi]              // .
        
        mov     dx, bx                  // 2
        add     esi, 4                  // .
        
        shr     ebx, 16                 // 3 
        and     eax, 0x7BDE7BDE         // .
        
        shr     eax, 1                  // 4
        sub     edx, ebx                // .
        
        xor     edx, 0xFFFFFFFF         // 5
        add     edi, -2                 // .
        
        mov     dx, [edi + 2*edx + 4]   // 6
        
        and     edx, 0x00007BDE         // 7
        shr     edx, 1                  // 8
        
        add     eax, edx                // 9
        dec     ecx                     // .
        
        mov     [edi + 2], ax           // 10
        jnz     _x_loop                 // .

        pop     ecx
        add     edi, p_add
        add     esi, t_add
        dec     ecx
        jnz     _y_loop
    }    
    
    // lower right corner
    t = (unsigned short*)&blurtab;
    p = (unsigned short*)&buffer[sx + (sy << 6) + (sy << 8)];
    /*
    for (int y = sy; y < Y_RES; y++) {
        for (int x = sx; x < X_RES; x++) {
            int ty = *t, tx = *(t + 1);
            *p = (((*p & BLENDMASK) >> 1) + ((*(p - tx - ty) & BLENDMASK) >> 1));
            p++; t += 2;
        }
        t += 2*sx; p += sx;
    }
    */
    t_add = 4*sx; p_add = 2*sx; x_count = X_RES - sx; y_count = Y_RES - sy;
    _asm {
        mov     esi, t
        mov     edi, p
        mov     ecx, y_count
        
        _y_loop:
        push    ecx
        mov     ecx, x_count
        xor     eax, eax
        xor     ebx, ebx
        xor     edx, edx
        
        _x_loop:
        mov     ax, [edi]               // 1
        mov     ebx, [esi]              // .
        
        mov     dx, bx                  // 2
        add     esi, 4                  // .
        
        shr     ebx, 16                 // 3 
        and     eax, 0x7BDE7BDE         // .
        
        shr     eax, 1                  // 4
        add     edx, ebx                // .
        
        xor     edx, 0xFFFFFFFF         // 5
        add     edi, 2                  // .
        
        mov     dx, [edi + 2*edx]       // 6
        
        and     edx, 0x00007BDE         // 7
        shr     edx, 1                  // 8
        
        add     eax, edx                // 9
        dec     ecx                     // .
        
        mov     [edi - 2], ax           // 10
        jnz     _x_loop                 // .

        pop     ecx
        add     edi, p_add
        add     esi, t_add
        dec     ecx
        jnz     _y_loop
    }
}
short sintab[65536], costab[65536];
float sintabf[65536], costabf[65536];

void initsintab() {
    int i, j;
    float r;
    
    for (i = 0; i < 65536; i++) {
        r = (sin(2 * pi * i / 65536));
        sintab[i] = 32767 * r;
        sintabf[i] = r;
        r = (cos(2 * pi * i / 65536));
        costab[i] = 32767 * r;
        costabf[i] = r;
    }
}

volatile unsigned short sx, sy;

unsigned short initmouse();
#pragma aux initmouse = "xor ax, ax" "int 0x33" value [ax] modify [ax];
void showmouse();
#pragma aux showmouse = "mov ax, 0x1" "int 0x33" modify [ax];
void getmouse() {
    _asm {
        mov     ax, 0x3
        int     0x33
        
        mov     [sx], cx
        mov     [sy], dx
    }
}

volatile int tick = 0;
void timer() {tick++;}

t_image pic;

int main(int argc, char *argv[]) {
    int i, timedemo = 0;
    
    initblurtab();
    initsintab();
    
    if (img_loadfile(&pic, argv[1])) {printf("can't load TGA file"); return 1;}
    if ((pic.width != 320) || (pic.height != 200) || (pic.bpp != 15)) {printf("unsupported TGA file"); return 1;}
    
    if (ptc_init()) return 1;
    if (ptc_open("", X_RES, Y_RES, 15, ptc_OPEN_FIND60HZ, &mode)) {ptc_close(); return 1;}
    
    if (strstr(strupr(argv[2]), "T"))  timedemo = 1;
    
    ptc_setcontext(&mode);
    
    ptc_update(pic.ptr);
    while(!kbhit()); getch();
    
    rtc_initTimer(3);
    rtc_setTimer(&timer, rtc_timerRate / 60);
    
    i = 0;
    while (timedemo ? (i < 1024) : !kbhit()) {
        if (timedemo) i++; else {i = tick; ptc_wait();}
        
        ptc_update(&buffer);
        memcpy(&buffer, pic.ptr, 128000);
        
        drawblur((X_RES/3) * costabf[(i << 7) & 0xFFFF] + (X_RES/2), (Y_RES/3) * sintabf[(i << 8) & 0xFFFF] + (Y_RES/2));
    }
    if (!timedemo) getch();
    
    rtc_freeTimer();
    
    ptc_close();
    if (timedemo) printf("timed %d timerticks in %d frames - %f fps", tick, i, (double)(60 * i) / tick);
    
    img_free(&pic);
    return 0;
}