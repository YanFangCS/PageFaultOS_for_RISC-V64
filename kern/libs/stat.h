#ifndef __KERN_LIBS_STAT_H__
#define __KERN_LIBS_STAT_H__

#include "libs/types.h"
#include "fs/fcntl.h"

// #define T_DIR       1
// #define T_FILE      2
// #define T_DEVICE    3

#define STAT_MAX_NAME 32

struct stat {
    char name[STAT_MAX_NAME];
    int dev;
    short type;
    uint64 size;
};

#endif