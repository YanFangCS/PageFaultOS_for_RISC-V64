#include "syscall/syscall.h"
#include "syscall/sysinfo.h"
#include "proc/proc.h"
#include "libs/types.h"
#include "mm/vm.h"
#include "syscall/sysnum.h"
#include "mm/memlayout.h"
#include "libs/string.h"


static uint64 (*syscalls[400])(void);

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
        panic("syscall error");
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
extern uint64 sys_wait(void);
extern uint64 sys_wait4(void);
extern uint64 sys_gettimeofday(void);
extern uint64 sys_nanosleep(void);
extern uint64 sys_fork(void);
extern uint64 sys_open(void);



void syscall_init(){
    memset((void *)syscalls, 0, sizeof(uint64) *400);
    syscalls[SYS_getcwd] = sys_getcwd;
    syscalls[SYS_pipe2] = sys_pipe2;
    syscalls[SYS_execve] = sys_execve;
    syscalls[SYS_open] = sys_open;
    syscalls[SYS_write] = sys_write;
    syscalls[SYS_read] = sys_read;
    syscalls[SYS_dup] = sys_dup;
    syscalls[SYS_dup3] = sys_dup3;
    syscalls[SYS_getcwd] = sys_getcwd;
    syscalls[SYS_getpid] = sys_getpid;
    syscalls[SYS_getppid] = sys_getppid;
    syscalls[SYS_fork] = sys_fork;
    syscalls[SYS_clone] = sys_fork;
    syscalls[SYS_exit] = sys_exit;
    syscalls[SYS_wait] = sys_wait;
    syscalls[SYS_wait4] = sys_wait4;
    syscalls[SYS_chdir] = sys_chdir;
    syscalls[SYS_close] = sys_close;
    syscalls[SYS_mkdirat] = sys_mkdirat;
    syscalls[SYS_openat] = sys_openat;
    syscalls[SYS_sched_yield] = sys_sched_yield;
    syscalls[SYS_brk] = sys_brk;
    syscalls[SYS_uname] = sys_uname;
    syscalls[SYS_pipe2] = sys_pipe2;
    syscalls[SYS_getdents64] = sys_getdents64;
    syscalls[SYS_mount] = sys_mount;
    syscalls[SYS_umount2] = sys_umount2;
    syscalls[SYS_times] = sys_times;
    syscalls[SYS_gettimeofday] = sys_gettimeofday;
    syscalls[SYS_mmap] = sys_mmap;
    syscalls[SYS_munmap] = sys_munmap;
    syscalls[SYS_fstat] = sys_fstat;
    syscalls[SYS_unlinkat] = sys_unlinkat;
}