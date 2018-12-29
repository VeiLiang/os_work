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
File        : FS_FAT_Misc.c
Purpose     : File system's FAT File System Layer misc routines
---------------------------END-OF-HEADER------------------------------
*/

/*********************************************************************
*
*       #include Section
*
**********************************************************************
*/

#include "FAT_Intern.h"
//#include "DmpMemory.h"

/*********************************************************************
*
*       defines, configurable
*
**********************************************************************
*/

/*********************************************************************
*
*       defines, non configurable
*
**********************************************************************
*/

/*********************************************************************
*
*       Public code
*
**********************************************************************
*/
const FAT_DIRENTRY_API * FAT_pDirEntryAPI = &FAT_SFN_API;
/*********************************************************************
*
*       Static code
*
**********************************************************************
*/

/*********************************************************************
*
*       _ClusterId2FATOff
*
*  Function description
*
*/
static U32 _ClusterId2FATOff(U8 FATType, U32 ClusterId) {
  switch (FATType) {
  case FS_FAT_TYPE_FAT12:
    return ClusterId + (ClusterId >> 1);
  case FS_FAT_TYPE_FAT16:
    return ClusterId << 1;
  }
  return ClusterId << 2;
}


/*********************************************************************
*
*       _SetFATSector
*
*  Function description
*    Assigns a FAT sector to a sector buffer
*/
static void _SetFATSector(FS_SB * pSB, U32 SectorNo, FS_FAT_INFO * pFATInfo) {
  FS_USE_PARA(pFATInfo);
  FS__SB_SetSector(pSB, SectorNo, FS_SB_TYPE_MANAGEMENT);
#if FS_MAINTAIN_FAT_COPY
  FS__SB_SetWriteCopyOff(pSB, pFATInfo->FATSize);
#endif
  
#if FS_PREFETCH_FAT
  pSB->SectorLimit = pFATInfo->RsvdSecCnt + pFATInfo->FATSize;
#endif
}

#if FS_FAT_USE_CLUSTER_CACHE
extern void FS_FAT_ClusterCacheFree (FS_FAT_INFO * pFATInfo, U32 Cluster);
extern void FS_FAT_ClusterCacheInitialize (FS_FAT_INFO * pFATInfo);
#endif
/*********************************************************************
*
*       _WriteFATEntry
*
*  Parameters
*    ClusterId    Id of the cluster to write. If it is invalid ( == 0), the routine does nothing. This is permitted !
*
*  Function description
*
*  Return value
*    0   O.K.
*    1   Error
*
*  Notes
*    None
*/
static char _WriteFATEntry(FS_VOLUME* pVolume, FS_SB * pSB, U32 ClusterId, U32 Value) {
	U32 SectorNo;
	U32 Off;          /* Total offset in bytes */
	U16 SectorOff;    /* Offset within the sector */
	U8 * pData;
	FS_FAT_INFO* pFATInfo;
	U32 ValueOld;     /* Previous value of this FATEntry */

	if (ClusterId == 0) {
		return 1;               /* Invalid cluster Id, ignore the write */
	}
	pFATInfo = &pVolume->FSInfo.FATInfo;

	Off = _ClusterId2FATOff(pFATInfo->FATType, ClusterId);
	if ((Off >> pFATInfo->ldBytesPerSector) >= pFATInfo->FATSize) {
		FS_DEBUG_ERROROUT("_WriteFATEntry is trying to write an entry beyond the FAT.");
		return 1;
	}
	SectorNo = pFATInfo->RsvdSecCnt + (Off >> pFATInfo->ldBytesPerSector);
	_SetFATSector(pSB, SectorNo, pFATInfo);
	FS__SB_Read(pSB);
	if (pSB->HasError) {
		return 1;            /* Read failed */
	}
	SectorOff = (U16)(Off & (pFATInfo->BytesPerSec - 1));
	pData = pSB->pBuffer + SectorOff;
	/*
	* Tell lower layer driver about the unused cluster
	*/
#if FS_SUPPORT_FREE_SECTOR || FS_SUPPORT_CACHE
	if (Value == 0) {
		U32 Sector;
		int    NumSectors;
		FS_PARTITION * pPart;
		pPart      = &pVolume->Partition;
		Sector     = FS_FAT_ClusterId2SectorNo(pFATInfo, ClusterId);
		NumSectors = pFATInfo->SecPerClus;
		FS_LB_FreePartSectors(pPart, Sector, NumSectors);
	}
#endif

#if FS_FAT_USE_CLUSTER_CACHE
	if(Value == 0)
	{
		// 
		
		FS_FAT_ClusterCacheFree (pFATInfo, ClusterId);
	}
#endif
	/*                                            */
	/* Update FATInfo, NextFreeCluster            */
	/*                                            */
	if (Value == 0) {
		if (ClusterId < pFATInfo->NextFreeCluster) {
			pFATInfo->NextFreeCluster = ClusterId;
		}
	} else {
		pFATInfo->NextFreeCluster = ClusterId + 1;
	}
	/*
	* Perform the actual write operation
	*/
	switch (pFATInfo->FATType) {
		case FS_FAT_TYPE_FAT32:
			ValueOld = FS_LoadU32LE(pData);
			FS_StoreU32LE(pData, Value);
			break;
		case FS_FAT_TYPE_FAT16:
			ValueOld = FS_LoadU16LE(pData);
			FS_StoreU16LE(pData, (U16)Value);
			break;
		default:
			FS__SB_MarkDirty(pSB);
			if (ClusterId & 1) {
				ValueOld = (*pData >> 4);
				*pData = (U8)((*pData & 0xf) | ((Value & 0xf) << 4));
				pData++;
				if (SectorOff == pFATInfo->BytesPerSec - 1) {    /* With Fat12, the next byte could be in the next sector */
					_SetFATSector(pSB, SectorNo + 1, pFATInfo);
					FS__SB_Read(pSB);
					pData  = pSB->pBuffer;
				}
				ValueOld |= ((unsigned)*pData << 4);
				*pData = (U8)(Value >> 4);
			} else {
				ValueOld = *pData;
				*pData = (U8)Value;
				pData++;
				if (SectorOff == pFATInfo->BytesPerSec - 1) {    /* With Fat12, the next byte could be in the next sector */
					_SetFATSector(pSB, SectorNo + 1, pFATInfo);
					FS__SB_Read(pSB);
					pData  = pSB->pBuffer;
				}
				ValueOld |= (*pData & 0xF) << 8;
				*pData = (U8)((*pData & 0xf0) | ((Value >> 8) & 0xf));
			}
	}
	FS__SB_MarkDirty(pSB);
	/*                                                                            */
	/* Update FATInfo, NumFreeClusters                                            */
	/*                                                                            */
	if (pFATInfo->NumFreeClusters != INVALID_NUM_FREE_CLUSTERS_VALUE) {
		if (ValueOld) {
			pFATInfo->NumFreeClusters++;
		}
		if (Value) {
			pFATInfo->NumFreeClusters--;
		}
	}

#if FS_FAT_TABLE_CACHE
	if(pFATInfo->FatTableCache)
	{
		pData = pFATInfo->FatTableCache + Off;
		switch (pFATInfo->FATType) {
			case FS_FAT_TYPE_FAT32:
				ValueOld = FS_LoadU32LE(pData);
				FS_StoreU32LE(pData, Value);
				break;
			case FS_FAT_TYPE_FAT16:
				ValueOld = FS_LoadU16LE(pData);
				FS_StoreU16LE(pData, (U16)Value);
				break;
			default:
				if (ClusterId & 1) {
					ValueOld = (*pData >> 4);
					*pData = (U8)((*pData & 0xf) | ((Value & 0xf) << 4));
					pData++;
					ValueOld |= ((unsigned)*pData << 4);
					*pData = (U8)(Value >> 4);
				} else {
					ValueOld = *pData;
					*pData = (U8)Value;
					pData++;
					ValueOld |= (*pData & 0xF) << 8;
					*pData = (U8)((*pData & 0xf0) | ((Value >> 8) & 0xf));
				}
		}
	}
#endif

	return 0;                   /* O.K. */
}

