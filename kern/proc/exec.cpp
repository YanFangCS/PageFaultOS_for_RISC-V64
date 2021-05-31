#include "libs/types.h"
#include "libs/param.h"
#include "mm/memlayout.h"
#include "libs/riscv.h"
#include "sync/spinlock.h"
#include "sync/sleeplock.h"
#include "proc/proc.h"
#include "libs/elf.h"
#include "fs/fat32.hpp"
#include "mm/kalloc.h"
#include "mm/vm.h"
#include "libs/printf.h"
#include "libs/string.h"
#include "fs/vfs.h"

// Load a program segment into pagetable at virtual address va.
// va must be page-aligned
// and the pages from va to va+sz must already be mapped.
// Returns 0 on success, -1 on failure.
static int
loadseg(pagetable_t pagetable, uint64 va, const char * path, uint offset, uint sz)
{
  uint i, n;
  uint64 pa;
  if((va % PGSIZE) != 0)
    panic("loadseg: va must be page aligned");

  for(i = 0; i < sz; i += PGSIZE){
    pa = walkaddr(pagetable, va + i);
    if(pa == NULL)
      panic("loadseg: address should exist");
    if(sz - i < PGSIZE)
      n = sz - i;
    else
      n = PGSIZE;
    if(vfsdread(path, reinterpret_cast<char *>(pa), n, offset + i) != n)
      return -1;
  }

  return 0;
}


int exec(char *path, char **argv)
{
  char *s, *last;
  int i, off;
  uint64 argc, sz = 0, sp, ustack[MAXARG+1], stackbase;
  struct elfhdr elf;
  // struct dirent *ep;
  struct prohdr ph;
  pagetable_t pagetable = 0;
  pagetable_t kpagetable = 0;
  struct proc *p = curproc();

  // Make a copy of p->kpt without old user space, 
  // but with the same kstack we are using now, which can't be changed
  if ((kpagetable = (pagetable_t)kalloc()) == NULL) {
    return 0;
  }
  memmove(kpagetable, p->kpagetable, PGSIZE);
  for (int i = 0; i < PX(2, MAXUVA); i++) {
    kpagetable[i] = 0;
  }

  if(vfsdread(path, reinterpret_cast<char *>(&elf), sizeof(elf), 0) != sizeof(elf)) {
    goto bad;
  }

  if (elf.magic != ELF_MAGIC) goto bad;

  // Load program into memory.
  for(i=0, off=elf.phoff; i<elf.phnum; i++, off+=sizeof(ph)){
    if(vfsdread(path, reinterpret_cast<char *>(&ph), sizeof(ph), off) != sizeof(ph))  goto bad;
    if(ph.type != ELF_PROG_LOAD)  continue;
    if(ph.memsz < ph.filesz)  goto bad;
    if(ph.vaddr + ph.memsz < ph.vaddr)  goto bad;
    uint64 sz1;
    if((sz1 = Uvmalloc(pagetable, kpagetable, sz, ph.vaddr + ph.memsz)) == 0) goto bad;
    sz = sz1;
    if(ph.vaddr % PGSIZE != 0)  goto bad;
    if(loadseg(pagetable, ph.vaddr, path, ph.off, ph.filesz) < 0) goto bad;
  }
  
  sz = PGROUNDDOWN(sz);
  uint64 sz1;
  if((sz1 = Uvmalloc(pagetable, kpagetable, sz, sz + PGSIZE)) == 0) goto bad;

  sz = sz1;
  sp = sz;
  stackbase = sz - PGSIZE;

  // Push argument strings, prepare rest of stack in ustack.
  for(argc = 0; argv[argc]; argc++) {
    if(argc >= MAXARG)
      goto bad;
    sp -= strlen(argv[argc]) + 1;
    sp -= sp % 16; // riscv sp must be 16-byte aligned
    if(sp < stackbase)
      goto bad;
    if(copyout(pagetable, sp, argv[argc], strlen(argv[argc]) + 1) < 0)
      goto bad;
    ustack[argc] = sp;
  }
  ustack[argc] = 0;

  // push the array of argv[] pointers.
  sp -= (argc+1) * sizeof(uint64);
  sp -= sp % 16;
  if(sp < stackbase)
    goto bad;
  if(copyout(pagetable, sp, (char *)ustack, (argc+1)*sizeof(uint64)) < 0)
    goto bad;

  // arguments to user main(argc, argv)
  // argc is returned via the system call return
  // value, which goes in a0.
  p->trapframe->a1 = sp;

  // Save program name for debugging.
  for(last=s=path; *s; s++)
    if(*s == '/')
      last = s+1;
  safestrcpy(p->name, last, sizeof(p->name));
    
  // Commit to the user image.
  // oldpagetable = p->pagetable;
  // oldkpagetable = p->kpagetable;
  p->pagetable = pagetable;
  p->kpagetable = kpagetable;
  p->sz = sz;
  p->trapframe->epc = elf.entry;  // initial program counter = main
  p->trapframe->sp = sp; // initial stack pointer
  //proc_freepagetable(oldpagetable, oldsz);
  w_satp(MAKE_SATP(p->kpagetable));
  sfence_vma();
  //kvmfree(oldkpagetable, 0);
  return argc; // this ends up in a0, the first argument to main(argc, argv)

 bad:

  // if(pagetable)
  //   proc_freepagetable(pagetable, sz);
  // if(kpagetable)
  //   kvmfree(kpagetable, 0);
  // if(ep){
  //   eunlock(ep);
  //   eput(ep);
  // }
  return 0;
}
