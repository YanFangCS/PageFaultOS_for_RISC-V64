#include <kalloc.h>
#include <types.h>
#include <memlayout.h>
#include <spinlock.h>
#include <riscv.h>
#include <string.h>

void FreeRange(void *pa_start, void *pa_end);


extern char kernel_end[];

struct run{
    struct run *next;
}

struct {
    struct spinlock lock;
    struct run * freelist;
    uint64 npages;
} kmem;

void kinit(void){
    initlock(&kmem.lock, "kmem");
    kmem.freelist = 0;
    kmem.npages = 0;
    FreeRange(kernel_end, (void *)PHYSTOP);
}

void FreeRange(void * pa_start, void * pa_end){
    char *p = (char *)PGROUNDDOWN((uint64)pa_start);
    for(; p + PGSIZE < pa_end; p += PGSIZE){
        kfree(p);
    }
}

void kfree(void *pa){
    struct run * r;
    if(((uint64)pa % PGSIZE) != 0 || (char *)pa < kernel_end || (uint64)pa > PHYSTOP){
        panic("kfree: invalid physical address");
    }

    memset(pa, 1, PGSIZE);

    r = (struct run* )pa;

    acquire(&kmem.lock);
    r->next = kmem.freelist;
    kmem.freelist = r;
    kmem.npages ++;
    release(&kmem.lock);
}

void * kalloc(void){
    struct run;

    acquire(&kmem.lock);
    r = kmem.freelist;
    if(r){
        kmem.freelist = r->next;
        kmem.npages--;
    }
    release(&kmem.lock);

    if(r) memset((char *)r, 5, PGSIZE);

    return (void *)r;
}


uint64 freemem_amount(void){
    return kmem.npages << PGSHIFT;
}