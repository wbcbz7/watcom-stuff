#include <conio.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "vbe.c"
#include "flexptc.h"

enum {
   ptc_d_open,
   ptc_d_close,
   ptc_d_setpal,
   ptc_d_setcontext
};

static char* ptc_strings[] = {
    "ptc_open()   -> ",
    "ptc_close()  -> ",
    "ptc_setpal() -> ",
    "ptc_setcontext() -> "
};

void ptc_error(int func, char* err) {
    printf(ptc_strings[func]); printf(err);
}

// internal static structure - DO NOT MODIFY! (reading is allowed ;)
static ptc_context ptc_int;
static int         vbe_avail = 0;

int ptc_init() {
    // check for vbe interface presence and get vesa info
    if (!vbe_Init()) {
        //ptc_error(ptc_d_open, err[ptc_errOpenVBEInitFail]);
        //return ptc_errOpenVBEInitFail;
        vbe_avail = 1;
    }
    return ptc_errOK;
}

//-------------------------------------------------------------------
int ptc_setpal(ptc_palette* pal) {
    static char* err[] ={
        "ok\n",
        "error: unacceptable in direct color modes\n",
    };
    
    if (ptc_int.host_bpp > 8) {
#ifdef __PTC_VERBOSE
        ptc_error(ptc_d_setpal, err[ptc_errSetpalDirectColorErr]);
#endif
        return ptc_errSetpalDirectColorErr;
    }
    
    if ((ptc_int.host_nonvga) || (ptc_int.host_dacwidth != 6)) {
        // use vesa call
        vbe_SetPalette(pal, 0, 256, 0);
    } else {
        // use VGA RAMDAC ports
        outp(0x3C8, 0);
        for (int i = 0; i < 256; i++) {
            outp(0x3C9, pal[i].r);
            outp(0x3C9, pal[i].g);
            outp(0x3C9, pal[i].b);
        }
    }
    
#ifdef __PTC_VERBOSE
    ptc_error(ptc_d_setpal, err[ptc_errOK]);
#endif
    return ptc_errOK;
}

unsigned char getvgamode();
#pragma aux getvgamode = "mov ah, 0xF" "int 0x10" value [al] modify [ah];

//-------------------------------------------------------------------
int ptc_close() {
    static char* err[] ={
        "ok\n",
    };
    
    if (ptc_int.bankedptr) free((void*)ptc_int.bankedptr);
    if (getvgamode() != 0x3) vbe_SetMode(0x3);
    if (vbe_avail) vbe_Done();
    
#ifdef __PTC_VERBOSE
    ptc_error(ptc_d_close, err[ptc_errOK]);
#endif
    return ptc_errOK;
}

