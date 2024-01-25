#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <conio.h>
#include <i86.h>
#include <dos.h>

#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))

bool allocMode = false;
bool useXms30 = true;
unsigned long memsize = 0;
unsigned long currentFreeMemory = 0, currentLargestBlock = 0;

char handlesFname[128] = "xmsgrab.hnd";
char handlesIdx[256] = "";
bool deallocateHandlesIdx = false;
bool deallocateHandles = false;
bool listInfo = false;

void __far *xmscall = NULL;

// no need for 486+ checks, just try to not call XMS 3.0 functions if we're on 286
// actually they should return error on 286, but who cares?
bool is386() {
    unsigned short _flags;

    _asm {

        // save old flags
        pushf
        // fuck interrupts!
        cli

        // change nested task and IOPL flags
        pushf
        pop     ax
        or      ax, 0xF000
        push    ax
        popf

        // test changes
        pushf
        pop     ax
        mov     [_flags], ax

        // restore old flags
        popf
    }
    return ((_flags & 0xF000) != 0);
}

// xms functions
unsigned short xms_version;
unsigned char xms_status;

struct xms_blockInfo {
    unsigned long size;
    unsigned char lockCount;
};

bool xms_isAvail();
#pragma aux xms_isAvail = \
      " mov     ax,4300h    " \
      " int     2Fh         " \
      value [al] modify [ax]


void __far * xms_getEntrypoint();
#pragma aux xms_getEntrypoint = \
      " push    es          " \
      " mov     ax,4310h    " \
      " int     2Fh         " \
      " mov     dx, es      " \
      " mov     ax, bx      " \
      " pop     es          " \
      value [dx ax] modify [ax bx dx]


unsigned short __declspec(naked) xms_getVersion() {
    _asm {
        push    bx
        push    dx

        mov     ah, 0
        call    [xmscall]

        pop     dx
        pop     bx

        ret
    }
}

unsigned long xms_getFreeMemory(unsigned long* largestBlock = NULL) {
    unsigned short rtn;
    unsigned long mem = 0, largest = 0;
    if (xmscall == NULL) return 0;

    if (useXms30) {
        _asm {
            .386
            mov     ah, 0x88
            call    [xmscall]

            // check for success
            mov     [mem], edx
            mov     [largest], eax
            mov     [xms_status], bl
        }
    } else {
        _asm {
            mov     ah, 0x8
            call    [xmscall]

            // check for success
            mov     word ptr [mem], dx
            mov     word ptr [largest], ax
            mov     [xms_status], bl
        }
    }

    if (largestBlock != NULL) *largestBlock = largest;
    return mem;
}

// -1 if fail
unsigned short xms_blockAlloc(unsigned long kbytes) {
    unsigned short handle, rtn = 1;
    if (xmscall == NULL) return -1;

    if (useXms30) {
        _asm {
            .386
            mov     ah, 0x89
            mov     edx, [kbytes]
            call    [xmscall]

            // check for success
            mov     [rtn], ax
            mov     [handle], dx
            mov     [xms_status], bl
        }
    } else {
        _asm {
            mov     ah, 0x9
            mov     dx, word ptr [kbytes]
            call    [xmscall]

            // check for success
            mov     [rtn], ax
            mov     [handle], dx
            mov     [xms_status], bl
        }
    }

    return (rtn == 1) ? handle : -1;
}

// true is success
bool xms_blockFree(unsigned short handle) {
    unsigned short rtn = 1;
    if (xmscall == NULL) return false;
    _asm {
            mov     ah, 0xA
            mov     dx, [handle]
            call    [xmscall]

            // check for success
            mov     [rtn], ax
            mov     [xms_status], bl
    }
    return (rtn == 1);
}


bool xms_getBlockInfo(unsigned short handle, xms_blockInfo *blockinfo) {

    unsigned short rtn = 1;

    // FUCK WATCOM
    unsigned long blksize = 0;
    unsigned char blklock = 0;


    if (xmscall == NULL) return -1;

    if (useXms30) {
        _asm {
            .386
            mov     ah, 0x8E
            mov     dx, [handle]
            call    [xmscall]

            // check for success
            mov     [rtn], ax
            mov     [blksize], edx
            mov     [blklock], bh
            mov     [xms_status], bl
        }
    } else {
        _asm {
            mov     ah, 0xE
            mov     dx, [handle]
            call    [xmscall]

            // check for success
            mov     [rtn], ax
            mov     word ptr [blksize], dx
            mov     [blklock], bh
            mov     [xms_status], bl
        }
    }

    if (blockinfo != NULL) {
        blockinfo->size = blksize;
        blockinfo->lockCount = blklock;
    }
    return (rtn == 1);
}

