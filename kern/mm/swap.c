#include <swap.h>



#define BEGIN_VALID_ADDR    0x1000

volatile int swap_init_ok = 0;

int swap_init(void){

}

int swap_init_mm(struct mm_struct *mm){

}

int swap_tick_event(struct mm_struct *mm){

}

int swap_map_swappable(struct mm_struct *mm, uintptr_t addr, struct Page *page, int swap_in){

}

int swap_set_unswappable(struct mm_struct *mm, uintptr_t addr){

}

int swap_out(struct mm_struct *mm, int n, int in_tick){

}

int swap_in(struct mm_struct *mm, uintptr_t addr, struct Page **ptr_result){

}



