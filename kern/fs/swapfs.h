#ifndef __KERN_MM_SWAPFS_H__
#define __KERN_MM_SWAPFS_H__

#include <memlayout.h>
#include <swap.h>

void swapfs_init();
int swapfs_read();
int swapfs_write();

#endif