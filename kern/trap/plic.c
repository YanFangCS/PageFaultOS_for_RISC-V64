#include <types.h>
#include <param.h>
#include <memlayout.h>
#include <riscv.h>
#include <sbi.h>
#include <plic.h>
#include <proc.h>
#include <printf.h>


void PlicInit(void){
    writed(1, PLIC_V + DISK_IRQ * sizeof(uint32));
    writed(1, PLIC_V + UART_IRQ * sizeof(uint32));

    #ifdef DEBUG
    printf("plicinit\n");
    #endif
}

void PlicInitHart(void){
    int hart = cpuid();
    #ifdef QEMU
    *(uint32*)PLIC_SENABLE(hart)= (1 << UART_IRQ) | (1 << DISK_IRQ);
    // set this hart's S-mode priority threshold to 0.
    *(uint32*)PLIC_SPRIORITY(hart) = 0;
    #else
    uint32 * hart_m_enable = (uint32 *)PLIC_MENABLE(hart);
    *(hart_m_enable) = readd(hart_m_enable) | (1 << DISK_IRQ);
    uint32 * hart0_m_int_enable_hi = hart_m_enable + 1;
    *(hart0_m_int_enable_hi) = readd(hart0_m_int_enable_hi) | (1 << (UART_IRQ % 32));
    #endif
    #ifdef  DEBUG
    printf("PlicInitHart\n");
    #endif
}

int Plic_Claim(void){
    int hart = cpuid();
    int irq;
    #ifndef QEMU
    irq = *(uint32 *)PLIC_MCLAIM(hart);
    #else
    irq = *(uint32 *)PLIC_SCLAIM(hart);
    #endif
    return irq;
}


void Plic_Complete(int irq){
    int hart = cpuid();

    #ifndef QEMU
    *(uint32 *)PLIC_MCLAIM(hart) = irq;
    #else
    *(uint32 *)PLIC_SCLAIM(hart) = irq;
    #endif;
}

