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
File        : FS_Misc.c
Purpose     : Misc. API functions
---------------------------END-OF-HEADER------------------------------
*/

/*********************************************************************
*
*        #include Section
*
**********************************************************************
*/

#include "FS_Int.h"
#include <stdio.h>

/*********************************************************************
*
*        Local data types
*
**********************************************************************
*/

typedef struct {
  const char * Mode;
  U8 AccessFlags;
  U8 DoDel;
  U8 DoOpen;
  U8 DoCreate;
} _FS_MODE_TYPE;

#define	printk	printf

/*********************************************************************
*
*       Static data
*
**********************************************************************
*/

/*********************************************************************
*
*       Static const
*
**********************************************************************
*/

static const _FS_MODE_TYPE _aAccessMode[] = {
                                         /* DEL  OPEN  CREATE  READ  WRITE  APPEND  CREATE  BINARY */
  { "r"   ,  FS_FILE_ACCESS_FLAG_R,         0,   1,    0},
  { "rb"  ,  FS_FILE_ACCESS_FLAGS_BR,       0,   1,    0}, /* 1,    0,     0,       0,     1 */
  { "w"   ,  FS_FILE_ACCESS_FLAGS_CW,       1,   0,    1}, /* 0,    1,     0,       1,     0 */
  { "wb"  ,  FS_FILE_ACCESS_FLAGS_BCW,      1,   0,    1}, /* 0,    1,     0,       1,     1 */
  { "a"   ,  FS_FILE_ACCESS_FLAGS_ACW,      0,   1,    1}, /* 0,    1,     1,       1,     0 */
  { "ab"  ,  FS_FILE_ACCESS_FLAGS_ABCW,     0,   1,    1}, /* 0,    1,     1,       1,     1 */
  { "r+"  ,  FS_FILE_ACCESS_FLAGS_RW,       0,   1,    0}, /* 1,    1,     0,       0,     0 */
  { "r+b" ,  FS_FILE_ACCESS_FLAGS_BRW,      0,   1,    0}, /* 1,    1,     0,       0,     1 */
  { "rb+" ,  FS_FILE_ACCESS_FLAGS_BRW,      0,   1,    0}, /* 1,    1,     0,       0,     1 */
  { "w+"  ,  FS_FILE_ACCESS_FLAGS_CRW,      1,   0,    1}, /* 1,    1,     0,       1,     0 */
  { "w+b" ,  FS_FILE_ACCESS_FLAGS_BCRW,     1,   0,    1}, /* 1,    1,     0,       1,     1 */
  { "wb+" ,  FS_FILE_ACCESS_FLAGS_BCRW,     1,   0,    1}, /* 1,    1,     0,       1,     1 */
  { "a+"  ,  FS_FILE_ACCESS_FLAGS_ACRW,     0,   1,    1}, /* 1,    1,     1,       1,     0 */
  { "a+b" ,  FS_FILE_ACCESS_FLAGS_ABCRW,    0,   1,    1}, /* 1,    1,     1,       1,     1 */
  { "ab+" ,  FS_FILE_ACCESS_FLAGS_ABCRW,    0,   1,    1}  /* 1,    1,     1,       1,     1 */
};

/*********************************************************************
*
*       Static code
*
**********************************************************************
*/

/*********************************************************************
*
*       _CalcSizeInKB
*
*  Function description
*     Given the numbers of clusters, sectors per cluster and bytes per secor,
*     calculate the equivalent number of kilo bytes.
*
*  Parameters:
*     NumClusters         - The Number of sectors
*     SectorsPerCluster   - The number of sectors in a cluster
*     BytesPerSector      - The number of bytes in a sector
*
*  Return value:
*     The number of kilo bytes (KB).
*/
static U32 _CalcSizeInKB(U32 NumClusters, U32 SectorsPerCluster, U32 BytesPerSector) {
  U32 BytesPerCluster;
  int NumShifts;

  BytesPerCluster = SectorsPerCluster * BytesPerSector;
  NumShifts = 10;
  do {
    if (BytesPerCluster == 1) {
      break;
    }
    BytesPerCluster >>= 1;
  } while (--NumShifts);
  return BytesPerCluster * (NumClusters >> NumShifts);
}


/*********************************************************************
*
*       _Text2Mode
*
*  Function description
*    Converts the "open-mode-string" into flags using a table.
*/
static int _Text2Mode(const char * sMode) {
  unsigned j;
  for (j = 0; j < COUNTOF(_aAccessMode); j++) { /* Go through whole list */
    if (FS_STRCMP(sMode, _aAccessMode[j].Mode) == 0) {
      return j;
    }
  }
  return -1;       /* Not a valid access mode */
}


