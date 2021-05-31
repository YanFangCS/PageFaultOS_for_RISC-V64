#ifndef __KERN_DRIVER_DISK_H__
#define __KERN_DRIVER_DISK_H__


#include "fs/buf.hpp"

void disk_init(void);
void disk_read(struct buf *b);
void disk_write(struct buf *b);
void disk_intr(void);

#endif