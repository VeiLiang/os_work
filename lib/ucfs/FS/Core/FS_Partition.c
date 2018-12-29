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
File        : API_Dev.c
Purpose     : Device and media tools
---------------------------END-OF-HEADER------------------------------
*/

/*********************************************************************
*
*       #include Section
*
**********************************************************************
*/

#include "FS_ConfDefaults.h"        /* FS Configuration */
#include "FS_CLib.h"
#include "FS_Lbl.h"
#include "FS_Int.h"

/*********************************************************************
*
*       #defines non-configurable
*
**********************************************************************
*/

/*
*   Partition table information
*/
#define PART_OFF_PARTITION0         0x01BE     /* Offset of start of partition table   */
#define SIZEOF_PARTITIONENTRY       0x10       /* Size of one entry in partition table */
#define PARTENTRY_OFF_TYPE          0x04
#define PARTENTRY_OFF_STARTSECTOR   0x08
#define PARTENTRY_OFF_NUMSECTORS    0x0C

/*********************************************************************
*
*       Static code
*
**********************************************************************
*/

/*********************************************************************
*
*       _GetPartitionNumSectors
*
*  Description:
*     Function to get the number of sectors of the partition
*
*  Parameters:
*     PartIndex          - The partition index. Valid range is 0..3.
*                          Since this is an internal function, this parameter
*                          is not checked for validity
*     pBuffer            - Data buffer
*
*  Return value:
*     <  0xFFFFFFFF      - The number of sectors
*     == 0xFFFFFFFF      - Invalid partition specified
*/
static U32 _GetPartitionNumSectors(U8 PartIndex, U8 *pBuffer) {
  int Off;
  Off  = PART_OFF_PARTITION0 + (PartIndex * SIZEOF_PARTITIONENTRY);
  Off += PARTENTRY_OFF_NUMSECTORS;
  return FS_LoadU32LE(&pBuffer[Off]);
}

/*********************************************************************
*
*       _GetPartitionStartSector
*
*  Description:
*     Function to retrieve the start sector of a given partition.
*
*  Parameters:
*     PartIndex          - The partition index. Valid range is 0..3.
*                          Since this is an internal function, this parameter
*                          is not checked for validity
*     pBuffer            - Data buffer
*
*  Return value:
*     <  0xFFFFFFFF      - The value of the start sector
*     == 0xFFFFFFFF      - Invalid partition specified
*/
static U32 _GetPartitionStartSector(U8 PartIndex, U8 *pBuffer) {
  int Off;
  Off  = PART_OFF_PARTITION0 + (PartIndex * SIZEOF_PARTITIONENTRY);
  Off += PARTENTRY_OFF_STARTSECTOR;
  return FS_LoadU32LE(&pBuffer[Off]);
}

/*********************************************************************
*
*       _GetPartitionType
*
*  Description:
*     Function to read the partition type
*
*  Parameters:
*     PartIndex          - The partition index. Valid range is 0..3.
*                          Since this is an internal function, this parameter
*                          is not checked for validity
*     pBuffer            - Data buffer
*
*  Return value:
*    The partition type
*/
#if 0
static U8 _GetPartitionType(U8 PartIndex, U8 *pBuffer) {
  int Off;
  Off  = PART_OFF_PARTITION0 + (PartIndex * SIZEOF_PARTITIONENTRY);
  Off += PARTENTRY_OFF_TYPE;
  return pBuffer[Off];
}
#endif

/*********************************************************************
*
*       _HasSignature
*
*/
static char _HasSignature(const U8 *pBuffer) {
  U16 Data;
  Data = FS_LoadU16LE(pBuffer + 0x1FE);
  if (Data == 0xAA55) {
    return 1;
  }
  return 0;
}

