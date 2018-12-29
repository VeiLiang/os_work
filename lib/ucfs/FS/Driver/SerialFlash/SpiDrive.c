/*********************************************************************
File        : SpiDrive.c
Purpose     : Device Driver for Serial Flash
---------------------------END-OF-HEADER------------------------------
*/

#include "FS.h"
#include "ssi.h"
#include "printk.h"
#include "resource.h"

#ifdef FS_SPI_MAXUNITS
  #define NUM_UNITS          FS_SPI_MAXUNITS
#else
  #define NUM_UNITS          1
#endif

static int _NumUnits;

static int ExecuteCommand(unsigned long* cmd, unsigned long* status);

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
  return "spi";
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
	int state, nOffset;

	nOffset = FAV_ADDR;
	
	#ifdef SPI_WRITE_CACHE_EN
	state= (int)spi_read_cache(nOffset+SectorNo*SPI_SECTOR_SIZE, (unsigned char *)pBuffer, NumSectors);
	#else
	state= (int)Spi_Read(nOffset+SectorNo*SPI_SECTOR_SIZE, (unsigned char *)pBuffer, SPI_SECTOR_SIZE*NumSectors);
	#endif

	if(state <= 0)
	{
		printk("spi multi read fail.................\n");
  		return -1;
	}
	return (0);
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
	int state, nOffset;

	nOffset = FAV_ADDR;
	#ifdef SPI_WRITE_CACHE_EN
	state= (int)spi_block_write(nOffset+SectorNo*SPI_SECTOR_SIZE, (unsigned char *)pBuffer, NumSectors);
	#else
	state= (int)Spi_Write(nOffset+SectorNo*SPI_SECTOR_SIZE, (unsigned char *)pBuffer, SPI_SECTOR_SIZE*NumSectors);
	#endif
	if(state <= 0)
	{
		printk("spi multi write fail.................\n");
  		return -1;
	}
	 return (0);
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

	switch (Cmd) {
		case FS_CMD_UNMOUNT:
			break;

		case FS_CMD_GET_DEVINFO: /* Get general device information */
			pDevInfo = (FS_DEV_INFO *)pBuffer;
			pDevInfo->BytesPerSector = 512;
			pDevInfo->NumSectors     = 2176;
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
static int _GetStatus(U8 Unit) {
  return FS_MEDIA_IS_PRESENT;
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
const FS_DEVICE_TYPE FS_SPIFLASH_Driver = {
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
static int FS_SPIFLASH_ExecuteCommand(unsigned long* cmd, unsigned long* status)
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
