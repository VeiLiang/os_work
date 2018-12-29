/*********************************************************************
File        : MMC_SD_Drive.c
Purpose     : Device Driver for MMC SD CARD
---------------------------END-OF-HEADER------------------------------
*/

#include "types.h"
#include "ark1660.h"
#include "sdmmc.h"
#include "FS.h"
#include "fs_card.h"
#include "fs_port.h"
#include "printk.h"

#ifdef FS_SDMMC_MAXUNITS
  #define NUM_UNITS          FS_SDMMC_MAXUNITS
#else
  #define NUM_UNITS          1
#endif

static int _NumUnits;

//static int ExecuteCommand(unsigned long* cmd, unsigned long* status);

#if SD_DEBUG
	#define DEBUG printk
#else
	#define DEBUG
#endif

//extern int flush_cache(void);//xd.c

volatile FS_CARD SDCard2={
	0, //Present = 0,
	1, //Changed = 1,
	0  //SecNum  = 0
};

static FS_u32          _FS_sd_logicalstart[NUM_UNITS];     /* start of partition */
static unsigned char     _FS_sd_mbrbuffer[0x200];                /* buffer for reading MBR */
//static signed char     _FS_xd_diskchange[FS_SDMMC_MAXUNIT];       /* signal flag for driver */
static unsigned char     _FS_sd_busycnt[NUM_UNITS];          /* counter for BSY LED on/off */

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
  return "sd&mmc2";
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
	int ret;

	if (Unit >= NUM_UNITS)
	{
		return -1;  /* No valid unit number */
	}

//	delay(880);	// 2008.11.7,Carl added for the slow one --> SanDisk 64M

	ret = SD_ReadSector(sdmmc_chip2, Unit, _FS_sd_logicalstart[Unit]+SectorNo, pBuffer, NumSectors);

	if (ret != 0)
	{
		ret = -1;
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
	int ret;

	if (Unit >= NUM_UNITS)
	{
		return -1;  /* No valid unit number */
	}

	delay(10);

 	ret= SD_WriteSector(sdmmc_chip2, Unit, _FS_sd_logicalstart[Unit]+SectorNo, (UINT8*)pBuffer, NumSectors, RepeatSame);

	if (ret != 0)
	{
		ret = -1;
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
  char *pBuf = pBuffer;
  unsigned int reg = 0;
  int i, j;
  
	switch (Cmd) {
	case FS_CMD_UNMOUNT:
		break;

	case FS_CMD_GET_DEVINFO: /* Get general device information */
		pDevInfo = (FS_DEV_INFO *)pBuffer;
		pDevInfo->BytesPerSector = 512;
		pDevInfo->NumSectors     = SDCard2.SecNum;
			break;

	case FS_CMD_GET_SDMMC_CID:
//		printk ("_IoCtl FS_CMD_GET_SDMMC_CID\n");

		if(!SDCard2.Present)   //card not in socket
		{ 
			DEBUG("card is not in the socket\n");
			return FS_MEDIA_NOT_PRESENT;
		}
		if (Unit >= NUM_UNITS)
		{
			return -1;  /* No valid unit number */
		}

		for(i = 0; i < 4; i++)
		{
			for(j = 0; j < 4; j++)
			{
				reg = SDCard2.REG_CID[i] >> 8*j;	
				*pBuf = reg;
				pBuf++;
			}
		}
		
		break;

	default:
		return -1;
	}

	return 0;
}

/*********************************************************************
*
*       _MMC_InitMedium
*
*  Description:
*    Initialize the card.
*
*  Parameters:
*    Unit        - Unit number.
*
*  Return value:
*    == 0                       - Device okay and ready for operation.
*    <  0                       - An error has occured.
*/
static int _InitMedium(U8 Unit){
  return 0;
}
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
	int x;

	DEBUG ("_FS_SD_DevStatus2\n");

	if(!SDCard2.Present)   //card not in socket
	{ 
		DEBUG("card is not in the socket\n");
		return FS_MEDIA_NOT_PRESENT;
	}
	if (Unit >= NUM_UNITS)
	{
		return -1;  /* No valid unit number */
	}
	/* card changed */
	if(SDCard2.Changed)
	{
		printk("sd card Changed!!!\n");

		_FS_sd_logicalstart[Unit]=0;
		SDCard2.Changed = 0; //reset the media changed flag

		delay(10);

		x = SD_ReadSector(sdmmc_chip2, Unit, 0, _FS_sd_mbrbuffer, 1);
	//	for(i=0;i<512;i++)
	//	{
	//		printk(" buf[%x]=%x",i,_FS_sd_mbrbuffer[i]);	
	//	}

		if (x != 0)
		{
			printk("\nSD_ReadSector err \n");
			return -1;
		}

		if(( _FS_sd_mbrbuffer[0]&0xff)==0xeb || (_FS_sd_mbrbuffer[0]&0xff)==0xe9 )
		{

		}
		else if((_FS_sd_mbrbuffer[510]&0xff)==0x55 && (_FS_sd_mbrbuffer[511]&0xff)==0xaa)
		{
			_FS_sd_logicalstart[Unit]  = (_FS_sd_mbrbuffer[0x1c6]&0xff);
			_FS_sd_logicalstart[Unit] += (0x100UL * (_FS_sd_mbrbuffer[0x1c7]&0xff));
			_FS_sd_logicalstart[Unit] += (0x10000UL * (_FS_sd_mbrbuffer[0x1c8]&0xff));
			_FS_sd_logicalstart[Unit] += (0x1000000UL *( _FS_sd_mbrbuffer[0x1c9]&0xff));
		}
		return FS_MEDIA_IS_PRESENT;
	}
  return FS_MEDIA_STATE_UNKNOWN;
}
/*********************************************************************
*
*       _GetNumUnits
*/
static int _GetNumUnits(void) {
  return _NumUnits;
}
/*********************************************************************
*
*       Public data
*
**********************************************************************
*/
const FS_DEVICE_TYPE FS_SDMMC2_Driver = {
  _GetDriverName,
  _AddDevice,
  _Read,
  _Write,
  _IoCtl,
  _InitMedium,
  _GetStatus,
  _GetNumUnits
};
/*********************************************************************
*
*       Utilits
*
**********************************************************************
*/
// Execute a flash firmware command.
static int FS_MMC_SD_ExecuteCommand(unsigned long* cmd, unsigned long* status)
{
	int ret = 0;

	return ret;
}
/*********************************************************************
*
*       End
*
**********************************************************************
*/
