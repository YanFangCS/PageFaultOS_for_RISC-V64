#include <types.h>
#include <param.h>
#include <spinlock.h>
#include <sleeplock.h>
#include <riscv.h>
#include <buf.h>
#include <sdcard.h>
#include <printf.h>
#include <disk.h>

struct {
    struct spinlock lock;
    struct buf buf[NBUF];
    struct buf head;
} bcache;

void binit(void){
    struct buf *b;

    initlock(&bcache.lock, "bcache");

    bcache.head.prev = &bcache.head;
    bcache.head.next = &bcache.head;

    for(b = bcache.buf; b < bcache.buf + NBUF; ++b){
        b->refcnt = 0;
        b->sectorno = ~0;
        b->dev = ~0;
        b->next = bcache.head.next;
        b->prev = &bcache.head;
        initsleeplock(&b->lock, "buffer");
        bcache.head.next->prev = b;
        bcache.head.next = b;
    }
}

static struct buf * bget(uint dev, uint sectorno){
    struct buf *b;

    acquire(&bcache.lock);

    for(b = bcache.head.next; b != &bcache.head; b=b->next){
        if(b->dev == dev && b->sectorno == sectorno){
            b->refcnt++;
            release(&bcache.lock);
            acquiresleep(&b->lock);
            return b;
        }
    }

    for(b = bcache.head.prev; b != &bcache.head; b = b->prev){
        if(b->refcnt == 0) {
            b->dev = dev;
            b->sectorno = sectorno;
            b->valid = 0;
            b->refcnt = 1;
            release(&bcache.lock);
            acquiresleep(&b->lock);
            return b;
        }
    }
    panic("bget: no buffers");
}

struct buf* bread(uint dev, uint sectorno) {
    struct buf *b;

    b = bget(dev, sectorno);
    if (!b->valid) {
        disk_read(b);
        b->valid = 1;
    }

    return b;
}

// Write b's contents to disk.  Must be locked.
void bwrite(struct buf *b) {
    if(!holdingsleep(&b->lock))
        panic("bwrite");
    disk_write(b);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void brelse(struct buf *b) {
    if(!holdingsleep(&b->lock))
        panic("brelse");

    releasesleep(&b->lock);

    acquire(&bcache.lock);
    b->refcnt--;
    if (b->refcnt == 0) {
        // no one is waiting for it.
        b->next->prev = b->prev;
        b->prev->next = b->next;
        b->next = bcache.head.next;
        b->prev = &bcache.head;
        bcache.head.next->prev = b;
        bcache.head.next = b;
    }
  
    release(&bcache.lock);
}

void bpin(struct buf *b) {
    acquire(&bcache.lock);
    b->refcnt++;
    release(&bcache.lock);
}

void bunpin(struct buf *b) {
    acquire(&bcache.lock);
    b->refcnt--;
    release(&bcache.lock);
}