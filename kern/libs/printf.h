#ifndef __KERN_LIBS_PRINTF_H__
#define __KERN_LIBS_PRINTF_H__

void printfinit(void);

void printf(char const*fmt, ...);

void panic(char const*s) __attribute__((noreturn));

void backtrace();

void print_logo();

#endif 
