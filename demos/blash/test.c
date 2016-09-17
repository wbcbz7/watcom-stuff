#include <conio.h>
#include "rtctimer.h"

int var = 0;

void myproc() {
    var++;
    outp(0x3C8, 0); outp(0x3C9, var); outp(0x3C9, var); outp(0x3C9, var);
}

int main() {
    rtc_initTimer();
    rtc_setTimer(&myproc, rtc_timerRate / 70);
    
    while (!kbhit()) {
        while ((inp(0x3DA) & 8) != 8) {}
        while ((inp(0x3DA) & 8) == 8) {}
        
    }; getch();
    
    rtc_freeTimer();
    outp(0x3C8, 0); outp(0x3C9, 0); outp(0x3C9, 0); outp(0x3C9, 0);
}