#include "flexptc.h"
#include <conio.h>

#define X_SIZE 640
#define Y_SIZE 350

#pragma pack(16);

unsigned short buffer15[X_SIZE * Y_SIZE];
unsigned char  buffer8 [X_SIZE * Y_SIZE];

ptc_context mode8, mode15;

int main() {
    int i=0, x, y;
    
    for (y = 0; y < Y_SIZE; y++) {
        for (x = 0; x < X_SIZE; x++) {
            buffer15[i  ] = (((x >> 3) & 0x1F) | (((y >> 3) & 0x1F) << 5)) | ((((x >> 3) ^ (y >> 3)) & 0x1F) << 10);
            buffer8 [i++] = (x ^ y);
        }
    }
    
    if (ptc_init()) return 1;
    // 8bpp test
    if (ptc_open("", X_SIZE, Y_SIZE, 8, ptc_OPEN_FIND60HZ, &mode8)) {ptc_close(); return 1;} 
    if (ptc_open("", X_SIZE, Y_SIZE, 15, ptc_OPEN_FIND60HZ, &mode15)){ptc_close(); return 1;}
    ptc_setcontext(&mode8);
    while (!kbhit()); getch();
    ptc_update(&buffer8);
    while (!kbhit()); getch();
    
    // 15bpp test
    ptc_setcontext(&mode15);
    while (!kbhit()); getch();
    ptc_update(&buffer15);
    while (!kbhit()); getch();
    ptc_close();
    //*/
}