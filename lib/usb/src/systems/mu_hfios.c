/******************************************************************
*                                                                *
*        Copyright Mentor Graphics Corporation 2004              *
*                                                                *
*                All Rights Reserved.                            *
*                                                                *
*    THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION *
*  WHICH IS THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS   *
*  LICENSORS AND IS SUBJECT TO LICENSE TERMS.                    *
*                                                                *
******************************************************************/

/*
 * uC/FS HFI implementation (bridge between mass-storage class
 * driver and uC/FS)
 * $Revision: 1.5 $
 */

#include "mu_cdi.h"
#include "mu_mem.h"
#include "mu_stdio.h"
#include "mu_diag.h"
#include "mu_hfi.h"
#include "mu_mapi.h"

#include "includes.h"

#include "fs.h"
#include "fs_types.h"
#include "types.h"

/****************************** TYPES *****************************/
#ifdef FS_USB_MAXUNITS
  #define MUSB_UCFS_UNIT_COUNT          FS_USB_MAXUNITS
#else
  #define MUSB_UCFS_UNIT_COUNT          4
#endif

#define READ_TIME					100
#define WRITE_TIME					150

/**
 * Data for bridge between HFI and uC/FS
 * @field DeviceInfo HFI device info
 * @field pDevice pointer to HFI dispatch table
 * @field hTransferLock semaphore to serialize transfer requests
 * due to mechanism for returning status
 * @field hTransferDone semaphore signalled when transfer is complete
 * @field wActualBlocks actual blocks transferred
 * @field bMediumReady TRUE if medium is ready; FALSE otherwise
 * @field bMediumChanged TRUE if medium has changed; FALSE otherwise
 */
typedef struct
{
	MUSB_HfiDeviceInfo DeviceInfo;
	MUSB_HfiDevice* pDevice;
	//OS_SEM stStartTransfer;
	//void* hStartTransfer;
	OS_EVENT* hStartTransfer;
	//OS_SEM stTransferDone;
	//void* hTransferDone;
	OS_EVENT* hTransferDone;
	uint16_t wActualBlocks;
	uint8_t bMediumReady;
	uint8_t bMediumChanged;
	uint8_t bDeviceStatus;
	uint32_t iDrive_start[MUSB_UCFS_UNIT_COUNT];		/* Partition start sector address.  */
    	uint32_t iDrive_end[MUSB_UCFS_UNIT_COUNT];			/* Partition end sector address.    */
    	uint32_t iDrive_fat_type[MUSB_UCFS_UNIT_COUNT];		/* Partition type                   */
} MGC_UcFsDevice;

/* 
 * Partition table descriptions. 
 */
typedef struct
{
    uint8_t	boot;		/* BootSignature */
    uint8_t	s_head;		/* StartHead */
    uint8_t	s_sec;		/* StartSector(Bit0-5) Bit6 and 7 are cylinder number */
    uint8_t	s_cyl;		/* StartCylinder Upper two bit of starting clyinder number are in Start Sector field */
    uint8_t	p_typ;		/* Partition Type */
    uint8_t	e_head;		/* EndHead */
    uint8_t	e_sec;		/* EndSector(Bit0-5) Bit6 and 7 are cylinder number*/
    uint8_t	e_cyl;		/* EndCylinder Upper two bit of ending clyinder number are in Start Sector field */
    uint32_t	r_sec;		/* Relativity sector */
    uint32_t	p_size;		/* Size of partition */
} MGC_MSD_PTABLE_ENTRY;

/* 
 * (Master) Boot Record structure 
 */
typedef struct
{
    MGC_MSD_PTABLE_ENTRY	ents[4];        /* Entry table */
    uint16_t					signature;      /* should be 0xAA55 */

} MGC_MSD_PTABLE_STRU;


/*
 * Definions for position value of Byte, short and int
 */
#define     POSITION_VALUE_8    (0x100)
#define     POSITION_VALUE_16   (uint32_t)(0x10000)
#define     POSITION_VALUE_24   (uint32_t)(0x1000000)

/*
 * FAT signature
 */


//#define BIG_ENDIAN
#ifdef BIG_ENDIAN
#define MSD_FAT_SIGNATURE        0x55AA  /* big enian */
#define SWOP(X) ((X) = (((X)<<8)+((X)>>8)))
#define SWAP4(X)  \
( (X) = ((X)<<24) + ((X)>>24) + (((X)>>8)&0x0000FF00) + (((X)<<8)&0x00FF0000) )
#else
#define MSD_FAT_SIGNATURE        0xAA55  /* little enian */
#define SWAP4(X) (X = X)
#define SWOP(X)  (X = X)
#endif

