#ifndef __KERN_MM_VMM_H__
#define __KERN_MM_VMM_H__

#include <defs.h>
#include <list.h>
#include <memlayout.h>
#include <sync.h>
#include <sem.h>

struct mm_struct;

// 描述应用程序对虚拟内存的需求
struct vma {
    struct mm_struct *  vmm;            // 使用相同页表的vma（虚拟内存区域）的集合
    uintptr_t           vm_start;       // vma起始地址，一个连续地址的虚拟内存空间的起始位置
    uintptr_t           vm_end;         // vma终结地址
    uint32_t            vm_flags;       // 虚拟内存空间的属性
    list_entry_t        list_link;      // 要求链接在一起的vma不相交
};

#define VM_READ     0x00000001
#define VM_WRITE    0x00000002
#define VM_EXEC     0x00000004
#define VM_STACK    0x00000008

struct mm_struct{
    list_entry_t        mmap_list;      // 链接所有属于同一目录页表的虚拟内存空间
    struct vma *        mmap_cache;     // 指向当前正在使用的虚拟内存空间
    pde_t             * pgdir;          // mm_struct结果维护的页表
    int                 map_count;      // 
    void              * sm_priv;        // 链接记录页访问情况的链表头
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
int mm_map(struct mm_struct *mm, uintptr_t addr, size_t len, uint32_t vm_flags,
            struct vma ** vma_store);
int do_pgfault(struct mm_struct *mm, uint_t error_code, uintptr_t addr);
int mm_unmap(struct mm_struct* mm, uintptr_t addr, size_t len);
int dup_mmap(struct mm_struct *dest, struct mm_struct *source);
void exit_map(struct mm_struct *mm);
uintptr_t get_unmapped_area(struct mm_struct *mm, size_t len);
int mm_brk(struct mm_struct *mm, uintptr_t addr, size_t len);

extern volatile unsigned int pgfault_num;
extern struct mm_struct *check_mm_struct;

bool user_mem_check(struct mm_struct *mm, uintptr_t start, size_t len, bool write);
bool copy_from_user(struct mm_struct *mm, void *dst, const void *src, size_t len, bool writable);
bool copy_to_user(struct mm_struct *mm, void *dst, const void *src, size_t len);
bool copy_string(struct mm_struct *mm, char *dst, const char *src, size_t maxn);

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