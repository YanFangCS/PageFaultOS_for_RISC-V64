#ifndef __KERN_FS_BUFFER_LAYER_H__
#define __KERN_FS_BUFFER_LAYER_H__

#include "sync/spinlock.h"
#include "sync/sleeplock.h"
#include "libs/types.h"

#define BUFFER_NUM 100
#define BFSIZE 512

struct buf {
  int valid;  // has data been read from disk?
  int disk;   // does disk "own" buf?
  int dev;
  int blockno;
  uint64 last_use_tick;
  struct sleeplock sleeplock;
  uint refcnt;
  uchar data[BFSIZE];
};

class BufferLayer {
public:
    void init();

  /**
   * @brief 申请一个buffer，使用优先选择
   *        未cache的buffer，若全部buffer都包含cache，
   *        则使用LRU算法进行淘汰
   * @note 一个buffer只能同时被一个线程所持有
   *
   */
    struct buf* allocBuffer(int dev, int blockno);

    void freeBuffer(struct buf *b);

  /**
   * @brief 读取给定块的的内容，返回一个包含
   *        该内容的buffer。
   */
    struct buf *read(int dev, int blockno);

  /**
   * @brief 将缓冲区写入磁盘
   * 
   */
    void write(struct buf *b);

public:
  struct buf bufCache[BUFFER_NUM];
  struct spinlock spinlock;
};
#endif