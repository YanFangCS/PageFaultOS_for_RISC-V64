#include <printf.h>
#include <stdarg.h>
#include <types.h>
#include <riscv.h>
#include <spinlock.h>
#include <console.h>
#include <param.h>


volatile int panicked = 0;

static char digits[] = "0123456789abcdef";

static struct {
    struct spinlock lock;
    int locking;
} pr;


void printstring(const char * s){
    while (*s) {
        consputc(*s++);
    }
}

static void printint(int num, int base, int sign){
    char buf[16];
    int i;
    uint x;
    if(sign && (sign = num < 0)){
        x = -num;
    } else {
        x = num;
    }

    i = 0;
    do{
        buf[i++] = digits[x % base];
    } while((x /= base) != 0);

    if(sign) buf[i++] = '-';

    while(--i >= 0) consputc(buf[i]);
}

static void printptr(uint64 x){
    int i;
    consputc('0');
    consputc('x');
    for (i = 0; i < (sizeof(uint64) * 2); i++, x <<= 4)
        consputc(digits[x >> (sizeof(uint64) * 8 - 4)]);
}

void printf(char *fmt, ...) {
    va_list ap;
    int i, c;
    int locking;
    char *s;

    locking = pr.locking;
    if(locking)
        acquire(&pr.lock);
  
    if (fmt == 0)
        panic("null fmt");

    va_start(ap, fmt);
    for(i = 0; (c = fmt[i] & 0xff) != 0; i++){
        if(c != '%'){
        consputc(c);
        continue;
    }
    c = fmt[++i] & 0xff;
    if(c == 0)
        break;
    switch(c){
        case 'd':
            printint(va_arg(ap, int), 10, 1);
            break;
        case 'x':
            printint(va_arg(ap, int), 16, 1);
            break;
        case 'p':
            printptr(va_arg(ap, uint64));
            break;
        case 's':
            if((s = va_arg(ap, char*)) == 0)
                s = "(null)";
            for(; *s; s++)
                consputc(*s);
            break;
        case '%':
            consputc('%');
            break;
        default:
            // Print unknown % sequence to draw attention.
            consputc('%');
            consputc(c);
            break;
        }
    }
    if(locking)
        release(&pr.lock);
}

void panic(char *s){
    printf("panic: ");
    printf(s);
    printf("\n");
    backtrace();
    panicked = 1;
    for( ; ; ) ;
}

void backtrace(void){
    uint64 *fp = (uint64 *)r_fp();
    uint64 *bottom = (uint64 *)PGROUNDDOWN((uint64)fp);
    printf("backtrace:\n");
    while(fp < bottom) {
        uint64 ra = *(fp - 1);
        printf("%p\n", pa - 4);
        fp = (uint64 *)*(fp - 2);
    }
}

void printfInit(void){
    initlock(&pr.lock, "pr");
    pr.locking = 1;
}


