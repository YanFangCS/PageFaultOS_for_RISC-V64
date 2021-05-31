#ifndef __KERN_FS_FCNTL_H__
#define __KERN_FS_FCNTL_H__

#define AT_FDCWD    -100

#define PROT_NONE       0x0
#define PROT_READ       0x1
#define PROT_WRITE      0x2
#define PROT_EXEC       0x4

#define O_READONLY  0x000
#define O_WRITEONLY 0x001
#define O_RDWR      0x002
#define O_APPEND    0x004
#define O_CREATE    0x200
#define O_TRUNC     0x400

#define MAP_SHARED  0x001
#define MAP_PRIVATE 0x002
#define T_FILE    1
#define T_DIR     2
#define T_DEVICE  3

#define O_DIRECTORY 0x0200000

#endif