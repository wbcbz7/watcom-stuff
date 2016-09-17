#ifndef WC_VBE20
#define WC_VBE20

#include <i86.h>
#include <string.h>
#include "dpmi.c"

#ifdef __cplusplus
extern "C" {
#endif

#include "vbe.h" 

/*
  Watcom C\C++ protected mode VESA 2.0 interface v.0.3
  by wbc\\bz7 18.o3.2o16
  
  based on VESAVBE.C by sumbissive \\ cubic team and $een
  
  vbe.c - main code ;)
*/

// VESA internal pointers and structures
static _dpmi_ptr          vbe_VbeInfoSeg      = {0,0};
static _dpmi_ptr          vbe_ModeInfoSeg     = {0,0};
static _dpmi_ptr          vbe_PalBufSeg       = {0,0};

static vbe_VbeInfoBlock  *vbe_VbeInfoPtr;
static vbe_ModeInfoBlock *vbe_ModeInfoPtr;
static unsigned long     *vbe_PalBuf;
static void              *vbe_LastPhysicalMap = NULL;

// internal REGS\SREGS structures
static union  REGS        vbe_regs;
static struct SREGS       vbe_sregs;
static _dpmi_rmregs       vbe_rmregs;

// VESA status variable - use it for checking da VBE status
static unsigned long      vbe_status;

// some variables for high-level VESA functions
static unsigned short     vbe_mode = 0;

// low-level VESA functions
// some text from VBE20.TXT

/*
     4.3. Function 00h - Return VBE Controller Information

     This required function returns the capabilities of the display
     controller, the revision level of the VBE implementation, and vendor
     specific information to assist in supporting all display controllers
     in the field.

     The purpose of this function is to provide information to the calling
     program about the general capabilities of the installed VBE software
     and hardware. This function fills an information block structure at
     the address specified by the caller. The VbeInfoBlock information
     block size is 256 bytes for VBE 1.x, and 512 bytes for VBE 2.0.

     Input:    AX   = 4F00h  Return VBE Controller Information
            ES:DI   =        Pointer to buffer in which to place
                             VbeInfoBlock structure
                             (VbeSignature should be set to 'VBE2' when
                             function is called to indicate VBE 2.0
                             information is desired and the information
                             block is 512 bytes in size.)

     Output:   AX   =         VBE Return Status  
*/
void vbe_ControllerInfo(vbe_VbeInfoBlock *p) {
    memset(&vbe_rmregs, 0, sizeof(vbe_rmregs));
    memset(vbe_VbeInfoPtr, 0, sizeof(vbe_VbeInfoBlock));
    // get vesa 2.0 info if possible
    memcpy(vbe_VbeInfoPtr->vbeSignature, VBE2SIGNATURE, 4);
    
    vbe_rmregs.EAX = 0x00004F00;
    vbe_rmregs.ES  = vbe_VbeInfoSeg.segment;
    vbe_rmregs.EDI = 0;
    dpmi_rminterrupt(0x10, &vbe_rmregs);
    vbe_status = vbe_rmregs.EAX;
    
    // also do not forget to translate all pointers from segment:offset to linear offset
    vbe_VbeInfoPtr->OemStringPtr = (char*)((((unsigned long)vbe_VbeInfoPtr->OemStringPtr >> 16) << 4) + (unsigned short)vbe_VbeInfoPtr->OemStringPtr);
    vbe_VbeInfoPtr->VideoModePtr = (unsigned short*)((((unsigned long)vbe_VbeInfoPtr->VideoModePtr >> 16) << 4) + (unsigned short)vbe_VbeInfoPtr->VideoModePtr);
    vbe_VbeInfoPtr->OemVendorNamePtr = (char*)((((unsigned long)vbe_VbeInfoPtr->OemVendorNamePtr >> 16) << 4) + (unsigned short)vbe_VbeInfoPtr->OemVendorNamePtr);
    vbe_VbeInfoPtr->OemProductNamePtr = (char*)((((unsigned long)vbe_VbeInfoPtr->OemProductNamePtr >> 16) << 4) + (unsigned short)vbe_VbeInfoPtr->OemProductNamePtr);
    vbe_VbeInfoPtr->OemProductRevPtr = (char*)((((unsigned long)vbe_VbeInfoPtr->OemProductRevPtr >> 16) << 4) + (unsigned short)vbe_VbeInfoPtr->OemProductRevPtr);
    
    if (p != NULL) memcpy(p, vbe_VbeInfoPtr, sizeof(vbe_VbeInfoBlock));
}

/*
     4.4. Function 00h - Return VBE Mode Information

     Input:    AX   = 4F01h   Return VBE mode information
               CX   =         Mode number
            ES:DI   =         Pointer to ModeInfoBlock structure

     Output:   AX   =         VBE Return Status

     Note: All other registers are preserved.
*/
void vbe_ModeInfo(unsigned short mode, vbe_ModeInfoBlock *p) {
    memset(&vbe_rmregs, 0, sizeof(vbe_rmregs));
    memset(vbe_ModeInfoPtr, 0, sizeof(vbe_ModeInfoBlock));
    
    vbe_rmregs.EAX = 0x00004F01;
    vbe_rmregs.ECX = (unsigned long)mode;
    vbe_rmregs.ES  = vbe_ModeInfoSeg.segment;
    vbe_rmregs.EDI = 0;
    dpmi_rminterrupt(0x10, &vbe_rmregs);
    vbe_status = vbe_rmregs.EAX;
    
    if (p != NULL) memcpy(p, vbe_ModeInfoPtr, sizeof(vbe_ModeInfoBlock));
}

/*
     4.5. Function 02h  - Set VBE Mode

     This required function initializes the controller and sets a VBE mode.
     The format of VESA VBE mode numbers is described earlier in this
     document. If the mode cannot be set, the BIOS should leave the
     graphics environment unchanged and return a failure error code.

     Input:    AX   = 4F02h     Set VBE Mode
               BX   =           Desired Mode to set
                    D0-D8  =    Mode number
                    D9-D13 =    Reserved (must be 0)
                    D14    = 0  Use windowed frame buffer model
                           = 1  Use linear/flat frame buffer model
                    D15    = 0  Clear display memory
                           = 1  Don't clear display memory

     Output:   AX   =           VBE Return Status

     Note: All other registers are preserved.
*/
void vbe_SetMode(unsigned short mode) {
    memset(&vbe_rmregs, 0, sizeof(vbe_rmregs));
    vbe_mode = mode;
    
    // set mode via VGA BIOS call if (mode < 0x100)
    if (mode < 0x100) {
        vbe_rmregs.EAX = (unsigned char)mode;
        dpmi_rminterrupt(0x10, &vbe_rmregs);
        vbe_status = 0x4F; // always successful bcoz we can't confirm this for VGA modeset
        return;
    }
    
#ifdef vbe_S3FIX
    // check if LFB + clear mode requested
    if ((mode & (vbe_MODE_LINEAR | vbe_MODE_NOCLEAR)) == vbe_MODE_LINEAR) {
        // set banked mode with clearing
        vbe_rmregs.EAX = 0x00004F02;
        vbe_rmregs.EBX = (unsigned long)(mode & ~vbe_MODE_LINEAR);
        dpmi_rminterrupt(0x10, &vbe_rmregs);
    
        // set LFB mode without clearing
        vbe_rmregs.EAX = 0x00004F02;
        vbe_rmregs.EBX = (unsigned long)(mode | vbe_MODE_NOCLEAR);
        dpmi_rminterrupt(0x10, &vbe_rmregs);
        vbe_status = vbe_rmregs.EAX;
        return;
    }
#endif

    vbe_rmregs.EAX = 0x00004F02;
    vbe_rmregs.EBX = (unsigned long)mode;
    dpmi_rminterrupt(0x10, &vbe_rmregs);
    vbe_status = vbe_rmregs.EAX;
}

/*
     4.6. Function 03h - Return current VBE Mode

     This required function returns the current VBE mode. The format of VBE
     mode numbers is described earlier in this document.

     Input:    AX   = 4F03h   Return current VBE Mode

     Output:   AX   =         VBE Return Status
               BX   =         Current VBE mode
                    D0-D13 =  Mode number
                    D14  = 0  Windowed frame buffer model
                         = 1  Linear/flat frame buffer model
                    D15  = 0  Memory cleared at last mode set
                         = 1  Memory not cleared at last mode set

     Note: All other registers are preserved.
*/

unsigned short vbe_GetMode() {
    memset(&vbe_rmregs, 0, sizeof(vbe_rmregs));
    
    vbe_rmregs.EAX = 0x00004F03;
    dpmi_rminterrupt(0x10, &vbe_rmregs);
    vbe_status = vbe_rmregs.EAX;
    
    return (unsigned short)vbe_rmregs.EBX;
}

// no function 0x4 - save\restore state - if someone need it, ask me

/*
     4.8. Function 05h - Display Window Control

     This required function sets or gets the position of the specified
     display window or page in the frame buffer memory by adjusting the
     necessary hardware paging registers.  To use this function properly,
     the software should first use VBE Function 01h (Return VBE Mode
     information) to determine the size, location and granularity of the
     windows.

     For performance reasons, it may be more efficient to call this
     function directly, without incurring the INT 10h overhead.  VBE
     Function 01h returns the segment:offset of this windowing function
     that may be called directly for this reason.  Note that a different
     entry point may be returned based upon the selected mode.  Therefore,
     it is necessary to retrieve this segment:offset specifically for each
     desired mode.

     Input:    AX   = 4F05h   VBE Display Window Control
               BH   = 00h          Set memory window
                    = 01h          Get memory window
               BL   =         Window number
                    = 00h          Window A
                    = 01h          Window B
               DX   =         Window number in video memory in window
                              granularity units  (Set Memory Window only)

     Output:   AX   =         VBE Return Status
               DX   =         Window number in window granularity units
                                   (Get Memory Window only)
*/
// there are two functions - get and set memory window
unsigned short vbe_GetWindowPos(unsigned short num) {
    memset(&vbe_rmregs, 0, sizeof(vbe_rmregs));
    
    vbe_rmregs.EAX = 0x00004F05;
    vbe_rmregs.EBX = (0x00000100 | (unsigned char)(num));
    dpmi_rminterrupt(0x10, &vbe_rmregs);
    vbe_status = vbe_rmregs.EAX;
    
    return (unsigned short)vbe_rmregs.EDX;
}

void vbe_SetWindowPos(unsigned short num, unsigned short pos) {
    memset(&vbe_rmregs, 0, sizeof(vbe_rmregs));
    
    vbe_rmregs.EAX = 0x00004F05;
    vbe_rmregs.EBX = (0x00000000 | (unsigned char)(num));
    vbe_rmregs.EDX = (unsigned short)(pos);
    dpmi_rminterrupt(0x10, &vbe_rmregs);
    vbe_status = vbe_rmregs.EAX;
}

/*
     4.9. Function 06h - Set/Get Logical Scan Line Length

     This required function sets or gets the length of a logical scan line.
     This allows an application to set up a logical display memory buffer
     that is wider than the displayed area. VBE Function 07h (Set/Get
     Display Start) then allows the application to set the starting
     position that is to be displayed.

     Input:    AX   = 4F06h   VBE Set/Get Logical Scan Line Length
               BL   = 00h          Set Scan Line Length in Pixels
                    = 01h          Get Scan Line Length
                    = 02h          Set Scan Line Length in Bytes
                    = 03h          Get Maximum Scan Line Length
               CX   =         If BL=00h  Desired Width in Pixels
                              If BL=02h  Desired Width in Bytes
                              (Ignored for Get Functions)

     Output:   AX   =         VBE Return Status
               BX   =         Bytes Per Scan Line
               CX   =         Actual Pixels Per Scan Line
                              (truncated to nearest complete pixel)
               DX   =         Maximum Number of Scan Lines
*/               
// ehm...here goes somekind of охгдеж :) bcoz this call returs a LOT of
// useful and necessary variables...nevermind

// set scanline length in PIXELS, not in bytes currently.
// bits 16-31 - actual scanline len in pixels, 0-15 - new bytes per scanline
unsigned long vbe_SetScanlineLength(unsigned short len) {
    memset(&vbe_rmregs, 0, sizeof(vbe_rmregs));
    
    vbe_rmregs.EAX = 0x00004F06;
    vbe_rmregs.EBX = 0x00000000;
    vbe_rmregs.ECX = (unsigned short)(len);
    dpmi_rminterrupt(0x10, &vbe_rmregs);
    vbe_status = vbe_rmregs.EAX;
    
    return ((unsigned short)vbe_rmregs.EBX) | (unsigned long)(((unsigned short)vbe_rmregs.ECX) << 16);
}
// bits 16-31 - actual scanline len in pixels, 0-15 - new bytes per scanline
unsigned long vbe_SetBytesPerScanline(unsigned short len) {
    memset(&vbe_rmregs, 0, sizeof(vbe_rmregs));
    
    vbe_rmregs.EAX = 0x00004F06;
    vbe_rmregs.EBX = 0x00000002;
    vbe_rmregs.ECX = (unsigned short)(len);
    dpmi_rminterrupt(0x10, &vbe_rmregs);
    vbe_status = vbe_rmregs.EAX;
    
    return ((unsigned short)vbe_rmregs.EBX) | (unsigned long)(((unsigned short)vbe_rmregs.ECX) << 16);
}
// bits 16-31 - scanline len in pixels, 0-15 - bytes per scanline
unsigned long vbe_GetScanlineLength() {
    memset(&vbe_rmregs, 0, sizeof(vbe_rmregs));
    
    vbe_rmregs.EAX = 0x00004F06;
    vbe_rmregs.EBX = 0x00000001;
    dpmi_rminterrupt(0x10, &vbe_rmregs);
    vbe_status = vbe_rmregs.EAX;
    
    return ((unsigned short)vbe_rmregs.EBX) | (unsigned long)(((unsigned short)vbe_rmregs.ECX) << 16);
}
// bits 16-31 - scanline len in pixels, 0-15 - bytes per scanline
unsigned long vbe_GetMaxScanlineLength() {
    memset(&vbe_rmregs, 0, sizeof(vbe_rmregs));
    
    vbe_rmregs.EAX = 0x00004F06;
    vbe_rmregs.EBX = 0x00000003;
    dpmi_rminterrupt(0x10, &vbe_rmregs);
    vbe_status = vbe_rmregs.EAX;
    
    return ((unsigned short)vbe_rmregs.EBX) | (unsigned long)(((unsigned short)vbe_rmregs.ECX) << 16);
}

/*
     4.10.     Function 07h - Set/Get Display Start

     This required function selects the pixel to be displayed in the upper
     left corner of the display.  This function can be used to pan and
     scroll around logical screens that are larger than the displayed
     screen.  This function can also be used to rapidly switch between two
     different displayed screens for double buffered animation effects.

     Input:    AX   = 4F07h   VBE Set/Get Display Start Control
               BH   = 00h          Reserved and must be 00h
               BL   = 00h          Set Display Start
                    = 01h          Get Display Start
                    = 80h          Set Display Start during Vertical
     Retrace
               CX   =         First Displayed Pixel In Scan Line
                              (Set Display Start only)
               DX   =         First Displayed Scan Line (Set Display Start
     only)

     Output:   AX   =         VBE Return Status
               BH   =         00h Reserved and will be 0 (Get Display Start
     only)
               CX   =         First Displayed Pixel In Scan Line (Get Display
                              Start only)
               DX   =         First Displayed Scan Line (Get Display
                              Start only)
*/
void vbe_SetDisplayStart(unsigned short x, unsigned short y, unsigned char flags) {
    memset(&vbe_rmregs, 0, sizeof(vbe_rmregs));
    
    vbe_rmregs.EAX = 0x00004F07;
    vbe_rmregs.EBX = (0x00000000 | ((unsigned char)(flags & vbe_WAITRETRACE)));
    vbe_rmregs.ECX = (unsigned short)x;
    vbe_rmregs.EDX = (unsigned short)y;
    dpmi_rminterrupt(0x10, &vbe_rmregs);
    vbe_status = vbe_rmregs.EAX;
    
}
// bits 16-31 - y, 0-15 - x
unsigned long vbe_GetDisplayStart() {
    memset(&vbe_rmregs, 0, sizeof(vbe_rmregs));
    
    vbe_rmregs.EAX = 0x00004F07;
    vbe_rmregs.EBX = 0x00000001;
    dpmi_rminterrupt(0x10, &vbe_rmregs);
    vbe_status = vbe_rmregs.EAX;
    
    return ((unsigned short)vbe_rmregs.ECX) | (unsigned long)(((unsigned short)vbe_rmregs.EDX) << 16);
}

/*

     4.11.     Function 08h - Set/Get DAC Palette Format

     This required function manipulates the operating mode or format of the
     DAC palette. Some DACs are configurable to provide 6 bits, 8 bits, or
     more of color definition per red, green, and blue primary colors.
     The DAC palette width is assumed to be reset to the standard VGA value
     of 6 bits per primary color during any mode set

     Input:    AX   = 4F08h   VBE Set/Get Palette Format
               BL   = 00h          Set DAC Palette Format
                    = 01h          Get DAC Palette Format
               BH   =         Desired bits of color per primary
                              (Set DAC Palette Format only)

     Output:   AX   =         VBE Return Status
               BH   =         Current number of bits of color per primary
*/
unsigned char vbe_SetDACWidth(unsigned char width) {
    memset(&vbe_rmregs, 0, sizeof(vbe_rmregs));
    
    vbe_rmregs.EAX = 0x00004F08;
    vbe_rmregs.EBX = ((unsigned long)(width << 8) & 0x0000FF00);
    dpmi_rminterrupt(0x10, &vbe_rmregs);
    vbe_status = vbe_rmregs.EAX;
    
    return ((unsigned char)(vbe_rmregs.EBX >> 8));
}

unsigned char vbe_GetDACWidth() {
    memset(&vbe_rmregs, 0, sizeof(vbe_rmregs));
    
    vbe_rmregs.EAX = 0x00004F08;
    vbe_rmregs.EBX = 0x00000001;
    dpmi_rminterrupt(0x10, &vbe_rmregs);
    vbe_status = vbe_rmregs.EAX;
    
    return ((unsigned char)(vbe_rmregs.EBX >> 8));
}

/*
4.12.     Function 09h - Set/Get Palette Data

     This required function is very important for RAMDAC's which are larger
     than a standard VGA RAMDAC.  The standard INT 10h BIOS Palette
     function calls assume standard VGA ports and VGA palette widths.  This
     function offers a palette interface that is independent of the VGA
     assumptions.


     Input:    AX   = 4F09h   VBE Load/Unload Palette Data
               BL   = 00h          Set Palette Data
                    = 01h          Get Palette Data
                    = 02h          Set Secondary Palette Data
                    = 03h          Get Secondary Palette Data
                    = 80h          Set Palette Data during Vertical Retrace
                                with Blank Bit on
               CX   =         Number of palette registers to update
               DX   =         First palette register to update
               ES:DI=         Table of palette values (see below for
     format)

     Output:   AX   =         VBE Return Status

     Format of Palette Values:   Alignment byte, Red byte, Green byte, Blue
     byte [wbcbz7 note: this is WRONG, should be blue\green\red\align byte]

     Note: The need for BL= 80h is for older style RAMDAC's where
     programming the RAM values during display time causes a "snow-like"
     effect on the screen.  Newer style RAMDAC's don't have this limitation
     and can easily be programmed at any time, but older RAMDAC's require
     that they be programmed during a non-display time only to stop the
     snow like effect seen when changing the DAC values.  When this is
     requested the VBE implementation will program the DAC with blanking
     on.  Check D2 of the Capabilities field returned by VBE Function 00h
     to determine if 80h should be used instead of 00h.

     Note: The need for the secondary palette is for anticipated future
     palette extensions, if a secondary palette does not exist in a
     implementation and these calls are made, the VBE implementation will
     return error code 02h.

     Note:  When in 6 bit mode, the format of the 6 bits is LSB, this is
     done for speed reasons, as the application can typically shift the
     data faster than the BIOS can.

     Note: All application should assume the DAC is defaulted to 6 bit
     mode.  The application is responsible for switching the DAC to higher
     color modes using Function 08h.

     Note: Query VBE Function 08h to determine the RAMDAC width before
     loading a new palette
*/

// if someone still stays in WTF state then I would explain this shit :)
// use typedef struct {unsigned char b, g, r, a} pal; then alloc an array
// of <num> palette entries starting with <start> index, then pass a pointer
// to this array. format - b0..6 for 6bit and b0..8 for 8bit palette data

void vbe_SetPalette(void* data, unsigned short start, unsigned short num, unsigned char flags) {
    memset(&vbe_rmregs, 0, sizeof(vbe_rmregs));
    memcpy(vbe_PalBuf, data, num * sizeof(unsigned long));
    
    vbe_rmregs.EAX = 0x00004F09;
    vbe_rmregs.EBX = (0x00000000 | ((unsigned char)(flags & vbe_WAITRETRACE)));
    vbe_rmregs.ECX = (unsigned short)num;
    vbe_rmregs.EDX = (unsigned short)start;
    vbe_rmregs.ES  = vbe_PalBufSeg.segment;
    vbe_rmregs.EDI = 0;
    dpmi_rminterrupt(0x10, &vbe_rmregs);
    vbe_status = vbe_rmregs.EAX;
}

void vbe_GetPalette(void* data, unsigned short start, unsigned short num, unsigned char flags) {
    memset(&vbe_rmregs, 0, sizeof(vbe_rmregs));
    
    vbe_rmregs.EAX = 0x00004F09;
    vbe_rmregs.EBX = (0x00000001 | ((unsigned char)(flags & vbe_WAITRETRACE)));
    vbe_rmregs.ECX = (unsigned short)num;
    vbe_rmregs.EDX = (unsigned short)start;
    vbe_rmregs.ES  = vbe_PalBufSeg.segment;
    vbe_rmregs.EDI = 0;
    dpmi_rminterrupt(0x10, &vbe_rmregs);
    vbe_status = vbe_rmregs.EAX;
    
    memcpy(data, vbe_PalBuf, num * sizeof(unsigned long));
}

// ----------------------------------------------------------------
// init procedure - call it before using ANY of functions below!
int vbe_Init() {
    dpmi_getdosmem((sizeof(vbe_VbeInfoBlock) >> 4) + 1, &vbe_VbeInfoSeg);
    dpmi_getdosmem((sizeof(vbe_ModeInfoBlock) >> 4) + 1, &vbe_ModeInfoSeg);
    dpmi_getdosmem(((sizeof(unsigned long) * 256) >> 4) + 1, &vbe_PalBufSeg);
    
    vbe_VbeInfoPtr  = (vbe_VbeInfoBlock*)(vbe_VbeInfoSeg.segment << 4);
    vbe_ModeInfoPtr = (vbe_ModeInfoBlock*)(vbe_ModeInfoSeg.segment << 4);
    vbe_PalBuf      = (unsigned long*)(vbe_PalBufSeg.segment << 4);
    
    vbe_ControllerInfo(NULL); // get controller info for internal procedures
    if (vbe_status != 0x4F) return -1; // fail
    
    vbe_mode = vbe_GetMode(); // get current mode
    return 0;
}

// uninit procedure - call it on da end of application.
void vbe_Done() {
    if (vbe_LastPhysicalMap != NULL) dpmi_unmapphysical(vbe_LastPhysicalMap);
    dpmi_freedosmem(&vbe_VbeInfoSeg);
    dpmi_freedosmem(&vbe_ModeInfoSeg);
    dpmi_freedosmem(&vbe_PalBufSeg);
}

// ----------------------------------------------------------------
// medium-level VESA procedures

// finds a (xres x yres x bpp) mode with requested ModeAttributes flags
// returns mode number or -1 if failed
int vbe_FindMode(unsigned short xres, unsigned short yres, unsigned short bpp, unsigned short flags) {
    vbe_ModeInfoBlock modeinfo;
    unsigned int rawmode, realbpp, rawflags;
    unsigned int i, j, k;
    
    // additional check for mode is supported by hardware
    rawflags = (flags | vbe_ATTR_HardwareMode);
    //rawflags = flags;
    
    vbe_ControllerInfo(NULL); if (vbe_status != 0x4F) return -1;
    
    for (i = 0; ((vbe_VbeInfoPtr->VideoModePtr[i] != 0xFFFF) && (vbe_VbeInfoPtr->VideoModePtr[i] != 0)); i++) {
        vbe_ModeInfo(vbe_VbeInfoPtr->VideoModePtr[i], &modeinfo);
        
#ifdef vbe_FAKECHECK
        // check for fake or buggy 15\16bpp modes
        // also fixes one of matrox vbe bugs - 15bpp modes have an wrong 16bpp value in modeinfo.BitsPerPixel
        if ((bpp == 15) || (bpp == 16)) {
            realbpp = modeinfo.RedMaskSize + modeinfo.GreenMaskSize + modeinfo.BlueMaskSize;
        } else
        
        // check for fake or buggy 24\32bpp modes
        // afaik 3dfx cards incorrectly(?) sets 24bpp in modeinfo.BitsPerPixel but in fact uses 32bpp (need to confirm)
        // so we need to calculate real bpp
        if ((bpp == 24) || (bpp == 32)) {
            realbpp = modeinfo.RedMaskSize + modeinfo.GreenMaskSize + modeinfo.BlueMaskSize + modeinfo.RsvdMaskSize;
        } else
#endif
        realbpp = modeinfo.BitsPerPixel;
   
        if ((modeinfo.XResolution == xres) && (modeinfo.YResolution == yres) &&
            (realbpp == bpp) && ((modeinfo.ModeAttributes & rawflags) == rawflags)) {
            rawmode = vbe_VbeInfoPtr->VideoModePtr[i];
            return rawmode;
        }
    }
    return -1;
}

// returns a mapped linear pointer to video memory, NULL if failed
// for banked modes returns window A address
// for LFB modes does all mapping so you must not call dpmi_mapphysical() before
// for VGA modes returns...ehm...well known 0xA0000 :)
void * vbe_GetVideoPtr() {
    int rawmode;
    void *map;
    vbe_ModeInfoBlock modeinfo;
    vbe_VbeInfoBlock  vbeinfo;
    
    // VGA mode
    if (vbe_mode < 0x100) {
        if (vbe_mode < 6) return (void*)0xB8000; else return (void*)0xA0000;
    }
    
    rawmode = (vbe_mode == 0x81FF ? vbe_mode : (vbe_mode & 0x7FF));
    vbe_ControllerInfo(&vbeinfo); if (vbe_status != 0x4F) return NULL;
    vbe_ModeInfo(rawmode, &modeinfo); if (vbe_status != 0x4F) return NULL;
    if ((vbe_mode & vbe_MODE_LINEAR) == vbe_MODE_LINEAR) {
    // VESA linear mode
        if (vbe_LastPhysicalMap != NULL) dpmi_unmapphysical(vbe_LastPhysicalMap);
        if (modeinfo.PhysBasePtr == 0) return NULL; 
        vbe_LastPhysicalMap = (void*)dpmi_mapphysical((vbeinfo.TotalMemory * 65536), modeinfo.PhysBasePtr);
        if (dpmi_status) return NULL; // fuck to NTVDM ;)
        map = vbe_LastPhysicalMap;
        return map;
    } else {
    // VESA banked mode
        //if ((modeinfo.WinAAttributes & vbe_ATTR_No_SegA000) == vbe_ATTR_No_SegA000) return NULL;
        map = (void*)(modeinfo.WinASegment << 4);
        return map;
    }
}

#ifdef __cplusplus
}
#endif

#endif
