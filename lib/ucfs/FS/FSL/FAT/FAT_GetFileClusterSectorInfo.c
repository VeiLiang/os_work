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
File        : FS_FAT_SetEndOfFile.c
Purpose     : FAT routine for setting the end of file position
---------------------------END-OF-HEADER------------------------------
*/

/*********************************************************************
*
*             #include Section
*
**********************************************************************
*/

#include "FAT_Intern.h"
#include "xm_type.h"
#include "xm_base.h"
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



// 预先访问空闲的簇块
static U32 FS_FAT_PreFindFreeClusters(FS_VOLUME * pVolume, FS_SB * pSB, U32 FirstCluster, U32 BlockSize) {
  U32 i;
  U32 LastCluster;
  FS_FAT_INFO* pFATInfo;
  U32 FreeClusterCount = 0;
  U32 ClusterCount;
  unsigned long end_ticket = XM_GetTickCount() + 20;

  pFATInfo = &pVolume->FSInfo.FATInfo;
  LastCluster = pFATInfo->NumClusters + FAT_FIRST_CLUSTER - 1;    /* Cluster id of first cluster is 2 */
  
  ClusterCount = (BlockSize + pFATInfo->BytesPerCluster - 1) / pFATInfo->BytesPerCluster;

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
  while(ClusterCount > 0)
  {
	  do {
		 if (FS_FAT_ReadFATEntry(pVolume, pSB, i) == 0) {
			FreeClusterCount ++;
			ClusterCount --; /* We found a free cluster */
			if(ClusterCount == 0)
				break;
		 }
		 if(XM_GetTickCount() >= end_ticket)
			 break;
	  } while (++i <= LastCluster);
	  
	  if(ClusterCount == 0)
		  return FreeClusterCount;
	  
	  /*
		* Continue search from first cluster on
		*/
	  for (i = FAT_FIRST_CLUSTER; i < FirstCluster; i++) {
		 if (FS_FAT_ReadFATEntry(pVolume, pSB, i) == 0) {
			/* We found a free cluster */
			FreeClusterCount ++;
			ClusterCount --;
			if(ClusterCount == 0)
				break;
		 }
		 if(XM_GetTickCount() >= end_ticket)
			 break;		 
	  }
	  
	  break;
  }
  
  return FreeClusterCount;                /* Error, no free cluster */
}

#if FS_FAT_USE_CLUSTER_CACHE
extern void FS_FAT_ClusterCacheScan (FS_VOLUME * pVolume, FS_SB * pSB);
#endif
U32 FS_PreFindFreeClusters (FS_FILE * pFile, U32 BlockSize)
{
  int           r;
  FS_SB            SB;
  FS_VOLUME * pVolume;
  U32  FirstCluster = 0;
  r = -1;
	
  if(pFile)
  {
	  FS_LOCK();
	  pVolume = pFile->pFileObj->pVolume; 
	  FS_LOCK_DRIVER(&pFile->pFileObj->pVolume->Partition.Device);
	  FS__SB_Create(&SB, &pVolume->Partition);
#if FS_FAT_USE_CLUSTER_CACHE
	  FS_FAT_ClusterCacheScan (pVolume, &SB);
#endif
	  //r = FS_FAT_PreFindFreeClusters (pVolume, &SB, FirstCluster, BlockSize);
	  r = 0;

	  FS__SB_Delete(&SB);
	  FS_UNLOCK_DRIVER(&pFile->pFileObj->pVolume->Partition.Device);
	  FS_UNLOCK();
  }
  return r;
}

/*********************************************************************
*
*       _CutUpdateDirEntry
*/
static void _CutUpdateDirEntry(FS_FILE_OBJ * pFileObj, FS_SB * pSB) {
  FS_FAT_DENTRY * pDirEntry;
  U32             TimeDate;
  U32             DirSectorNo;
  U16             BytesPerSector;
  U16             SectorOff;

  BytesPerSector = pFileObj->pVolume->FSInfo.FATInfo.BytesPerSec;
  DirSectorNo    = pFileObj->Data.Fat.DirEntrySector;
  FS__SB_SetSector(pSB, DirSectorNo, FS_SB_TYPE_DIRECTORY);
  SectorOff = (pFileObj->Data.Fat.DirEntryIndex  * sizeof(FS_FAT_DENTRY)) & (BytesPerSector - 1);
  pDirEntry = (FS_FAT_DENTRY *) (pSB->pBuffer + SectorOff);

  if (FS__SB_Read(pSB) == 0) {
    //
    // Modify directory entry
    //
    FS_StoreU32LE(&pDirEntry->data[DIR_ENTRY_OFF_SIZE], pFileObj->Size);
    FS_FAT_WriteDirEntryCluster(pDirEntry, pFileObj->FirstCluster);
    TimeDate = FS_X_GetTimeDate();
    FS_StoreU16LE(&pDirEntry->data[DIR_ENTRY_OFF_WRITE_TIME], (U16)(TimeDate & 0xffff));
    FS_StoreU16LE(&pDirEntry->data[DIR_ENTRY_OFF_WRITE_DATE], (U16)(TimeDate >> 16));
    FS__SB_Flush(pSB);                   /* Write the modified directory entry */
  }
}