#if FS_SUPPORT_LFN_INFO

/*********************************************************************
*
*       _CopyString
*
*/
static int _CopyString(char * pDest,  const void * p, int MaxLen) {
  int i;
  const U8 * pSrc;
  U16 nSrc;
  U16 *pDst;

  pSrc = (const U8 *)p;
  pDst = (U16 *)pDest;
  for (i = 0; i < MaxLen; i++)
  {
    nSrc = *pSrc++;
    nSrc |= ((U16)(*pSrc++)<<8);
    if(nSrc == 0xffff)
    {
      break;
    }
    *pDst++ = nSrc;
  }
  return i * 2;
}

/*********************************************************************
*
*       _CopyLongName
*
*/
static int _CopyLongName(char * pDest, const U8 * pSrc) {
  int r;
  r = 0;
  r += _CopyString (pDest,  pSrc, 5);
  r += _CopyString (pDest + r, (pSrc + 13), 6);
  r += _CopyString (pDest + r, (pSrc + 27), 2);
  return r;
}

/*********************************************************************
*
*       _CalcNumChar
*
*/
static U8 _CalcNumChar(const U8 * pSrc, int MaxLen) {
  U8 i;
  for (i = 0; i < MaxLen; i++, pSrc += 2) {
    if (*pSrc == 0xff) {
      break;
    }
  }
  return i;
}

/*********************************************************************
*
*       _GetNumCharInEntry
*
*/
static int _GetNumCharInEntry(const U8 * pEntry) {
  int r;
  r = 0;
  r += _CalcNumChar(pEntry,      5);
  r += _CalcNumChar(pEntry + 13, 6);
  r += _CalcNumChar(pEntry + 27, 2);
  return r;
}

/*********************************************************************
*
*       _CompareShortDirEntry
*
*  Return value
*    0        Equal
*    1        Not equal
*/
static U8 _CalcCheckSum(FS_83NAME * pShortName) {
  const char * sShortName;
  U8 Sum;
  int i;

  sShortName = &pShortName->ac[0];
  Sum = 0;
  for (i = 0; i < 11; i++) {
    if (Sum & 1) {
      Sum = (Sum >> 1) | 0x80;
    } else {
      Sum >>= 1;
    }
    Sum += *sShortName++;
  }
  return Sum;
}

/*********************************************************************
*
*       _SetDirPosIndex
*
*  Description:
*    Increments/Decrements the position of DirPos
*/
static void _SetDirPosIndex(FS_DIR_POS * pDirPos, int Pos) {
  pDirPos->DirEntryIndex += Pos;
}

#endif

/*********************************************************************
*
*       _SFN_ReadDirEntryInfo
*
*  Description:
*    Search for short directory entry and store the information
*    in the DirEntryInfo structure.
*
*  Return value:
*    ==-1     - Error, invlaid directory or no more entries available
*    == 0     - Success.
*
*/
static int _SFN_ReadDirEntryInfo(FS__DIR * pDir, FS_DIRENTRY_INFO * pDirEntryInfo, FS_SB * pSB) {
  FS_FAT_DENTRY  * pDirEntry;
  U32              DirIndex;
  FS_FAT_INFO    * pFATInfo;
  FS_VOLUME      * pVolume;
  int              r;

#if FS_SUPPORT_LFN_INFO
  int NumEntries;
  int Index;
  U8 CheckSum;
  U8 CalcCheckSum;
  char  IsDifferent;
  int    NumBytesCopied;
  int    DirEntryLen;
  U8     UseShortName;
  char * p;
#endif

  pVolume    = pDir->pVolume;
  pFATInfo   = &pVolume->FSInfo.FATInfo;
  DirIndex   = pDir->DirEntryIndex;
  r          = -1;
  if (DirIndex == 0) {
    FS_FAT_InitDirEntryScan(pFATInfo, &pDir->DirPos, pDir->FirstCluster);
  }
  do {
    pDirEntry = FS_FAT_GetDirEntry(pVolume, pSB, &pDir->DirPos);
    FS_FAT_IncDirPos(&pDir->DirPos);
    if (pDirEntry == 0) {
      break;
    }
    if (pDirEntry->data[0] == 0x00) {         /* Last entry found ? */
      break;
    }
    if (pDirEntry->data[0] != (U8)0xE5) { /* not a deleted file */
      U8 Attrib;
      Attrib = pDirEntry->data[DIR_ENTRY_OFF_ATTRIBUTES];
      if ((Attrib != (FS_FAT_ATTR_LONGNAME)) && ((Attrib & FS_FAT_ATTR_VOLUME_ID) != FS_FAT_ATTR_VOLUME_ID)) { /* Also not a long entry nor a volume id, so it is a valid entry */
        FS_FAT_CopyShortName(pDirEntryInfo->sFileName, (const char*)&pDirEntry->data[0], pDirEntryInfo->SizeofFileName);
        FS_FAT_CopyDirEntryInfo(pDirEntry, pDirEntryInfo);
        pDir->DirEntryIndex++;
#if FS_SUPPORT_LFN_INFO
		pDirEntryInfo->sFileName[FS_LFN_INFO_OFFSET] = 0;
		pDirEntryInfo->sFileName[FS_LFN_INFO_OFFSET+1] = 0;
        _SetDirPosIndex(&pDir->DirPos, -1);
        pDirEntry = FS_FAT_GetDirEntry(pVolume, pSB, &pDir->DirPos);
        if(pDirEntry != 0)
		{
			CalcCheckSum = _CalcCheckSum((FS_83NAME *)&pDirEntry->data[0]);
			_SetDirPosIndex(&pDir->DirPos, -1);
			pDirEntry = FS_FAT_GetDirEntry(pVolume, pSB, &pDir->DirPos);
			if(pDirEntry != 0)
			{
				Attrib = pDirEntry->data[DIR_ENTRY_OFF_ATTRIBUTES];

//				printk("_SFN_ReadDirEntryInfo  Attrib = 0x%0x    FS_FAT_ATTR_LONGNAME = 0x%0x \n",Attrib,FS_FAT_ATTR_LONGNAME);
				if(Attrib == (FS_FAT_ATTR_LONGNAME))
				{
					CheckSum = pDirEntry->data[13];
					NumEntries = 1;
					IsDifferent  = 0;
					while((pDirEntry->data[0]&0x40)!=0x40)
					{
						_SetDirPosIndex(&pDir->DirPos, -1);
						pDirEntry = FS_FAT_GetDirEntry(pVolume, pSB, &pDir->DirPos);
						if(pDirEntry == 0)
						{
							IsDifferent = 1;
							break;
						}
						Attrib = pDirEntry->data[DIR_ENTRY_OFF_ATTRIBUTES];
						if(Attrib != (FS_FAT_ATTR_LONGNAME))
						{
							IsDifferent = 1;
							break;
						}
						if(pDirEntry->data[13] != CheckSum)
						{
							IsDifferent = 1;
						}
						NumEntries++;
					}
					_SetDirPosIndex(&pDir->DirPos, NumEntries-1);
					if((IsDifferent==0) && (CalcCheckSum == CheckSum))
					{
						_SetDirPosIndex(&pDir->DirPos, 1);
						Index          = NumEntries - 1;
						NumBytesCopied = 0;
						DirEntryLen    = 0;
						UseShortName   = 0;
						p              = pDirEntryInfo->sFileName + FS_LFN_INFO_OFFSET;
						if (p) {
						  do {
							_SetDirPosIndex(&pDir->DirPos, -1);
							pDirEntry = FS_FAT_GetDirEntry(pVolume, pSB, &pDir->DirPos);
							/* Check if we exceed the maximum of DirName size */
							if ((DirEntryLen + _GetNumCharInEntry(&pDirEntry->data[1])) > (pDirEntryInfo->SizeofFileName - 1)) {
							  /* We cannot process this long file name */
							  _SetDirPosIndex(&pDir->DirPos, -Index);
							  UseShortName = 1;
							  break;
							}
							NumBytesCopied = _CopyLongName(p, &pDirEntry->data[1]);

//							printk("_CopyLongName NumBytesCopied = %d \n", NumBytesCopied);
							
							p             += NumBytesCopied;
							DirEntryLen   += NumBytesCopied;
						  } while (Index--);
						}
						_SetDirPosIndex(&pDir->DirPos, NumEntries);
						pDirEntry = FS_FAT_GetDirEntry(pVolume, pSB, &pDir->DirPos);
						if (UseShortName) {
						  FS_FAT_CopyShortName(pDirEntryInfo->sFileName, (const char*)&pDirEntry->data[0], pDirEntryInfo->SizeofFileName);
						} else {
						  if (p) {
							*p = 0;
							*(p+1) = 0;
						  }
						}
						_SetDirPosIndex(&pDir->DirPos, -1);
					}
				}
			}
			_SetDirPosIndex(&pDir->DirPos, 1);
        }
        _SetDirPosIndex(&pDir->DirPos, 1);
#endif
        r = 0;
        break;
      }
    }
  } while (1);
  return r;
}

