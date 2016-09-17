#include <i86.h>
#include <string.h>

// some useful DPMI functions
// by wbc\\bz7 zo.oz.zolb

#pragma pack (push, 1)

// dpmi realmode regs structire
typedef struct {
    unsigned long EDI;
    unsigned long ESI;
    unsigned long EBP;
    unsigned long reserved;
    unsigned long EBX;
    unsigned long EDX;
    unsigned long ECX;
    unsigned long EAX;
    unsigned short flags;
    unsigned short ES,DS,FS,GS,IP,CS,SP,SS;
} _dpmi_rmregs;

// dpmi segment:selector pair
typedef struct {
    unsigned short int segment;
    unsigned short int selector;
} _dpmi_ptr;

#pragma pack (pop)

// internal regs/sregs/rmregs structures - do not modify!
static union  REGS  dpmi_regs;
static struct SREGS dpmi_sregs;
static _dpmi_rmregs dpmi_rmregs;

// dpmi status (carry flag)
static unsigned int dpmi_status;

//-----------------------
// low-level DPMI functions
// some texts in comments from PMODEW.TXT

/*
2.11 - Function 0100h - Allocate DOS Memory Block:
--------------------------------------------------

  Allocates low memory through DOS function 48h and allocates it a descriptor.

In:
  AX     = 0100h
  BX     = paragraphs to allocate

Out:
  if successful:
    carry flag clear
    AX	   = real mode segment address
    DX	   = protected mode selector for memory block

  if failed:
    carry flag set
    AX	   = DOS error code
    BX	   = size of largest available block
*/
void dpmi_getdosmem(int size, _dpmi_ptr *p) {
    memset(&dpmi_sregs, 0, sizeof(dpmi_sregs));
    memset(&dpmi_regs, 0, sizeof(dpmi_regs));
    
    dpmi_regs.w.ax = 0x100;
    dpmi_regs.w.bx = (unsigned short)size;
    
    int386x(0x31, &dpmi_regs, &dpmi_regs, &dpmi_sregs);
    
    p->segment = dpmi_regs.w.ax;
    p->selector = dpmi_regs.w.dx;
    
    dpmi_status = dpmi_regs.x.cflag;
}

/*
2.12 - Function 0101h - Free DOS Memory Block:
----------------------------------------------

  Frees a low memory block previously allocated by function 0100h.

In:
  AX     = 0101h
  DX     = protected mode selector for memory block

Out:
  if successful:
    carry flag clear

  if failed:
    carry flag set
    AX	   = DOS error code
*/
void dpmi_freedosmem(_dpmi_ptr *p) {
    memset(&dpmi_sregs, 0, sizeof(dpmi_sregs));
    memset(&dpmi_regs, 0, sizeof(dpmi_regs));
    
    dpmi_regs.w.ax = 0x101;
    dpmi_regs.w.dx = p->selector;
    
    int386x(0x31, &dpmi_regs, &dpmi_regs, &dpmi_sregs);
    
    dpmi_status = dpmi_regs.x.cflag;
}

