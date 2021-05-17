#ifndef __KERN_SYNC_WAIT_H__
#define __KERN_SYNC_WAIT_H__

#include <list.h>

typedef struct {
    list_entry_t wait_head;
} wait_queue_t;


#endif