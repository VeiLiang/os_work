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
File        : FS_Journal.c
Purpose     : Implementation of Journal for embedded file system
---------------------------END-OF-HEADER------------------------------

Sector usage of journal:
      -------------------------------------------
      0                          Status
      1 - n                      Copy-list
      n+1 - m-2                  Raw data to be transferred
      m-1                        Info sector

      where n: Number of sectors used for Copy-list
            m: Total number of sectors in journal
      -------------------------------------------

Management organisation
      -------------------------------------------
      0x00 - 0x1F                Header: Number of data sectors etc
      0x20 - ....                Assignment info, 16 bytes per sector
      -------------------------------------------

*/

/*********************************************************************
*
*             #include Section
*
**********************************************************************
*/

#include <stdlib.h>

#include "FS_Int.h"

/*********************************************************************
*
*       Defines, fixed
*
**********************************************************************
*/
#define VERSION                    10000
#define SIZEOF_SECTOR_LIST_ENTRY      16
#define JOURNAL_INDEX_INVALID      (0xFFFFFFFF)

#define INFO_SECTOR_TAG            "Journal info\0\0\0"
#define MAN_SECTOR_TAG             "Journal status\0"
#define SIZEOF_INFO_SECTOR_TAG     16
#define SIZEOF_MAN_SECTOR_TAG      16

/*********************************************************************
*
*       Defines, configurable
*
**********************************************************************
*/
/*********************************************************************
*
*       Offsets in info sector
*/
#define OFF_INFO_VERSION             (0x10)
#define OFF_INFO_NUM_TOTAL_SECTORS   (0x20)

/*********************************************************************
*
*       Offsets in management info
*/
#define OFF_MAN_SECTOR_CNT           (0x10)
#define OFF_MAN_SECTOR_LIST          (0x20)

/*********************************************************************
*
*       Type definitions
*
**********************************************************************
*/

/*********************************************************************
*
*       INST structure (and STATUS sub-structure)
*
*  This is the central data structure for the entire driver.
*  It contains data items of one instance of the driver.
*/

typedef struct {
	int OpenCnt;
	U32 NumSectorsTotal;        // Total number of sectors allocated to journal
  U32 NumSectorsData;         // Number of sectors available for data
	U32 BytesPerSector;         // Number of bytes per sector. Typically 512.
	U32 PBIInfoSector;          // Physical sector index of last sector of journal. The contents of this sector never change.
	U32 PBIStatusSector;        // Physical sector index of first sector of journal. Used to store status information.
	U32 PBIStartSectorList;     // Physical sector index of first sector of the sector list.
	U32 PBIFirstDataSector;     // Physical sector index of first sector used to store user data ("payload").
	U32 SectorCnt;              // Count of sectors currently in journal
  U8  IsPresent;
} STATUS;

typedef struct {
  STATUS Status;
  U32    NumEntriesJ2P;       // Number of entries for which memory has been allocated.
	U32 *  apJ2P;               // Journal to physical table. Input: journal index (file system view). Output: Physical index (hardware/driver view)
  FS_VOLUME * pVolume;
} INSt;


/*********************************************************************
*
*       Static data
*
**********************************************************************
*/
static INSt  * _apInst[FS_NUM_VOLUMES];

/*********************************************************************
*
*       Static code
*
**********************************************************************
*/


/*********************************************************************
*
*       _Volume2Inst
*/
static INSt * _Volume2Inst(FS_VOLUME * pVolume) {
  int Unit;
  INSt * pInst;
  Unit = pVolume - FS__aVolume;
  if (_apInst[Unit] == NULL) {
    _apInst[Unit] = (INSt *)FS_AllocZeroed(sizeof(INSt));
    pInst = _apInst[Unit];
    pInst->pVolume = pVolume;
    pVolume->Partition.Device.Data.JournalUnit = Unit;
  }
  pInst = _apInst[Unit];
  return pInst;
}

/*********************************************************************
*
*       _WriteSectorOut
*/
static int _WriteSectorOut(INSt * pInst, U32 SectorIndex, const U8 * pData) {
  FS_DEVICE * pDevice;
  U8          Unit;

  pDevice = &pInst->pVolume->Partition.Device;
  Unit = pDevice->Data.Unit;
  return pDevice->pType->pfWrite(Unit, SectorIndex, pData, 1, 0);
}