/*********************************************************************
*
*       _SFN_FindDirEntry
*
*  Description:
*    Tries to locate the directory entry in the specified directory.
*    The short name space is searched first;
*    if LFN support is activated, the long name space is search
*    if no short name match was found.
*
*  Parameters:
*    pVolume     - Volume information
*    pEntryName  - Directory entry name
*
*  Return value:
*    != NULL     - pointer to directory entry (in the smart buffer)
*    NULL        - Entry not found
*/
static FS_FAT_DENTRY * _SFN_FindDirEntry(FS_VOLUME * pVolume, FS_SB * pSB, const char *pEntryName, int Len, U32 DirStart, U8 AttrRequired, int * pLongDirEntryIndex) {
  FS_USE_PARA(pLongDirEntryIndex);
  return FS_FAT_FindDirEntryShort(pVolume, pSB, pEntryName, Len, DirStart, AttrRequired);
}



/*********************************************************************
*
*       _SFN_CreateDirEntry
*
*/
static FS_FAT_DENTRY * _SFN_CreateDirEntry(FS_VOLUME  * pVolume, FS_SB      * pSB, const char * pFileName, U32 DirStart, U32 ClusterId, U8 Attributes, U32 Size, U16 Time, U16 Date) {
  FS_FAT_DENTRY * pDirEntry;
  FS_83NAME   FATEntryName;

  pDirEntry = NULL;
  if (FS_FAT_Make83Name(&FATEntryName, pFileName, 0) == 0) {
    pDirEntry = FS_FAT_FindEmptyDirEntry(pVolume, pSB, DirStart);
    if (pDirEntry) {
      FS_FAT_WriteDirEntry83(pDirEntry, &FATEntryName, ClusterId, Attributes, Size, Time, Date);
      FS__SB_MarkDirty(pSB);
    }
  } else {
    FS_DEBUG_ERROROUT("File or directory name is not a legal 8.3 name: Either too long or illegal characters.");
  }
  return pDirEntry;
}



/*********************************************************************
*
*       _CheckBPB
*
*  Description:
*    Checks the Bios-Parameter-Block (BPB) of the volume
*    and initialize the FS specific information in the volume structure.
*
*  Parameters:
*    pVolume     - Pointer to volume structure.
*    pBuffer     - Pointer to buffer with read BPB.
*
*  Return value:
*    1           - OK     - BPB contains valid information.
*    0           - Error- - BPB contains invalid information.
*/
static int _CheckBPB(FS_VOLUME * pVolume, U8 * pBuffer) {
  FS_FAT_INFO * pFATInfo;
  U16           BytesPerSector;
  U32           FirstDataSector;
  U32           NumSectors;
  U32           NumClusters;
  U32           FATSize;
  U32           FirstSectorAfterFAT;
  U8            SectorsPerCluster;
  U16           Signature;
  U8            FATType;
  int           i;
  U8            NumFats;

  //
  // Check if this a valid BPB
  //
  Signature     = FS_LoadU16LE(&pBuffer[BPB_OFF_SIGNATURE]);
  if (Signature != 0xaa55) {
    FS_DEBUG_WARN("BPB signature invalid, volume not formatted.\n");
    return 0;                         /* Error, not a valid BPB */
  }
  BytesPerSector          = FS_LoadU16LE(&pBuffer[BPB_OFF_BYTES_PER_SECTOR]);  /* _512_,1024,2048,4096           */
  if ((BytesPerSector & 0xFE00) == 0) {  /* must be divisible by 512 */
    FS_DEBUG_ERROROUT("BPB error: BytesPerSector not valid");
    return 0;                         /* Error, not a valid BPB */
  }
  // 20180921 发现某些品牌卡(SanDisk)只有一个FAT
  NumFats = pBuffer[BPB_OFF_NUM_FATS];
  //if (pBuffer[BPB_OFF_NUM_FATS] != 2) {               /* Verify that NumFATs == 2 */
  if (NumFats != 2 && NumFats != 1) {               /* Verify that NumFATs == 1,2 */
    FS_DEBUG_ERROROUT("BPB error: Only 1,2 FATs supported");
    return 0;                         /* Error, not a valid BPB */
  }
  SectorsPerCluster = pBuffer[BPB_OFF_SECTOR_PER_CLUSTER];
  if (SectorsPerCluster == 0) {
    FS_DEBUG_ERROROUT("BPB error: SectorsPerCluster == 0");
    return 0;                         /* Error, not a valid BPB */
  }
  //
  // Analyze BPB and fill the FATInfo structure in pVolume.
  //
  pFATInfo = &pVolume->FSInfo.FATInfo;
  FS_MEMSET(pFATInfo, 0, sizeof(FS_FAT_INFO));
  NumSectors              = FS_LoadU16LE(&pBuffer[BPB_OFF_NUMSECTORS_16BIT]);                /* RSVD + FAT + ROOT + FATA (<64k) */
  if (NumSectors == 0) {
    NumSectors            = FS_LoadU32LE(&pBuffer[BPB_OFF_NUMSECTORS_32BIT]);                /* RSVD + FAT + ROOT + FATA (>=64k) */
  }

  FATSize                 = FS_LoadU16LE(&pBuffer[BPB_OFF_FATSIZE_16BIT]);                /* number of FAT sectors          */
  if (FATSize == 0) {
    U16 ExtFlags       = FS_LoadU16LE(&pBuffer[BPB_OFF_FAT32_EXTFLAGS]);                /* mirroring info                 */
    // Check FAT mirroring flags for FAT32 volumes
    if (ExtFlags & 0x008F) {
      FS_DEBUG_ERROROUT("BPB error: FAT32 feature \"FAT mirroring\" not supported.");
      return 0;                         /* Error, not a valid BPB */
    }
    //
    // Fat32
    //
    FATSize                 = FS_LoadU32LE(&pBuffer[BPB_OFF_FATSIZE_32BIT]);    /* number of FAT sectors          */
    pFATInfo->RootDirPos    = FS_LoadU32LE(&pBuffer[BPB_OFF_ROOTDIR_CLUSTER]);  /* root dir cluster for FAT32. Typically cluster 2. */
  }
  pFATInfo->NumSectors    = NumSectors;
  pFATInfo->FATSize       = FATSize;
  // 20180921 发现某些品牌卡(SanDisk)只有一个FAT
  //pFATInfo->NumFATs       = 2;
  pFATInfo->NumFATs       = NumFats;
  pFATInfo->SecPerClus    = SectorsPerCluster;
  pFATInfo->RsvdSecCnt    = FS_LoadU16LE(&pBuffer[14]);                /* 1 for FAT12 & FAT16            */
  pFATInfo->RootEntCnt    = FS_LoadU16LE(&pBuffer[17]);                /* number of root dir entries     */
  pFATInfo->BytesPerSec   = BytesPerSector;
  FirstSectorAfterFAT     = pFATInfo->RsvdSecCnt + pFATInfo->NumFATs * FATSize;

  FirstDataSector = FirstSectorAfterFAT + pFATInfo->RootEntCnt / (BytesPerSector / FS_FAT_DENTRY_SIZE);    /* Add number of sectors of root directory */
  pFATInfo->FirstDataSector = FirstDataSector;
  //
  // Compute the shift for bytes per sector
  //
  for (i = 9; i < 16; i++) {
    if ((1L << i) == BytesPerSector) {
      pFATInfo->ldBytesPerSector = (U16)i;
      break;
    }
  }
  //
  // Compute NumClusters
  //
  NumClusters = (NumSectors - FirstDataSector) / SectorsPerCluster;
  pFATInfo->NumClusters     = NumClusters;
  pFATInfo->BytesPerCluster = (U32)SectorsPerCluster * (U32)BytesPerSector;
  //
  // Determine the type of FAT (12/16/32), based on the number of clusters. (acc. MS spec)
  //
  FATType = FS_FAT_GetFATType(NumClusters);
  pFATInfo->FATType         = FATType;
  if(FATType != FS_FAT_TYPE_FAT32) {
    pFATInfo->RootDirPos = FirstSectorAfterFAT;     /* For FAT12 / FAT16 */
  }
  pFATInfo->NumFreeClusters = INVALID_NUM_FREE_CLUSTERS_VALUE;


  if(pVolume->SupportSpecialCache)
  {
#if FS_FAT_TABLE_CACHE
	  pFATInfo->FatTableCache = (U8*)Dmpmalloc(pFATInfo->FATSize * pFATInfo->BytesPerSec);
	  if(pFATInfo->FatTableCache)
	  {
	  	FS_LB_ReadBurst(&pVolume->Partition, pFATInfo->RsvdSecCnt , FATSize, pFATInfo->FatTableCache, FS_SECTOR_TYPE_DATA);
	  }
#endif
  }

#if FS_FAT_USE_CLUSTER_CACHE
  {
	  FS_FAT_ClusterCacheInitialize (pFATInfo);
  }

#endif


#if FS_FAT_USE_FSINFO_SECTOR
  if (FATType == FS_FAT_TYPE_FAT32) {
    U16  FSInfo_Sector;
    U32  FAT32Signature;
    U32  NumFreeClusters;

    FSInfo_Sector = FS_LoadU16LE(&pBuffer[BPB_OFF_FAT32_FSINFO_SECTOR]);
    pFATInfo->FSInfoSector = FSInfo_Sector;
    if (FS_LB_ReadPart(&pVolume->Partition, FSInfo_Sector, pBuffer, FS_SECTOR_TYPE_DATA) < 0) {
      return 0;               /* Error, FAT32 FSInfo sector could not be read */
    }
    /* Use FSInfo Sector if it has the correct signatures */
    FAT32Signature = FS_LoadU32LE(&pBuffer[FSINFO_OFF_SIGNATURE_1]);
    if (FAT32Signature == FSINFO_SIGNATURE_1) {
      FAT32Signature = FS_LoadU32LE(&pBuffer[FSINFO_OFF_SIGNATURE_2]);
      if (FAT32Signature == FSINFO_SIGNATURE_2) {
        FAT32Signature = FS_LoadU32LE(&pBuffer[FSINFO_OFF_SIGNATURE_3]);
        if (FAT32Signature == FSINFO_SIGNATURE_3) {
          pFATInfo->NextFreeCluster = FS_LoadU32LE(&pBuffer[FSINFO_OFF_NEXT_FREE_CLUSTER]);
          NumFreeClusters = FS_LoadU32LE(&pBuffer[FSINFO_OFF_FREE_CLUSTERS]);
          if (NumFreeClusters <= NumClusters) {
            pFATInfo->NumFreeClusters = NumFreeClusters;
            //
            //  To be on the safe side, we invalidate the NumFreeClusters value in
            //  the FS info sector.
            //  When the drive is not unmounted properly [removing the card without calling
            //  FS_Unmount()], the NumFreeClusters value in the FS Info sector is still the same,
            //  since it was not updated.
            //  When remounting the volume, the value will be interpreted as the correct NumFreeClusters value,
            //  even when there are more or less clusters available.
            //
            FS_StoreU32LE(&pBuffer[FSINFO_OFF_FREE_CLUSTERS], INVALID_NUM_FREE_CLUSTERS_VALUE);
            FS_LB_WritePart(&pVolume->Partition, FSInfo_Sector, pBuffer, FS_SECTOR_TYPE_DATA);
          }
        }
      }
    }
  }
#endif
  return 1;                   // OK. BPB contains valid data.
}

