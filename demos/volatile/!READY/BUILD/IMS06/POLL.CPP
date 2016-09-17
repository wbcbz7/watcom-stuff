#include "timer.h"

int pollInit(void (*proc)())
{
  tmInit(proc, 17100, 8192);
  return 1;
}

void pollClose()
{
  tmClose();
}
