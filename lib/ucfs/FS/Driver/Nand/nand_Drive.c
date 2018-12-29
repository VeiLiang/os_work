#include "types.h"
#include "printk.h"
#include "fs_dev.h"
#include "fs_api.h"
#include "fs_port.h"
#include "fs_conf.h"
#include "fs_lbl.h"
#include "nand_Driver.h"
#include "nand_logic.h"
#include "nand_base.h"
#include "DmpMemory.h"
#include "ucos_ii.h"
#include "SysCfg.h"
#define	NF

#ifdef	NF

#define	SZ_1M	0x100000
#define	FS_NF_MAXUNIT		2
#define	NUM_UNITS		FS_NF_MAXUNIT
#define	FS_NF_UNIT0_SIZE	(30*SZ_1M)

#define	FS_LBL_MEDIACHANGED     0x0001

static int _NumUnits;

static int _IoCtl(U8 Unit, I32 Cmd, I32 Aux, void *pBuffer);
//////////////////////////////////////////////////////////////////////////////////////////

unsigned char	bFlashBusy = 0;

unsigned char	g_PageBuffer[NF_PAGE_SIZE];
unsigned char	g_BlockBuffer[NF_BLOCK_SIZE][NF_PAGE_SIZE];

FS_u32 gDEVNFReady[FS_NF_MAXUNIT] = {0, 0,};		//Flag for if the NF driver have been initialized
FS_u32 gFS_nf_diskchange[FS_NF_MAXUNIT] = {0,0};


#ifdef SYS_UCOSII
extern OS_EVENT *NandOpt_Sem;
#endif

static __UNITINFO gUnit[FS_NF_MAXUNIT] =
{
	{0,				0x1E00000/512},
	{0x1E00000/512,	0x500000/512},
};

static BlockBufferPara g_NFBlockBuffer =
{
	0xffffffff,
	0,
};

static PageBufferPara g_NFPageBuffer =
{
	0xffffffff,
	0xffffffff,
};

static unsigned long GetUnit0Size(void)
{
	return FS_NF_UNIT0_SIZE;
}

static int SectorToPage(unsigned long unit, unsigned int sector, unsigned int *block, unsigned int *page, unsigned int *offset)
{
	*offset = ((sector + gUnit[unit].mUnitStart) % (NandInfo.PageSize >> 9))<<9;
	*page = ((sector + gUnit[unit].mUnitStart) / (NandInfo.PageSize >> 9));
	*block = (*page / NandInfo.PagePerBlk) + START_BLK;

	*page %= NandInfo.PagePerBlk;

        return 0;
}


int NF_Status(unsigned char Unit)
{
	// code1. if driver is not initilized , init it
	unsigned long i, j;
	long long fsValidSize;
//printk("************FS_DevStatus\n");

	if(g_NFBlockBuffer.mIsDirty==1)
	{
		bFlashBusy = 0;
		_IoCtl(Unit,FS_CMD_FLASH_WRITE_BACK,0,0);
	}
       
	if (!gDEVNFReady[Unit])
	{
	/*
	   The function is called the first time. For each unit,
	   the flag for 'diskchange' is set. That makes sure, that
	   FS_LBL_MEDIACHANGED is returned, if there is already a
	   media in the reader.
	*/
		for(i=0, j=0; i<FS_NF_MAXUNIT; i++)
		{
			j += gDEVNFReady[i];
		}

		if(j==0)
		{
			//get unit0 start address and size
			gUnit[0].mUnitStart = 0;
			gUnit[0].mUnitSize = GetUnit0Size()>> 9;
			//get unit1 start address and size
			gUnit[1].mUnitStart = gUnit[0].mUnitStart + gUnit[0].mUnitSize;
			fsValidSize = (NandInfo.BlkNum -BadBlkNum-START_BLK) * NandInfo.BlkSize;
			gUnit[1].mUnitSize = (fsValidSize>> 9) - gUnit[1].mUnitStart;			
		}

		gDEVNFReady[Unit] = 1;
		gFS_nf_diskchange[Unit] = 0;

		return FS_LBL_MEDIACHANGED;
	}

	if(gFS_nf_diskchange[Unit])
	{
		return FS_LBL_MEDIACHANGED;
	}

	return 0;
}