/*********************************************************************
*
*       Public code
*
**********************************************************************
*/

/*********************************************************************
*
*       FS_FAT_CheckBPB
*
*  Description:
*    Read Bios-Parameter-Block from a device and analyze this sector.
*
*  Parameters:
*    pVolume     - pointer to volume the BPB should be read.
*
*  Return value:
*    1           - BPB successfully read and contains valid information.
*    0           - An error has occured.
*/
int FS_FAT_CheckBPB(FS_VOLUME * pVolume) {
  int     r;
  U8 * pBuffer;
  U16  BytesPerSector;

  pBuffer = FS__AllocSectorBuffer();
  r = 0;
  if (pBuffer == NULL) {
    return 0;
  }
  BytesPerSector = FS_GetSectorSize(&pVolume->Partition.Device);
  /*
   * Check if the a sector fits into the sector buffer
   */
  if ((BytesPerSector > FS__MaxSectorSize) || (BytesPerSector == 0)) {
    FS_DEBUG_ERROROUT1("_ReadBPB: Invalid BytesPerSector value: ", BytesPerSector);
    FS__FreeSectorBuffer(pBuffer);
    return 0;
  }

  r = FS_LB_ReadPart(&pVolume->Partition, 0, (void*)pBuffer, FS_SECTOR_TYPE_DATA);
  if (r < 0) {
    FS__FreeSectorBuffer(pBuffer);
    return 0;
  }

  r = _CheckBPB(pVolume, pBuffer);
  FS__FreeSectorBuffer(pBuffer);
  return r;
}

/*********************************************************************
*
*       FS_FAT_GetFATType
*
*  Description:
*    Returns the FAT type.
*
*  Parameters:
*    Number of available clusters
*
*  Return value:
*    FAT type
*/
char FS_FAT_GetFATType(U32 NumClusters) {
  char FATType;
  if (NumClusters < 4085) {
    FATType = FS_FAT_TYPE_FAT12;
  } else if (NumClusters < 65525) {
    FATType = FS_FAT_TYPE_FAT16;  /* FAT16 */
  } else {
    FATType = FS_FAT_TYPE_FAT32;    /* FAT32 */
  }
  return FATType;
}

/*********************************************************************
*
*       FS_FAT_ClusterId2SectorNo
*
*  Function description
*    Converts the cluster Id into a sector number. The SectorNo can then
*    be used to read write the data.
*
*  Return value
*    SectorNo of the first sector of the specified cluster
*
*  Notes
*    The data area (start of cluster 2) starts right behind the root directory,
*    which in turn lies right behind the FAT(s) (usually 2).
*/
U32 FS_FAT_ClusterId2SectorNo(FS_FAT_INFO * pFATInfo, U32 ClusterId) {
  return pFATInfo->FirstDataSector + (ClusterId - FAT_FIRST_CLUSTER) * pFATInfo->SecPerClus;
}

