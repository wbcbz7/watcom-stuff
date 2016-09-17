
#include <math.h>
#include <strings.h>
#include <stdlib.h>
#include <conio.h>
#include <stdio.h>

#include "usmplay.h"
#include "rtctimer.h"

#define X_SIZE 320
#define Y_SIZE 200
#define DIST   300

typedef struct {
    float x, y, z, d;
} vertex;

typedef struct { float x, y; } vertex2d;

short sintab[65536];
float sintabf[65536], costabf[65536];

unsigned char  *screen = 0xA0000;

#define pi        3.141592653589793
#define ee        1.0E-6
#define bb        2 * 1.0E+4
#define bb2       1.0E+6

#define sat(a, l) ((a > l) ? l : a)
#define sqr(a)    ((a)*(a))
#define sgn(a)    (((a) > 0) ? 1 : -1)

USM *module;
int notimer = 0, noretrace = 0, mode13h = 0, manual = 1, lowres = 0, fakelowres = 0, lvdpidor = 0;

//--------------------------------------------------

void initsintab() {
    int i, j;
    float r;
    
    for (i = 0; i < 65536; i++) {
        r = (sin(2 * pi * i / 65536));
        sintab[i] = 32767 * r;
        sintabf[i] = r;
        r = (cos(2 * pi * i / 65536));
        costabf[i] = r;
    }
}

int normal = 0;

void KillAll() {
    USMP_StopPlay();
    USMP_FreeModule(module);
    
    setvidmode(3); if (lvdpidor == 0) rtc_freeTimer();
    if ((lvdpidor == 0) && (Error_Number!=0)) { Display_Error(Error_Number); exit(0); }
    if (normal == 0) {if (lvdpidor == 1) {
        hp_main();
        setvidmode(0x13);
        outpw(0x3D4, ((rand() % 256) << 8) | 0x13);
        outpw(0x3D4, ((rand() % 256) << 8) | 0x9);
        w1_main();
    } else {puts("hey, why you pressed that escape key?!\0"); exit(0);}} 
}

//------------------------------------------------

#include "parts\\tunnel.c"
#include "parts\\tunnel2.c"
#include "parts\\3dstuff.c"
#include "parts\\twirl.c"
#include "parts\\twirl2.c"
#include "parts\\twirl3.c"
#include "parts\\twirl4.c"
#include "parts\\3dstuff2.c"
#include "parts\\fdtunnel.c"

#include "parts\\slime.c" // hidden paaaaaart! :)
#include "parts\\fdplanes.c"



void setvidmode(char mode) { 
    _asm {
        mov ah, 0
        mov al, mode
        int 10h 
    } 
}

