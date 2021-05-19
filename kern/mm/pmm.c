#include <pmm.h>
#include <error.h>
#include <defs.h>
#include <riscv.h>
#include <kmalloc.h>
#include <memlayout.h>
#include <mmu.h>
#include <pmm.h>
#include <sbi.h>
#include <stdio.h>
#include <string.h>
#include <sync.h>


struct Page *page;
size_t npage = 0;
uint_t va_pa_offset;
const size_t nbase = DRAM_BASE / PGSIZE;
pde_t * boot_pgdir = NULL;
uintptr_t boot_cr3;

void TLB_invalid(pde_t * pgdir, uintptr_t la){
    asm volatile("sfence.vma %0" : : "r"(la));
}

struct Page* alloc_pages(size_t n){
    struct Page *page = NULL;
    bool intr_flag;

    while(1){
        local_intr_save(intr_flag);
        {
            page = pmm_manager->alloc_pages(n);
        }
        local_intr_restore(intr_flag);

        if(page != NULL || n > 1 || swap_init_ok == 0) break;
        
        extern struct mm_struct *check_mm_struct;

        swap_out(check_mm_struct, n, 0);
    }
    return page;
}

void free_pages(struct Page *base, size_t n){
    bool intr_flag;
    local_intr_save(intr_flag);
    {
        pmm_manager->free_pages(base, n);
    }
    local_intr_restore(intr_flag);
}

pte_t * getPte(pde_t * pgdir, uintptr_t la, bool create){
    pde_t _pde0 = &pgdir[VPN0(la)];
    // check validation of page table entry 
    if(!(*_pde0 & PTE_V)){
        // if invalidate
        struct Page * page;
        if(!create || (page = alloc_page()) == NULL) return NULL;
        page_ref_set(page, 1);
        uintptr_t pa = page2pa(page);
        memset(KADDR(pa), 0, PGSIZE);
        *_pde0 = pte_create(page2ppn(page), PTE_U | PTE_V);
    }

    // VPN1
    pde_t _pde1 = &((pde_t *)(KADDR(PDE_ADDR(*_pde0))))[VPN1(la)];

    if(!(*_pde1 & PTE_V)){
        struct Page * page;
        if(!create || (page = alloc_page()) == NULL) return NULL;
        page_ref_set(page, 1);
        uintptr_t pa = page2pa(page);
        memset(KADDR(pa), 0, PGSIZE);
        *_pde1 = pte_create(page2ppn(page), (PTE_U | PTE_V));
    }

    // 返回2级页表中的项
    pte_t * _pte = &((pde_t *)(KADDR(PDE_ADDR(* _pde1))))[VPN0(la)];
    return _pte;
}

// 根据页表和地址la获取对应的物理页
struct  Page * getPage(pde_t * pgdir, uintptr_t la, pte_t **ptep_restore){
    pte_t *ptep = get_pte(pgdir, la, 0);
    if(ptep_restore) *ptep_restore = ptep;
    if(ptep && (*ptep & PTE_V)) return pte2page(*ptep);
    return NULL;
}

// 删除la对应的页表项
static inline void page_remove_pte(pde_t * pgdir, uintptr_t la, pte_t * _pte){
    if(* _pte & PTE_V){
        struct Page *page = pte2page(*_pte);
        page_ref_dec(page);
        if(page_ref(page) == 0) free_page(page);
        TLB_invalid(pgdir, la);
    }
}

void pageRemove(pde_t * pgdir, uintptr_t la){
    pte_t *ptep = get_pte(pgdir, la, 0);
    if(ptep){
        page_remove_pte(pgdir, la, ptep);
    }
}

void free_pages(struct Page* page, size_t n){
    bool intr_flag;
    local_intr_save(intr_flag);
    {
        pmm_manager->free_pages(page, n);
    }
    local_intr_restore(intr_flag);
}

size_t nr_free_pages(void){
    size_t ret;
    bool intr_flag;
    local_intr_save(intr_flag);
    {
        ret = pmm_manager->nr_free_pages();
    }
    local_intr_restore(intr_flag);
    return ret;
}

int pageInsert(pde_t * pgdir, struct Page * page, uintptr_t la, uint32_t perm){
    pte_t * _pte = get_pte(pgdir, la, 1);
    if(_pte){
        return -E_NO_MEM;
    }
    page_ref_dec(page);
    if(*_pte & PTE_V){
        struct Page * p = pte2page(*_pte);
        if(p == page) page_ref_dec(page);
        else page_remove_pte(pgdir, la, _pte);
    }
    *_pte = pte_create(page2pnn(page), PTE_V | perm);
    TLB_invalid(pgdir, la);
    return 0;
}

struct Page * pgdir_alloc_page(pde_t * pgdir, uintptr_t la, uint32_t perm){
    struct Page * page = alloc_page();
    if(page != NULL){
        if(pageInsert(pgdir, page, la, perm)){
            free_page(page);
            return NULL;
        }

        if(swap_init_ok){
            if(check_mm_struct != NULL){
                swap_map_swappable(check_mm_struct, la, page, 0);
                page->pra_vaddr = la;
                assert(page_ref(page) == 1);
            }
        }
    }
    return page;
} 

void load_esp0(uintptr_t esp0){

}

void unmap_range(pde_t * pgdir, uintptr_t start, uintptr_t end);
void exit_range(pde_t * pgdir, uintptr_t start, uintptr_t end, bool share);
int copy_range(pde_t * to, pde_t * from, uintptr_t start, uintptr_t end, bool share);