#include "fs/vfs.h"
#include "libs/printf.h"
#include "libs/string.h"
#include "fs/fcntl.h"
#include "fs/devicefs.h"
#include "fs/fat32.hpp"
#include "sync/spinlock.h"
#include "proc/proc.h"
#include "libs/param.h"
#include "proc/pipe.hpp"
#include "fs/fcntl.h"

struct file fileTable[NFILE];
struct spinlock fileTableLock;
struct spinlock mountedFsLock;
FileSystem *mountedFS[NFILESYSTEM];

void vfsinit(){
    struct file * fp;

    for(int i = 0; i < NFILESYSTEM; ++i){
        mountedFS[i] = NULL;
    }

    memset(fileTable, 0, sizeof(struct file) * NFILE);
    for (fp = fileTable; fp < fileTable + NFILE; fp++) {
        fp->type = fp->FD_NONE;
    }
    initlock(&fileTableLock, (char *)"filetable");

    initlock(&mountedFsLock, (char *)"mount fs");
    vfsmount(FileSystemType::DEVFS, (char *)"/dev", (char *)"");
    vfsmount(FileSystemType::FAT32, (char *)"/", (char *)"/dev/hda1");
}

void getAbPath(const char * path, char * newpath){
    const char *curdir = curproc()->workDir;
    const char *oldpath = path;
    memset(newpath, 0, MAXPATH);

    if (oldpath[0] == '.' && oldpath[1] == '/') {
        oldpath += 2;
    }

    if (oldpath[0] == '/') {
        memcpy(newpath, oldpath, strlen(oldpath));
    } else {
        acquire(&curproc()->lock);
        memcpy(newpath, curdir, strlen(curdir));
        memcpy(newpath + strlen(curdir), oldpath, strlen(oldpath));
        release(&curproc()->lock);
    }
}

FileSystem *vfsgetFs(const char *filepath) {
    int bestMatch = 0;
    int bestLength = 0;
    for (size_t i = 0; i < NFILESYSTEM; ++i) {
        if (mountedFS[i] == NULL) continue;

        auto &mp = mountedFS[i];

        bool match = true;
        for (size_t j = 0; mp->mountPos[j] != 0 && filepath[i] != 0; ++j) {
            if (mp->mountPos[j] != filepath[j]) {
                match = false;
                break;
            }
        }

        if (match && strlen(mp->mountPos) > bestLength) {
            bestLength = strlen(mp->mountPos);
            bestMatch = i;
        }
    }
    return mountedFS[bestMatch];
}

FileSystem *createFileSystem(FileSystemType type, const char *mountPoint, const char *specialDev, int dev) {
    switch (type) {
        case FileSystemType::DEVFS:
            return new DeviceFS(mountPoint, specialDev);
        case FileSystemType::FAT32:
            return new DeviceFS(mountPoint, specialDev);
        default:
            panic("create file system");
            break;
    }
    return nullptr;
}

struct file * allocFileHandle(){
    acquire(&fileTableLock);
    struct file *fp;
    for (fp = fileTable; fp < fileTable + NFILE; fp++) {
        if (fp->ref == 0 && fp->type == fp->FD_NONE) {
            release(&fileTableLock);
            fp->ref = 1;
            return fp;
        }
    }
    panic("alloc file");
    return NULL;
}

void freeFileHandle(struct file * fp){
    memset(fp, 0, sizeof(struct file));
    fp->type = fp->FD_NONE;
}

int vfsopen(const char *filename, size_t flags) {
    char path[MAXPATH];
    memset(path, 0, MAXPATH);
    getAbPath((char *)filename, path);

    auto fs = vfsgetFs(path);
    struct file *fp = allocFileHandle();
    if (fs->open(path, flags, fp) == -1) {
        freeFileHandle(fp);
        return -1;
    }

    safestrcpy(fp->filepath, path, strlen(path) + 1);
    fp->readable = !(flags & O_WRITEONLY);
    fp->writable = (flags & O_WRITEONLY) || (flags & O_RDWR);
    int fd = registFileHandle(fp);
    return fd;
}

