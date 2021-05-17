#ifndef __KERN_MM_MEMLAYOUT_H__
#define __KENR_MM_MEMLAYOUT_H__


#include <defs.h>
#include <list.h>


#define PG_RESERVED 0
#define PG_PROPERTY 1


#define KERNBASE        0xFFFFFFFFC0200000
#define KMEMSIZE        0x7E00000
#define KERNTOP         (KERNBASE + KERNSIZE)

#define KERNEL_BEGIN_PADDR  0x80200000
#define KERNEL_BEGIN_VADDR  KERNBASE
#define PHYSICAL_MEMORY_END 0x88000000
// PHYSICAL_MEMORY_END - KERNEL_BEGIN_PADDR = KMEMSIZE 0x7E000000

#define KSTACKPAGE          2
#define USERTOP             0x80000000
#define USTACKTOP           USERTOP
#define USTACKPAGE          256
#define USTACKSIZE          (USTACKPAGE * PGSIZE)

#define USERBASE            0x00200000
#define UTEXT               0x00800000
#define USTAB               USERBASE


#define USER_ACCESS(start, end)                         \
                    (USERBASE <= (start) && (start) < (end) && (end) < USERTOP)

#define KERN_ACCESS(start, end)                         \
                    (KERNBASE <= (start) && (start) < (end) && (end) < KERNTOP)

#ifndef __ASSEMBLER__



#include <defs.h>
#include <atomic.h>
#include <list.h>

typedef uintptr_t   pte_t;
typedef uintptr_t   pde_t;
typedef pte_t       swap_entry_t;


struct Page{
    int             ref;                    // 内存页的引用数
    uint64_t        flags;             // 内存页的状态标志
    unsigned int    property;      // 空闲内存块数
    list_entry_t    page_link;
    list_entry_t    pra_page_link;
    uintptr_t       pra_vaddr;
};


#define PG_RESERVED                 0
#define PG_PROPERTY                 1

#define SetPageReserved(page)       set_bit(PG_RESERVED, &((page)->flags))
#define ClearPageReserved(page)     clear_bit(PG_RESERVED, &((page)->flags))
#define PageReserved(page)          test_bit(PG_RESERVED, &((page)->flags))
#define SetPageProperty(page)       set_bit(PG_PROPERTY, &((page)->flags))
#define ClearPageProperty(page)     clear_bit(PG_PROPERTY, &((page)->flags))
#define PageProperty(page)          test_bit(PG_PROPERTY, &((page)->flags))

#define le2page(le, member) to_struct((le), struct Page, member)

typedef struct{
    list_entry_t free_list;
    unsigned int nr_free;
} free_area_t;



#endif /* !__ASSEMBLER__ */



#endif __KERN_MM_MEMLAYOUT_H__