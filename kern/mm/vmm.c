#include <vmm.h>
#include <sync.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <error.h>
#include <pmm.h>
#include <riscv.h>
#include <kmalloc.h>

// 根据输入参数vm_start、vm_end、vm_flags
// 来创建并初始化描述一个虚拟内存空间的vma_struct结构变量
struct vma * vma_create(uintptr_t vm_start, uintptr_t vm_end, uint32_t vm_flags){
    struct vma * vma = (struct vma *)kmalloc(sizeof(struct vma));
    
    if(vma){
        vma->vm_start = vm_start;
        vma->vm_end = vm_end;
        vma->vm_flags = vm_flags;
    }

    return vma;
}

// 根据输入参数addr和mm变量，
// 查找在mm变量中的mmap_list双向链表中某个vma包含此addr
// 即 vma->vma_start <= addr < vma->vma_end
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


static inline void check_vma_overlap(struct vma * prev, struct vma* next){
    assert(prev->vm_start < prev->vm_end);
    assert(next->vm_start < next->vm_end);
    assert(prev->vm_end <= next->vm_start);
}

// 把一个vma变量按照其空间位置[vma->vm_start,vma->vm_end]从小到大的顺序
// 插入到所属的mm变量中的mmap_list双向链表中。
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

int mm_map(struct mm_struct *mm, uintptr_t addr, size_t len, uint32_t vm_flags, struct vma ** vma_store){
    uintptr_t start = ROUNDDOWN(addr, PGSIZE), end = ROUNDUP(addr + len, PGSIZE);
    if (!USER_ACCESS(start, end)) {
        return -E_INVAL;
    }

    assert(mm != NULL);

    int ret = -E_INVAL;

    struct vma * _vma;
    if((_vma = findVMA(mm, start)) != NULL && end > _vma->vm_start){
        goto out;
    }
    ret = -E_NO_MEM;

    if((_vma = vma_create(start, end, vm_flags)) == NULL){
        goto out;
    }
    insert_vma(mm, _vma);
    if(vma_store != NULL){
        *vma_store = _vma;
    }
    ret = 0;
out:
    return ret;    
}

int dup_mmap(struct mm_struct * dst, struct mm_struct * src){
    assert(dst != NULL && src != NULL);
    list_entry_t *list = &(src->mmap_list), *le = list;
    while((le = list_next(le)) != list){
        struct vma *_vma, *_nvma;
        _vma = le2vma(le, list_link);
        _nvma = vma_create(_vma->vm_start, _vma->vm_end, _vma->vm_flags);
        if(_nvma == NULL){
            return -E_NO_MEM;
        }

        insert_vma(dst, _nvma);

        bool share = 0;
        if(copy_range(dst->pgdir, src->pgdir, _vma->vm_start, _vma->vm_end, share) != 0){
            return -E_NO_MEM;
        }
    }
    return 0;
}


volatile unsigned int pgfault_num = 0;

static void
check_pgfault(void) {
    size_t nr_free_pages_store = nr_free_pages();

    check_mm_struct = mm_create();
    assert(check_mm_struct != NULL);

    struct mm_struct *mm = check_mm_struct;
    pde_t *pgdir = mm->pgdir = boot_pgdir;
    assert(pgdir[0] == 0);

    struct vma *_vma = vma_create(0, PTSIZE, VM_WRITE);
    assert(_vma != NULL);

    insert_vma(mm, _vma);

    uintptr_t addr = 0x100;
    assert(findVMA(mm, addr) == _vma);

    int i, sum = 0;

    for (i = 0; i < 100; i ++) {
        *(char *)(addr + i) = i;
        sum += i;
    }
    for (i = 0; i < 100; i ++) {
        sum -= *(char *)(addr + i);
    }

    assert(sum == 0);

    pde_t *pd1=pgdir,*pd0=page2kva(pde2page(pgdir[0]));
    page_remove(pgdir, ROUNDDOWN(addr, PGSIZE));
    free_page(pde2page(pd0[0]));
    free_page(pde2page(pd1[0]));
    pgdir[0] = 0;
    flush_tlb();

    mm->pgdir = NULL;
    mm_destroy(mm);
    check_mm_struct = NULL;

    assert(nr_free_pages_store == nr_free_pages());

    cprintf("check_pgfault() succeeded!\n");
}

