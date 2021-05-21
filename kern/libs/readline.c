#include <stdio.h>

#define BUFSIZE 1024
static char buf[BUFSIZE];


char * readline(char *prompt){
    if(prompt == NULL){
        cprintf("prompt is invalid\n");
    }
    int i = 0, c;
    while(1){
        c = getchar();
        if(c < 0){
            return NULL;
        }else if(c >= ' ' && i < BUFSIZE - 1){
            cputchar(c);
            buf[i++] = c;
        }else if(c == '\b' && i > 0){
            cputchar(c);
            --i;
        }else if(c == '\r' || c == '\n'){
            cputchar(c);
            buf[i] = '\0';
            return buf;
        }
    }
}