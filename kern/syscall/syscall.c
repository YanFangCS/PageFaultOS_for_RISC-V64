#include <syscall.h>
#include <sysinfo.h>
#include <proc.h>
#include <types.h>
#include <vm.h>
#include <sysnum.h>
#include <memlayout.h>

static inline uint64 argraw(int n){
    struct trapframe *tf = curproc()->trapframe;
    switch(n){
        case 0:
            return tf->a0;
        case 1:
            return tf->a1;
        case 2:
            return tf->a2;
        case 3:
            return tf->a3;
        case 4:
            return tf->a4;
        case 5:
            return tf->a5;
        case 6:
            return tf->a6;
        case 7:
            return tf->a7;    
    }
    panic("argraw: invalid register");
    return -1;
}

int fetchaddr(uint64 addr, uint64 *ip){
    struct proc * p = curproc();
    // invalid addr
    if(addr >= p->sz || (addr + sizeof(uint64) >= p->sz)){
        panic("fetchaddr: invalid addr");
        return -1;
    }
    if(copyin2((char *)ip, addr, sizeof(*ip)) != 0 ){
        panic("fetchaddr: cannot fetch");
        return -1;
    }
    return 0;
}

int fetchstr(uint64 addr, char *buf, int max){
    struct proc * p = curproc();
    // invalid addr
    if(addr >= p->sz || (addr + sizeof(uint64) >= p->sz)){
        panic("fetchstr: invalid addr");
        return -1;
    }
    if(copyinstr2(buf, addr, max) != 0){
        panic("fetchstr: cannot fetch");
        return -1;
    }
    return 0;
}

int argaddr(int n, uint64 * addr){
    *addr = argraw(n);
    return 0;
}

int argint(int n, int *addr){
    *addr = argraw(n);
    return 0;
}

int argstr(int n, char * buf, int max){
    uint64 addr;
    if(argaddr(n, &addr) < 0){
        return -1;
    }
    return fetchstr(addr, buf, max);
}


void syscall(void){
    int num;
    struct proc * p = curproc();

    num = p->trapframe->a7;
    if(num > 0 && num < NELEM(syscalls) && syscalls[num]){
        p->trapframe->a0 = syscalls[num]();

        if((p->tmask & (1 << num)) != 0){
            printf("pid %d: %s -> %d\n", p->pid, syscalls[num], p->trapframe->a0);
        }
    } else {
        printf("pid %d %s : unknown sys call %d\n" ,p->pid ,p->name, num);
        p->trapframe->a0 = -1;
    }
}


extern uint64 sys_read(void);
extern uint64 sys_write(void);
extern uint64 sys_close(void);
extern uint64 sys_fstat(void);
extern uint64 sys_open(void);
extern uint64 sys_mkdirat(void);
extern uint64 sys_chdir(void);
extern uint64 sys_pipe2(void);
extern uint64 sys_getcwd(void);
extern uint64 sys_getpid(void);
extern uint64 sys_getppid(void);
extern uint64 sys_execve(void);
extern uint64 sys_exit(void);
extern uint64 sys_brk(void);
extern uint64 sys_dup(void);
extern uint64 sys_dup3(void);
extern uint64 sys_openat(void);
extern uint64 sys_close(void);
extern uint64 sys_getdents64(void);
extern uint64 sys_linkat(void);
extern uint64 sys_unlinkat(void);
extern uint64 sys_umount2(void);
extern uint64 sys_mount(void);
extern uint64 sys_clone(void);
extern uint64 sys_munmap(void);
extern uint64 sys_mmap(void);
extern uint64 sys_times(void);
extern uint64 sys_uname(void);
extern uint64 sys_sched_yield(void);
extern uint64 sys_wait4(void);
extern uint64 sys_gettimeofday(void);
extern uint64 sys_nanosleep(void);
extern uint64 sys_fork(void);

static uint64 (*syscalls[])(void) = {
    [SYS_getcwd]       sys_getcwd,
    [SYS_pipe2]        sys_pipe2,
    [SYS_dup]          sys_dup,
    [SYS_dup3]         sys_dup3,
    [SYS_chdir]        sys_chdir,
    [SYS_openat]       sys_openat,
    [SYS_close]        sys_close,
    [SYS_getdents64]   sys_getdents64,
    [SYS_read]         sys_read,
    [SYS_write]        sys_write,
    [SYS_linkat]       sys_linkat,
    [SYS_unlinkat]     sys_unlinkat,
    [SYS_mkdirat]      sys_mkdirat,
    [SYS_umount2]      sys_umount2,
    [SYS_mount]        sys_mount,
    [SYS_fstat]        sys_fstat,
    [SYS_clone]        sys_clone,
    [SYS_execve]       sys_execve,
    [SYS_wait4]        sys_wait4,
    [SYS_exit]         sys_exit,
    [SYS_getppid]      sys_getppid,
    [SYS_getpid]       sys_getpid,
    [SYS_brk]          sys_brk,
    [SYS_munmap]       sys_munmap,
    [SYS_mmap]         sys_mmap,
    [SYS_times]        sys_times,
    [SYS_uname]        sys_uname,
    [SYS_sched_yield]  sys_sched_yield,
    [SYS_gettimeofday] sys_gettimeofday,
    [SYS_nanosleep]    sys_nanosleep,
    [SYS_fork]         sys_fork,
};

static char * syscallnames[] = {
    [SYS_getcwd]        "getcwd",
    [SYS_pipe2]         "pipe2",
    [SYS_dup]           "dup",
    [SYS_dup3]          "dup3",
    [SYS_chdir]         "chdir",
    [SYS_openat]        "openat",
    [SYS_close]         "close",
    [SYS_getdents64]    "getdents64",
    [SYS_read]          "read",
    [SYS_write]         "write",
    [SYS_linkat]        "linkat",
    [SYS_unlinkat]      "unlinkat",
    [SYS_mkdirat]       "mkdirat",
    [SYS_umount2]       "umount2",
    [SYS_mount]         "mount",
    [SYS_fstat]         "fstat",
    [SYS_clone]         "clone",
    [SYS_execve]        "exceve",
    [SYS_wait4]         "wait4",
    [SYS_exit]          "exit",
    [SYS_getpid]        "getpid",
    [SYS_getppid]       "getppid",
    [SYS_brk]           "brk",
    [SYS_munmap]        "munmap",
    [SYS_mmap]          "mmap",
    [SYS_times]         "times",
    [SYS_uname]         "uname",
    [SYS_sched_yield]   "sched_yield",
    [SYS_gettimeofday]  "gettimeofday",
    [SYS_nanosleep]     "nanosleep",
    [SYS_fork]          "fork",
};


uint64 sys_sysinfo(void){
    uint64 addr;
    if(argaddr(0, &addr) < 0){
        return -1;
    }

    struct sysinfo info;
    info.freemem = freemem_amount();
    info.nums_of_proc = procnum();

    if(copyout2(addr, (char *)&info, sizeof(info)) < 0){
        return -1;
    }
    
    return 0;
}