/*********************************************************************
*
*       _InitStatus
*
*  Description:
*    Initialize the status data.
*    This makes sure the all routine that depend on this status
*    are working with correct status information.
*
*/
static void _InitStatus(INSt * pInst) {
  STATUS * pStatus;

  pStatus = &pInst->Status;
  //
  //  Invalidate all information with 0.
  //
  FS_MEMSET(pStatus, 0, sizeof(STATUS));
}

/*********************************************************************
*
*       _InitInst
*/
static void _InitInst(INSt * pInst, U32 FirstSector, U32 NumSectors) {
	U32    NumBytes;
  U32    NumSectorsData;
  U32    NumSectorsManagement;
  U32    FirstSectorAfterJournal;
  U32    BytesPerSector;

  _InitStatus(pInst);
  FirstSectorAfterJournal   = FirstSector + NumSectors;
  pInst->Status.PBIInfoSector      = FirstSectorAfterJournal - 1;               // Info sector. Contents never change
  pInst->Status.PBIStatusSector    = FirstSectorAfterJournal - NumSectors;      // Status sector. First sector in journal.
  pInst->Status.PBIStartSectorList = pInst->Status.PBIStatusSector + 1;                // Start of sector list
  //
  // Compute the number of sectors which can be used to store data
  //
  BytesPerSector            = pInst->pVolume->FSInfo.Info.BytesPerSector;
  NumBytes                  = (NumSectors - 3) * BytesPerSector;                            // Total number of bytes for data & management. 3 sectors subtract for info, status and head of sector list
  NumSectorsData            = NumBytes / (BytesPerSector + SIZEOF_SECTOR_LIST_ENTRY);       // This computation is a bit simplified and may waste one sector in some cases
  NumSectorsManagement      = FS__DivideU32Up(NumSectorsData * SIZEOF_SECTOR_LIST_ENTRY, BytesPerSector);
  //
  // Store information
  //
  pInst->Status.BytesPerSector     = BytesPerSector;
  pInst->Status.NumSectorsData     = NumSectorsData;
  pInst->Status.PBIFirstDataSector = pInst->Status.PBIStartSectorList + NumSectorsManagement;      // Data sectors follow sector list
  //
  // Allocate memory for lookup table if necessary
  //
  NumBytes = NumSectorsData * sizeof(U32);
  //
  //  Initialize the journal assignment (J2P) table
  //
  if (pInst->NumEntriesJ2P < NumSectorsData) {
    // FS_Alloc_FreePtr(&pInst->apJ2P);
    pInst->NumEntriesJ2P = NumSectorsData;
    pInst->apJ2P = (U32 *)FS_AllocZeroed(sizeof(U32) * NumSectorsData);
  }
}

/*********************************************************************
*
*       _FindSector
*
*  Function description
*    Locate a logical sector (as seen by the file system) in the journal.
*
*  Return value
*    If found:   Position in journal
*    else    :   JOURNAL_INDEX_INVALID
*
*/
static U32 _FindSector(INSt * pInst, U32 lsi) {
  U32 u;
  U32 SectorCnt;

  SectorCnt = pInst->Status.SectorCnt;
  for (u = 0; u < SectorCnt; u++) {
    if (*(pInst->apJ2P + u) == lsi) {
      return u;
    }
  }
  return JOURNAL_INDEX_INVALID;
}

/*********************************************************************
*
*       _CopyData
*
*  Function description:
*    Copies data from journal to real position in FS.
*/
static int _CopyData(INSt * pInst, U8 * pData) {
  U32 SectorCnt;
  U32 lsi;
  U32 u;
  U8  Unit;
  FS_DEVICE * pDevice;

  pDevice   = &pInst->pVolume->Partition.Device;
  Unit      = pDevice->Data.Unit;
  SectorCnt = pInst->Status.SectorCnt;
  for (u = 0; u < SectorCnt; u++) {
    //
    // Read from journal
    //
    if (pDevice->pType->pfRead(Unit, pInst->Status.PBIFirstDataSector + u, pData, 1) < 0) {
      FS_DEBUG_ERROROUT("Read error while playing journal");
      return -1;    // Error
    }
    //
    // Write to device
    //
    lsi = *(pInst->apJ2P + u);
    if (pDevice->pType->pfWrite(Unit, lsi, pData, 1, 0) < 0) {
      FS_DEBUG_ERROROUT("Error playing journal");
      return -1;    // Error
    }
  }
  return 0;
}

