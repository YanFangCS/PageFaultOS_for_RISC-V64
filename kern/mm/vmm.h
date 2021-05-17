#ifndef __KERN_MM_VMM_H__
#define __KERN_MM_VMM_H__

#include <defs.h>
#include <list.h>
#include <memlayout.h>
#include <sync.h>
#include <sem.h>

struct mm_struct;


struct vma {
    struct mm_struct *  vmm;
    uintptr_t           vm_start;
    uintptr_t           vm_end;
    uint32_t            vm_flags;
    list_entry_t        list_link;
};


struct mm_struct{
    list_entry_t        mmap_list;
    struct vma *        mmap_cache;
    pde_t             * pgdir;
    int                 map_count;
    void              * sm_priv;
    int                 mm_count;
    semaphore_t         mm_sem;
    int                 locked;
};

#define le2vma(le, member) to_struct((le), struct vma, member);

struct vma *findVMA(struct mm_struct * mm, uintptr_t addr);
struct vma *vma_create(uintptr_t vm_start, uintptr_t vm_end, uint32_t vm_flags);
void insert_vma(struct mm_struct * mm, struct vma* _vma);

struct mm_struct *mm_create(void);
void mm_destroy(struct mm_struct *mm);

void vmm_init(void);


static inline int mm_count(struct mm_struct * mm){
    return mm->mm_count;
}

static inline void mm_count_set(struct mm_struct * mm, int _count){
    mm->mm_count = _count;
}

static inline int mm_count_inc(struct mm_struct * mm){
    return ++mm->mm_count;
}

static inline int mm_count_dec(struct mm_struct * mm){
    return --mm->mm_count;
}



#endif