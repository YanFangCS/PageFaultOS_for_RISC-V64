#ifndef __KERN_MM_ALLOCOP_H__
#define __KERN_MM_ALLOCOP_H__

#include "libs/types.h"
void * operator new(uint64 size);
void operator delete(void *p);

#endif