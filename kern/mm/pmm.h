#ifndef __KERN_MM_PMM_H__
#define __KERN_MM_PMM_H__

#include <defs.h>
#include <mmu.h>
#include <memlayout.h>
#include <atomic.h>
#include <error.h>
#include <assert.h>

// pmm --> physical memory manager
struct pmm_manager {
    const char * name;
    void (*init)(void);
    void (*init_memmap)(struct Page* base, size_t n);
    struct Page *(*alloc_pages)(size_t n);
    void (*free_pages)(struct Page* base, size_t n);
    size_t (*nr_free_pages)(void);
    void (*check)(void);
};

extern const struct pmm_manager * pmm_manager;


void pmm_init(void);
void free_pages(struct Page*page, size_t n);
struct  Page *alloc_pages(size_t n);

#define alloc_page() alloc_pages(1)
#define free_page(page) free_pages(page, 1);

pte_t * get_pte(pde_t * pgdir, uintptr_t la, bool create);
struct  Page * getPage(pde_t * pgdir, uintptr_t la, pte_t ** pte_store);
void pageRemove(pde_t * pgdir, uintptr_t la);
int  pageInsert(pde_t * pgdir, struct Page * page, uintptr_t la, uint32_t perm);

void load_esp0(uintptr_t esp0);
void TLB_invalid(pde_t * pddir, uintptr_t);

void load_esp0(uintptr_t esp0);
struct Page * pgdir_alloc_page(pde_t *pgdir, uintptr_t la, uint32_t perm);
void unmap_range(pde_t * pgdir, uintptr_t start, uintptr_t end);
void exit_range(pde_t * pgdir, uintptr_t start, uintptr_t end, bool share);
int copy_range(pde_t * to, pde_t * from, uintptr_t start, uintptr_t end, bool share);

extern const struct pmm_manager * pmm_manager;
extern pde_t * boot_dir;
extern const size_t nbase;
extern uintptr_t boot_cr3;

extern uint_t va_pa_offset;
extern size_t npage;
extern struct Page * pages;

// functions transforming between kernel vitrual address ans physical address
uintptr_t PADDR(uintptr_t kva){
    uintptr_t _kva = (uintptr_t) kva;
    if(_kva < KERNBASE || KERNTOP < _kva){
        panic("PADDR called with invalid kva %08lx", _kva);
    }
    return _kva - va_pa_offset;
}

uintptr_t KADDR(uintptr_t pa){
    uintptr_t _pa = (uintptr_t) pa;
    size_t _ppn = PPN(_pa);
    if(_ppn >= npage){
        panic("KADDR called with invalid pa %08lx", _pa);
    }
    return _pa + va_pa_offset;
}

static inline ppn_t page2ppn(struct Page *page){
    return page - pages + nbase;
}

static inline uintptr_t page2pa(struct Page *page){
    return page2pnn(page) << PGSHIFT;
}

static inline struct Page * pa2page(uintptr_t pa){
    if(PPN(pa) >= npage){
        panic("pa2page(physical address to page) called with invalid pa %08lx", pa);
    }
    return &pages[PPN(pa) - nbase];
} 

static inline void * page2kva(struct Page *page){
    return KADDR(page2pa(page));
}

static inline struct Page * kva2page(void *kva){
    return pa2page(PADDR(kva));
}

static inline struct Page * pte2page(pte_t pte){
    if(!(pte & PTE_V)){
        panic("pte2page called with invalid pte(page table entry)");
    }
    return pa2page(PTE_ADDR(pte));
}

static inline struct Page * pde2page(pde_t pde){
    return pa2page(PADDR(pde));
}

static inline int page_ref(struct Page * page){
    return page->ref;
}

static inline void page_ref_set(struct Page * page, int _ref){
    page->ref = _ref;
}

static inline int page_ref_inc(struct Page * page){
    return ++page->ref;
}

static inline int page_ref_dec(struct Page * page){
    return --page->ref;
}

static inline void flush_tlb(){
    asm volatile("sfence.vma");
}


// 物理地址 = 物理页号 + 页内偏移地址
// 物理页号 = 0级页表项
static inline pte_t pte_create(uintptr_t ppn, int type){
    return (ppn << PTE_PPN_SHIFT | PTE_V | type);
}

static inline pte_t ptd_create(uintptr_t ppn){
    return pte_create(ppn , PTE_V);
}

extern char bootstack[], bootstacktop[];

#endif