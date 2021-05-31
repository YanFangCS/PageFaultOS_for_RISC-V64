#ifndef __KERN_CONSOLE_CONSOLE_H__
#define __KERN_CONSOLE_CONSOLE_H__

#define INPUT_BUF 128

#include "libs/types.h"
#include "sync/spinlock.h"

struct console {
    char buf[INPUT_BUF];
    int read_index;
    int write_index;
    struct spinlock lock;
};

void consoleinit(void);
void consputc(int c);
void consoleintr(int c);
int consoleread(uint64 dst, int user, int n);
int consolewrite(uint64 src, int user, int n);

#endif