/*********************************************************************
*
*       _IsBPB
*
*/
static char _IsBPB(const U8 *pBuffer) {
  /*
   *  Check if there is a x86 unconditional jmp instruction,
   *  which indicates that there is a BootParameterBlock
   */
  /* Check for the 1-byte relative jump with opcode 0xe9 */
  if (pBuffer[0] == 0xe9) {
    return 1;
  }
  /* Check for the 2-byte relative jump with opcode 0xeb */
  if ((pBuffer[0] == 0xeb) && (pBuffer[2] == 0x90)) {
    return 1;
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
*       FS__GetMediaStartSec
*
*  Description:
*    Get logical start sector.
*    This function is used by some older drivers and should now be
*    obsolete.
*
*  Parameters:
*    PartIndex       - partintion index
*    pBuffer         - A pointer to a buffer containing the first
*                      sector of the media. This should contain the
*                      master boot record (MBR) or Bios Parameter Block
*                      (BPB) if the device has no partition table.
*
*  Return value:
*    < 0xFFFFFFFF    - Number of the first sector of the medium.
*    ==0xFFFFFFFF    - No valid MBR/BPB found.
*/
U32 FS__GetMediaStartSec(U8 PartIndex, U8 *pBuffer) {
  U32  StartSector;

  /*
   *  Check if the sector data contains a valid signature.
   *  This signature is identical for MBR (master boot record) and BPB.
   *  If this signature is not present, further processing does not make sense. --> Error
   */
  if (_HasSignature(pBuffer) == 0) {
    return 0xFFFFFFFF;     /* Error: This sector is neither MBR nor BPB. */
  }
  /*
   *  Check if there is a x86 unconditional jmp instruction,
   *  which indicates, that there is a BiosParameterBlock
   */
  if (_IsBPB(pBuffer) == 0) {
    StartSector = _GetPartitionStartSector(PartIndex, pBuffer);
  } else {
    StartSector = 0;
  }
  return StartSector;
}


/*********************************************************************
*
*       FS__GetMediaStartSecEx
*
*  Description:
*    Get logical start sector from master boot record
*    or partition table.
*
*  Parameters:
*    pVolume         - pointer to volume.
*    pBuffer         - A pointer to a buffer containing the first
*                      sector of the media. This should contain the
*                      master boot record (MBR) or Bios Parameter Block
*                      (BPB) if the device has no partition table.
*
*  Return value:
*    < 0xFFFFFFF     - Number of the first sector of the medium.
*    ==0xFFFFFFF     - No valid MBR/BPB found.
*/
U32 FS__GetMediaStartSecEx(FS_VOLUME * pVolume, U32 * pNumSectors, U8 *pBuffer) {
  U32      StartSector;
  U32      NumSectors;
  FS_DEVICE * pDevice;

  if ((pVolume == NULL) || (pBuffer == NULL)) {
    return 0xFFFFFFFF;
  }
  NumSectors = 0;
  pDevice    = &pVolume->Partition.Device;
  if (FS_LB_ReadDevice(pDevice, 0, pBuffer, FS_SECTOR_TYPE_DATA) < 0) {
    return 0xFFFFFFFF;     /* Error: Sector read failed */
  }

  if (_HasSignature(pBuffer) == 0) {
    return 0xFFFFFFFF;     /* Error: This sector is neither MBR nor BPB. */
  }
  if (_IsBPB(pBuffer) == 0) {
    FS_DEV_INFO DevInfo;
    U32         NumTotalSectorsOfPartition;
    /* Seems to not be a valid BPB.
     * We now assume that it is a boot sector which contains a valid partition
     * table.
     */

    StartSector = _GetPartitionStartSector(0, pBuffer);
    NumSectors  = _GetPartitionNumSectors(0, pBuffer);
    FS_LB_GetDeviceInfo(pDevice, &DevInfo);
    if ((NumSectors == 0) || (StartSector == 0)) {
      return 0xFFFFFFFF;  /* Error, partition table entry 0 is not valid */
    }
    //
    // Allow a tolerance of 0.4% in order of having a larger partition
    // than are reported by device.
    //
    NumTotalSectorsOfPartition = ((StartSector + NumSectors) >> 8) / 255;
    if (NumTotalSectorsOfPartition > DevInfo.NumSectors) {
      FS_DEBUG_WARN("Warning: Partition 0's size is greater than the reported size by the volume");
      return 0xFFFFFFFF;  /* Error, partition table entry 0 is out of bounds */
    }
  } else {
    StartSector = 0;
  }
  if (pNumSectors) {
    *pNumSectors = NumSectors;
  }
  return StartSector;
}

/*************************** End of file ****************************/
