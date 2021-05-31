#include "libs/sbi.h"
#include "libs/types.h"
#include "mm/memlayout.h"
#include "sync/spinlock.h"
#include "trap/trap.h"
#include "libs/sbi.h"
#include "libs/riscv.h"
#include "proc/proc.h"
#include "trap/plic.h"
#include "sync/timer.h"
#include "syscall/syscall.h"
#include "mm/vm.h"
#include "fs/vfs.h"
#include "mm/kalloc.h"
#include "console/console.h"
#include "driver/disk.h"

extern char trampoline[], uservec[], userret[];

extern void kernvec();

void TrapInitHart(void){
    w_stvec((uint64)kernvec);
    w_sstatus(r_sstatus() | SSTATUS_SIE);
    w_sie(r_sie() | SIE_SEIE| SIE_SSIE | SIE_STIE);
    set_next_timeout();
}

void UserTrap(void){
    int which_dev = 0;
    
    if((r_sstatus() & SSTATUS_SPP) != 0){
        panic("UserTrap: not from user mode");
    }

    w_stvec((uint64)kernvec);

    struct proc *p = curproc();

    p->trapframe->epc = r_sepc();

    if(r_scause() == 8){
        if(p->killed) exit(-1);

        p->trapframe->epc += 4;

        intr_on();
        syscall();
    }
    else if(r_scause() == 13 || r_scause() == 5){
        uint64 eaddr;
        struct vma * vma = 0;
        char *mem;
        eaddr = r_stval();
        for (int i = 0; i < NOMMAPFILE; i++) {
            if (p->vma[i] != 0 && eaddr >= p->vma[i]->addr && eaddr < p->vma[i]->addr + p->vma[i]->length) {
                vma = p->vma[i];
                break;
            }
        }
        if(vma != 0){
            mem = (char *)kalloc();
            memset(mem, 0, PGSIZE);
            eaddr = PGROUNDDOWN(eaddr);
            int perm = PTE_V | PTE_U;
            if (vma->prot & PROT_READ) perm |= PTE_R;
            if (vma->prot & PROT_WRITE) perm |= PTE_W;
            MapPages(p->pagetable, eaddr, (uint64)mem, PGSIZE, perm);
            vfsread(vma->f, false, mem, PGSIZE, eaddr - vma->addr);
        }
        else {
            p->killed = 1;
        }
    } else if ((which_dev = DevIntr()) != 0){

    } else {
        printf("\nUserTrap(): unexpected scause %p pid=%d %s\n", r_scause(), p->pid, p->name);
        printf("    sepc=%p stval=%p\n", r_sepc(), r_stval());
        p->killed = 1;
    }


    if(p->killed) exit(-1);
    if(which_dev == 2) yield();

    UserTrapRet();
}


void UserTrapRet(void){
    struct proc *p = curproc();

    intr_off();

    w_stvec(TRAMPOLINE + (uservec - trampoline));

    p->trapframe->kernel_satp = r_satp();
    p->trapframe->kernel_sp = p->kstack + PGSIZE;
    p->trapframe->kernel_trap = (uint64)UserTrap;
    p->trapframe->kernel_hartid = r_tp();


    unsigned long x = r_sstatus();
    x &= ~SSTATUS_SPP;
    x |= SSTATUS_SPIE;
    w_sstatus(x);

    w_sepc(p->trapframe->epc);

    uint64 satp = MAKE_SATP(p->pagetable);

    uint64 fn = TRAMPOLINE + ((uint64)userret - (uint64)trampoline);
    ((void (*)(uint64, uint64))fn)(TRAPFRAME, satp);
}


void KernelTrap(){
    int which_dev = 0;
    uint64 sepc = r_sepc();
    uint64 sstatus = r_sstatus();
    uint64 scause = r_scause();

    if((sstatus & SSTATUS_SPP) == 0){
        panic("kerneltrap : not from supervisor mode");
    }
    if(intr_get() != 0){
        panic("kerneltrap : interrupts enabled");
    }
    
    if((which_dev = DevIntr()) == 0){
        printf("\nscause %p\n", scause);
        printf("sepc=%p stval=%p hart=%d\n", r_sepc(), r_stval(), r_tp());
        struct proc *p = curproc();
        if (p != 0) {
            printf("pid: %d, name: %s\n", p->pid, p->name);
        }
        panic("kerneltrap");
    }

    if(which_dev == 2 && curcpu() != 0 && curproc()->state == RUNNING){
        yield();
    }

    w_sepc(sepc);
    w_sstatus(sstatus);
}



int DevIntr(void){
    uint64 scause = r_scause();

    #ifdef QEMU
    if ((0x8000000000000000L & scause) && 9 == (scause & 0xff)) 
    #else
    if (0x8000000000000001L == scause && 9 == r_stval()) 
    #endif
    {
        int irq = Plic_Claim();
        if(UART_IRQ == irq){
            int c = sbi_console_getchar();
            if(c != -1){
                consoleintr(c);
            }
        }
        else if(DISK_IRQ == irq){
            disk_intr();
        }
        else if(irq){
            printf("unexpected interrupt irq = %d\n", irq);
        }

        if(irq) {
            Plic_Complete(irq);
        }

        #ifdef QEMU
        w_sip(r_sip() & ~2);
        sbi_set_mie();
        #endif

        return 1;
    }
    else if (0x8000000000000005L == scause){
        timer_tick();
        return 2;
    }
    else {
        return 0;
    }
}

void trapframeDump(struct trapframe *tf){
    printf("a0: %p\t", tf->a0);
    printf("a1: %p\t", tf->a1);
    printf("a2: %p\t", tf->a2);
    printf("a3: %p\n", tf->a3);
    printf("a4: %p\t", tf->a4);
    printf("a5: %p\t", tf->a5);
    printf("a6: %p\t", tf->a6);
    printf("a7: %p\n", tf->a7);
    printf("t0: %p\t", tf->t0);
    printf("t1: %p\t", tf->t1);
    printf("t2: %p\t", tf->t2);
    printf("t3: %p\n", tf->t3);
    printf("t4: %p\t", tf->t4);
    printf("t5: %p\t", tf->t5);
    printf("t6: %p\t", tf->t6);
    printf("s0: %p\n", tf->s0);
    printf("s1: %p\t", tf->s1);
    printf("s2: %p\t", tf->s2);
    printf("s3: %p\t", tf->s3);
    printf("s4: %p\n", tf->s4);
    printf("s5: %p\t", tf->s5);
    printf("s6: %p\t", tf->s6);
    printf("s7: %p\t", tf->s7);
    printf("s8: %p\n", tf->s8);
    printf("s9: %p\t", tf->s9);
    printf("s10: %p\t", tf->s10);
    printf("s11: %p\t", tf->s11);
    printf("ra: %p\n", tf->ra);
    printf("sp: %p\t", tf->sp);
    printf("gp: %p\t", tf->gp);
    printf("tp: %p\t", tf->tp);
    printf("epc: %p\n", tf->epc);
}