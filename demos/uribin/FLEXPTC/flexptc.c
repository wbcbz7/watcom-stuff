#include <conio.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "vbe.c"
#include "flexptc.h"

enum {
   ptc_d_open,
   ptc_d_close,
   ptc_d_setpal
};

static char* ptc_strings[] = {
    "ptc_open()   -> ",
    "ptc_close()  -> ",
    "ptc_setpal() -> "
};

void ptc_error(int func, char* err) {
    printf(ptc_strings[func]); printf(err);
}

// internal static structure - DO NOT MODIFY! (reading is allowed ;)
static struct {
    int mode, pitch, width, height, start, pagesize;
    int host_bpp, mode_bpp;
    int nonvga, dacwidth;
    unsigned long ptr;
    vbe_VbeInfoBlock  vbeinfo;
    vbe_ModeInfoBlock modeinfo;
} ptc_int;

int ptc_setpal(ptc_palette* pal) {
    int i;
    static char* err[] ={
        "ok\n",
        "error: unacceptable in direct color modes\n",
    };
    
    if (ptc_int.host_bpp > 8) {
#ifdef __PTC_VERBOSE
        ptc_error(ptc_d_close, err[ptc_errSetpalDirectColorErr]);
#endif
        return ptc_errSetpalDirectColorErr;
    }
    
    if ((ptc_int.nonvga) || (ptc_int.dacwidth != 6)) {
        // use vesa call
        vbe_SetPalette(pal, 0, 256, 0);
    } else {
        // use VGA RAMDAC ports
        outp(0x3C8, 0);
        for (i = 0; i < 256; i++) {
            outp(0x3C9, pal[i].r);
            outp(0x3C9, pal[i].g);
            outp(0x3C9, pal[i].b);
        }
    }
    
#ifdef __PTC_VERBOSE
    ptc_error(ptc_d_close, err[ptc_errOK]);
#endif
    return ptc_errOK;
}