/*********************************************************************
*
*       FS_FAT_ReadFATEntry
*
*  Function description
*
*  Return value
*    The value of the FAT entry as defined in the FAT spec.
*
*  Notes
*    (1) Pointer incrementing
*        The data pointer is pre-incremented before accesses for the
*        FAT32 entries. This is so because it does in fact allow the
*        compiler to generate better code.
*    (2) This cast is necessary, because the value is promoted to a
*        signed int. On 16bit target this value could be negative and
*        would result in using the wrong FAT entry value.
*
*/
U32 FS_FAT_ReadFATEntry(FS_VOLUME * pVolume, FS_SB * pSB, U32 ClusterId) {
	U32 FATEntry;
	U32 SectorNo;
	U32 Off;          /* Total offset in bytes */
	U16 SectorOff;    /* Offset within the sector */
	U8  FATType;
	U8 * pData;
	FS_FAT_INFO* pFATInfo;

	pFATInfo = &pVolume->FSInfo.FATInfo;
	FATType = pFATInfo->FATType;
	Off = _ClusterId2FATOff(FATType, ClusterId);
	if ((Off >> pFATInfo->ldBytesPerSector) >= pFATInfo->FATSize) {
		//FS_DEBUG_ERROROUT1("FS_FAT_ReadFATEntry is trying to read beyond the FAT. ClusterId = 0x", ClusterId);
		// 20160920 zhuoyonghong
		// 此处返回0表示该簇为可用簇，这样会导致该簇可能被分配。
		// return 0;
		// 应返回0xFFFFFFFF表示已占用
		return 0xFFFFFFFF;
	}
	
#if FS_FAT_TABLE_CACHE  
	if(pFATInfo->FatTableCache)
	{
		pData = pFATInfo->FatTableCache + Off;
		FATEntry = *pData++;
		FATEntry |= ((U32)*pData) << 8;                         /* Note 1, Note 3 */
		switch (FATType) 
		{
			case FS_FAT_TYPE_FAT32:      /* We have to read 2 more bytes */
				FATEntry |= ((U32)*++pData) << 16;          /* Note 1 */
				FATEntry |= ((U32)*++pData) << 24;          /* Note 1 */
				break;
				
			case FS_FAT_TYPE_FAT12:
				if (ClusterId & 1) 
				{
					FATEntry >>= 4;
				}
				else
				{
					FATEntry &= 0xfff;
				}
				break;
		}
	}
	else
	{
		SectorNo = pFATInfo->RsvdSecCnt + (Off >> pFATInfo->ldBytesPerSector);
		_SetFATSector(pSB, SectorNo, pFATInfo);
		FS__SB_Read(pSB);

		SectorOff = (U16)(Off & (pFATInfo->BytesPerSec - 1));
		pData = pSB->pBuffer + SectorOff;

		FATEntry = *pData++;
		if (SectorOff == pFATInfo->BytesPerSec - 1) {    /* With Fat12, the next byte could be in the next sector */
			_SetFATSector(pSB, SectorNo + 1, pFATInfo);
			FS__SB_Read(pSB);
			pData = pSB->pBuffer;
		}
		FATEntry |= ((U32)*pData) << 8;                         /* Note 1, Note 3 */
		switch (FATType) {
			case FS_FAT_TYPE_FAT32:      /* We have to read 2 more bytes */
				FATEntry |= ((U32)*++pData) << 16;          /* Note 1 */
				FATEntry |= ((U32)*++pData) << 24;          /* Note 1 */
				break;
			case FS_FAT_TYPE_FAT12:
				if (ClusterId & 1) {
					FATEntry >>= 4;
				} else {
					FATEntry &= 0xfff;
				}
		}
	}
#else
	{
		SectorNo = pFATInfo->RsvdSecCnt + (Off >> pFATInfo->ldBytesPerSector);
		_SetFATSector(pSB, SectorNo, pFATInfo);
		FS__SB_Read(pSB);

		SectorOff = (U16)(Off & (pFATInfo->BytesPerSec - 1));
		pData = pSB->pBuffer + SectorOff;

		FATEntry = *pData++;
		if (SectorOff == pFATInfo->BytesPerSec - 1) {    /* With Fat12, the next byte could be in the next sector */
			_SetFATSector(pSB, SectorNo + 1, pFATInfo);
			FS__SB_Read(pSB);
			pData = pSB->pBuffer;
		}
		FATEntry |= ((U32)*pData) << 8;                         /* Note 1, Note 3 */
		switch (FATType) {
			case FS_FAT_TYPE_FAT32:      /* We have to read 2 more bytes */
				FATEntry |= ((U32)*++pData) << 16;          /* Note 1 */
				FATEntry |= ((U32)*++pData) << 24;          /* Note 1 */
				break;
			case FS_FAT_TYPE_FAT12:
				if (ClusterId & 1) {
					FATEntry >>= 4;
				} else {
					FATEntry &= 0xfff;
				}
		}

	}
#endif

	return FATEntry;
}

#if FS_FAT_USE_CLUSTER_CACHE
extern U32 FS_FAT_ClusterCacheAllocate (FS_FAT_INFO * pFATInfo);
extern int FS_FAT_ClusterCacheIsFullCoverage (FS_FAT_INFO * pFATInfo, U32 *pLowLimit, U32 *pHighLimit);
#endif
/*********************************************************************
*
*       FS_FAT_FindFreeCluster
*
*  Function description
*    Finds the first available sector in the FAT.
*    Search starts at the specified cluster number, which makes it
*    possible to allocate consecutive sectors (if available)
*
*  Parameters
*    FirstCluster         Index of the first free cluster to look at.
*                         Can be 0 or out of range, in which case the first cluster assumed to be free is used.
*
*  Return value
*    ClusterId   if free cluster has been found
*    0           if no free cluster is available
*
*  Notes
*/
U32 FS_FAT_FindFreeCluster(FS_VOLUME * pVolume, FS_SB * pSB, U32 FirstCluster) {
  U32 i;
  U32 LastCluster;
  FS_FAT_INFO* pFATInfo;

  pFATInfo = &pVolume->FSInfo.FATInfo;
  LastCluster = pFATInfo->NumClusters + FAT_FIRST_CLUSTER - 1;    /* Cluster id of first cluster is 2 */

#if FS_FAT_USE_CLUSTER_CACHE
  {
	  U32 LowLimit, HighLimit;
	  int ret;
	  i = FS_FAT_ClusterCacheAllocate(pFATInfo);
	  if(i != 0)
		  return i;	// We found a free cluster
	  // 检查是否已全覆盖
	  ret = FS_FAT_ClusterCacheIsFullCoverage (pFATInfo, &LowLimit, &HighLimit);
	  if(ret == 1)
	  {
		  // 已全覆盖, 无需后续的搜索
		  return 0;                /* Error, no free cluster */
	  }
	  else if(ret == 0)
	  {
		  /*
		  if(HighLimit < LastCluster)
			  FirstCluster = HighLimit + 1;
		  else if(LowLimit > FAT_FIRST_CLUSTER)
			  FirstCluster = FAT_FIRST_CLUSTER;		// 从头开始搜索
		  */
	  }
  }
#endif
  /* Compute the first cluster to look at. If no valid cluster is specified, try the next one which should be free. */
  if ((FirstCluster < FAT_FIRST_CLUSTER) || (FirstCluster > LastCluster)) {
    FirstCluster = pFATInfo->NextFreeCluster;
  }
  if ((FirstCluster < FAT_FIRST_CLUSTER) || (FirstCluster > LastCluster)) {
    FirstCluster = FAT_FIRST_CLUSTER;
  }
  i = FirstCluster;
  /*
   * Search starting with the given cluster
   */
  do {
    if (FS_FAT_ReadFATEntry(pVolume, pSB, i) == 0) {
      return i;      /* We found a free cluster */
    }
  } while (++i <= LastCluster);
  /*
   * Continue search from first cluster on
   */
  for (i = FAT_FIRST_CLUSTER; i < FirstCluster; i++) {
    if (FS_FAT_ReadFATEntry(pVolume, pSB, i) == 0) {
      return i;      /* We found a free cluster */
    }
  }
  return 0;                /* Error, no free cluster */
}



/*********************************************************************
*
*       FS_FAT_MarkClusterEOC
*
*  Function description
*    Marks the given cluster as the last on ein the cluster chain
*
*  Return value
*    0   O.K.
*    1   Error
*/
char FS_FAT_MarkClusterEOC(FS_VOLUME * pVolume, FS_SB * pSB, U32 Cluster) {
  return _WriteFATEntry(pVolume, pSB, Cluster, 0xFFFFFFF);
}

/*********************************************************************
*
*       FS_FAT_LinkCluster
*
*  Function description
*
*  Notes
*/
void FS_FAT_LinkCluster(FS_VOLUME * pVolume, FS_SB * pSB, U32 LastCluster, U32 NewCluster) {
  if (LastCluster) {
    _WriteFATEntry(pVolume, pSB, LastCluster, NewCluster);
  }
  FS_FAT_MarkClusterEOC(pVolume, pSB, NewCluster);
}

