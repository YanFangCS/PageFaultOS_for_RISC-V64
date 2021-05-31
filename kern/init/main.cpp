#include "libs/types.h"
#include "libs/param.h"
#include "mm/memlayout.h"
#include "libs/riscv.h"
#include "libs/sbi.h"
#include "console/console.h"
#include "libs/printf.h"
#include "mm/kalloc.h"
#include "driver/clock.h"
#include "trap/plic.h"
#include "mm/vm.h"
#include "driver/disk.h"
#include "fs/buf.hpp"
#include "proc/proc.h"
#include "sync/timer.h"
#ifndef QEMU
#include "driver/sdcard.h"
#include "driver/fpioa.h"
#include "driver/dmac.h"
#endif

struct cpu cpus[2];
struct console console;
BufferLayer bufferlayer;

static inline void inithartid(unsigned long hartid){ 
    asm volatile("mv tp, %0" : : "r"(hartid & 0x1));
}

volatile static int started = 0;


void main(unsigned long hartid, unsigned long dtb_pa){
    inithartid(hartid);

    if (hartid == 0) {
        consoleinit();
        printfinit();   // init a lock for printf 
        // print_logo();
        #ifdef DEBUG
        printf("hart %d enter main()...\n", hartid);
        #endif
        kinit();         // physical page allocator
        kvmInit();       // create kernel page table
        kvmInitHart();   // turn on paging
        timerInit();     // init a lock for timer
        TrapInitHart();  // install kernel trap vector, including interrupt handler
        procinit();
        PlicInit();
        PlicInitHart();
#ifdef K210
        clockInit();
        fpioa_pin_init();
        dmac_init();
#endif 
        disk_init();
        bufferlayer.init();
        initProcTable();
        initFirstProc();
        // fileinit();      // file table
        // userinit();      // first user process
    
        for(int i = 1; i < NCPU; i++) {
            unsigned long mask = 1 << i;
            sbi_send_ipi(&mask);
        }
        printf("hart %d finished init\n", r_tp());
        __sync_synchronize();
        started = 1;
    }
    else
    {
        intr_off();
        // hart 1
        while (started == 0)
            ;
        __sync_synchronize();
        #ifdef DEBUG
        printf("hart %d enter main()...\n", hartid);
        #endif
        kvmInitHart();
        TrapInitHart();
        PlicInitHart();  // ask PLIC for device interrupts
        printf("hart 1 init done\n");
        while(1) {
            
        }
    }
    scheduler();
}