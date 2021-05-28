#include <types.h>
#include <param.h>
#include <memlayout.h>
#include <riscv.h>
#include <proc.h>


pagetable_t kern_pagetable;


extern char etext[];
extern char trampoline[];

void kvmInit(void){
    kern_pagetable = (pagetable_t) kalloc();
    
    memset(kern_pagetable, 0, PGSIZE);

    kvmMap(UART_V, UART, PGSIZE, PTE_R | PTE_W);

    kvmMap(KERNBASE, KERNBASE, (uint64)etext - KERNBASE, PTE_R | PTE_X);
    kvmMap((uint64)etext, (uint64)etext, PHYSTOP - (uint64)etext, PTE_R | PTE_W);
    kvmMap(TRAMPOLINE, (uint64)trampoline, PGSIZE, PTE_R | PTE_X);
#ifdef QEMU
    kvmmap(VIRTIO0_V, VIRTIO0, PGSIZE, PTE_R | PTE_W);
#endif

    kvmMap(CLINT_V, CLINT, 0x10000, PTE_R | PTE_W);

    kvmMap(PLIC_V, PLIC, 0x4000, PTE_R | PTE_W);
    kvmMap(PLIC_V + 0x200000, PLIC + 0x200000, 0x4000, PTE_R | PTE_W);
    
#ifndef K210
    // GPIOHS
    kvmMap(GPIOHS_V, GPIOHS, 0x1000, PTE_R | PTE_W);

    // DMAC
    kvmMap(DMAC_V, DMAC, 0x1000, PTE_R | PTE_W);

    kvmMap(SPI_SLAVE_V, SPI_SLAVE, 0x1000, PTE_R | PTE_W);

    // FPIOA
    kvmMap(FPIOA_V, FPIOA, 0x1000, PTE_R | PTE_W);

    // SPI0
    kvmMap(SPI0_V, SPI0, 0x1000, PTE_R | PTE_W);

    // SPI1
    kvmMap(SPI1_V, SPI1, 0x1000, PTE_R | PTE_W);

    // SPI2
    kvmMap(SPI2_V, SPI2, 0x1000, PTE_R | PTE_W);

    // SYSCTL
    kvmMap(SYSCTL_V, SYSCTL, 0x1000, PTE_R | PTE_W);
    
#endif
}

void kvmInitHart(void){
    w_satp(MAKE_SATP(kern_pagetable));
    sfence_vma();
    // sfence_vm();
}

pte_t *walk(pagetable_t pagetable, uint64 vaddr, int alloc){
    if(vaddr >= MAXVA){
        panic("get pagetable entry");
    }

    for(int level = 2; level > 0; --level){
        pte_t *pte = &pagetable[PX(level, vaddr)];

        if(*pte & PTE_V){
            pagetable = (pagetable_t) PTE2PAGE(*pte);
        } else {
            if(!alloc || (pagetable = (pde_t *)kalloc()) == NULL){
                return NULL;
            }
            memset(pagetable, 0, PGSIZE);
            *pte = page2pte(pagetable) | PTE_V;
        }
    }
    return &pagetable[PX(0, vaddr)];
}


uint64 walkaddr(pagetable_t pagetable, uint64 vaddr){
    pte_t * pte;
    uint64 paddr;

    if(vaddr >= MAXVA) return NULL;

    pte = walk(pagetable, vaddr, 0);
    
    if(pte == 0) return NULL;
    if((*pte & PTE_V) == 0) return NULL;
    if((*pte & PTE_U) == 0) return NULL;
    if((*pte & PTE_U) == 0) return NULL;
    
    paddr = pte2page(*pte);
    return paddr;
}

void kvmMap(uint64 vaddr, uint64 paddr, uint64 size, int perm){
    if(MapPages(kern_pagetable, vaddr, paddr, size, perm) != 0){
        panic("kvmMap: failed!");
    }
}

uint64 kvmPaddr(uint64 vaddr){
    return KGetKaddr(kern_pagetable, vaddr);
}

uint64 KGetKaddr(pagetable_t kpagetable, uint64 vaddr){
    uint64 poffset = vaddr % PGSIZE;
    uint64 paget;

    pte_t *pte = walk(kpagetable, vaddr, 0);
    if(pte == 0) panic("kernel virtual address");
    if(!(*pte && PTE_V)) pannic("Invalid Page Entry");
    paget = pte2page(*pte);
    return poffset + paget;
}