/*********************************************************************
*
*       _ClearJournal
*
*  Function description:
*    Clear the journal. This means resetting the sector count and updating (= writing to device) management info.
*/
static void _ClearJournal(INSt * pInst, U8 * pData) {
  //
  // Write Status sector
  //
  FS_MEMSET(pData, 0, pInst->Status.BytesPerSector);
  FS_MEMCPY(pData, MAN_SECTOR_TAG, 16);
  _WriteSectorOut(pInst, pInst->Status.PBIStatusSector, pData);
  FS_MEMSET(pInst->apJ2P, 0, sizeof(U32) * pInst->Status.SectorCnt);
  pInst->Status.SectorCnt = 0;
}

/*********************************************************************
*
*       _CleanJournal
*
*  Function description:
*    This routines copies the data in the journal to the "real" destination and cleans the journal in the foloowing steps:
*    - Write journal management info
*    - Copy data
*    - Clear journal (rewriting management info)
*/
static int _CleanJournal(INSt * pInst) {
  U32    u;
  U32    SectorCnt;
  U32    lsi;
  U8   * pData;
  FS_SB  sb;
  U32    BytesPerSector;
  U32    Off;
  U32    BPSMask;
  int    r = 0;

  SectorCnt = pInst->Status.SectorCnt;
  BytesPerSector = pInst->Status.BytesPerSector;
  //
  // Write out the journal only if there are any data written to it.
  //
  if (SectorCnt) {
    FS__SB_Create(&sb, &pInst->pVolume->Partition);
    pData = sb.pBuffer;
    FS_MEMSET(pData, 0, BytesPerSector);
    //
    // Write Copy-list
    //
    BPSMask = (BytesPerSector -1);
    for (u = 0; u < SectorCnt; u++) {
      Off = u * SIZEOF_SECTOR_LIST_ENTRY;
      lsi = *(pInst->apJ2P + u);
      FS_StoreU32LE(pData + (Off & BPSMask), lsi);
      //
      // Write sector if it is either last entry of copy list or in last entry of this sector
      //
      if ((u == SectorCnt - 1) | ((Off & BPSMask) + SIZEOF_SECTOR_LIST_ENTRY == BytesPerSector)) {
        U32 SectorIndex;

        SectorIndex = Off / BytesPerSector + pInst->Status.PBIStartSectorList;
        if (SectorIndex == pInst->Status.PBIFirstDataSector) {
          FS_DEBUG_ERROROUT("Fatal error: Writing management information into the data area");
        }
        _WriteSectorOut(pInst, SectorIndex, pData);
        FS_MEMSET(pData, 0, BytesPerSector);
      }
    }
    //
    // Write Status sector
    //
    FS_MEMSET(pData, 0, BytesPerSector);
    FS_MEMCPY(sb.pBuffer, MAN_SECTOR_TAG, SIZEOF_MAN_SECTOR_TAG);
    FS_StoreU32LE(pData + OFF_MAN_SECTOR_CNT, SectorCnt);
    _WriteSectorOut(pInst, pInst->Status.PBIStatusSector, pData);
    //
    // Copy data from journal to its real destination
    //
    _CopyData(pInst, pData);
    _ClearJournal(pInst, pData);
    //
    // Cleanup
    //
    FS__SB_Delete(&sb);
  }
  return r;
}

/*********************************************************************
*
*       _Write
*
*  Description:
*    FS driver function. Writes one or more logical sectors to storage device.
*
*  Return value:
*      0                       - Data successfully written.
*    !=0                        - An error has occured.
*
*/
static int _Write(INSt * pInst, U32 LogSec, const void* pData, U32 NumSectors, U8 RepeatSame) {
  U32 psi;
  U32 JournalIndex;
  FS_DEVICE * pDevice;

  pDevice = &pInst->pVolume->Partition.Device;
  while (NumSectors) {
    //
    // Try to locate sector in journal
    //
    JournalIndex = _FindSector(pInst, LogSec);
    if (JournalIndex != JOURNAL_INDEX_INVALID) {
      psi = JournalIndex + pInst->Status.PBIFirstDataSector;
    } else {
      if (pInst->Status.SectorCnt == pInst->Status.NumSectorsData) {
        FS_DEBUG_WARN("Journal is full. Flushing.");      // Note: Journal can no longer guarantee that operations are atomic!
        _CleanJournal(pInst);
      }
      *(pInst->apJ2P + pInst->Status.SectorCnt) = LogSec;
      psi = pInst->Status.SectorCnt + pInst->Status.PBIFirstDataSector;
      pInst->Status.SectorCnt++;
    }
    //
    // Write to device
    //
    if (pDevice->pType->pfWrite(pDevice->Data.Unit, psi, pData, 1, 0) < 0) {
      return -1;    // Error
    }
    NumSectors--;
    LogSec++;
    if (RepeatSame == 0) {
      pData = (const void *)(pInst->Status.BytesPerSector + (U8*)pData);    // pData += BytesPerSector
    }
  }
  return 0;
}

