#include <types.h>
#include <riscv.h>
#include <param.h>
#include <stat.h>
#include <spinlock.h>
#include <proc.h>
#include <pipe.h>
#include <file.h>
#include <fat32.h>
#include <fs/fcntl.h>
#include <syscall.h>
#include <vm.h>

static int argfd(int n, int *pfd, struct file **pf){
    int fd;
    struct file *f;

    if(argint(n, &fd) < 0)  return -1;
    if(fd < 0 || fd >= NOFILE || (f = curproc()->ofile[fd]) == NULL) return -1;
    if(pfd) *pfd = fd;
    if(pf) *pf = f;
    return 0;
}

static int fdalloc(struct file *f){
    int fd;
    struct proc * p = curcpu();

    for(fd = 0; fd < NOFILE; ++fd){
        if(p->ofile[fd] == 0){
            p->ofile[fd] = f;
            return fd;
        }
    }
    return -1;
}


uint64 sys_read(void){
    struct file * f;
    int n;
    uint64 p;

    if(argfd(0, 0, &f) < 0 || argint(2, &n) < 0 || argaddr(1, &p) < 0)
        return -1;
    return fileread(f, p, n);
}

uint64 sys_write(void){
    struct file * f;
    int n;
    uint64 p;

    if(argfd(0, 0, &f) < 0 || argint(2, &n) < 0 || argaddr(1, &p) < 0)
        return -1;
    return filewrite(f, p, n);
}

uint64 sys_close(void){
    int fd;
    struct file *f;

    if(argfd(0, &fd, &f) < 0)   return -1;
    curproc()->ofile[fd] = 0;
    fileclose(f);
    return 0;
}

uint64 sys_fstat(void){
    struct file *f;
    uint64 st; // user pointer to struct stat

    if(argfd(0, 0, &f) < 0 || argaddr(1, &st) < 0)
        return -1;
    return filestat(f, st);
}

static struct dirent * create(char *path, short type, int mode){
    struct dirent *ep, *dp;
    char name[FAT32_MAX_FILENAME + 1];

    if((dp = enameparent(path, name)) == NULL)
        return NULL;

    if (type == T_DIR) {
        mode = ATTR_DIRECTORY;
    } else if (mode & O_READONLY) {
        mode = ATTR_READ_ONLY;
    } else {
        mode = 0;  
    }

    elock(dp);
    if ((ep = ealloc(dp, name, mode)) == NULL) {
        eunlock(dp);
        eput(dp);
        return NULL;
    }
  
    if ((type == T_DIR && !(ep->attribute & ATTR_DIRECTORY)) ||
        (type == T_FILE && (ep->attribute & ATTR_DIRECTORY))) {
        eunlock(dp);
        eput(ep);
        eput(dp);
        return NULL;
    }

    eunlock(dp);
    eput(dp);

    elock(ep);
    return ep;
}

uint64 sys_open(void){
    char path[FAT32_MAX_PATH];
    int fd, omode;
    struct file *f;
    struct dirent *ep;

    if(argstr(0, path, FAT32_MAX_PATH) < 0 || argint(1, &omode) < 0)
        return -1;

    if(omode & O_CREATE){
        ep = create(path, T_FILE, omode);
        if(ep == NULL){
            return -1;
        }
    } else {
        if((ep = ename(path)) == NULL){
            return -1;
        }
        elock(ep);
        if((ep->attribute & ATTR_DIRECTORY) && omode != O_READONLY){
            eunlock(ep);
            eput(ep);
            return -1;
        }
    }

    if((f = filealloc()) == NULL || (fd = fdalloc(f)) < 0){
        if (f) {
            fileclose(f);
        }
        eunlock(ep);
        eput(ep);
        return -1;
    }

    if(!(ep->attribute & ATTR_DIRECTORY) && (omode & O_TRUNC)){
        etrunc(ep);
    }

    f->type = FD_ENTRY;
    f->off = (omode & O_APPEND) ? ep->file_size : 0;
    f->ep = ep;
    f->readable = !(omode & O_WRITEONLY);
    f->writable = (omode & O_WRITEONLY) || (omode & O_RDWR);

    eunlock(ep);

    return fd;
}

uint64 sys_mkdir(void){
    char path[FAT32_MAX_PATH];
    struct dirent *ep;

    if(argstr(0, path, FAT32_MAX_PATH) < 0 || (ep = create(path, T_DIR, 0)) == 0){
        return -1;
    }
    eunlock(ep);
    eput(ep);
    return 0;
}

uint64 sys_chdir(void){
    char path[FAT32_MAX_PATH];
    struct dirent *ep;
    struct proc *p = myproc();
  
    if(argstr(0, path, FAT32_MAX_PATH) < 0 || (ep = ename(path)) == NULL){
        return -1;
    }
    elock(ep);
    if(!(ep->attribute & ATTR_DIRECTORY)){
        eunlock(ep);
        eput(ep);
        return -1;
    }
    eunlock(ep);
    eput(p->cwd);
    p->cwd = ep;
    return 0;
}


uint64 sys_pipe2(void){
    uint64 fdarray; // user pointer to array of two integers
    struct file *rf, *wf;
    int fd0, fd1;
    struct proc *p = myproc();

    if(argaddr(0, &fdarray) < 0)
        return -1;
    if(pipealloc(&rf, &wf) < 0)
        return -1;
    fd0 = -1;
    if((fd0 = fdalloc(rf)) < 0 || (fd1 = fdalloc(wf)) < 0){
        if(fd0 >= 0)
            p->ofile[fd0] = 0;
        fileclose(rf);
        fileclose(wf);
        return -1;
    }
    // if(copyout(p->pagetable, fdarray, (char*)&fd0, sizeof(fd0)) < 0 ||
    //    copyout(p->pagetable, fdarray+sizeof(fd0), (char *)&fd1, sizeof(fd1)) < 0){
    if(copyout2(fdarray, (char*)&fd0, sizeof(fd0)) < 0 ||
       copyout2(fdarray+sizeof(fd0), (char *)&fd1, sizeof(fd1)) < 0){
        p->ofile[fd0] = 0;
        p->ofile[fd1] = 0;
        fileclose(rf);
        fileclose(wf);
        return -1;
    }
    return 0;
}

uint64 sys_getcwd(void){
    uint64 addr;
    if (argaddr(0, &addr) < 0)
        return -1;

    struct dirent *de = curproc()->cwd;
    char path[FAT32_MAX_PATH];
    char *s;
    int len;

    if (de->parent == NULL) {
        s = "/";
    } else {
        s = path + FAT32_MAX_PATH - 1;
        *s = '\0';
        while (de->parent) {
            len = strlen(de->filename);
            s -= len;
            if (s <= path)          // can't reach root "/"
                return -1;
            strncpy(s, de->filename, len);
            *--s = '/';
            de = de->parent;
        }
    }

    // if (copyout(myproc()->pagetable, addr, s, strlen(s) + 1) < 0)
    if (copyout2(addr, s, strlen(s) + 1) < 0)
        return -1;
  
    return 0;
}

int sys_getpid(void){
    return curproc()->pid;
}


uint64 sys_exec(void){
    
}

uint64 sys_exit(void){
    
}