#ifndef __KERN_MM_MMU_H__
#define __KERN_MM_MMU_H__

#ifndef __ASSEMBLER__
#include <defs.h>
#endif
  
// RISC-V uses 39-bit virtual address to access 56-bit physical address
// linear address 'la'
// +------9------+-------9-------+------9-----+--------12------+
// |    VPN[2]   |     VPN[1]    |    VPN[0]  |   page offset  |
// +------9------+-------9-------+------9-----+--------12------+

/*
 * RV32Sv32 page table entry:
 * | 31 10 | 9             7 | 6 | 5 | 4  1 | 0
 *    PFN    reserved for SW   D   R   TYPE   V
 *
 * RV64Sv39 / RV64Sv48 page table entry:
 * | 63           48 | 47 10 | 9             7 | 6 | 5 | 4  1 | 0
 *   reserved for HW    PFN    reserved for SW   D   R   TYPE   V
 */

/*
 * RV64Sv39 / page table entry:
 * | 63     54 | 53  28 | 27  19 | 18  10 | 9 8 | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 
 *   reserved    PPN[2]   PPN[1]   PPN[0]   RSW   D   A   G   U   X   W   R   V
 */

#define VPN0SHIFT        12
#define VPN1SHIFT       21
#define VPN2SHIFT       30
#define PTE_PPN_SHIFT   10

#define NPDEENTRY   512
#define NPTEENTRY   512

// decompose la to vpn0, vpn1, vpn2 and page offset
#define VPN0(la)    ((((uintptr_t)(la)) >> VPN0SHIFT) & 0X1FF)
#define VPN1(la)    ((((uintptr_t)(la)) >> VPN1SHIFT) & 0x1FF)
#define VPN2(la)    ((((uintptr_t)(la)) >> VPN2SHIFT) & 0x1FF)
#define PGOFF(la)   (((uintptr_t)(la)) & 0xFFF)

#define PGADDR(vp2, vpn1, vpn0, off) ((uintptr_t)(((vpn2) << VPN2SHIFT)) | ((vpn1) << VPN1SHIFT) | ((vpn0) << VPN0SHIFT) | off)
// underconstruction
#define PTE_ADDR(pte)  0x10


// page table size and entries
#define PGSIZE      4096
#define PGSHIFT     12
#define PTSHIFT     21
#define PTSIZE      (PGSIZE * NPTEENTRY)
#define PDSIZE      (PTSIZE * NPDEETEY)


//page table entry fields
#define PTE_V       0x001   // valid
#define PTE_R       0x002   // read
#define PTE_W       0x004   // write
#define PTE_X       0x008   // execute
#define PTE_U       0x010   // user
#define PTE_G       0x020   // global
#define PTE_A       0x040   // accessed
#define PTE_D       0x080   


#define PAGE_TABLE_DIR  (PTE_V)
#define READ_ONLY       (PTE_V | PTE_R)
#define READ_WRITE      (PTE_V | PTE_R | PTE_W)
#define EXEC_ONLY       (PTE_V | PTE_E)
#define READ_EXEC       (PTE_V | PTE_X)
#define READ_WRITE_EXEC (PTE_V | PTE_R | PTE_E)
#define PTE_USER        (PTE_V | PTE_R | PTE_W | PTE_X | PTE_U | PTE_V)

#endif /* !__KERN_MM_MMU_H__ */