/*********************************************************************
*
*       _Read
*
*  Description:
*    FS driver function. Reads one logical sector to given buffer space
*
*  Parameters:
*    DevIndex                  - Device index number.
*    Logical Sector            - Sector number
*    Buffer                    - Pointer to data buffer
*
*
*  Return value:
*    ==0                       - Device okay and ready for operation.
*    <0                        - An error has occured.
*
*/
static int _Read(INSt * pInst, U32 LogSec, void *pData, U32 NumSectors) {
  U32 psi;
  U32 JournalIndex;
  FS_DEVICE * pDevice;

  pDevice = &pInst->pVolume->Partition.Device;
  while (NumSectors) {
    psi = LogSec;
    //
    // Try to locate sector in journal
    //
    JournalIndex = _FindSector(pInst, LogSec);
    if (JournalIndex != JOURNAL_INDEX_INVALID) {
      psi = JournalIndex + pInst->Status.PBIFirstDataSector;
    }
    //
    // Read from device
    //
    if (pDevice->pType->pfRead(pDevice->Data.Unit, psi, pData, 1) < 0) {
      return -1;    // Error
    }
    NumSectors--;
    LogSec++;
    pData = (void *)(pInst->Status.BytesPerSector + (U8*)pData);    // pData += BytesPerSector
  }
  return 0;
}

/*********************************************************************
*
*       Public code
*
**********************************************************************
*/

/*********************************************************************
*
*       FS_JOURNAL_Create
*
*  Function description:
*    Creates the journal
*
*  Return value:
*    == 0             - O.K.
*    != 0             - Error
*/
int FS_JOURNAL_Create(FS_VOLUME * pVolume, U32 FirstSector, U32 NumSectors) {
  FS_SB  sb;
  U8 *   pData;
  INSt * pInst;
  U32    BytesPerSector;

  pInst = _Volume2Inst(pVolume);
  _InitInst(pInst, FirstSector, NumSectors);
  BytesPerSector = pInst->Status.BytesPerSector;
  //
  // Prepare and write Info sector
  //
  FS__SB_Create(&sb, &pVolume->Partition);
  pData = sb.pBuffer;
  FS_MEMSET(pData, 1, BytesPerSector);
  FS_MEMCPY(sb.pBuffer, INFO_SECTOR_TAG, SIZEOF_INFO_SECTOR_TAG);
  FS_StoreU32LE(pData + OFF_INFO_VERSION,           VERSION);
  FS_StoreU32LE(pData + OFF_INFO_NUM_TOTAL_SECTORS, NumSectors);
  _WriteSectorOut(pInst, pInst->Status.PBIInfoSector, pData);
  //
  // Clear journal
  //
  _ClearJournal(pInst, pData);
  pInst->Status.IsPresent = 1;
  pVolume->Partition.Device.Data.JournalIsActive = 1;
  //
  // Cleanup
  //
  FS__SB_Delete(&sb);
  return 0;
}


