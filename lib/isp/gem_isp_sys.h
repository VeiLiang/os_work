// =============================================================================
// File        : Gem_isp_sys.h
// Version     : v1.0
// Author      : Honglei Zhu
// Date        : 2014.12.5
// -----------------------------------------------------------------------------
// Description :
//
// -----------------------------------------------------------------------------
#ifndef  _GEM_ISP_SYS_H_
#define  _GEM_ISP_SYS_H_

#include "Gem_isp.h"

#define	USE_ISR_TO_SWITCH_MODE

#define GEM_SYS_BASE  			(0x000)
#define GEM_MSK_BASE  			(0x05c)


// 新的ERIS直方图版本
#define GEM_STS_BASE  (0x1a8 + 0x0c + 0x0c)	// 新增3个寄存器

#define GEM_VIFRASE_BASE		(0x6d * 4)		// 输出帧数

typedef struct isp_sys_
{
   unsigned char ispenbale;  // 1 bit
   unsigned char ckpolar;    // 1 bit
   unsigned char vcpolar;    // 1 bit
   unsigned char hcpolar;    // 1 bit
   unsigned char vmskenable; // 1 bit, 保留, 必须设置为0
   unsigned char frameratei; // 8 bit
   unsigned char framerateo; // 8 bit, 保留, 必须设置为0
	
	unsigned int vifrasel0;	 // 32bit, 输出帧序列控制, 最大可定义64帧
	unsigned int vifrasel1;
	
	unsigned char resizebit;	  // bit16-bit17, Raw data resize (0:normal 1:cut 1bit 2: cut 2 bit 3: cut 3 bit)
   unsigned char sensorbit;   // 2 bit
   unsigned char bayermode;   // 2 bit
   unsigned char sonyif; 
   unsigned int sonysac1;
   unsigned int sonysac2;
   unsigned int sonysac3;
   unsigned int sonysac4;
   unsigned int imagewidth; // 16 bit
   unsigned int imageheight; // 16 bit
   unsigned int imagehblank; // 16 bit
   unsigned int zonestridex; // 16 bit
   unsigned int zonestridey;  // 16 bit
   unsigned char vmanSftenable; 
   unsigned char vchkIntenable;
   unsigned char pabtIntenable;
   unsigned char fendIntenable;
   unsigned char fabtIntenable;
   unsigned char babtIntenable;
   unsigned char ffiqIntenable;
   unsigned char pendIntenable;
	unsigned char infoIntenable;		// "曝光状态信息已完成(表示可以读取AE状态信息)"中断允许位, 
												//	ISP内部的曝光统计已完成, 外部程序此时可以正确获取AE信息, 执行下一次曝光处理

   unsigned char vmanSftset;      
   unsigned char vchkIntclr;
   unsigned char pabtIntclr;
   unsigned char fendIntset;
   unsigned char fendIntclr;   
   unsigned char fabtIntclr;
   unsigned char babtIntclr;
   unsigned char ffiqIntclr;
   unsigned char pendIntclr;
   unsigned char infoStaclr;  			// 清除ISP的曝光统计完成标识  

   unsigned char vchkIntraw;
   unsigned char pabtIntraw;
   unsigned char fendIntraw;
   unsigned char fabtIntraw;
   unsigned char babtIntraw;
   unsigned char ffiqIntraw;
   unsigned char pendIntraw;
	unsigned char infoIntraw;		
   
   unsigned char vchkIntmsk;
   unsigned char pabtIntmsk;
   unsigned char fendIntmsk;
   unsigned char fabtIntmsk;
   unsigned char babtIntmsk;
   unsigned char ffiqIntmsk;
   unsigned char pendIntmsk;
	unsigned char infoIntmsk;
   unsigned char preserve0;
   unsigned char preserve1;
   
   unsigned char fendIntid[4];
   unsigned int ffiqIntdelay;
   unsigned char fendStaid;
   unsigned char infoStadone;
   
   unsigned char debugmode;  	 // 1 bit, 0 关闭调试模式,     1 开启调试模式
   unsigned char testenable;   // 1 bit, 0 关闭dram测试模式, 1 开启dram测试模式
   unsigned char rawmenable;   // 1 bit, 0: 关闭RAW数据输出  1: 开启RAW数据输出 
   unsigned char yuvenable;    // 1 bit, 0: 关闭YUV数据输出  1: 开启YUV数据输出
   unsigned char refenable;    // 1 bit，关闭3D降噪时，应同时disable参考帧， 否则ISP会继续读写参考帧。
   unsigned char yuvformat;  	 // 2 bit
   unsigned char dmalock;      // 2 bit, 锁定会导致H264 codec时间增加
										 //		2:        使能,  存取效率高
										 //		其他数值: 关闭
   unsigned int hstride;       // 16 bit
   unsigned int refaddr;       // 32 bit
   unsigned int rawaddr0;  // 32 bit
   unsigned int rawaddr1;  // 32 bit
   unsigned int rawaddr2;  // 32 bit
   unsigned int rawaddr3;  // 32 bit
   unsigned int yaddr0;    // 32 bit
   unsigned int yaddr1;    // 32 bit
   unsigned int yaddr2;    // 32 bit
   unsigned int yaddr3;    // 32 bit
   unsigned int uaddr0;    // 32 bit
   unsigned int uaddr1;    // 32 bit
   unsigned int uaddr2;    // 32 bit
   unsigned int uaddr3;    // 32 bit
   unsigned int vaddr0;    // 32 bit
   unsigned int vaddr1;    // 32 bit
   unsigned int vaddr2;    // 32 bit
   unsigned int vaddr3;    // 32 bit
   
   unsigned int frameno;    // 32 bit
	
	unsigned int isp_reset_request;		// ISP异常，需要复位操作
} isp_sys_t;

typedef struct isp_sys_ *isp_sys_ptr_t;

void isp_sys_init (isp_sys_ptr_t p_sys, isp_param_ptr_t p_isp);

void isp_sys_init_io (isp_sys_ptr_t p_sys);

unsigned int isp_sys_status_read(isp_sys_ptr_t p_sys);

unsigned int get_isp_framebufferno(void);


unsigned int return_isp_frame();

void Reset_isp_frame();

void isp_sys_infomask_clr (void);

void isp_disable (void);

void isp_enable (void);

void isp_dump_sys_register (void);
#endif