unsigned short allocHandle(unsigned long size) {
    printf("allocating %8ld kbytes...", size);
    unsigned short handle = xms_blockAlloc(size);

    if (handle != -1) {
        printf("success, handle = 0x%4X\n", handle);
    } else {
        printf("fail, retcode = 0x%X\n", xms_status);
    }
    return handle;
}

void allocOnce() { allocHandle(memsize);}

void leaveFree() {

    // determine how much memory should we grab
    unsigned long memoryToWaste = (currentFreeMemory < memsize) ? currentFreeMemory : currentFreeMemory - memsize;

    // open handles list
    FILE *f = fopen(handlesFname, "wb");
    if (!f) {
        perror("error: can't open handlers file for writing:");
    }

    unsigned short handle;

    // try grab one large block
    handle = allocHandle(memoryToWaste);
    if (handle != -1) {
        // write to handles list and exit
        fwrite(&handle, sizeof(handle), 1, f);
    } else {
        // grab by maximum allocation unit memory blocks, allow 16 failed attempts
        int timeout = 16;
        while ((memoryToWaste != 0) && (timeout != 0)) {
            // update current largest block size
            xms_getFreeMemory(&currentLargestBlock);
            // try to allocate
            handle = allocHandle((memoryToWaste >= currentLargestBlock ? currentLargestBlock : memoryToWaste));
            if (handle != -1)  {
                fwrite(&handle, sizeof(handle), 1, f);
                memoryToWaste -= (memoryToWaste >= currentLargestBlock ? currentLargestBlock : memoryToWaste);
            } else timeout--;
        }
    }

    fclose(f);
}

void freeHandles(unsigned short *handles, int size) {
    printf("freeing handles...\n");
    for (int i = 0; i < size; i++) {
        printf(" 0x%04X: ", *handles);
        if (xms_blockFree(*handles++)) {
            printf("success\n");
        } else {
            printf("fail, retcode = 0x%X\n", xms_status);
        }
    }
}

void freeFromFile() {
    // load handlers list
    FILE *f = fopen(handlesFname, "rb");
    if (!f) {
        if (errno != ENOENT) perror("error: can't load handlers file");
        return;
    }

    // get file size
    unsigned long fsize;
    fseek(f, 0, SEEK_END);
    fsize = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (fsize >= 65536UL) {
        printf("error: file size must not exceed 64k\n");
        fclose(f);
        return;
    }

    // allocate memory for handles
    unsigned short *handles = new unsigned short[fsize >> 1];
    fread(handles, fsize, 1, f);
    fclose(f);
    remove(handlesFname);

    freeHandles(handles, fsize >> 1);
    delete handles;
}

void freeIdx() {
    unsigned short handles[64]; int handlePtr = 0;

    char* tok = strtok(handlesIdx, ",;:");
    if (!strcmp(tok, "")) {
        printf("warning: no handles to free specified!");
        return;
    }

    while (tok != NULL) {
        sscanf(tok, "%i", &handles[handlePtr++]);
        tok = strtok(NULL, ",;:");
    }

    freeHandles(handles, handlePtr);
}

// parse command line
void parse(int argc, char *argv[]) {
    bool curmode = true;                // rom first
    char tmp[128] = {0};                // dos command line is not that long...

    for (int i = 1; i < argc; i++) {
        strncpy(tmp, argv[i], sizeof(char)*128); strupr(tmp);

        char *tok = strtok(tmp, "/-=");

        if (!strcmp(tok, "A")) {allocMode = true; continue;}
        if (!strcmp(tok, "I")) {listInfo = true; continue;}
        if (!strcmp(tok, "U")) {deallocateHandles = true; continue;}
        if (!strcmp(tok, "L")) {useXms30 = false; continue;}

        // set custom handles file
        if (!strcmp(tok, "FILE")) {
            tok = strtok(NULL, "-=");
            strncpy(handlesFname, tok, sizeof(char)*128);
            continue;
        }

        // set handles indices
        if (!strcmp(tok, "H")) {
            deallocateHandlesIdx = true;
            tok = strtok(NULL, "-=");
            strncpy(handlesIdx, tok, sizeof(char)*256);
            continue;
        }

        // at last - get memsize
        if (!strcmp(tok, "SIZE")) {
            tok = strtok(NULL, "-=");
        }
        {
            unsigned long sz  = 0UL; sscanf(tok, "%ld", &sz);
            unsigned long mul = 1UL;

            if (strstr(tok, "M")) mul = 1024UL;
            memsize = sz*mul;
            continue;
        }

    }
}

