/*
**********************************************************************
*                          Micrium, Inc.
*                      949 Crestview Circle
*                     Weston,  FL 33327-1848
*
*                            uC/FS
*
*             (c) Copyright 2001 - 2006, Micrium, Inc.
*                      All rights reserved.
*
***********************************************************************

----------------------------------------------------------------------
----------------------------------------------------------------------
File        : FS_FAT_Read.c
Purpose     : FAT read routines
---------------------------END-OF-HEADER------------------------------
*/

/*********************************************************************
*
*             #include Section
*
**********************************************************************
*/

#include "FAT_Intern.h"

/*********************************************************************
*
*       Data structures
*
**********************************************************************
*/

#if FS_SUPPORT_BURST
typedef struct {
  U32  FirstSector;
  U32  NumSectors;
  FS_SB * pSBData;
  void  * pData;
} BURST_INFO_R;
#endif


/*********************************************************************
*
*       Static code
*
**********************************************************************
*/

/*********************************************************************
*
*       _ReadBurst
*
*/
#if FS_SUPPORT_BURST
static int _ReadBurst(BURST_INFO_R * pBurstInfo) {
  if (pBurstInfo->NumSectors) {
//printk("_ReadBurst NumSectors = 0x%x\r\n", pBurstInfo->NumSectors);  	
    if (FS_LB_ReadBurst(pBurstInfo->pSBData->pPart,
                        pBurstInfo->FirstSector,
                        pBurstInfo->NumSectors,
                        pBurstInfo->pData, FS_SECTOR_TYPE_DATA))
    {
      FS_DEBUG_ERROROUT("Burst read error");
      return 1;     /* read error */
    }
  }
  return 0;         /* No problem ! */
}
#endif

#if FS_SUPPORT_BURST
#define SUPPORT_TEMP_DATA_CACHE		1

#if SUPPORT_TEMP_DATA_CACHE
#define MAX_BUF_SECTORS		128
#pragma data_alignment=1024
__no_init static unsigned int lg_pTmpBuf[MAX_BUF_SECTORS*512/4];
#endif

#endif

