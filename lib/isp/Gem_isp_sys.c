// =============================================================================
// File        : Gem_isp_sys.c
// Version     : v1.0
// Author      : Honglei Zhu
// Date        : 2014.12.5
// -----------------------------------------------------------------------------
// Description :
//
// -----------------------------------------------------------------------------
#include "hardware.h"
#include "Gem_isp.h"
#include "Gem_isp_sys.h"
#include "Gem_isp_io.h"
#include "Gem_isp_ae.h"
#include "Gem_isp_denoise.h"

#include "Gem_isp_sensor.h"
#include "Gem_isp_awb.h"
#include <stdio.h>

extern void XMSYS_SensorNotifyDataFrame (int channel, int frame_id);


static unsigned int framebufferno=0;


unsigned int get_isp_framebufferno(void)
{
	return framebufferno;
}



void isp_sys_init_io (isp_sys_ptr_t p_sys)
{
  int i, data0, data1, data2, data3, data4, data5, data6;
  
  // 输出帧控制序列
  // 至少输出一帧, bit0必须设置
  data0 = p_sys->vifrasel0;
  Gem_write ((GEM_VIFRASE_BASE + 0x00), data0);
  data0 = p_sys->vifrasel1;
  Gem_write ((GEM_VIFRASE_BASE + 0x04), data0);

  data0 	= (p_sys->ckpolar << 1)
	  		| (p_sys->vcpolar << 2) 
			| (p_sys->hcpolar << 3) 
			| (p_sys->vmanSftenable << 4) 
			| (p_sys->vmskenable << 5) 
			| (p_sys->frameratei << 8) 
			| (p_sys->framerateo << 16)
			;
          
  data1 = (p_sys->imagewidth) | (p_sys->imageheight<<16);
  //XM_printf("width  :%d\n",p_sys->imagewidth  );
  //XM_printf("height :%d\n",p_sys->imageheight );
  data2 = p_sys->imagehblank 
	  		| ((p_sys->resizebit & 0x03) << 16)
	  		| (p_sys->sensorbit << 18) 
			| (p_sys->bayermode << 20) 
			| (p_sys->sonyif << 22)
			;
  //XM_printf("Sensor bit set:%d \n", p_sys->sensorbit );
  //XM_printf("Sensor bayer set:%d \n", p_sys->bayermode );
  
  //XM_printf("//slice line:%d\n", p_sys->ffiqIntdelay);
  data3 = ((p_sys->vchkIntenable & 0x01) <<  0) 
	  	  | ((p_sys->pabtIntenable & 0x01) <<  1) 
		  | ((p_sys->fendIntenable & 0x01) <<  2) 
		  | ((p_sys->fabtIntenable & 0x01) <<  3) 
		  | ((p_sys->babtIntenable & 0x01) <<  4) 
		  | ((p_sys->ffiqIntenable & 0x01) <<  5) 
		  | ((p_sys->pendIntenable & 0x01) <<  6) 
		  | ((p_sys->infoIntenable & 0x01) <<  7)		// ISP内部的AE信息已统计完成, 外部程序可以获取AE信息, 执行下一次曝光处理	  
			  
		 // | ((0xF << 9))	// 9,  11, 12置为1会使能对应的pabt, fabt, babt异常复位frame sync.
		 // | ((0x1F << 9))	// 9,  11, 12 置为1会使能对应的pabt, fabt, babt异常复位frame sync.
			  					// 10 idle状态复位判定使能/禁止, 
			  					//		0 不判定, 会出现ISP挂起
			  					//		1   判定, 场同步异常保护(清除异常状态, 恢复ISP)	
			  					// 13 使能/禁止idle状态丢帧功能
			  					//		0 禁止
			  					//		1 使能
			  					//	缺省时应将以上位全部设置为1
			| (0xFF << 9)
	  
			| ((p_sys->ffiqIntdelay  & 0xFF) << 24)
			;
          
  data4 = (p_sys->zonestridex) | (p_sys->zonestridey << 16);
  data5 = (p_sys->sonysac1 << 16) | (p_sys->sonysac2 << 0);
  data6 = (p_sys->sonysac3 << 16) | (p_sys->sonysac4 << 0);
  
  Gem_write ((GEM_SYS_BASE+0x00), data0);
  Gem_write ((GEM_SYS_BASE+0x04), data1);
  Gem_write ((GEM_SYS_BASE+0x08), data2);
  Gem_write ((GEM_SYS_BASE+0x0c), data3);
  Gem_write ((GEM_SYS_BASE+0x10), data4); 
  Gem_write ((GEM_SYS_BASE+0x60), data5); 
  Gem_write ((GEM_SYS_BASE+0x64), data6); 
  
  data0 	= (p_sys->debugmode) 
	  		| (p_sys->testenable << 1) 
			| (p_sys->rawmenable << 2) 
			| (p_sys->yuvenable << 3) 
			| (p_sys->refenable << 4) 
			| (p_sys->yuvformat << 5) 
			| (p_sys->dmalock << 7) 
			| (p_sys->hstride << 16)
			;
	Gem_write ((GEM_SYS_BASE+0x14), data0);
   // XM_printf("p_sys->hstride :%d\n", p_sys->hstride );
   // XM_printf("p_sys->yuvformat = %d\n", p_sys->yuvformat );

	data0 = p_sys->refaddr;
	Gem_write ((GEM_SYS_BASE+0x18), data0);

	data0 = p_sys->rawaddr0;
	data1 = p_sys->rawaddr1;
	data2 = p_sys->rawaddr2;
	data3 = p_sys->rawaddr3;
	Gem_write ((GEM_SYS_BASE+0x1c), data0);
	Gem_write ((GEM_SYS_BASE+0x20), data1);
	Gem_write ((GEM_SYS_BASE+0x24), data2);
	Gem_write ((GEM_SYS_BASE+0x28), data3);
  
	data0 = p_sys->yaddr0;
	data1 = p_sys->yaddr1;
	data2 = p_sys->yaddr2;
	data3 = p_sys->yaddr3;
	// YUV 地址  Y
	Gem_write ((GEM_SYS_BASE+0x2c), data0);
	Gem_write ((GEM_SYS_BASE+0x30), data1);
	Gem_write ((GEM_SYS_BASE+0x34), data2);
	Gem_write ((GEM_SYS_BASE+0x38), data3);

	data0 = p_sys->uaddr0;
	data1 = p_sys->uaddr1;
	data2 = p_sys->uaddr2;
	data3 = p_sys->uaddr3;
	Gem_write ((GEM_SYS_BASE+0x3c), data0);
	Gem_write ((GEM_SYS_BASE+0x40), data1);
	Gem_write ((GEM_SYS_BASE+0x44), data2);
	Gem_write ((GEM_SYS_BASE+0x48), data3);

	data0 = p_sys->vaddr0;
	data1 = p_sys->vaddr1;
	data2 = p_sys->vaddr2;
	data3 = p_sys->vaddr3;
	Gem_write ((GEM_SYS_BASE+0x4c), data0);
	Gem_write ((GEM_SYS_BASE+0x50), data1);
	Gem_write ((GEM_SYS_BASE+0x54), data2);
	Gem_write ((GEM_SYS_BASE+0x58), data3);

  	if (p_sys->vmanSftset == 1) 
	{
		// 这个是ID号， 与清除位操作不一样
      // 发出一个脉冲信号
		data0 = (0x00);
		Gem_write ((GEM_MSK_BASE+0x00), data0);
	}

	if (p_sys->vchkIntclr == 1)// 帧头 同步 中断 清除
	{// 
		data0 = (0x01);
		Gem_write ((GEM_MSK_BASE+0x00), data0);
	}

  if (p_sys->pabtIntclr == 1)
  {
    data0 = (0x02);
    Gem_write ((GEM_MSK_BASE+0x00), data0);
  }

  if (p_sys->fendIntset == 1)
  {
    for (i = 0; i < 4; i++)
    {
      data0 = (0x03) | (p_sys->fendIntid[i]<<16);
      Gem_write ((GEM_MSK_BASE+0x00), data0);
    }
  }

  if (p_sys->fendIntclr == 1)
  {
    data0 = (0x04);
    Gem_write ((GEM_MSK_BASE+0x00), data0);
  }

  if (p_sys->fabtIntclr == 1)
  {
    data0 = (0x05);
    Gem_write ((GEM_MSK_BASE+0x00), data0);
  }

  if (p_sys->babtIntclr == 1)
  {
    data0 = (0x06);
    Gem_write ((GEM_MSK_BASE+0x00), data0);
  }

  if (p_sys->infoStaclr == 1)
  {
    data0 = (0x07);
    Gem_write ((GEM_MSK_BASE+0x00), data0);
  }

  if (p_sys->ffiqIntclr == 1)
  {
    data0 = (0x08);
    Gem_write ((GEM_MSK_BASE+0x00), data0);
  }

  if (p_sys->pendIntclr == 1)
  {
    data0 = (0x09);
    Gem_write ((GEM_MSK_BASE+0x00), data0);
  }
  
  if(p_sys->ispenbale)
  {
		//isp_dump_sys_register ();  
  }

  data0 =  Gem_read ((GEM_SYS_BASE+0x00));
  data1 = data0 | p_sys->ispenbale;
  Gem_write ((GEM_SYS_BASE+0x00), data1);

}