/**************************** FORWARDS ****************************/
//int MGC_UcFsDeviceStatus(uint8_t bUnit);
//int MGC_UcFsDeviceRead(uint8_t bUnit, uint32_t dwSector, void *pBuffer, uint32_t NumSectors);
//int MGC_UcFsDeviceWrite(uint8_t bUnit, uint32_t dwSector, void *pBuffer, uint32_t NumSectors, uint8_t RepeatSame);
//int MGC_UcFsDeviceIoCtl(uint8_t bUnit, int32_t dwCmd, int32_t dwAux, void *pBuffer);
//int MGC_UcFsDeviceReadBurst(uint8_t bUnit, uint32_t dwSector, uint32_t dwNumSectors, 
//			    void *pBuffer);
//int MGC_UcFsDeviceWriteBurst(uint8_t bUnit, uint32_t dwSector, uint32_t dwNumSectors, 
//			     const void *pBuffer);
//int MGC_UcFsDeviceInit(uint8_t bUnit);
int MGC_UcFsDeviceStatus(U8 bUnit);
int MGC_UcFsDeviceRead(U8 bUnit, U32 dwSector, void *pBuffer, U32 NumSectors);
int MGC_UcFsDeviceWrite(U8 bUnit, U32 dwSector, const void *pBuffer, U32 NumSectors, U8 RepeatSame);
int MGC_UcFsDeviceIoCtl(U8 bUnit, I32 dwCmd, I32 dwAux, void *pBuffer);
int MGC_UcFsDeviceReadBurst(U8 bUnit, U32 dwSector, U32 dwNumSectors, 
			    void *pBuffer);
int MGC_UcFsDeviceWriteBurst(U8 bUnit, U32 dwSector, U32 dwNumSectors, 
			     const void *pBuffer);
int MGC_UcFsDeviceInit(U8 bUnit);

/**************************** GLOBALS *****************************/
static uint8_t MGC_bInited = FALSE;

/** uC/FS units */
static MGC_UcFsDevice MGC_aUcFsDevice[MUSB_UCFS_UNIT_COUNT];
static int NumUnits;
/*************************** FUNCTIONS ****************************/

static void MGC_UcFsInit()
{
	MGC_UcFsDevice* pUcFsDevice;
	uint8_t bIndex;
	OS_ERR err;

	MUSB_MemSet(MGC_aUcFsDevice, 0, sizeof(MGC_aUcFsDevice));
	for(bIndex = 0; bIndex < (uint8_t)MUSB_UCFS_UNIT_COUNT; bIndex++)
	{
		pUcFsDevice = &(MGC_aUcFsDevice[bIndex]);
		//OSSemCreate(&pUcFsDevice->stStartTransfer, NULL, 1, &err);
		pUcFsDevice->hStartTransfer = OSSemCreate(1);
		if(!pUcFsDevice->hStartTransfer)
		//if(OS_ERR_NONE != err)
		{
			/* TODO: back out */
			return;
		}
		//else
			//pUcFsDevice->hStartTransfer = &pUcFsDevice->stStartTransfer;
		
		pUcFsDevice->hTransferDone = OSSemCreate(0);
		if(!pUcFsDevice->hTransferDone)
		//OSSemCreate(&pUcFsDevice->stTransferDone, NULL, 0, &err);
		//if(OS_ERR_NONE != err)
		{
			/* TODO: back out */
			return;
		}
		//else
			//pUcFsDevice->hTransferDone = &pUcFsDevice->stTransferDone;
	}
	MGC_bInited = TRUE;
}

/* Implementation */
MUSB_HfiStatus 
MUSB_HfiAddDevice(MUSB_HfiVolumeHandle* phVolume,
		  const MUSB_HfiDeviceInfo* pInfo, 
		  MUSB_HfiDevice* pDevice)
{
	MGC_UcFsDevice* pUcFsDevice;
	uint8_t bIndex;

	MUSB_PRINTK("===MUSB_HfiAddDevice===\n");

	if(!MGC_bInited)
	{
		MGC_UcFsInit();
	}
	if(!MGC_bInited)
	{
		return MUSB_HFI_NO_MEMORY;
	}

	/* use first available unit */
	for(bIndex = 0; bIndex < (uint8_t)MUSB_UCFS_UNIT_COUNT; bIndex++)
	{
		pUcFsDevice = &(MGC_aUcFsDevice[bIndex]);
		if(!pUcFsDevice->pDevice)
		{
			MUSB_MemCopy(&(pUcFsDevice->DeviceInfo), pInfo, sizeof(MUSB_HfiDeviceInfo));
			pUcFsDevice->pDevice = pDevice;
			*phVolume = pUcFsDevice;
			pUcFsDevice->bMediumReady = (MUSB_HFI_MEDIA_UNKNOWN != pInfo->MediaType) ? TRUE : FALSE;
			MUSB_PRINTK("===MUSB_HfiAddDevice MUSB_HFI_SUCCESS===\n");	
			pUcFsDevice->bDeviceStatus = 0;
			return MUSB_HFI_SUCCESS;
		}
	}

	MUSB_PRINTK("===MUSB_HfiAddDevice MUSB_HFI_NO_MEMORY===\n");
	return MUSB_HFI_NO_MEMORY;
}

