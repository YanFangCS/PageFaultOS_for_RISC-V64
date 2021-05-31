#ifndef FILE_SYSTEM_HPP
#define FILE_SYSTEM_HPP

#include "mm/allocop.h"
#include "libs/printf.h"
#include "fs/fs.h"
#include "fs/file.h"
#include "libs/types.h"
#include "libs/param.h"
#include "fat32bpb.h"
#include "libs/string.h"

#define TF_MAX_PATH 256
#define TF_FILE_HANDLES 5

#define TF_FLAG_DIRTY 0x01
#define TF_FLAG_OPEN 0x02
#define TF_FLAG_SIZECHANGED 0x04
#define TF_FLAG_ROOT 0x08

#define TYPE_FAT12 0
#define TYPE_FAT16 1
#define TYPE_FAT32 2

#define TF_MARK_BAD_CLUSTER32 0x0ffffff7
#define TF_MARK_BAD_CLUSTER16 0xfff7
#define TF_MARK_EOC32 0x0fffffff
#define TF_MARK_EOC16 0xfff8

#define LFN_ENTRY_CAPACITY 13  // uint16 per LFN entry
#define LFN_NAME_CAPACITY 26  // byte per LFN entry

#define TF_ATTR_DIRECTORY 0x10

#ifdef DEBUG

#define dbg_printf(...) printf(__VA_ARGS__)
#define dbg_printHex(x, y) printHex(x, y)

#ifdef TF_DEBUG
typedef struct struct_TFStats {
  unsigned long sector_reads;
  unsigned long sector_writes;
} TFStats;

#define tf_printf(...) printf(__VA_ARGS__)
#define tf_printHex(x, y) printHex(x, y)
#else
#define tf_printf(...)
#define tf_printHex(x, y)
#endif  // TF_DEBUG

#else  // DEBUG
#define dbg_printf(...)
#define dbg_printHex(x, y)
#define tf_printf(...)
#define tf_printHex(x, y)
#endif  // DEBUG



#define LSN(CN, bpb) SSA + ((CN - 2) * bpb->SectorsPerCluster)

#ifndef min
#define min(x, y) (x < y) ? x : y
#define max(x, y) (x > y) ? x : y
#endif

// Ultimately, once the filesystem is checked for consistency, you only need a few
// things to keep it up and running.  These are:
// 1) The type (fat16 or fat32, no fat12 support)
// 2) The number of sectors per cluster
// 3) Everything needed to compute indices into the FATs, which includes:
//    * Bytes per sector, which is fixed at 512
//    * The number of reserved sectors (pulled directly from the BPB)
// 4) The current sector in memory.  No sense reading it if it's already in memory!

typedef struct struct_tfinfo {
  // FILESYSTEM INFO PROPER
  char type;  // 0 for FAT16, 1 for FAT32.  FAT12 NOT SUPPORTED
  char sectorsPerCluster;
  uint32 firstDataSector;
  uint32 totalSectors;
  uint16 reservedSectors;
  // "LIVE" DATA
  uint32 currentSector;
  char sectorFlags;
  uint32 rootDirectorySize;
  char buffer[512];
} TFInfo;

/////////////////////////////////////////////////////////////////////////////////

typedef struct struct_TFFILE {
  uint32 parentStartCluster;
  uint32 startCluster;
  uint32 currentClusterIdx;
  uint32 currentCluster;
  short currentSector;
  short currentByte;
  uint32 pos;
  char flags;
  char attributes;
  char mode;
  uint32 size;
  char filename[TF_MAX_PATH];
} TFFile;

#define TF_MODE_READ 0x01
#define TF_MODE_WRITE 0x02
#define TF_MODE_APPEND 0x04
#define TF_MODE_OVERWRITE 0x08

#define TF_ATTR_READ_ONLY 0x01
#define TF_ATTR_HIDDEN 0x02
#define TF_ATTR_SYSTEM 0x04
#define TF_ATTR_VOLUME_LABEL 0x08
#define TF_ATTR_LONG_NAME 0x0F
#define TF_ATTR_DIRECTORY 0x10
#define TF_ATTR_ARCHIVE 0x20
#define TF_ATTR_DEVICE 0x40  // Should never happen!
#define TF_ATTR_UNUSED 0x80

// New error codes
#define TF_ERR_NO_ERROR 0
#define TF_ERR_BAD_BOOT_SIGNATURE 1
#define TF_ERR_BAD_FS_TYPE 2

#define TF_ERR_INVALID_SEEK 1

// FS Types
#define TF_TYPE_FAT16 0
#define TF_TYPE_FAT32 1

// New Datas
extern TFInfo tf_info;
extern TFFile tf_file;

class Fat32FileSystem final : public FileSystem {
public:
    ~Fat32FileSystem() override{};
  /**
   * @brief 默认构造函数
   *
   */
    Fat32FileSystem(){};

  /**
   * @brief 带参构造函数
   *
   */
    Fat32FileSystem(const char *mountPoint, const char *specialDev) {
      safestrcpy(this->mountPos, mountPoint, strlen(mountPoint) + 1);
      safestrcpy(this->specialDev, specialDev, strlen(specialDev) + 1);
    };

