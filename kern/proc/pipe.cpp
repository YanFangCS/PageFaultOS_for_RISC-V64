#include "libs/types.h"
#include "libs/riscv.h"
#include "libs/param.h"
#include "sync/spinlock.h"
#include "fs/file.h"
#include "proc/pipe.hpp"
#include "proc/proc.h"
#include "mm/kalloc.h"
#include "mm/vm.h"

pipe::pipe(struct file * f0, struct file * f1){
    this->readable = 1;
    this->writeable = 1;
    this->nread = 0;
    this->nwrite = 0;
    initlock(&(this->lock), (char *)"pipe");
    f0->type = f0->FD_PIPE;
    f0->readable = true;
    f0->writable = false;
    f0->pip = this;

    f1->type = f1->FD_PIPE;
    f1->readable = false;
    f1->writable = true;
    f1->pip = this;
}

int pipe::piperead(uint64 addr, int n){
    int i;
    struct proc * p = curproc();
    char ch;
    acquire(&(this->lock));

    while(this->nread == this->nwrite && this->writeable){
        if(p->killed){
            release(&(this->lock));
            return -1;
        }
        sleep(&(this->nread), &(this->lock));
    }

    for(i = 0; i < n; ++i){
        if(this->nread == this->nwrite) break;
        ch = this->buf[this->nread++ % PIPESIZE];
        if (copyout(p->pagetable, addr + i, &ch, 1) == -1) break;
    }
    wakeup(&(this->nwrite));
    release(&(this->lock));
    return i;
}

int pipe::pipewrite(uint64 addr, int n){
    int i;
    char ch;
    struct proc * p = curproc();

    acquire(&(this->lock));
    for(i = 0; i < n; ++i){
        while (this->nwrite == this->nread + PIPESIZE){
            if(this->readable == 0 || p->killed){
                release(&(this->lock));
                return -1;
            }
            wakeup(&(this->nread));
            sleep(&(this->nwrite), &(this->lock));
        }
        if(copyin(p->pagetable, &ch, addr + i, 1) == -1) break;
        this->buf[this->nwrite++ % PIPESIZE] = ch;
    }
    wakeup(&(this->nread));
    release(&(this->lock));
    return i;
}

void pipe::pipeclose(EndType endt){
    acquire(&(this->lock));
    if(endt == EndType::READ_END){
        this->readable = 0;
        wakeup(&(this->nwrite));
    } else {
        this->writeable = 0;
        wakeup(&(this->nread));
    }
    if((!this->readable) && (!this->writeable)){
        release(&(this->lock));
        delete this;
    } else {
        release(&(this->lock));
    }
}



