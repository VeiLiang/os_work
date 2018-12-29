/////////////////////////////////////////////////////////////////////////////////////////
#ifndef _NAND_DRIVER_H_
#define _NAND_DRIVER_H_

#ifdef __cplusplus
extern "C" {
#endif

#define FS_NF_WCACHE_NUM	16
#define FS_NF_RCACHE_NUM	16

//#define FS_CMD_FLASH_WRITE_BACK		1  // removed waring

#define NF_PAGE_SIZE	2048    //最大支持4K
#define NF_BLOCK_SIZE	64     //最多支持128个
#define NF_SECTOR_SIZE  512

#define NAND_READ_ERR_NUM		10

typedef struct
{
	unsigned long mUnitStart;		//unit is sector
	unsigned long mUnitSize;		//unit is sector
} __UNITINFO;

typedef struct
{
	unsigned int mBlockNo;	/*The block NO. that is buffered, 0xffffffff 
				* means no block is buffered
				*/
	unsigned int mIsDirty;
}BlockBufferPara;


typedef struct
{
	int mBlockNo;
	int mPageNo;
}PageBufferPara;

extern unsigned char g_PageBuffer[NF_PAGE_SIZE];
extern unsigned char g_BlockBuffer[NF_BLOCK_SIZE][NF_PAGE_SIZE];

#ifdef __cplusplus
}
#endif

#endif