/* Implementation */
void 
MUSB_HfiMediumInserted(MUSB_HfiVolumeHandle 	 hVolume,
		       const MUSB_HfiMediumInfo* pMediumInfo)
{
	MGC_UcFsDevice* pUcFsDevice = (MGC_UcFsDevice*)hVolume;
	MUSB_DeviceDriver* pMsdDriver;

	MUSB_PRINTK("===MUSB_HfiMediumInserted===\n");
	
	pUcFsDevice->bMediumReady = TRUE;
	MUSB_MemCopy(&(pUcFsDevice->DeviceInfo.InitialMedium), pMediumInfo, sizeof(MUSB_HfiMediumInfo));
}

/* Implementation */
void MUSB_HfiMediumRemoved(MUSB_HfiVolumeHandle hVolume)
{
	MGC_UcFsDevice* pUcFsDevice = (MGC_UcFsDevice*)hVolume;

	MUSB_PRINTK("===MUSB_HfiMediumRemoved===\n");

	pUcFsDevice->bMediumReady = FALSE;
}

/* Implementation */
void MUSB_HfiDeviceRemoved(MUSB_HfiVolumeHandle hVolume)
{
	MGC_UcFsDevice* pUcFsDevice = (MGC_UcFsDevice*)hVolume;
	
	MUSB_PRINTK("===MUSB_HfiDeviceRemoved===\n");

	pUcFsDevice->bMediumReady = FALSE;
	pUcFsDevice->pDevice = NULL;		
}

/******************************************************************************
 * Function    : MUSB_HfiGetPartitionInfo()
 * Purpose     : Partition table interpretor
 *               Given a  device handle and the addresses of two tables,
 *               partition start and partition end, this routine interprets 
 *               the partion tables on the physical drive and fills in the 
 *               start and end blocks of each partition. Extended partitions are
 *               supported. If there are more than max partitions,
 *               only max will be returned.             
 * Parameters  : buf_P : Pointer to the MSD device buf.
 *               pstart    : Partition start sector string.
 *               pend      : Partition end sector string.
 *               fat_type  : pointer to the varible to hold pointer to list
 *                           of fat types. 
 * Called By   : MSD layer during the MSD Inquiry
 * Returns     : The number of partitions found on the disk 
 *               if the partition information is currect.
 *               zero if the any error in the partition information
 ******************************************************************************/