/*********************************************************************
*
*       Public data
*
**********************************************************************
*/
FS_FILE     FS__aFilehandle[FS_NUM_FILE_HANDLES];
FS_FILE_OBJ FS__aFileObj   [FS_NUM_FILE_OBJECTS];
/*********************************************************************
*
*       Public code
*
**********************************************************************
*/

/*********************************************************************
*
*       Public code, internal functions
*
**********************************************************************
*/

/*********************************************************************
*
*       FS__AllocFileHandle
*
*  Function description:
*    Returns a free file handle.
*
*  Return value:
*    pFile      - A valid free file handle
*
*/
FS_FILE * FS__AllocFileHandle(void) {
  FS_FILE * pFile;
  unsigned int  i;
  pFile = (FS_FILE*)NULL;
  /*  Find next free entry in array */
  FS_LOCK_SYS();
  for (i =0; i < COUNTOF(FS__aFilehandle); i++) { /* While no free entry found. */
    if (FS__aFilehandle[i].InUse == 0) {
      pFile = &FS__aFilehandle[i];
      FS_MEMSET(pFile, 0, sizeof(FS_FILE));
      pFile->InUse = 1;
      break;
    }
  }
  FS_UNLOCK_SYS();
  return pFile;
}

/*********************************************************************
*
*       FS__FreeFileHandle
*
*  Function description:
*    Closes the file handle and mark it as free.
*
*  Parameters:
*    pFile       - Pointer to an opened file handle.
*
*/
void FS__FreeFileHandle(FS_FILE * pFile) {
  if (pFile) {
    FS_LOCK_SYS();
    pFile->InUse = 0;
    pFile->pFileObj = (FS_FILE_OBJ*)NULL;
    FS_UNLOCK_SYS();
  }
}


/*********************************************************************
*
*       FS__AllocFileObj
*
*  Function description:
*    Returns a free file handle.
*
*  Parameter(s)
*    sFullFileName    Points to a full file name. May NOT be NULL.
*
*  Return value:
*    pFileObj   - A valid free file Obj
*
*/
FS_FILE_OBJ * FS__AllocFileObj(const char * sFullFileName) {
  FS_FILE_OBJ * pFileObj;
  unsigned int  i;

  pFileObj = (FS_FILE_OBJ*)NULL;
  /*  Find next free entry */
  FS_LOCK_SYS();
  for (i =0; i < COUNTOF(FS__aFileObj); i++) { /* While no free entry found. */
    if (FS__aFileObj[i].UseCnt == 0) {
      pFileObj = &FS__aFileObj[i];
      FS_MEMSET(pFileObj, 0, sizeof(FS_FILE_OBJ));
      pFileObj->UseCnt++;
#if FS_MULTI_HANDLE_SAFE
      FS_STRCPY(pFileObj->acFullFileName, sFullFileName);
#else
      FS_USE_PARA(sFullFileName);
#endif
      break;
    }
  }
  FS_UNLOCK_SYS();
  return pFileObj;
}

/*********************************************************************
*
*       FS__GetFileObj
*
*  Function description:
*    Returns a free file handle.
*
*  Return value:
*    pFileObj     - A valid file obj
*
*/
FS_FILE_OBJ * FS__GetFileObj(const char * sFullFileName) {
  FS_FILE_OBJ * pFileObj;
#if FS_MULTI_HANDLE_SAFE
  unsigned int  i;

  pFileObj = (FS_FILE_OBJ*)NULL;
  /*  Find next free entry */
  FS_LOCK_SYS();
  for (i =0; i < COUNTOF(FS__aFileObj); i++) { /* While no free entry found. */
    if (FS_STRCMP(sFullFileName, FS__aFileObj[i].acFullFileName) == 0) {
      pFileObj = &FS__aFileObj[i];
      pFileObj->UseCnt++;
      break;
    }
  }
  FS_UNLOCK_SYS();
#else
  FS_USE_PARA(sFullFileName);
  pFileObj = (FS_FILE_OBJ *)NULL;
#endif
  return pFileObj;
}

/*********************************************************************
*
*       FS__FreeFileObj
*
*  Function description:
*    Closes the file object.
*
*  Parameters:
*    pFile       - Pointer to an open file obj.
*
*/
void FS__FreeFileObj(FS_FILE_OBJ * pFileObj) {
  if (pFileObj) {
    FS_LOCK_SYS();
    if (pFileObj->UseCnt) {
      pFileObj->UseCnt--;
    }
#if FS_MULTI_HANDLE_SAFE
    if (pFileObj->UseCnt == 0) {
      //
      // Empty string when this file object is not used anymore.
      //
      pFileObj->acFullFileName[0] = '\0';
    }
#endif
    FS_UNLOCK_SYS();
  }
}

