#ifndef _SPI_H
#define _SPI_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SPI_READ_FAST	1
#if  SPI_READ_FAST
#define   SPI_READ_FAST_DUAL   0   
#endif 


typedef struct
{	
	unsigned char Id;
	char * Mf_Name;
}tSPI_Mf_ID;

typedef struct
{
	unsigned char Mf_ID;
	unsigned char Type;
	unsigned char Capacity;
	unsigned char DeviceID;
	unsigned int ChipSize;
	unsigned int SectorSize;   // Bottom
	unsigned int BlockSize;    // Top
	char * Part;
}tSPI_DevInfo;


   
//#define SPI_WRITE_DMA  
//#define SPI_READ_DMA   
//#define SPI_WRITEZERO_DMA  
	
	
#define SPI_SECTOR_SIZE  512


INT32 init_SPI(void);

UINT8 SpiReadSta(void);
void SpiWriteSta(UINT8 data );

UINT32 Spi_GetSize(void);

INT8 *Spi_GetPart(void);

INT32 Spi_Write(UINT32 Dst, UINT8 *buffer, UINT32 byteNum);

INT32 Spi_Read(UINT32 src, UINT8 *buffer, UINT32 byteNum);

void Spi_ChipErase(UINT32 ulChip);

void Spi_Int_init (UINT32 IntType );

#ifdef __cplusplus
}
#endif

#endif

