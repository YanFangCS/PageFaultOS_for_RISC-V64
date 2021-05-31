#ifndef __KERN_SYNC_TIMER_H__
#define __KERN_SYNC_TIMER_H__


#include "libs/types.h"
#include "sync/spinlock.h"

extern struct spinlock tickslock;
extern uint ticks;


void timerInit(void);
void set_next_timeout(void);
void timer_tick(void);


#endif