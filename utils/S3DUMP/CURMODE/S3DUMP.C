#include <conio.h>
#include <stdio.h>
//#include "vbe.c"

unsigned char cr[256], sr[256];
FILE *f;

//vbe_VbeInfoBlock  vbeinfo;
//vbe_ModeInfoBlock modeinfo;

void main(int argc, char* argv[]) {
    int i, j, k, mode, rawflags;
    char *modestr;
    
    //if (vbe_Init()) {puts("can't init vbe interface \n"); return;}
    //vbe_ControllerInfo(&vbeinfo);
    
    //puts("dumping CRTC\\sequencer regs, press any key to continue..."); while (!kbhit()) {} getch();
    
    // hehe some of this code directly ripped from vbe.c :)
    
    //for (k = 0; ((vbeinfo.VideoModePtr[k] != 0xFFFF) && (vbeinfo.VideoModePtr[k] != 0)); k++) {
        sound(800); delay(5); nosound();
        
        //vbe_ModeInfo(vbeinfo.VideoModePtr[k], &modeinfo);
        //rawflags = vbe_ATTR_HardwareMode;
        //if ((modeinfo.ModeAttributes & rawflags) != rawflags) continue;
        //mode = vbeinfo.VideoModePtr[k];
        
        //vbe_SetMode(mode); if (vbe_status != 0x4F) continue;
        //sprintf(modestr, "%3x.bin", mode);
        
        //f = fopen(modestr, "wb");
        
        f = fopen(argv[1], "wb");

        // unlock S3 extensions
        outpw(0x3C4, 0x0608);
        outpw(0x3D4, 0x4838);
        outpw(0x3D4, 0xA039);
        
        for (i = 0; i < 256; i++) {
           outp(0x3d4, i); cr[i] = inp(0x3d5);
           outp(0x3c4, i); sr[i] = inp(0x3c5);
        }
        
        // lock S3 extensions
        outpw(0x3C4, 0x0008);
        outpw(0x3D4, 0x0038);
        outpw(0x3D4, 0x0039);
        
        fwrite(&cr, sizeof(cr), 1, f);
        fwrite(&sr, sizeof(sr), 1, f);
        
        fclose(f);
    //}
    
    for (i = 0; i < 30; i++) {sound(20*i); while ((inp(0x3DA) & 8) != 8); while ((inp(0x3DA) & 8) == 8);}
    nosound();
    
    //vbe_SetMode(0x3);
    puts("done");
    //vbe_Done();
}
