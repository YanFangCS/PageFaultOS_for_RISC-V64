#ifndef __KERN_MM_VM_H__
#define __KERN_MM_VM_H__

#include <types.h>
#include <riscv.h>

void            kvmInit(void);
void            kvmInitHart(void);
uint64          kvmpa(uint64);
void            kvmMap(uint64, uint64, uint64, int);
int             MapPages(pagetable_t, uint64, uint64, uint64, int);
pagetable_t     uvmcreate(void);
// void            uvminit(pagetable_t, uchar *, uint);
void            uvminit(pagetable_t, pagetable_t, uchar *, uint);
uint64          uvmalloc(pagetable_t, pagetable_t, uint64, uint64);
uint64          uvmdealloc(pagetable_t, pagetable_t, uint64, uint64);
// int             uvmcopy(pagetable_t, pagetable_t, uint64);
int             uvmcopy(pagetable_t, pagetable_t, pagetable_t, uint64);
void            uvmfree(pagetable_t, uint64);
// void            uvmunmap(pagetable_t, uint64, uint64, int);
void            vmunmap(pagetable_t, uint64, uint64, int);
void            uvmclear(pagetable_t, uint64);
uint64          walkaddr(pagetable_t, uint64);
int             copyout(pagetable_t, uint64, char *, uint64);
int             copyin(pagetable_t, char *, uint64, uint64);
int             copyinstr(pagetable_t, char *, uint64, uint64);
pagetable_t     proc_kpagetable(void);
void            kvmfreeusr(pagetable_t kpt);
void            kvmfree(pagetable_t kpagetable, int stack_free);
uint64          kwalkaddr(pagetable_t pagetable, uint64 va);
int             copyout2(uint64 dstva, char *src, uint64 len);
int             copyin2(char *dst, uint64 srcva, uint64 len);
int             copyinstr2(char *dst, uint64 srcva, uint64 max);
void            vmprint(pagetable_t pagetable);


#endif