  /**
   * @brief 初始化文件系统
   *
   */
    void init();

  /**
   * @brief 打开一个文件
   *
   * @param filePath
   * @param flags
   * @param fp
   * @return int
   */
    int open(const char *filePath, uint64 flags, struct file *fp) override;

  /**
   * @brief 从指定文件的指定位置读取n个字节到buf中
   *
   * @param path 文件路径
   * @param buf  保存文件数据的缓冲区
   * @param offset 读取偏移量
   * @param n 期望读取的字节数
   * @return size_t 读取字节数，失败返回-1
   */
    size_t read(const char *path, bool user, char *buf, int offset, int n) override;

  /**
   * @brief 将buf写入到指定文件的指定位置
   *
   * @param path 文件路径
   * @param buf 需要写入的数据
   * @param offset 写入偏移量
   * @param n 写入字节数
   * @return size_t 写入字节数，失败返回-1
   */
  size_t write(const char *path, bool user, const char *buf, int offset, int n) override;

  /**
   * @brief Clear a portion of a file (write zeroes)
   * @param filepath The path to the file to write
   * @param count The amount of bytes to write
   * @param offset The offset at which to start writing
   * @param written output reference to indicate the number of bytes written
   * @return 0 on success, an error code otherwise
   */
    size_t clear(const char *filepath, size_t count, size_t offset, size_t &written) override;

  /**
   * @brief Change the size of a file
   * @param filepath The path to the file to modify
   * @param size The new size of the file
   * @return 0 on success, an error code otherwise
   */
    size_t truncate(const char *filepath, size_t size) override;

  /**
   * @brief 通过文件路径获取文件信息
   * @param filepath 文件的绝对路径
   * @param fp 指向文件信息指针
   * @return 0成功，-1，失败
   */
    int get_file(const char *filepath, struct file *fp) override;

  /**
   * @brief 获取给定目录下的目录项
   * @param filepath 目录的绝对路径
   * @param contents
   * @param len contens的长度
   * @return 返回该目录下的目录项的数量
   */
    int ls(const char *filepath, char *contents, int len, bool user = false) override;

  /**
   * @brief Create the given file on the file system
   * @param filepath The path to the file to create
   * @return 0 on success, an error code otherwise
   */
    size_t touch(const char *filepath) override;

  /**
   * @brief Create the given directory on the file system
   * @param filepath The path to the directory to create
   * @return 0 on success, an error code otherwise
   */
    int mkdir(const char *filepath) override;

  /**
   * @brief Remove the given file from the file system
   * @param filepath The path to the file to remove
   * @return 0 on success, an error code otherwise
   */
    size_t rm(const char *filepath) override;

  // New backend functions
    int tf_fetch(uint32 sector);
    int tf_store(void);
    uint32 tf_get_fat_entry(uint32 cluster);
    int tf_set_fat_entry(uint32 cluster, uint32 value);
    int tf_unsafe_fseek(TFFile *fp, int base, long offset);
    TFFile *tf_fnopen(const char *filename, const char *mode, int n);
    int tf_free_clusterchain(uint32 cluster);
    int tf_create(const char *filename);
    void tf_release_handle(TFFile *fp);
    TFFile *tf_parent(const char *filename, const char *mode, int mkParents);
    int tf_shorten_filename(char *dest, char *src, char num);

  // New frontend functions
    int tf_init();
    int tf_fflush(TFFile *fp);
    int tf_fseek(TFFile *fp, size_t base, long offset);
    int tf_fclose(TFFile *fp);
    int tf_fread(char *dest, int size, TFFile *fp, bool user = false);
    int tf_find_file(TFFile *current_directory, char *name);
    int tf_compare_filename(TFFile *fp, char *name);
    uint32 tf_first_sector(uint32 cluster);
    char *tf_walk(char *filename, TFFile *fp);
    TFFile *tf_fopen(const char *filename, const char *mode);
    int tf_fwrite(const char *src, int sz, TFFile *fp, bool user = false);
    int tf_fputs(char *src, TFFile *fp);
    int tf_mkdir(const char *filename, int mkParents);
    int tf_remove(char *filename);
    void tf_print_open_handles(void);

    uint32 tf_find_free_cluster();
    uint32 tf_find_free_cluster_from(uint32 c);

    uint32 tf_initializeMedia(uint32 totalSectors);
    uint32 tf_initializeMediaNoBlock(uint32 totalSectors, int start);

  // hidden functions... IAR requires that all functions be declared
    TFFile *tf_get_free_handle();
    uint64 tf_get_open_handles();

    int tf_place_lfn_chain(TFFile *fp, char *filename, char *sfn);
    void tf_choose_sfn(char *dest, char *src, TFFile *fp);
    char *tf_create_lfn_entry(char *filename, FatFileEntry *entry);
  // New Datas
    TFInfo tf_info;
    TFFile tf_file_handles[TF_FILE_HANDLES];
};

char upper(char c);
#endif