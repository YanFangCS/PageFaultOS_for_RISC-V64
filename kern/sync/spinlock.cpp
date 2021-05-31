#include "sync/spinlock.h"
#include "libs/string.h"
#include "libs/printf.h"
#include "libs/param.h"
#include "trap/intr.h"
#include "proc/proc.h"

void initlock(struct spinlock* lock, const char *name){
    lock->locked = 0;
    strncpy(lock->name, name, strlen(name) + 1);
    lock->cpu = 0;
}

void acquire(struct spinlock *lock){
    push_off();
    if(holding(lock)){
        panic("acquire");
    }

    while(__sync_lock_test_and_set(&lock->locked, 1) != 0);

    __sync_synchronize();

    lock->cpu = curcpu();
}

void release(struct spinlock * lock){
    if(!holding(lock)){
        panic("release");
    }
    
    lock->cpu = 0;
    
    __sync_synchronize();
    __sync_lock_release(&lock->locked);
    
    pop_off();
}

int holding(struct spinlock * lock){
    int r = (lock->locked && lock->cpu == curcpu());
    return r;
}