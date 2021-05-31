#ifndef __KERN_FS_FILE_H__
#define __KERN_FS_FILE_H__

#define DT_FIFO 1
#define DT_DIR 4

#define DIENT_BASE_LEN sizeof(long) * 2 + sizeof(short) + sizeof(char)

#include "libs/types.h"
#include "libs/param.h"
#include "proc/pipe.hpp"
#include "fs/fcntl.h"

struct file {
    char filepath[MAXPATH];
    int directory;
    uint64 size;
    int ref;
    int readable;
    int writable;
    enum {
        FD_NONE, FD_PIPE, FD_ENTRY, FD_DEVICE
    } type;
    pipe * pip;
    size_t location;
    size_t position;
} ;

struct dirent {
    unsigned long d_ino;	// 索引结点号
    long d_off;	// 到下一个dirent的偏移
    unsigned short d_reclen;	// 当前dirent的长度
    unsigned char d_type;	// 文件类型
    char d_name[];	//文件名
};

typedef unsigned int mode_t;
typedef long int off_t;

struct kstat {
        uint64 st_dev;
        uint64 st_ino;
        mode_t st_mode;
        uint32 st_nlink;
        uint32 st_uid;
        uint32 st_gid;
        uint64 st_rdev;
        unsigned long __pad;
        off_t st_size;
        uint32 st_blksize;
        int __pad2;
        uint64 st_blocks;
        long st_atime_sec;
        long st_atime_nsec;
        long st_mtime_sec;
        long st_mtime_nsec;
        long st_ctime_sec;
        long st_ctime_nsec;
        unsigned __unused[2];
};

#endif