int MUSB_HfiGetPartitionInfo (int8_t *buf_P, uint32_t *pstart, uint32_t *pend, uint32_t *fat_type)
{	              
	MGC_MSD_PTABLE_STRU	*ppart;
	int8_t					workbuf[256];
	int8_t             			*pbuf;
	uint16_t					i;
	uint16_t					extflg;
	uint16_t					signature;
	uint16_t					numsec16;
	uint16_t					nparts;
	uint16_t					stemp;
	uint32_t					ltemp;
	uint32_t					ltemp2;
	uint32_t					partition_address;
	uint32_t					extpart_base_address;
	uint32_t					rsec;
	uint32_t					psize;
	uint32_t					numsec32;

	/* 
	* Initialize retrun value, extpart flag and partition address.
	*/
	nparts	= 0;
	extflg	= 0;
	partition_address = extpart_base_address = 0L;

	while (1)                 /* Partition record loop. */
	{
		/* 
		* Check number of maximum partitions. 
		*/
		if(nparts == MUSB_UCFS_UNIT_COUNT)
		{
			break;
		}

		/* 
		* Copy the table to a word aligned buffer so some compilers 
		* don't screw up. 
		*/
		pbuf = buf_P;
		/* 
		* The Partition info starts at 466 location buf_P[1be] 
		*/
		pbuf += 0x1be;
		/*
		* Copy the drive partition information
		* Here 66 is sizeof(PTABLE_STRU) but that value comes
		* as 68 which is wrong. Hence hardcoded here.
		*/
		MUSB_MemCopy(workbuf, pbuf, 66);

		ppart  = (MGC_MSD_PTABLE_STRU *) workbuf;  /* Move to PTABLE pointer */
		stemp  = ppart->signature;    /* Set signature. */

		/*
		* Swap the stemp value and assign to the signature
		*/
		signature = SWOP(stemp);

		/* 
		* Check signature. 
		*/
		if (signature != MSD_FAT_SIGNATURE)
		{
			//printk("signature is 0x%x\n", signature);
			break;
		}

		/* 
		* Check whether it has a good dos formatted floppy 
		*/
		do
		{ 
			//printk("*(buf_P) is 0x%x\n", *(buf_P));
			if ( (( *(buf_P) == (int8_t)0xE9 ) || ( *(buf_P) == (int8_t)0xEB))&&(!extflg) )
			{
				/* 
				* Partition will start from sec 0 in floppies 
				*/
#if 1//add for ipod 
				uint8_t fat_temp[4096];
				uint8_t status = 0; 
				unsigned int RsvdSecCnt=0;
				RsvdSecCnt= ((unsigned char *)buf_P)[14] + 256 * ((unsigned char *)buf_P)[15]; 
				//printk("pos 0 RsvdSecCnt is %d\n", RsvdSecCnt);
				status = MGC_UcFsDeviceRead(0, RsvdSecCnt, (int8_t  *)(&fat_temp), 1);
				
				if(status != 0)
				{
					//printk("pos 1\n");
					return(nparts);
				}
				if((fat_temp)[0]!=0xf8)
				{
					//printk("pos 2\n");
					break;
				}
#endif
				*(pstart + nparts) = 0; 
				numsec16 = (uint16_t)( (*(buf_P+19) << 8) + *(buf_P+20) );
				numsec32 = (uint32_t) ( (*(buf_P+32)) * POSITION_VALUE_24 +
					(*(buf_P+33)) * POSITION_VALUE_16 + 
				 	(*(buf_P+34)) * POSITION_VALUE_8 + 
				 	(*(buf_P+35)) );

				if (numsec16 != 0)
				{
					//printk("pos 3\n");
					*(pend + nparts) = numsec16;    /* for FAT12 and FAT16 */
					*(fat_type + nparts) = 0x06;/* Set the partition type FAT16. */
				}
				else
				{
					//printk("pos 4\n");
					*(pend + nparts) = numsec32;    /* for FAT32 */
					*(fat_type + nparts) = 0x01;/* Set the partition type FAT32. */
				}
				nparts =1;
				break;
			}
		}while(0);
		/* 	
		* Read through the partition table. Find the primary DOS partition.
		*/
		for (i = 0; i < 4; i++)
		{
			/*  
			* Partition Type values 
			*  0x01     12-bit FAT.
			*  0x04     16-bit FAT. Partition smaller than 32MB.
			*  0x06     16-bit FAT. Partition larger than or equal to 
			*           32MB.
			*  0x0E     Same as PART_DOS4_FAT(06h),
			*           but uses Logical Block Address Int 13h 
			*           extensions.
			*  0x0B     32-bit FAT. Partitions up to 2047GB.
			*  0x0C     Same as PART_DOS32(0Bh), 
			*           but uses Logical Block Address Int 13h 
			*           extensions.
			*/
			if ((ppart->ents[i].p_typ == 0x01) ||(ppart->ents[i].p_typ == 0x04) ||
				(ppart->ents[i].p_typ == 0x06) ||(ppart->ents[i].p_typ == 0x0E) ||
				(ppart->ents[i].p_typ == 0x0B) ||(ppart->ents[i].p_typ == 0x0C))
			{
				/* 
				* Set the partition type. 
				*/
				*(fat_type + nparts) = ppart->ents[i].p_typ;

				/* 
				* Get the relative start. 
				*/
				ltemp = (uint32_t)ppart->ents[i].r_sec;
				SWAP4(ltemp);
				ltemp += partition_address;
				/* 
				* Set the partition start sector. 
				*/
				*(pstart + nparts) = ltemp;

				/* 
				* Get the partition size. 
				*/
				rsec = (uint32_t)ppart->ents[i].r_sec;
				SWAP4(rsec);
				psize = (uint32_t)ppart->ents[i].p_size;
				SWAP4(psize);

				/* 
				* Set the partition end sector. 
				*/
				ltemp2 = (uint32_t) (rsec + psize - 1);
				ltemp2 += partition_address;
				*(pend + nparts) = ltemp2;

				/* 
				* Increment number of partitions.
				*/
				nparts++;
			}
		}

		/* 
		* Now see if it has an extended partion.
		*/
		for (i = 0; i < 4; i++)
		{
			/* 
			* Partition Type values 
			* 0x05     Extended MS-DOS Partition.
			* 0x0F     Same as PART_EXTENDED(05h),
			*          but uses Logical Block Address Int 13h 
			*          extensions.
			*/
			if ((ppart->ents[i].p_typ == 0x05) ||(ppart->ents[i].p_typ == 0x0F))
			{
				uint8_t status = 0;
				/* 
				* Get the address of the extended partition. 
				*/
				ltemp = (uint32_t)ppart->ents[i].r_sec;
				SWAP4(ltemp);
				/* 
				* Add extended partition base address. 
				*/
				ltemp += extpart_base_address;	

				/* 
				* Set the next partition address. 
				*/
				partition_address = ltemp;
				/* 
				* First extended partition? 
				*/
				if (extflg == 0)
				{
					/* 
					* Set extended partition base address. 
					*/
					extpart_base_address = ltemp;
					status = MGC_UcFsDeviceRead(0, extpart_base_address, buf_P, 1);
					if(status != 0)
					{
						//printk("get partirion err..............\n");
						return(nparts);
					}
					extflg = 1;
					break;
				}
				status = MGC_UcFsDeviceRead(0, partition_address, buf_P, 1);
				if(status != 0)
				{
					//printk("get partirion err..............\n");
					return(nparts);
				}		 
				break;
			}
		}

		/* No extended partitions, bail. */
		if (i == 4)
		{
			break;
		}
	}

	if(nparts == 0)
	{
		//printk("MGC_MSD_Get_Drive_partition_Info nparts == 0 \n");
		*(pstart + nparts) = 0;
		*(fat_type + nparts) = 0x01;
		nparts=1;
	}
	return (nparts);
}

