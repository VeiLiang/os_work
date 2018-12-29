// =============================================================================
// File        : Gem_isp_io.c
// Version     : v1.0
// Author      : Honglei Zhu
// Date        : 2014.12.5
// -----------------------------------------------------------------------------
// Description :
//
// -----------------------------------------------------------------------------
#include  "ark1960.h"

#define GEM_ISP_BASE_ADDR    ISP_BASE //0x70090000)   
#define GEM_ISP_REGS_SIZE  (0x400) 
/*

void Gem_write (unsigned int reg_base, unsigned int data) 
{
   (*(unsigned int *)(GEM_ISP_BASE_ADDR+reg_base)) = data;
}

unsigned int Gem_read (unsigned int reg_base) 
{
   return (*(unsigned int *)(GEM_ISP_BASE_ADDR+reg_base));
}
*/

void Gem_io_write (unsigned int reg_base, unsigned int data)
{
	(*(unsigned int *)(GEM_ISP_BASE_ADDR+reg_base)) = data;
}

unsigned int Gem_io_read (unsigned int reg_base)
{
	return (*(unsigned int *)(GEM_ISP_BASE_ADDR+reg_base));
}