/*********************************************************************
*
*       _Mount
*
*  Function description:
*    Mounts the journal layer, replaying the journal.
*
*  Return value:
*    == 0             - O.K.
*    != 0             - Error
*/
static int _MountEx(FS_VOLUME * pVolume, U32 LastSectorInFS) {
  FS_SB  sb;
  int    r;
  U8   * pData;
  U8     Unit;
  INSt * pInst;
  U32    FirstSector;
  U32    NumSectors;
  U32    SectorCnt;
  U32    lsi;
  U32    u;
  U32    Off;
  U32    BytesPerSector;
  const FS_DEVICE_TYPE * pDriver;

  pInst = _Volume2Inst(pVolume);
  FS__SB_Create(&sb, &pVolume->Partition);
  pData   = sb.pBuffer;
  Unit    = pVolume->Partition.Device.Data.Unit;
  pDriver = pVolume->Partition.Device.pType;
  r       = 1;

  //
  // Read info sector
  //
  if (pDriver->pfRead(Unit, LastSectorInFS, pData, 1)) {    // Read the info sector (last sector of partition)
    FS_DEBUG_WARN("Read of journal info sector failed");
    goto CleanUp;
  }
  //
  // Check sector for validity
  //
  if (FS_MEMCMP(pData, INFO_SECTOR_TAG, SIZEOF_INFO_SECTOR_TAG)) {
    FS_DEBUG_WARN("No journal found");
    goto CleanUp;
  }
  if (FS_LoadU32LE(pData + OFF_INFO_VERSION) != VERSION) {
    FS_DEBUG_WARN("Journal file version does not match");
    goto CleanUp;
  }
  //
  // Retrieve static information from info sector. This info is written when the journal is created and never changes.
  //
  NumSectors  = FS_LoadU32LE(pData + OFF_INFO_NUM_TOTAL_SECTORS);
  FirstSector = LastSectorInFS - NumSectors + 1;
  _InitInst(pInst, FirstSector, NumSectors);
  BytesPerSector = pInst->Status.BytesPerSector;
  //
  //  Read status sector, check for validity
  //
  if (pDriver->pfRead(Unit, pInst->Status.PBIStatusSector, pData, 1)) {    // Read the status sector
    FS_DEBUG_WARN("Read of journal status sector failed");
    goto CleanUp;
  }
  if (FS_MEMCMP(pData, MAN_SECTOR_TAG, SIZEOF_MAN_SECTOR_TAG)) {
    FS_DEBUG_WARN("Management sector invalid");
    goto CleanUp;
  }
  //
  //  Check if any entries are in the journal
  //
  SectorCnt = FS_LoadU32LE(pData + OFF_MAN_SECTOR_CNT);
  if (SectorCnt) {
    //
    // Read management info
    //
    for (u = 0; u < SectorCnt; u++) {
      Off = u * SIZEOF_SECTOR_LIST_ENTRY;
      if ((Off & (BytesPerSector - 1)) == 0) {
        if (pDriver->pfRead(Unit, Off / BytesPerSector + pInst->Status.PBIStartSectorList, pData, 1)) {
          FS_DEBUG_WARN("Management sector information could not be read");
          goto CleanUp;
        }
      }
      Off &= BytesPerSector - 1;
      lsi = FS_LoadU32LE(pData + Off);
      *(pInst->apJ2P + u) = lsi;
    }
    pInst->Status.SectorCnt = SectorCnt;
    //
    // Copy data from journal to its real destination
    //
    _CopyData(pInst, pData);
    _ClearJournal(pInst, pData);
  }
  // Journal mounted successfully
  r = 0;
  pInst->Status.IsPresent = 1;
  pVolume->Partition.Device.Data.JournalIsActive = 1;
CleanUp:
  //
  // Cleanup
  //
  FS__SB_Delete(&sb);
  return r;
}


/*********************************************************************
*
*       FS_JOURNAL_Mount
*
*  Function description:
*    Mounts the journal layer, replaying the journal.
*
*  Return value:
*    == 0             - O.K.
*    != 0             - Error
*/
int FS_JOURNAL_Mount(FS_VOLUME * pVolume) {
  int r = 1;
  if (pVolume->IsMounted == FS_MOUNT_RW) {
    if (FS_OPEN_JOURNAL_FILE(pVolume) == 0) {
      U32 LastSector;
      LastSector = FS_GET_INDEX_OF_LAST_SECTOR(pVolume);
      r = _MountEx(pVolume, LastSector);
    }
  }
  return r;
}


/*********************************************************************
*
*       FS_JOURNAL_Begin
*
*  Function description:
*    Opens the journal. This means all relevant data is written to the journal,
*    INSTXXXead of the "real destination".
*
*  Return value:
*    == 0             - O.K.
*    != 0             - Error
*/
int FS_JOURNAL_Begin(FS_VOLUME * pVolume) {
  INSt * pInst;

  pInst = _Volume2Inst(pVolume);
  return ++pInst->Status.OpenCnt;
}

/*********************************************************************
*
*       FS_JOURNAL_End
*
*  Function description:
*    Closes the journal.
*    This means all relevant data is written to the journal,
*    INSTXXXead of the "real destination".
*
*  Return value:
*    == 0             - O.K.
*    != 0             - Error
*/
int FS_JOURNAL_End(FS_VOLUME * pVolume) {
  INSt * pInst;

  pInst = _Volume2Inst(pVolume);
	if (--pInst->Status.OpenCnt == 0) {
    if (pInst->Status.IsPresent) {
      // Replay
      _CleanJournal(pInst);
    }
	}
  return pInst->Status.OpenCnt;
}