size_t vfsread(int fd, bool user, char *dst, size_t n, size_t offset) {
    struct file *f = getFileByFd(fd);
    if (f == NULL || !f->readable) {
        return -1;
    }
    return vfsread(f, user, dst, n, offset);
}

size_t vfsread(struct file *fp, bool user, char *dst, size_t n, size_t offset) {
    int r = 0;
    if (fp == NULL || !fp->readable) {
        return -1;
    }
    auto fs = vfsgetFs(fp->filepath);

    // if () strncpy(path, dir, strlen(dir));
    // strncpy(path + strlen(dir), f->file_name, strlen(f->file_name));

    switch (fp->type) {
        case fp->FD_PIPE:
            r = fp->pip->piperead((uint64)(dst), n);
            break;
        case fp->FD_DEVICE:
            r = fs->read(fp->filepath, user, dst, 0, n);
            break;
        case fp->FD_ENTRY:
            r = fs->read(fp->filepath, user, dst, fp->position, n);
            fp->position += r;
            break;
        default:
            panic("vfsread");
    }
    return r;
}

size_t vfswrite(int fd, bool user, const char *src, size_t n, size_t offset) {
    struct file *f = getFileByFd(fd);

    if (f == NULL || !f->writable) {
        return -1;
    }
    return vfswrite(f, user, src, n, offset);
}

size_t vfswrite(struct file *fp, bool user, const char *src, size_t n, size_t offset) {
    int r = 0;

    if (fp == NULL || !fp->writable) {
        return -1;
    }

    auto fs = vfsgetFs(fp->filepath);


    switch (fp->type) {
        case fp->FD_PIPE:
            fp->pip->pipewrite((uint64)(src), n);
            break;
        case fp->FD_DEVICE:
            r = fs->write(fp->filepath, user, src, 0, n);
            break;
        case fp->FD_ENTRY:
      // fat32->read(fp->filepath, user, dst, fp->position, 0);
            r = fs->write(fp->filepath, user, src, 0, n);
            fp->size = fp->position + r < fp->size ? fp->size : fp->position + r;
            fp->position += r;
            break;
        default:
            panic("vfswrite");
            return 0;
    }
    return r;
}

void vfsclose(struct file *fp) {

};

int vfsmkdirat(int dirfd, const char *filepath) {
    char path[MAXPATH];
    memset(path, 0, MAXPATH);
    struct file *fp;
    if (dirfd == AT_FDCWD) {
        getAbPath((char *)filepath, path);
    } else {
        if (filepath[0] == '/') {
            memcpy(path, filepath, strlen(filepath));
        } else {
            fp = getFileByFd(dirfd);
            memcpy(path, fp->filepath, strlen(fp->filepath));
            memcpy(path + strlen(fp->filepath), filepath, strlen(filepath));
        }
    }
    auto fs = vfsgetFs(path);
    return fs->mkdir(path);
};

int vfsopenat(int dirfd, const char *filepath, int flags) {
    char path[MAXPATH];
    memset(path, 0, MAXPATH);
    struct file *fp = NULL;
    if (dirfd == AT_FDCWD) {
        getAbPath((char *)filepath, path);
    } else {
        if (filepath[0] == '/') {
            memcpy(path, filepath, strlen(filepath));
        } else {
            fp = getFileByFd(dirfd);
            memcpy(path, fp->filepath, strlen(fp->filepath));
            memcpy(path + strlen(fp->filepath), filepath, strlen(filepath));
        }
    }
    return vfsopen(filepath, flags);
}

int vfsrm(int dirfd, char *filepath) {
    char path[MAXPATH];
    memset(path, 0, MAXPATH);
    struct file *fp = NULL;
    if (dirfd == AT_FDCWD) {
        getAbPath((char *)filepath, path);
    } else {
        if (filepath[0] == '/') {
            memcpy(path, filepath, strlen(filepath));
        } else {
            fp = getFileByFd(dirfd);
            memcpy(path, fp->filepath, strlen(fp->filepath));
        memcpy(path + strlen(fp->filepath), filepath, strlen(filepath));
        }
    }
    auto fs = vfsgetFs(path);
    return fs->rm(path);
};

