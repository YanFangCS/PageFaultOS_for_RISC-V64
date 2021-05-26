#ifndef __KERN_MM_ALLOC_H__
#define __KERN_MM_ALLOC_H__

#include <types.h>

void * kalloc(void);
void   kfree(void *);
void   kinit(void);
uint64 freemem_amount(void);

#endif