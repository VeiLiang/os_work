#include "types.h"
#include <assert.h>
#include <xm_printf.h>
#include "cortex_a5_cp15.h"
#include "cortex_a5_mmu.h"
#include <stdio.h>

#define MMU_ENABLE	

#ifdef MMU_ENABLE
#define DCACHE_ENABLE
#define ICACHE_ENABLE

//#define L2_CACHE_ENABLE
#endif

#pragma data_alignment=16384
__no_init unsigned int mmu_tlb_table[4096];

#define _MMUTT_STARTADDRESS		((unsigned int)mmu_tlb_table)

/**
 * \brief Initializes MMU.
 * \param pTB  Address of the translation table.
 */
void MMU_Initialize(void)
{
    unsigned int index;
    unsigned int addr;
	 	 
	 unsigned int *pTB = (unsigned int *)_MMUTT_STARTADDRESS;

    /* Reset table entries */
    // 初始化所有表项无效
    for (index = 0; index < 4096; index++)
        pTB[index] = 0x00;
    
    // 将DDR物理空间(0x80000000 ~ 0x80100000)映射到逻辑地址空间0x00000000 ~ 0x00100000    
    // 中断向量
    pTB[0x000] = (0x800 << 20)| // Physical Address
                 //   ( 1 << 12)| // TEX[0]
                    ( 3 << 10)| // Access in supervisor mode (AP)
                    ( 0xF <<  5)| // Domain 0xF
                    ( 1 <<  4)| // (XN)
                    ( 0 <<  3)| // C bit : cachable => YES
						  ( 0 <<  2)| // B bit : write-back => YES  
                    ( 2 <<  0); // Set as 1 Mbyte section

	 
    /* section RAM 0, 与Codec互斥使用 */
    /* SRAM address (after remap) 0x0010_0000 */
    pTB[0x001] = (0x001 << 20)| // Physical Address
                   // ( 1 << 12)| // TEX[0]
                    ( 3 << 10)| // Access in supervisor mode (AP)
                  ( 0xF <<  5)| // Domain 0xF
                    ( 1 <<  4)| // (XN)
                    ( 0 <<  3)| // C bit : cachable => YES
                    ( 0 <<  2)| // B bit : write-back => YES
                    ( 2 <<  0); // Set as 1 Mbyte section

    /* section RAM 1, 与Codec互斥使用 */
    /* SRAM address (after remap) 0x0020_0000 */
    pTB[0x002] = (0x002 << 20)| // Physical Address
                   // ( 1 << 12)| // TEX[0]
                    ( 3 << 10)| // Access in supervisor mode (AP)
                  ( 0xF <<  5)| // Domain 0xF
                    ( 1 <<  4)| // (XN)
                    ( 0 <<  3)| // C bit : cachable => YES
                    ( 0 <<  2)| // B bit : write-back => YES
                    ( 2 <<  0); // Set as 1 Mbyte section

    /* 片内SRAM address (after remap) 0x0030_0000 */
    pTB[0x003] = (0x003 << 20)| // Physical Address
                   // ( 1 << 12)| // TEX[0]
                    ( 3 << 10)| // Access in supervisor mode (AP)
                  ( 0xF <<  5)| // Domain 0xF
                    ( 1 <<  4)| // (XN)
                    ( 1 <<  3)| // C bit : cachable => YES
                    ( 1 <<  2)| // B bit : write-back => YES
                    ( 2 <<  0); // Set as 1 Mbyte section

    // AHB2APB0 0x40000000 256MB
	 for(addr = 0x400; addr < 0x500; addr++)
   	 pTB[addr] = ((addr >> 4) << 24)| // Physical Address
                      ( 1 << 18)|   // 16MB Supersection
                      ( 3 << 10)| // Access in supervisor mode (AP)
                      ( 0 <<  5)| // Domain 0，Supersection only support domain 0
                      ( 1 <<  4)| // (XN)
                      ( 0 <<  3)| // C bit : cachable => NO
                      ( 0 <<  2)| // B bit : write-back => NO
                      ( 2 <<  0); // Set as 1 Mbyte section

	// rs_nand	32'h5000_0000,256M
	for(addr = 0x500; addr < 0x600; addr++)
   	 pTB[addr] = (addr << 20)| // Physical Address
                      ( 3 << 10)| // Access in supervisor mode (AP)
                    ( 0xF <<  5)| // Domain 0xF
                      ( 1 <<  4)| // (XN)
                      ( 0 <<  3)| // C bit : cachable => NO
                      ( 0 <<  2)| // B bit : write-back => NO
                      ( 2 <<  0); // Set as 1 Mbyte section
	 
	// sdmmc	   32'h6000_0000,128M
	// sdmmc1	32'h6800_0000,128M
	for(addr = 0x600; addr < 0x700; addr++)
   	 pTB[addr] = (addr << 20)| // Physical Address
                      ( 3 << 10)| // Access in supervisor mode (AP)
                    ( 0xF <<  5)| // Domain 0
                      ( 1 <<  4)| // (XN)
                      ( 0 <<  3)| // C bit : cachable => NO
                      ( 0 <<  2)| // B bit : write-back => NO
                      ( 2 <<  0); // Set as 1 Mbyte section

	 
	 // AHB_CTL 0x70000000 256MB
	 for(addr = 0x700; addr < 0x800; addr++)
   	 pTB[addr] = ((addr >> 4) << 24)| // Physical Address
                      ( 1 << 18)| // 16MB Supersection
                      ( 3 << 10)| // Access in supervisor mode (AP)
                      ( 0 <<  5)| // Domain 0，Supersection only support domain 0
                      ( 1 <<  4)| // (XN)
                      ( 0 <<  3)| // C bit : cachable => NO
                      ( 0 <<  2)| // B bit : write-back => NO
                      ( 2 <<  0); // Set as 1 Mbyte section
	 
	 // 定义128MB物理内存 (N141 128封装内置128MB DDR2 DRAM)
	 // DDR SDRAM 0x80000000 ~ 0x88000000
#if 0    
	 for(addr = 0x800; addr < 0x880; addr++)
    {
		 pTB[addr] = (addr << 20)|   // Physical Address
                      ( 3 << 10)|   // Access in supervisor mode (AP)
                      ( 1 << 12)|   // TEX[0]
                    ( 0xF <<  5)|   // Domain 0xF
                      ( 1 <<  4)|   // (XN)
                      ( 1 <<  3)|   // C bit : cachable => YES
                      ( 1 <<  2)|   // B bit : write-back => YES
							//( 0 <<  2)|   // B bit : write-back => YES	 
                      ( 2 <<  0);   // Set as 1 Mbyte section
	 }
	 
#else    
	 for(addr = 0x800; addr < 0x880; addr++)
    {
		 pTB[addr] = ( (addr >> 4) << 24)|   // Physical Address
                      ( 1 << 18)|   // 16MB Supersection
                      ( 3 << 10)|   // Access in supervisor mode (AP)
                      ( 1 << 12)|   // TEX[0]
                      ( 0 <<  5)|   // Domain 0x0，Supersection only support domain 0
                      ( 0 <<  4)|   // (XN)
                      ( 1 <<  3)|   // C bit : cachable => YES
                      ( 1 <<  2)|   // B bit : write-back => YES
                      ( 2 <<  0);   // Set as 1 Mbyte section
	 }
   // unsigned int ID_MMFR3 = CP15_ReadID_MMFR3();
   // printf ("16MB supersections supported %s\n", 
   //        ((ID_MMFR3 >> 28) == 0) ? "YES" : "NO");
    
 //   unsigned int TLBHR = CP15_ReadTLBHR();
    // TLBHR |= (1 << 3);  // [3] Set if 16MB supersections are present in the TLB
                           // [2] Set if 1MB sections are present in the TLB
  //  TLBHR = (1 << 2) | (1 << 3);  // [3] Set if 16MB supersections are present in the TLB
 //   CP15_WriteTLBHR (TLBHR);
 //   TLBHR = CP15_ReadTLBHR();
 //   printf ("TLBHR=0x%08x\n", TLBHR);
    
    /*
    unsigned int DACR = CP15_ReadDACR();
    DACR &= ~0x03;
    // b01 = Client. Accesses are checked against the access permission bits in the TLB entry.
    DACR |= 0x01;
    CP15_WriteDACR (DACR);
    */
#endif
    
    // 0x90000000 ~ 0x98000000 虚拟地址空间，non-cachable
    // 映射到物理地址0x80000000 ~ 0x88000000
	 for(addr = 0x900; addr < 0x980; addr++)
    {
		 pTB[addr] = ( (addr >> 4) << 24)|   // Physical Address
                      ( 1 << 18)|   // 16MB Supersection
                      ( 3 << 10)|   // Access in supervisor mode (AP)
                      ( 1 << 12)|   // TEX[0]
                      ( 0 <<  5)|   // Domain 0x0，Supersection only support domain 0
                      ( 0 <<  4)|   // (XN)
                      ( 0 <<  3)|   // C bit : cachable => YES
                      ( 0 <<  2)|   // B bit : write-back => YES
                      ( 2 <<  0);   // Set as 1 Mbyte section
	 }
    

    CP15_WriteTTB((unsigned int)pTB);
    /* Program the domain access register */
    CP15_WriteDomainAccessControl(0xC0000003); // domain 0 & 15: access are not checked
}

