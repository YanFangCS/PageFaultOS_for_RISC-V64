#include "include/types.h"
#include "include/param.h"
#include "include/memlayout.h"
#include "include/riscv.h"
#include "include/spinlock.h"
#include "include/proc.h"
#include "include/intr.h"
#include "include/kalloc.h"
#include "include/printf.h"
#include "include/string.h"
#include "include/fat32.h"
#include "include/file.h"
#include "include/trap.h"
#include "include/vm.h"


struct cpu cpus[NCPU];

struct proc proc[NPROC];

struct proc *initproc;

int nextsafe = 1;
struct spinlock pid_lock;

extern void forkret(void);
extern void swtch(struct context*, struct context*);
static void wakeup1(struct proc *chan);
static void freeproc(struct proc *p);

extern char trampoline[]; // trampoline.S

void procinit(void){
    struct proc *p;

    for(p = proc; p < &proc[NPROC]; p++) {
      initlock(&p->lock, "proc");
      memset(cpus, 0, sizeof(cpus));
  }
}
//返回cpu id
int cpuid()
{
  int id = r_tp();
  return id;
}

//获取当前cpu struct
struct cpu* curcpu(void){
    int id = cpuid();
    struct cpu *c = &cpus[id];
    return c;
}
//返回指向当前进程的指针，如果无则返回0
struct proc* curproc(void) {
  push_off();
  struct cpu *c = curcpu();
  struct proc *p = c->proc;
  pop_off();
  return p;
}
//申请进程号
int allocpid() {
  int pid;
  
  acquire(&pid_lock);
  pid = nextsafe;
  nextsafe = nextsafe + 1;
  release(&pid_lock);

  return pid;
}

//申请进程
static struct proc* allocproc(void)
{
  struct proc *p;

  for(p = proc; p < &proc[NPROC]; p++) {
    acquire(&p->lock);
    if(p->state == UNUSED) {
      goto found;
    } else {
      release(&p->lock);
    }
  }
  return NULL;

found:
  p->pid = allocpid();
  if((p->trapframe = (struct trapframe *)kalloc()) == NULL){
    release(&p->lock);
    return NULL;
  }
  if ((p->pagetable = proc_pagetable(p)) == NULL ||
      (p->kpagetable = proc_kpagetable()) == NULL) {
    freeproc(p);
    release(&p->lock);
    return NULL;
  }

  p->kstack = VKSTACK;
  memset(&p->context, 0, sizeof(p->context));
  p->context.ra = (uint64)forkret;
  p->context.sp = p->kstack + PGSIZE;
  return p;
}
//释放进程内存
static void freeproc(struct proc *p)
{
  if(p->trapframe)
    kfree((void*)p->trapframe);
  p->trapframe = 0;
  if (p->kpagetable) {
    kvmfree(p->kpagetable, 1);
  }
  p->kpagetable = 0;
  if(p->pagetable) proc_freepagetable(p->pagetable, p->sz);
  p->pagetable = 0;
  p->sz = 0;
  p->pid = 0;
  p->parent = 0;
  p->name[0] = 0;
  p->chan = 0;
  p->killed = 0;
  p->xstate = 0;
  p->state = UNUSED;
}


