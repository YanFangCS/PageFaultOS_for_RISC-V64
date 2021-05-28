#include <clock.h>
#include <param.h>
#include <riscv.h>
#include <sbi.h>
#include <printf.h>
#include <spinlock.h>
#include <proc.h>


struct spinlock tickslock;
uint ticks;

void timerInit(void){
    initlock(&tickslock, "time");
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