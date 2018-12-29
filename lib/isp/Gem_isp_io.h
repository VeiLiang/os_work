// =============================================================================
// File        : Gem_isp_io.h
// Version     : v1.0
// Author      : Honglei Zhu
// Date        : 2014.12.5
// -----------------------------------------------------------------------------
// Description :  
//
// -----------------------------------------------------------------------------
#ifndef  _GEM_ISP_IO_H_
#define  _GEM_ISP_IO_H_

#include "ark1960.h"

void Gem_write (unsigned int reg_base, unsigned int data); 
unsigned int Gem_read (unsigned int reg_base);

void Gem_io_write (unsigned int reg_base, unsigned int data); 
unsigned int Gem_io_read (unsigned int reg_base);

 
#define GEM_ISP_BASE_ADDR    ISP_BASE //0x70090000)   
#define GEM_ISP_REGS_SIZE  (0x400) 

#define Gem_write(reg_base,data) 	(*(volatile unsigned int *)(GEM_ISP_BASE_ADDR+reg_base)) = data
#define Gem_read(reg_base) 			(*(volatile unsigned int *)(GEM_ISP_BASE_ADDR+reg_base))



#endif