/*********************************************************************
*
*       _BuildFullFileName
*
*  Function description
*    Stores the full filename (including volume and path) into the destination
*    Buffer
*
*  Return value:
*    0      O.K.
*    1
*/
#if FS_MULTI_HANDLE_SAFE
static int _BuildFullFileName(FS_VOLUME * pVolume, const char * sFileName, char * sDest) {
  int Len;
  int Len1;
  const char * sDriverName;
  FS_DEVICE  * pDevice;

  if (pVolume == (FS_VOLUME *)NULL) {
    return 1;
  }

  pDevice = &pVolume->Partition.Device;
  sDriverName = pDevice->pType->pfGetName(pDevice->Data.Unit);
  Len = FS_STRLEN(sDriverName);
  FS_STRCPY(sDest, sDriverName);
  *(sDest + Len++) = ':';
  *(sDest + Len++) = '0' + pDevice->Data.Unit;
  *(sDest + Len++) = ':';
  Len1 = FS_STRLEN(sFileName);
  if ((Len1 + Len) >= FS_MAX_LEN_FULL_FILE_NAME) {
    FS_DEBUG_ERROROUT("_BuildFullFileName: sFileName is too long to store in sDest.");
    return 1;   /* Error: file name is too long to store in sDest */
  }
  if (*sFileName != FS_DIRECTORY_DELIMITER) {
    *(sDest + Len++) = FS_DIRECTORY_DELIMITER;
  }
  FS_STRCPY((sDest + Len), sFileName);
  return 0;
}
#endif

/*********************************************************************
*
*       FS__FTell
*
*  Function description:
*    Internal version if FS_FTell
*    Return position of a file pointer.
*
*  Parameters:
*    pFile         - Pointer to a FS_FILE data structure.
*
*  Return value:
*    >=0           - Current position of the file pointer.
*    ==-1          - An error has occured.
*/
I32 FS__FTell(FS_FILE *pFile) {
  I32 r;
  r = -1;
  if (pFile) {
    FS_LOCK_SYS();
    r =  pFile->FilePos;
    FS_UNLOCK_SYS();
  }
  return r;
}

/*********************************************************************
*
*       FS__FCloseNL
*
*  Function description:
*    Internal version of FS_FClose.
*    Close a file referred by pFile.
*
*  Parameters:
*    pFile       - Pointer to a FS_FILE data structure.
*
*  Return value:
*    1           - Error, File handle can not be closed.
*    0           - File handle has been closed.
*/
int FS__FCloseNL(FS_FILE *pFile) {
  if (pFile->InUse) {
    FS_FILE_OBJ * pFileObj;

    pFileObj = pFile->pFileObj;
    FS_CLOSE_FILE(pFile);  /* Execute the FSL function */
    FS__FreeFileObj(pFileObj);
    FS__FreeFileHandle(pFile);
  }
  return 0;
}

/*********************************************************************
*
*       FS__FClose
*
*  Function description:
*    Internal version of FS_FClose.
*    Close a file referred by pFile.
*
*  Parameters:
*    pFile       - Pointer to a FS_FILE data structure.
*
*  Return value:
*    1           - Error, File handle can not be closed.
*    0           - File handle has been closed.
*/
int FS__FClose(FS_FILE *pFile) {
  FS_FILE_OBJ * pFileObj;
  FS_DEVICE   * pDevice;
  FS_VOLUME   * pVolume;
  char          InUse;
  int           r;

  r = 1;
  pVolume = (FS_VOLUME *)NULL;
  pDevice = (FS_DEVICE *)NULL;
  FS_LOCK_SYS();
  InUse = pFile->InUse;
  pFileObj = pFile->pFileObj;
  if (pFileObj) {
    pVolume  = pFileObj->pVolume;
  }
  if (pVolume) {
    pDevice  = &pVolume->Partition.Device;
  }
  FS_UNLOCK_SYS();
  if ((InUse == 0) || (pVolume == NULL)) {
    return 1;
  }
  FS_USE_PARA(pDevice);
  FS_USE_PARA(pVolume);
  FS_LOCK_DRIVER(pDevice);
#if FS_OS_LOCK_PER_DRIVER
  FS_LOCK_SYS();
  if (pFileObj != pFile->pFileObj) {
    InUse = 0;
  }
  if (pFile->InUse == 0) {
    InUse = 0;
  }
  FS_UNLOCK_SYS();
  if (InUse == 0) {      // Let's make sure the file is still valid
    FS_DEBUG_ERROROUT("Application error: File handle has been invalidated by other thread during wait");
    FS_UNLOCK_DRIVER(pDevice);
    return 1;

  } else
#endif
  {
    FS_JOURNAL_BEGIN (pVolume);
    FS__FCloseNL(pFile);
    FS_JOURNAL_END (pVolume);
  }
  FS_UNLOCK_DRIVER(pDevice);
  r = 0;
  return r;
}


