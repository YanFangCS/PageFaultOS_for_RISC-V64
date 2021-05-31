#ifndef __KERN_PROC_PIPE_H__
#define __KERN_PROC_PIPE_H__

#include "libs/types.h"
#include "sync/spinlock.h"
#include "fs/file.h"

#define PIPESIZE    512

class pipe {
private:
    bool readable;
    bool writeable;
    uint nwrite;
    uint nread;
    struct spinlock lock;
    char buf[PIPESIZE];

public:
    enum EndType {
        READ_END,   WRITE_END
    };

    pipe(struct file * f0, struct file * f1);
    int piperead(uint64 addr, int n);
    int pipewrite(uint64 addr, int n);
    void pipeclose(EndType endt);
} ;

#endif