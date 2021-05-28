#ifndef __KERN_PROC_PIPE_H__
#define __KERN_PROC_PIPE_H__

#include <types.h>
#include <spinlock.h>
#include <file.h>

#define PIPESIZE    512

struct pipe {
    struct spinlock lock;
    char data[PIPESIZE];
    uint nread;
    uint nwrite;
    int readopen;
    int writeopen;
}; 

int  pipealloc(struct file ** f0, struct file **f1);
void pipeclose(struct pipe * pi, int writable);
int  pipewrite(struct pipe * pi, uint64 addr, int n);
int  piperead(struct pipe * pi, uint64 addr, int n);

#endif