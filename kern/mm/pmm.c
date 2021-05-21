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

void ummap_range(pde_t * pgdir, uintptr_t start, uintptr_t end){
    assert(start % PGSIZE == 0 && end % PGSIZE == 0);
    assert(USER_ACCESS(start, end));


    do{
        pte_t *ptep = get_pte(pgdir, start, 0);
        if(ptep == NULL){
            start = ROUNDDOWN(start + PTSIZE, PTSIZE);
            continue;
        }
        if(* ptep != 0){
            page_remove_pte(pgdir, start, ptep);
        }
        start += PGSIZE;
    }while(start < end);
}

void exit_range(pde_t * pgdir, uintptr_t start, uintptr_t end){
    assert(start % PGSIZE == 0 && end % PGSIZE == 0);
    assert(USER_ACCESS(start, end));

    uintptr_t d1start, d0start;
    int free_pt, free_pd0;
    pde_t * pd0, * pt, pde1, pde0;
    d1start = ROUNDDOWN(start, PDSIZE);
    d0start = ROUNDDOWN(start, PTSIZE);

    do {
        pde1 = pgdir[VPN2(d1start)];

        if(pde1 && PTE_V){
            pd0 = page2kva(pde2page(pde1));
            free_pd0 = 1;

            do{
                pde0 = pd0[VPN1(d0start)];

                if(pde0 & PTE_V){
                    pt = page2kva(pde2page(pde0));
                    free_pt = 1;
                    for(int i = 0; i < NPTEENTRY; ++i){
                        if(pt[i] & PTE_V){
                            free_pt = 0;
                            break;
                        }
                    }
                    if(free_pt){
                        free_page(pde2page(pde0));
                        pd0[VPN1[d0start]] = 0;
                    }
                }else{
                    free_pd0 = 0;
                }
                d0start += PTSIZE;
            } while(d0start != 0 && d0start < d1start + PDSIZE && d0start < end);
            
            if(free_pd0){
                free_page(pde2page(pde1));
                pgdir[VPN2(d1start)] = 0;
            }
        }
        d1start += PDSIZE;
        d0start += d1start;
    } while(d1start != 0 && d1start < end);
}

int copy_range(pde_t * dst, pde_t * src, uintptr_t start, uintptr_t end, bool share ){
    assert(start % PGSIZE == 0 && end % PGSIZE == 0);
    assert(USER_ACCESS(start, end));

    do {
        pte_t * ptep = get_pte(src, start, 0);
        pte_t * nptep;
        if(ptep == NULL){
            start = ROUNDDOWN(start + PTSIZE, PTSIZE);
            continue;
        }

        if(ptep && PTE_V){
            if((nptep = get_pte(dst, start, 1)) == NULL){
                return -E_NO_MEM;
            }
            uint32_t perm = (*ptep & PTE_USER);
            struct Page * page = pte2page(*ptep);
            struct Page * npage = alloc_page();
            assert(page != NULL);
            assert(npage != NULL);
            int ret = 0;
            void * kvasrc = page2kva(page);
            void * kvadst = page2kva(npage);

            memcpy(kvadst, kvasrc, PGSIZE);

            ret = pageInsert(dst, npage, start, perm);
            assert(ret == 0);
        }
    }while(start != 0 && start < end);
    return 0;
}

void load_esp0(uintptr_t esp0){
    

}

// 使能分页机制
static void boot_map_segment(pde_t *pgdir, uintptr_t la, size_t n,
                             uintptr_t pa, uint_t perm){
    assert(PGOFF(la) == PGOFF(pa));
    size_t n = ROUNDUP(n + PGOFF(la), PGSIZE) / PGSIZE;
    la = ROUNDDOWN(la, PGSIZE);
    pa = ROUNDDOWN(pa, PGSIZE);
    for(; n > 0; n--, la += PGSIZE, pa += PGSIZE){
        pte_t * ptep = get_pte(pgdir, la, 1);
        assert(ptep != NULL);
        *ptep = pte_create(pa >> PGSHIFT, perm | PTE_V);
    }
}


// 为页表分配内存
static void *boot_alloc_page(void){
    struct Page * p = alloc_page();
    if(p == NULL){
        panic("boot_alloc_page failed\n");
    }
    return page2kva(p);
}

static switch_kernel_memlayout(){
    pde_t * kern_pgdir = (pde_t *)boot_alloc_page();
    memset(kern_pgdir, 0, PGSIZE);

    extern const char etext[];
    uintptr_t retext = ROUNDDOWN((uintptr_t)etext, PGSIZE);
    boot_map_segment(kern_pgdir, KERNBASE, retext-KERNBASE, PADDR(KERNBASE), PTE_R | PTE_X);
    boot_map_segment(kern_pgdir, retext, KERNTOP - retext, PADDR(retext), PTE_R | PTE_W);

    boot_pgdir = kern_pgdir;
    boot_cr3 = PADDR(boot_pgdir);
    lcr3(boot_cr3);
    flush_tlb();
    cprintf("Page table directory switch succeeded!\n");

    extern char bootstackguard[], boot_page_table_sv39[];
    if ((bootstackguard + PGSIZE == bootstack) && (bootstacktop == boot_page_table_sv39)){
        // check writeable and set 0
        memset(boot_page_table_sv39,0,PGSIZE);
        bootstack[-1] = 0;
        bootstack[-PGSIZE] = 0;

        // set pages beneath and above the kernel stack as guardians
        boot_map_segment(boot_pgdir,bootstackguard,PGSIZE,PADDR(bootstackguard),0);
        boot_map_segment(boot_pgdir,boot_page_table_sv39,PGSIZE,PADDR(boot_page_table_sv39),0);
        flush_tlb();

        cprintf("Kernel stack guardians set succeeded!\n");
    }
}
