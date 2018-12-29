#ifndef _CORTEX_A5_MMU_
#define _CORTEX_A5_MMU_

/*----------------------------------------------------------------------------
 *        Headers
 *----------------------------------------------------------------------------*/
#include "cortex_a5_cp15.h"

/*----------------------------------------------------------------------------
 *        Exported functions
 *----------------------------------------------------------------------------*/

extern void MMU_Init(void);
extern void dma_flush_range(UINT32 ulStart, UINT32 ulEnd);
extern void dma_inv_range (UINT32 ulStart, UINT32 ulEnd);
extern void dma_clean_range(UINT32 ulStart, UINT32 ulEnd);
extern unsigned int vaddr_to_page_addr (unsigned int addr);
extern unsigned int page_addr_to_vaddr (unsigned int addr);



#endif /* #ifndef _CORTEX_A5_MMU_ */

