#include "libs/types.h" 
#include "libs/riscv.h"
#include "libs/param.h"
#include "sync/spinlock.h"
#include "sync/sleeplock.h"
#include "fs/fat32.hpp"
#include "fs/file.h"
#include "proc/proc.h"
#include "libs/printf.h"
#include "libs/string.h"
#include "mm/vm.h"

// struct devsw devsw[NDEV];

struct {
    struct spinlock lock;
    struct file file[NFILE];
} ftable;

void fileInit(void){
    initlock(&ftable.lock, (char *)"filetable");
    struct file *f;
    for(f = ftable.file; f < ftable.file + NFILE; ++f){
        memset(f, 0, sizeof(struct file));
    }
}

struct file* filealloc(void){
    struct file * f;
    acquire(&ftable.lock);
    for(f = ftable.file; f < ftable.file + NFILE; ++f){
        if(f->ref == 0){
            f->ref = 1;
            release(&ftable.lock);
            return f;
        }
    }
    release(&ftable.lock);
    return NULL;
}
