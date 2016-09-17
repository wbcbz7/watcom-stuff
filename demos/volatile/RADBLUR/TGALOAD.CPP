#ifndef TGALOAD
#define TGALOAD
// tga loader блабла там всякая херня в комментах лень писать :)
// by wbcbz7 lz.o7.zolb
// ну там инклудить надо, понимаешь... (ц)

#include <stdio.h>

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

// tga header
#pragma pack (push, 1)
typedef struct {
    unsigned char  IDLength;        /* 00h  Size of Image ID field */
    unsigned char  ColorMapType;    /* 01h  Color map type */
    unsigned char  ImageType;       /* 02h  Image type code */
    unsigned short CMapStart;       /* 03h  Color map origin */
    unsigned short CMapLength;      /* 05h  Color map length */
    unsigned char  CMapDepth;       /* 07h  Depth of color map entries */
    unsigned short XOffset;         /* 08h  X origin of image */
    unsigned short YOffset;         /* 0Ah  Y origin of image */
    unsigned short Width;           /* 0Ch  Width of image */
    unsigned short Height;          /* 0Eh  Height of image */
    unsigned char  PixelDepth;      /* 10h  Image pixel size */
    unsigned char  ImageDescriptor; /* 11h  Image descriptor byte */
} tga_header;
#pragma pack (pop)

// вообще, надо делать format-independent, но мне как-то лень =)

enum {
    img_errOK, img_errFileNotFound, img_errIO, img_errEOF, img_errInvalidFormat
};

// there are two functions - for file i\o via stdio (i love stdio! :) and for memory io
int img_loadfile(t_image *image, char* fname) {
    FILE* f;
    unsigned char *pal;
    int shift;
    tga_header header;
    
    // open file
    f = fopen(fname, "rb");
    if (!f) return img_errFileNotFound;
    
    // read header
    fread(&header, sizeof(tga_header), 1, f);
    if (ferror(f)) {fclose(f); return img_errIO;}
    
    // clear image
    memset(image, 0, sizeof(t_image));
    
    image->width    = header.Width;
    image->height   = header.Height;
    image->xoffset  = header.XOffset;
    image->yoffset  = header.YOffset;
    
    switch(header.ImageType) {
        case 1: // image + palette
            if ((header.PixelDepth > 8) || (header.CMapDepth < 18) ||
                (header.ColorMapType != 1)) {
                fclose(f); return img_errInvalidFormat;
            }
            image->palstart = header.CMapStart;
            image->palnum   = header.CMapLength;
            
            // load palette data
            image->pal = new unsigned int[image->palnum]; 
            pal = (unsigned char*)image->pal;
            shift = (header.CMapDepth == 18 ? 2 : 0);
            
            fseek(f, sizeof(tga_header) + header.IDLength, SEEK_SET);
            
            for (int i = 0; i < image->palnum; i++) {
                *pal = fgetc(f); *pal++ <<= shift;
                *pal = fgetc(f); *pal++ <<= shift;
                *pal = fgetc(f); *pal++ <<= shift;
                if (header.CMapDepth == 32) *pal++ = fgetc(f); else pal++;
                if (ferror(f)) {fclose(f); return img_errIO;}
            }
            image->bpp      = header.PixelDepth;
            break;
            
        case 2: // direct color image
            if ((header.PixelDepth < 8) || (header.ColorMapType == 1)) {
                fclose(f); return img_errInvalidFormat;
            }
            // since no pallete inside we can ignore it
            image->bpp  = header.PixelDepth;
            image->attr |= img_ATTR_DIRECTCOLOR;
            // special case for 16bpp images - they are 15bpp + alpha
            if (image->bpp == 16) {image->bpp = 15; image->attr |= img_ATTR_ALPHA;}
            if (image->bpp == 32)  image->attr |= img_ATTR_ALPHA;
            break;
            
        case 3: // grayscale image
            if ((header.PixelDepth > 8) || (header.ColorMapType == 1)) {
                fclose(f); return img_errInvalidFormat;
            }
            
            image->palstart = 0;
            image->palnum   = 256;
            
            // create monochrome palette data
            image->pal = new unsigned int[image->palnum]; 
            pal = (unsigned char*)image->pal;
            
            for (int i = 0; i < image->palnum; i++) {
                *pal++ = i;
                *pal++ = i; 
                *pal++ = i;
                pal++;
            }
            image->bpp      = header.PixelDepth;
            break;
            
        default: // compressed\etc...
            fclose(f); return img_errInvalidFormat;
    }
    
    image->pitch    = header.Width * ((image->bpp + 7) >> 3);
    
    int order = (header.ImageDescriptor & 0x20);
    fseek(f, sizeof(tga_header) + header.IDLength + (header.CMapLength * ((header.CMapDepth + 7) >> 3)), SEEK_SET);
    if (feof(f)) {fclose(f); return img_errEOF;}
    
    // allocate memory for image
    image->ptr = new unsigned char[image->pitch*image->height];
    
    // read image
    // image stored just as width*((bpp+7)>>3)*height array, no pitch correction is required
    if (order) {
        // best case - image arranged from upper left corner
        // so read it as one BIIIIG chunk
        fread(image->ptr, image->pitch*image->height, 1, f);
        if (ferror(f)) {fclose(f); return img_errIO;}
    } else {
        // image arranged from lower left corner, ffuck! >_<
        unsigned char *p = (unsigned char*)(image->ptr) + (image->pitch*(image->height-1));
        for (int i = 0; i < image->height; i++) {
            fread(p, image->pitch, 1, f);
            if (ferror(f)) {fclose(f); return img_errIO;}
            p -= image->pitch;
        }
    }
    
    // that's all, image is loaded
    fclose(f);
    return img_errOK;
}

int img_free(t_image *image) {
    if (image->ptr) delete[] image->ptr;
    if (image->pal) delete[] image->pal;
    memset(image, 0, sizeof(t_image));
    return img_errOK;
}

#endif