/*********************************************************************
*
*       _CutFileFromHead
*
*  Purpose
*    Truncates the file. This means, the file's cluster chain will
*    be shorten
*
*/
static int _CutFileFromHead(FS_FILE * pFile, FS_VOLUME * pVolume, FS_SB * pSB, I32 nCutPos	) {
  FS_FAT_INFO * pFATInfo;
  FS_FILE_OBJ * pFileObj;
  U32           NumCluster2Del;
  U32           NumCutClusters;
  U32           CutSize;
  int           r;
  U32 Cluster;
  U32 NextCluster;
  U32 FirstCluster;
  

  pFileObj        = pFile->pFileObj;
  // 从FilePos指定的位置（簇对齐），将之前的簇块释放
  CutSize         = nCutPos;
  pFATInfo        = &pVolume->FSInfo.FATInfo;
  //
  // Calculate the number of cluster for the new file size.
  //
  NumCutClusters  = (CutSize + pFATInfo->BytesPerCluster - 1) / pFATInfo->BytesPerCluster;
  //
  //   Number of clusters to delete
  //
  if(NumCutClusters == 0)
	  return 0;
  if(nCutPos & (pFATInfo->BytesPerCluster - 1))	// cluster size alignment
	  return 0;
  
  
  //
  // Go to the cluster from where we will deallocate the "unused" clusters.
  //
  Cluster = pFileObj->FirstCluster;
  if(Cluster == 0)
	  return 0;
  
  FirstCluster = pFileObj->FirstCluster;
  NumCluster2Del = 0;
  do {
	  NumCluster2Del ++;
	  NumCutClusters --;
	  NextCluster = FS_FAT_WalkCluster(pVolume, pSB, Cluster, 1);
	  if(NextCluster == 0)
		  break;
	  Cluster = NextCluster;
  } while (NumCutClusters > 0);
  //
  // Update the Cluster information in pFileObj.
  //
  pFileObj->Data.Fat.CurClusterAbs  = NextCluster;
  pFileObj->Data.Fat.CurClusterFile = 0;
  pFileObj->Data.Fat.NumAdjClusters = 0;
  
  pFile->FilePos = 0;
  pFileObj->FirstCluster = NextCluster;
  if(NextCluster == 0)
    pFileObj->Size = 0;
  else
	 pFileObj->Size = pFileObj->Size - NumCluster2Del * pFATInfo->BytesPerCluster;
  
  // 为防止Cluster表断链造成的大磁盘空间丢失，首先修改目录项的簇表指针，这样即使簇表写入未完成，也仅丢失一小部分的磁盘空间
  _CutUpdateDirEntry (pFileObj, pSB);
  
  //
  // Free the cluster chain.
  //
  FS_FAT_FreeClusterChain(pVolume, pSB, FirstCluster, NumCluster2Del);
  
 r = 0;
 return r;
}

// 将文件头开始到达当前文件位置的内容删除
int FS_CutFile (FS_FILE * pFile, I32 nCutPos)
{
  int           r;
  FS_SB            SB;

  FS_VOLUME * pVolume;
  r = -1;
  
  if(pFile)
  {
	  FS_LOCK();
	  pVolume = pFile->pFileObj->pVolume; 
	  FS_LOCK_DRIVER(&pFile->pFileObj->pVolume->Partition.Device);
	  FS__SB_Create(&SB, &pVolume->Partition);
	  r = _CutFileFromHead (pFile, pVolume, &SB, nCutPos);
	  FS__SB_Delete(&SB);
	  FS_UNLOCK_DRIVER(&pFile->pFileObj->pVolume->Partition.Device);
	  FS_UNLOCK(); 
  }
  return r;
}

