#ifndef TGALOAD_H
#define TGALOAD_H
// tga loader блабла там всякая херня в комментах лень писать :)
// by wbcbz7 lz.o7.zolb
// ну там инклудить надо, понимаешь... (ц)

typedef struct {
    unsigned int   width;
    unsigned int   pitch;
    unsigned int   height;
    unsigned int   xoffset;    // relative to (0,0)
    unsigned int   yoffset;    // relative to (0,0)
    unsigned int   bpp;
    unsigned int   attr;       // see below
    unsigned char* ptr;        // stored from upper-left corner
    // palette-specific data
    unsigned int   palstart;   // start index
    unsigned int   palnum;     // num of palette entries
    unsigned int*  pal;        // always stored as 32bpp ARGB
} t_image;

//  8bpp rgb images stored in unsigned char      as RGB332
// 12bpp rgb images stored in unsigned short     as ARGB4444
// 15bpp rgb images stored in unsigned short     as ARGB1555
// 16bpp rgb images stored as unsigned short     as RGB565
// 24bpp rgb images stored as 3*unsigned char(!) as RGB888
// 32bpp rgb images stored as unsigned int       as ARGB8888

// for 15bpp alpha defined as 0 - opaque, 1 - transparent
// for 32bpp alpha defined as 0 - transparent, 255 - opaque,
// any value in (0, 255) - transparency;
// same for 12bpp but alpha is limited to [0; 16)

// t_image attributes
// direct color image
#define img_ATTR_DIRECTCOLOR 1
// alpha bits are used as alpha channel
#define img_ATTR_ALPHA       2

enum {
    img_errOK, img_errFileNotFound, img_errIO, img_errEOF, img_errInvalidFormat
};

int img_loadfile(t_image *image, char* fname);
int img_free(t_image *image);

#endif