#ifndef __KERN_MM_KMALLOC_H__
#define __KERN_MM_KMALLOC_H__

#include <defs.h>

void kmalloc_init(void);

void *kmalloc(size_t n);
void kfree(void *obj);

size_t kallocated(void);

#endif;