/*********************************************************************
*
*       _FSeekNL
*
*  Function description:
*    Internal version of FS_FSeek
*    Set current position of a file pointer.
*
*  Parameters:
*    pFile       - Pointer to a FS_FILE data structure.
*    Offset      - Offset for setting the file pointer position.
*    Origin      - Mode for positioning the file pointer.
*
*  Return value:
*    ==0         - File pointer has been positioned according to the
*                  parameters.
*    ==-1        - An error has occured.
*/
static int _FSeekNL(FS_FILE *pFile, I32 Offset, int Origin) {
  U32  uOffset;

  uOffset = (U32)Offset;
  if (pFile == NULL) {
    return -1;
  }
  switch (Origin) {
  case FS_SEEK_SET:
    break;
  case FS_SEEK_CUR:
    uOffset += pFile->FilePos;
    break;
  case FS_SEEK_END:
    uOffset += pFile->pFileObj->Size;
    break;
  default:
    pFile->Error = FS_ERR_INVALIDPAR;
    FS_DEBUG_WARN("FS__FSeek: Illegal parameter");
    return -1;
  }
  if (pFile->FilePos != uOffset) {
    pFile->FilePos  = uOffset;
    pFile->Error    = FS_ERR_OK;    /* Clear any previous error */
  }
  return 0;
}

/*********************************************************************
*
*       FS__FSeek
*
*  Function description:
*    Internal version of FS_FSeek
*    Set current position of a file pointer.
*
*  Parameters:
*    pFile       - Pointer to a FS_FILE data structure.
*    Offset      - Offset for setting the file pointer position.
*    Origin      - Mode for positioning the file pointer.
*
*  Return value:
*    ==0         - File pointer has been positioned according to the
*                  parameters.
*    ==-1        - An error has occured.
*/
int FS__FSeek(FS_FILE *pFile, I32 Offset, int Origin) {
  int r;

  FS_LOCK_SYS();
  r = _FSeekNL(pFile, Offset, Origin);
  FS_UNLOCK_SYS();
  return r;
}

/*********************************************************************
*
*       FS__IoCtlNL
*
*  Function description:
*    Internal version of FS_IoCtl.
*    Execute device command.
*
*  Parameters:
*    pVolume     - Pointer to the device structure.
*    Cmd         - Command to be executed.
*    Aux         - Parameter depending on command.
*    pBuffer     - Pointer to a buffer used for the command.
*
*  Return value:
*    Command specific. In general a negative value means an error.
*/
int FS__IoCtlNL(FS_VOLUME * pVolume, I32 Cmd, I32 Aux, void *pBuffer) {
  int Status;

  switch (Cmd) {
  case FS_CMD_FORMAT_LOW_LEVEL:
  case FS_CMD_REQUIRES_FORMAT:
  case FS_CMD_FREE_SECTORS:
  case FS_CMD_GET_DEVINFO:
  case FS_CMD_SET_DELAY:
    break;
  case FS_CMD_UNMOUNT_FORCED:
  case FS_CMD_UNMOUNT:
    return FS_IOCTL(&pVolume->Partition.Device, Cmd, Aux, pBuffer);
  default:
    if (FS__AutoMountNL(pVolume)) {
      return -1;
    }
  }
  Status = FS_LB_GetStatus(&pVolume->Partition.Device);
  if (Status >= 0) {
    FS_LB_InitMediumIfRequired(&pVolume->Partition.Device);
    return FS_IOCTL(&pVolume->Partition.Device, Cmd, Aux, pBuffer);
  }
  return -1;
}

/*********************************************************************
*
*       FS__IoCtl
*
*  Function description:
*    Internal version of FS_IoCtl.
*    Execute device command.
*
*  Parameters:
*    pVolume     - Pointer to the specified volume.
*    Cmd         - Command to be executed.
*    Aux         - Parameter depending on command.
*    pBuffer     - Pointer to a buffer used for the command.
*
*  Return value:
*    Command specific. In general a negative value means an error.
*/
int FS__IoCtl(FS_VOLUME * pVolume, I32 Cmd, I32 Aux, void *pBuffer) {
  int          r;
  r = -1;
  if (pVolume) {
    FS_LOCK_DRIVER(&pVolume->Partition.Device);
    r = FS__IoCtlNL(pVolume, Cmd, Aux, pBuffer);
    FS_UNLOCK_DRIVER(&pVolume->Partition.Device);
  }
  return r;
}


