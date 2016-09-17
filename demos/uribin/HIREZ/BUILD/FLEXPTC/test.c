#include "flexptc.c"

#define X_SIZE 640
#define Y_SIZE 350

#pragma pack(16);

unsigned long buffer32[X_SIZE * Y_SIZE];
unsigned char buffer8 [X_SIZE * Y_SIZE];

int main() {
    int i=0, x, y;
    
    for (y = 0; y < Y_SIZE; y++) {
        for (x = 0; x < X_SIZE; x++) {
            buffer32[i  ] = ((x & 0xFF) + ((y & 0xFF) << 8)) + (((x ^ y) & 0xFF) << 16);
            buffer8 [i++] = (x ^ y);
        }
    }
    /*
    // 8bpp test
    if (ptc_open("", X_SIZE, Y_SIZE, 8, 0)) return 0; 
    while (!kbhit()); getch();
    ptc_update(&buffer8);
    
    while (!kbhit()); getch();
    ptc_close();
    
    */
    // 32bpp test
    if (ptc_open("", X_SIZE, Y_SIZE, 32, 0)) return 0; 
    while (!kbhit()); getch();
    ptc_update(&buffer32);
    
    while (!kbhit()); getch();
    ptc_close();
    //*/
}