// AMD Interwave local ROM\DRAM dumper - by wbcbz7 o1.o3.2o15
// outputs IWDRAM.BIN and IWROM.BIN files (16 MB each, full dump)

#include <stdio.h>
#include <conio.h>
#include <dos.h>
#include "iwdump.h"

FILE          *f;
int           iw_baseport;
unsigned char buffer[BUF_SIZE];

void iwread(unsigned long pos, void *data, unsigned long length, int rom) {
    int i;
    unsigned char *buf = (unsigned char*)data;
     
    rom = (rom & 1) << 1;
    
    // set dram\rom and enable autoincrement
    outp (iw_baseport+IW_PIDX,   IW_LMCI);
    outp (iw_baseport+IW_PDATAH, ((inp(iw_baseport+IW_PDATAH) & 0xFB) | rom | 1));
    
    outp (iw_baseport+IW_PIDX,   IW_LMALI);
    outpw(iw_baseport+IW_PDATAL, (pos & 0xFFFF));
    outp (iw_baseport+IW_PIDX,   IW_LMALH);
    outp (iw_baseport+IW_PDATAH, (pos >> 16));
    
    for (i = 0; i < length; i++) {
        buf[i] = inp(iw_baseport+IW_LMBDR);
        //buf[i] = (i & 0xFF); // debug
    }
    
    // set dram as source
    outp (iw_baseport+IW_PIDX,   IW_LMCI);
    outp (iw_baseport+IW_PDATAH, (inp(iw_baseport+IW_PDATAH) & 0xFD));
}

// return 0 if ok
int iwinit() {
    int i;
    char a;

    // reset card
    outp (iw_baseport+IW_PIDX,   0x4C);
    outp (iw_baseport+IW_PDATAH, 0);
    for (i = 0; i < 40; i++) {
        inp(iw_baseport+IW_LMBDR);
    }
    outp (iw_baseport+IW_PIDX,   0x4C);
    outp (iw_baseport+IW_PDATAH, 1);
    for (i = 0; i < 40; i++) {
        inp(iw_baseport+IW_LMBDR);
    }
    
    // try to set enhanced mode
    outp (iw_baseport+IW_PIDX,   0x99);
    a = inp(iw_baseport+IW_PDATAH);
    outp (iw_baseport+IW_PIDX,   0x19);
    outp (iw_baseport+IW_PDATAH, (a | 1));
    
    outp (iw_baseport+IW_PIDX,   0x99);
    a = inp(iw_baseport+IW_PDATAH) & 1;
    return (a == 0 ? 1 : 0);
}

int iwgetport() {
    char *env = getenv("ULTRASND");
    int  port, dummy;
    
    if (!env) return 0;
    sscanf(env, "%x,%d,%d,%d,%d", &port, &dummy, &dummy, &dummy, &dummy);
    return port;
}

int main() {
    int i, j, pos = 0;
    
    puts("AMD Interwave local ROM\\DRAM dumper - by wbcbz7 01.03.2016");
    iw_baseport = iwgetport();
    if (iw_baseport == 0) {puts("error: can't get Interwave base port - check ULTRASND environment varable"); exit(1);}
    
    if (!iwinit()) {puts("error: can't init Interwave chip"); exit(1);}
    
    cputs("dumping ROM content, please wait");
    f = fopen(ROM_FILE, "wb");
    for (i = 0; i < (TOTAL_MEM / BUF_SIZE); i++) {
        iwread(pos, &buffer, BUF_SIZE, 1);
        fwrite(&buffer, sizeof(unsigned char), BUF_SIZE, f);
        pos += BUF_SIZE;
        if (i & 0xF) cputs(".");
    }
    fclose(f);
    puts("");
    
    pos = 0;
    cputs("dumping DRAM content, please wait");
    f = fopen(RAM_FILE, "wb");
    for (i = 0; i < (TOTAL_MEM / BUF_SIZE); i++) {
        iwread(pos, &buffer, BUF_SIZE, 0);
        fwrite(&buffer, sizeof(unsigned char), BUF_SIZE, f);
        pos += BUF_SIZE;
        if (i & 0xF) cputs(".");
    }
    fclose(f);
    
    puts("\ndone");
}
