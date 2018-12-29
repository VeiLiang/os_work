#ifndef __XM_CORE_LINEAR_MEMORY_H__
#define __XM_CORE_LINEAR_MEMORY_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	char		*base;				// �ѵ�ַ, ����δ����ĵ�ַ
	u32		size;					// ���Կռ��ֽڴ�С
	u32	 *	virtual_address;	// ���Կռ�������ַ, ��ַ�Ѱ���cache line size����
	u32		bus_address;		// ���Կռ�������ַ, ��ַ�Ѱ���cache line size����
} xm_core_linear_memory_t;

// ���������������Կռ�
// size		���Կռ���ֽڴ�С
// align		���Կռ�Ķ���߽�
// linear_memory	�����������Ե�ַ�ռ�
// ����ֵ
//	-1			���Կռ����ʧ��
// 0			���Կռ����ɹ�
int xm_core_allocate_linear_memory (u32 size, u32 align, xm_core_linear_memory_t *linear_memory);

// �ͷ��������Կռ�
// linear_memory	���ͷŵ����Ե�ַ�ռ�
//	-1			���Կռ��ͷ�ʧ��
// 0			���Կռ��ͷųɹ�
int xm_core_free_linear_memory (xm_core_linear_memory_t * linear_memory);



#ifdef __cplusplus
}
#endif

#endif /* __XM_CORE_LINEAR_MEMORY_H__ */