void set160x200() {
    // thanks to type one \\ (tfl-tdv | pulpe(??)) for info about this mode
    // translated to C, should work with S3 cards
    
    // вся жопа в том что нормально этот код на реалах не работает :)
    
    // method one - set mode 0xD and turn 256 color mode on hand
    // 10% works on matrox, ati and S3 (serious palette corruption)
    // 1%  works on trident (even more serious corruption)
    // i guess that it should work on tseng, but dunno

    /*

    setvidmode(0xD);
    
    outp (0x3D4, 9); outp(0x3D5, ((inp(0x3D5) & 0x60) | 0x3));
    outpw(0x3D4, 0x2813);
    
    // unlock S3 extensions
    outpw(0x3C4, 0x0608);
    outpw(0x3D4, 0x4838);
    // and test for presence of S3 card
    outp (0x3D4, 0x30);
    // if S3 detected - use S3 method of VCLK/2
    if (inp(0x3D5) >  0x80) {
        outp(0x3C4, 1);    outp(0x3C5, (inp(0x3C5) & 0xF7));
        outp(0x3C4, 0x15); outp(0x3C5, (inp(0x3C5) | 0x10));
    }
    // lock S3 extensions
    outpw(0x3C4, 0x0008);
    outpw(0x3D4, 0x0038);
        
  
    outpw(0x3C4, 0x0E04);
    outpw(0x3D4, 0x4014);
    outpw(0x3D4, 0xA317);
    outp (0x3CE, 5); outp(0x3CF, (inp(0x3CF) | 0x40));
    inp  (0x3DA); outp(0x3C0, 0x30); outp(0x3C0, 0xC1);
    inp  (0x3DA); outp(0x3C0, 0x34); outp(0x3C0, 0x00);
    
    */

    // method two - set mode 13h and then load horiz.params from mode Dh
    // 100% works on matrox, 70% on S3 (palette corruption)
    // does not work on nvidia and crashes on ati (why?)
    
    //setvidmode(0x13);
    outp (0x3D4, 0x11); outp(0x3D5, (inp(0x3D5) & 0x7F));
    
    outpw(0x3C4, 0x0901); // FUCK YOU S3! :)
    
    // but...they strikes back!
    // unlock S3 extensions
    outpw(0x3C4, 0x0608);
    outpw(0x3D4, 0x4838);
    // and test for presence of S3 card
    outp (0x3D4, 0x30);
    // if S3 detected - use S3 method of VCLK/2
    if (inp(0x3D5) >  0x80) {
        outp(0x3C4, 1);    outp(0x3C5, (inp(0x3C5) & 0xF7));
        outp(0x3C4, 0x15); outp(0x3C5, (inp(0x3C5) | 0x10));
    }
    // lock S3 extensions
    outpw(0x3C4, 0x0008);
    outpw(0x3D4, 0x0038);
    
    outpw(0x3D4, 0x2D00);
    outpw(0x3D4, 0x2701);
    outpw(0x3D4, 0x2802);
    outpw(0x3D4, 0x9003);
    outpw(0x3D4, 0x2B04);
    outpw(0x3D4, 0x8F05);
    
    outpw(0x3D4, 0x1413);
    
    // blacklist (use fake mode):
    // currus logic - out of range DAMNIT!
    // fuckinTrident - DAMN IT LOOKS SOOOOOO ULGY!
    // nvidia - just does not work at all :)

    // короче, 160x100 работает нормально только на матроксе :D
}

void set60hz() {
    // again thanks to type one for info ;)
    
    ///*
    outp (0x3D4, 0x11); outp(0x3D5, (inp(0x3D5) & 0x7F)); // unlock registers
    outp (0x3C2, 0xE3);   // misc. output
    outpw(0x3D4, 0x0B06); // vertical total
    outpw(0x3D4, 0x3E07); // overflow
    outpw(0x3D4, 0xC310); // vertical start retrace
    outpw(0x3D4, 0x8C11); // vertical end retrace
    outpw(0x3D4, 0x8F12); // vertical display enable end
    outpw(0x3D4, 0x9015); // vertical blank start
    outpw(0x3D4, 0x0B16); // vertical blank end
    //*/
    /*
    _asm {
        mov dx,3d4h   // remove protection
        mov al,11h
        out dx,al
        inc dl
        in  al,dx
        and al,7fh
        out dx,al

        mov dx,3c2h   // misc output
        mov al,0e3h   // clock
        out dx,al

        mov dx,3d4h
        mov ax,00B06h // Vertical Total
        out dx,ax
        mov ax,03E07h // Overflow
        out dx,ax
        mov ax,0C310h // Vertical start retrace
        out dx,ax
        mov ax,08C11h // Vertical end retrace
        out dx,ax
        mov ax,08F12h // Vertical display enable end
        out dx,ax
        mov ax,09015h // Vertical blank start
        out dx,ax
        mov ax,00B16h // Vertical blank end
        out dx,ax 
    }
    //*/
}

//------------------------------------------------