static int NandRead(FS_u32 Unit, FS_u32 Sector, char *pBuffer)
{
	unsigned int block;
	unsigned int page;
	unsigned int offset;
	unsigned int EccNum = NAND_READ_ERR_NUM;
	unsigned int Status;
	int i; //for test


#ifdef SYS_UCOSII
	INT8U err;
  	OSSemPend(NandOpt_Sem, 0, &err);
#endif

/*	if(bFlashBusy)
	{
		return -1;
	}
	else
	{
		bFlashBusy = 1;
	}    
*/
	//把扇区号转换成块、页
	SectorToPage(Unit, Sector, &block, &page, &offset);

	//检查页是否在Buffer中
	if ((block != g_NFPageBuffer.mBlockNo)  || (page != g_NFPageBuffer.mPageNo))
	{
		//把Block的数据读出来
		g_NFPageBuffer.mBlockNo = block;
		g_NFPageBuffer.mPageNo = page;
		
		//NFC_LogicPageRead(g_NFPageBuffer.mBlockNo, g_NFPageBuffer.mPageNo, g_PageBuffer);
		/**************************by zxh add*****************************/
		//检查是否在块缓存中
		if ((block == g_NFBlockBuffer.mBlockNo) )
		{
			memcpy(g_PageBuffer ,g_BlockBuffer[page],NF_PAGE_SIZE);
		}
		else 
		{
			while(EccNum--)
			{
				Status = NFC_LogicPageRead(g_NFPageBuffer.mBlockNo, g_NFPageBuffer.mPageNo, g_PageBuffer);
				if(Status == NAND_ECC_ERR)
				{
					delay(500000);
					printk("...........err.........\n");
					continue;	//遇到Ecc校验出错,重新再读
				}
				else if (Status == NAND_DMA_ERR)  //DMA 出错
				{
					bFlashBusy = 0;	
#ifdef SYS_UCOSII
				  	OSSemPost(NandOpt_Sem);
#endif					
					return -1;
				}				
				break;
			}
		}
		if(EccNum==0)
		{
			bFlashBusy = 0;	
			printk("read err...........\n");
#ifdef SYS_UCOSII
		  	OSSemPost(NandOpt_Sem);
#endif	
			return -1;
		}
	}

       //写扇区数据
	memcpy( pBuffer, (g_PageBuffer + offset),NF_SECTOR_SIZE);        
	bFlashBusy = 0;
#ifdef SYS_UCOSII
  	OSSemPost(NandOpt_Sem);
#endif		
	return NAND_SUCCE;
}

static int NandWrite(FS_u32 Unit, FS_u32 Sector, char *pBuffer)
{
	int Status;
	unsigned int block;
	unsigned int page;
	unsigned int offset;
#ifdef SYS_UCOSII
	INT8U err;
  	OSSemPend(NandOpt_Sem, 0, &err);
#endif
        
	/*if(bFlashBusy)
	{
		return -1;
	}
	else
	{
		bFlashBusy = 1;
	}*/

	//把扇区号转换成块、页
	SectorToPage(Unit, Sector, &block, &page, &offset);

	if(block != g_NFBlockBuffer.mBlockNo)
	{
		if(0xffffffff!=g_NFBlockBuffer.mBlockNo)
		{
			//回写数据
			bFlashBusy = 0;
			_IoCtl(Unit,FS_CMD_FLASH_WRITE_BACK,0,0);
		}
		g_NFBlockBuffer.mBlockNo = block;
		Status = NFC_LogicBlockRead(g_NFBlockBuffer.mBlockNo);                           
		if(Status!=NAND_SUCCE)
		{
			bFlashBusy = 0;
#ifdef SYS_UCOSII
			OSSemPost(NandOpt_Sem);
#endif		
			return -1;
		}
	}

        //写扇区数据
	 memcpy((g_BlockBuffer[page] + offset), pBuffer, NF_SECTOR_SIZE);  
        //printk("FSWrite block = %d,Page=%d,offset=%d\n",block,page,offset);
	if ((block == g_NFPageBuffer.mBlockNo) && (page == g_NFPageBuffer.mPageNo))
	{
		memcpy( (g_PageBuffer + offset), pBuffer,NF_SECTOR_SIZE);
	}
		
	g_NFBlockBuffer.mIsDirty = 1;     

	bFlashBusy = 0;
#ifdef SYS_UCOSII
	OSSemPost(NandOpt_Sem);
#endif	 	
	return NAND_SUCCE;
}

/*********************************************************************
*
*       Static code
*
**********************************************************************
*/
/*********************************************************************
*
*       _GetDriverName
*/
static const char * _GetDriverName(U8 Unit) {
	FS_USE_PARA(Unit);
	return "nf";
}
/*********************************************************************
*
*       _AddDevice
*/
static int _AddDevice(void) {
	if (_NumUnits >= NUM_UNITS) {
		return -1;
	}
	return _NumUnits++;
}

/*********************************************************************
*
*       _Read
*
*  Description:
*    FS driver function. Read a sectors from the disk.
*
*  Parameters:
*    Unit        - Unit number.
*    SectorNo    - Sector to be read from the device.
*    pBuffer     - Pointer to buffer for storing the data.
*    NumSectors  - number of sectors to be read
*
*  Return value:
*    ==0         - Sector has been read and copied to pBuffer.
*    <0          - An error has occured.
*/
static int _Read(U8 Unit, U32 SectorNo, void *pBuffer, U32 NumSectors)
{
	U32 ret = 0;
	U32 StartSectorNo = SectorNo;
	U8 *tmpBuffer = (unsigned char *)pBuffer;
	U32	Nums = NumSectors;

	while(Nums--)
	{
		ret = NandRead(Unit, StartSectorNo++, tmpBuffer);

		if(!ret)
			tmpBuffer += NF_SECTOR_SIZE;
		else
			break;
	}
	
	return ret;
}

