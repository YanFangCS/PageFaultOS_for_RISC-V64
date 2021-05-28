#ifndef __KERN_SYNC_SPINLOCK_H__
#define __KERN_SYNC_SPINLOCK_H__


#include <types.h>

struct cpu;

struct spinlock {    
    uint32  locked;
    char *name;
    struct cpu * cpu;
};

void initlock(struct spinlock*, char *);

void acquire(struct spinlock *);

void release(struct spinlock *);

int holding(struct spinlock *);
#endif