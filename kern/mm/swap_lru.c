#include <swap_lru.h>


list_entry_t pra_list_head;

// 初始化时什么都不做
static int lru_init(void){
    return 0;
}

static int lru_init_mm(struct mm_struct *mm){
    list_init(&pra_list_head);
    mm->sm_priv = &pra_list_head;
    return 0;
}

// 时钟中断时什么都不做
static int lru_tick_event(struct mm_struct *mm){
    return 0;
}

static int lru_map_swappable(struct mm_struct *mm, uintptr_t addr, struct Page *page, int swap_in){
    list_entry_t *head  = (list_entry_t *)mm->sm_priv;
    list_entry_t *entry = &(page->pra_page_link);
    list_entry_t *temp = head;
    while((temp = list_next(temp)) != head){
        if(temp == entry){
            list_del(temp);
            break;
        }
    }

    assert(head != NULL && entry != NULL);

    list_add_before(head, entry);
    return 0;
}

static int lru_set_unswappable(struct mm_struct *mm, uintptr_t addr){
    return 0;
}

static int lru_swap_out_victim(struct mm_struct *mm, struct Page ** ptr_page, int in_tick){
    list_entry_t *head = (list_entry_t *)mm->sm_priv;
    assert(head != NULL);
    assert(in_tick == 0);

    list_entry_t * entry = list_next(head);
    list_del(entry);
    *ptr_page = le2page(entry, pra_page_link);
    return 0;
}

static int lru_check_swap(void){
    cprintf("write Virt Page c in fifo_check_swap\n");
    *(unsigned char *)0x3000 = 0x0c;
    assert(pgfault_num==4);
    cprintf("write Virt Page a in fifo_check_swap\n");
    *(unsigned char *)0x1000 = 0x0a;
    assert(pgfault_num==4);
    cprintf("write Virt Page d in fifo_check_swap\n");
    *(unsigned char *)0x4000 = 0x0d;
    assert(pgfault_num==4);
    cprintf("write Virt Page b in fifo_check_swap\n");
    *(unsigned char *)0x2000 = 0x0b;
    assert(pgfault_num==4);
    cprintf("write Virt Page e in fifo_check_swap\n");
    *(unsigned char *)0x5000 = 0x0e;
    assert(pgfault_num==5);
    cprintf("write Virt Page b in fifo_check_swap\n");
    *(unsigned char *)0x2000 = 0x0b;
    assert(pgfault_num==5);
    cprintf("write Virt Page a in fifo_check_swap\n");
    *(unsigned char *)0x1000 = 0x0a;
    assert(pgfault_num==6);
    cprintf("write Virt Page b in fifo_check_swap\n");
    *(unsigned char *)0x2000 = 0x0b;
    assert(pgfault_num==7);
    cprintf("write Virt Page c in fifo_check_swap\n");
    *(unsigned char *)0x3000 = 0x0c;
    assert(pgfault_num==8);
    cprintf("write Virt Page d in fifo_check_swap\n");
    *(unsigned char *)0x4000 = 0x0d;
    assert(pgfault_num==9);
    cprintf("write Virt Page e in fifo_check_swap\n");
    *(unsigned char *)0x5000 = 0x0e;
    assert(pgfault_num==10);
    cprintf("write Virt Page a in fifo_check_swap\n");
    assert(*(unsigned char *)0x1000 == 0x0a);
    *(unsigned char *)0x1000 = 0x0a;
    assert(pgfault_num==11);
    return 0;
}


struct swap_manager swap_manager_lru = {
    .name               =   "lru swap manager",
    .init               =   &lru_init,
    .init_mm            =   &lru_init_mm,
    .tick_event         =   &lru_tick_event,
    .map_swappable      =   &lru_map_swappable,
    .set_unswappable    =   &lru_set_unswappable,
    .check_swap         =   &lru_check_swap,
};