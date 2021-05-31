#include "sync/timer.h"
#include "libs/param.h"
#include "libs/riscv.h"
#include "libs/sbi.h"
#include "libs/printf.h"
#include "sync/spinlock.h"
#include "proc/proc.h"


struct spinlock tickslock;
uint ticks;

void timerInit(void){
    initlock(&tickslock, (char *)"time");
}

void set_next_timeout(void){
    sbi_set_timer(r_time() + INTERVAL);
}

void timer_tick(void){
    acquire(&tickslock);
    ticks++;
    wakeup(&ticks);
    release(&tickslock);
    set_next_timeout();
}