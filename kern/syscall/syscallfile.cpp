#include "libs/types.h"
#include "libs/riscv.h"
#include "libs/param.h"
#include "libs/stat.h"
#include "sync/spinlock.h"
#include "proc/proc.h"
#include "proc/pipe.hpp"
#include "fs/file.h"
#include "fs/fat32.hpp"
#include "fs/fcntl.h"
#include "syscall/syscall.h"
#include "fs/vfs.h"
#include "mm/vm.h"

static int argfd(int n, int *pfd, struct file **pf){
    int fd;
    struct file *f;

    if(argint(n, &fd) < 0)  return -1;
    if(fd < 0 || fd >= NOFILE || (f = curproc()->ofile[fd]) == NULL) return -1;
    if(pfd) *pfd = fd;
    if(pf) *pf = f;
    return 0;
}

uint64 sys_read(void){
    int fd, n;
    uint64 uaddr;
  // LOG_DEBUG("sys_read");
    if (argint(0, &fd) < 0 || argint(2, &n) < 0 || argaddr(1, &uaddr) < 0) return -1;

    return vfsread(fd, 1, (char *)(uaddr), n, 0);
}

uint64 sys_write(void){
    int fd, n;
    uint64 uaddr;
    if (argint(0, &fd) < 0 || argint(2, &n) < 0 || argaddr(1, &uaddr) < 0) return -1;
    return vfswrite(fd, 1, (char *)(uaddr), n, 0);
}

uint64 sys_close(void){
    int fd;
    if (argint(0, &fd) < 0) {
        return -1;
    }
    struct file *fp = getFileByFd(fd);
    struct proc * p = curproc();
    vfsclose(fp);
    acquire(&(p->lock));
    p->ofile[fd] = NULL;
    release(&(p->lock));
    return 0;
}

uint64 sys_fstat(void){
    int fd;
    struct file *fp;
    struct kstat kst;
    uint64 kstAddr;
    if (argfd(0, &fd, &fp) < 0 || argaddr(1, &kstAddr) < 0) {
        return -1;
    }
    memset(&kst, 0, sizeof(struct kstat));
    kst.st_dev = 1;
    kst.st_size = fp->size;
    kst.st_nlink = 1;
    return copyout(curproc()->pagetable, kstAddr, (char *)(&kst), sizeof(struct kstat));
}

uint64 sys_dup(void){
    int fd;
    if (argint(0, &fd) < 0) {
        return -1;
    }
    struct file *fp = getFileByFd(fd);
    if (fp == NULL) {
        return -1;
    }
    vfsdup(fp);
    return registFileHandle(fp);
}

uint64 sys_dup3(void){
    int oldfd, newfd, ansfd;
    if (argint(0, &oldfd) < 0 || argint(1, &newfd) < 0) {
        return -1;
    }
    struct file *fp = getFileByFd(oldfd);
    vfsdup(fp);
    ansfd = registFileHandle(fp, newfd);
    if (ansfd < 0) {
        vfsclose(fp);
    }
    return ansfd;
}

uint64 sys_mount(void) {
    char special[MAXPATH];
    char dir[MAXPATH];
    char fstype[MAXFSTYPE];
    if (argstr(0, special, MAXPATH) < 0 || argstr(1, dir, MAXPATH) < 0 || argstr(2, fstype, MAXFSTYPE) < 0) {
        return -1;
    }

    if (strncmp(fstype, "vfat", 4) == 0) {
        vfsmount(FileSystemType::FAT32, dir, special);
        return 0;
    }
    return -1;
}

uint64 sys_umount2(void){
    char dir[MAXPATH];
    int type;
    if (argstr(0, dir, MAXPATH) < 0 || argint(1, &type) < 0) {
        return -1;
    }
    return vfsumount(dir);
}

uint64 sys_openat(void){
    char filename[MAXPATH];
    int fd, mode, flags;
    int n;
    if (argint(0, &fd) < 0 || (n = argstr(1, filename, MAXPATH)) < 0) {
        return -1;
    }

    if (argint(2, &flags) || argint(3, &mode)) {
        return -1;
    }
    fd = vfsopen(filename, flags);
    return fd;
}

uint64 sys_open(void){
    char path[MAXPATH];
    int fd, mode;
    int n;
    if ((n = argstr(0, path, MAXPATH)) < 0 || argint(1, &mode) < 0) {
        return -1;
    }

    fd = vfsopen(path, mode);
    return fd;
}