#ifdef ISP_DEBUG
void isp_dump_sys_register (void)
{
	int i;
	for (i = 0; i <= 0x64; i += 4)
	{
		XM_printf ("GEM_SYS_BASE+0x%02x = 0x%08x\n", i, Gem_read(GEM_SYS_BASE+i));
	}
}
#endif


unsigned int isp_sys_status_read(isp_sys_ptr_t p_sys)
{
  unsigned int isp_status;
  
  // read mask status 
  isp_status = Gem_read (GEM_STS_BASE+0x00); 
  //isp_status = rISP_INT_STATUS; 
  
  //1. bit.0:帧开始中断，帧头
  p_sys->vchkIntmsk  = isp_status & 0x01;  //(isp_status<<31) >> 31;
  //2. bit.1:pclk buffer 满了，sensor 数据已经，无法处理
  p_sys->pabtIntmsk = (isp_status >> 1) & 0x01; //(isp_status<<30) >> 30;
  
  //3. bit.2:帧完成中断 
  p_sys->fendIntmsk = (isp_status >> 2) & 0x01; //(isp_status<<29) >> 29;
  //4. bit.3:地址异常 ，地址 数据溢出 ，或0
  p_sys->fabtIntmsk = (isp_status >> 3) & 0x01; //(isp_status<<28) >> 28;
  
  //5. bit.4:总线带宽异常，3D降噪数据无法同步，时间赶不上
 p_sys->babtIntmsk = (isp_status >> 4) & 0x01; //(isp_status<<27) >> 27;
  //6. bit.5:16行 为单位的slice中断
  p_sys->ffiqIntmsk = (isp_status >> 5) & 0x01;
  //7. bit.6:中止 中断
  p_sys->pendIntmsk = (isp_status >> 6) & 0x01;
  // 帧序列号，取值：0 1 2 3 
  p_sys->fendStaid = (isp_status >> 8) & 0x03; //(isp_status<<26) >> 26;

  if(!p_sys->infoIntenable)	// 非中断方式(poll)
  {
	  // RAW Interrupt Factor
	  p_sys->infoStadone = ( Gem_read (GEM_STS_BASE+0x04) >> 7) & 0x01;   
  }
  
  //16-23位快中断状态计数器 ，共计8位 ，表示每次要完成的行数，需要乘以16；
  
  // read raw status 
  isp_status = Gem_read (GEM_STS_BASE+0x04);
  
  
  return isp_status;
}

