#include <stdio.h>
#include <math.h>
#include "rtctimer.h"
#include "vbe.c"

#define clockdivisor 6

#define infostr "vidbench v.0.6 - testing VGA\\VESA system perfomance - by wbc\\bz7 ob.o3.2o16"

int counter = 0, burstnum = 64, quiet = 0, info = 0;

typedef struct {
    int write8,  read8,  move8,
        write16, read16, move16,
        write32, read32, move32;
} _result;

typedef struct {
    int vga;
    int mode, lfb, unchain, xres, yres, bpp, fbsize;
} _mode;

_result           result;
_mode             mode;

vbe_VbeInfoBlock  vbeinfo;
vbe_ModeInfoBlock modeinfo;

unsigned char     *scrptr = (unsigned char*)0xA0000;
unsigned char     mbuffer[128 * 1024]; // 128kb move buffer
unsigned short    rndbuffer[256];

void timerproc() {
    counter++;
}

// write procedures

void write8(unsigned int a);
#pragma aux write8 =  "mov edi, scrptr"  \
                      "mov ecx, 64*1024" \
                      "rep stosb"        \
                      parm [eax] modify [ecx edi];

void write16(unsigned int a);
#pragma aux write16 = "mov edi, scrptr"  \
                      "mov ecx, 32*1024" \
                      "rep stosw"        \
                      parm [eax] modify [ecx edi];

void write32(unsigned int a);
#pragma aux write32 = "mov edi, scrptr"  \
                      "mov ecx, 16*1024" \
                      "rep stosd"        \
                      parm [eax] modify [ecx edi];

//read procedures

void read8();
#pragma aux read8 =   "mov esi, scrptr"  \
                      "mov ecx, 64*1024" \
                      "rep lodsb"        \
                      modify [eax ecx esi];

void read16();
#pragma aux read16 =  "mov esi, scrptr"  \
                      "mov ecx, 32*1024" \
                      "rep lodsw"        \
                      modify [eax ecx esi];

void read32();
#pragma aux read32 =  "mov esi, scrptr"  \
                      "mov ecx, 16*1024" \
                      "rep lodsd"        \
                      modify [eax ecx esi];
                  
// move procedures
                      
void move8(long *ptr);
#pragma aux move8 =   "mov edi, scrptr"  \
                      "mov ecx, 64*1024" \
                      "rep movsb"        \
                      parm [esi] modify [ecx edi];

void move16(long *ptr);
#pragma aux move16 =  "mov edi, scrptr"  \
                      "mov ecx, 32*1024" \
                      "rep movsw"        \
                      parm [esi] modify [ecx esi];

void move32(long *ptr);
#pragma aux move32 =  "mov edi, scrptr"  \
                      "mov ecx, 16*1024" \
                      "rep movsd"        \
                      parm [esi] modify [ecx esi];
                      
//----------------------------

void setbiosmode (unsigned short c);
#pragma aux setbiosmode = "int 0x10" parm[ax] modify[eax ebx ecx edx esi edi];
//void setvesamode (unsigned short c);
//#pragma aux setvesamode = "mov ax, 0x4F02" "int 0x10" parm[bx] modify[eax ebx ecx edx esi edi];
unsigned char vesatest();
#pragma aux vesatest = "mov ax, 0x4F03" "int 0x10" value[al] modify[eax ebx ecx edx esi edi];


#define error(n) {puts(n), exit(1);}


void setmodex() {
   outpw(0x3C4, 0x0604);
   outpw(0x3D4, 0xE317);
   outpw(0x3D4, 0x0014);
}

void test8() {
    int i, c0, c1;
    
    outp(0x3C4, 0x2);
    
    c0 = counter;
    for (i = 0; i < burstnum; i++) {write8(rand() & 0xFF); if ((mode.mode < 0x13) || (mode.unchain)) {outp(0x3C5, (counter & 0xF));}}
    c1 = counter; result.write8 = ((burstnum << 16) / (c1 - c0)) * rtc_timerRate;
    
    c0 = counter;
    for (i = 0; i < burstnum; i++) {read8();}
    c1 = counter; result.read8 = ((burstnum << 16) / (c1 - c0)) * rtc_timerRate;
    
    c0 = counter;
    for (i = 0; i < burstnum; i++) {move8((long*)&mbuffer + rndbuffer[i]); if ((mode.mode < 0x13) || (mode.unchain)) {outp(0x3C5, (counter & 0xF));}}
    c1 = counter; result.move8 = ((burstnum << 16) / (c1 - c0)) * rtc_timerRate;
}

