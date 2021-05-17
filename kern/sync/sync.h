#ifndef __KERN_SYNC_SYNC_H__
#define __KERN_SYNC_SYNC_H__

#include <riscv.h>
#include <defs.h>
#include <intr.h>

static inline bool __intr_save(void){
    if(read_csr(sstatus) & SSTATUS_SIE){
        intr_disable();
        return 1;
    }
}

static inline void __intr_restore(bool flag){
    if(flag) intr_enable();
}

#define local_intr_save(x)      do {x = __intr_save(); } while(0);
#define local_intr_restore(x)   __intr_restore(x);

#endif