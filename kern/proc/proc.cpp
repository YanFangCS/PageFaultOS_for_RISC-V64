#include "proc/proc.h"
#include "mm/memlayout.h"
#include "libs/param.h"
#include "libs/string.h"
#include "console/console.h"
#include "fs/fcntl.h"
#include "fs/file.h"
#include "driver/disk.h"
#include "driver/sdcard.h"
#include "fs/fat32.hpp"
#include "fs/buf.hpp"
#include "fs/vfs.h"
#include "mm/kalloc.h"
#include "trap/trap.h"
#include "sync/timer.h"
#include "trap/intr.h"
#include "mm/memlayout.h"
#include "libs/time.h"
#include "mm/vm.h"

extern struct console console;
extern BufferLayer bufferlayer;
extern Fat32FileSystem fatFS;

extern char trampoline[], uservec[], userret[];

#define NVMA 15

struct vma vma[NVMA];
struct proc proc[NPROC];

struct proc *initproc;
struct proc procTable[NPROC];
#define KSTACK_SIZE (PGSIZE * 2)
alignas(4096) char stack[KSTACK_SIZE * 2 * (NPROC + 1)];

int nextpid = 1;
struct spinlock pid_lock;

struct proc *allocproc();

extern char trampoline[]; // trampoline.S

void reg_info(void) {
  printf("register info: {\n");
  printf("sstatus: %p\n", r_sstatus());
  printf("sip: %p\n", r_sip());
  printf("sie: %p\n", r_sie());
  printf("sepc: %p\n", r_sepc());
  printf("stvec: %p\n", r_stvec());
  printf("satp: %p\n", r_satp());
  printf("scause: %p\n", r_scause());
  printf("stval: %p\n", r_stval());
  printf("sp: %p\n", r_sp());
  printf("tp: %p\n", r_tp());
  printf("ra: %p\n", r_ra());
  printf("}\n");
}

int cpuid(){
    int id = r_tp();
    return id;
}

struct cpu * curcpu(){
    int id = cpuid();
    struct cpu * c = &cpus[id];
    return c;
}

struct proc * curproc(){
    push_off();
    struct proc * p = curcpu()->proc;
    pop_off();
    return p;
}

void initProcTable(){
    struct proc * p;
    for(int i = 0; i < NPROC; ++i){
        p = &procTable[i];
        initlock(&(p->lock), (char *)"task");
        p->pid = i;
        // task->kstack = (uint64) kalloc();
        p->kstack = (uint64)(stack + KSTACK_SIZE * i);
        p->trapframe = 0;
        p->state = UNUSED;
        p->killed = 0;
        p->xstate = 0;
        p->sz = 0;
        p->mticks = 0;
        memset(p->vma, 0, sizeof(struct vma *) * NOMMAPFILE);
        p->uticks = 0;
        memset(p->workDir, 0, MAXPATH);
        memset(p->ofile, 0, sizeof(struct file *) * NOFILE);
    }
}

/*
0000000 17 05 00 00 13 05 45 02 97 05 00 00 93 85 35 02
0000020 93 08 d0 0d 73 00 00 00 9b 08 d0 05 73 00 00 00
0000040 ef f0 9f ff 2f 69 6e 69 74 00 00 24 00 00 00 00
0000060 00 00 00 00
0000064
*/