/*********************************************************************
*
*       _ReadData
*
*/
static U32 _ReadData(U8 *pData, U32 NumBytesReq, FS_FILE * pFile, FS_SB * pSBData, FS_SB * pSBfat) {
  U32        NumBytesRead;
  U32        BytesPerCluster;
  FS_VOLUME *   pVolume;
  U32        NumBytesCluster;
  U32        LastByteInCluster;
  int           SectorOff;
  U32        SectorNo;
  FS_FILE_OBJ * pFileObj;
  FS_FAT_INFO * pFATInfo;
  char        ZeroCopyAllowed;

#if FS_SUPPORT_BURST
  BURST_INFO_R    BurstInfo;
#endif

#if SUPPORT_TEMP_DATA_CACHE	
	unsigned int bUseTmpBuf = 0;
	U8* pStartData = 0;
#endif


  /*
   * Init / Compute some values used thruout the routine
   */
  pFileObj        = pFile->pFileObj;
  pVolume         = pFileObj->pVolume;
  pFATInfo        = &pVolume->FSInfo.FATInfo;
  BytesPerCluster = pFATInfo->BytesPerCluster;
  NumBytesRead    = 0;
#if FS_SUPPORT_BURST
  BurstInfo.NumSectors  = 0;
  BurstInfo.FirstSector = 0xFFFFFFFF;
  BurstInfo.pSBData     = pSBData;
#endif


  //
  // Check if Zero copy is possible
  //
  ZeroCopyAllowed = 1;
#if FS_SUPPORT_CHECK_MEMORY
  {
    FS_MEMORY_IS_ACCESSIBLE_CALLBACK * pfMemoryIsAccessible;

    pfMemoryIsAccessible = pVolume->Partition.Device.Data.pfMemoryIsAccessible;
    if (pfMemoryIsAccessible) {
      if (pfMemoryIsAccessible(pData, NumBytesReq) == 0) {
        ZeroCopyAllowed = 0;
      }
    }
  }
#endif
  /*
   * Main loop
   * We determine the cluster (allocate as necessary using the FAT buffer)
   * and write data into the cluster
   */
  do {
    /*
     * Locate current cluster.
     */
    if (FS_FAT_GotoCluster(pFile, pSBfat)) {
      FS_DEBUG_ERROROUT("Too few cluster allocated to file");
      return NumBytesRead;           /* File truncated (to few clusters) */
    }

    LastByteInCluster = BytesPerCluster * (pFileObj->Data.Fat.CurClusterFile + 1);
    NumBytesCluster   = LastByteInCluster - pFile->FilePos;
    if (NumBytesCluster > NumBytesReq) {
      NumBytesCluster = NumBytesReq;
    }
    SectorOff = pFile->FilePos & (pFATInfo->BytesPerSec - 1);
    SectorNo  = FS_FAT_ClusterId2SectorNo(pFATInfo, pFileObj->Data.Fat.CurClusterAbs);
    SectorNo += (pFile->FilePos >> pFATInfo->ldBytesPerSector) & (pFATInfo->SecPerClus -1);
    /*
     * Read data from the cluster, iterating over sectors
     */
	do {
		int NumBytesSector;
		NumBytesSector = pFATInfo->BytesPerSec - SectorOff;
		if ((U32)NumBytesSector > NumBytesCluster) {
			NumBytesSector = NumBytesCluster;
		}
      //
      // Do we have to read one sector into intermediate buffer ?
      //
		if   ((ZeroCopyAllowed == 0)
#if FS_DRIVER_ALIGMENT > 1      // Not required, just to avoid warnings
			|| (((int)pData & (FS_DRIVER_ALIGMENT - 1)))
#endif
			|| (NumBytesSector != pFATInfo->BytesPerSec))
		{
	        //
	        // Safe, but slow: Read one sector using memory of a smart buffer and copy data to destination
	        //
#if SUPPORT_TEMP_DATA_CACHE		        
	        	if(((int)pData & (FS_DRIVER_ALIGMENT - 1)) && (NumBytesSector == pFATInfo->BytesPerSec))
	        	{
	        		if(!bUseTmpBuf)
	        		{
		        		bUseTmpBuf = 1;
					BurstInfo.pData       = lg_pTmpBuf;					
	        		}
				if(BurstInfo.NumSectors < MAX_BUF_SECTORS)
				{
					if(BurstInfo.NumSectors == 0)
					{
						BurstInfo.FirstSector = SectorNo;
						BurstInfo.NumSectors  = 1;
						pStartData = pData;
					}
					else
					{
						if (SectorNo != BurstInfo.FirstSector + BurstInfo.NumSectors) 
						{
							if (_ReadBurst(&BurstInfo)) {
								return NumBytesRead;
							}
							FS_MEMCPY(pStartData, lg_pTmpBuf, pFATInfo->BytesPerSec * BurstInfo.NumSectors); 
							
							BurstInfo.FirstSector = SectorNo;
							BurstInfo.NumSectors  = 1;
							pStartData       = pData;
						} 
						else 
						{
							BurstInfo.NumSectors++;		
						}
					}
				}
				else
				{
					if (_ReadBurst(&BurstInfo)) {				
						return NumBytesRead;
					}				
					FS_MEMCPY(pStartData, lg_pTmpBuf, pFATInfo->BytesPerSec * BurstInfo.NumSectors); 
					
					BurstInfo.FirstSector = SectorNo;
					BurstInfo.NumSectors  = 1;
					pStartData = pData;					
					
				}
	        	}
			else
#endif				
			{
#if SUPPORT_TEMP_DATA_CACHE			
				if(bUseTmpBuf && BurstInfo.NumSectors > 0)
				{
					if (_ReadBurst(&BurstInfo)) {
						return NumBytesRead;
					}
					FS_MEMCPY(pStartData, lg_pTmpBuf, pFATInfo->BytesPerSec * BurstInfo.NumSectors); 			
					BurstInfo.NumSectors = 0;					
				}
#endif
				FS__SB_SetSector(pSBData, SectorNo, FS_SB_TYPE_DATA);
				if (FS__SB_Read(pSBData)) {
					return NumBytesRead;
				}
				FS_MEMCPY(pData, pSBData->pBuffer + SectorOff, NumBytesSector); 
			}
		} else {
	        //
	        // Zero copy variant. Check if we need to read the previous burst data
	        //
		#if FS_SUPPORT_BURST == 0
			if (FS_LB_ReadPart(pSBData->pPart, SectorNo, pData, FS_SECTOR_TYPE_DATA)) {
				return NumBytesRead;
			}
	        #else
			if (SectorNo != BurstInfo.FirstSector + BurstInfo.NumSectors) {
				if (_ReadBurst(&BurstInfo)) {
					return NumBytesRead;
				}
				BurstInfo.FirstSector = SectorNo;
				BurstInfo.NumSectors  = 1;
				BurstInfo.pData       = pData;
			} else {
				BurstInfo.NumSectors++;
			}
	        #endif
		}
      //
      // Update management info
      //
		pData           += NumBytesSector;
		NumBytesCluster -= NumBytesSector;
		NumBytesReq     -= NumBytesSector;
		NumBytesRead    += NumBytesSector;
		pFile->FilePos  += NumBytesSector;
		SectorNo++;
		SectorOff = 0;                /* Next sector will be written from start */
	} while (NumBytesCluster);
  } while (NumBytesReq);
#if FS_SUPPORT_BURST

#if SUPPORT_TEMP_DATA_CACHE
	if(bUseTmpBuf)
	{
		if (_ReadBurst(&BurstInfo)) {
			return NumBytesRead;
		}
		FS_MEMCPY(pStartData, lg_pTmpBuf, pFATInfo->BytesPerSec * BurstInfo.NumSectors); 	
		BurstInfo.NumSectors = 0;		
	}
	else
#endif		
	{
		  if (_ReadBurst(&BurstInfo)) {
		    FS_DEBUG_ERROROUT("Burst read error");
		    NumBytesRead = 0;               /* We do not know how many bytes have been read o.k., so reporting 0 is on the safe side */
		  }
	}
#endif
  return NumBytesRead;
}


