#include "mm/allocop.h"
#include "mm/kalloc.h"

void * operator new(uint64 size){
    return kalloc();
}

void operator delete(void *p){
    return kfree(p);
}
