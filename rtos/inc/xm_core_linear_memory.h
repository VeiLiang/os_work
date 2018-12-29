#ifndef __XM_CORE_LINEAR_MEMORY_H__
#define __XM_CORE_LINEAR_MEMORY_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	char		*base;				// 堆地址, 可能未对齐的地址
	u32		size;					// 线性空间字节大小
	u32	 *	virtual_address;	// 线性空间的虚拟地址, 地址已按照cache line size对齐
	u32		bus_address;		// 线性空间的物理地址, 地址已按照cache line size对齐
} xm_core_linear_memory_t;

// 分配对齐的物理线性空间
// size		线性空间的字节大小
// align		线性空间的对齐边界
// linear_memory	保存分配的线性地址空间
// 返回值
//	-1			线性空间分配失败
// 0			线性空间分配成功
int xm_core_allocate_linear_memory (u32 size, u32 align, xm_core_linear_memory_t *linear_memory);

// 释放物理线性空间
// linear_memory	待释放的线性地址空间
//	-1			线性空间释放失败
// 0			线性空间释放成功
int xm_core_free_linear_memory (xm_core_linear_memory_t * linear_memory);



#ifdef __cplusplus
}
#endif

#endif /* __XM_CORE_LINEAR_MEMORY_H__ */