alignas(PGSIZE) uchar initcode[] = {
    0x17, 0x05, 0x00, 0x00, 0x13, 0x05, 0x45, 0x02, 0x97, 0x05, 0x00, 0x00, 0x93,
    0x85, 0x35, 0x02, 0x93, 0x08, 0xd0, 0x0d, 0x73, 0x00, 0x00, 0x00, 0x9b, 0x08,
    0xd0, 0x05, 0x73, 0x00, 0x00, 0x00, 0xef, 0xf0, 0x9f, 0xff, 0x2f, 0x69, 0x6e,
    0x69, 0x74, 0x00, 0x00, 0x24, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

void initFirstProc(){
    struct proc * p = allocproc();
    UvmInit(p->pagetable, p->kpagetable, initcode, sizeof(initcode));
    p->sz = PGSIZE;
    p->trapframe->epc = 0x40;
    p->trapframe->sp = PGSIZE;

    memmove(p->name, "initcode", sizeof(p->name));
    p->workDir[0] = '/';
    p->state = RUNNABLE;
    initproc = p;
}

void
procinit(void)
{
    struct proc *p;
  
    initlock(&pid_lock, (char *)"nextpid");
    for(p = proc; p < &proc[NPROC]; p++) {
        initlock(&p->lock, (char *)"proc");
    }

    memset(cpus, 0, sizeof(cpus));
    #ifdef DEBUG
    printf("procinit\n");
    #endif
}

// fork的子进程的会从此处开始执行
void forkret(void) {
    static int first = 1;

    // 这里需要释放进程锁
    release(&(curproc()->lock));

    if (first) {
    // File system initialization must be run in the context of a
    // regular process (e.g., because it calls sleep), and thus cannot
    // be run from main().
    //
        first = 0;
        vfsinit();
        // char buf[1000];
        // memset(buf, 0, 1000);
        // int fd = vfsopen("/", O_RDONLY);
        // // LOG_DEBUG("fd=%d",fd);
        // // while (1)
    // // {
    // //   /* code */
    // // }

    // vfsls(fd, buf, false);
    // struct dirent *dt = (struct dirent *)buf;

    // LOG_DEBUG("name=%s", ((struct dirent *)(dt->d_off))->d_name);
    // while (1)
    //   ;
    // ;
    }
    UserTrapRet();
}

// 分配一个进程，并设置其初始执行函数为forkret
struct proc *allocproc() {
    struct proc * p;
    for (p = procTable + 1; proc < &procTable[NPROC]; p++) {
        acquire(&p->lock);
        if (p->state == UNUSED) {
            goto found;
        } else {
            release(&p->lock);
        }
    }
    return 0;

found:
    if ((p->trapframe = (struct trapframe *)kalloc()) == 0) {
        release(&p->lock);
        return 0;
    }

    // 为进程创建页表
    p->pagetable = proc_pagetable(p);

    memset(&p->context, 0, sizeof(p->context));
    memset(p->trapframe, 0, sizeof(*p->trapframe));

    p->context.sp = p->kstack + PGSIZE;
    p->context.ra = (uint64)forkret;
    release(&(p->lock));
    return p;
}

/**
 *
 * 创建一个进程可以使用的pagetable, 只映射了trampoline页,
 * 用于进入和离开内核空间
 *
 * @return
 */
pagetable_t proc_pagetable(struct proc *p) {
    pagetable_t pagetable;

    // 创建一个空的页表
    pagetable = Uvmcreate();
    if (pagetable == 0) return 0;

    // 映射trampoline代码(用于系统调用)到虚拟地址的顶端
    if (MapPages(pagetable, TRAMPOLINE, PGSIZE, (uint64)trampoline, PTE_R | PTE_X) < 0) {
        Uvmfree(pagetable, 0);
        return 0;
    }
    // 将进程的trapframe映射到TRAPFRAME, TRAMPOLINE的低位一页
    if (MapPages(pagetable, TRAPFRAME, PGSIZE, (uint64)(p->trapframe), PTE_R | PTE_W) < 0) {
        vmunmap(pagetable, TRAMPOLINE, 1, 0);
        Uvmfree(pagetable, 0);
        return 0;
    }
    return pagetable;
}

/**
 * 调度函数，for循环寻找RUNABLE的进程，
 * 并执行，当使用只有一个进程时(shell),
 * 使CPU进入低功率模式。
 *  内核调度线程将一直执行该函数
 */
void scheduler() {
    struct proc *p;
    struct cpu * c = curcpu();
    int alive = 0;
    c->proc = 0;
    for (;;) {
        intr_on();
        alive = 0;
        for (int i = 0; i < NPROC; i++) {
            p = &procTable[i];
            acquire(&(p->lock));
            if (p->state != UNUSED && p->state != ZOMBIE) {
                alive++;
            }
            if (p->state == ZOMBIE) {
                wakeup(initproc);
            }
            if (p->state == RUNNABLE) {
                p->state = RUNNING;
                c->proc = p;
                pswitch(&c->context, &p->context);
                c->proc = 0;
            }
            release(&(p->lock));
        }
        if (alive <= 2) {
            intr_on();
            asm volatile("wfi");
        }
    }
}

void sleep(void *chan, struct spinlock *lock) {
    struct proc * p = curproc();

    // 由于要改变p->state所以需要持有p->proc_lock, 然后
    // 调用before_sched。只要持有了p->proc_lock，就能够保证不会
    // 丢失wakeup(wakeup 会锁定p->proc_lock)，
    // 所以解锁lock是可以的
    if (lock != &p->lock) {  // DOC: sleeplock0
        acquire(&(p->lock));
        release(lock);
    }
    // sleep
    p->chan = chan;
    p->state = SLEEPING;

    prepareSched();

    // 重置chan
    p->chan = 0;

    // Reacquire original lock.
    if (lock != &p->lock) {
        release(&(p->lock));
        acquire(lock);
    }
}

void prepareSched() {
    int intr_enable;
    struct proc * p = curproc();

    if (holding(&(p->lock))) panic("sched p->lock");
    if (curcpu()->noff != 1) panic("sched locks");
    if (p->state == RUNNING) panic("sched running");
    if (intr_get()) panic("sched interruptible");

    intr_enable = curcpu()->intena;
    // LOG_INFO("enter switch cpuid=%d task=%p gp=%d", Cpu::cpuid(), Cpu::curcpu()->task, r_gp());
    pswitch(&p->context, &(curcpu()->context));
    // LOG_INFO("leave switch cpuid=%d task=%p gp=%d", Cpu::cpuid(), Cpu::curcpu()->task, r_gp());
    // LOG_DEBUG("epc=%p", myTask()->trapframe->epc);
    curcpu()->intena = intr_enable;
}

// 睡眠一定时间
void sleepTime(uint64 sleep_ticks) {
    uint64 now = ticks;
    acquire(&tickslock);
    for (; ticks - now < sleep_ticks;) {
        sleep(&ticks, &tickslock);
    }
    release(&tickslock);
}

// 唤醒指定chan上的进程
void wakeup(void *chan) {
    struct proc * p;
    for (p = procTable; p < &procTable[NPROC]; p++) {
    // task->lock.lock();
        if (p->state == SLEEPING && p->chan == chan) {
            p->state = RUNNABLE;
        }
    // task->lock.unlock();
    }
}

int fork() {
    struct proc * child;
    struct proc * p = curproc();
    // 分配一个新的进程
    if ((child = allocproc()) == 0) {
        return -1;
    }

  // 将父进程的内存复制到子进程中
    if (Uvmcopy(p->pagetable, child->pagetable, child->kpagetable, p->sz) < 0) {
        return -1;
    }
    child->sz = p->sz;
    child->parent = p;
  // child->trapframe->ra = MAXVA;

  // 复制父进程的用户空间的寄存器
    *(child->trapframe) = *(p->trapframe);
  // memmove(child->trapframe, task->trapframe, sizeof(struct trapframe));
  // 设置子进程fork的返回值为0
    child->trapframe->a0 = 0;

  // 复制文件资源
    for (int i = 0; i < NOFILE; i++) {
        if (p->ofile[i] != 0) {
            child->ofile[i] = vfsdup(p->ofile[i]);
        }
    }

    struct vma *vma, *childVma;
    for (int i = 0; i < NOMMAPFILE; i++) {
        vma = p->vma[i];
        child->vma[i] = 0;
        if (vma) {
            childVma = allocVma();
            *(childVma) = *(vma);
            vfsdup(vma->f);
            child->vma[i] = childVma;
        }
    }

    safestrcpy(child->workDir, p->workDir, strlen(p->workDir) + 1);

    safestrcpy(child->name, p->name, sizeof(p->name) + 1);

    child->state = RUNNABLE;
    return child->pid;
}

int clone(uint64 stack, int flags) {
    struct proc * child;
    struct proc * p = curproc();
    // 分配一个新的进程
    if ((child = allocproc()) == 0) {
        return -1;
    }

    // 将父进程的内存复制到子进程中
    if (Uvmcopy(p->pagetable, child->pagetable, child->kpagetable, p->sz) < 0) {
        return -1;
    }
    child->sz = p->sz;
    child->parent = p;
    // 复制父进程的用户空间的寄存器
    *(child->trapframe) = *(p->trapframe);
    // memmove(child->trapframe, task->trapframe, sizeof(struct trapframe));
    // 设置子进程fork的返回值为0
    child->trapframe->a0 = 0;

    // 复制文件资源
    for (int i = 0; i < NOFILE; i++) {
        if (p->ofile[i] != 0) {
            child->ofile[i] = vfsdup(p->ofile[i]);
        }
    }

  // 复制vma
    struct vma *vma, *childVma;
    for (int i = 0; i < NOMMAPFILE; i++) {
        vma = p->vma[i];
        child->vma[i] = 0;
        if (vma) {
            childVma = allocVma();
            *(childVma) = *(vma);
            vfsdup(vma->f);
            child->vma[i] = childVma;
        }
    }

    safestrcpy(child->workDir, p->workDir, strlen(p->workDir) + 1);

    safestrcpy(child->name, p->name, sizeof(p->name) + 1);

    child->trapframe->sp = stack;

    child->state = RUNNABLE;
    return child->pid;
}

// int clone() {
//   // Task *child;
//   // Task *task = myTask();
// }

/**
 * @brief 增加或者减少用户内存n字节, 该函数
 * 不会释放页表，只是的unmap
 *
 * @param n 大于0增加，小于0减少
 * @return int 成功返回0，失败返回-1
 */
int growproc(int n) {
    uint sz;
    struct proc * p = curproc();
    sz = p->sz;
    if (n > 0) {
        if ((sz = Uvmalloc(p->pagetable, p->kpagetable, sz, sz + n)) == 0) {
            return -1;
        }
    } else if (n < 0) {
        sz = UvmDealloc(p->pagetable, p->kpagetable, sz, sz + n);
    }
    p->sz = sz;
    return 0;
}

void freeProcPagetable(pagetable_t pagetable, uint64 sz) {
    vmunmap(pagetable, TRAMPOLINE, 1, 0);
    vmunmap(pagetable, TRAPFRAME, 1, 0);
    Uvmfree(pagetable, sz);
}

static void freeproc(struct proc *p) {
    if (p->trapframe) kfree((void *)p->trapframe);
    p->trapframe = 0;
    if (p->pagetable != 0) {
        freeProcPagetable(p->pagetable, p->sz);
    }
    memset(p->vma, 0, sizeof(struct vma *) * NOMMAPFILE);
    p->pagetable = 0;
    p->sz = 0;
    p->name[0] = 0;
    p->chan = 0;
    p->killed = 0;
    p->xstate = 0;
    p->state = UNUSED;
    p->parent = 0;
}

/**
 * 等待子进程退出, 返回其子进程id
 * 没有子进程返回-1， 将退出状态复
 * 制到status中。
 */
int wait(uint64 vaddr) {
    struct proc *child;  // 子进程
    struct proc *p;
    int pid;
    bool havechild;
    p = curproc();
    acquire(&(p->lock));
    for (;;) {
        havechild = 0;
        for (child = procTable; child < &procTable[NPROC]; child++) {
            if (child->parent == p) {
                acquire(&(child->lock));
                havechild = 1;
                if (child->state == ZOMBIE) {
                    pid = child->pid;
                    if (vaddr != 0 && copyout(p->pagetable, vaddr, (char *)&child->xstate, sizeof(child->xstate)) < 0) {
                        release(&(child->lock));
                        release(&(p->lock));
                        return -1;
                    }
                    freeproc(child);
                    release(&(child->lock));
                    release(&(p->lock));
                    return pid;
                }
                release(&(child->lock));
            }
        }
        if (!havechild) {
            return -1;
        }
        sleep(p, &(curproc()->lock));  // 等待子进程唤醒
    }
}

/**
 * @brief 等待某一子进程
 *
 * @param pid 子进程id
 * @param vaddr status地址
 * @return int
 */
int wait4(int pid, uint64 vaddr) {
    if (pid >= NPROC) {
        return -1;
    }

    struct proc *p = curproc();
    struct proc *child = &procTable[pid];
    acquire(&(child->lock));
    if (child->parent != p) {
        release(&(child->lock));
        return -1;
    }
    release(&(child->lock));
    acquire(&(p->lock));
    while (1) {
        acquire(&(child->lock));
        child->xstate = child->xstate << 8;
        if (child->state == ZOMBIE) {
            if (vaddr != 0 && copyout(p->pagetable, vaddr, (char *)&child->xstate, sizeof(child->xstate)) < 0) {
                release(&(child->lock));
                release(&(p->lock));
                return -1;
            }
            freeproc(child);
            release(&(child->lock));
            release(&(p->lock));
            return pid;
        }
        release(&(child->lock));
        sleep(p, &(curproc()->lock));  // 等待子进程唤醒
    }
}

//
// 进程退出，并释放资源
//
// 这里将state设置为ZOMBIE,
// 让父进程来设置其state为UNUSED
// 若父进程已经exit, 则会由init进
// 程来完成父进程在exit时，会将其
// 子进程的parent设置为init进程
//
void exit(int status) {
    struct proc *p, *child;
    p = curproc();
    // 关闭打开的文件
    for (int fd = 0; fd < NOFILE; fd++) {
        if (p->ofile[fd] != NULL) {
            vfsclose(p->ofile[fd]);
            p->ofile[fd] = 0;
        }   
    }

  // 归还当前目录inode
  // putback_inode(task->current_dir);
    memset(p->workDir, 0, MAXPATH);
  // 将子进程托付给init进程
    for (child = procTable; child < &procTable[NPROC]; child++) {
        if (child->parent == p) {
            child->parent = initproc;
        }
    }

    struct vma *vma;
    for (int i = 0; i < NOMMAPFILE; i++) {
        vma = p->vma[i];
        if (vma) {
            if (vma->flag & MAP_SHARED) {
                vfsrewind(vma->f);
                vfswrite(vma->f, 1, (const char *)(vma->addr), vma->length, 0);
            }
            vmunmap(p->pagetable, vma->addr, PGROUNDUP(vma->length) / PGSIZE, 0);
      // uvmunmap(p->pagetable, vma->addr, PGROUNDUP(vma->length) / PGSIZE, 0);
            vfsclose(vma->f);
            freeVma(vma);
        }
    }
    p->state = ZOMBIE;
    p->xstate = status;
    wakeup(p->parent);
    acquire(&(p->lock));
    prepareSched();
    panic("exit");
}

//
// 让出cpu
//
void yield() {
    struct proc *p = curproc();
    acquire(&(p->lock));
    p->state = RUNNABLE;
    prepareSched();
    release(&(p->lock));
}

/**
 *  根据user_dst将源数据复制内核地址或用户地址
 *  @param user_dst dst是否为用户空间地址
 *  @param copy的长度
 * @return 成功返回0，失败返回-1
 */
int either_copyout(int user_dst, uint64 dst, void *src, uint64 len) {
    struct proc *p = curproc();
    if (user_dst) {
        return copyout(p->pagetable, dst, (char *)(src), len);
    } else {
        memmove((char *)dst, src, len);
        return 0;
    } 
}

/**
 *  根据user_dst将数据从内核地址或用户地址copy到dst中
 *  @param user_src dst是否为用户空间地址
 *  @param len copy的长度
 * @return 成功返回0，失败返回-1
 */
int either_copyin(void *dst, int user_src, uint64 src, uint64 len) {
    struct proc *p = curproc();
    if (user_src) {
        return copyin(p->pagetable, (char *)(dst), src, len);
    } else {
        memmove(dst, (char *)src, len);
        return 0;
    }
}

/**
 * @brief 将fp添加到进程的打开文件列表中
 *
 * @param fp 需要添加的文件指针
 * @return 成功返回文件描述符，失败返回-1
 */
int registFileHandle(struct file *fp, int fd) {
    if (fd > NFILE) {
        return -1;
    }
    struct proc *p = curproc();
    acquire(&(p->lock));
    if (fd < 0) {
        for (int i = 0; i < NOFILE; i++) {
            if (p->ofile[i] == NULL) {
                p->ofile[i] = fp;
                release(&(p->lock));
                return i;
            }
        }
        panic("registFileHandle");
    }
    if (p->ofile[fd] == NULL) {
        p->ofile[fd] = fp;
        release(&(p->lock));
        return fd;
    }
    release(&(p->lock));
    return -1;
}

/**
 * @brief 获取fd对应的file
 *
 * @param fd 文件描述符
 * @return struct file* 对应的文件描述符
 */
struct file *getFileByFd(int fd) {
    if (fd < 0 || fd > NOFILE) return NULL;

    struct proc * p = curproc();
    acquire(&(p->lock));
    struct file *fp = curproc()->ofile[fd];
    release(&(p->lock));
    return fp;
}

int procTimes(struct tms *tm) {
    struct proc * p = curproc();
    struct proc * child;
    memset(tm, 0, sizeof(struct tms));
    tm->tms_stime = p->mticks;
    tm->tms_utime = p->uticks;

    for (child = procTable; child < &procTable[NPROC]; child++) {
        acquire(&(child->lock));
        if (child->parent == p) {
            tm->tms_cutime += child->uticks;
            tm->tms_cstime += child->mticks;
        }
        release(&(child->lock));
    }
    return 0;
}

struct vma *allocVma() {
    struct vma *a;
    for (a = vma; a < vma + NVMA; a++) {
        if (a->f == 0) {
            return a;
        }
    }
    return 0;
}

void freeVma(struct vma *a) { memset(a, 0, sizeof(struct vma)); }
