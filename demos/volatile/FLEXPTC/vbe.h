#ifndef WC_VBE20_H
#define WC_VBE20_H
/*
  vbe.h - headers and defines
*/

// prevent crashes on some S3 BIOSes
#define vbe_S3FIX

// check for fake 15\16 bit modes
#define vbe_FAKECHECK

#pragma pack (push, 1)

// VESA pre-defined structures
typedef struct {
  unsigned char  vbeSignature[4];
  unsigned short vbeVersion;
  char *         OemStringPtr;
  unsigned long  Capabilities;
  unsigned short * VideoModePtr;
  unsigned short TotalMemory;
  // added in vbe 2.0
  unsigned short OemSoftwareRev;
  char *         OemVendorNamePtr;
  char *         OemProductNamePtr;
  char *         OemProductRevPtr;
  unsigned char  Reserved[222];
  unsigned char  OemData[256];
} vbe_VbeInfoBlock ;

typedef struct {
  unsigned short ModeAttributes;
  unsigned char  WinAAttributes;
  unsigned char  WinBAttributes;
  unsigned short Granularity;
  unsigned short WinSize;
  unsigned short WinASegment;
  unsigned short WinBSegment;
  void *         WinFuncPtr;
  unsigned short BytesPerScanline;
  unsigned short XResolution;
  unsigned short YResolution;
  unsigned char  XCharSize;
  unsigned char  YCharSize;
  unsigned char  NumberOfPlanes;
  unsigned char  BitsPerPixel;
  unsigned char  NumberOfBanks;
  unsigned char  MemoryModel;
  unsigned char  BankSize;
  unsigned char  NumberOfImagePages;
  unsigned char  Reserved;
  unsigned char  RedMaskSize;
  unsigned char  RedFieldPosition;
  unsigned char  GreenMaskSize;
  unsigned char  GreenFieldPosition;
  unsigned char  BlueMaskSize;
  unsigned char  BlueFieldPosition;
  unsigned char  RsvdMaskSize;
  unsigned char  RsvdFieldPosition;
  unsigned char  DirectColorModeInfo;
  // added in vbe 2.0
  void *         PhysBasePtr;
  void *         OffScreenMemOffset;
  unsigned short OffScreenMemSize;
  unsigned char  reserved2[206];
} vbe_ModeInfoBlock;

#pragma pack (pop)

#define VBE2SIGNATURE            "VBE2"
#define VBE3SIGNATURE            "VBE3"

// VESA bitfields

// mode number bitfields
#define vbe_MODE_USERCRTC         0x800
#define vbe_MODE_LINEAR          0x4000
#define vbe_MODE_NOCLEAR         0x8000

// status bitfields
#define vbe_STATUS_OK                 0
#define vbe_STATUS_FAIL           0x100
#define vbe_STATUS_NOTSUPPORTED   0x200
#define vbe_STATUS_INVALIDMODE    0x300

// ниже спиздил у submissive :)
/* Bit-Definition of the ControllerInfo Capability fields */
#define vbe_CAP_8bit_DAC              1
#define vbe_CAP_Not_VGA_Compatible    2
#define vbe_CAP_Use_VBE_DAC_Functions 4

/* Bit-Definition of the ModeInfo Attribute fields */
#define vbe_ATTR_HardwareMode         1
#define vbe_ATTR_TTY_Support          4
#define vbe_ATTR_ColorMode            8
#define vbe_ATTR_GraphicsMode        16
#define vbe_ATTR_No_VGA_Mode         32
#define vbe_ATTR_No_SegA000          64
#define vbe_ATTR_LFB_Support        128

/* Bit-Definitions of the Window Attributes */
#define vbe_WATTR_Relocatable         1
#define vbe_WATTR_Readable            2
#define vbe_WATTR_Writeable           4

/* Definitions of the MemoryModel Field */
#define vbe_MM_TextMode               0
#define vbe_MM_CGA_Graphics           1
#define vbe_MM_Hercules_Graphics      2
#define vbe_MM_Planar                 3
#define vbe_MM_PackedPixel            4
#define vbe_MM_UnChained              5
#define vbe_MM_DirectColor            6
#define vbe_MM_YUV                    7

// bitfield for funktions 0x7 and 0x9
#define vbe_WAITRETRACE            0x80

// functions
void vbe_ControllerInfo(vbe_VbeInfoBlock *p);
void vbe_ModeInfo(unsigned short mode, vbe_ModeInfoBlock *p);
void vbe_SetMode(unsigned short mode);
unsigned short vbe_GetMode();
unsigned short vbe_GetWindowPos(unsigned short num);
void vbe_SetWindowPos(unsigned short num, unsigned short pos);
unsigned long vbe_SetScanlineLength(unsigned short len);
unsigned long vbe_SetBytesPerScanline(unsigned short len);
unsigned long vbe_GetScanlineLength();
unsigned long vbe_GetMaxScanlineLength();
void vbe_SetDisplayStart(unsigned short x, unsigned short y, unsigned char flags);
unsigned long vbe_GetDisplayStart();
unsigned char vbe_SetDACWidth(unsigned char width);
unsigned char vbe_GetDACWidth();
void vbe_SetPalette(void* data, unsigned short start, unsigned short num, unsigned char flags);
void vbe_GetPalette(void* data, unsigned short start, unsigned short num, unsigned char flags);
int vbe_Init();
void vbe_Done();
int vbe_FindMode(unsigned short xres, unsigned short yres, unsigned short bpp, unsigned short flags);
void * vbe_GetVideoPtr();
int vbe_GetStatus();

#endif
