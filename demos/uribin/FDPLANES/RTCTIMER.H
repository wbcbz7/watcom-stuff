#include <i86.h>
#include <dos.h>
#include <stdlib.h>
#include <conio.h> 
// RTC timer procedures v.1.01
// by wbc\\bz7 12.11.2o15 - o7.12.2o15

// some guidelines :)
// - timer can run at freqs from 2 to 8192 hz (look at rtc_timeRate and 
//   rtc_clockDivisor defines)
// - your timer proc should avoid any calls to some stdlib functions and
//   bios and dos interrupts bcoz it can lead to crash
// - works in real and protected mode under pure dos
// - works under windows (at least under mustdie'98 :) but 8192hz isn't stable

// comversion table:
// rtc_timerRate | rtc_clockDivisor
//     1024      |        6
//     2048      |        5
//     4096      |        4
//     8192      |        3
// I guess nobody will use freqs below 1024hz ;), but if you want to use they
// look at this formula:
//
// rtc_timerRate = (32768 >> (rtc_clockDivisor - 1)); rtc_timerRate <= 8192 (!)

// changelog:
// v.1.01  - added manual divisor selection in rtc_InitTimer()
// v.1.00  - initial release (used in blash\bz7)

int rtc_timerRate    = 1024;  // default
int rtc_clockDivisor = 6;

void (*rtc_timerProc)() = NULL; // timer procedure
void (__interrupt __far *rtc_oldhandler)(); // internal procedure - old INT70h handler

int rtc_divisor;

volatile int rtc_downcount;
void __interrupt __far rtc_handler() {
    rtc_downcount--;
    if (rtc_downcount == 0) { 
        if (rtc_timerProc != NULL) (*rtc_timerProc)();
        rtc_downcount = rtc_divisor;
    }
    outp(0x70, 0xC);  inp(0x71); // clear interrupt flags in RTC
    outp(0xA0, 0x20); outp(0x20, 0x20);  // and send EOI to interrupt controller
}

// call it before any rtc_setTimer() calls
void rtc_initTimer(int divisor) {
    rtc_clockDivisor = divisor & 0xF; 
    rtc_timerRate = (32768 >> (rtc_clockDivisor - 1));
    
    _disable();
    rtc_oldhandler = _dos_getvect(0x70);
    _dos_setvect(0x70, rtc_handler);
    outp(0xA1, (inp(0xA1) & 0xFE));                                        // unmask IRQ8
    outp(0x70, 0x8A); outp(0x71, ((inp(0x71) & 0xF0) | rtc_clockDivisor)); // disable NMI and select rate
    outp(0x70, 0x8B); outp(0x71,  (inp(0x71) | 0x40));                     // enable periodic interrupt
    outp(0x70, 0xD);  inp(0x71);                                           // enable NMI
    rtc_divisor = 0x7FFFFFFF; rtc_downcount = rtc_divisor;
    _enable();
}

// call it before exit or you will get...guess that? :)
void rtc_freeTimer() {
    _disable();
    _dos_setvect(0x70, rtc_oldhandler);
    // we will not mask IRQ8 because we will return control to BIOS interrupt handler
    outp(0x70, 0x8A); outp(0x71, ((inp(0x71) & 0xF0) | 0x6)); // disable NMI and select 1024 hz rate
    outp(0x70, 0x8B); outp(0x71,  (inp(0x71) & 0xBF));        // disable periodic interrupt
    outp(0x70, 0xD);  inp(0x71);                              // enable NMI
    _enable();
}

// replaces current timer
void rtc_setTimer(void *func, int divisor) {
    _disable();
    rtc_timerProc = func;
    rtc_divisor = divisor; rtc_downcount = divisor;
    _enable();
}