/*********************************************************************
*
*       FS_JOURNAL_Delete
*
*  Function description:
*    Deletes the journal.
*    Typically called before formatting a medium
*
*  Return value:
*    == 0             - O.K.
*    != 0             - Error
*/
void FS_JOURNAL_Delete(FS_VOLUME * pVolume, U32 LastSectorInFS) {
  FS_SB  sb;
  INSt * pInst;
  U8   * pData;
  U8     Unit;

  pInst = _Volume2Inst(pVolume);
  FS__SB_Create(&sb, &pVolume->Partition);
  pData = sb.pBuffer;
  Unit    = pVolume->Partition.Device.Data.Unit;
  //
  // Overwrite info sector
  //
  * pData = 0;
  if (pVolume->Partition.Device.pType->pfWrite(Unit, LastSectorInFS, pData, 1, 0)) {    // Write the info sector (last sector of partition)
    FS_DEBUG_ERROROUT("Failed to invalidate journal");
  }
  //
  // Update Inst structure
  //
  pInst->Status.SectorCnt = 0;
  pInst->Status.IsPresent = 0;
  pVolume->Partition.Device.Data.JournalIsActive = 0;
  FS__SB_Delete(&sb);
}

/*********************************************************************
*
*       FS_JOURNAL_Invalidate
*
*  Function description:
*    Invalidates the journal.
*    Typically called when formatting a medium to avoid replaying of the journal
*
*/
void FS_JOURNAL_Invalidate(FS_VOLUME * pVolume) {
  INSt * pInst;

  pInst = _Volume2Inst(pVolume);
  //
  //  Invalidate all status information in the instance structure.
  //
  _InitStatus(pInst);
   pVolume->Partition.Device.Data.JournalIsActive = 0;
}

/*********************************************************************
*
*       FS_JOURNAL_IsPresent
*
*  Function description:
*    Returns if a journal is present and active.
*
*/
int FS_JOURNAL_IsPresent(FS_VOLUME * pVolume) {
  INSt * pInst;

  pInst = _Volume2Inst(pVolume);
  return pInst->Status.IsPresent;
}

/*********************************************************************
*
*       FS_JOURNAL_GetNumFreeSectors
*
*  Function description
*    Returns
*
*/
int FS_JOURNAL_GetNumFreeSectors(FS_VOLUME * pVolume) {
  INSt * pInst;

  pInst = _Volume2Inst(pVolume);
  return pInst->Status.NumSectorsData - pInst->Status.SectorCnt;
}

/*********************************************************************
*
*       FS_JOURNAL_Read
*
*  Function description
*    Reads one or multiple sectors from journal
*
*/
int FS_JOURNAL_Read(const FS_DEVICE * pDevice, U32 SectorNo, void * pBuffer, U32 NumSectors) {
  INSt * pInst;

  pInst = _apInst[pDevice->Data.JournalUnit];
  return _Read(pInst, SectorNo, pBuffer, NumSectors);
}

/*********************************************************************
*
*       FS_JOURNAL_Write
*
*  Function description
*    Write one or multiple sectors to journal
*
*/
int FS_JOURNAL_Write(const FS_DEVICE * pDevice, U32 SectorNo, const void * pBuffer, U32 NumSectors, U8 RepeatSame) {
  INSt * pInst;

  pInst = _apInst[pDevice->Data.JournalUnit];
  return _Write(pInst, SectorNo, pBuffer, NumSectors, RepeatSame);
}


/*********************************************************************
*
*       FS_CreateJournal
*
*  Function description:
*    Creates the journal file.
*
*  Return value:
*    0    O.K., Successfully created
*    1    Journal file already present
*   <0    Error
*/
int FS_CreateJournal(const char * sVolume, U32 NumBytes) {
  int  r;
  FS_VOLUME * pVolume;
  U32 FirstSector;
  U32 NumSectors;

  r = -1;          // Error - Failed to get volume information
  FS_LOCK();
  if (sVolume) {
    pVolume = FS__FindVolume(sVolume, NULL);
    if (pVolume) {
      if (FS__AutoMount(pVolume) == FS_MOUNT_RW)  {
        if (FS_JOURNAL_IsPresent(pVolume) == 0) {
          FS_LOCK_DRIVER(&pVolume->Partition.Device);
          r = FS_CREATE_JOURNAL_FILE(pVolume, NumBytes, &FirstSector, &NumSectors);
          if (r == 0) {
            FS_JOURNAL_Create(pVolume,  FirstSector, NumSectors);
          }
          FS_UNLOCK_DRIVER(&pVolume->Partition.Device);
        }
      }
    }
  }
  FS_UNLOCK();
  return r;
}



/*************************** End of file ****************************/