/*********************************************************************
*
*       FS_FAT_Read
*
*  Description:
*    FS internal function. Read data from a file.
*
*  Parameters:
*    pData       - Pointer to a data buffer for storing data transferred
*                  from file.
*    Size        - Size of an element to be transferred from file to data
*                  buffer
*    N           - Number of elements to be transferred from the file.
*    pFile       - Pointer to a FS_FILE data structure.
*
*  Return value:
*    Number of elements read.
*/

U32 FS_FAT_Read(FS_FILE *pFile, void * pData, U32 NumBytesReq) {
  U32           NumBytesRead;
  FS_SB         sbData;          /* Sector buffer for Data */
  FS_SB         sbfat;           /* Sector buffer for FAT handling */
  FS_FILE_OBJ * pFileObj;
  FS_VOLUME   * pVolume;


  pFileObj = pFile->pFileObj;
  pVolume  = pFileObj->pVolume;
  /*
   * Check if file status is O.K..
   * If not, return.
   */
  if (pFile->Error) {
    return 0;                 /* Error */
  }

  if (pFile->FilePos >= pFileObj->Size) {
    pFile->Error = FS_ERR_EOF;
    return 0;

  }

  /*
   * Make sure we do not try to read beyond the end of the file
   */
  {
    U32 NumBytesAvail;
    NumBytesAvail = pFileObj->Size - pFile->FilePos;
    if (NumBytesReq > NumBytesAvail) {
      NumBytesReq = NumBytesAvail;
    }
  }
  if (NumBytesReq == 0) {
    pFile->Error = FS_ERR_EOF;
    return 0;
  }

  if (pFileObj->FirstCluster == 0) {
    FS_DEBUG_ERROROUT("Can not read: No cluster in directory entry");
    return 0;
  }
  /*
   * Allocate sector buffers.
   */

  FS__SB_Create(&sbfat,  &pVolume->Partition);
  FS__SB_Create(&sbData, &pVolume->Partition);
  /*
   * Do the work in a static subroutine
   */
  NumBytesRead = _ReadData((U8 *)pData, NumBytesReq, pFile, &sbData, &sbfat);
  /*
   * If less bytes have been read than intended
   *   - Set error code in file structur (unless already set)
   *   - Invalidate the Current cluster Id to make sure we read allocation list from start next time we read
   */
  if (NumBytesRead != NumBytesReq) {
    if (pFile->Error == 0) {
      FS_DEBUG_ERROROUT("Read error");
      pFile->Error = FS_ERR_READERROR;
    }
  }
  /*
   * Cleanup
   */
  FS__SB_Delete(&sbfat);
  FS__SB_Delete(&sbData);
  return NumBytesRead;
}

/*************************** End of file ****************************/
