
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

// actual start
tga_header in, out;
unsigned long  *inbuf;
unsigned short *outbuf;

enum {
    tgaload_ok, tgaload_nofile, tgaload_ioerr, tgaload_fmterr, tgaload_memerr
};

static char* tga_load_str[] = {
    "ok\n",
    "error: input file not found",
    "error: I\O error",
    "error: incorrect input format",
    "error: memory allocation error"
};

// load input TGA file
int tga_load(char *fname, tga_header *head, unsigned long **buf, int *order) {
    FILE *f;
    
    // open file
    f = fopen(fname, "rb");
    if (!f) return tgaload_nofile;
    
    // read header
    fread(head, sizeof(tga_header), 1, f);
    if (ferror(f)) {fclose(f); return tgaload_ioerr;}
    
    // check format
    if ((head->ImageType != 2) || (head->PixelDepth < 24)) {fclose(f); return tgaload_fmterr;}
    
    *order ^= (head->ImageDescriptor & 0x20 ? -1 : 0);
    fseek(f, sizeof(tga_header) + head->IDLength + (head->CMapLength * ((head->CMapDepth + 7) >> 3)), SEEK_SET);
    if (feof(f)) {fclose(f); return tgaload_ioerr;}
    
    *buf = new unsigned long[head->Width*head->Height];
    
    unsigned long *p = (*order ? *buf + head->Width * (head->Height-1) : *buf);
    for (int i = 0; i < head->Height; i++) {
        if (head->PixelDepth == 24) {
            for (int j = 0; j < head->Width; j++) {
                int hui = fgetc(f);
                hui |= (fgetc(f) << 8);
                hui |= (fgetc(f) << 16) | 0xFF000000;
                *p++ = hui;
            }
            p -= head->Width;
        } else fread(p, head->Width, sizeof(unsigned long), f);
        if (ferror(f)) {fclose(f); return tgaload_ioerr;}
        if (*order) p -= head->Width; else p += head->Width;
    }
    
    fclose(f);
    return tgaload_ok;
}

#define mk16(a, r, g, b) (((a >> 7) << 15) | ((r >> 3) << 10) | ((g >> 3) << 5) | ((b >> 3) << 0))
#define findclosest(a)     (a & 0xF8)
#define sat(a, l)          (a > l ? l : (a < 0 ? 0 : a))

/*
static unsigned int dithertab[4*4] = {
    0, 6, 1, 7,
    4, 2, 5, 3,
    1, 7, 0, 6,
    5, 3, 4, 2
};
*/

static unsigned int dithertab[4*4] = {
    0, 3, 0, 3,
    2, 1, 2, 1,
    0, 3, 0, 3,
    2, 1, 2, 1
};

int tga_convert(unsigned short *out, unsigned long *in, tga_header *head, int method) {
    unsigned long  *p = in;
    unsigned short *v = out;
    
    for (int y = 0; y < head->Height; y++) {
        for (int x = 0; x < head->Width; x++) {
            int a = (*p   >> 24) & 0xFF;
            int r = (*p   >> 16) & 0xFF;
            int g = (*p   >>  8) & 0xFF;
            int b = (*p++ >>  0) & 0xFF;
            int tabptr = ((y & 3) << 2) | (x & 3);
            
            if (method) {
                r = findclosest(sat((r + dithertab[tabptr]), 0xFF));
                g = findclosest(sat((g + dithertab[tabptr]), 0xFF));
                b = findclosest(sat((b + dithertab[tabptr]), 0xFF));
            } else {
                r = findclosest(r);
                g = findclosest(g);
                b = findclosest(b);
            }
            a = (a < 16 ? 0x80 : 0);
            
            *v++ = mk16(a, r, g, b);
        }
        printf("."); fflush(stdout);
    }
    
    return 0;
}

int main(int argc, char *argv[]) {
    int rtn, order = 0, method = 0;    
    
    printf("TGA 24\\32->15bpp converter - by wbcbz7 zl.o8.zolb\n");
    
    if (argc < 3) {
        printf("usage: tgaconv.exe [infile] [outfile] <-f> <-d>\n");
        printf("       -f - flip image vertically\n");
        printf("       -d - dither image\n");
        return 1;
    }
    for (int i = 3; i < argc; i++) {
        if (strstr(strupr(argv[i]), "F")) order = -1;
        if (strstr(strupr(argv[i]), "D")) method = 1;
    }
    
    // allocate memory for image
    //inbuf = new unsigned long[1024*1024];
    
    rtn = tga_load(argv[1], &in, &inbuf, &order);
    if (rtn) {printf("tga_load() -> %s\n", tga_load_str[rtn]); return 1;}
    
    // gotta loaded picture :)
    printf("infile:  %s -> %dx%d %dbpp\n", argv[1], in.Width, in.Height, in.PixelDepth);
    outbuf = new unsigned short[in.Width * in.Height];
    
    // convert it
    printf("converting: ");
    tga_convert(outbuf, inbuf, &in, method);
    printf(" done\n");
    
    // write file to disk
    memset(&out, 0, sizeof(tga_header));
    out.ImageType       = 2;
    out.Width           = in.Width;
    out.Height          = in.Height;
    out.PixelDepth      = 16;
    out.ImageDescriptor = ((order & 1) << 5) | 1;
    
    FILE *f = fopen(argv[2], "wb");
    fwrite(&out,   sizeof(tga_header), 1, f);
    fwrite(outbuf, in.Width * in.Height, sizeof(unsigned short), f);
    fclose(f);
    
    printf("outfile: %s -> %dx%d %dbpp\n", argv[2], out.Width, out.Height, out.PixelDepth);
    printf("done\n");
    
    delete[] inbuf; delete[] outbuf;
    return 0;
}