/*********************************************************************
*
*       FS_FAT_FindLastCluster
*
*  Parameters
*    Cluster    Any cluster of the cluster chain. Must be valid (0 not allowed)
*
*  Function description
*    returns the last cluster in the cluster chain
*
*/
U32 FS_FAT_FindLastCluster(FS_VOLUME * pVolume, FS_SB * pSB, U32 Cluster, U32 * pNumClusters) {
  U32 NumClusters = 0;
  do {
    U32 NextCluster;
    NextCluster = FS_FAT_WalkCluster(pVolume, pSB, Cluster, 1);
    if (NextCluster == 0) {
      break;
    }
    NumClusters++;
    Cluster = NextCluster;
  } while (1);
  if (pNumClusters) {
    *pNumClusters = NumClusters;
  }
  return Cluster;
}


/*********************************************************************
*
*       FS_FAT_AllocCluster
*
*  Function description
*    Allocates a cluster and (optionally) links it to an existing cluster
*    chain, specified by the last cluster
*
*  Return value
*    0             On error (No new cluster)
*    ClusterId >0  If new cluster has been allocated (and possibly added to the chain)
*/
U32 FS_FAT_AllocCluster(FS_VOLUME * pVolume, FS_SB * pSB, U32 LastCluster) {
  U32 NewCluster;

  NewCluster = FS_FAT_FindFreeCluster(pVolume, pSB, LastCluster);
  if (NewCluster) {
    FS_FAT_LinkCluster(pVolume, pSB, LastCluster, NewCluster);
  }
  return NewCluster;
}

/*********************************************************************
*
*       FS_FAT_WalkCluster
*
*  Function description
*    Walks a chain of clusters and returns the cluster Id of the
*    cluster found
*
*  Return value
*    if (ClusterChain long enough) {
*      ClusterId of destination cluster
*    } else {
*      last cluster in chain
*    }
*
*  Parameters
*    pNumClusters      IN:  Number of clusters to walk
*                      OUT: Number of clusters which could not be walked
*  Notes
*/
U32 FS_FAT_WalkClusterEx(FS_VOLUME * pVolume, FS_SB * pSB, U32 Cluster, U32 * pNumClusters) {
  U32 NumClusters;

  NumClusters = *pNumClusters;
  for (; NumClusters; NumClusters--) {
    U32 FATEntry = FS_FAT_ReadFATEntry(pVolume, pSB, Cluster);
    if (pSB->HasError) {
      return 0;             /* Read error */
    }
    //
    // Check validity of FATEntry
    //
    if (FATEntry > pVolume->FSInfo.FATInfo.NumClusters + 1) {
      break;
    }
    Cluster = FATEntry;
  }
  *pNumClusters = NumClusters;
  return Cluster;
}

/*********************************************************************
*
*       FS_FAT_WalkCluster
*
*  Function description
*    Walks a chain of clusters and returns the cluster Id of the
*    cluster found
*
*  Return value
*    ClusterId   if cluster is in chain
*    0           if cluster is not valid
*
*  Notes
*/
U32 FS_FAT_WalkCluster(FS_VOLUME * pVolume, FS_SB * pSB, U32 Cluster, U32 NumClusters) {
  Cluster = FS_FAT_WalkClusterEx(pVolume, pSB, Cluster, &NumClusters);
  if (NumClusters) {
    Cluster = 0;     // Could not go all the way
  }
  return Cluster;
}

/*********************************************************************
*
*       _CalcLogClusterPos
*
*  Function description
*    Calculates the logical cluster index of a file
*
*  Return value
*    Logical cluster index
*
*/
static U32 _CalcLogClusterPos(FS_FILE * pFile) {
  FS_FAT_INFO* pFATInfo;

  pFATInfo = &pFile->pFileObj->pVolume->FSInfo.FATInfo;
  return pFile->FilePos / pFATInfo->BytesPerCluster;
}

/*********************************************************************
*
*       FS_FAT_WalkAdjFileClusters
*
*  Function description
*    Walks the adjacent clusters within a file
*
*  Return value
*    NumClustersToWalk
*/
static U32 _WalkAdjFileClusters(FS_FILE * pFile) {
  U32           CurClusterIndex;
  unsigned      i;
  U32           NumClustersToWalk;
  FS_FILE_OBJ * pFileObj;

  pFileObj = pFile->pFileObj;
  CurClusterIndex = _CalcLogClusterPos(pFile);
  if (CurClusterIndex < pFileObj->Data.Fat.CurClusterFile) {
    pFileObj->Data.Fat.CurClusterFile = 0xFFFFFFFF;   /// File Postition moved back ... Invalidate
  }
  if (pFileObj->Data.Fat.CurClusterFile == 0xFFFFFFFF) {
    pFileObj->Data.Fat.CurClusterFile = 0;
    pFileObj->Data.Fat.CurClusterAbs  = pFileObj->FirstCluster;
    pFileObj->Data.Fat.NumAdjClusters = 0;
  }

  //
  // Walk to the right cluster starting from the last known cluster postion.
  //
  NumClustersToWalk = CurClusterIndex - pFileObj->Data.Fat.CurClusterFile;
  if (NumClustersToWalk <= pFileObj->Data.Fat.NumAdjClusters) {
    i = NumClustersToWalk;
  } else {
    i = pFileObj->Data.Fat.NumAdjClusters;
  }
  pFileObj->Data.Fat.CurClusterAbs  += i;
  pFileObj->Data.Fat.CurClusterFile += i;
  pFileObj->Data.Fat.NumAdjClusters -= i;
  NumClustersToWalk -= i;
  return NumClustersToWalk;
}

/*********************************************************************
*
*       FS_FAT_GotoCluster
*
*  Purpose
*    Find the cluster which matches the current file position
*
*  Return value
*    Number of clusters missing
*
*  Notes
*    (1) Caller
*        This routine is called both when reading and writing a file.
*        When reading, a non-zero return value is an error meaning: File is too short
*        When writing, a non-zero return value means that add. clusters need to be allocated
*/
int FS_FAT_GotoCluster(FS_FILE * pFile, FS_SB * pSBfat) {
  U32        NumClustersToWalk;
  U32        NumClustersRem;
  U32        CurClusterAbs;
  FS_FILE_OBJ * pFileObj;
#if FS_SUPPORT_FAT_CHAIN
  U32        CurClusterIndex;
  U32        i;
#endif

  pFileObj  = pFile->pFileObj;


  if (pFileObj->FirstCluster == 0) {
    return 1 + _CalcLogClusterPos(pFile);
  }


  NumClustersToWalk = _WalkAdjFileClusters(pFile);
  if (NumClustersToWalk == 0) {
    return 0;                // Same cluster, we are done
  }

#if FS_SUPPORT_FAT_CHAIN
  if(pFile->AccessFlags & FS_FILE_ACCESS_FLAG_R)
  {
      CurClusterIndex = _CalcLogClusterPos(pFile);
	  for(i = pFile->FATChainCount; i > 0 ; i--)
	  {
		  if(CurClusterIndex >= pFile->FATChain[i-1].FileCluster)
			  break;
	  }
	  i--;
	  CurClusterAbs = pFile->FATChain[i].ClusterAbs + CurClusterIndex - pFile->FATChain[i].FileCluster;
	  NumClustersRem = 0;
  }
  else
#endif
  {
	  NumClustersRem = NumClustersToWalk;
	  CurClusterAbs = FS_FAT_WalkClusterEx(pFileObj->pVolume, pSBfat, pFileObj->Data.Fat.CurClusterAbs, &NumClustersRem);
  }
  //
  // Update values in pFile.
  //
  pFileObj->Data.Fat.CurClusterAbs   = CurClusterAbs;
  pFileObj->Data.Fat.CurClusterFile += NumClustersToWalk - NumClustersRem;   // Advance cluster index by number of clusters walked

  if (NumClustersRem == 0) {
    pFileObj->Data.Fat.NumAdjClusters = FS_FAT_GetNumAdjClustersInChain(pFileObj->pVolume, pSBfat, CurClusterAbs);
  }
  return NumClustersRem;
}