static int FS_FAT_CutOffFile (FS_FILE * pFile, FS_SB *pSB)
{
  FS_VOLUME      * pVolume;
  FS_FILE_OBJ    * pFileObj;
  FS_FAT_INFO    * pFATInfo;
  U8  FATType;
  U32 Cluster;
  U32 Off;          /* Total offset in bytes */
  U32 NumCluster2Del;
  U32 FirstCluster;
  U32 NumCutClusters;
  U32 LastSectorNo, SectorNo;
  U32 NextCluster;
  U32 SectorCount;			// 每次最多写入2个扇区
  U32 SectorSetCount;		// 扇区号非连续的扇区集个数
  int ret = 0;
  
  pFileObj        = pFile->pFileObj;
  pVolume         = pFileObj->pVolume;
  pFATInfo        = &pVolume->FSInfo.FATInfo;
  FATType 			= pFATInfo->FATType;
  
  if(FATType == FS_FAT_TYPE_FAT12)	// 不支持FAT12
	  return (-1);

  Cluster = pFileObj->FirstCluster;
  if(Cluster == 0)
	  return 1;		// 结束
  if(pFileObj->Size == 0)
	  return 1;
    
  // 计算文件总的簇数
  NumCutClusters  = (pFileObj->Size + pFATInfo->BytesPerCluster - 1) / pFATInfo->BytesPerCluster;
  FirstCluster = Cluster;
  NumCluster2Del = 0;
  
  // 计算文件的第一个簇所在的扇区号
  Off = _ClusterId2FATOff(FATType, FirstCluster);
  LastSectorNo = (U32)(pFATInfo->RsvdSecCnt + (Off >> pFATInfo->ldBytesPerSector));
  SectorCount = 1;
  SectorSetCount = 1;
  
  // 统计需要删除的簇数
  do
  {
	 NumCluster2Del ++;
	 NumCutClusters --;
	 
	 NextCluster = FS_FAT_WalkCluster(pVolume, pSB, Cluster, 1);
	 if(NextCluster == 0)
	 {
		 ret = 1;
		 break;
	 }
	 
	 // 计算下一个簇所在的物理扇区号
	 Off = _ClusterId2FATOff(FATType, NextCluster);
	 SectorNo = pFATInfo->RsvdSecCnt + (Off >> pFATInfo->ldBytesPerSector);
	 if(SectorNo != LastSectorNo)
	 {
		 if(SectorNo != (LastSectorNo + 1))
			 SectorSetCount ++;
		 LastSectorNo = SectorNo;
		 SectorCount ++;
		 // 每次删除操作时，尽量减少不同扇区读写操作的次数，避免对其他任务磁盘IO阻塞时间过长。
		 // 此处定义每次删除过程中读写的非连续扇区区域数不超过2, 连续的总扇区数不超过4
		 //if(SectorSetCount >= 2 || SectorCount >= 4)		
		 // 20170929 减少对磁盘的写访问次数, 优化文件删除效率
		 if(SectorSetCount >= 8 || SectorCount >= 16)		
		 {
			 ret = 0;	// 标记删除尚未完成
			 break;
		 }
	 }
	 
	 Cluster = NextCluster;
	 
  } while(NumCutClusters > 0);
  
  if(NumCutClusters == 0)
  {
	  NextCluster = 0;
	  ret = 0;
  }
  
  //
  // Update the Cluster information in pFileObj.
  //
  pFileObj->Data.Fat.CurClusterAbs  = NextCluster;
  pFileObj->Data.Fat.CurClusterFile = 0;
  pFileObj->Data.Fat.NumAdjClusters = 0;
  
  pFileObj->FirstCluster = NextCluster;
  if(NextCluster == 0 || NumCutClusters == 0)
    pFileObj->Size = 0;
  else
  {
	  if(pFileObj->Size >= NumCluster2Del * pFATInfo->BytesPerCluster)
	  	 pFileObj->Size = pFileObj->Size - NumCluster2Del * pFATInfo->BytesPerCluster;
	  else
		 pFileObj->Size = 0;
  }
  
  // 为防止Cluster表断链操作后掉电造成的大磁盘空间丢失(簇与目录项失去连接)，
  // 首先修改目录项的簇表指针(指向簇链断开处)，这样即使簇表写入未完成，也仅丢失一小部分(簇链断开处前面的内容)的磁盘空间
  _CutUpdateDirEntry (pFileObj, pSB);  
  
  //
  // Free the cluster chain.
  //
  FS_FAT_FreeClusterChain(pVolume, pSB, FirstCluster, NumCluster2Del);
  
  return ret;
}

// 按照最优化的方法将文件长度逐步阶段从头截断
// 每次截断的内容,其簇位于相同/或连续的扇区中
int FS_CutOffFile (FS_FILE * pFile)
{
  int           r;
  FS_SB         SB;
  FS_VOLUME * pVolume;
  r = -1;

  if(pFile)
  {
	  FS_LOCK();
	  pVolume = pFile->pFileObj->pVolume; 
	  FS_LOCK_DRIVER(&pFile->pFileObj->pVolume->Partition.Device);
	  FS__SB_Create(&SB, &pVolume->Partition);
	  r = FS_FAT_CutOffFile (pFile, &SB);
	  FS__SB_Delete(&SB);
	  FS_UNLOCK_DRIVER(&pFile->pFileObj->pVolume->Partition.Device);
	  FS_UNLOCK();
	  
	  FS_CACHE_Clean("");  
  }
  
  return r;
}




/*************************** End of file ****************************/
