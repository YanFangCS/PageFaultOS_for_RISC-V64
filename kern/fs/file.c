#include <types.h> 
#include <riscv.h>
#include <param.h>
#include <spinlock.h>
#include <sleeplock.h>
#include <fat32.h>
#include <file.h>
#include <proc.h>
#include <printf.h>
#include <string.h>
#include <vm.h>


struct devsw devsw[NDEV];

struct {
    struct spinlock lock;
    struct file file[NFILE];
} ftable;

void fileInit(void){
    initlock(&ftable.lock, "filetable");
    struct *f;
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