/*********************************************************************
*
*       _WriteEmptyFATSectors
*
*  Function description:
*    This ...
*
*
*  Notes
*/
#if FS_FAT_OPTIMIZE_DELETE
static void _WriteEmptyFATSectors(FS_SB * pSB, U32 FirstFreeFATSector, U32 LastFreeFATSector) {
  U32 NumSectors;
  if (LastFreeFATSector != 0xFFFFFFFF) {
    NumSectors = LastFreeFATSector - FirstFreeFATSector + 1;
    if (NumSectors) {
      FS__SB_Clean(pSB);
      FS_MEMSET(pSB->pBuffer, 0, FS__MaxSectorSize);
      FS_LB_WriteMultiple(pSB->pPart, FirstFreeFATSector, NumSectors, pSB->pBuffer, FS_SECTOR_TYPE_MAN);
      //
      //  Invalidate in smart buffer that this sector has been read.
      //
      pSB->Read = 0;
    }
  }
}
#endif
/*********************************************************************
*
*       _SectorIsBlank
*
*  Function description:
*    This ...
*
*
*/
#if FS_FAT_OPTIMIZE_DELETE
static int _SectorIsBlank(U32 * pSectorBuffer, unsigned SectorSizeU32) {
  do {
    if (*pSectorBuffer++ != 0) {
      return 0;                  /* Not blank */
    }
  } while (--SectorSizeU32);
  return 1;                      /* Blank */
}
#endif
/*********************************************************************
*
*       FS_FAT_FreeClusterChain
*
*  Function description
*    Marks all clsuters in a cluster chain as free.
*
*  Return value
*    Number of remaining clusters: (NumClusters - NumDeletedClusters)
*
*/
U32 FS_FAT_FreeClusterChain(FS_VOLUME * pVolume, FS_SB * pSB, U32 FirstCluster, U32 NumClusters) {
  U32 NextCluster;

#if FS_FAT_OPTIMIZE_DELETE
  if (pVolume->FSInfo.FATInfo.FATType != FS_FAT_TYPE_FAT12) {
    FS_FAT_INFO * pFATInfo;
    U32 FirstFreeFATSector = 0xFFFFFFFF;
    U32 LastFreeFATSector  = 0xFFFFFFFF;

    pFATInfo = &pVolume->FSInfo.FATInfo;

    for (; NumClusters; NumClusters--) {
      U32 Off;
      U32 SectorNo;

      Off      = _ClusterId2FATOff(pFATInfo->FATType, FirstCluster);
      SectorNo = pFATInfo->RsvdSecCnt + (Off >> pFATInfo->ldBytesPerSector) - 1;
      Off &= pFATInfo->BytesPerSec - 1;
      if (Off == 0) {
        if (_SectorIsBlank((U32 *)pSB->pBuffer, pFATInfo->BytesPerSec >>  2)) {
          if (SectorNo == LastFreeFATSector + 1) {
            LastFreeFATSector++;
          } else {
            _WriteEmptyFATSectors(pSB, FirstFreeFATSector, LastFreeFATSector);
            FirstFreeFATSector = SectorNo;
            LastFreeFATSector  = SectorNo;
          }
          FS__SB_MarkNotDirty(pSB);
        }
      }
      NextCluster = FS_FAT_WalkCluster(pVolume, pSB, FirstCluster, 1);
      _WriteFATEntry(pVolume, pSB, FirstCluster, 0);
      if (NextCluster == 0) {
        break;
      }
      FirstCluster = NextCluster;
    }
    _WriteEmptyFATSectors(pSB, FirstFreeFATSector, LastFreeFATSector);
  } else
#endif
  {
    for (; NumClusters; NumClusters--) {
      NextCluster = FS_FAT_WalkCluster(pVolume, pSB, FirstCluster, 1);
      _WriteFATEntry(pVolume, pSB, FirstCluster, 0);
      if (NextCluster == 0) {
        break;
      }
      FirstCluster = NextCluster;
    }
  }
  return NumClusters;
}

/*********************************************************************
*
*       FS_FAT_GetNumAdjClustersInChain
*
*  Function description
*    Return thes number of adjacent clusters in this cluster chain.
*    This allows us to find out if multiple adjacent clusters are used for file storage
*    and reduces the number of FAT-read operations
*
*  Return value
*    Number of adjacent clusters following (in this FAT sector)
*
*  Notes
*    (1) Optimization
*    This code is basically a pure optimization; it allows the read / write routines
*    to reduce the number of FAT reads. Since this routine scans only one FAT sector,
*    the real number may be greater than the value returned
*/
U16 FS_FAT_GetNumAdjClustersInChain(FS_VOLUME * pVolume, FS_SB * pSB, U32 CurCluster) {
  FS_FAT_INFO * pFATInfo;
  U32        BitOff;
  int           NumRemEntries;
  U16        NumClusters;

  NumClusters         = 0;
  pFATInfo            = &pVolume->FSInfo.FATInfo;

  BitOff              = pFATInfo->FATType * CurCluster;         /* Bit number in FAT. */
  BitOff             &= (pFATInfo->BytesPerSec << 3) - 1;       /* Bit number in sector */
  NumRemEntries       = ((pFATInfo->BytesPerSec << 3) - BitOff) / pFATInfo->FATType;

  for (; NumRemEntries > 0; NumRemEntries--) {
    U32 NextCluster;
    NextCluster = FS_FAT_ReadFATEntry(pVolume, pSB, CurCluster);
    if (NextCluster != ++CurCluster) {
      break;                                                    /* This one is not adjacent */
    }
    NumClusters++;
  }
  return NumClusters;
}


/*********************************************************************
*
*       FS_FAT_AllocClusterBlock
*
*  Description:
*    Allocates a cluster block.
*
*  Parameters:
*    pVolume      - Pointer to a volume
*    FirstCluster - First cluster that shall be used for allocation
*    NumClusters  - Number of cluster to allocate
*
*  Return value:
*    ==0         - O.K., allocated NumCluster on block
*    !=0         - Error, clusters can not be allocated as requested
*/
int FS_FAT_AllocClusterBlock(FS_VOLUME * pVolume, U32 FirstCluster, U32 NumClusters, FS_SB * pSB) {
  U32 i;
  U32 LastClusterId;

  //
  // Check if parameters are valid
  //
  if (FirstCluster + NumClusters - FAT_FIRST_CLUSTER> pVolume->FSInfo.FATInfo.NumClusters) {
    return 1;          // Trying to allocate out of bounds clusters.
  }
  //
  // Check if all requested clusters are available
  //
  for (i = 0; i < NumClusters; i++) {
    if (FS_FAT_ReadFATEntry(pVolume, pSB, i + FirstCluster)) {
      return 2;          // Cluster is already used
    }
  }
  //
  // Mark all clusters as used. The first one is the head of the cluster chain
  //
  LastClusterId = FirstCluster;
  for (i = 0; i < NumClusters; i++) {
    LastClusterId = FS_FAT_AllocCluster(pVolume, pSB, LastClusterId);
  }
  FS_FAT_MarkClusterEOC(pVolume, pSB, LastClusterId);
  return 0;
}

/*********************************************************************
*
*       FS_FAT_IsValidShortNameChar
*
*  Return value
*    1      valid character
*    0      not a valid character
*/
int FS_FAT_IsValidShortNameChar(U8 c) {
  if ((c > 0) && (c <= 0x20)) {
     return 0;  // Error
  }
  switch (c) {
  case '"':
//  case '&':
  case '*':
//  case '+':
//  case ',':
  case '/':
  case ':':
//  case ';':
  case '<':
//  case '=':
  case '>':
  case '?':
  case '[':
  case ']':
  case '\\':
     return 0;  // Error
  }
  return 1;     // O.K.
}


