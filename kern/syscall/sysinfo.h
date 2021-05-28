#ifndef __KERN_SYSCALL_SYSINFO_H__
#define __KERN_SYSCALL_SYSINFO_H__

#include <types.h>

struct sysinfo {
    uint64 freemem;
    uint64 nums_of_proc;
};

#endif