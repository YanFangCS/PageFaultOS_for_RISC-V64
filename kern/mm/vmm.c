#include <vmm.h>
#include <sync.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <error.h>
#include <pmm.h>
#include <riscv.h>
#include <kmalloc.h>

// 创建 
struct mm_struct *mm_create(void){
    struct mm_struct *mm = (struct mm_struct *)kmalloc(sizeof(struct mm_struct));

    if(mm){
        list_init(&mm->mmap_list);
        mm->map_count = 0;
        mm->mmap_cache = NULL;
        mm->pgdir = NULL;

        mm_count_set(mm, 0);
        

        // todo list:
        // mm->sem init
        // mm->sm_priv
    }
    return mm;
}

struct vma * vma_create(uintptr_t vm_start, uintptr_t vm_end, uint32_t vm_flags){
    struct vma * vma = (struct vma *)kmalloc(sizeof(struct vma));
    
    if(vma){
        vma->vm_start = vm_start;
        vma->vm_end = vm_end;
        vma->vm_flags = vm_flags;
    }

    return vma;
}


struct vma * findVMA(struct mm_struct *mm, uintptr_t addr){
    struct vma *_vma = NULL;
    if (mm != NULL) {
        _vma = mm->mmap_cache;
        if (!(_vma != NULL && _vma->vm_start <= addr && _vma->vm_end > addr)) {
                bool found = 0;
                list_entry_t *list = &(mm->mmap_list), *le = list;
                while ((le = list_next(le)) != list) {
                    _vma = le2vma(le, list_link);
                    if (_vma->vm_start<=addr && addr < _vma->vm_end) {
                        found = 1;
                        break;
                    }
                }
                if (!found) {
                    _vma = NULL;
                }
        }
        if (_vma != NULL) {
            mm->mmap_cache = _vma;
        }
    }
    return _vma;
}


// 按vma->vm_addr的顺序找到vma的插入点，将其放入mmap_cache
void insert_vma(struct mm_struct * mm, struct vma* _vma){
    list_entry_t *le = &(mm->mmap_list);
    list_entry_t *list = le, *prev = le;

    while((le = list_next(le)) != list){
        struct vma * _t = le2vma(le, list_link);
        if(_t->vm_start > _vma->vm_start) break;
        prev = le;
    }

    //  todo:检查虚拟内存区域是否重叠 

    list_entry_t * lenext = list_next(prev);
    _vma->vmm = mm;
    list_add_after(lenext, &_vma->list_link);

    mm->map_count++;
}


void mm_destroy(struct mm_struct *mm){
    assert(mm->mm_count == 0);
    list_entry_t * list = &(mm->mmap_list), * t = list;

    while((t = list_next(list)) != list){
        list_del(t);
        kfree(le2vma(t, list_link));
    }

    kfree(mm);
    mm = NULL;
}

void vmm_init(void){
    return;
}