void dma_inv_range (UINT32 ulStart, UINT32 ulEnd);

void MMU_Init(void)
{
	unsigned int ACTLR;
#ifdef L2_CACHE_ENABLE	
#ifdef L2_CACHE_EXCLUSIVE_CACHE_ENABLE
	CP15_SETL1L2EX();
#endif
#endif
	
	if(CP15_IsIcacheEnabled())
		CP15_DisableIcache();
	if(CP15_IsDcacheEnabled())
		CP15_DisableDcache();

	if(CP15_IsMMUEnabled())
		CP15_DisableMMU();
		
	MMU_Initialize ();
	
	ACTLR = CP15_ReadAuxiliaryControl();
	// [14:13] L1PCTL L1 Data prefetch control. 
	//	The value of this field determines the maximum number of outstanding data
	//	prefetches allowed in the L1 memory system, not counting those generated by software load/PLD instructions:
	//	00 = prefetch disabled
	//	01 = 1 outstanding prefetch allowed
	//	10 = 2 outstanding prefetches allowed
	//	11 = 3 outstanding prefetches allowed.
	//ACTLR &= ~(0x3 << 13);
	
	// [10] DODMBS Disable optimized Data Memory Barrier behavior.
	//ACTLR |= (1 << 10);
	
	// [11] DWBST Disable data write bursts to normal non-cacheable memory. Set in conjunction with RADIS to disable data
	// write bursts to cacheable memory..	
	// [12] RADIS Disable Data Cache read-allocate mode. See Read allocate mode on page 2-4.
	//ACTLR |= (1 << 12) | (1 << 11);

   // Beacuse all ARMv7-A processors can do speculative memory accesses(指令/数据预取), 
   // it will also be nessary to invalidate after using the DMA.
	// [17] RSDIS Disable return stack operation.
	//ACTLR |= (1 << 17);
	ACTLR &= ~(1 << 17);		// enable return stack operation
	
	CP15_WriteAuxiliaryControl(ACTLR);
	
   CP15_EnableMMU(); 
		
#ifdef ICACHE_ENABLE
	CP15_EnableIcache(); 
#endif
#ifdef DCACHE_ENABLE
	CP15_EnableDcache(); 
#endif

#ifdef L2_CACHE_ENABLE		
	cache_l2c310_init();	
#endif
}