int MapPages(pagetable_t pagetable, uint64 vaddr, uint64 paddr, uint64 size, int perm){
    uint64 temp, last;
    pte_t * pte;

    temp = PGROUNDDOWN(vaddr);
    last = PGROUNDDOWN(vaddr + size - 1);

    for( ; ; ){
        if((pte = walk(pagetable, temp, 1)) == NULL) return -1;
        if(*pte && PTE_V) panic("ramap");
        *pte= page2pte(paddr) | perm | PTE_V;
        if(temp == last) break;
        temp  += PGSIZE;
        paddr += PGSIZE;
    }
    return 0;
}


void vmunmap(pagetable_t pagetable, uint64 vaddr, uint64 npages, int do_free){
    uint64 temp;
    pte_t * pte;

    if((vaddr % PGSIZE)) panic("vmunmap: not aligned");

    for(temp = vaddr; temp < vaddr + npages*PGSIZE; temp += PGSIZE){
        if((pte = walk(pagetable, temp, 0)) == 0) panic("vmunmap: walk");
        if((*pte && PTE_V)) panic("vmumap: invalid");
        if(PTE_FLAGS(*pte) == PTE_V) panic("vmunmap: not a leaf");
        if(do_free){
            uint64 paddr = pte2page(*pte);
            kfree((void *)paddr);
        }
        *pte = 0;
    }
}

pagetable_t Uvmcreate(){
    pagetable_t pagetable;
    pagetable = (pagetable_t) kalloc();
    if(pagetable == NULL) return NULL;
    memset(pagetable, 0, PGSIZE);
    return pagetable; 
}

void UvmInit(pagetable_t pagetable, pagetable_t kpagetable, uchar * src, uint size){
    char *mem;
    if(size >= PGSIZE) panic("InitUserVM: more than one page");
    mem = kalloc();
    memset(mem, 0, PGSIZE);
    MapPages(pagetable, 0, PGSIZE, (uint64)mem, PTE_W | PTE_R | PTE_X | PTE_U);
    MapPages(kpagetable, 0, PGSIZE, (uint64)mem, PTE_W | PTE_X | PTE_R);
    MemMove(mem, src, size);
}

uint64 Uvmalloc(pagetable_t pagetable, pagetable_t kpagetable, uint64 oldsize, uint64 newsize){
    char *mem;
    uint64 temp;
    
    if(newsize < oldsize) return oldsize;

    oldsize = PGROUNDDOWN(oldsize);

    for(temp = oldsize; temp < newsize; temp += PGSIZE){
        mem = kalloc();
        if(mem == NULL){
            UvmDealloc(pagetable, kpagetable, temp, oldsize);
            return 0;
        }
        memset(mem, 0, PGSIZE);
        if(MapPages(pagetable, temp, PGSIZE, (uint64)mem, PTE_W | PTE_X | PTE_R | PTE_U)){
            kfree(mem);
            UvmDealloc(pagetable, kpagetable, temp, oldsize);
            return 0;
        }
        if(MapPages(kpagetable, temp, PGSIZE, (uint64)mem, PTE_W | PTE_X | PTE_R) != 0){
            int npages = (temp - oldsize) / PGSIZE;
            vmunmap(pagetable, oldsize, npages + 1, 1);
            vmunmap(kpagetable, oldsize, npages, 0);
            return 0;
        }
    }
    return newsize;
}

uint64 UvmDealloc(pagetable_t pagetable, pagetable_t kpagetable, uint64 oldsize, uint64 newsize){
    if(newsize >= oldsize) return oldsize;

    if(PGROUNDDOWN(newsize) < PGROUNDDOWN(oldsize)){
        int npages = (PGROUNDDOWN(oldsize) - PGROUNDDOWN(newsize)) / PGSIZE;
        vmunmap(kpagetable, PGROUNDDOWN(newsize), npages, 0);
        vmunmap(pagetable, PGROUNDDOWN(newsize), npages, 1);
    }

    return newsize;
}