void test16() {
    int i, c0, c1;
    
    c0 = counter;
    for (i = 0; i < burstnum; i++) {write16((rand() & 0xFF) * 0x0101); if ((mode.mode < 0x13) || (mode.unchain)) {outp(0x3C5, (counter & 0xF));}}
    c1 = counter; result.write16 = ((burstnum << 16) / (c1 - c0)) * rtc_timerRate;
    
    c0 = counter;
    for (i = 0; i < burstnum; i++) {read16();}
    c1 = counter; result.read16 = ((burstnum << 16) / (c1 - c0)) * rtc_timerRate;
    
    c0 = counter;
    for (i = 0; i < burstnum; i++) {move16((long*)&mbuffer + rndbuffer[i]); if ((mode.mode < 0x13) || (mode.unchain)) {outp(0x3C5, (counter & 0xF));}}
    c1 = counter; result.move16 = ((burstnum << 16) / (c1 - c0)) * rtc_timerRate;
}

void test32() {
    int i, c0, c1;
    
    c0 = counter;
    for (i = 0; i < burstnum; i++) {write32((rand() & 0xFF) * 0x01010101); if ((mode.mode < 0x13) || (mode.unchain)) {outp(0x3C5, (counter & 0xF));}}
    c1 = counter; result.write32 = ((burstnum << 16) / (c1 - c0)) * rtc_timerRate;
    
    c0 = counter;
    for (i = 0; i < burstnum; i++) {read32();}
    c1 = counter; result.read32 = ((burstnum << 16) / (c1 - c0)) * rtc_timerRate;
    
    c0 = counter;
    for (i = 0; i < burstnum; i++) {move32((long*)&mbuffer + rndbuffer[i]); if ((mode.mode < 0x13) || (mode.unchain)) {outp(0x3C5, (counter & 0xF));}}
    c1 = counter; result.move32 = ((burstnum << 16) / (c1 - c0)) * rtc_timerRate;
}

void fillbuffer() {
    int i, j;
    for (i = 0; i < 128 * 1024; i++) {
        mbuffer[i] = (rand() & 0xFF);
    }
    for (i = 0; i < burstnum; i++) {rndbuffer[i] = (rand() & 0x7FFF);}
}

void help() {
    puts(infostr);
    puts("");
    puts("command line switches:");
    puts(" -q, -quiet    - quiet mode");
    puts(" -m(mode)      - select VGA\\VESA mode (in HEXADECIMAL, default is mode 0x13)");
    puts(" -f(x),(y),(b) - find VESA mode with desired resolution and bpp");
    puts(" -l, -lfb      - use linear frame buffer for VESA modes (requires VBE 2.0)");
    puts(" -i            - display adapter info");
    puts(" -mx           - use unchained mode-x (use only with -m13!)");
    puts(" -b(num)       - set transfer size in (num * 64 KiB), default is 4096 KiB");
    puts("");
    puts("example: vidbench.exe -m12 -b16    - test VGA mode 0x12 with 1024 KiB transfer");
    puts("example: vidbench.exe -m13 -mx -q  - test VGA mode-x, quiet mode");
    puts("example: vidbench.exe -f640,480,16 - find 640x480x16bpp VESA mode and test it");
    puts("example: vidbench.exe -m101 -lfb   - test VESA 640x480x8bpp mode with LFB");
}