/*********************************************************************
*
*       FS_FAT_CopyShortName
*
*/
void FS_FAT_CopyShortName(char * pDest, const char* pSrc, int MaxNumBytes) {
  int i;
  int NumCharacter2Copy;

  if (pDest) {
    NumCharacter2Copy = MIN(MaxNumBytes - 1, 11);
    for (i = 0; i < NumCharacter2Copy; i++) {
      /* Start of extension. If we have no Space, then append the dot. */
      if ((i == 8) && (*pSrc != ' ')) {
        *pDest++ = '.';
      }
      /* if the first character of the directory entry is 0x05,
       * it is changed to 0xe5. FAT spec V1.03: FAT directories
       */
      if ((i == 0) && (*pSrc == 0x05)) {
        pSrc++;
        *pDest++ = (char)0xe5;
      } else if (*pSrc == ' ') { /* Copy everything except Spaces */
         pSrc++;
      } else {
        *pDest++ = *pSrc++;
      }
    }
    *pDest = 0;
  }
}

/*********************************************************************
*
*       FS_FAT_FindDirEntry
*
*  Description:
*    Tries to locate the directory entry in the specified directory.
*    The short name space is searched first;
*    if LFN support is activated, the long name space is search
*    if no short name match was found.
*
*  Parameters:
*    pVolume     - Volume information
*    pEntryName  - Directory entry name
*
*  Return value:
*    != NULL     - pointer to directory entry (in the smart buffer)
*    NULL        - Entry not found
*/
FS_FAT_DENTRY * FS_FAT_FindDirEntry(FS_VOLUME * pVolume, FS_SB * pSB, const char *pEntryName, int Len, U32 DirStart, U8 AttrRequired, int * pLongDirEntryIndex) {
  FS_FAT_DENTRY       * pDirEntry;

  pDirEntry = FAT_pDirEntryAPI->pfFindDirEntry(pVolume, pSB, pEntryName, Len, DirStart, AttrRequired, pLongDirEntryIndex);
  if (FAT_pDirEntryAPI != &FAT_SFN_API) {
    if (pDirEntry == NULL) {
      pDirEntry = FAT_SFN_API.pfFindDirEntry(pVolume, pSB, pEntryName, Len, DirStart, AttrRequired, pLongDirEntryIndex);
    }
  }
  return pDirEntry;
}

/*********************************************************************
*
*       FS_FAT_CopyDirEntryInfo
*
*/
void FS_FAT_CopyDirEntryInfo(FS_FAT_DENTRY * pDirEntry, FS_DIRENTRY_INFO * pDirEntryInfo) {
  pDirEntryInfo->Attributes     = pDirEntry->data[DIR_ENTRY_OFF_ATTRIBUTES];
  pDirEntryInfo->LastWriteTime  = (U32)FS_LoadU16LE(&pDirEntry->data[DIR_ENTRY_OFF_WRITE_DATE]) << 16 | FS_LoadU16LE(&pDirEntry->data[DIR_ENTRY_OFF_WRITE_TIME]);
  pDirEntryInfo->CreationTime   = (U32)FS_LoadU16LE(&pDirEntry->data[DIR_ENTRY_OFF_CREATION_DATE]) << 16 | FS_LoadU16LE(&pDirEntry->data[DIR_ENTRY_OFF_CREATION_TIME]);
  pDirEntryInfo->LastAccessTime = (U32)FS_LoadU16LE(&pDirEntry->data[DIR_ENTRY_OFF_LAST_ACCESS_DATE]) << 16;
  pDirEntryInfo->FileSize       = FS_LoadU32LE(&pDirEntry->data[DIR_ENTRY_OFF_SIZE]);
}

/*********************************************************************
*
*       FS_FAT_CreateJournalFile
*
*  Description:
*    Creates a Journal file for the file system.
*
*  Parameters:
*    pVolume       - Pointer to a mounted volume.
*
*  Return value:
*    0    O.K., Successfully created
*    1    Journal file already present
*   <0    Error
*/
int FS_FAT_CreateJournalFile(FS_VOLUME * pVolume, U32 NumBytes, U32 * pFirstSector, U32 * pNumSectors) {
  FS_FILE       FileHandle;
  FS_FILE_OBJ   FileObj;
  FS_SB         sb;
  FS_FAT_INFO * pFATInfo;
  int           r;
  U32           FirstCluster;
  U32           NumClusters;
  U8            Attributes;

  pFATInfo = &pVolume->FSInfo.FATInfo;
  NumClusters = FS__DivideU32Up(NumBytes, pFATInfo->BytesPerCluster);
  FS__SB_Create(&sb, &pVolume->Partition);
  FS_MEMSET(&FileHandle, 0, sizeof(FS_FILE));
  FS_MEMSET(&FileObj, 0, sizeof(FS_FILE_OBJ));
  //
  // Create journal file
  //
  FileHandle.AccessFlags = FS_FILE_ACCESS_FLAGS_CW;
  FileHandle.pFileObj    = &FileObj;
  FileObj.pVolume        = pVolume;
  if (FS_FAT_Open(FS_JOURNAL_FILE_NAME, &FileHandle, 0, 0, 1)) {
    r = -1;
    goto End;
  }
  //
  // Add clusters to journal file
  //
  FirstCluster = pFATInfo->NumClusters - NumClusters + FAT_FIRST_CLUSTER;
  if (FS_FAT_AllocClusterBlock(pVolume, FirstCluster, NumClusters, &sb)) {
    r = -1;
    goto End;
  }
  //
  // Update file object infomation
  //
  FileObj.FirstCluster = FirstCluster;
  FileObj.Size         = NumClusters * pFATInfo->BytesPerCluster;
  //
  //  Call FS_EFS_Close to update directory entry
  //
  FS_FAT_Close(&FileHandle);
  //
  // Set the file's attribute to SYSTEM and HIDDEN
  //
  Attributes = FS_ATTR_HIDDEN | FS_ATTR_SYSTEM;
  FS_FAT_SetDirEntryInfo(pVolume, FS_JOURNAL_FILE_NAME, &Attributes, FS_DIRENTRY_SET_ATTRIBUTES);
  //
  // Set return value and out parameters
  //
  r = 0;
  *pFirstSector = FS_FAT_ClusterId2SectorNo(pFATInfo, FirstCluster);
  *pNumSectors  = NumClusters * pFATInfo->SecPerClus;
End:
  //
  // Cleanup
  //
  FS__SB_Delete(&sb);
  return r;
}


/*********************************************************************
*
*       FS_FAT_OpenJournalFile
*
*  Description:
*    Tries a Journal file for the file system.
*
*  Parameters:
*    pVolume       - Pointer to a mounted volume.
*
*  Return value:
*     0    O.K., journal file exists
*   !=0    Error
*/
int FS_FAT_OpenJournalFile(FS_VOLUME * pVolume) {
  FS_FILE       FileHandle;
  FS_FILE_OBJ   FileObj;
  int           r;

  r  = 0;
  FS_MEMSET(&FileHandle, 0, sizeof(FS_FILE));
  FS_MEMSET(&FileObj,    0, sizeof(FS_FILE_OBJ));
  //
  // Open journal file
  //
  FileHandle.AccessFlags = FS_FILE_ACCESS_FLAG_R;
  FileHandle.pFileObj    = &FileObj;
  FileObj.pVolume        = pVolume;
  if (FS_FAT_Open(FS_JOURNAL_FILE_NAME, &FileHandle, 0, 1, 0)) {
    r = 1;
  }
  return r;
}

/*********************************************************************
*
*       FS_FAT_GetIndexOfLastSector
*
*  Description:
*    Returns the last sector that is used by the FS.
*
*  Parameters:
*    pVolume       - Pointer to a mounted volume.
*
*/
U32  FS_FAT_GetIndexOfLastSector(FS_VOLUME * pVolume) {
  FS_FAT_INFO * pFATInfo;

  pFATInfo = &pVolume->FSInfo.FATInfo;
  return FS_FAT_ClusterId2SectorNo(pFATInfo, pFATInfo->NumClusters + FAT_FIRST_CLUSTER - 1) + pFATInfo->SecPerClus - 1;
}



/*********************************************************************
*
*       Public const
*
**********************************************************************
*/
const FAT_DIRENTRY_API FAT_SFN_API = {
  _SFN_ReadDirEntryInfo,
  _SFN_FindDirEntry,
  _SFN_CreateDirEntry,
  NULL
};



/*************************** End of file ****************************/