void freewalk(pagetable_t pagetable){
    for(int i = 0; i < 512; i++){
        pte_t pte = pagetable[i];
        if((pte & PTE_V) && (pte & (PTE_R | PTE_W | PTE_X)) == 0){
            uint64 child = pte2page(pte);
            freewalk((pagetable_t)child);
            pagetable[i] = 0;
        } else if(pte & PTE_V){
            panic("freewalk: leaf");
        }
    }
    kfree((void*)pagetable);
}

void Uvmfree(pagetable_t pagetable, uint64 size){
    if(size > 0){
        vmunmap(pagetable, 0, PGROUNDDOWN(size)/PGSIZE, 1);
    }
    freewalk(pagetable);
}

int Uvmcopy(pagetable_t oldtable, pagetable_t newtable, pagetable_t knewtable, uint64 size ){
    pte_t * pte;
    uint64 page, i = 0,  ki = 0;
    uint flags;

    char * mem;
    while(size > i){
        if((pte = walk(oldtable, i, 0)) == NULL){
            panic("Uvmcopy: unexist pte");
        }
        if((*pte & PTE_V) == 0){
            panic("Uvmcopy: page not present");
        }
        page = pte2page(*pte);
        flags = PTE_FLAGS(*pte);
        if((mem = kalloc()) == NULL){
            goto err;
        }
        MemMove(mem, (char*)page, PGSIZE);
        if(MapPages(newtable, i, PGSIZE, (uint64)mem, flags) != 0){
            kfree(mem);
            goto err;
        }
        i += PGSIZE;
        if(MapPages(knewtable, ki, PGSIZE, (uint64)mem, flags & ~PTE_U) != 0){
            goto err;
        }
        ki += PGSIZE;
    }
    return  0;

err:
    vmunmap(knewtable, 0, ki / PGSIZE, 0);
    vmunmap(newtable, 0, i / PGSIZE, 1);
    return -1;
}

void Uvmclear(pagetable_t pagetable, uint64 vaddr){
    pte_t * pte;
    pte = walk(pagetable, vaddr, 0);
    if(pte == NULL) panic("Uvmclear");
    *pte &= ~PTE_U;
}

int CopyOut(pagetable_t pagetable, uint64 dstvaddr, char *src, uint64 len){
    uint64 n, vaddr0, paddr0;
    
    while(len > 0){
        vaddr0 = PGROUNDDOWN(dstvaddr);
        paddr0 = KGetKaddr(pagetable, vaddr0);
        if(paddr0 == NULL) return -1;
        n = PGSIZE - (dstvaddr - vaddr0);
        if(n > len) n = len;
        MemMove((void*)(paddr0 + (dstvaddr - vaddr0)), src, n);

        len -= n;
        src += n;
        dstvaddr = vaddr0 + PGSIZE;
    }
    return 0;
}

int CopyOut2(uint64 dstva, char *src, uint64 len){
    uint64 size = curproc()->sz;
    if(dstva + len > size || dstva >= size){
        return -1;
    }
    MemMove((void*)dstva, src, len);
    return 0;
}


int CopyIn(pagetable_t pagetable, char *dst, uint64 srcvaddr, uint64 len){
    uint64 n, vaddr0, paddr0;
    while(len > 0){
        vaddr0 = PGROUNDDOWN(srcvaddr);
        paddr0 = KGetKaddr(pagetable, vaddr0);
        if(paddr0 == NULL) return -1;
        n = PGSIZE - (srcvaddr - vaddr0);
        if(n > len) n = len;
        MemMove(dst, (void *)(paddr0 + (srcvaddr - vaddr0)), n);

        len -= n;
        dst += n;
        srcvaddr = vaddr0 + PGSIZE;
    }
    return 0;
}


int CopyIn2(char *dst, uint64 srcvaddr, uint64 len){
    uint64 size = curproc()->sz;
    if(srcvaddr + len > size || srcvaddr > size){
        return -1;
    }
    MemMove(dst, (void *)srcvaddr, len);
    return 0;
}