size_t vfsls(int fd, char *buffer, int len, bool user = false) {
    struct file *fp = getFileByFd(fd);
    if (fp == NULL || !fp->directory) {
        return -1;
    }
    auto fs = vfsgetFs(fp->filepath);
    return fs->ls(fp->filepath, buffer, len, user);
}

size_t vfsmounts(char *buffer, size_t size) { return 0; }

int vfsmount(FileSystemType type, const char *mountPoint, const char *specialDev) {
    char absolutMp[MAXPATH];
    getAbPath(mountPoint, absolutMp);
    auto fs = createFileSystem(type, absolutMp, specialDev, 0);
    fs->init();
    acquire(&mountedFsLock);
    for (int i = 0; i < NFILESYSTEM; i++) {
        if (mountedFS[i] == NULL) {
            release(&mountedFsLock);
            mountedFS[i] = fs;
            return 0;
        }
    }
    release(&mountedFsLock);
    panic("mount file system");
    return -1;
}

int vfsumount(const char *mpdir) {
    char absolutMp[MAXPATH];
    getAbPath(mpdir, absolutMp);
    FileSystem *fs;
    int absolutMpLen = strlen(absolutMp);
    int n = 0;
    acquire(&mountedFsLock);
    for (int i = 0; i < NFILESYSTEM; i++) {
        fs = mountedFS[i];
        if (fs == NULL) {
            continue;
        }
        n = strlen(fs->mountPos);
        if (strncmp(fs->mountPos, absolutMp, max(n, absolutMpLen)) == 0) {
            mountedFS[i] = NULL;
            release(&mountedFsLock);
            delete fs;
            return 0;
        }
    }
    release(&mountedFsLock);
    return -1;
}

size_t vfsdread(const char *file, char *buffer, size_t count, size_t offset) {
    auto fs = vfsgetFs(file);
    int r = fs->read(file, false, buffer, offset, count);
    return r;
}

size_t vfsdwrite(const char *file, const char *buffer, size_t count, size_t offset) {
    auto fs = vfsgetFs(file);
    int r = fs->write(file, false, buffer, offset, count);
    return r;
}

struct file *vfsdup(struct file *fp) {
    acquire(&fileTableLock);
    if (fp->ref < 1) {
        panic("vfsdup");
    }
    fp->ref++;
    release(&fileTableLock);
    return fp;
}

int vfschdir(char *filepath) {
    char path[MAXPATH];
    struct proc *p = curproc();

    memset(path, 0, MAXPATH);
    struct file *fp = allocFileHandle();
    getAbPath((char *)filepath, path);
    auto fs = vfsgetFs(path);

    if (fs->open(path, O_READONLY, fp) < 0) {
        return -1;
    }

    if (!fp->directory) {
        freeFileHandle(fp);
        return -1;
    }

    int n = strlen(path);
    if (path[n - 1] != '/') {
        path[n] = '/';
        path[n + 1] = 0;
    }
    acquire(&p->lock);
    safestrcpy(p->workDir, path, strlen(path) + 1);
    release(&p->lock);
    freeFileHandle(fp);
    return 0;
}

int vfscreatePipe(int fds[]) {
    struct file *f0 = allocFileHandle();
    struct file *f1 = allocFileHandle();
    pipe *pi = new pipe(f0, f1);
    if (pi == NULL) {
        freeFileHandle(f0);
        freeFileHandle(f1);
        return -1;
    }
    int fd0 = registFileHandle(f0);
    int fd1 = registFileHandle(f1);
    fds[0] = fd0;
    fds[1] = fd1;
    return 0;
}

struct file *vfsrewind(struct file *fp) {
    acquire(&fileTableLock);
    if (fp->ref < 1) panic("filerewind");
    fp->position = 0;
    release(&fileTableLock);
    return fp;
}