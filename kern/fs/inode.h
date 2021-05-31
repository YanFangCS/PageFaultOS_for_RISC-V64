#include "libs/types.h"
#include "mm/allocop.h"

#define min(a, b) ((a) < (b) ? (a) : (b))

#define ROOTINO 1 // root i-number, 不知道为什么0不行
#define BFSIZE 1024 // 块的大小

#define FSMAGIC 0x10203040

#define NDIRECT 12
#define NINDIRECT (BFSIZE / sizeof(uint_t))
#define MAXFILE (NDIRECT + NINDIRECT)

#define IPB (BFSIZE / sizeof(struct dinode))

#define IBLOCK(i, sb) ((i) / IPB + sb.inodestart)

#define BPB (BFSIZE * 8)

#define BBLOCK(b, sb) ((b) / BPB + sb.bmapstart)

#define DIRSIZ 14

#define NINODE 50

#define T_DIR     1   // Directory
#define T_FILE    2   // File
#define T_DEVICE  3   // Device

// Disk layout:
// [ boot block | super block | log | inode blocks |
//                                          free bit map | data blocks]
//
// mkfs computes the super block and builds an initial file system. The
// super block describes the disk layout:
struct superblock {
    uint magic; // 魔法数字
    uint size; // 文件映像中块的数量
    uint nblocks; // 数据块的数量
    uint ninodes; // inode的数量
    uint inodestart; // 第一个inode块的块号
    uint bmapstart; // 第一个bitmap块的块号
};

struct dinode {
    short type; // 文件类型
    short major; // Major device number (T_DEVICE)
    short minor; // Minor device number (T_DEVICE)
    short nlink; // 文件系统中链接该inode的数量
    uint size; // 文件的大小
    uint addrs[NDIRECT + 1]; // 数据块地址
};

struct direntry {
    ushort inum;    // 目录项对应的inode
    char name[DIRSIZ];  // 名称
};