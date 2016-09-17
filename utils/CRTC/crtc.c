#include <conio.h>
#include <stdio.h>

unsigned char cr[256], sr[256];
FILE *f;

// vga timings
int htotal, hdispend, hblankstart, hblankend, hsyncstart, hsyncend;
int vtotal, vdispend, vblankstart, vblankend, vsyncstart, vsyncend, vcharheight;

// additional timing values
unsigned int hactive, hfrontporch, hbackporch, hsyncwidth;
unsigned int vactive, vfrontporch, vbackporch, vsyncwidth;

int main(int argc, char* argv[]) {
    
    if (argc < 2) {puts("usage: crtc.exe <filename.ext>"); return 1;}
    
    f = fopen(argv[1], "rb");
    
    fread(&cr, sizeof(cr), 1, f);
    fread(&sr, sizeof(sr), 1, f);
    
    fclose(f);
    
    htotal      = (cr[0x0]) + 5;
    hdispend    =  cr[0x1];
    hblankstart =  cr[0x2];
    hblankend   =  (hblankstart & ~0x3F) | (cr[0x3] & 0x1F) | ((cr[0x5] & 0x80) >> 2);
    hsyncstart  =  cr[0x4];
    hsyncend    =  (hsyncstart & ~0xF) | (cr[0x5] & 0xF);  if ((cr[0x5] & 0xF) < (hsyncstart & 0xF)) hsyncend += (0xF + 1);
    
    hactive     = hdispend + 1;
    hfrontporch = hsyncstart - hdispend;
    hbackporch  = htotal - hsyncend;
    hsyncwidth  = hsyncend - hsyncstart;
    
    vtotal      = (cr[0x6]  | ((cr[0x7] & 0x1) << 8) | ((cr[0x7] & 0x20) << 4)) + 2;
    vdispend    =  cr[0x12] | ((cr[0x7] & 0x2) << 7) | ((cr[0x7] & 0x40) << 3);
    vblankstart =  cr[0x15] | ((cr[0x7] & 0x8) << 5) | ((cr[0x9] & 0x20) << 4);
    vblankend   =  (vblankstart & ~0xFF) | (cr[0x16]); if (cr[0x16] < (vblankstart & 0xFF)) vblankend += (0xFF + 1);
    vsyncstart  =  cr[0x10] | ((cr[0x7] & 0x4) << 6) | ((cr[0x7] & 0x80) << 2);
    vsyncend    =  (vsyncstart & ~0xF) | (cr[0x11] & 0xF);  if ((cr[0x11] & 0xF) < (vsyncstart & 0xF)) vsyncend += (0xF + 1);
    
    vactive     = vdispend + 1;
    vfrontporch = vsyncstart - vdispend;
    vbackporch  = vtotal - vsyncend;
    vsyncwidth  = vsyncend - vsyncstart;
    
    printf(" horizontal total       - %4d chars -> %4d pixels\n", htotal, (htotal << 3));
    printf(" horizontal display end - %4d chars -> %4d pixels\n", hdispend, (hdispend << 3));
    printf(" horizontal blank start - %4d chars -> %4d pixels\n", hblankstart, (hblankstart << 3));
    printf(" horizontal blank end   - %4d chars -> %4d pixels\n", hblankend, (hblankend << 3));
    printf(" horizontal sync  start - %4d chars -> %4d pixels\n", hsyncstart, (hsyncstart << 3));
    printf(" horizontal sync  end   - %4d chars -> %4d pixels\n", hsyncend, (hsyncend << 3));
    printf("\n");
    printf(" horizontal active disp - %4d chars -> %4d pixels\n", hactive, (hactive << 3));
    printf(" horizontal front porch - %4d chars -> %4d pixels\n", hfrontporch, (hfrontporch << 3));
    printf(" horizontal sync width  - %4d chars -> %4d pixels\n", hsyncwidth, (hsyncwidth << 3));
    printf(" horizontal back porch  - %4d chars -> %4d pixels\n", hbackporch, (hbackporch << 3));
    
    printf("\n");
    
    printf(" vertical   total       - %4d scanlines\n", vtotal);
    printf(" vertical   display end - %4d scanlines\n", vdispend);
    printf(" vertical   blank start - %4d scanlines\n", vblankstart);
    printf(" vertical   blank end   - %4d scanlines\n", vblankend);
    printf(" vertical   sync  start - %4d scanlines\n", vsyncstart);
    printf(" vertical   sync  end   - %4d scanlines\n", vsyncend);
    printf("\n");
    printf(" vertical   active disp - %4d scanlines\n", vactive);
    printf(" vertical   front porch - %4d scanlines\n", vfrontporch);
    printf(" vertical   sync width  - %4d scanlines\n", vsyncwidth);
    printf(" vertical   back porch  - %4d scanlines", vbackporch);
    
    return 0;
}