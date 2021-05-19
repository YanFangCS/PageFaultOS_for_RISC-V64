#include <swap_lru.h>


static int lru_init(void){

}

static int lru_init_mm(struct mm_struct *mm){

}

static int lru_tick_event(struct mm_struct *mm){

}

static int lru_map_swappable(struct mm_struct *mm, uintptr_t addr, struct Page *page, int swap_in){

}

static int lru_set_unswappable(struct mm_struct *mm, uintptr_t addr){

}

static int lru_swap_out_victim(struct mm_struct *mm, struct Page ** ptr_page, int in_tick){

}

static int lru_check_swap(void){

}


struct swap_manager swap_manager_lru = {
    // all functions are not done until now
    .name               =   "lru swap manager",
    .init               =   &lru_init,
    .init_mm            =   &lru_init_mm,
    .tick_event         =   &lru_tick_event,
    .map_swappable      =   &lru_map_swappable,
    .set_unswappable    =   &lru_set_unswappable,
    .check_swap         =   &lru_check_swap,
};