//const char *MGC_UcFsGetDeviceName(uint8_t bUnit)
const char *MGC_UcFsGetDeviceName(U8 bUnit)
{
	return "usb";
}

static int MGC_UcFsAddDevice(void) 
{
	if (NumUnits >= MUSB_UCFS_UNIT_COUNT) 
	{
		return -1;
	}
	return NumUnits++;
}

static int MGC_UcFsGetNumUnits(void) 
{
	return NumUnits;
}

/**
 * @return 1 for media changed
 * @return 0 for success
 * @return <0 for failure
 */
//int MGC_UcFsDeviceStatus(uint8_t bUnit)
int MGC_UcFsDeviceStatus(U8 bUnit)
{
	MGC_UcFsDevice* pUcFsDevice = &(MGC_aUcFsDevice[bUnit]);
	uint8_t buf[8192];		//最大的sec 大小
	uint8_t status;
	
	/* report error if off-line for any reason */
	if(!pUcFsDevice->pDevice || !pUcFsDevice->bMediumReady)
	{
		MUSB_PRINTK("===MGC_UcFsDeviceStatus Error===\n");
		return -1;
	}

	/* report medium changes */
	if(pUcFsDevice->bMediumChanged)
	{
		pUcFsDevice->bMediumChanged = FALSE;
		MUSB_PRINTK("===MGC_UcFsDeviceStatus Changes===\n");
		return 1;
	}

	/* OK */
	MUSB_PRINTK("===MGC_UcFsDeviceStatus OK===\n");
		
	status = MGC_UcFsDeviceRead(0, 0, buf, 1);
	if(!status)
	{
		status = MUSB_HfiGetPartitionInfo((int8_t *)buf, (uint32_t *)&(pUcFsDevice->iDrive_start), 
			(uint32_t *)&(pUcFsDevice->iDrive_end), (uint32_t *)&(pUcFsDevice->iDrive_fat_type));
		if(!status)
			return -1;
		MUSB_PRINTK("MUSB_HfiGetPartitionInfo status is %d \n", status);
		MUSB_PRINTK("iDrive_start is 0x%x iDrive_end is 0x%x iDrive_fat_type is 0x%x \n", pUcFsDevice->iDrive_start[0], 
			pUcFsDevice->iDrive_end[0], pUcFsDevice->iDrive_fat_type[0]);
	}
	pUcFsDevice->bDeviceStatus = 1;
	return 0;
}

static void MGC_UcFsTransferComplete(MUSB_HfiVolumeHandle hVolume, 
				     uint16_t wActualBlocks)
{
	OS_ERR err;
	
	MGC_UcFsDevice* pUcFsDevice = (MGC_UcFsDevice*)hVolume;

	pUcFsDevice->wActualBlocks = wActualBlocks;
	//OSSemPost((OS_SEM *)pUcFsDevice->hTransferDone, OS_OPT_POST_1, &err);
	OSSemPost(pUcFsDevice->hTransferDone);
}