/*********************************************************************
*
*       _Write
*
*  Description:
*    Write sectors.
*
*  Parameters:
*    Unit        - Unit number.
*    SectorNo    - First sector to be written to the device.
*    NumSectors  - Number of sectors to be written to the device.
*    pBuffer     - Pointer to buffer for holding the data.
*
*  Return value:
*    ==0         - O.K.: Sector has been written to device.
*    <0          - An error has occured.
*/
static int _Write(U8 Unit, U32 SectorNo, const void *pBuffer, U32 NumSectors, U8 RepeatSame)
{	
	U32 ret = 0;
	U32 StartSectorNo = SectorNo;
	U8 *tmpBuffer = (unsigned char *)pBuffer;
	U32	Nums = NumSectors;

	if(RepeatSame == 1)
	{
		while(Nums--)
		{
			ret = NandWrite(Unit, StartSectorNo++, tmpBuffer);

			if(ret)
				break;		
		}
	}
	else
	{
		while(Nums--)
		{
			ret = NandWrite(Unit, StartSectorNo++, tmpBuffer);

			if(!ret)
			{
				tmpBuffer += NF_SECTOR_SIZE;
			}
			else
				break;
		}	
	}

 	return ret;	                                                                                            
}

/*********************************************************************
*
*       _IoCtl
*
*  Description:
*    FS driver function. Execute device command.
*
*  Parameters:
*    Unit        - Unit number.
*    Cmd         - Command to be executed.
*    Aux         - Parameter depending on command.
*    pBuffer     - Pointer to a buffer used for the command.
*
*  Return value:
*    Command specific. In general a negative value means an error.
*/
static int _IoCtl(U8 Unit, I32 Cmd, I32 Aux, void *pBuffer) {
	FS_DEV_INFO * pDevInfo;
	int res;
	FS_u32 *info;
	long long fsValidSize;
	
	switch (Cmd) 
	{
		case FS_CMD_UNMOUNT:
			break;
				
		case FS_CMD_GET_DEVINFO: /* Get general device information */
			pDevInfo = (FS_DEV_INFO *)pBuffer;
			pDevInfo->BytesPerSector = 512;
			pDevInfo->NumSectors     = NandInfo.ChipSize/pDevInfo->BytesPerSector;

			break;

		case FS_CMD_FLASH_WRITE_BACK:
			//块buffer写回nandflash
			if ((0xffffffff != g_NFBlockBuffer.mBlockNo))
			{
				res = NFC_LogicBlockErase(g_NFBlockBuffer.mBlockNo);
				//res = 0,则成功
				if (res)
				{
					bFlashBusy = 0;
					return res;
				}

				res = NFC_LogicBlockWrite(g_NFBlockBuffer.mBlockNo);
				//res = 0,则成功
				if (res)
				{
					bFlashBusy = 0;
					return res;
				}
				g_NFBlockBuffer.mIsDirty = 0;
			}
			break;
			
		default:
			return -1;
	}
  
	return 0;
}
/*********************************************************************
*
*       _Nand_InitMedium
*
*  Description:
*    Initialize the Nand.
*
*  Parameters:
*    Unit        - Unit number.
*
*  Return value:
*    == 0                       - Device okay and ready for operation.
*    <  0                       - An error has occured.
*/
static int _InitMedium(U8 Unit)
{
	unsigned char *pageBuffer;
	unsigned char	*pOriginalSpace;
			
	pOriginalSpace = (unsigned char *)Dmpmalloc(NF_PAGE_SIZE+31);
	if(pOriginalSpace == NULL)
	{
		printk("nand file buf malloc error\n");
		return -1;
	}
	pageBuffer = (unsigned char *)(((unsigned int)pOriginalSpace +31) & 0xffffffe0);
//	printk("pageBuffer addr is 0x%x\r\n", pageBuffer);

	memset(pageBuffer, 0xFF,NF_PAGE_SIZE);
		
	if(NFC_ReadSysParaAndBBT(pageBuffer))
	{
		printk("Read Failed!!!..\n");
		Dmpfree(pOriginalSpace);
		return NAND_FAIL;
	}

	if( pOriginalSpace)	
	{	
		Dmpfree(pOriginalSpace);
		pOriginalSpace=NULL;
	}



	return 0;
}
/*********************************************************************
*
*       _GetNumUnits
*/

/*********************************************************************
*
*       _GetStatus
*
*  Description:
*    FS driver function. Get status of the disk.
*
*  Parameters:
*    Unit    - Device number.
*
*  Return value:
*    FS_MEDIA_STATE_UNKNOWN    - Media state is unknown
*    FS_MEDIA_NOT_PRESENT      - Media is not present
*    FS_MEDIA_IS_PRESENT       - Media is present
*/
static int _GetStatus(U8 Unit)
{	
	return FS_MEDIA_IS_PRESENT;
}

/*********************************************************************/
static int _GetNumUnits(void) {
  return _NumUnits;
}

/*********************************************************************
*
*       Public data
*
**********************************************************************
*/
const FS_DEVICE_TYPE FS_NF_Driver = {
  _GetDriverName,
  _AddDevice,
  _Read,
  _Write,
  _IoCtl,
  _InitMedium,
  _GetStatus,
  _GetNumUnits
};

#endif