// only vbe 2.0 lfb supported (no vga\fakemode support)
// caption is ignored
int ptc_open(char* caption, int width, int height, int bpp, int flags) {
    static char* err[] ={
        "ok\n",
        "error: can't init VBE interface\n",
        "error: incorrect VBE version\n",
        "error: hardware fault or incompatibility\n",
        "error: incorrect console format requested\n",
        "error: can't find mode for requested console format\n",
        "terminated by user\n"
    };
    
    // internal mode list structure
    static struct {
        int mode;
        vbe_ModeInfoBlock info;
    } mode[30];
    vbe_ModeInfoBlock     modeinfo;
    
    // palette array - just for phun ;)
    ptc_palette pal[256];
    
    char ch;
    
    int i, j, k, psel, p, rawmode, realbpp, rawflags;
    int cond = 0;
    
    // check if requested mode is correct
    if (!((bpp == 8) || (bpp == 32))) {
        ptc_error(ptc_d_open, err[ptc_errOpenIncorrectFormat]);
        return ptc_errOpenIncorrectFormat;
    }
    
    // check for vbe interface presence and get vesa info
    if (vbe_Init()) {
        ptc_error(ptc_d_open, err[ptc_errOpenVBEInitFail]);
        return ptc_errOpenVBEInitFail;
    }
    
    vbe_ControllerInfo(&ptc_int.vbeinfo);
    
    if (ptc_int.vbeinfo.vbeVersion < 0x102) {
        vbe_Done();
        ptc_error(ptc_d_open, err[ptc_errOpenIncorrectVBEVersion]);
        return ptc_errOpenIncorrectVBEVersion;
    }
    
    ptc_int.nonvga = (ptc_int.vbeinfo.Capabilities & vbe_CAP_Not_VGA_Compatible ? 1 : 0);
    
#ifdef __PTC_VERBOSE
    // print all necessary info onto console
    puts  (" ------------------------------------------ ");
    printf(" VESA BIOS Extensions version: %d.%d\n",  (ptc_int.vbeinfo.vbeVersion >> 8), (ptc_int.vbeinfo.vbeVersion & 0xFF));
    printf(" VESA OEM string:              %s\n",     ptc_int.vbeinfo.OemStringPtr);
    printf(" Total memory avaiable:        %d KiB\n", (ptc_int.vbeinfo.TotalMemory << 6));
    puts  (" ------------------------------------------ ");
#endif
    
    printf(" requested console mode: %dx%dx%dbpp\n", width, height, bpp);
    
    // scan through all available modes to find desired mode
    // ripped code from vbe.c :))))
    
    // additional check for mode is supported by hardware
    rawflags = vbe_ATTR_HardwareMode;
    
    p = 0;
    for (i = 0; ((ptc_int.vbeinfo.VideoModePtr[i] != 0xFFFF) && (ptc_int.vbeinfo.VideoModePtr[i] != 0)); i++) {
        vbe_ModeInfo(ptc_int.vbeinfo.VideoModePtr[i], &modeinfo);
        
        // check for fake or buggy 15\16bpp modes
        // also fixes one of matrox vbe bugs - 15bpp modes have an wrong 16bpp value in modeinfo.BitsPerPixel
        if ((modeinfo.BitsPerPixel == 15) || (modeinfo.BitsPerPixel == 16)) {
            realbpp = modeinfo.RedMaskSize + modeinfo.GreenMaskSize + modeinfo.BlueMaskSize;
        } else
        
        // check for fake or buggy 24\32bpp modes
        // afaik 3dfx cards incorrectly(?) sets 24bpp in modeinfo.BitsPerPixel but in fact uses 32bpp (need to confirm)
        // so we need to calculate real bpp
        if ((modeinfo.BitsPerPixel == 24) || (modeinfo.BitsPerPixel == 32)) {
            realbpp = modeinfo.RedMaskSize + modeinfo.GreenMaskSize + modeinfo.BlueMaskSize + modeinfo.RsvdMaskSize;
        } else
        realbpp = modeinfo.BitsPerPixel;
        
        modeinfo.BitsPerPixel = realbpp;
        
        if ((realbpp >= 8) && (bpp > 8) && !(flags & ptc_OPEN_NOFALLBACK)) {
            cond = ((modeinfo.XResolution == width) && (modeinfo.YResolution >= height) && ((modeinfo.ModeAttributes & rawflags) == rawflags));
        } else {
            cond = ((modeinfo.XResolution == width) && (modeinfo.YResolution >= height) && (realbpp == bpp) && ((modeinfo.ModeAttributes & rawflags) == rawflags)); 
        }
        
        if (cond) {
            mode[p].mode = ptc_int.vbeinfo.VideoModePtr[i];
            memcpy(&mode[p].info, &modeinfo, sizeof(vbe_ModeInfoBlock));
            p++;
        }
    }
    // add VGA mode 0x13 :)
    if ((width == 320) && (height <= 200)) {
        mode[p].mode = 0x13; mode[p].info.XResolution = 320; mode[p].info.YResolution = 200;
        mode[p].info.BitsPerPixel = 8; mode[p].info.BytesPerScanline = 320;
        mode[p].info.ModeAttributes |= vbe_ATTR_LFB_Support; // mode 0x13 IS linear :)
        p++;
    }
    
    if (p == 0) {
        vbe_Done();
        ptc_error(ptc_d_open, err[ptc_errOpenModeNotFound]);
        return ptc_errOpenModeNotFound;
    }
    
    // display mode selector
    printf(" select available mode: \n");
    for (i = 0; i < p; i++) {
        printf(" %c - %dx%dx%dbpp %s%s%s - mode 0x%X %s\n",
               (i >= 10 ? (i - 10 + 'A') : (i + '0')),
               mode[i].info.XResolution, mode[i].info.YResolution, mode[i].info.BitsPerPixel,
               (mode[i].info.BitsPerPixel < 10 ? " " : ""), (mode[i].mode < 0x100 ? "VGA  " : "VESA "),
               ((mode[i].info.ModeAttributes & vbe_ATTR_LFB_Support) && !(flags & ptc_OPEN_FORCEBANKED) ? "linear" : "banked"), mode[i].mode,
               ((bpp > 8) && (mode[i].info.BitsPerPixel == 8)) ? " (grayscale)" : "");
    }
    puts  (" ------------------------------------------ ");
    printf(" press [0 - %c] to select mode, [ESC] to exit... \n", (p >= 11 ? (p - 11 + 'A') : (p - 1 + '0')));
    
    // messy keyscan routine
    do {
        ch = getch();
        if (ch == 0x1B) {
            vbe_Done();
            ptc_error(ptc_d_open, err[ptc_errOpenUserExit]);
            return ptc_errOpenUserExit;
        }
        if (ch < '0') continue;
        if ((ch <= 'z') && (ch >= 'a')) ch -= 32;
        psel = (ch >= 'A' ? (ch + 10 - 'A') : (ch - '0')); 
    } while (psel >= p);
    
    // save mode info block and all info
    memcpy(&ptc_int.modeinfo, &mode[psel].info, sizeof(vbe_ModeInfoBlock));
    
    ptc_int.mode     = mode[psel].mode | ((ptc_int.modeinfo.ModeAttributes & vbe_ATTR_LFB_Support) && !(flags & ptc_OPEN_FORCEBANKED) && (mode[psel].mode >= 0x100) ? vbe_MODE_LINEAR : 0);
    ptc_int.pitch    = ptc_int.modeinfo.BytesPerScanline;
    ptc_int.width    = width;
    ptc_int.height   = height;
    ptc_int.start    = ptc_int.pitch * ((ptc_int.modeinfo.YResolution - ptc_int.height) >> 1);
    ptc_int.pagesize = ptc_int.pitch * ptc_int.modeinfo.YResolution;
    ptc_int.host_bpp = ptc_int.modeinfo.BitsPerPixel;
    ptc_int.mode_bpp = bpp;
    ptc_int.nonvga  |= (ptc_int.modeinfo.ModeAttributes & vbe_ATTR_No_VGA_Mode ? 1 : 0);
    
    vbe_SetMode(ptc_int.mode);
    ptc_int.ptr = (unsigned long)vbe_GetVideoPtr();
    
    if ((ptc_int.host_bpp == 8) && (ptc_int.mode_bpp > 8)) {
        // fill our palette
        for (i = 0; i < 256; i++) {
            pal[i].r = pal[i].g = pal[i].b = (i >> 2);
        }
        ptc_setpal(&pal[0]);
    }
    
#ifdef __PTC_VERBOSE
    ptc_error(ptc_d_open, err[ptc_errOK]);
#endif
    return ptc_errOK;
}