void isp_sys_infosync_clr (void)
{
   unsigned int infosync_clr;
   
   infosync_clr = (0x07);  
   Gem_write ((GEM_MSK_BASE+0x00), infosync_clr);	
   	
}

void isp_sys_set_frame_ready (unsigned int frame_id)
{
	unsigned int frame_set = (0x03) | ((frame_id & 0x03) << 16);
	//XM_printf ("push %d\n", frame_id);
	Gem_write ((GEM_MSK_BASE+0x00), frame_set);
}


extern isp_sys_t p_sys;
extern isp_denoise_t p_denoise;
extern isp_awb_t p_awb;
extern isp_ae_t p_ae;


//#define	UPDATE_AWB_BLACK_USE_ISR

#ifdef USE_ISR_TO_SWITCH_MODE
extern volatile unsigned int new_isp_mode;
#endif

#define	isp_debug_printf	XM_printf
//#define	isp_debug_printf	

extern void isp_histogram_bands_data (unsigned int data1, unsigned int data2);
extern void isp_ae_yavg_s_read (isp_ae_ptr_t p_ae);

#define	ISP_SCALAR_RUN			rISP_SCALE_EN = 0x03

static int do_isp_scalar = 0;

void isp_scalar_run(void)
{
	do_isp_scalar = 1;
}

