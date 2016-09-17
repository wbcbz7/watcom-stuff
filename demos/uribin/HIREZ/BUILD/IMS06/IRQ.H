#ifndef __IRQ_H
#define __IRQ_H

int irqInit(int irq, void (*rout)(), int pre, int stk);
void irqClose();
void irqReInit();

#endif
