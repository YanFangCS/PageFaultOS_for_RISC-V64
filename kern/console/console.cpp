#include "console/console.h"
#include "libs/sbi.h"
#include "proc/proc.h"
#include "sync/spinlock.h"

#define ENTER (12)
#define BACKSPACE (0x100)
#define CTRL(x) (x - '@')


extern struct console console;
 
void consoleinit(){
    initlock(&console.lock, (char *)"console");
    console.read_index = 0;
    console.write_index = 0;
}

void consputc(int c){
    if(c == BACKSPACE){
        sbi_console_putchar('\b');
        sbi_console_putchar(' ');
        sbi_console_putchar('\b');
    } else {
        sbi_console_putchar(c);
    }
}


void consoleintr(int c){
    switch (c){
        case '\x08':  // backspace
        case '\x7f':  // del
            if (console.read_index != console.write_index) {
                console.write_index--;
                consputc(BACKSPACE);
                consputc('\a');
            }
            break;
        case CTRL('P'):
            // print_proc();
            break;
        default:
#ifdef K210
            if (c == '\r') break;
#else
            c = (c == '\r') ? '\n' : c;
#endif
            consputc(c);
        // 保存输入字符
            console.buf[console.write_index++ % INPUT_BUF] = c;
            if (c == '\n') {
                wakeup(&console.read_index);
            }
        break;
    }
}


int consoleread(uint64 dst, int user, int n){
    char c;
    int expect = n;
    acquire(&console.lock);
    while (n > 0) {
        while (console.read_index == console.write_index) {
            sleep(&console.read_index, &console.lock);
        }
        c = console.buf[console.read_index++ % INPUT_BUF];
        if (either_copyout(user, dst, &c, 1) < 0) break;
        dst++;
        n--;
        // 当输入一整行时，需要返回
        if (c == '\n') {
            break;
        }
    }
    release(&console.lock);
    return expect - n;
}

int consolewrite(uint64 src, int user, int n){
    int i;

    acquire(&console.lock);
    for (i = 0; i < n; i++) {
        char c;
        if (either_copyin(&c, user, src + i, 1) == -1) break;
        sbi_console_putchar(c);
    }
    release(&console.lock);

    return i;
}