int MGC_UcFsDeviceReadSingle(uint8_t bUnit, uint32_t dwSector, void *pBuffer)
{
	MUSB_HfiStatus eStatus;
	MGC_UcFsDevice* pUcFsDevice = &(MGC_aUcFsDevice[bUnit]);
	MUSB_HfiDevice* pDevice = pUcFsDevice->pDevice;
	int iResult = -1;
	//OS_ERR err;
	//CPU_TS ts;
        INT8U err;

	if(pUcFsDevice->bDeviceStatus)
	{
		dwSector = dwSector + pUcFsDevice->iDrive_start[bUnit];
		if(dwSector > pUcFsDevice->iDrive_end[bUnit])
			return iResult;
	}
	
	if(pUcFsDevice->bMediumReady == FALSE)
		return iResult;

	/* lock out other transfers */
	//OSSemPend((OS_SEM *)pUcFsDevice->hStartTransfer, 0, OS_OPT_PEND_BLOCKING, &ts, &err);
	OSSemPend(pUcFsDevice->hStartTransfer, 0, &err);
	pUcFsDevice->wActualBlocks = 0;

	/* TODO: a sector is FS_SEC_SIZE */
	eStatus = pDevice->pfReadDevice(pDevice->pPrivateData, dwSector, 0,
			1, (uint8_t*)pBuffer, MGC_UcFsTransferComplete, FALSE);

	/* pend on semaphore for completion */
	//OSSemPend((OS_SEM *)pUcFsDevice->hTransferDone, READ_TIME, OS_OPT_PEND_BLOCKING, &ts, &err);
	OSSemPend(pUcFsDevice->hTransferDone, READ_TIME, &err);
	if((MUSB_HFI_SUCCESS == eStatus) && (OS_ERR_NONE == err) && (1 == pUcFsDevice->wActualBlocks))
	{
		iResult = 0;
	}

	/* allow another transfer */
	//OSSemPost((OS_SEM *)pUcFsDevice->hStartTransfer, OS_OPT_POST_1, &err);
	OSSemPost(pUcFsDevice->hStartTransfer);
	return iResult;
}

//int MGC_UcFsDeviceWriteSingle(uint8_t bUnit, uint32_t dwSector, void *pBuffer)
int MGC_UcFsDeviceWriteSingle(U8 bUnit, U32 dwSector, const void *pBuffer)
{
	//OS_ERR err;
	//CPU_TS ts;
        INT8U err;
	MUSB_HfiStatus eStatus;
	MGC_UcFsDevice* pUcFsDevice = &(MGC_aUcFsDevice[bUnit]);
	MUSB_HfiDevice* pDevice = pUcFsDevice->pDevice;
	int iResult = -1;

	if(pUcFsDevice->bDeviceStatus)
	{
		dwSector = dwSector + pUcFsDevice->iDrive_start[bUnit];
		if(dwSector > pUcFsDevice->iDrive_end[bUnit])
			return iResult;
	}
	
	if(pUcFsDevice->bMediumReady == FALSE)
		return iResult;

	/* lock out other transfers */
	//OSSemPend((OS_SEM *)pUcFsDevice->hStartTransfer, 0, OS_OPT_PEND_BLOCKING, &ts, &err);
	OSSemPend(pUcFsDevice->hStartTransfer, 0, &err);
	pUcFsDevice->wActualBlocks = 0;

	/* TODO: a sector is FS_SEC_SIZE */
	eStatus = pDevice->pfWriteDevice(pDevice->pPrivateData, dwSector, 0,
			1, (uint8_t*)pBuffer, FALSE, MGC_UcFsTransferComplete, FALSE);

	/* pend on semaphore for completion */
	//OSSemPend((OS_SEM *)pUcFsDevice->hTransferDone, WRITE_TIME, OS_OPT_PEND_BLOCKING, &ts, &err);
	OSSemPend(pUcFsDevice->hTransferDone, WRITE_TIME, &err);
	if((MUSB_HFI_SUCCESS == eStatus) && (OS_ERR_NONE == err) && (1 == pUcFsDevice->wActualBlocks))
	{
		iResult = 0;
	}

	/* allow another transfer */
	//OSSemPost((OS_SEM *)pUcFsDevice->hStartTransfer, OS_OPT_POST_1, &err);
	OSSemPost(pUcFsDevice->hStartTransfer);

	return iResult;
}