/*********************************************************************
*
*       FS__CalcSizeInBytes
*
*  Function description
*     Given the numbers of clusters, sectors per cluster and bytes per secor,
*     calculate the equivalent number of bytes.
*
*  Parameters:
*     NumClusters         - The Number of sectors
*     SectorsPerCluster   - The number of sectors in a cluster
*     BytesPerSector      - The number of bytes in a sector
*
*  Return value:
*     The number of bytes, or if the number would exceed the range U32
*     can represent, return 0xFFFFFFFF
*/
U32 FS__CalcSizeInBytes(U32 NumClusters, U32 SectorsPerCluster, U32 BytesPerSector) {
  if (_CalcSizeInKB(NumClusters, SectorsPerCluster, BytesPerSector) < 0x400000) {
    return NumClusters * SectorsPerCluster * BytesPerSector;
  } else {
    return 0xFFFFFFFF;    // Max. value of U32. The size in bytes does not fit into a U32.
  }
}

/*********************************************************************
*
*       FS__Remove
*
*  Function description:
*    Internal version of FS_Remove
*    Removes a file.
*    There is no real 'delete' function in the FSL, but the FSL's 'open'
*    function can delete a file.
*
*  Parameters:
*    pFileName   - Fully qualified file name.
*
*  Return value:
*    ==0         - File has been removed.
*    ==-1        - An error has occured.
*/
int FS__Remove(const char *pFileName) {
  int r;
  FS_FILE * pFile;
  r = -1;

  pFile = FS__FOpenEx(pFileName, FS_FILE_ACCESS_FLAG_W, 0, 1, 0);
  if (pFile) {
    FS__FreeFileObj(pFile->pFileObj);
    FS__FreeFileHandle(pFile);
    r = 0;
  }
  return r;
}


/*********************************************************************
*
*       FS__OpenEx
*
*  Function description:
*    Either opens an existing file or create a new one or delete
*    a file existing file.
*
*  Parameters:
*    pVolume     - Pointer to a volume structure.
*    sFilePath   - String to the relative path of the file.
*    AccessFlags - Type of Access.
*    DoCreate    - Shall the file be created.
*    DoDel       - Shall the existing file be deleted.
*    DoOpen      - Shall the file be opened.
*
*  Return value:
*    ==0         - Unable to open the file.
*    !=0         - Address of an FS_FILE data structure.
*/
FS_FILE * FS__OpenEx(FS_VOLUME * pVolume, const char * sFilePath, U8 AccessFlags, char DoCreate, char DoDel, char DoOpen) {
  FS_FILE      * pFile;
  FS_FILE_OBJ  * pFileObj;
  int            r;

  pFile    = NULL;
  pFileObj = NULL;
  /* Find correct FSL  (device:unit:name) */
  //
  // Allocate file object.
  // The procedure depends. If multiple handles per file are allowed, we first need to check if
  // the file is already open.
  //
#if FS_MULTI_HANDLE_SAFE
  {
    char ac[FS_MAX_LEN_FULL_FILE_NAME];
    if (_BuildFullFileName(pVolume, sFilePath, ac)) {
      goto Error;
    }
    /* Find file obj (if the file is already open), else alloc one */
    pFileObj = FS__GetFileObj(ac);
    if ((void*)pFileObj == NULL) {
      pFileObj = FS__AllocFileObj(ac);
    }
  }
#else
  pFileObj = FS__AllocFileObj(NULL);
#endif
  if ((void *)pFileObj == NULL) {
    FS_DEBUG_ERROROUT("FS_FOpen(): No file object available");
    goto Error;
  }
  //
  // Allocate file handle.
  //
  pFile    = FS__AllocFileHandle();
  if ((void*)pFile == NULL) {
    FS_DEBUG_ERROROUT("FS_FOpen(): No file handle available");
    goto Error;
  }
  //
  // Write information to the file handle.
  //
  FS_LOCK_SYS();
  pFile->AccessFlags = AccessFlags;
  pFile->pFileObj    = pFileObj;
  pFileObj->pVolume  = pVolume;
  FS_UNLOCK_SYS();
  FS_JOURNAL_BEGIN (pVolume);
  r = FS_OPEN_FILE(sFilePath, pFile, DoDel, DoOpen, DoCreate);
  FS_JOURNAL_END (pVolume);
  if (r) {
    goto Error;        /* Illegal access flags */
  }
  goto Done;
Error:
  FS__FreeFileHandle(pFile);
  pFile = (FS_FILE*)NULL;
  FS__FreeFileObj(pFileObj);
Done:
  return pFile;
}

