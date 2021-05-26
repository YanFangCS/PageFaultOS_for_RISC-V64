#include <sbi.h>
#include <types.h>
#include <memlayout.h>
#include <spinlock.h>
#include <trap.h>
#include <sbi.h>
#include <riscv.h>
#include <proc.h>

extern char trampoline[], uservec[], userret[];

extern void kernvec();

void TrapInitHart(void){
    w_stvec((uint64)kernvec);
    w_sstatus(r_sstatus() | SSTATUS_SIE);
    w_sie(r_sie() | SIE_SEIE| SIE_SSIE | SIE_STIE);
    set_next_timeout();
}

void UserTrapRet(void){
    struct proc *p = myproc();

    intr_off();

    w_stvec(TRAMPOLINE + (uservec - trampoline));

    p->trapframe->kernel_satp = r_satp();
    p->trapframe->kernel_sp = p->stack
}