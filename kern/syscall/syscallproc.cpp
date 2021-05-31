#include "driver/clock.h"
#include "libs/string.h"
#include "mm/kalloc.h"
#include "proc/proc.h"
#include "syscall/syscall.h"
#include "libs/time.h"
#include "fs/vfs.h"
#include "sync/timer.h"
#include "libs/param.h"
#include "libs/riscv.h"
#include "libs/types.h"
#include "mm/vm.h"

uint64 sys_exit(void){
    int n;
    if (argint(0, &n) < 0) return -1;
    exit(n);
    return 0;
}

uint64 sys_fork(void){
    int flags;
    uint64 stackHighAddr;
    if( argint(0, &flags) < 0 || argaddr(1, &stackHighAddr) < 0)
        return -1;
    if(stackHighAddr == 0) return fork();
    else return clone(stackHighAddr, flags);
}

uint64 sys_execve(void){
    char path[MAXPATH], *argv[MAXARG];
    uint64 uargv, uarg = 0;
    int ret = 0;
    if (argstr(0, path, MAXPATH) < 0 || argaddr(1, &uargv) < 0) return -1;
    if (strlen(path) < 1) return -1;
    memset(argv, 0, sizeof(argv));
    for (int i = 0;; i++) {
        if (i > MAXARG) goto bad;
        if (uargv != 0 && fetchaddr(uargv + sizeof(uint64) * i, &uarg) < 0) goto bad;
        if (uarg == 0 || uargv == 0) {
            argv[i] = 0;
            break;
        }
        argv[i] = (char *)(kalloc());
        if (argv[i] == 0) goto bad;
        if (fetchstr(uarg, argv[i], PGSIZE) < 0) goto bad;
    }
    vfscalAbPath(path);
    ret = exec(path, argv);
    for (int i = 0; i <= MAXARG && argv[i] != 0; i++) kfree(argv[i]);
    return ret;

bad:
    for (int i = 0; i <= MAXARG && argv[i] != 0; i++) kfree(argv[i]);
    return -1;
}


uint64 sys_getpid(void){
    return curproc()->pid;
}

uint64 sys_getppid(void){
    return curproc()->parent->pid;
}

uint64 sys_wait(void){
    int pid;
    uint64 vaddr;
    if(argint(0, &pid) < 0|| argaddr(0, &vaddr) < 0) return -1;
    if(pid == -1) return wait(vaddr);
    return wait4(pid, vaddr);
}

uint64 sys_wait4(void){
    int pid;
    uint64 uaddr;
    if (argint(0, &pid) < 0 || argaddr(1, &uaddr) < 0) {
        return -1;
    }
    if (pid == -1) {
        return wait(uaddr);
    }
    return wait4(pid, uaddr);
}

uint64 sys_sched_yield(){
    yield();
    return 0;
}

uint64 sys_brk(void){
    uint64 addr;
    if( argaddr(0, &addr) < 0) return -1;
    struct proc * p = curproc();
    if(addr == 0) return p->sz;
    growproc(addr - p->sz);
    return p->sz;
}

uint64 sys_times(void){
    struct tms tm;
    uint64 utmaddr;
    int start = ticks;
    if(argaddr(0, &utmaddr) < 0) return -1;
    procTimes(&tm);
    copyout(curproc()->pagetable, utmaddr, (char *)&tm, sizeof(struct tms));
    return ticks - start;
}

uint64 sys_gettimeofday(){
    uint64 addr;
    if(argaddr(0, &addr) < 0){
        return -1;
    }
    TimeVal tm;
    int i = 4000000;
    while(i-- > 0) ;
#ifdef K210
    tm.sec = getTimeStamp();
    tm.usec = 0;
#endif 
    copyout(curproc()->pagetable, addr, (char *)(&tm), sizeof(TimeVal));
    return 0;
}

uint64 sys_uname(void) {
    uint64 addr;
    if( argaddr(0, &addr) < 0) return -1;

    return 0;
}