pagetable_t proc_pagetable(struct proc *p)
{
  pagetable_t pagetable;

  // An empty page table.
  pagetable = uvmcreate();
  if(pagetable == 0)
    return NULL;

  if(mappages(pagetable, TRAMPOLINE, PGSIZE,
              (uint64)trampoline, PTE_R | PTE_X) < 0){
    uvmfree(pagetable, 0);
    return NULL;
  }
  if(mappages(pagetable, TRAPFRAME, PGSIZE,
              (uint64)(p->trapframe), PTE_R | PTE_W) < 0){
    vmunmap(pagetable, TRAMPOLINE, 1, 0);
    uvmfree(pagetable, 0);
    return NULL;
  
  return pagetable;
}

void proc_freepagetable(pagetable_t pagetable, uint64 sz)
{
  vmunmap(pagetable, TRAMPOLINE, 1, 0);
  vmunmap(pagetable, TRAPFRAME, 1, 0);
  uvmfree(pagetable, sz);
}

uchar initcode[] = {
  0x17, 0x05, 0x00, 0x00, 0x13, 0x05, 0x45, 0x02,
  0x97, 0x05, 0x00, 0x00, 0x93, 0x85, 0x35, 0x02,
  0x93, 0x08, 0x70, 0x00, 0x73, 0x00, 0x00, 0x00,
  0x93, 0x08, 0x20, 0x00, 0x73, 0x00, 0x00, 0x00,
  0xef, 0xf0, 0x9f, 0xff, 0x2f, 0x69, 0x6e, 0x69,
  0x74, 0x00, 0x00, 0x24, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00
};

void userinit(void)
{
  struct proc *p;
  p = allocproc();
  initproc = p;
  uvminit(p->pagetable , p->kpagetable, initcode, sizeof(initcode));
  p->sz = PGSIZE;
  p->trapframe->epc = 0x0;     
  p->trapframe->sp = PGSIZE;  
  safestrcpy(p->name, "initcode", sizeof(p->name));
  p->state = RUNNABLE;
  p->tmask = 0;
  release(&p->lock);
}

// 扩展或缩小n字节的进程用户内存
// 返回0成功，返回-1失败
int
growproc(int n)
{
  uint sz;
  struct proc *p = curproc();

  sz = p->sz;
  if(n > 0){
    if((sz = uvmalloc(p->pagetable, p->kpagetable, sz, sz + n)) == 0) {
      return -1;
    }
  } else if(n < 0){
    sz = uvmdealloc(p->pagetable, p->kpagetable, sz, sz + n);
  }
  p->sz = sz;
  return 0;
}

// 创建一个复制当前进程的新进程
int
fork(void)
{
  int i, pid;
  struct proc *np;
  struct proc *p = curproc();

  if((np = allocproc()) == NULL){
    return -1;
  }
  if(uvmcopy(p->pagetable, np->pagetable, np->kpagetable, p->sz) < 0){
    freeproc(np);
    release(&np->lock);
    return -1;
  }
  np->sz = p->sz;
  np->parent = p;
  np->tmask = p->tmask;
  *(np->trapframe) = *(p->trapframe);
  np->trapframe->a0 = 0;
  for(i = 0; i < NOFILE; i++)
    if(p->ofile[i])
      np->ofile[i] = filedup(p->ofile[i]);
  np->cwd = edup(p->cwd);
  safestrcpy(np->name, p->name, sizeof(p->name));
  pid = np->pid;
  np->state = RUNNABLE;
  release(&np->lock);
  return pid;
}

// 将当前进程的废弃孩子传递给initproc
void
reparent(struct proc *p)
{
  struct proc *pp;
  for(pp = proc; pp < &proc[NPROC]; pp++){
    if(pp->parent == p){
      acquire(&pp->lock);
      release(&pp->lock);
    }
  }
}

// 推出当前进程，无返回
// 一个已经返回的进程保持在僵尸状态
// 直到父进程进行wait()后才会解除
void
exit(int status)
{
  struct proc *p = curproc();

  if(p == initproc)
    panic("init exiting");

  // 关闭所有文件
  for(int fd = 0; fd < NOFILE; fd++){
    if(p->ofile[fd]){
      struct file *f = p->ofile[fd];
      fileclose(f);
      p->ofile[fd] = 0;
    }
  }

  eput(p->cwd);
  p->cwd = 0;
  acquire(&initproc->lock);
  wakeup1(initproc);
  release(&initproc->lock);
  acquire(&p->lock);
  struct proc *original_parent = p->parent;
  release(&p->lock);
  
  acquire(&original_parent->lock);

  acquire(&p->lock);

  reparent(p);

  wakeup1(original_parent);

  p->xstate = status;
  p->state = ZOMBIE;

  release(&original_parent->lock);

  sched();
  panic("zombie exit");
}

// 等待孩子进程退出并返回其进程号
// 如果没有孩子则返回-1
int
wait(uint64 addr)
{
  struct proc *np;
  int havekids, pid;
  struct proc *p = curproc();

  acquire(&p->lock);

  for(;;){
    havekids = 0;
    for(np = proc; np < &proc[NPROC]; np++){

      if(np->parent == p){
        acquire(&np->lock);
        havekids = 1;
        if(np->state == ZOMBIE){
          pid = np->pid;
          if(addr != 0 && copyout2(addr, (char *)&np->xstate, sizeof(np->xstate)) < 0) {
            release(&np->lock);
            release(&p->lock);
            return -1;
          }
          freeproc(np);
          release(&np->lock);
          release(&p->lock);
          return pid;
        }
        release(&np->lock);
      }
    }

    if(!havekids || p->killed){
      release(&p->lock);
      return -1;
    }
    
    // Wait for a child to exit.
    sleep(p, &p->lock);  //DOC: wait-sleep
  }
}

// 调度器
// 无返回值并一直循环
void
scheduler(void)
{
  struct proc *p;
  struct cpu *c = curcpu();
  extern pagetable_t kernel_pagetable;

  c->proc = 0;
  for(;;){
    intr_on();
    
    int found = 0;
    for(p = proc; p < &proc[NPROC]; p++) {
      acquire(&p->lock);
      if(p->state == RUNNABLE) {
        p->state = RUNNING;
        c->proc = p;
        w_satp(MAKE_SATP(p->kpagetable));
        sfence_vma();
        swtch(&c->context, &p->context);
        w_satp(MAKE_SATP(kernel_pagetable));
        sfence_vma();
        c->proc = 0;

        found = 1;
      }
      release(&p->lock);
    }
    if(found == 0) {
      intr_on();
      asm volatile("wfi");
    }
  }
}


void
sched(void)
{
  int intena;
  struct proc *p = curproc();

  if(!holding(&p->lock))
    panic("sched p->lock");
  if(curcpu()->noff != 1)
    panic("sched locks");
  if(p->state == RUNNING)
    panic("sched running");
  if(intr_get())
    panic("sched interruptible");

  intena = curcpu()->intena;
  swtch(&p->context, &curcpu()->context);
  curcpu()->intena = intena;
}

void
yield(void)
{
  struct proc *p = curproc();
  acquire(&p->lock);
  p->state = RUNNABLE;
  sched();
  release(&p->lock);
}


void
forkret(void)
{
  static int first = 1;

  release(&curproc()->lock);

  if (first) {
    first = 0;
    fat32_init();
    curproc()->cwd = ename("/");
  }

  usertrapret();
}

// 释放锁并在chan上睡眠（原子操作）.
void
sleep(void *chan, struct spinlock *lk)
{
  struct proc *p = curproc();
  
  if(lk != &p->lock){  //DOC: sleeplock0
    acquire(&p->lock);  //DOC: sleeplock1
    release(lk);
  }

  // Go to sleep.
  p->chan = chan;
  p->state = SLEEPING;

  sched();

  p->chan = 0;

  if(lk != &p->lock){
    release(&p->lock);
    acquire(lk);
  }
}

// 唤醒所有在chan上睡眠的进程
void
wakeup(void *chan)
{
  struct proc *p;

  for(p = proc; p < &proc[NPROC]; p++) {
    acquire(&p->lock);
    if(p->state == SLEEPING && p->chan == chan) {
      p->state = RUNNABLE;
    }
    release(&p->lock);
  }
}

// 如果在wait()中睡眠，唤醒它; 由exit()调用
static void
wakeup1(struct proc *p)
{
  if(!holding(&p->lock))
    panic("wakeup1");
  if(p->chan == p && p->state == SLEEPING) {
    p->state = RUNNABLE;
  }
}

// 将进程号为pid的进程kill掉
int
kill(int pid)
{
  struct proc *p;

  for(p = proc; p < &proc[NPROC]; p++){
    acquire(&p->lock);
    if(p->pid == pid){
      p->killed = 1;
      if(p->state == SLEEPING){
        p->state = RUNNABLE;
      }
      release(&p->lock);
      return 0;
    }
    release(&p->lock);
  }
  return -1;
}

// 根据user_dst将源数据复制内核地址或用户地址
// 成功则返回0，失败返回-1
int
either_copyout(int user_dst, uint64 dst, void *src, uint64 len)
{
  if(user_dst){
    return copyout2(dst, src, len);
  } else {
    memmove((char *)dst, src, len);
    return 0;
  }
}

// 根据user_dst将数据从内核地址或用户地址copy到dst中
// 成功则返回0，失败返回-1
int
either_copyin(void *dst, int user_src, uint64 src, uint64 len)
{
  if(user_src){
    return copyin2(dst, src, len);
  } else {
    memmove(dst, (char*)src, len);
    return 0;
  }
}

// 输出进程的状态
// 用于debug
void
procdump(void)
{
  static char *states[] = {
  [UNUSED]    "unused",
  [SLEEPING]  "sleep ",
  [RUNNABLE]  "runble",
  [RUNNING]   "run   ",
  [ZOMBIE]    "zombie"
  };
  struct proc *p;
  char *state;

  printf("\nPID\tSTATE\tNAME\tMEM\n");
  for(p = proc; p < &proc[NPROC]; p++){
    if(p->state == UNUSED)
      continue;
    if(p->state >= 0 && p->state < NELEM(states) && states[p->state])
      state = states[p->state];
    else
      state = "???";
    printf("%d\t%s\t%s\t%d", p->pid, state, p->name, p->sz);
    printf("\n");
  }
}
//统计当前进程数目
uint64
procnum(void)
{
  int num = 0;
  struct proc *p;

  for (p = proc; p < &proc[NPROC]; p++) {
    if (p->state != UNUSED) {
      num++;
    }
  }

  return num;
}