int do_pgfault(struct mm_struct *mm, uint_t errorcode, uintptr_t addr){
    int ret = -E_INVAL;
    struct vma * _vma = findVMA(mm, addr);

    pgfault_num++;
    if(_vma == NULL || _vma->vm_start > addr){
        cprintf("invalid addr %x, and can not find it in vma", addr);
        goto failed;
    }

    uint32_t perm = PTE_U;
    if(_vma->vm_flags & VM_WRITE){
        perm |= READ_WRITE;
    }
    addr = ROUNDDOWN(addr, PGSIZE);

    ret = -E_NO_MEM;

    pte_t * ptep = NULL;

    if((ptep = get_pte(mm->pgdir, addr, 1)) == NULL){
        cprintf("get_pte in do_pgfault failed\n");
        goto failed;
 
    }

    if(*ptep == 0){
        if(pgdir_alloc_page(mm->pgdir, addr, perm) == NULL){
            cprintf("pgdir_alloc_page in do_pgfault failed\n");
            goto failed;
        }
    }
    else{
        if(swap_init_ok){
            struct Page * page = NULL;
            if(( ret = swap_in(mm, addr, &page)) != 0){
                cprintf("swap_in in do_pgfault failed\n");
                goto failed;
            }
            page_insert(mm->pgdir, page, addr, perm);
            swap_map_swappable(mm, addr, page, 1);
            page->pra_vaddr = addr;
        }else{
            cprintf("no swap_init_ok but ptep is %x, failed\n", *ptep);
            goto failed;
        }
    }
    ret = 0;
failed:
    return ret;
}

void exit_mmap(struct mm_struct *mm){
    assert(mm != NULL && mm_count(mm) == 0);
    pde_t *pgdir = mm->pgdir;
    list_entry_t *list = &(mm->mmap_list), *le  = list;
    while((le = list_next(le)) != list){
        struct vma * _vma = le2vma(le, list_link);
        unmap_range(pgdir, _vma->vm_start, _vma->vm_end);
    }
    while((le = list_next(le)) != list){
        struct vma * _vma = le2vma(le, list_link);
        exit_range(pgdir, _vma->vm_start, _vma->vm_end);
    }
}


//
uintptr_t get_unmapped_area(struct mm_struct *mm, size_t len){

}

//
int mm_brk(struct mm_struct *mm, uintptr_t addr, size_t len){

}


bool user_mem_check(struct mm_struct *mm, uintptr_t addr, size_t len, bool write){
    if(mm != NULL){
        if(!USER_ACCESS(addr, addr + len)){
            return 0;
        }
        struct vma * _vma;
        uintptr_t start = addr, end = addr + len;
        while(start < end){
            if((_vma = findVMA(mm, start)) == NULL || start < _vma->vm_start){
                return 0;
            }
            if(!(_vma->vm_flags & ((write) ? VM_WRITE : VM_READ))){
                return 0;
            }
            if(write && (_vma->vm_flags & VM_STACK)){
                if(start < _vma->vm_start + PGSIZE){
                    return 0;
                }
            }
            start = _vma->vm_end;
        }
        return 1;
    }
    retuen KERN_ACCESS(addr, addr + len);
}


// 
bool copy_from_user(struct mm_struct *mm, void *dst, const void *src, size_t len, bool writable){

}


// 
bool copy_to_user(struct mm_struct *mm, void *dst, const void *src, size_t len){

}


// 
bool copy_string(struct mm_struct *mm, char *dst, const char *src, size_t maxn){
    
}


bool copy_string(struct mm_struct *mm, char *dst, const char *src, size_t maxn){
    size_t alen, part = ROUNDDOWN((uintptr_t)src + PGSIZE, PGSIZE) - (uintptr_t)src;
    while(1){
        if(part > maxn){
            part = maxn;
        }
        if(!user_mem_check(mm, (uintptr_t)src, part, 0)){
            return 0;
        }
        if((alen = strnlen(src, part)) < part){
            memcpy(dst, src, alen + 1);
        }
        if(part == maxn){
            return 0;
        }
        memcpy(dst, src, part);
        dst += part, src += part, maxn -= part;
        part = PGSIZE;
    }
}