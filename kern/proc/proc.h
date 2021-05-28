#ifndef __KERN_PROC_PROC_H__
#define __KERN_PROC_PROC_H__

#include <param.h>
#include <riscv.h>
#include <types.h>
#include <spinlock.h>
#include <file.h>
#include <fat32.h>
#include <trap.h>

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
    struct trapframe *trapframe; // 中断帧，trampoline.S保存的进程数据在这里
    struct context context;      // 上下文
    struct file *ofile[NOFILE];  // 用户打开的文件
    struct dirent *cwd;          // 当前目录
    char name[16];               // 进程名
    int tmask;                    // trace mask
};

void            reg_info(void);
int             cpuid(void);
void            exit(int);
int             fork(void);
int             growproc(int);
pagetable_t     proc_pagetable(struct proc *);
void            proc_freepagetable(pagetable_t, uint64);
int             kill(int);
struct cpu*     curcpu(void);
struct cpu*     getmycpu(void);
struct proc*    curproc();
void            procinit(void);
void            scheduler(void) __attribute__((noreturn));
void            sched(void);
void            setproc(struct proc*);
void            sleep(void*, struct spinlock*);
void            userinit(void);
int             wait(uint64);
void            wakeup(void*);
void            yield(void);
int             either_copyout(int user_dst, uint64 dst, void *src, uint64 len);
int             either_copyin(void *dst, int user_src, uint64 src, uint64 len);
void            procdump(void);
uint64          procnum(void);
void            test_proc_init(int);

#endif