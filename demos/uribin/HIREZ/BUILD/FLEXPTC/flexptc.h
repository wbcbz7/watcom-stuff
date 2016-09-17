#ifndef FLEXPTC_H
#define FLEXPTC_H

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

// ------- ptc_open() ------------------------------

// ptc_open() flags
// display mode select dialog
#define ptc_OPEN_MANUAL         1     // ignored
// do not search for lower directcolor modes, i.e if 32bpp mode is requested
// then do not search for 15\16\24bpp fallback modes
#define ptc_OPEN_NOFALLBACK     2
// use hardware double buffer
#define ptc_OPEN_HWDOUBLEBUFFER 4     // ignored
// force VESA 1.2 banked mode
#define ptc_OPEN_FORCEBANKED    8     // ignored
// do not set new modd
#define ptc_OPEN_FILLCONTEXT   16

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

#endif