/*********************************************************************
*
*       FS__FOpenEx
*
*  Function description:
*    Either opens an existing file or create a new one or delete
*    a file existing file.
*
*  Parameters:
*    sFullFileName - Fully qualified file name.
*    AccessFlags   - Type of Access.
*    DoCreate      - Shall the file be created.
*    DoDel         - Shall the existing file be deleted.
*    DoOpen        - Shall the file be opened.
*
*  Return value:
*    ==0         - Unable to open the file.
*    !=0         - Address of an FS_FILE data structure.
*/
FS_FILE * FS__FOpenEx(const char * sFullFileName, U8 AccessFlags, char DoCreate, char DoDel, char DoOpen) {
  FS_FILE      * pFile;
  const char   * sFileName;
  FS_VOLUME    * pVolume;
  int            r;

  pVolume = FS__FindVolume(sFullFileName, &sFileName);
  
  if ((void*)pVolume == NULL) {
  	printk("FS__FindVolume is NULL\r\n");
    return NULL;
  }
  r = FS__AutoMount(pVolume);
  if (r <= 0) {
    FS_DEBUG_ERROROUT("FS__OpenEx: Volume can not be mounted");
    return (FS_FILE *)NULL;
  }
  //
  //  Check if we want to write data to the volume and the device is mounted read-only.
  //
  if ((AccessFlags & FS_FILE_ACCESS_FLAGS_ACW) && (r != FS_MOUNT_RW)) {
    FS_DEBUG_ERROROUT("FS__OpenEx: Volume is mounted read-only, cannot either create file nor write/append to file.");
    return NULL;
  }
  pFile = NULL;
  FS_LOCK_DRIVER(&pVolume->Partition.Device);
#if FS_OS_LOCK_PER_DRIVER
  if (pVolume->IsMounted == 0) {
    FS_DEBUG_ERROROUT("Application error: Volume has been unmounted by other thread during wait");
  } else
#endif
  {
    pFile = FS__OpenEx(pVolume, sFileName, AccessFlags, DoCreate, DoDel, DoOpen);
  }
  FS_UNLOCK_DRIVER(&pVolume->Partition.Device);
  return pFile;
}

/*********************************************************************
*
*       FS__FOpen
*
*  Function description:
*    Internal version of FS_FOpen.
*    Open an existing file or create a new one.
*
*  Parameters:
*    pFileName   - Fully qualified file name.
*    pMode       - Mode for opening the file.
*
*  Return value:
*    ==0         - Unable to open the file.
*    !=0         - Address of an FS_FILE data structure.
*/
FS_FILE * FS__FOpen(const char * pFileName, const char * pMode) {
  FS_FILE      * pFile;
  int            ModeIndex;
  U8          AccessFlags;
  char           DoCreate;
  char           DoDel;
  char           DoOpen;

  pFile = NULL;
  /* Check mode */
  ModeIndex = _Text2Mode(pMode);
  if (ModeIndex < 0) {
    FS_DEBUG_ERROROUT("FS_FOpen(): Illegal access flags.");
  } else {
    /* All checks have been performed, lets do the work  */
    AccessFlags        = _aAccessMode[ModeIndex].AccessFlags;
    DoDel              = _aAccessMode[ModeIndex].DoDel;
    DoOpen             = _aAccessMode[ModeIndex].DoOpen;
    DoCreate           = _aAccessMode[ModeIndex].DoCreate;
    pFile = FS__FOpenEx(pFileName, AccessFlags, DoCreate, DoDel, DoOpen);
  }
  return pFile;
}

/*********************************************************************
*
*       Public code, API functions
*
**********************************************************************
*/

/*********************************************************************
*
*       FS_FOpen
*
*  Function description:
*    Open an existing file or create a new one.
*
*  Parameters:
*    pFileName   - Fully qualified file name.
*    pMode       - Mode for opening the file.
*
*  Return value:
*    ==0         - Unable to open the file.
*    !=0         - Address of an FS_FILE data structure.
*/
FS_FILE * FS_FOpen(const char * pFileName, const char * pMode) {
  FS_FILE      * pFile;

  FS_LOCK();
  pFile = FS__FOpen(pFileName, pMode);
  FS_UNLOCK();
  return pFile;
}



