#include <i86.h>
#include <dos.h>
#include <math.h>
#include <strings.h>
#include <stdlib.h>
#include <conio.h> 

const divisor = 8192 / 70;

void (__interrupt __far *oldrtchandler)();
void (*callnow)();

int dummy;

volatile int downcount;
void __interrupt __far rtchandler() {
    downcount--;
    if (downcount == 0) { 
        (*callnow)();
        downcount = divisor;
    } else {
        outp(0x3C8, 0); outp(0x3C9, 0); outp(0x3C9, 0); outp(0x3C9, 0);
    }
    outp(0x70, 0xC);  dummy = inp(0x71); // clear interrupt flags in RTC
    outp(0xA0, 0x20); outp(0x20, 0x20);  // and send EOI to interrupt controller
}

void timerProc() {  // called if counter was triggered
    outp(0x3C8, 0); outp(0x3C9, 63); outp(0x3C9, 63); outp(0x3C9, 63);
}

void initTimer() {
    outp(0xA1, (inp(0xA1) & 0xFE));                           // unmask IRQ8
    outp(0x70, 0x8A); outp(0x71, ((inp(0x71) & 0xF0) | 0x3)); // disable NMI and select 8192 hz rate
    outp(0x70, 0x8B); outp(0x71,  (inp(0x71) | 0x40));        // enable periodic interrupt
    outp(0x70, 0xD);  dummy = inp(0x71);                      // enable NMI
}

void freeTimer() {
    // we will not mask IRQ8 because we will return control to BIOS interrupt handler
    outp(0x70, 0x8A); outp(0x71, ((inp(0x71) & 0xF0) | 0x6)); // disable NMI and select 1024 hz rate
    outp(0x70, 0x8B); outp(0x71,  (inp(0x71) & 0xBF));        // disable periodic interrupt
    outp(0x70, 0xD);  dummy = inp(0x71);                      // enable NMI
}

int main() {
    _disable();
    oldrtchandler = _dos_getvect(0x70);
    initTimer();
    callnow = timerProc;
    _dos_setvect(0x70, rtchandler); downcount = divisor;
    _enable();
    
    while (!kbhit()) {}; getch();
    
    _disable();
    freeTimer();
    _dos_setvect(0x70, oldrtchandler);
    _enable();
}
