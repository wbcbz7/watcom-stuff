#ifndef FLEXPTC_H
#define FLEXPTC_H

#include "vbe.h"
// flexptc - small DOS VGA\VESA 2.0 framebuffer library
// by wbc\\bz7 zq.o3.0x7e0

// error codes
enum {
    // normal termination
    ptc_errOK = 0
};

// global flags
// verbose error handling
//#define __PTC_VERBOSE

// palette type
typedef union {
    struct { unsigned char b, g, r, a;};
    unsigned int argb;
} ptc_palette;

// ------- ptc_context internal structure -----------

// ptc_context mode structure
typedef struct {
    int mode;           // mode number
    int width;          // mode width
    int height;         // mode height
    int pagesize;       // mode page size
    int host_start;     // host start address (relative to ptr!)
    int host_type;      // host buffer type (see defines)
    int host_pitch;     // host scanline length in bytes
    int host_pagesize;  // host framebuffer page size
    int host_tricks;    // host tricks bitfield
    int host_bpp;       // host bits per pixel
    int host_rate;      // host refresh rate
    int host_nonvga;    // host non-VGA mode flag
    int host_dacwidth;  // host RAMDAC width
    int mode_bpp;       // mode bits per pixel
    // framebuffer pointers.
    // bankedptr used only for VESA banked\Mode-X host modes
    unsigned long ptr, bankedptr;
    // VESA adapter\mode info blocks
    vbe_VbeInfoBlock  vbeinfo;
    vbe_ModeInfoBlock modeinfo;
} ptc_context;

// ptc_context.host_type values
enum {
    // linear
    ptc_HOSTTYPE_LINEAR,
    // banked (needs to use VESA calls to bank switching)
    ptc_HOSTTYPE_BANKED,
    // planar aka Mode-X (needs to use port I\O to switch planes)
    ptc_HOSTTYPE_PLANAR
};

// ptc_context.host_tricks bitfields
// force 60hz via VGA CRTC (only for 320x200!)
#define ptc_TRICKS_60HZ         1
// horizontal pixel double (maaatrox :)
#define ptc_TRICKS_HDOUBLE      2
// vertical pixel double
#define ptc_TRICKS_VDOUBLE      4
// bpp conversion needed
#define ptc_TRICKS_BPPCONVERT   8


// ------- ptc_open() ------------------------------

// ptc_open() flags
// display mode select dialog
#define ptc_OPEN_MANUAL         1     // ignored, always manual
// do not search for lower directcolor modes, i.e if 32bpp mode is requested
// then do not search for 15\16\24bpp fallback modes
#define ptc_OPEN_NOFALLBACK     2
// use hardware double buffer
#define ptc_OPEN_HWDOUBLEBUFFER 4     // ignored
// force VESA 1.2 banked mode
#define ptc_OPEN_FORCEBANKED    8
// set new mode immediately
#define ptc_OPEN_SETMODE       16
// do not search for software doubled modes
#define ptc_OPEN_NODOUBLE      32
// prefer 320x200 60hz forced modes
#define ptc_OPEN_FIND60HZ      64

// ptc_open() error codes
enum {
    // ptc_open(): hardware fault\incompatibility
    ptc_errOpenHWFail          = 1,
    // ptc_open(): can't init VBE interface
    ptc_errOpenVBEInitFail,
    // ptc_open(): incorrect VBE version
    ptc_errOpenIncorrectVBEVersion,
    // ptc_open(): incorrect console format requested
    ptc_errOpenIncorrectFormat,
    // ptc_open(): can't find mode for this console format
    ptc_errOpenModeNotFound,
    // ptc_open(): can't allocate memory for temp linear screen buffer
    ptc_errOpenBankedPizdec,
    // ptc_open(): exit by user
    ptc_errOpenUserExit
};

// ------- ptc_setpal()  ---------------------------

// ptc_setpal() error codes
enum {
    // ptc_setpal(): inacceptable in direct color modes
    ptc_errSetpalDirectColorErr = 1,
};

// ------- ptc_setcontext()  -----------------------

// ptc_setcontext() error codes
enum {
    // ptc_setcontext(): can't set requested mode
    ptc_errSetcontextModeError = 1,
    // ptc_setcontext(): error: can't get pointer to frame buffer
    ptc_errSetcontextPtrError
};

// procedures
int ptc_init();
int ptc_setpal(ptc_palette*);
int ptc_close();
int ptc_setcontext(ptc_context*);
int ptc_open(char*, int, int, int, int, ptc_context*);
void ptc_wait();
int ptc_update(void* buffer);


#endif