int main(int argc, char *argv[]) {
    int p, ch;
    
    for (p = 1; p < argc; p++) {
        if (strcmp(strupr(argv[p]), "NOTIMER") == 0)   notimer   = 1;
        if (strcmp(strupr(argv[p]), "NORETRACE") == 0) noretrace = 1;
        if (strcmp(strupr(argv[p]), "70HZ") == 0)      mode13h   = 1;
        if (strcmp(strupr(argv[p]), "LOWRES") == 0)    lowres    = 1;
        if (strcmp(strupr(argv[p]), "SETUP") == 0)     manual    = 0;
        
        if (strcmp(strupr(argv[p]), "AAA") == 0) {
           puts("ЫЫЫ, за срач на поуете надо отвечать, мудерилка картонная :)\0"); exit(0); }
        if (strcmp(strupr(argv[p]), "BREEZE") == 0) {
           puts("sasha? who? dunno ;)\0"); exit(0); }
        if (strcmp(strupr(argv[p]), "KARBO") == 0) {
           puts("\0");
           puts(" <*> я чистил тут смари\0");
           puts(" <*> карбофоса мачетой\0");
           puts(" <*> рубанул сука\0");
           puts(" <*> по пальцу!\0");
           puts(" <*> заклеил суперклеем\0");
           puts(" <*> 20 лет прошло\0");
           puts(" <*> а шрам остался\0");
           puts(" <*> и карбо прыгает как укупник на параше\0");
           puts("\0");
           puts(" поехавший крабосос strikes down! in da toilet!\0");
           exit(0); }
        if (strcmp(strupr(argv[p]), "LVD_IDIOT") == 0) {lvdpidor = 1;} 
        if (strcmp(strupr(argv[p]), "WCT") == 0) {puts("буээээээээээээээээ!\0"); exit(0);}
    }
    
    HardwareInit(_psp);
    rtc_initTimer();
    if (manual == 0) {USS_Setup();} else {puts("run \"blash setup\" for manual sound setup"); USS_AutoSetup();}
    if (Error_Number!=0) { Display_Error(Error_Number); exit(0); }

    if (notimer   == 1) {puts("timer sync disabled\0");}
    if (noretrace == 1) {puts("vertical retrace sync disabled\0");}
    if (mode13h   == 1) {puts("320x200 70Hz mode used\0");}
    
    if (lowres == 1) {
        puts("select 160x200 mode: ");
        puts("1 - hardware (Matrox\\S3\\Tseng?)");
        puts("2 - fake (other cards and S3 also if first one is buggy)");
    
        do {ch = getch();} while (!strchr ("12\x1B", ch));
        fakelowres = ((ch == 0x32) ? 1 : 0); // а мы тут, мать твою, костылями балуемся (:
        if (ch == 27) {KillAll();}
    }
    
    cputs("init .");

    module = XM_Load(LM_File, 0x020202020, "musik.xm");
    if (Error_Number!=0) { puts(".\0"); Display_Error(Error_Number); exit(0); }
    
    cputs(".");
    initsintab();
    cputs(".");
    t1_init();
    cputs(".");
    t2_init();
    cputs(".");
    fx1_init();
    cputs(".");
    w1_init();
    cputs(".");
    w2_init();
    cputs(".");
    w3_init();
    cputs(".");
    w4_init();
    cputs(".");
    fx2_init();
    cputs(".");
    fd1_init();
    cputs(".");
    fd2_init();
    puts(".");
    
    
    puts("get down! ;)\0");
    
    setvidmode(0x13);    

    if ((lowres == 1) && (fakelowres == 0)) {set160x200();}
    if (mode13h == 0) {set60hz();}
    
    USS_SetAmpli(150); // to prevent clipping on sbcovoxpcspeaker                 
    USMP_StartPlay(module);
    USMP_SetOrder(0);
    
    if (Error_Number!=0) { setvidmode(3); Display_Error(Error_Number); exit(0); }
    
    while (Row < 32) {}
    inp  (0x3DA); outp(0x3C0, 0x31); outp(0x3C0, 0xFF);
    t1_main();
    inp  (0x3DA); outp(0x3C0, 0x31); outp(0x3C0, 0);
    t2_main();
    fx1_main();
    if ((lowres == 1) && (fakelowres == 0)) {setvidmode(0x13); if (mode13h == 0) {set60hz();}}
    w1_main();
    w2_main();
    w3_main();
    w4_main();
    fx2_main();
    
    if ((lowres == 1) && (fakelowres == 0)) {set160x200();     if (mode13h == 0) {set60hz();}}
    fd1_main();
    fd2_main();
    
    normal = 1;
    KillAll();

    
    puts("blash - final - b-state - 2015\0");
    
}
