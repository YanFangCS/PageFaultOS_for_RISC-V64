#ifndef __KERN_SYNC_CLOCK_H__
#define __KERN_SYNC_CLOCK_H__


#include <types.h>
#include <spinlock.h>

extern struct spinlock tickslock;
extern uint ticks;


void clockInit(void);
void set_next_timeout(void);
void timer_tick(void);


#endif