/*
2.20 - Function 0300h - Simulate Real Mode Interrupt:
-----------------------------------------------------

  Simulates an interrupt in real mode. The function transfers control to the
address specified by the real mode interrupt vector. The real mode handler
must return by executing an IRET.

In:
  AX     = 0300h
  BL     = interrupt number
  BH     = must be 0
  CX     = number of words to copy from the protected mode stack to the real
           mode stack
  ES:EDI = selector:offset of real mode register data structure in the
           following format:

           Offset  Length  Contents
           00h     4       EDI
           04h     4       ESI
           08h     4       EBP
           0ch     4       reserved, ignored
           10h     4       EBX
           14h     4       EDX
           18h     4       ECX
           1ch     4       EAX
           20h     2       CPU status flags
           22h     2       ES
           24h     2       DS
           26h     2       FS
           28h     2       GS
           2ah     2       IP (reserved, ignored)
           2ch     2       CS (reserved, ignored)
           2eh     2       SP
           30h     2       SS

Out:
  if successful:
    carry flag clear
    ES:EDI = selector offset of modified real mode register data structure

  if failed:
    carry flag set

Notes:
) The CS:IP in the real mode register data structure is ignored by this
  function. The appropriate interrupt handler will be called based on the
  value passed in BL.

) If the SS:SP fields in the real mode register data structure are zero, a
  real mode stack will be provided by the host. Otherwise the real mode SS:SP
  will be set to the specified values before the interrupt handler is called.

) The flags specified in the real mode register data structure will be put on
  the real mode interrupt handler's IRET frame. The interrupt handler will be
  called with the interrupt and trace flags clear.

) Values placed in the segment register positions of the data structure must
  be valid for real mode. That is, the values must be paragraph addresses, not
  protected mode selectors.

) The target real mode handler must return with the stack in the same state
  as when it was called. This means that the real mode code may switch stacks
  while it is running, but must return on the same stack that it was called
  on and must return with an IRET.

) When this function returns, the real mode register data structure will
  contain the values that were returned by the real mode interrupt handler.
  The CS:IP and SS:SP values will be unmodified in the data structure.

) It is the caller's responsibility to remove any parameters that were pushed
  on the protected mode stack.
*/
void dpmi_rminterrupt(int int_num, _dpmi_rmregs *regs) {
    memset(&dpmi_sregs, 0, sizeof(dpmi_sregs));
    memset(&dpmi_regs, 0, sizeof(dpmi_regs));
    
    dpmi_regs.w.ax  = 0x300;
    dpmi_regs.w.bx  = (int_num & 0xFF);
    
    dpmi_sregs.es   = FP_SEG(regs);
    dpmi_regs.x.edi = FP_OFF(regs);
    
    int386x(0x31, &dpmi_regs, &dpmi_regs, &dpmi_sregs);
    
    dpmi_status = dpmi_regs.x.cflag;
}

/*
2.32 - Function 0800h - Physical Address Mapping:
-------------------------------------------------

  Converts a physical address into a linear address. This functions allows the
client to access devices mapped at a specific physical memory address.
Examples of this are the frame buffers of certain video cards in extended
memory.

In:
  AX     = 0800h
  BX:CX  = physical address of memory
  SI:DI  = size of region to map in bytes

Out:
  if successful:
    carry flag clear
    BX:CX  = linear address that can be used to access the physical memory

  if failed:
    carry flag set

Notes:
) It is the caller's responsibility to allocate and initialize a descriptor
  for access to the memory.

) Clients should not use this function to access memory below the 1 MB
  boundary.
*/
void *dpmi_mapphysical(unsigned long size, void *p) {
    memset(&dpmi_sregs, 0, sizeof(dpmi_sregs));
    memset(&dpmi_regs, 0, sizeof(dpmi_regs));
    
    dpmi_regs.w.ax  = 0x800;
    
    dpmi_regs.w.bx = (unsigned short)(((unsigned long)p) >> 16);
    dpmi_regs.w.cx = (unsigned short)(((unsigned long)p) & 0xFFFF);
    
    dpmi_regs.w.si = (unsigned short)(((unsigned long)size) >> 16);
    dpmi_regs.w.di = (unsigned short)(((unsigned long)size) & 0xFFFF);
    
    int386x(0x31, &dpmi_regs, &dpmi_regs, &dpmi_sregs);
    
    dpmi_status = dpmi_regs.x.cflag;
    
    return (void *)((dpmi_regs.w.bx << 16) | dpmi_regs.w.cx);
}

