#include <spinlock.h>
#include <string.h>
#include <param.h>

void initlock(struct spinlock* lock, char *name){
    lock->locked = 0;
    strcpy(lock->name, name);
    lock->cpu = 0;
}

void acquire(struct spinlock *lock){
    push_off();
    if(holding(lock)){
        panic("acquire");
    }

    while(__sync_lock_test_and_set(&lock->locked, 1) != 0);

    __sync_synchornize();

    lock->cpu = curcpu();
}

void release(struct spinlock * lock){
    if(!holding(lock)){
        panic("release");
    }
    
    lock->cpu = 0;
    
    __sync_synchornize();
    __sync_lock_release(&lock->locked);
    
    pop_off();
}

int holding(struct spinlock * lock){
    int r = (lock->locked && lock->cpu == curcpu());
    return r;
}