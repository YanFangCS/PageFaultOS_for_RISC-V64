#ifndef __KERN_MM_VM_H__
#define __KERN_MM_VM_H__

#include "libs/types.h"
#include "libs/riscv.h"

void        kvmInit(void);
void        kvmInitHart(void);
pte_t *     walk(pagetable_t pagetable, uint64 vaddr, int alloc);
uint64      walkaddr(pagetable_t pagetable, uint64 vaddr);
void        kvmMap(uint64 vaddr, uint64 paddr, uint64 size, int perm);
uint64      kvmPaddr(uint64 vaddr);
uint64      KGetKaddr(pagetable_t kpagetable, uint64 vaddr);
int         MapPages(pagetable_t pagetable, uint64 vaddr, uint64 paddr, uint64 size, int perm);
void        vmunmap(pagetable_t pagetable, uint64 vaddr, uint64 npages, int do_free);
pagetable_t Uvmcreate();
void        UvmInit(pagetable_t pagetable, pagetable_t kpagetable, uchar * src, uint size);
uint64      Uvmalloc(pagetable_t pagetable, pagetable_t kpagetable, uint64 oldsize, uint64 newsize);
uint64      UvmDealloc(pagetable_t pagetable, pagetable_t kpagetable, uint64 oldsize, uint64 newsize);
void        freewalk(pagetable_t pagetable);
void        Uvmfree(pagetable_t pagetable, uint64 size);
int         Uvmcopy(pagetable_t oldtable, pagetable_t newtable, pagetable_t knewtable, uint64 size );
void        Uvmclear(pagetable_t pagetable, uint64 vaddr);
int         copyout(pagetable_t pagetable, uint64 dstvaddr, char *src, uint64 len);
int         copyout2(uint64 dstva, char *src, uint64 len);
int         copyin(pagetable_t pagetable, char *dst, uint64 srcvaddr, uint64 len);
int         copyin2(char *dst, uint64 srcvaddr, uint64 len);
int         copyinstr(pagetable_t pagetable, char *dst, uint64 srcvaddr, uint64 max);
int         copyinstr2(char *dst, uint64 srcvaddr, uint64 max);
pagetable_t proc_kpagetable();
void        Kfreewalk(pagetable_t kpagetable);
void        KvmfreeUser(pagetable_t kpagetable);
void        Kvmfree(pagetable_t kpagetable, int stack_free);
void        vmprint(pagetable_t pagetable);

#endif