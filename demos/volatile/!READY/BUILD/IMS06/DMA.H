#ifndef __IRQDMA_H
#define __IRQDMA_H

void dmaStart(int ch, void *buf, int len, int ai);
void dmaStop();
int dmaGetBufPos();
void* dmaAlloc(int &len, __segment &pmsel);
void dmaFree(__segment pmsel);

#endif