int main(int argc, char *argv[]) {
    int i;
    
    srand(inp(0x40) | (inp(0x40) << 8));
    fillbuffer();
    
    // default settings
    mode.mode = 0x13; mode.vga = 1; mode.unchain = 0; mode.lfb = 0; // где вы видели LFB у режима 0x13? :)
    
    for (i = 1; i < argc; i++) {
        if (strstr(strupr(argv[i]), "I"))  {info = 1; continue;}
        if (strstr(strupr(argv[i]), "L"))  {mode.lfb = 1; continue;}
        
        // direct mode set
        if (strstr(strupr(argv[i]), "MX")) {mode.unchain = 1; continue;}
        if (strstr(strupr(argv[i]), "M"))  {sscanf(strstr(strupr(argv[i]), "M") + strlen("M"), "%x", &mode.mode); if (mode.mode >= 0x100) mode.vga = 0; continue;}
        
        // find mode and set it
        if (strstr(strupr(argv[i]), "F"))  {sscanf(strstr(strupr(argv[i]), "F") + strlen("F"), "%i,%i,%i", &mode.xres, &mode.yres, &mode.bpp); mode.vga = 0; continue;}
        
        // set burst size
        if (strstr(strupr(argv[i]), "B"))  {sscanf(strstr(strupr(argv[i]), "B") + strlen("B"), "%i", &burstnum); continue;}
        if (strstr(strupr(argv[i]), "Q"))  {quiet = 1; continue;}
        if (strstr(strupr(argv[i]), "?"))  {help(); exit(0);}
    }
    
    if (!quiet) puts(infostr);
    
    if (mode.vga == 0) {
        if (vesatest() != 0x4F) {puts("error: can't find VESA BIOS Extensions!"); exit(1);}
        if (vbe_Init()) {puts("error: can't init VBE interface!"); vbe_Done(); exit(1);}
        vbe_ControllerInfo(&vbeinfo);
        if (vbe_status != 0x4F) {puts("error: can't get controller info!"); vbe_Done(); exit(1);}
    }
    
    if ((mode.vga == 0) && (mode.xres != 0) && (mode.yres != 0)) {
        mode.mode = vbe_FindMode(mode.xres, mode.yres, mode.bpp, (mode.lfb == 0 ? 0 : vbe_ATTR_LFB_Support));
        if (mode.mode == -1) {printf("error: can't find %ix%ix%ibpp %sVESA mode!\n", mode.xres, mode.yres, mode.bpp, (mode.lfb == 0 ? "" : "LFB ")); vbe_Done(); exit(1);}
    }
    
    // да, найдем инфу по режиму ДЖВА раза! :)
    if (mode.mode >= 0x100) {
        vbe_ModeInfo(mode.mode, &modeinfo);
        if (vbe_status != 0x4F) {printf("error: can't get info for mode 0x%X!\n", mode.mode); vbe_Done(); exit(1);}
        if ((modeinfo.ModeAttributes & vbe_ATTR_HardwareMode) != vbe_ATTR_HardwareMode) {printf("error: mode 0x%X exists, but not supported by hardware!\n", mode.mode); vbe_Done(); exit(1);}
        mode.xres = modeinfo.XResolution; mode.yres = modeinfo.YResolution; mode.bpp = modeinfo.BitsPerPixel;
    }
    
    if ((mode.vga == 0) && (mode.lfb == 1)) {
        if (modeinfo.PhysBasePtr == 0) {printf("error: can't find LFB pointer for %ix%ix%ibpp VESA mode!\n", mode.xres, mode.yres, mode.bpp); vbe_Done(); exit(1);}
    }
    
    if (mode.vga == 1) {
        switch (mode.mode) {
            // cga modes are disabled because of bugs
            //case 0x5 : mode.xres = 320; mode.yres = 200; mode.bpp = 2; scrptr = 0xB8000; break;
            //case 0x6 : mode.xres = 640; mode.yres = 200; mode.bpp = 2; scrptr = 0xB8000; break;
        
            case 0xD : mode.xres = 320; mode.yres = 200; mode.bpp = 4; break;
            case 0xE : mode.xres = 640; mode.yres = 200; mode.bpp = 4; break; 
            case 0xF : mode.xres = 640; mode.yres = 350; mode.bpp = 2; break;
            case 0x10: mode.xres = 640; mode.yres = 350; mode.bpp = 4; break;
            case 0x11: mode.xres = 640; mode.yres = 480; mode.bpp = 1; break;
            case 0x12: mode.xres = 640; mode.yres = 480; mode.bpp = 4; break;
            case 0x13: mode.xres = 320; mode.yres = 200; mode.bpp = 8; break;
            default  : puts("error: unsupported mode!"); exit(1);
        }
        mode.lfb = 0;
    };
    
    mode.fbsize = (mode.xres * mode.yres) * ((mode.bpp + 7) >> 3);
    
    if (quiet == 0) {
        printf("testing %s mode%s 0x%X (%ix%ix%ibpp)%s, press any key... \n", 
              (mode.vga == 0 ? "VESA" : "VGA"), (mode.unchain == 1 ? "-x" : ""), mode.mode, mode.xres, mode.yres, mode.bpp, (mode.lfb == 0 ? "" : " LFB"));
        while (!kbhit()) {} getch();
    }
    
    rtc_initTimer(clockdivisor);
    rtc_setTimer(&timerproc, 1);
    
    if (mode.vga == 1) {
        setbiosmode(mode.mode);
        if (mode.unchain) setmodex();
    } else {
        vbe_SetMode(mode.mode | (mode.lfb == 0 ? 0 : vbe_MODE_LINEAR));
        if (vbe_status != 0x4F) {printf("error: can't set mode 0x%X!", mode.mode); vbe_Done(); exit(1);}
        
        scrptr = vbe_GetVideoPtr();
        if (scrptr == NULL) {
            if (mode.lfb == 1) {setbiosmode(0x3); printf("error: can't map LFB for mode 0x%X! (NTVDM - sux)\n", mode.mode); vbe_Done(); exit(1);} 
            else {setbiosmode(0x3); printf("error: can't get banked window segment for mode 0x%X! (pretty weird...)\n", mode.mode); vbe_Done(); exit(1);} 
        }
    }
    
    test8();
    test16();
    test32();
    
    setbiosmode(0x3);
    rtc_freeTimer();
    if (!mode.vga) vbe_Done();
    
    if (info == 1) {
        printf("video adapter type: %s", (vesatest() == 0x4F ? "VESA, " : "VGA \n"));
        if (vesatest() == 0x4F) {
            if (mode.vga) {vbe_Init(); vbe_ControllerInfo(&vbeinfo); vbe_Done();}
            printf("version %i.%i \n", (unsigned char)(vbeinfo.vbeVersion >> 8), (unsigned char)(vbeinfo.vbeVersion));
            printf("VESA OEM string:    %s \n", vbeinfo.OemStringPtr);
            if (vbeinfo.vbeVersion >= 0x200) printf("VESA adapter name:  %s \n", vbeinfo.OemProductNamePtr);
        }
        puts("-----------------------------------------------------------------------");
    }
    
    printf("results for %s mode%s 0x%X (%ix%ix%ibpp)%s, %i KiB trasfer: \n",
          (mode.vga == 0 ? "VESA" : "VGA"), (mode.unchain == 1 ? "-x" : ""), mode.mode, mode.xres, mode.yres, mode.bpp, (mode.lfb == 0 ? "" : " LFB"), (burstnum << 6));
    printf("\n");
    printf("8-bit  write: %10.3f MB/s  - %7.3f MiB/s  - %7.2f fps\n", ((double)result.write8  / (1000*1000)), ((double)result.write8  / (1024*1024)), ((double)result.write8  / mode.fbsize));
    printf("8-bit   read: %10.3f MB/s  - %7.3f MiB/s  - %7.2f fps\n", ((double)result.read8   / (1000*1000)), ((double)result.read8   / (1024*1024)), ((double)result.read8   / mode.fbsize));
    printf("8-bit   move: %10.3f MB/s  - %7.3f MiB/s  - %7.2f fps\n", ((double)result.move8   / (1000*1000)), ((double)result.move8   / (1024*1024)), ((double)result.move8   / mode.fbsize));
    printf("\n");
    printf("16-bit write: %10.3f MB/s  - %7.3f MiB/s  - %7.2f fps\n", ((double)result.write16 / (1000*1000)), ((double)result.write16 / (1024*1024)), ((double)result.write16 / mode.fbsize));
    printf("16-bit  read: %10.3f MB/s  - %7.3f MiB/s  - %7.2f fps\n", ((double)result.read16  / (1000*1000)), ((double)result.read16  / (1024*1024)), ((double)result.read16  / mode.fbsize));
    printf("16-bit  move: %10.3f MB/s  - %7.3f MiB/s  - %7.2f fps\n", ((double)result.move16  / (1000*1000)), ((double)result.move16  / (1024*1024)), ((double)result.move16  / mode.fbsize));
    printf("\n");
    printf("32-bit write: %10.3f MB/s  - %7.3f MiB/s  - %7.2f fps\n", ((double)result.write32 / (1000*1000)), ((double)result.write32 / (1024*1024)), ((double)result.write32 / mode.fbsize));
    printf("32-bit  read: %10.3f MB/s  - %7.3f MiB/s  - %7.2f fps\n", ((double)result.read32  / (1000*1000)), ((double)result.read32  / (1024*1024)), ((double)result.read32  / mode.fbsize));
    printf("32-bit  move: %10.3f MB/s  - %7.3f MiB/s  - %7.2f fps\n", ((double)result.move32  / (1000*1000)), ((double)result.move32  / (1024*1024)), ((double)result.move32  / mode.fbsize));
    printf("\n");
    
    return 0;
}
