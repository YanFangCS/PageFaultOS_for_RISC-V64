#include "libs/printf.h"
#include "sync/sleeplock.h"
#include "fs/buf.hpp"
#include "driver/disk.h"
#include "proc/proc.h"
#include "sync/timer.h"
#include "libs/param.h"
#include "libs/riscv.h"
#include "libs/types.h"
#include "mm/allocop.h"

#define BUFFER_NUM 100

void BufferLayer::init() {
    initlock(&(this->spinlock), (char *)"cachebuffer");
    for (int i = 0; i < BUFFER_NUM; i++) {
        initsleeplock(&(this->bufCache->sleeplock), (char *)"buf");
    }
}


struct buf *BufferLayer::allocBuffer(int dev, int blockno) {
    struct buf *b;
    struct buf *earliest = 0;
    acquire(&(this->spinlock));
    for (b = this->bufCache; b < this->bufCache + BUFFER_NUM; b++) {
        if (b->refcnt == 0 && (earliest == 0 || b->last_use_tick < earliest->last_use_tick)) {
            earliest = b;
        }
        if (b->blockno == blockno) {
            release(&(this->spinlock));
            b->refcnt++;
            b->last_use_tick = ticks;
            acquiresleep(&(b->sleeplock));
            return b;
        }
    }
    release(&(this->spinlock));
    if (earliest == 0) {
        panic((const char *)"alloc buf");
    }
    b = earliest;
    b->valid = 0;
    b->refcnt = 1;
    b->blockno = blockno;
    b->dev = dev;
    b->last_use_tick = ticks;
    acquiresleep(&(b->sleeplock));
    return b;
}

// 释放缓冲区
void BufferLayer::freeBuffer(struct buf *b) {
    acquire(&(this->spinlock));
    b->refcnt--;
    release(&(this->spinlock));
	this->write(b);
	release(&(this->spinlock));
}

// 读取给定块的内容，返回一个包含该内容的buf
struct buf *BufferLayer::read(int dev, int blockno) {
    struct buf *b = allocBuffer(dev, blockno);
    if (!b->valid) {
        disk_read(b);
    }
    b->valid = 1;
    return b;
}

// 将缓冲区写入磁盘
void BufferLayer::write(struct buf *b){
	disk_write(b);
};