/*
2.33 - Function 0801h - Free Physical Address Mapping:
------------------------------------------------------

  Releases a mapping of physical to linear addresses that was previously
obtained with function 0800h.

In:
  AX     = 0801h
  BX:CX  = linear address returned by physical address mapping call

Out:
  if successful:
    carry flag clear

  if failed:
    carry flag set

Notes:
) The client should call this function when it is finished using a device
  previously mapped to linear addresses with function 0801h.
*/  
void dpmi_unmapphysical(void *p) {
    memset(&dpmi_sregs, 0, sizeof(dpmi_sregs));
    memset(&dpmi_regs, 0, sizeof(dpmi_regs));
    
    dpmi_regs.w.ax  = 0x801;
    
    dpmi_regs.w.bx = (unsigned short)(((unsigned long)p) >> 16);
    dpmi_regs.w.cx = (unsigned short)(((unsigned long)p) & 0xFFFF);
    
    int386x(0x31, &dpmi_regs, &dpmi_regs, &dpmi_sregs);
    
    dpmi_status = dpmi_regs.x.cflag;
}

// dpmi realmode interrupt caller using REGS\SREGS structures
// WARNING: be careful with pointing to memory structures because they MUST
// be located in DOS memory area, not in extendend memory!
// (use dpmi_getdosmem() and dpmi_freedosmem())
int rmint386x(int intnum, union REGS *in, union REGS *out, struct SREGS *seg) {
    _dpmi_rmregs rmregs;
    union REGS   inregs;
    struct SREGS sregs;
    
    memcpy(&inregs, in, sizeof(union REGS));
    memcpy(&sregs, seg, sizeof(struct SREGS));
    
    // translate regs from REGS to rmregs
    rmregs.EAX = inregs.x.eax;
    rmregs.EBX = inregs.x.ebx;
    rmregs.ECX = inregs.x.ecx;
    rmregs.EDX = inregs.x.edx;
    rmregs.ESI = inregs.x.esi;
    rmregs.EDI = inregs.x.edi;
    // translate regs from SREGS to rmregs
    rmregs.ES  = sregs.es;
    rmregs.DS  = sregs.ds;
    rmregs.FS  = sregs.fs;
    rmregs.GS  = sregs.gs;
    // call dmpi_rminterrupt()
    dpmi_rminterrupt(intnum, &rmregs);
    // translate regs back
    sregs.es  = rmregs.ES;
    sregs.ds  = rmregs.DS;
    sregs.fs  = rmregs.FS;
    sregs.gs  = rmregs.GS;
    inregs.x.eax = rmregs.EAX;
    inregs.x.ebx = rmregs.EBX;
    inregs.x.ecx = rmregs.ECX;
    inregs.x.edx = rmregs.EDX;
    inregs.x.esi = rmregs.ESI;
    inregs.x.edi = rmregs.EDI;
    
    memcpy(out, &inregs, sizeof(union REGS));
    memcpy(seg, &sregs,  sizeof(struct SREGS));
    
    return rmregs.EAX;
}

void rmintr(int intnum, union REGPACK *r) {
    _dpmi_rmregs  rmregs;
    union REGPACK regs;

    memcpy(&regs, r, sizeof(union REGPACK));
    
    // translate regs from REGS to rmregs
    rmregs.EAX = regs.x.eax;
    rmregs.EBX = regs.x.ebx;
    rmregs.ECX = regs.x.ecx;
    rmregs.EDX = regs.x.edx;
    rmregs.ESI = regs.x.esi;
    rmregs.EDI = regs.x.edi;
    rmregs.EBP = regs.x.ebp;
    // translate regs from SREGS to rmregs
    rmregs.ES  = regs.x.es;
    rmregs.DS  = regs.x.ds;
    rmregs.FS  = regs.x.fs;
    rmregs.GS  = regs.x.gs;
    // call dmpi_rminterrupt()
    dpmi_rminterrupt(intnum, &rmregs);
    // translate regs back
    regs.x.es  = rmregs.ES;
    regs.x.ds  = rmregs.DS;
    regs.x.fs  = rmregs.FS;
    regs.x.gs  = rmregs.GS;
    regs.x.eax = rmregs.EAX;
    regs.x.ebx = rmregs.EBX;
    regs.x.ecx = rmregs.ECX;
    regs.x.edx = rmregs.EDX;
    regs.x.esi = rmregs.ESI;
    regs.x.edi = rmregs.EDI;
    regs.x.ebp = rmregs.EBP;
    
    memcpy(r, &regs, sizeof(union REGPACK));
}
