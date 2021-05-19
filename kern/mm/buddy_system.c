#include <buddy_system.h>
#include <pmm.h>
#include <list.h>
#include <memlayout.h>
#include <string.h>

/* In the first fit algorithm, the allocator keeps a list of free blocks (known as the free list) and,
   on receiving a request for memory, scans along the list for the first block that is large enough to
   satisfy the request. If the chosen block is significantly larger than that requested, then it is 
   usually split, and the remainder added to the list as another free block.
   Please see Page 196~198, Section 8.2 of Yan Wei Min's chinese book "Data Structure -- C programming language"
*/

// memory allocator implement -- buddy system implement

free_area_t free_area;
#define free_list (free_area.free_list)
#define nr_free   (free_area.nr_free)



void bs_init(){
    list_init(&free_list);
    nr_free = 0;
}

/*
 * bs_init_memmap:  CALL GRAPH: kern_init --> pmm_init-->page_init-->init_memmap--> pmm_manager->init_memmap
 *              This fun is used to init a free block (with parameter: addr_base, page_number).
 *              First you should init each page (in memlayout.h) in this free block, include:
 *                  p->flags should be set bit PG_property (means this page is valid. In pmm_init fun (in pmm.c),
 *                  the bit PG_reserved is setted in p->flags)
 *                  if this page  is free and is not the first page of free block, p->property should be set to 0.
 *                  if this page  is free and is the first page of free block, p->property should be set to total num of block.
 *                  p->ref should be 0, because now p is free and no reference.
 *                  We can use p->page_link to link this page to free_list, (such as: list_add_before(&free_list, &(p->page_link)); )
 *              Finally, we should sum the number of free mem block: nr_free+=n
 */
static void bs_init_memmap(struct Page * base, size_t n){
    assert(n > 0);
    struct Page *temp = base;
    for(; temp != base + n; ++temp){
        assert(PageReserved(temp));
        temp->property = 0;
        temp->flags = 0;
    }
    base->property = n;
    SetPageProperty(base);
    nr_free += n;

    if(list_empty(&free_list)){
        list_add(&free_list, &(base->page_link));
    } else {
        list_entry_t * le = &free_list;
        while((le = list_next(le)) != &free_list){
            struct Page * page = le2page(le, page_link);
            if(base < page){
                list_add_before(le, &(base->page_link));
                break;
            } else if (list_next(le) == &free_list){
                list_add(le, &(base->page_link));
            }
        }
    } 
}


static struct Page * bs_alloc_pages(size_t n){
    assert(n > 0);
    if(n > nr_free) return NULL;

    struct Page * page;
    list_entry_t *le = &free_list;
    while((le = list_next(le)) != &free_list){
        struct Page * p = le2page(le, page_link);
        if(p->property >= n){
            page = n;
            break;
        }
    }    
    
    if(page != NULL){
        list_entry_t * prev = list_prev(&(page->page_link));
        list_del(&(page->page_link));
        if(page->property > n){
            struct Page * p = page + n;
            p->property -= n;
            SetPageProperty(p);
            list_add(prev, &(p->page_link));
        }
        nr_free -= n;
        ClearPageProperty(page);
    }
    return page;
}

static void bs_free_pages(struct Page * base, size_t n){
    assert(n > 0);
    struct Page * p = base;
    for(; p != base + n; ++p){
        assert(!PageReserved(p) && !PageProperty(p));
        p->flags = 0;
        page_ref_set(p, 0);
    }

    base->property = n;
    SetPageProperty(base);

    list_entry_t * le = list_next(&free_list);
    while( le != &free_list){
        p = le2page(le, page_link);
        le = list_next(le);
        if(base + base->property == p){
            base->property += p->property;
            ClearPageProperty(p);
            list_del(&(p->page_link));
        } else if (p + p->property == base){
            p->property += base->property;
            ClearPageProperty(base);
            base = p;
            list_del(&(p->page_link));
        }
    }
    nr_free += n;
    list_add(le, &(p->page_link));
}

static size_t bs_nr_free_pages(void){
    return nr_free;
}

static void bs_check(void){
    int count = 0, total = 0;
    list_entry_t *le = &free_list;
    while ((le = list_next(le)) != &free_list) {
        struct Page *p = le2page(le, page_link);
        assert(PageProperty(p));
        count ++, total += p->property;
    }
    assert(total == nr_free_pages());

    basic_check();

    struct Page *p0 = alloc_pages(5), *p1, *p2;
    assert(p0 != NULL);
    assert(!PageProperty(p0));

    list_entry_t free_list_store = free_list;
    list_init(&free_list);
    assert(list_empty(&free_list));
    assert(alloc_page() == NULL);

    unsigned int nr_free_store = nr_free;
    nr_free = 0;

    free_pages(p0 + 2, 3);
    assert(alloc_pages(4) == NULL);
    assert(PageProperty(p0 + 2) && p0[2].property == 3);
    assert((p1 = alloc_pages(3)) != NULL);
    assert(alloc_page() == NULL);
    assert(p0 + 2 == p1);

    p2 = p0 + 1;
    free_page(p0);
    free_pages(p1, 3);
    assert(PageProperty(p0) && p0->property == 1);
    assert(PageProperty(p1) && p1->property == 3);

    assert((p0 = alloc_page()) == p2 - 1);
    free_page(p0);
    assert((p0 = alloc_pages(2)) == p2 + 1);

    free_pages(p0, 2);
    free_page(p2);

    assert((p0 = alloc_pages(5)) != NULL);
    assert(alloc_page() == NULL);

    assert(nr_free == 0);
    nr_free = nr_free_store;

    free_list = free_list_store;
    free_pages(p0, 5);

    le = &free_list;
    while ((le = list_next(le)) != &free_list) {
        struct Page *p = le2page(le, page_link);
        count --, total -= p->property;
    }
    assert(count == 0);
    assert(total == 0);
}


const struct pmm_manager bs_system = {
    .name = "buddy system",
    .init = bs_init,
    .init_memmap = bs_init_memmap,
    .alloc_pages = bs_alloc_pages,
    .free_pages = bs_free_pages,
    .nr_free_pages = bs_nr_free_pages,
    .check = bs_check,
};