int ptc_close() {
    static char* err[] ={
        "ok\n",
    };
    
    vbe_SetMode(0x3);
    vbe_Done();
    
#ifdef __PTC_VERBOSE
    ptc_error(ptc_d_close, err[ptc_errOK]);
#endif
    return ptc_errOK;
}

void ptc_wait() {
    int x, y, i;
    if (ptc_int.nonvga) {
        // here is a trick for nonvga modes
        i = vbe_GetDisplayStart();
        y = (i >> 16); x = (i & 0xFFFF);
        vbe_SetDisplayStart(x, y, vbe_WAITRETRACE);
        
    } else {
        // good old VGA port 0x3DA ;)
        while (inp(0x3DA) & 8); while (!(inp(0x3DA) & 8));
    }
}

#include "convert.c"

int ptc_update(void* buffer) {
    int width, height, pitch, buf_pitch, x, padding;
    unsigned long conbuf, buf;
    
    width     = ptc_int.width;
    height    = ptc_int.height;
    pitch     = ptc_int.pitch;
    buf_pitch = ptc_int.width * ((ptc_int.mode_bpp + 7) >> 3); 
    //padding   = pitch - buf_pitch;
    conbuf    = ptc_int.ptr + ptc_int.start;
    buf       = (unsigned long)buffer;
    
    // LINEAR MODES ONLY!11!
    
    if ((pitch - (ptc_int.width * ((ptc_int.host_bpp + 7) >> 3))) == 0) {
        height = 1; pitch = 0; buf_pitch = 0;
        buf_pitch = ptc_int.width * ((ptc_int.mode_bpp + 7) >> 3) * ptc_int.height;
    }
    if (ptc_int.host_bpp == ptc_int.mode_bpp) {
        while (height--) {
            ptc_fastmemcpy((void*)conbuf, (void*)buf, buf_pitch);
            //memcpy((void*)conbuf, (void*)buf, buf_pitch); // plain memcpy() - not ideal but anyway...
            conbuf += pitch; buf += buf_pitch;
        }
        return ptc_errOK;
    }
    if ((ptc_int.host_bpp == 24) && (ptc_int.mode_bpp == 32)) {
        while (height--) {
            ptc_convert_argb8888_rgb888((void*)conbuf, (void*)buf, (buf_pitch >> 2));
            conbuf += pitch; buf += buf_pitch;
        }
        return ptc_errOK;
    }
    if ((ptc_int.host_bpp == 16) && (ptc_int.mode_bpp == 32)) {
        while (height--) {
            ptc_convert_argb8888_rgb565((void*)conbuf, (void*)buf, (buf_pitch >> 2));
            conbuf += pitch; buf += buf_pitch;
        }
        return ptc_errOK;
    }
    if ((ptc_int.host_bpp == 15) && (ptc_int.mode_bpp == 32)) {
        while (height--) {
            ptc_convert_argb8888_rgb555((void*)conbuf, (void*)buf, (buf_pitch >> 2));
            conbuf += pitch; buf += buf_pitch;
        }
        return ptc_errOK;
    }
    if ((ptc_int.host_bpp == 8) && (ptc_int.mode_bpp == 32)) {
        while (height--) {
            ptc_convert_argb8888_gray8((void*)conbuf, (void*)buf, (buf_pitch >> 2));
            conbuf += pitch; buf += buf_pitch;
        }
        return ptc_errOK;
    }
    return ptc_errOK;
}