static int isp_pixel_clock_abnormal_count = 0;

void isp_isr (void)
{
   unsigned int frame_id, frame_set, frame_clr;
	unsigned int int_status ;
	unsigned int line, val;

   int_status  = Gem_read(GEM_STS_BASE+0x00);	

   // bit.0 帧 开始 bit.0:帧开始中断，帧头
	// frame start interrupt
   if(int_status & 0x01 )
   {
		// write 1 at bit[7:0] means frame start sync interrupt clear
      Gem_write ((GEM_MSK_BASE+0x00), 0x01);
   }
   
   // bit.1  点异常  bit.0:帧开始中断，帧头
	// sensor data use pixel clk is full (pixel fast than coreclk), sensor FIFO的输入端压入数据过快, FIFO的输出端取出数据过慢, 导致FIFO溢出. 
	// Pixel Clock出现一个干扰, 相当于输入端压入一个新的数据. 干扰过多, 会导致FIFO溢出而出现 Pixel clock abnormal
   if(int_status & (0x01<<1) )
   {
      // 数据点异常处理  
      // write 2 at bit[7:0] means pixel clock abnormal interrupt clear 
		Gem_write ((GEM_MSK_BASE+0x00), 0x02);
      //isp_debug_printf ("isp pixel clock abnormal\n\n");
		isp_pixel_clock_abnormal_count ++;
		if(isp_pixel_clock_abnormal_count >= 8)
		{
			isp_debug_printf ("isp pixel clock abnormal error, isp disabled\n\n");
			rISP_SYS &= ~1;		// 等待ISP复位
		}
   }
   
   // bit.2:帧完成中断 
	// frame real finish interrupt
   if( int_status & (0x01<<2) ) // frame finish int
	{		
		isp_pixel_clock_abnormal_count = 0;
		
		// 完成的ID
      frame_id = (int_status >> 8) & 0x3;		// frame id number
		framebufferno = frame_id;
		// Clear frame finished status  
		frame_clr = 0x04;
		Gem_write ((GEM_MSK_BASE+0x00), frame_clr);   
		
		
#ifdef USE_ISR_TO_SWITCH_MODE
		if(new_isp_mode != (unsigned int)(-1))
		{
			unsigned int data0;
			
#if ISP_3D_DENOISE_SUPPORT
			if(new_isp_mode == ISP_WORK_MODE_NORMAL)	// 正常模式下开启3D
				p_denoise.enable3d = 7;
			else
				p_denoise.enable3d = 0;
			data0 =  ((p_denoise.enable2d & 0x07) <<  0) 
					| ((p_denoise.enable3d & 0x07) <<  3) 
					| ((p_denoise.sel_3d_table & 0x03) << 8)	// 3D高斯表选择
					| ((p_denoise.sensitiv0 & 0x07) << 10)	// 2D滤波器0(差分)灵敏度设置, 0 滤波关闭	
					| ((p_denoise.sensitiv1 & 0x07) << 13)	// 2D滤波器1(边沿)灵敏度设置, 0 滤波关闭
					| ((p_denoise.sel_3d_matrix & 0x01) << 16)		// 3D相干性矩阵区域选择， 0 邻近 1 中心点
					;
			Gem_write ((GEM_DENOISE_BASE+0x00), data0);
#endif
			//isp_denoise_init_io (&p_denoise);	
			if(new_isp_mode == ISP_WORK_MODE_NORMAL)	// 正常模式下开启3D
			{
				p_sys.debugmode  = 0;
				p_sys.testenable = 0; // 开启dram测试模式  
				p_sys.rawmenable = 0; // 1 允许RAW写出
				p_sys.yuvenable  = 1; // 0:关掉数据输出  1:打开
#if ISP_3D_DENOISE_SUPPORT
				p_sys.refenable  = 1; // 1;3D 参考帧开启 0:关闭 
#else
				p_sys.refenable  = 0; // 1;3D 参考帧开启 0:关闭 
#endif
			}
			else
			{
				// RAW
				p_sys.debugmode  = 1;
				p_sys.testenable = 0; // 开启dram测试模式  
				p_sys.rawmenable = 1; // 1 允许RAW写出
				p_sys.yuvenable  = 1; // 0:关掉数据输出  1:打开
				p_sys.refenable  = 0; // 1;3D 参考帧开启 0:关闭 
			}
			//cmos_sensor->cmos_isp_sys_init (&p_sys, &g_isp_param);
		  	data0 = (p_sys.debugmode) 
					| (p_sys.testenable << 1) 
					| (p_sys.rawmenable << 2) 
					| (p_sys.yuvenable << 3) 
					| (p_sys.refenable << 4) 
					| (p_sys.yuvformat << 5) 
					| (p_sys.dmalock << 7) 
					| (p_sys.hstride << 16)
					;
			Gem_write ((GEM_SYS_BASE+0x14), data0);			
			new_isp_mode = (unsigned int)(-1);
		}
#endif
		
		//XM_printf ("pop %d\n", frame_id);
		XMSYS_SensorNotifyDataFrame (0, frame_id);  
		
		if(do_isp_scalar)
		{
			do_isp_scalar = 0;
			ISP_SCALAR_RUN;
			XM_printf ("scalar run\n");
		}
		
		extern void isp_gamma_adjust(void);
		isp_gamma_adjust();
		
#ifdef ISP_ADJUST_CM_SUPPORT
		extern void isp_cm_adjust (void);
		isp_cm_adjust ();
#endif
	}
	
   // bit.3:地址异常 ，地址 数据溢出 ，或0
	// 与软件帧同步相关, 判断软件帧同步计数是否与硬件帧计数一致. 可以忽略
	// address abnormal interrupt (overflow or zero abnormal)
   if( int_status  & (0x01<<3) )
   {
		// write 5 at bit[7:0] means address abnoraml interrupt clear 
		Gem_write ((GEM_MSK_BASE+0x00), 0x05);
      //isp_debug_printf ("isp address abnormal!\n\n");
		
		//isp_dump_sys_register ();
   }
	
   // bit.4:总线带宽异常，3D降噪数据无法同步，时间赶不上
	// bus bandwidth abnormal (means 3d denoise can't sync ,can't capture time)
   if( int_status  & (0x01<<4) )
   {
		isp_debug_printf ("isp bus bandwidth abnormal!\n\n");
		Gem_write ((GEM_MSK_BASE+0x00),  0x06);
		
		// dma memory to memory 拷贝时, 如果设置bus lock, 会导致总线带宽异常.
		//  因此出现线带宽异常时, 应检查是否存在类似的bus lock
		if(0)
		//if(reset_index > 1)
		{
			//reset_index = 0;
			// write 6 at bit[7:0] means bus transfer error interrrupt clear
			//Gem_write ((GEM_MSK_BASE+0x00),  0x06);
			//rISP_SYS &= ~1;
			
			// ISP的参考帧读会占用总线，导致CPU无法读取指令而挂起。
			// Bus异常时，应首先关闭参考帧，释放总线
			// disable 3D's reference frame
			unsigned int data0 = Gem_read (GEM_SYS_BASE+0x14);
			data0 &= ~(1 << 4);
			Gem_write (GEM_SYS_BASE+0x14, data0);
			
			
			//isp_debug_printf ("bus bandwidth abnormal!!\n\n\n");
			
			// 关闭bus异常中断请求
			unsigned int data3 = Gem_read (GEM_SYS_BASE+0x0c);
			data3 &= ~(1 << 4);
			Gem_write (GEM_SYS_BASE+0x0c, data3);
			
			p_sys.isp_reset_request = 1;		// 请求ISP复位
		}
   }
   
   // bit.5
   // 16行为单位的快中断 fast_int = (isp_status >> 5) & 0x01
	// slice interrupt(use 16 per step)
   if( int_status  & (0x01<<5) )
   {
		// bit16-bit31 means the line number should finish in this fiq interrupt 
      line = (int_status>>16);
		(void)line;
		// write 8 at bit[7:0] means fast interrupt clear
		Gem_write ((GEM_MSK_BASE+0x00), 0x08);
		//isp_debug_printf ("isp slice int=%d!!\n\n\n", line);	
   }
   
   // 挂起中断 bit.6:中止 中断
	// suspend interrupt(means termination)
   if(  int_status  & (0x01<<6) )
   {
      // write 9 at bit[7:0] means suspend interrupt clear
      Gem_write ((GEM_MSK_BASE+0x00), 0x09);
      isp_debug_printf ("isp suspend interrupt!\n\n\n");
   }
	
	// 曝光统计完成中断
	if( int_status & (0x01 << 7) )
	{
		unsigned int data0;
		
		isp_pixel_clock_abnormal_count = 0;
		
		// 检查白平衡参数是否需要更新
#ifdef UPDATE_AWB_BLACK_USE_ISR
		data0 	= ((p_awb.enable  & 0x01) <<  0) 
					| ((p_awb.mode    & 0x03) <<  1) 	// bit1-bit2     mode 
					// 0: unite gray white average 
					//	1: unite color temperature average  
					//	2: zone color temperature Weight
					| ((p_awb.manual  & 0x01) <<  3) 	// bit3  manual, 0: auto awb  1: mannual awb
					| ((p_awb.black   & 0xFF) <<  8) 
					| ((p_awb.white   & 0xFF) << 16)
					;
		Gem_write ((GEM_AWB0_BASE+0x00), data0);
		
#endif
		
		// write 7 at bit[7:0] means frame information sync bit (after ae) clear
		Gem_write ((GEM_MSK_BASE+0x00), 0x07);
		p_sys.infoStadone = 1;
		//isp_debug_printf ("infodone interrupt!\n\n\n");	
		
		data0 = Gem_read (GEM_AE1_BASE+0x04);
		isp_histogram_bands_data (data0, Gem_read (GEM_AE1_BASE+0x08));
		
		isp_ae_yavg_s_read (&p_ae);
		
		isp_ae_done_event_set ();
	}
}

void isp_sys_infomask_clr (void)
{
   unsigned int infomaskclr;

   infomaskclr = (0x07);  
   Gem_write ((GEM_MSK_BASE+0x00), infomaskclr);	

}

void isp_enable (void)
{
	unsigned int data;
	data =  Gem_read ((GEM_SYS_BASE+0x00));
	data |= 0x01;
	Gem_write ((GEM_SYS_BASE+0x00), data);
}

void isp_disable (void)
{
	unsigned int data;
	data =  Gem_read ((GEM_SYS_BASE+0x00));
	data &= ~0x01;
	Gem_write ((GEM_SYS_BASE+0x00), data);
}