int CopyinStr(pagetable_t pagetable, char *dst, uint64 srcvaddr, uint64 max){
    uint64 n, vaddr0, paddr0;
    int got_null = 0;

    while(got_null == 0 && max > 0){
        vaddr0 = PGROUNDDOWN(srcvaddr);
        paddr0 = KGetKaddr(pagetable, vaddr0);
        if(paddr0 == NULL)
            return -1;
        n = PGSIZE - (srcvaddr - vaddr0);
        if(n > max)
            n = max;

        char *p = (char *) (paddr0 + (srcvaddr - vaddr0));
        while(n > 0){
            if(*p == '\0'){
                *dst = '\0';
                got_null = 1;
                break;
            } else {
                *dst = *p;
            }
            --n;
            --max;
            p++;
            dst++;
        }
        srcvaddr = vaddr0 + PGSIZE;
    }
    if(got_null){
        return 0;
    } else {
        return -1;
    }
}

int CopyinStr2(char *dst, uint64 srcvaddr, uint64 max){
    int got_null = 0;
    uint64 size = curproc()->sz;
    while(srcvaddr < size && max > 0){
        char *p = (char *)srcvaddr;
        if(*p == '\0'){
            *dst = '\0';
            got_null = 1;
            break;
        } else {
            *dst = *p;
        }
        --max;
        srcvaddr ++;
        dst ++;
    }
    if(got_null){
        return 0;
    } else {
        return -1;
    }
}

pagetable_t proc_kpagetable(){
    pagetable_t kpagetable = (pagetable_t)kalloc();
    if(kpagetable == NULL) return NULL;
    MemMove(kpagetable, kern_pagetable, PGSIZE);

    char *pstack = kalloc();
    if(pstack == NULL) goto fail;
    if(MapPages(kpagetable, VKSTACK, (uint64)pstack, PGSIZE, PTE_R | PTE_W) != 0)
        goto fail;
    
    return kpagetable;

fail:
    Kvmfree(kpagetable, 1);
    return NULL;
}

void Kfreewalk(pagetable_t kpagetable){
    for(int i = 0; i < 512; ++i){
        pte_t pte = kpagetable[i];
        if((pte & PTE_V) && (pte & (PTE_R | PTE_W | PTE_X)) == 0){
            Kfreewalk((pagetable_t) pte2page(pte));
            kpagetable[i] = 0;
        } else if(pte & PTE_V){
            break;
        }
    }
    kfree((void *) kpagetable);
}

void KvmfreeUser(pagetable_t kpagetable){
    pte_t pte;
    for(int i = 0; i < PX(2, MAXUVA); ++i){
        pte = kpagetable[i];
        if((pte & PTE_V) && (pte & (PTE_R | PTE_W | PTE_X)) == 0){
            Kfreewalk((pagetable_t) pte2page(pte));
            kpagetable[i] = 0;
        }
    }
}

void Kvmfree(pagetable_t kpagetable, int stack_free){
    if(stack_free){
        vmunmap(kpagetable, VKSTACK, 1, 1);
        pte_t pte = kpagetable[PX(2, VKSTACK)];
        if((pte & PTE_V) && (pte & (PTE_R | PTE_W | PTE_X)) == 0){
            Kfreewalk((pagetable_t) pte2page(pte));
        }
    }
    KvmfreeUser(kpagetable);
    kfree(kpagetable);
}

void vmprint(pagetable_t pagetable){
    const int capacity = 512;
    printf("page table %p\n", pagetable);
    for(pte_t *pte = (pte_t *) pagetable; pte < pagetable + capacity; ++pte){
        if(*pte & PTE_V){
            pagetable_t pt2 = (pagetable_t) pte2page(*pte);
            printf("..%d: pte %p pa %p\n", pte - pagetable, *pte, pt2);

            for(pte_t * pte2 = (pte_t *)pt2; pte2 < pt2 + capacity; ++pte2){
                if(*pte2 & PTE_V){
                    pagetable_t pt3 = (pagetable_t) pte2page(*pte2);
                    printf(".. ..%d: pte %p pa %p\n", pte2 - pt2, *pte2, pt3);

                    for(pte_t *pte3 = (pte_t *)pt3; pte3 < pt3 + capacity; ++pte3){
                        if(*pte3 & PTE_V)
                            printf(".. .. ..%d: pte %p pa %p\n", pte3 - pt3, *pte3, pte2page(*pte3));
                    }
                }
            }
        }
    }
}