//int MGC_UcFsDeviceReadBurst(uint8_t bUnit, uint32_t dwSector, uint32_t dwNumSectors, 
//			    void *pBuffer)
int MGC_UcFsDeviceReadBurst(U8 bUnit, U32 dwSector, U32 dwNumSectors, 
				void *pBuffer)
{
	//OS_ERR err;
	//CPU_TS ts;
        INT8U err;
	MUSB_HfiStatus eStatus;
	MGC_UcFsDevice* pUcFsDevice = &(MGC_aUcFsDevice[bUnit]);
	MUSB_HfiDevice* pDevice = pUcFsDevice->pDevice;
	int iResult = -1;

	if(pUcFsDevice->bDeviceStatus)
	{
		dwSector = dwSector + pUcFsDevice->iDrive_start[bUnit];
		if(dwSector > pUcFsDevice->iDrive_end[bUnit])
			return iResult;
	}

	if(pUcFsDevice->bMediumReady == FALSE)
		return iResult;

	/* lock out other transfers */
	//OSSemPend((OS_SEM *)pUcFsDevice->hStartTransfer, 0, OS_OPT_PEND_BLOCKING, &ts, &err);
	OSSemPend(pUcFsDevice->hStartTransfer, 0, &err);
	pUcFsDevice->wActualBlocks = 0;

	/* TODO: a sector is FS_SEC_SIZE */
	eStatus = pDevice->pfReadDevice(pDevice->pPrivateData, dwSector, 0,
		dwNumSectors, (uint8_t*)pBuffer, MGC_UcFsTransferComplete, FALSE);

	/* pend on semaphore for completion */
	//OSSemPend((OS_SEM *)pUcFsDevice->hTransferDone, dwNumSectors*READ_TIME, OS_OPT_PEND_BLOCKING, &ts, &err);
	OSSemPend(pUcFsDevice->hTransferDone, dwNumSectors*READ_TIME, &err);
	if((MUSB_HFI_SUCCESS == eStatus) && (OS_ERR_NONE == err) && (dwNumSectors == pUcFsDevice->wActualBlocks))
	{
		iResult = 0;
	}
	
	/* allow another transfer */
	//OSSemPost((OS_SEM *)pUcFsDevice->hStartTransfer, OS_OPT_POST_1, &err);
	OSSemPost(pUcFsDevice->hStartTransfer);

	return iResult;
}

//int MGC_UcFsDeviceWriteBurst(uint8_t bUnit, uint32_t dwSector, uint32_t dwNumSectors, 
//			     const void *pBuffer)
int MGC_UcFsDeviceWriteBurst(U8 bUnit, U32 dwSector, U32 dwNumSectors, 
				 const void *pBuffer)
{
	//OS_ERR err;
	//CPU_TS ts;
        INT8U err;
	MUSB_HfiStatus eStatus;
	MGC_UcFsDevice* pUcFsDevice = &(MGC_aUcFsDevice[bUnit]);
	MUSB_HfiDevice* pDevice = pUcFsDevice->pDevice;
	int iResult = -1;

	if(pUcFsDevice->bDeviceStatus)
	{
		dwSector = dwSector + pUcFsDevice->iDrive_start[bUnit];
		if(dwSector > pUcFsDevice->iDrive_end[bUnit])
			return iResult;
	}
	
	if(pUcFsDevice->bMediumReady == FALSE)
		return iResult;

	/* lock out other transfers */
	//OSSemPend((OS_SEM *)pUcFsDevice->hStartTransfer, 0, OS_OPT_PEND_BLOCKING, &ts, &err);
	OSSemPend(pUcFsDevice->hStartTransfer, 0, &err);
	pUcFsDevice->wActualBlocks = 0;

	/* TODO: a sector is FS_SEC_SIZE */
	eStatus = pDevice->pfWriteDevice(pDevice->pPrivateData, dwSector, 0,
		dwNumSectors, (uint8_t*)pBuffer, FALSE, MGC_UcFsTransferComplete, FALSE);

	/* pend on semaphore for completion */
	//OSSemPend((OS_SEM *)pUcFsDevice->hTransferDone, dwNumSectors*WRITE_TIME, OS_OPT_PEND_BLOCKING, &ts, &err);
	OSSemPend(pUcFsDevice->hTransferDone, dwNumSectors*WRITE_TIME, &err);
	if((MUSB_HFI_SUCCESS == eStatus) && (OS_ERR_NONE == err) && (dwNumSectors == pUcFsDevice->wActualBlocks))
	{
		iResult = 0;
	}

	/* allow another transfer */
	//OSSemPost((OS_SEM *)pUcFsDevice->hStartTransfer, OS_OPT_POST_1, &err);
	OSSemPost(pUcFsDevice->hStartTransfer);

	return iResult;
}

