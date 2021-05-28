#ifndef __KERN_TRAP_PLIC_H__
#define __KERN_TRAP_PLIC_H__

#include <memlayout.h>


#ifdef QEMU
#define UART_IRQ    10
#define DISK_IRQ    1
#else
#define UART_IRQ    33
#define DISK_IRQ    27
#endif

void PlicInit(void);
void PlicInitHart(void);
int  Plic_Claim(void);
void Plic_Complete(int irq);

#endif