//-------------------------------------------------------------------
// copy proper ptc_context struct before calling
int ptc_setcontext(ptc_context *context = &ptc_int) {
    static char* err[] ={
        "ok\n",
        "error: can't set requested mode\n",
        "error: can't get pointer to frame buffer\n",
    };

    int i;
    // palette array - just for phun ;)
    ptc_palette pal[256];

    // clear old mode info
    if (ptc_int.bankedptr) free((void*)ptc_int.bankedptr);
    
    memcpy(&ptc_int, context, sizeof(ptc_context));
    
    vbe_SetMode(ptc_int.mode);/*
    if (vbe_status != 0x4F) {
        ptc_close();
        ptc_error(ptc_d_setcontext, err[ptc_errSetcontextModeError]);
        return ptc_errSetcontextModeError;
    }*/
    // commented because univbe often returns error code while mode is set properly
    
    if (ptc_int.host_tricks & ptc_TRICKS_HDOUBLE) {
        outp(0x3D4, 0x9); outp(0x3D5, (inp(0x3D5) | 1));
    }
    if (ptc_int.host_tricks & ptc_TRICKS_60HZ) {
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
    
    ptc_int.ptr = (unsigned long)vbe_GetVideoPtr();
    if (!ptc_int.ptr) {
        ptc_close();
        ptc_error(ptc_d_setcontext, err[ptc_errSetcontextPtrError]);
        return ptc_errSetcontextPtrError;
    }
    
    if ((ptc_int.host_bpp == 8) && (ptc_int.mode_bpp > 8)) {
        // fill our palette
        for (i = 0; i < 256; i++) {
            pal[i].r = pal[i].g = pal[i].b = (i >> 2);
        }
        ptc_setpal(&pal[0]);
    }
    return ptc_errOK;
}

//-------------------------------------------------------------------
// only vbe 1.2+ banked\linear and vga mode 0x13 supported (no modex\fakemode support)
// caption is ignored
int ptc_open(char* caption, int width, int height, int bpp, int flags, ptc_context *context = &ptc_int) {
    static char* err[] ={
        "ok\n",
        "error: can't init VBE interface\n",
        "error: incorrect VBE version\n",
        "error: hardware fault or incompatibility\n",
        "error: incorrect console format requested\n",
        "error: can't find mode for requested console format\n",
        "error: can't allocate memory for temp linear screen buffer\n",
        "terminated by user\n"
    };
    static char wait[4] = {'|', '/', '-', '\\'};
    
#define ptc_OPEN_MAXMODES   30
    
    // internal mode list structure
    typedef struct {
        int mode;
        int mode_trick;
        vbe_ModeInfoBlock info;
    } modetab;
    static modetab        mode[ptc_OPEN_MAXMODES];
    vbe_ModeInfoBlock     modeinfo;
    
    // sort class
    // yep, I will use bubble sort instead of qsort because it's stable 
    class sort {
        public:
            int bsort(void* base, int num) {
                modetab tmpbuf;
                modetab *p = (modetab*)base;
                
                // sort by bpp
                for (int i = 0; i < (num - 1); i++) {
                    int swapped = 0;
                    for (int j = 0; j < (num - i - 1); j++) {
                        if ((p[j+1].info.BitsPerPixel > p[j].info.BitsPerPixel) && !(p[j+1].mode_trick & ptc_TRICKS_BPPCONVERT)) {
                            memcpy(&tmpbuf, &p[j], sizeof(modetab));
                            memcpy(&p[j], &p[j+1], sizeof(modetab));
                            memcpy(&p[j+1], &tmpbuf, sizeof(modetab));
                            swapped = 1;
                        }
                    }
                    if (!swapped) break;
                }
                
                // sort by 60hz tweak if needed
                for (int i = 0; i < (num - 1); i++) {
                    int swapped = 0;
                    for (int j = 0; j < (num - i - 1); j++) {
                        if ((p[j+1].mode_trick & ptc_TRICKS_60HZ) && !(p[j].mode_trick & ptc_TRICKS_60HZ)) {
                            memcpy(&tmpbuf, &p[j], sizeof(modetab));
                            memcpy(&p[j], &p[j+1], sizeof(modetab));
                            memcpy(&p[j+1], &tmpbuf, sizeof(modetab));
                            swapped = 1;
                        }
                    }
                    if (!swapped) break;
                }
                
                // sort by yres
                for (int i = 0; i < (num - 1); i++) {
                    int swapped = 0;
                    for (int j = 0; j < (num - i - 1); j++) {
                        if (p[j].info.YResolution > p[j+1].info.YResolution) {
                            memcpy(&tmpbuf, &p[j], sizeof(modetab));
                            memcpy(&p[j], &p[j+1], sizeof(modetab));
                            memcpy(&p[j+1], &tmpbuf, sizeof(modetab));
                            swapped = 1;
                        }
                    }
                    if (!swapped) break;
                }
                
                return 0;
            }
    } sort;
    
    char ch;
    
    int i, j, k, psel, p, rawmode, realbpp, rawflags, trickflags;
    int cond = 0, doub_cond = 0;
    
    memset(&mode, 0, sizeof(mode));
    
    // check if requested mode is correct
    if (!((bpp == 8) || (bpp == 15) || (bpp == 16))) {
        ptc_error(ptc_d_open, err[ptc_errOpenIncorrectFormat]);
        return ptc_errOpenIncorrectFormat;
    }
    
    context->host_dacwidth = 6;
    
    if (vbe_avail) {
        vbe_ControllerInfo(&context->vbeinfo);
        context->host_nonvga = (context->vbeinfo.Capabilities & vbe_CAP_Not_VGA_Compatible ? 1 : 0);
    }
    
#ifdef __PTC_VERBOSE
    // print all necessary info onto console
    puts  (" ------------------------------------------ ");
    printf(" VESA BIOS Extensions version: %d.%d\n",  (context->vbeinfo.vbeVersion >> 8), (context->vbeinfo.vbeVersion & 0xFF));
    printf(" VESA OEM string:              %s\n",     context->vbeinfo.OemStringPtr);
    printf(" Total memory avaiable:        %d KiB\n", (context->vbeinfo.TotalMemory << 6));
    puts  (" ------------------------------------------ ");
#endif
    
    printf(" requested console mode: %dx%dx%dbpp\n", width, height, bpp);
    printf(" scanning for modes: "); fflush(stdout);
    p = 0;
    
    if (vbe_avail) {
    
        // scan through all available modes to find desired mode
        // ripped code from vbe.c :))))
    
        // additional check for mode is supported by hardware
        rawflags = vbe_ATTR_HardwareMode;
    
        for (i = 0; ((context->vbeinfo.VideoModePtr[i] != 0xFFFF) && (context->vbeinfo.VideoModePtr[i] != 0)); i++) {
            // rotate wait symbol
            printf("%c\b", wait[(i & 3)]); fflush(stdout);
            
            vbe_ModeInfo(context->vbeinfo.VideoModePtr[i], &modeinfo);
        
            // check for fake or buggy 15\16bpp modes
            // also fixes one of matrox vbe bugs - 15bpp modes have an wrong 16bpp value in modeinfo.BitsPerPixel
            // ..aaand yes, FUCK YOU VBE15BPP!!!
            if ((modeinfo.BitsPerPixel == 15) || (modeinfo.BitsPerPixel == 16)) {
                realbpp = modeinfo.RedMaskSize + modeinfo.GreenMaskSize + modeinfo.BlueMaskSize;
            } else realbpp = modeinfo.BitsPerPixel;
        
            modeinfo.BitsPerPixel = realbpp;
            
            doub_cond = ((width <= 320) &&
                         ((modeinfo.XResolution >> 1) == width) &&
                         !(modeinfo.ModeAttributes & vbe_ATTR_No_VGA_Mode) &&
                         !(flags & ptc_OPEN_NODOUBLE));
            
            if ((realbpp > 8) && (bpp > 8) && !(flags & ptc_OPEN_NOFALLBACK)) {
                if (doub_cond)
                    cond = (((modeinfo.YResolution >> 1) >= height) &&
                             (realbpp <= 16) &&
                             ((modeinfo.ModeAttributes & rawflags) == rawflags));
                else
                    cond = ((modeinfo.XResolution == width) &&
                            (modeinfo.YResolution >= height) &&
                            (modeinfo.YResolution < (height << 1)) &&
                            (realbpp <= 16) &&
                            ((modeinfo.ModeAttributes & rawflags) == rawflags));
            } else {
                cond = ((modeinfo.XResolution == width) &&
                        (modeinfo.YResolution >= height) &&
                        (modeinfo.YResolution < (height << 1)) &&
                        (realbpp == bpp) &&
                        ((modeinfo.ModeAttributes & rawflags) == rawflags)); 
            }
        
            if (cond) {
                if (doub_cond) mode[p].mode_trick |= ptc_TRICKS_HDOUBLE;
                if (realbpp != bpp) mode[p].mode_trick |= ptc_TRICKS_BPPCONVERT;
                mode[p].mode = context->vbeinfo.VideoModePtr[i];
                memcpy(&mode[p].info, &modeinfo, sizeof(vbe_ModeInfoBlock));
                if ((modeinfo.XResolution == 320) && (modeinfo.YResolution == 200) && (flags & ptc_OPEN_FIND60HZ)) {
                    p++;
                    mode[p].mode_trick |= ptc_TRICKS_60HZ;
                    mode[p].mode = context->vbeinfo.VideoModePtr[i];
                    memcpy(&mode[p].info, &modeinfo, sizeof(vbe_ModeInfoBlock));
                }
                p++;
                printf("."); fflush(stdout);
            }
        }
    }
    
    // add VGA mode 0x13 :)
    if ((width == 320) && (height <= 200) && (bpp == 8)) {
        mode[p].mode = 0x13; mode[p].info.XResolution = 320; mode[p].info.YResolution = 200;
        mode[p].info.BitsPerPixel = 8; mode[p].info.BytesPerScanline = 320;
        mode[p].info.ModeAttributes |= vbe_ATTR_LFB_Support; // mode 0x13 IS linear :)
        printf("."); fflush(stdout);
        p++;
        if (flags & ptc_OPEN_FIND60HZ) {
            mode[p].mode = 0x13; mode[p].info.XResolution = 320; mode[p].info.YResolution = 200;
            mode[p].info.BitsPerPixel = 8; mode[p].info.BytesPerScanline = 320;
            mode[p].info.ModeAttributes |= vbe_ATTR_LFB_Support;
            mode[p].mode_trick |= ptc_TRICKS_60HZ;
            printf("."); fflush(stdout);
            p++;
        }
    }
    // no support for mode-x, sorry
    
    printf("done\n");
    if (p == 0) {
        ptc_error(ptc_d_open, err[ptc_errOpenModeNotFound]);
        return ptc_errOpenModeNotFound;
    }
    
    // sort our mode array
    sort.bsort(&mode, p);
    
    // display mode selector
    printf(" select available mode: \n");
    for (i = 0; i < p; i++) {
        printf(" %c - %dx%dx%dbpp %s%s%s - mode 0x%04X%s%s%s%s\n",
               (i >= 10 ? (i - 10 + 'A') : (i + '0')),
               mode[i].info.XResolution, mode[i].info.YResolution, mode[i].info.BitsPerPixel,
               (mode[i].info.BitsPerPixel < 10 ? " " : ""), (mode[i].mode < 0x100 ? "VGA  " : "VESA "),
               ((mode[i].info.ModeAttributes & vbe_ATTR_LFB_Support) && !(flags & ptc_OPEN_FORCEBANKED) ? "linear" : "banked"), mode[i].mode,
               ((mode[i].mode_trick & ptc_TRICKS_60HZ) ? " 60hz tweak" : ""),
               (((bpp > 8) && (mode[i].info.BitsPerPixel == 8)) ? " grayscale" : ""),
               ((mode[i].mode_trick & ptc_TRICKS_HDOUBLE) ? " horizontal doubled" : ""),
               (((mode[i].info.XResolution == width) && (mode[i].info.YResolution == height) &&
                (mode[i].info.BitsPerPixel == bpp) &&
                (flags & ptc_OPEN_FIND60HZ ? mode[i].mode_trick & ptc_TRICKS_60HZ : 1)) ? " (recommended)" : ""));
        if ((mode[i].info.YResolution != mode[i+1].info.YResolution) && (i < (p-1))) printf(" -------------- \n");
    }
    printf(" ------------------------------------------ \n");
    printf(" press [0 - %c] to select mode, [ESC] to exit... \n", (p >= 11 ? (p - 11 + 'A') : (p - 1 + '0')));
    
    // messy keyscan routine
    do {
        ch = getch();
        if (ch == 0x1B) {
            ptc_error(ptc_d_open, err[ptc_errOpenUserExit]);
            return ptc_errOpenUserExit;
        }
        if (ch < '0') continue;
        if ((ch <= 'z') && (ch >= 'a')) ch -= 32;
        psel = (ch >= 'A' ? (ch + 10 - 'A') : (ch - '0')); 
    } while (psel >= p);
    
    // save mode info block and all info
    memcpy(&context->modeinfo, &mode[psel].info, sizeof(vbe_ModeInfoBlock));
    context->host_tricks     = mode[psel].mode_trick;
    context->modeinfo.YResolution = ((context->host_tricks & ptc_TRICKS_HDOUBLE) ? (context->modeinfo.YResolution >> 1) : context->modeinfo.YResolution);
    context->mode            = mode[psel].mode | ((context->modeinfo.ModeAttributes & vbe_ATTR_LFB_Support) && !(flags & ptc_OPEN_FORCEBANKED) && (mode[psel].mode >= 0x100) ? vbe_MODE_LINEAR : 0);
    context->host_pitch      = context->modeinfo.BytesPerScanline;
    context->width           = width;
    context->height          = height;
    context->host_start      = context->host_pitch * ((context->modeinfo.YResolution - context->height) >> 1);
    context->host_pagesize   = context->host_pitch * context->modeinfo.YResolution;
    context->pagesize        = context->host_pitch * height;
    context->host_bpp        = context->modeinfo.BitsPerPixel;
    context->mode_bpp        = bpp;
    context->host_nonvga    |= (context->modeinfo.ModeAttributes & vbe_ATTR_No_VGA_Mode ? 1 : 0);
    if (context->vbeinfo.vbeVersion < 0x200) {context->host_nonvga = 0; context->host_dacwidth = 6;}
    context->host_type       = (!(context->modeinfo.ModeAttributes & vbe_ATTR_LFB_Support) || ((flags & ptc_OPEN_FORCEBANKED) && (context->mode >= 0x100))? ptc_HOSTTYPE_BANKED : ptc_HOSTTYPE_LINEAR);
    
    // a workaround for banked modes - create temp linear screen buffer
    if (context->host_type == ptc_HOSTTYPE_BANKED) {
        context->bankedptr = (unsigned long)calloc(context->host_pagesize, 1);
        if (!context->bankedptr) {
            ptc_error(ptc_d_open, err[ptc_errOpenBankedPizdec]);
            return ptc_errOpenBankedPizdec;
        }
    } else context->bankedptr = 0;
    
    if (flags & ptc_OPEN_SETMODE) ptc_setcontext(context);
    
#ifdef __PTC_VERBOSE
    ptc_error(ptc_d_open, err[ptc_errOK]);
#endif
    return ptc_errOK;
}

//-------------------------------------------------------------------
void ptc_wait() {
    int x, y, i;
    if (ptc_int.host_nonvga) {
        // here is a trick for nonvga modes
        i = vbe_GetDisplayStart();
        y = (i >> 16); x = (i & 0xFFFF);
        vbe_SetDisplayStart(x, y, vbe_WAITRETRACE);
        
    } else {
        // good old VGA port 0x3DA ;)
        while (inp(0x3DA) & 8); while (!(inp(0x3DA) & 8));
    }
}

#include "convert.cpp"

//-------------------------------------------------------------------
int ptc_update(void* buffer) {
    int width, height, pitch, buf_pitch, x, padding;
    unsigned long bankedbuf, linbuf, conbuf, buf;
    
    width     = ptc_int.width;
    height    = ptc_int.height;
    pitch     = ptc_int.host_pitch;
    buf_pitch = ptc_int.width * ((ptc_int.mode_bpp + 7) >> 3); 
    //padding   = pitch - buf_pitch;
    bankedbuf = ptc_int.bankedptr + ptc_int.host_start;
    linbuf    = ptc_int.ptr + ptc_int.host_start;
    conbuf    = ((ptc_int.host_type == ptc_HOSTTYPE_LINEAR) ? linbuf : bankedbuf);
    buf       = (unsigned long)buffer;
    
    
    if ((pitch - (ptc_int.width * ((ptc_int.host_bpp + 7) >> 3))) == 0) {
        height = 1; pitch = 0;
        buf_pitch = ptc_int.width * ((ptc_int.mode_bpp + 7) >> 3) * ptc_int.height;
        
        if ((ptc_int.host_type == ptc_HOSTTYPE_BANKED) && (ptc_int.host_bpp == ptc_int.mode_bpp)) {
            ptc_lineartobanked((void*)ptc_int.ptr, (void*)buf, ptc_int.host_start, buf_pitch);
            return ptc_errOK;
        }
    }
    
    if ((ptc_int.host_tricks & ptc_TRICKS_HDOUBLE) && (ptc_int.host_bpp == ptc_int.mode_bpp)) {
        while (height--) {
            ptc_convert_rgb565_dup((void*)conbuf, (void*)buf, (buf_pitch >> 1));
            conbuf += pitch; buf += buf_pitch;
        }
    }
    
    if (!(ptc_int.host_tricks & ptc_TRICKS_HDOUBLE)) {
        if (ptc_int.host_bpp == ptc_int.mode_bpp) {
            while (height--) {
                ptc_fastmemcpy((void*)conbuf, (void*)buf, buf_pitch);
                conbuf += pitch; buf += buf_pitch;
            }
            return ptc_errOK;
        }
        if ((ptc_int.host_bpp == 15) && (ptc_int.mode_bpp == 16)) {
            while (height--) {
                ptc_convert_rgb565_rgb555((void*)conbuf, (void*)buf, (buf_pitch >> 1));
                conbuf += pitch; buf += buf_pitch;
            }
        }
        if ((ptc_int.host_bpp == 16) && (ptc_int.mode_bpp == 15)) {
            while (height--) {
                ptc_convert_rgb555_rgb565((void*)conbuf, (void*)buf, (buf_pitch >> 1));
                conbuf += pitch; buf += buf_pitch;
            }
        }
    }
    
    if (ptc_int.host_type == ptc_HOSTTYPE_BANKED) ptc_lineartobanked((void*)ptc_int.ptr, (void*)ptc_int.bankedptr, 0, ptc_int.host_pagesize);
    
    return ptc_errOK;
}
