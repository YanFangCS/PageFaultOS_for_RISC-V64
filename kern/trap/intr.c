#include <types.h>
#include <param.h>
#include <proc.h>
#include <intr.h>
#include <printf.h>


void push_off(void){
    int old = intr_get();
    intr_off();

    if(curcpu()->noff == 0){
        curcpu()->intena = old;
    }
    curcpu()->noff += 1;
}

void pop_off(void){
    struct cpu *c = curcpu();

    if(intr_get()){
        panic("pop_off - interruptible");
    }
    if(c->noff > 1){
        panic("pop_off");
    }

    c->noff --;
    if(c->noff == 1 && c->intena) intr_on();
}

