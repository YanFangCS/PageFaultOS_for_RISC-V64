#ifndef __KERN_SYSCALL_SYSCALL_H__
#define __KERN_SYSCALL_SYSCALL_H__

#include "libs/types.h"
#include "syscall/sysnum.h"

void syscall(void);
int fetchaddr(uint64 addr, uint64 *ip);
int fetchstr(uint64 addr, char *buf, int max);
int argaddr(int n, uint64 * ip);
int argint(int n, int *ip);
int argstr(int n, char * buf, int max);


#endif