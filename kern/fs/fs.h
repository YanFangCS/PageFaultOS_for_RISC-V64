#ifndef __KERN_FS_FS_H__
#define __KERN_FS_FS_H__

#include "mm/allocop.h"
#include "libs/printf.h"
#include "fs/file.h"
#include "libs/param.h"
#include "libs/types.h"

class FileSystem {
public:
    char mountPos[MAXPATH];
    char specialDev[MAXPATH];
    int dev;

public:
    virtual void init(){};
    virtual ~FileSystem(){};
    virtual int open(const char *filePath, uint64 flags, struct file *fp) = 0;
    virtual size_t read(const char *path, bool user, char *buf, int offset, int n) = 0;
    virtual size_t write(const char *path, bool user, const char *buf, int offset, int n) = 0;
    virtual size_t clear(const char *filepath, size_t count, size_t offset, size_t &written) = 0;
    virtual int get_file(const char *filepath, struct file *fp) = 0;
    virtual size_t truncate(const char *filepath, size_t size) = 0;
    virtual int ls(const char *filepath, char *contents, int len, bool user = false) = 0;
    virtual size_t touch(const char *filepath) = 0;
    virtual int mkdir(const char *filepath) = 0;
    virtual size_t rm(const char *filepath) = 0;
} ;

#endif