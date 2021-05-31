#ifndef __KERN_PROC_PROC_HPP__
#define __KERN_PROC_PROC_HPP__

#include "libs/param.h"
#include "libs/riscv.h"
#include "libs/types.h"
#include "sync/spinlock.h"
#include "fs/file.h"
#include "fs/fat32.hpp"
#include "trap/trap.h"

// Saved registers for kernel context switches.
struct context {
    uint64 ra;
    uint64 sp;

    // callee-saved
    uint64 s0;
    uint64 s1;
    uint64 s2;
    uint64 s3;
    uint64 s4;
    uint64 s5;
    uint64 s6;
    uint64 s7;
    uint64 s8;
    uint64 s9;
    uint64 s10;
    uint64 s11;
};

struct vma {
    uint64 addr;
    int length;
    int prot;
    int flag;
    struct file * f;
};

// Per-CPU state.
struct cpu {
    struct proc *proc;          // The process running on this cpu, or null.
    struct context context;     // swtch() here to enter scheduler().
    int noff;                   // Depth of push_off() nesting.
    int intena;                 // Were interrupts enabled before push_off()?
};

extern struct cpu cpus[NCPU];

enum procstate { UNUSED, SLEEPING, RUNNABLE, RUNNING, ZOMBIE };

// Per-process state
struct proc {
    struct spinlock lock;         //进程锁

    // p->lock must be held when using these:
    enum procstate state;        // 进程状态
    struct proc *parent;         // 父进程
    void *chan;                  // 如果非空，在chan上睡眠
    int killed;                  // 如果非空，将killed
    int xstate;                  // 返回给父进程的退出状态
    int pid;                     // 进程号
    // these are private to the process, so p->lock need not be held.
    uint64 kstack;               // 进程的内核空间栈
    uint64 sz;                   // 进程使用的空间大小
    pagetable_t pagetable;       // 用户页表
    pagetable_t kpagetable;      // 内核页表
    int mticks;
    int uticks;
    struct trapframe *trapframe; // 中断帧，trampoline.S保存的进程数据在这里
    struct context context;      // 上下文
    struct vma * vma[NOMMAPFILE];
    struct file *ofile[NOFILE];  // 用户打开的文件
    char workDir[MAXPATH];          // 当前目录
    char name[16];               // 进程名
    int tmask;                    // trace mask
};


extern "C" void pswitch(struct context * , struct context *);

void            initProcTable(void);
void            reg_info(void);
int             cpuid(void);
void            exit(int);
int             exec(char * path, char ** argv);
int             fork(void);
int             clone(uint64 stack, int flags);
int             growproc(int);
pagetable_t     proc_pagetable(struct proc *);
void            freeProcPagetable(pagetable_t, uint64);
int             kill(int);
struct cpu*     curcpu(void);
struct cpu*     getcurcpu(void);
struct proc*    curproc();
void            procinit(void);
void            scheduler(void) __attribute__((noreturn));
void            sched(void);
void            setproc(struct proc*);
void            sleep(void*, struct spinlock*);
int             wait(uint64);
int             wait4(int pid, uint64 vaddr);
void            wakeup(void*);
void            yield(void);
int             either_copyout(int user_dst, uint64 dst, void *src, uint64 len);
int             either_copyin(void *dst, int user_src, uint64 src, uint64 len);
void            procdump(void);
uint64          procnum(void);
int             clone(uint64 stack, int flags);
int             registFileHandle(struct file *fp, int fd = -1);
void            prepareSched();
struct file *   getFileByFd(int fd);
struct vma *    allocVma();
void            freeVma(struct vma * a);
void            initFirstProc(void);
struct proc *   allocproc();
int             procTimes(struct tms *tm);

#endif