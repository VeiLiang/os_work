#include <stdlib.h>
#include "hardware.h"
#include "xm_core_linear_memory.h"
#include "xm_core.h"

int xm_core_allocate_linear_memory (u32 size, u32 align, xm_core_linear_memory_t *linear_memory)
{
	void *base;
	if(linear_memory == NULL)
		return -1;
	base = (void *)kernel_malloc (size + align * 2);
	if(base == NULL)
		return -1;
	
	// size按最小32字节分配, 一个完整的cache line (Cortex A5)
	size = (size + align - 1) & ~(align - 1);
	
	linear_memory->size = size;
	linear_memory->base = base;
	
	linear_memory->virtual_address = (u32 *)((u32)(linear_memory->base + align - 1) & ~(align - 1)); 
	
	// 仅无效对齐的区域
	dma_inv_range ((u32)linear_memory->virtual_address, size + (u32)linear_memory->virtual_address);
	 
	// 将虚拟地址转换为物理地址
	linear_memory->bus_address = (u32)vaddr_to_page_addr ((u32)linear_memory->virtual_address);
	linear_memory->virtual_address = (u32 *)linear_memory->bus_address;
	// dma_inv_range ((u32)base, (u32)base + size + align);
	return 0;
}

int xm_core_free_linear_memory (xm_core_linear_memory_t * linear_memory)
{
	if(linear_memory == NULL || linear_memory->virtual_address == NULL)
		return -1;
	
	if(linear_memory->base)
	{
		kernel_free (linear_memory->base);
	}
	linear_memory->base = 0;
	linear_memory->size = 0;
	linear_memory->bus_address = 0;
	linear_memory->virtual_address = 0;
	return 0;
}