/*********************************************************************
*
*       FS_Remove
*
*  Function description:
*    Remove a file.
*    There is no real 'delete' function in the FSL, but the FSL's 'open'
*    function can delete a file.
*
*  Parameters:
*    pFileName   - Fully qualified file name.
*
*  Return value:
*    ==0         - File has been removed.
*    ==-1        - An error has occured.
*/
int FS_Remove(const char *sFileName) {
  int r;

  FS_LOCK();
  r = FS__Remove(sFileName);
  FS_UNLOCK();
  return r;
}


/*********************************************************************
*
*       FS_FClose
*
*  Function description:
*    Close a file referred by pFile.
*
*  Parameters:
*    pFile       - Pointer to a FS_FILE data structure.
*
*  Return value:
*    1           - Error, File handle can not be closed.
*    0           - File handle has been closed.
*/
int FS_FClose(FS_FILE *pFile) {
  int r;

  FS_LOCK();
  r  = 1;
  if (pFile) {
    r = FS__FClose(pFile);
  }
  FS_UNLOCK();
  return r;
}



/*********************************************************************
*
*       FS_IoCtl
*
*  Function description:
*    Execute device command.
*
*  Parameters:
*    pDevName    - Fully qualified directory name.
*    Cmd         - Command to be executed.
*    Aux         - Parameter depending on command.
*    pBuffer     - Pointer to a buffer used for the command.
*
*  Return value:
*    Command specific. In general a negative value means an error.
*/
int FS_IoCtl(const char *pDevName, I32 Cmd, I32 Aux, void *pBuffer) {
  const char * s;
  int r;
  FS_VOLUME  * pVolume;

  FS_LOCK();
  pVolume = FS__FindVolume(pDevName, &s);
  r = FS__IoCtl(pVolume, Cmd, Aux, pBuffer);
  FS_UNLOCK();
  return r;
}


/*********************************************************************
*
*       FS_BeginTransaction
*/
void FS_BeginTransaction(const char *sVolume) {
  FS_VOLUME  * pVolume;

  FS_LOCK();
  pVolume = FS__FindVolume(sVolume, NULL);
  FS_JOURNAL_BEGIN(pVolume);
  FS_UNLOCK();
}

/*********************************************************************
*
*       FS_EndTransaction
*/
void FS_EndTransaction(const char *sVolume) {
  FS_VOLUME  * pVolume;

  FS_LOCK();
  pVolume = FS__FindVolume(sVolume, NULL);
  FS_JOURNAL_END(pVolume);
  FS_UNLOCK();
}



/*********************************************************************
*
*       FS_FSeek
*
*  Function description:
*    Set current position of a file pointer.
*
*  Parameters:
*    pFile       - Pointer to a FS_FILE data structure.
*    Offset      - Offset for setting the file pointer position.
*    Origin      - Mode for positioning the file pointer.b
*
*  Return value:
*    ==0         - File pointer has been positioned according to the
*                  parameters.
*    ==-1        - An error has occured.
*/
int FS_FSeek(FS_FILE * pFile, I32 Offset, int Origin) {
  int r;
  FS_LOCK();
  r = FS__FSeek(pFile, Offset, Origin);
  FS_UNLOCK();
  return r;
}

/*********************************************************************
*
*       FS_FTell
*
*  Function description:
*    Return position of a file pointer.
*
*  Parameters:
*    pFile         - Pointer to a FS_FILE data structure.
*
*  Return value:
*    >=0           - Current position of the file pointer.
*    ==-1          - An error has occured.
*/
I32 FS_FTell(FS_FILE *pFile) {
  I32 r;
  FS_LOCK();
  r = FS__FTell(pFile);
  FS_UNLOCK();
  return r;
}

/*********************************************************************
*
*       FS_FEof
*
*  Function description:
*    Returns if end of file has been reached.
*
*  Parameters:
*    pFile         - Pointer to a FS_FILE data structure.
*
*  Return value:
*    == 1          - End of File has been reached.
*    == 0          - End of File has not been reached.
*/
int FS_FEof(FS_FILE * pFile) {
  int r;
  char InUse;
  FS_LOCK();
  r = 1;
  if (pFile) {

    FS_LOCK_SYS();
    InUse = pFile->InUse;
    FS_UNLOCK_SYS();
    if (InUse) {
      r = (pFile->FilePos >= pFile->pFileObj->Size);
    } else {
      r = -1;
    }
  }
  FS_UNLOCK();
  return r;
}