/**
 * @return 0 for success
 * @return <0 for failure
 */
//int MGC_UcFsDeviceRead(uint8_t bUnit, uint32_t dwSector, void *pBuffer, uint32_t NumSectors)
int MGC_UcFsDeviceRead(U8 bUnit, U32 dwSector, void *pBuffer, U32 NumSectors)
{	
	//MUSB_PRINTK("===MGC_UcFsDeviceRead NumSectors is %d===\n", NumSectors);
	
	if(NumSectors == 1)
		return MGC_UcFsDeviceReadSingle(bUnit, dwSector, pBuffer);
	else
		return MGC_UcFsDeviceReadBurst(bUnit, dwSector, NumSectors, pBuffer);
}

/**
 * @return 0 for success
 * @return <0 for failure
 */
//int MGC_UcFsDeviceWrite(uint8_t bUnit, uint32_t dwSector, void *pBuffer, uint32_t NumSectors, uint8_t RepeatSame)
int MGC_UcFsDeviceWrite(U8 bUnit, U32 dwSector, const void *pBuffer, U32 NumSectors, U8 RepeatSame)
{
	int i,ret;
	//MUSB_PRINTK("===MGC_UcFsDeviceRead NumSectors is %d===\n", NumSectors);
	
	if(NumSectors == 1)
	{
		return MGC_UcFsDeviceWriteSingle(bUnit, dwSector, pBuffer);
	}
	else if(RepeatSame)
	{
		for(i=0; i < NumSectors; i++)
		{
			ret = MGC_UcFsDeviceWriteSingle(bUnit, dwSector+i, pBuffer);
			if(ret)
				break;
		}
		return ret;
	}
	else
	{
		return MGC_UcFsDeviceWriteBurst(bUnit, dwSector, NumSectors, pBuffer);
	}
}

//int MGC_UcFsDeviceIoCtl(uint8_t bUnit, int32_t dwCmd, int32_t dwAux, void *pBuffer)
int MGC_UcFsDeviceIoCtl(U8 bUnit, I32 dwCmd, I32 dwAux, void *pBuffer)
{
	FS_DEV_INFO *pInfo;
	MGC_UcFsDevice* pUcFsDevice = &(MGC_aUcFsDevice[bUnit]);

	/*
	FS_DEV_INFO is a simple structure defined in fs_dev.h that is filled with
	the geometrical values of the device.
	*/
	FS_USE_PARA(dwAux);
	/* This check makes sure, the device number is within valid range */
	switch (dwCmd)
	{
		case FS_CMD_GET_DEVINFO:
		if (pBuffer == 0) 
		{
			/* avoid writing into non existing buffers */
			return -1;
		}
		pInfo = (FS_DEV_INFO *)pBuffer; /* The parameter pBuffer contains the pointer to the structure */
		/*
		The number of hidden sectors is used the reserve a certain amount of sectors for
		operating system specific data and the partition table. If you do not need or
		have a partition table or special system boot code on the drive, the number 
		of hidden sectors is zero.
		*/
		pInfo->NumHeads        = 0;  /* Relevant only for mechanical drives   */
		pInfo->SectorsPerTrack = 2;  /* Relevant only for mechanical drives   */
		pInfo->BytesPerSector = 512;
		//pInfo->NumSectors      = pUcFsDevice->DeviceInfo.InitialMedium.dwBlockCountLo;	//手机当U  盘用的时候读取不到
		//MUSB_PRINTK("pInfo->NumSectors is %d\n", pInfo->NumSectors);
		/* Now all necessary information is complete. */
		break;
	}
	return 0;  /* Return with zero if no problems have occurred. */
}

//int MGC_UcFsDeviceInit(uint8_t bUnit)
int MGC_UcFsDeviceInit(U8 bUnit)
{
	if(!MGC_bInited)
	{
		MGC_UcFsInit();
	}
	if(!MGC_bInited)
	{
		return -1;
	}
	return 0;
}

/* This structure contains pointers to the functions that all drivers
   of the file system must provide. */
/* NOTE: An extern of this should be added to fs_dev.h */

const FS_DEVICE_TYPE FS_MUSB_driver = {
    MGC_UcFsGetDeviceName,                 	/* Name of the device   */
    MGC_UcFsAddDevice,			    	/* number of units */
    MGC_UcFsDeviceRead,    				/* Device read sector   */
    MGC_UcFsDeviceWrite,   				/* Device write sector  */
    MGC_UcFsDeviceIoCtl,    				/* IO control interface */
    MGC_UcFsDeviceInit,
    MGC_UcFsDeviceStatus,  				/* Device state         */
    MGC_UcFsGetNumUnits
};

