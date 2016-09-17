#include <stdio.h>
#include <math.h>

#define refclk 14318000

unsigned int mclk, pll_n, pll_m, pll_k;
unsigned int outclk;

int main(int argc, char *argv[]) {
    int i, j;
   
    if (argc < 2) {puts("usage: mclkconv.exe <m> <n> <k> or mclkconv.exe <outclk in hex>"); return 1;}
    
    if (argc == 4) {
        sscanf(argv[1], "%d", &pll_m);
        sscanf(argv[2], "%d", &pll_n);
        sscanf(argv[3], "%d", &pll_k);
    
        mclk   = ((pll_n & 0x1F) | ((pll_k & 0x7) << 5) | ((pll_m & 0x7F) << 8));
    } else {
        if (argc == 2) sscanf(argv[1], "%x", &mclk); else {
            sscanf(argv[1], "%x", &j);
            sscanf(argv[2], "%x", &i);
            mclk = (i << 8) + j;
        }
        
        pll_m = (mclk >> 8) & 0x7F;
        pll_n = mclk & 0x1F;
        pll_k = (mclk >> 5) & 0x7;
    }
    
    outclk = (refclk * (pll_m + 2)) / ((pll_n + 2) * pow(2, pll_k));
    printf("m = %i, n = %i, k = %i -> mclk - %8.4f mhz -> 0x%04X -> (0x%02X, 0x%02X)\n", pll_m, pll_n, pll_k, (double)((double)outclk / (1000*1000)), mclk, (mclk & 0xFF), (mclk >> 8));
}