/*********************************************************************
*
*       FS_FError
*
*  Function description:
*    Return error status of a file.
*
*  Parameters:
*    pFile         - Pointer to a FS_FILE data structure.
*
*  Return value:
*    == FS_ERR_OK  - No error.
*    != FS_ERR_OK  - An error has occured.
*/
I16 FS_FError(FS_FILE * pFile) {
  I16 r;

  FS_LOCK();
  r = FS_ERR_INVALIDPAR;
  if (pFile) {
    r = pFile->Error;
  }
  FS_UNLOCK();
  return r;
}


/*********************************************************************
*
*       FS_ClearErr
*
*  Function description:
*    API function. Clear error status of a file.
*
*  Parameters:
*    pFile       - Pointer to a FS_FILE data structure.
*
*  Return value:
*    None.
*/
void FS_ClearErr(FS_FILE * pFile) {
  FS_LOCK();
  if (pFile) {
    pFile->Error = FS_ERR_OK;
  }
  FS_UNLOCK();
}

/*********************************************************************
*
*       FS_GetFreeSpace
*
*  Function description:
*    Returns amount of free space on the drive.
*
*  Parameters:
*    pDevName    - Driver name. e.g. "ram:" or "mmc:"
*    DevIndex    - not used any more
*
*  Return value:
*    Number of bytes available on the drive.
*    If the volume can not be found, 0 is returned.
*
*  Notes
*    (1) Obsolete: This function is obsolete. Use FS_GetVolumeFreeSpace() instead.
*    (2) Max. value:
*        Since the return value is a 32 bit value, the maximum that can
*        be return is 0xFFFFFFFF = 2^32 - 1.
*        If there is more space available than 0xFFFFFFFF, the return value is 0xFFFFFFFF.
*/

U32 FS_GetFreeSpace(const char *pDevName) {
  FS_DISK_INFO Info;
  U32          r;

  FS_LOCK();
  r = 0;
  FS_MEMSET(&Info, 0, sizeof(FS_DISK_INFO));
  if (FS__GetVolumeInfo(pDevName, &Info) != -1) {
    r = FS__CalcSizeInBytes(Info.NumFreeClusters, Info.SectorsPerCluster, Info.BytesPerSector);
  }
  FS_UNLOCK();
  return r;
}

/*********************************************************************
*
*       FS_GetTotalSpace
*
*  Function description:
*    Returns total amount of free space on the drive.
*
*  Parameters:
*    pDevName    - Driver name. e.g. "ram:" or "mmc:"
*    DevIndex    - not used any more
*
*  Return value:
*    Total number of bytes on the drive.
*    If the volume can not be found, 0 is returned.
*
*    (1) Obsolete: This function is obsolete. Use FS_GetVolumeSize() instead.
*    (2) Max. value:
*        Since the return value is a 32 bit value, the maximum that can
*        be return is 0xFFFFFFFF = 2^32 - 1.
*        If there is more space available than 0xFFFFFFFF, the return value is 0xFFFFFFFF.
*/
U32 FS_GetTotalSpace(const char * pDevName) {
  FS_DISK_INFO Info;
  U32          r;

  FS_LOCK();
  r = 0;
  FS_MEMSET(&Info, 0, sizeof(FS_DISK_INFO));
  if (FS__GetVolumeInfo(pDevName, &Info) != -1) {
    r = FS__CalcSizeInBytes(Info.NumTotalClusters, Info.SectorsPerCluster, Info.BytesPerSector);
  }
  FS_UNLOCK();
  return r;
}

/*********************************************************************
*
*       FS_MMC_GetCardId
*
*  Function description:
*    Get SD or MMC card CID register information
*
*    pDevName    - Driver name. e.g. "sd&mmc0:"
*    pCardId	    - Data Stored buffer
*  Return value:
*      0     - O.K.
*     -1     - Device is not ready or general error.
*/

int  FS_MMC_GetCardId(const char * pDevName, MMC_CARD_ID * pCardId){
  int         r;
  FS_VOLUME *pVolume = NULL;
  	
  r =  -1;  // Set as error so far

  FS_LOCK();
  pVolume = FS__FindVolume(pDevName, NULL);
  if (pVolume) {
    FS_DEVICE * pDevice;

    pDevice = &pVolume->Partition.Device;
    FS_LOCK_DRIVER(pDevice);
    r = pDevice->pType->pfIoCtl(pDevice->Data.Unit, FS_CMD_GET_SDMMC_CID, 0, pCardId->aData);
    FS_UNLOCK_DRIVER(pDevice);
  }
  FS_UNLOCK();
  
  return r;
}
/*************************** End of file ****************************/
