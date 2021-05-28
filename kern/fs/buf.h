#ifndef __KERN_FS_BUF_H__
#define __KERN_FS_BUF_H__

#define BFSIZE 512

#include <sleeplock.h>

struct buf{
    int valid; 
    int disk;
    uint dev;
    uint sectorno;
    struct sleeplock lock;
    uint refcnt;
    struct buf * prev;
    struct buf * next;
    uchar data[BFSIZE];
};

void bfinit(void);
struct buf * bread(uint, uint);
void brelease(struct buf* buffer);
void bwrite(struct buf* buffer);


#endif