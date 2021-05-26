#ifndef __KERN_SYNC_SLEEPLOCK_H__
#define __KERN_SYNC_SLEEPLOCK_H__

#include <types.h>
#include <spinlock.h>

struct spinlock;

struct sleeplock{
    uint locked;
    struct spinlock lk;

    char *name;
    int pid;
};

void acquiresleep(struct sleeplock* slk);
void releasesleep(struct sleeplock* slk);
int  holdingsleep(struct sleeplock* slk);
void initsleeplock(struct sleeplock* slk, char *name);

#endif