uint64 sys_mkdirat(void){
    int fd;
    char path[MAXPATH];
    if (argint(0, &fd) < 0 || argstr(1, path, MAXPATH) < 0) {
        return -1;
    }
    vfsmkdirat(fd, path);
    return 0;
}

uint64 sys_chdir(void){
    char path[MAXPATH];
    memset(path, 0, MAXPATH);
    if (argstr(0, path, MAXPATH) < 0) {
        return -1;
    }
    return vfschdir(path);
}

uint64 sys_pipe2(void){
    uint64 fdarray;
    int fds[2];
    struct proc * p = curproc();
    if (argaddr(0, &fdarray) < 0) {
        return -1;
    }

    vfscreatePipe(fds);
    if (copyout(p->pagetable, fdarray, (char *)&fds[0], sizeof(fds[0])) < 0 ||
        copyout(p->pagetable, fdarray + sizeof(fds[0]), (char *)&fds[1], sizeof(fds[0])) < 0) {
        vfsclose(getFileByFd(fds[0]));
        vfsclose(getFileByFd(fds[1]));
        return -1;
    }
    return 0;
}

uint64 sys_getdents64(void){
    uint64 addr;
    int fd;
    int len;
    if (argint(0, &fd) < 0 || argaddr(1, &addr) || argint(2, &len)) {
        return -1;
    }
    int n = vfsls(fd, (char *)addr, len, 1);
    return n;
}

uint64 sys_unlinkat(){
    int fd;
    char filepath[MAXPATH];
    int flags;
    if (argint(0, &fd) < 0 || argstr(1, filepath, MAXPATH) < 0 || argint(2, &flags) < 0) {
        return -1; 
    }
    return vfsrm(fd, filepath);
}

uint64 sys_getcwd(void){
    uint64 userBuf;
    int n;
    if (argaddr(0, &userBuf) < 0 || argint(1, &n) < 0) {
        return -1;
    }
    char *cwd = curproc()->workDir;
    if (strlen(cwd) > n) {
        return -1;
    }
    if (either_copyout(1, userBuf, (void *)(cwd), strlen(cwd)) < 0) {
        return 0;
    }
    return userBuf;
}

uint64 sys_mmap(void) { 
    uint64 addr;
    int length, prot, flags, fd, offset;
    int vmasz = 0;
    struct vma *a;
    struct file *f;
    struct proc *p = curproc();
    if (argaddr(0, &addr) < 0) return -1;
    if (argint(1, &length) < 0) return -1;
    if (argint(2, &prot) < 0) return -1;
    if (argint(3, &flags) < 0) return -1;
    if (argfd(4, &fd, &f) < 0) return -1;
    if (argint(5, &offset) < 0) return -1; 
    a = allocVma();
    vfsdup(f);
    f->position = 0;
    // filerewind(f);
    a->f = f;
    a->length = length;
    for (int i = 0; i < NOMMAPFILE; i++) {
        if (p->vma[i] != 0) vmasz += p->vma[i]->length;
    }
    a->addr = PGROUNDDOWN(MAXVA - PGSIZE * 5 - vmasz - length);
  // 设置权限
    a->prot = prot;
    a->flag = flags;
    for (int i = 0; i < NOMMAPFILE; i++) {
        if (p->vma[i] == 0) {
            p->vma[i] = a;
        break;
        }
    }
    return a->addr;
}

uint64 sys_munmap(void){
    struct proc * p = curproc();
    uint64 addr;
    struct vma *vma = 0;
    int index = 0;
    int sz;
    if (p->vma == 0) return -1;

    if (argaddr(0, &addr) < 0) return -1;
    if (argint(1, &sz) < 0) return -1;

    for (int i = 0; i < NOMMAPFILE; i++) {
        if (p->vma[i] != 0 && addr >= p->vma[i]->addr && addr < p->vma[i]->addr + p->vma[i]->length) {
            vma = p->vma[i];
            index = i;
            break;
        }
    }
    if (vma == 0) return -1;
    if (vma->flag & MAP_SHARED) {
        vfsrewind(vma->f);
        vfswrite(vma->f, 1, (const char *)(vma->addr), sz, 0);
    }
    vmunmap(p->pagetable, addr, PGROUNDUP(sz) / PGSIZE, 0);
    vma->length -= sz;
    if (vma->length == 0) {
        vfsclose(vma->f);
        freeVma(vma);
        p->vma[index] = 0;
    }
    return 0;
}