#ifdef DCACHE_ENABLE

void dma_inv_range (UINT32 ulStart, UINT32 ulEnd)
{	
    
	if(ulStart >= 0x90000000 && ulStart < 0x98000000)
		return;
	
	//if(ulStart & 0x1F /* || ulEnd & 0x1F */)
	{
		//printf ("inv non %08x\n", ulStart);
	}
#ifdef L2_CACHE_ENABLE		
	
#ifdef L2_CACHE_EXCLUSIVE_CACHE_ENABLE	
#error	only non-exclusive cache support
#endif
	
	// The robust code sequence for invalidation with a non-exclusive cache arrangement
	// InvalLevel2 Address ; forces the address out past level 2
	// CACHE SYNC ; Ensures completion of the L2 inval
	// InvalLevel1 Address ; This is broadcast within the cluster
	// DSB ; Ensure completion of the inval as far as Level 2.		
	cache_l2c310_invalidate_dcache_for_dma (ulStart, ulEnd);
	cache_l2c310_cache_sync ();
	
#endif
	
	CP15_invalidate_dcache_for_dma (ulStart, ulEnd);
}

void dma_clean_range(UINT32 ulStart, UINT32 ulEnd)
{
	if(ulStart >= 0x90000000 && ulStart < 0x98000000)
		return;
	//if(ulStart & 0x1F /*|| ulEnd & 0x1F*/ )
	{
	//	printf ("clean non %08x\n", ulStart);
	}

	CP15_clean_dcache_for_dma (ulStart, ulEnd);
	
#ifdef L2_CACHE_ENABLE		
	//	The pseudo code sequence for supporting this scenario is:
	//	CleanLevel1 Address ; This is broadcast within the cluster
	//	DSB ; Ensure completion of the clean as far as Level 2
	//	CleanLevel2 Address ; forces the address out past level 2
	//	CACHE SYNC ; Ensures completion of the L2 clean	
	cache_l2c310_clean_dcache_for_dma (ulStart, ulEnd);
	cache_l2c310_cache_sync ();
#endif
}

