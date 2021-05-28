#include <sleeplock.h>
#include <riscv.h>
#include <proc.h>

void initsleeplock(struct sleeplock * slk, char *name){
    initlock(&slk->lk, "sleep lock");
    slk->name = name;
    slk->locked = 0;
    slk->pid = 0;
}


void acquiresleep(struct sleeplock * slk){
    acquire(&slk->lk);
    while(slk->locked){
        sleep(slk, &slk->lk);
    }
    slk->locked = 1;
    slk->pid = curproc()->pid;
    release(&slk->lk);
}


void releasesleep(struct sleeplock * slk){
    acquire(&slk->lk);
    slk->locked = 0;
    slk->pid = 0;
    wakeup(slk);
    release(&slk->lk);
}


int holdingsleep(struct sleeplock * slk){
    int r;
    acquire(&slk->lk);
    r = slk->locked && (slk->pid == curproc()->pid);
    release(&slk->lk);
    return r;
}