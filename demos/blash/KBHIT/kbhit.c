#include <conio.h>

char a[12];

int main() {
    while (!kbhit()) {
        itoa(getch(), a, 16);
        puts(a);
    }
}