void dma_flush_range(UINT32 ulStart, UINT32 ulEnd)
{	
	if(ulStart >= 0x90000000 && ulStart < 0x98000000)
		return;
	//if(ulStart & 0x1F)	// || ulEnd & 0x1F)
	{
		//printf ("flush non %08x\n", ulStart);
	}
#ifdef L2_CACHE_ENABLE	
	//	So the code sequence for Clean and Invalidate
	//	over the two levels of cache must be:
	//
	//	CleanLevel1 Address ; This is broadcast within the cluster
	//	DSB ; Ensure completion of the clean as far as Level 2
	//	Clean&InvalLevel2 Address ; forces the address out past level 2
	//	SYNC ; Ensures completion of the L2 inval
	//	Clean&InvalLevel1 Address ; This is broadcast within the cluster
	//	DSB ; Ensure completion of the clean&inval as far as Level 2 (no data lost)	
	
	CP15_clean_dcache_for_dma (ulStart, ulEnd);
	cache_l2c310_flush_dcache_for_dma (ulStart, ulEnd);
	cache_l2c310_cache_sync ();
#endif
	CP15_flush_dcache_for_dma (ulStart, ulEnd);	

}

#else

void dma_inv_range(UINT32 ulStart, UINT32 ulEnd)
{
	return;
}

void dma_clean_range(UINT32 ulStart, UINT32 ulEnd)
{
	return;
}

void dma_flush_range(UINT32 ulStart, UINT32 ulEnd)
{
	return;
}

#endif

#ifdef MMU_ENABLE
// 虚拟地址到页地址(非Cache区域)
unsigned int vaddr_to_page_addr (unsigned int addr)
{
	if(addr < 0x80000000)
		return addr;
	else if(addr >= 0x88000000)
		return addr;
	return addr + 0x10000000;
}

unsigned int page_addr_to_vaddr (unsigned int addr)
{
	if(addr < 0x90000000)
		return addr;
	else if(addr >= 0x98000000)
		return addr;
	return addr - 0x10000000;
}
#else
unsigned int vaddr_to_page_addr (unsigned int addr)
{
	return addr;
}
unsigned int page_addr_to_vaddr (unsigned int addr)
{
	return addr;
}

#endif