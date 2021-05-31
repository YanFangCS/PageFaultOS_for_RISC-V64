#ifndef __KERN_FS_DEVICEFS_H__
#define __KERN_FS_DEVICEFS_H__

#include "libs/types.h"
#include "libs/string.h"
#include "fs/fs.h"
#include "mm/allocop.h"

class DeviceFS : public FileSystem {
public:
    DeviceFS() {};
    ~DeviceFS() {};

    DeviceFS(const char *mountPos, const char *specialDev) {
        safestrcpy(this->mountPos, mountPos, strlen(mountPos) + 1);
        safestrcpy(this->specialDev, specialDev, strlen(specialDev) + 1);
    };

    void init();
    int open(const char *filePath, uint64 flags, struct file *fp) override;
    size_t read(const char *path, bool user, char *buf, int offset, int n) override;
    size_t write(const char *path, bool user, const char *buf, int offset, int n) override;
    size_t clear(const char *filepath, size_t count, size_t offset, size_t &written) override;
    int get_file(const char *filepath, struct file *fp) override;
    int ls(const char *filepath, char *contents, int len, bool user = false) override;
    size_t touch(const char *filepath) override;
    int mkdir(const char *filepath) override;
    size_t rm(const char *filepath) override;
    size_t truncate(const char *filepath, size_t size) override;
} ;

#endif