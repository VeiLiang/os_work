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
File        : FS_OS_Interface.c
Purpose     : File system OS interface
---------------------------END-OF-HEADER------------------------------
*/

/*********************************************************************
*
*       #include Section
*
**********************************************************************
*/

#include <stdlib.h>

#include "FS_Int.h"
#include "FS_OS.h"
#include "FS_Lbl.h"
#include "FS_CLib.h"

#if ((FS_OS) && (FS_OS_LOCK_PER_DRIVER))              /* One lock per driver */


/*********************************************************************
*
*       Typedefs
*
**********************************************************************
*/
typedef struct DRIVER_LOCK DRIVER_LOCK;


struct DRIVER_LOCK  {
  DRIVER_LOCK          * pNext;
  U8                     Id;
  const FS_DEVICE_TYPE * pDriver;
  U8                     References;
};

/*********************************************************************
*
*       Static data
*
**********************************************************************
*/
static int           _NumDriverLocks;
static DRIVER_LOCK * _pDriverLock;

/*********************************************************************
*
*       Static code
*
**********************************************************************
*/
/*********************************************************************
*
*       _AddDriver
*
* Function description
*   Adds a driver to the lock list.
*   If the driver is already in the list, its reference count is incremented;
*   if not, a memory block is allocated and added to the lock list as last element.
*/
static void _AddDriver(const FS_DEVICE_TYPE * pDriver) {
  DRIVER_LOCK * pDriverLock;
  DRIVER_LOCK ** ppPrev;

  pDriverLock = _pDriverLock;
  ppPrev       = &_pDriverLock;
  do {
    if (pDriverLock == NULL) {
      pDriverLock = (DRIVER_LOCK * )FS_AllocZeroed(sizeof(DRIVER_LOCK));
      pDriverLock->Id       = _NumDriverLocks++;
      pDriverLock->pDriver  = pDriver;
      pDriverLock->References++;
      *ppPrev = pDriverLock;
      break;
    }

    if (pDriverLock->pDriver == pDriver) {
      pDriverLock->References++;
      break;
    }
    ppPrev      = &pDriverLock->pNext;
    pDriverLock = pDriverLock->pNext;

  } while (1);

}


/*********************************************************************
*
*       _Driver2Id
*
* Function description
*   Retrieves the lock Id of the device driver.
*   The lock Id is unique for every device driver.
*/
static unsigned _Driver2Id(const FS_DEVICE_TYPE * pDriver) {
  DRIVER_LOCK * pDriverLock;

  pDriverLock = _pDriverLock;
  do {
    if (pDriverLock->pDriver == pDriver) {
      return pDriverLock->Id;
    }
    pDriverLock = pDriverLock->pNext;
  }  while (pDriverLock);
  FS_DEBUG_ERROROUT("_Driver2Id: Driver was not in driver lock list.");
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
*       FS_OS_AddDriver
*
*/
void FS_OS_AddDriver(const FS_DEVICE_TYPE * pDriver) {
  _AddDriver(pDriver);
}


/*********************************************************************
*
*       FS_OS_LockDriver
*
*/
void  FS_OS_LockDriver(const FS_DEVICE * pDevice) {
  unsigned int LockIndex;

  LockIndex = 0;
  if (pDevice) {
    LockIndex = _Driver2Id(pDevice->pType);
  }
  LockIndex += FS_LOCK_ID_DEVICE;
  FS_X_OS_Lock(LockIndex);
}

/*********************************************************************
*
*       FS_OS_UnlockDriver
*
*/
void  FS_OS_UnlockDriver(const FS_DEVICE * pDevice) {
  unsigned int LockIndex;

  LockIndex = 0;
  if (pDevice) {
    LockIndex = _Driver2Id(pDevice->pType);
  }
  LockIndex += FS_LOCK_ID_DEVICE;
  FS_X_OS_Unlock(LockIndex);
}

/*********************************************************************
*
*       FS_OS_GetNumDriverLocks
*
*/
unsigned FS_OS_GetNumDriverLocks(void) {
  return _NumDriverLocks;
}

#endif
/*************************** End of file ****************************/