void listBlockInfo() {
    // iterate through each possoble handle
    xms_blockInfo blk = {0};
    printf("\n------------------------\nextended memory block info:\n");

    unsigned short i = 0;
    do {
        if (xms_getBlockInfo(i, &blk)) {
            // lock and get physical address



            printf(" 0x%04X: %8ld kbytes, locked %d times\n", i, blk.size, blk.lockCount);
        }
    } while (++i != 0);
}

void help() {

    printf("usage: xmsgrab [memsize] [-a] [-l] [-u] [-i] [-file=] [-h=xxx,xxx,...]\n" \
           "       [memsize] - amount of FREE memory left after run in kbytes\n" \
           "                  (e.g 1 for 1kb, 64 for 64kb, 16M for 16mb)\n" \
           "       -u        - un-allocate previously allocated memory\n" \
           "       -a        - allocate [memsize] kbytes only, don't write to handles list\n" \
           "       -h        - deallocate handle(s) xxx (comma-separated, dec/hex) \n" \
           "       -i        - list info about each allocated XMS handle\n" \
           "       -l        - ignore XMS 3.00 extensions\n" \
           "       -file     - set handles file (default is XMSGRAB.HND)\n" \
           "examples:\n" \
           "    xmsgrab.exe 4M\n" \
           "        - leave 4 MB free extended memory, write handles to XMSGRAB.HND\n" \
           "    xmsgrab.exe -u -l\n" \
           "        - restore default free memory amount, ignore XMS 3.00 extensions\n" \
           "    xmsgrab.exe -u 16M -file=handles.lst\n" \
           "        - restore default free memory amount, then reduce to 16 MB and write\n" \
           "          handles to HANDLES.LST\n" \
           "    xmsgrab.exe -a 3072\n" \
           "        - allocate 3072 KB of memory\n" \
           "    xmsgrab.exe -h=22403,0x3250\n" \
           "        - deallocate handles 22403 and 0x3250 (if possible)\n");
}

int main(int argc, char *argv[]) {

    printf("xmsgrab -- wbcbz7 o8.ob.zozo\n");
    if (argc < 2) { help(); return 1; }
    printf("----------------------------------------------\n");
    parse(argc, argv);

    // detect XMS
    if (xms_isAvail()) {
        // get pointer
        xmscall = xms_getEntrypoint();

        // get version
        xms_version = xms_getVersion();
        printf("XMS ver %d.%02d detected, ", (xms_version >> 8), (xms_version & 0xFF));

    } else {
        printf("error: XMS not found!\n");
        return -1;
    }

    // detect 386
    bool _is386 = is386();
    printf("%s CPU detected\n", (_is386 ? "386+" : "286"));

    // filter out XMS 3.0 calls
    if ((xms_version < 0x300) || (_is386 == false)) useXms30 = false;

    // get free memory
    currentFreeMemory = xms_getFreeMemory(&currentLargestBlock);
    printf("%ld KB free, %ld KB largest block\n", currentFreeMemory, currentLargestBlock);

    // print information and exit
    if (listInfo) {
        listBlockInfo();
        return 0;
    }

    // kill blocks by -h key
    if (deallocateHandlesIdx) freeIdx();

    // free memory if requested
    if (((deallocateHandles) || (memsize != 0)) && (!allocMode)) freeFromFile();

    // request memory
    if (memsize != 0) {
        if (allocMode) {
            allocOnce();
        } else {
            leaveFree();
        }
    }

    currentFreeMemory = xms_getFreeMemory(&currentLargestBlock);
    printf("done - %ld KB free, %ld KB largest block\n", currentFreeMemory, currentLargestBlock);
    return 0;
}
