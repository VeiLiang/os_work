/**********************************************************************
File        : ConfigDrive.c
Purpose     : Configuration file for FS with Flash Drive
---------------------------END-OF-HEADER------------------------------
*/

#include <stdio.h>
#include "hardware.h"
#include "FS.h"
#include "printk.h"
#include <xm_type.h>
#include <xm_base.h>
#include "xm_printf.h"

/*********************************************************************
*
*       Defines, configurable
*
*       This section is the only section which requires changes
*       using the RAM disk driver as a single device.
*
**********************************************************************
*/
#if SDMMC_DEV_COUNT > 1
#define ALLOC_SIZE                 (4*1024)   // Size defined in bytes
#else
//#define ALLOC_SIZE                 (2*1024)   // Size defined in bytes
#define ALLOC_SIZE                 (4*1024)   // Size defined in bytes
#endif

#define RAMDISK_NUM_SECTORS           384      //
#define RAMDISK_BYTES_PER_SECTOR      512      //

#define FS_WD_DEV0NAME    "\\\\.\\A:"          // Windows drive name for "win:0:"

/*********************************************************************
*
*       Static data
*
**********************************************************************
*/
#pragma data_alignment=32
static U32   _aMemBlock[ALLOC_SIZE / 4];      // Memory pool used for semi-dynamic allocation in FS_X_Alloc().
       U32   FS_NumBytesAllocated;            // Public for diagnostic purposes only. Allows user to check how much memory was really needed.

/*********************************************************************
*
*       Public code
*
*       This section does not require modifications in most systems.
*
**********************************************************************
*/

/*********************************************************************
*
*       FS_X_AddDevices
*
*  Function description
*    This function is called by the FS during FS_Init().
*    It is supposed to add all devices, using primarily FS_AddDevice().
*
*  Note
*    (1) Other API functions
*        Other API functions may NOT be called, since this function is called
*        during initialisation. The devices are not yet ready at this point.
*/
void FS_X_AddDevices(void)
{

//  void  * pRamDisk;

  //
  // Allocate memory for the RAM disk
  //
  //pRamDisk = FS_Alloc(RAMDISK_NUM_SECTORS * RAMDISK_BYTES_PER_SECTOR);
  //
  // Add driver
  //
  //FS_AddDevice(&FS_RAMDISK_Driver);
  //
  // Configure driver
  //
  //FS_RAMDISK_Configure(0, pRamDisk, RAMDISK_BYTES_PER_SECTOR, RAMDISK_NUM_SECTORS);

  //
  // Add driver
  //
//  FS_AddDevice(&FS_SPIFLASH_Driver);

  //
  // Add driver
  //
  FS_AddDevice(&FS_SDMMC_Driver);
#if SDMMC_DEV_COUNT > 1
  FS_AddDevice(&FS_SDMMC1_Driver);
#endif
//  FS_AddDevice(&FS_SDMMC2_Driver);
  //
  // Add driver
  //
  
#ifdef _XMSYS_FS_UDISK_SUPPORT_	 
	FS_AddDevice(&FS_MUSB_driver);
#endif
  //FS_AddDevice(&FS_USBDISK_Driver);
  
  //
  //Add driver
  //
//  FS_AddDevice(&FS_NF_Driver);

  //
  //Add driver
  //
//  FS_AddDevice(&FS_NF_Driver);
  
#ifdef PC_VER
  //
  // Add driver
  //
  FS_AddDevice(&FS_WINDRIVE_Driver);
  //
  // Configure driver
  //
  WINDRIVE_Configure(0, FS_WD_DEV0NAME);
#endif
}

/*********************************************************************
*
*       FS_X_GetTimeDate
*
*  Description:
*    Current time and date in a format suitable for the file system.
*
*    Bit 0-4:   2-second count (0-29)
*    Bit 5-10:  Minutes (0-59)
*    Bit 11-15: Hours (0-23)
*    Bit 16-20: Day of month (1-31)
*    Bit 21-24: Month of year (1-12)
*    Bit 25-31: Count of years from 1980 (0-127)
*
*/
U32 FS_X_GetTimeDate(void) {
  U32 r;
  U16 Sec, Min, Hour;
  U16 Day, Month, Year;
  XMSYSTEMTIME Time;

  XM_GetLocalTime (&Time);
  // 1980 based. Means that 2007 would be 27.
  Year = (U16)(Time.wYear - 1980);
  Month = Time.bMonth;
  Day = Time.bDay;
  Hour = Time.bHour;
  Min = Time.bMinute;
  Sec = Time.bSecond;
/*
  Sec   = 0;        // 0 based.  Valid range: 0..59
  Min   = 0;        // 0 based.  Valid range: 0..59
  Hour  = 0;        // 0 based.  Valid range: 0..23
  Day   = 1;        // 1 based.    Means that 1 is 1. Valid range is 1..31 (depending on month)
  Month = 1;        // 1 based.    Means that January is 1. Valid range is 1..12.
  Year  = 0;        // 1980 based. Means that 2007 would be 27.
  */
  r   = Sec / 2 + (Min << 5) + (Hour  << 11);
  r  |= (Day + (Month << 5) + (Year  << 9)) << 16;
  return r;
}

/*********************************************************************
*
*      Logging: OS dependent

Note:
  Logging is used in higher debug levels only. The typical target
  build does not use logging and does therefore not require any of
  the logging routines below. For a release build without logging
  the routines below may be eliminated to save some space.
  (If the linker is not function aware and eliminates unreferenced
  functions automatically)

*/
void FS_X_Log(const char *s) {
  //printk("%s\r\n", s);
	XM_printf ("%s\r\n", s);
}

void FS_X_Warn(const char *s) {
  FS_X_Log(s);
}

void FS_X_ErrorOut(const char *s) {
  FS_X_Log( s);
}

/*********************************************************************
*
*       FS_X_Alloc
*
*  Function description
*    Semi-dynamic memory allocation.
*    This fucntion is called during FS_Init() to allocate memory required
*    for the different components of the file system.
*    Since in a typical embedded application this process is not reversed,
*    there is no counterpart such as "Free()", which helps us to keep the
*    allocation function very simple and associated memory overhead small.
*/
void * FS_X_Alloc(U32 NumBytes) {
  void * p;

  //NumBytes = (NumBytes + 3) & ~3;     // Round upwards to a multiple of 4 (memory is managed in 32-bit units)
  NumBytes = (NumBytes + 0x1F) & ~0x1F;     // Round upwards to a multiple of 32 (memory is managed in cache line aligned)
  if (NumBytes + FS_NumBytesAllocated > ALLOC_SIZE) {
    return NULL;                      // Out of memory. Fatal error caught in caller.
  }
  p = &_aMemBlock[FS_NumBytesAllocated >> 2];
  FS_NumBytesAllocated += NumBytes;
  return p;
}

/*************************** End of file ****************************/
