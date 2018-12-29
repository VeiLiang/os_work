/*
	scale.c
*/
#include "hardware.h"
#include "arkn141_scale.h"
#include <xm_type.h>
#include <xm_dev.h>
#include <stdio.h>
#include <assert.h>
#include "dma.h"
#include "xm_core.h"

unsigned int vaddr_to_page_addr (unsigned int addr);



static OS_RSEMA arkn141_scalar_access_semid;	// 互斥访问信号量
static OS_EVENT arkn141_scalar_finish_event;	// 完成事件

static OS_CSEMA SEMAScaleFrame, SEMAScaleMidNum  ;


static unsigned short Y_line,UV_line;
static void ScaleIntHander()
{
	int status = rAXI_SCALE_INT_STATUS;

//	printf("rAXI_SCALE_INT_CTL:0x%08x\r\n",rAXI_SCALE_INT_CTL );
//	printf("In Scale Int mid_num Hander! \n");
//	printf(":%02x\n",status);
//	printf("rAXI_SCALE_INT_CTL:0x%08x\r\n",rAXI_SCALE_INT_CTL );
	
	//if( status & (WB_MIDNUM_FINISH_INT | WB_MIDNUM_FINISH_FLAG) )
	if( status & WB_MIDNUM_FINISH_INT  )
	{
		Y_line = rAXI_SCALE_WR_Y_VCNT&0xfff;
		UV_line =(rAXI_SCALE_WR_Y_VCNT>>11)&0x7ff;
		OS_SignalCSema( &SEMAScaleMidNum ); 
		rAXI_SCALE_CLCD_INT_CLR = (1<<2);
	}
	else if( status & (WB_FRAME_FINISH_INT) )
	{
		OS_SignalCSema( &SEMAScaleFrame ); 
		rAXI_SCALE_CLCD_INT_CLR = (1<<0);
	}	
	
}


void scale_arkn141_set_writeback_addr(unsigned int Yaddr ,unsigned int Uaddr ,unsigned int Vaddr )
{
   rAXI_SCALE_WB_DEST_YADDR = Yaddr;
   rAXI_SCALE_WB_DEST_UADDR = Uaddr;
#if SCALE_FORMAT_YUV420   //支持YUV420   UV数据分开	
   rAXI_SCALE_WB_DEST_VADDR = Vaddr;
#endif
}



void scale_arkn141_writeback_start()
{
   delay(100);
   rAXI_SCALE_WB_START = 0x01;/*一场开始寄存器，当有上升沿触发时开始一场的scale操作及回写操作。*/
   delay(100);
   rAXI_SCALE_WB_START = 0x00;
}

//#pragma inline
void  scale_arkn141_open()
{
	// 创建互斥信号量，保护scalar访问
	OS_CREATERSEMA (&arkn141_scalar_access_semid);
	// 完成事件
	OS_EVENT_Create (&arkn141_scalar_finish_event);
	
	OS_CREATECSEMA( &SEMAScaleFrame );
	OS_CREATECSEMA( &SEMAScaleMidNum );
	rAXI_SCALE_EN		= 0x03;
}

//#pragma inline
void  scale_arkn141_close()
{
	rAXI_SCALE_EN		= 0x0;
	OS_DI();
	irq_disable ( SCALE_INT );
   rAXI_SCALE_CLCD_INT_CLR = 0x07;
   rAXI_SCALE_INT_CTL = 0;
   OS_EI();
	OS_DeleteCSema (&SEMAScaleFrame);
	OS_DeleteCSema (&SEMAScaleMidNum);
	
	
	OS_EVENT_Delete (&arkn141_scalar_finish_event);
	OS_DeleteRSema (&arkn141_scalar_access_semid);
}


//extern void ScaleIntHander();

void Scale_Int_init (int status )
{
	// Scale 中断
	/*
	bit 1:
1	R/W	0	AXI SCALE write back bresp error interrupt mask
0: mask
1: enable
	bit 0:
0	R/W	0	AXI SCALE write back frame finish interupt mask
0: mask
1: enable
		*/
	// 使能SCALE中断
	// 清除SCALE中断
	rAXI_SCALE_INT_CTL = status;
	//rAXI_SCALE_INT_CTL = 0;
	
	rAXI_SCALE_CLCD_INT_CLR = status;
   
	OS_DI();
	irq_disable ( SCALE_INT );
	request_irq(SCALE_INT, PRIORITY_FIFTEEN, ScaleIntHander);
	OS_EI();
   
	//printf("Scale_Int_init IRQ\r\n");
	
}

int GetVideoLayerSourceFormat(int yuvformat)
{
	if (     yuvformat == FORMAT_YUV422 || yuvformat == FORMAT_Y_UV422 )
		return 0;
	else if( yuvformat == FORMAT_YUV420 || yuvformat == FORMAT_Y_UV420 )
		return 1;
	else if( yuvformat == FORMAT_YUYV   || yuvformat == FORMAT_YVYU || 
			   yuvformat == FORMAT_UYVY   || yuvformat == FORMAT_VYUY  )
		return 2;
	else if( yuvformat == FORMAT_YUV )
		return 3;
	else
	{
		printf("yuvformat error !\n");
		return FORMAT_SCALE_ERROR;
	}
}


void scale_arkn141_set_source_addr(unsigned char *yuv, unsigned int w,unsigned h, int yuvformat  )
{
	unsigned char *yin,*uin,*vin ;
	unsigned int weight = w*h;
	
	yin = yuv;
	if( yuvformat == FORMAT_YUV420 || yuvformat == FORMAT_Y_UV420 )
	{
		uin = yuv+weight;
		vin = yuv+weight+(weight>>2);
	}
	else if(    yuvformat == FORMAT_YUV422 || yuvformat == FORMAT_Y_UV422 )
	{
		uin = yuv+weight;
		vin = yuv+weight+(weight>>1);
	}
	else if( yuvformat == FORMAT_YUYV   || yuvformat == FORMAT_YVYU || yuvformat == FORMAT_YUV 
			  || yuvformat == FORMAT_UYVY   || yuvformat == FORMAT_VYUY  )
	{
		uin =0;
		vin =0;
	}
	else
	{
		printf("yuvformat address error !\n");
		return ;
	}
	

   rAXI_SCALE_VIDEO_ADDR1 =  (int)yin;		/* Y (YUV/YUYV) data start address.*/
   rAXI_SCALE_VIDEO_ADDR2 =  (int)uin;	   /* U or UV data start address. */
   rAXI_SCALE_VIDEO_ADDR3 =  (int)vin;
}


/*
scalar_arkn141_useint
	支持开窗写回

共计 19个参数 
*/

int  scalar_arkn141_useint( scale_s *scale )
{
	int status=0;
	int timeoutMax = ((scale->owidth*scale->oheight)>>10); // 944 宽度等不到中断 ，加长时间 
	//int timeoutMax = (40000);
	int ret=SCALE_RET_OK;
	int val;
	int time;

	int yuv_format ;
	if( !scale )
	{
		printf("Function:%s scale = NULL! \r\n",__FUNCTION__);
		return SCALE_RET_INV_PARA;
	}
	if( scale->Midline >= scale->oheight )
	{
		printf("Function:%s Midline > oheight! \r\n",__FUNCTION__);
		return SCALE_RET_INV_PARA;
	}
   yuv_format = GetVideoLayerSourceFormat( scale->source_format );
   
	rAXI_SCALE_EN		= 0x03;
	rAXI_SCALE_RESERVED = (scale->Midline&0xfff);
	
	if(scale->source_format == FORMAT_Y_UV422 || scale->source_format == FORMAT_Y_UV420 )
	{
      if( scale->source_format == FORMAT_Y_UV420 && (scale->inwidth==scale->owidth) &&  (scale->inheight == scale->oheight) &&  !scale->inwindow_enable && !scale->owindow_enable )//test all open
         rAXI_SCALE_CONTROL= 0x0<<29|0x0<<25|0x0<<24|0x8<<20|0x2<<16|0x0<<12|0x0<<8|0x0<<6|0x1<<5|0x0<<4|0x0<<3|0x1<<2| (yuv_format&0x3) <<0;
      else
         rAXI_SCALE_CONTROL= 0x0<<29|0x0<<25|0x0<<24|0x8<<20|0x2<<16|0x0<<12|0x0<<8|0x0<<6|0x0<<4|0x0<<3|0x1<<2| (yuv_format&0x3) <<0;
   }
	else if(yuv_format == FORMAT_YUYV )
	{
		if(scale->source_format == FORMAT_YUYV )
			rAXI_SCALE_CONTROL= 0x0<<29|0x0<<25|0x0<<24|0x8<<20|0x2<<16|0x0<<12|0x0<<8|0x0<<6|0x0<<4|0x0<<3|0x0<<2| FORMAT_YUYV <<0;
		else if(scale->source_format == FORMAT_YVYU )
			rAXI_SCALE_CONTROL= 0x0<<29|0x0<<25|0x0<<24|0x8<<20|0x2<<16|0x0<<12|0x0<<8|0x1<<6|0x0<<4|0x0<<3|0x0<<2| FORMAT_YUYV <<0;
		else if(scale->source_format == FORMAT_UYVY )
			rAXI_SCALE_CONTROL= 0x0<<29|0x0<<25|0x0<<24|0x8<<20|0x2<<16|0x0<<12|0x0<<8|0x2<<6|0x0<<4|0x0<<3|0x0<<2| FORMAT_YUYV <<0;
		else if(scale->source_format == FORMAT_VYUY )
			rAXI_SCALE_CONTROL= 0x0<<29|0x0<<25|0x0<<24|0x8<<20|0x2<<16|0x0<<12|0x0<<8|0x3<<6|0x0<<4|0x0<<3|0x0<<2| FORMAT_YUYV <<0;
		else
		{
			printf("yuv_format == FORMAT_YUYV NG\r\n");
			return SCALE_RET_INV_PARA;
		}
	}
	else
		rAXI_SCALE_CONTROL= 0x0<<29|0x0<<25|0x0<<24|0x8<<20|0x2<<16|0x0<<12|0x0<<8|0x0<<6|0x0<<4|0x0<<3|0x0<<2| (yuv_format&0x3) <<0;
      
	rAXI_SCALE_VIDEO_SOURCE_SIZE =  scale->inheight<<12|    scale->inwidth<<0;  /*	height<<12 | width */
   rAXI_SCALE_VIDEO_WINDOW_POINT=  scale->inwindow_y<<12|   scale->inwindow_x<<0;   /* y_positong<<12 | x_position */
   rAXI_SCALE_VIDEO_WINDOW_SIZE = (scale->inwindow_height)<<12| (scale->inwindow_width)<<0;  /* High_window_video<<12 | wide_window_video */
	/**Window 表示取的源的位置 以及宽度和高度 **/ /*底部 和右侧 显示错误*/
	
	/* 横轴 必须16点对齐 */
	rAXI_SCALE_VIDEO_SIZE = scale->oheight<<12| scale->owidth<<0; /*scale size */
	if( scale->source_format == FORMAT_Y_UV420 && 
      (scale->inwidth==scale->owidth) &&  (scale->inheight == scale->oheight) &&  !scale->inwindow_enable && !scale->owindow_enable )//test all open
      rAXI_SCALE_SCALE_CTL = 0<<9| 0<<8 | 1<<7| 0<<6 | 1<<5 | 0<<4 | 0<<3 | 0<<2 | 0<<1 | 0;
   else
      rAXI_SCALE_SCALE_CTL = 0<<9| 1<<8 | 1<<7| 0<<6 | 1<<5 | 0<<4 | 0<<3 | 0<<2 | 0<<1 | 0;

	rAXI_SCALE_SCALE_VXMOD = 512<<11 | 0<<0;/*V_xmod_odd_init<<11 | V_xmod_even_init */
   rAXI_SCALE_SCALE_CTL0 = 0<<18 | ((scale->inwindow_width)*1024/scale->owidth)<<0; /*left cut number in line scaler<<18 | hfz */
   rAXI_SCALE_SCALE_CTL1 = 0<<18 | ((scale->inwindow_height)*1024/scale->oheight)<<0; /* Up cut line number in veritical scaler <<18 | vfz */
   rAXI_SCALE_RIGHT_BOTTOM_CUT_NUM = 0<<8  | 0<<0; /* right_cut_num<<8 | bottom_cut_num */
   
	/* 写回ram数据的数据每行点数 */
   rAXI_SCALE_WB_DATA_HSIZE_RAM = scale->ostride;/*The number of the horizontal pix in write back ram, 
											it is equal or bigger than the wide of scale, x16 */

	// Source :YUV FORMAT Address set
	scale_arkn141_set_source_addr(scale->yuv ,scale->inwidth, scale->inheight, scale->source_format );
	
	scale_arkn141_set_writeback_addr((UINT32)scale->yout , (UINT32)scale->uout , (UINT32) scale->vout ) ;
	time = OS_GetTime();
	irq_enable(SCALE_INT);
	scale_arkn141_writeback_start();
	
	//检查中断完成情况，超时则返回 ！
	// 等待中断处理程序返回 Scale 完成 
	// 处理逻辑 ，必须先置位帧中断， 这样MID_num中断才会产生
	
	if( scale->Midline&0xfff ) 
	{
		printf("MidNum:%d \r\n", scale->Midline );
		if(OS_WaitCSemaTimed(&SEMAScaleMidNum, timeoutMax ) == 0 )
		{
			ret = SCALE_RET_TIMEOUT;
			//如果出问题则 先关闭 Scale
			status = rAXI_SCALE_INT_STATUS&0x30;
			rAXI_SCALE_EN		= 0x00;		
			printf("MidNum: status: 0x%08x  \r\n",status );
			printf("Scale timeout.time:%dms\r\n", timeoutMax );
			// 只清除了Mid_num 
			// 如果进入中断，在中断里面会被清除的，这里因为没有进入中断，所以手工清除
			rAXI_SCALE_CLCD_INT_CLR = (1<<2);// o=status, 
		}
#if 0
		printf("Y_line:%d UV_line:%d\r\n",Y_line , UV_line );
		if( (UV_line < scale->Midline) || (Y_line < scale->Midline) )
			printf("!!!Scale Error (UV_line < scale->Midline) || (Y_line < Midline) \r\n\r\n");
#endif		
	}

	if(OS_WaitCSemaTimed(&SEMAScaleFrame, timeoutMax )== 0 )
	{
		ret= SCALE_RET_TIMEOUT ;
		//如果出问题则 先关闭 Scale
		status = rAXI_SCALE_INT_STATUS&0x3;
		rAXI_SCALE_EN		= 0x00;		
		printf("Scale timeout.time:%dms\r\n", timeoutMax );
		printf("Frame status: 0x%08x \r\n",rAXI_SCALE_INT_STATUS );
		// 只清除了Frmae int
		// 如果进入中断，在中断里面会被清除的，这里因为没有进入中断，所以手工清除
		rAXI_SCALE_CLCD_INT_CLR = (1<<0);;
	}
	rAXI_SCALE_EN		= 0x0;
	
	irq_disable(SCALE_INT);
	time = OS_GetTime() - time;
	printf("Scale waste time : %d ms\n",time);
	return ret;
}// 用于支持 Slice中断

/*
	查询方式调用 Scale 
*/
int  scalar_arkn141_usereq(
					  unsigned char *yuv, // 输入数据
					  // 开窗坐标(x,y)          开窗大小 宽度与高度
					  int window_x,int window_y,int window_width,int window_height,
  
					  int iwidth, // 输入图像大小  宽度与高度
					  int iheight ,
					  int owidth, // 输出图像大小  宽度与高度
					  int oheight , 
                 int ostride ,
					  
					  int source_format, // Video layer source format
					  unsigned char *yout,// 输出数据
					  unsigned char *uout,
					  unsigned char *vout,
                 int Midline
					)
{
	int status=0;
	int timeout;
	//int timeoutMax = ((owidth*oheight)>>8); // 944 宽度等不到中断 ，加长时间 
	int timeoutMax = (40000);
	int ret= SCALE_RET_OK ;
	int val;
	int layer= 2 ; 
	int yuv_format=0;

	int time;
	
	if( Midline >= oheight )
	{
		printf("Function:%s Midline > oheight! \r\n", __FUNCTION__ );
		return SCALE_RET_INV_PARA;
	}
   
	rAXI_SCALE_EN		= 0x03;
	rAXI_SCALE_RESERVED = (Midline&0xfff);
	
	yuv_format = GetVideoLayerSourceFormat( source_format );
	//                         6                              5   
	if(source_format == FORMAT_Y_UV422 || source_format == FORMAT_Y_UV420 )
		rAXI_SCALE_CONTROL= 0x0<<29|0x0<<25|0x0<<24|0x8<<20|0x2<<16|0x0<<12|0x0<<8|0x0<<6|0x0<<4|0x0<<3|0x1<<2| yuv_format <<0;
	else if(yuv_format == FORMAT_YUYV )//2
	{
		if(source_format == FORMAT_YUYV )
			rAXI_SCALE_CONTROL= 0x0<<29|0x0<<25|0x0<<24|0x8<<20|0x2<<16|0x0<<12|0x0<<8|0x0<<6|0x0<<4|0x0<<3|0x0<<2| FORMAT_YUYV <<0;
		else if(source_format == FORMAT_YVYU )
			rAXI_SCALE_CONTROL= 0x0<<29|0x0<<25|0x0<<24|0x8<<20|0x2<<16|0x0<<12|0x0<<8|0x1<<6|0x0<<4|0x0<<3|0x0<<2| FORMAT_YUYV <<0;
		else if(source_format == FORMAT_UYVY )
			rAXI_SCALE_CONTROL= 0x0<<29|0x0<<25|0x0<<24|0x8<<20|0x2<<16|0x0<<12|0x0<<8|0x2<<6|0x0<<4|0x0<<3|0x0<<2| FORMAT_YUYV <<0;
		else if(source_format == FORMAT_VYUY )
			rAXI_SCALE_CONTROL= 0x0<<29|0x0<<25|0x0<<24|0x8<<20|0x2<<16|0x0<<12|0x0<<8|0x3<<6|0x0<<4|0x0<<3|0x0<<2| FORMAT_YUYV <<0;
		else
		{
			printf("yuv_format == FORMAT_YUYV NG\r\n");
			return SCALE_RET_INV_PARA;
		}
	}
	else
		rAXI_SCALE_CONTROL= 0x0<<29|0x0<<25|0x0<<24|0x8<<20|0x2<<16|0x0<<12|0x0<<8|0x0<<6|0x0<<4|0x0<<3|0x0<<2| yuv_format <<0;
	
	rAXI_SCALE_VIDEO_SOURCE_SIZE =  iheight<<12|    iwidth<<0;  /*	height<<12 | width */
   rAXI_SCALE_VIDEO_WINDOW_POINT=  window_y<<12|   window_x<<0;   /* y_positong<<12 | x_position */
   rAXI_SCALE_VIDEO_WINDOW_SIZE = (window_height)<<12| (window_width)<<0;  /* High_window_video<<12 | wide_window_video */
	/**Window 表示取的源的位置 以及宽度和高度 **/ /*底部 和右侧 显示错误*/
	
	/*水平 横轴 必须16点对齐 */
	rAXI_SCALE_VIDEO_SIZE = oheight<<12| owidth<<0; /*scale size */
   rAXI_SCALE_SCALE_CTL = 0<<9| 1<<8 | 1<<7| 0<<6 | 1<<5 | 0<<4 | 0<<3 | 0<<2 | 0<<1 | 0;

	rAXI_SCALE_SCALE_VXMOD = 512<<11 | 0<<0;/*V_xmod_odd_init<<11 | V_xmod_even_init */
   rAXI_SCALE_SCALE_CTL0 = 0<<18 | ((window_width)*1024/owidth)<<0; /*left cut number in line scaler<<18 | hfz */
   rAXI_SCALE_SCALE_CTL1 = 0<<18 | ((window_height)*1024/oheight)<<0; /* Up cut line number in veritical scaler <<18 | vfz */
   rAXI_SCALE_RIGHT_BOTTOM_CUT_NUM = 0<<8  | 0<<0; /* right_cut_num<<8 | bottom_cut_num */
   
	/* 写回ram数据的数据每行点数 */
   rAXI_SCALE_WB_DATA_HSIZE_RAM = ostride;/*The number of the horizontal pix in write back ram, 
											it is equal or bigger than the wide of scale, x16 */

	// Source :YUV FORMAT Address set
	scale_arkn141_set_source_addr(yuv ,iwidth, iheight, source_format );
	
	// Destination: Address set
	scale_arkn141_set_writeback_addr((UINT32)yout , (UINT32)uout ,(UINT32) vout);
	
//	if(owidth == 944)
//		printf("944 width\n");
	time = OS_GetTime();
	
	timeout = 0;
	// 使用查询方式 调用Scale 
	
	printf("Not Use_Scale_Int\r\n");
	// 使用查询方式
	// 不使用中断的方式完成Scale 
   if( Midline )
   {
      while( 1 )
      {//  处理 mid_line
         OS_Delay (1);
         if(rAXI_SCALE_INT_STATUS & (0x3<<5) )
         {
            printf("mid_line timewaste=%d ms  \r\n",timeout ) ;
            break;
         }
         if( timeout >= timeoutMax )
         {
            printf("timeout=%d ms wait for scale interrupt @ owidth=%d  oheight=%d \r\n",timeout, owidth, oheight ) ;
            ret= SCALE_RET_TIMEOUT;
            break;
         }
         else
            timeout++;
      }
      rAXI_SCALE_CLCD_INT_CLR = (1<<2);// 0x4
   }
	while( 1 )
	{// 处理 帧 完成中断
		OS_Delay (1);
		if(rAXI_SCALE_INT_STATUS & 0x3 )
		{
			printf("timewaste=%d ms  \r\n",timeout ) ;
			break;
		}
		if( timeout >= timeoutMax )
		{
         rAXI_SCALE_EN		= 0x00;
			printf("timeout=%d ms wait for scale interrupt @ owidth=%d  oheight=%d \r\n",timeout, owidth, oheight ) ;
			ret= SCALE_RET_TIMEOUT;
			// 先关闭 Scale
			status = rAXI_SCALE_INT_STATUS&0x3;
			
			break;
		}
		else
			timeout++;
	}
	rAXI_SCALE_CLCD_INT_CLR = 3;
	time = time-OS_GetTime();
	printf("Scale waste time:%dms\n",time);
	return ret;
} 

// 读取当前的middle finish计数
unsigned int arkn141_scalar_get_current_middle_line_count (void)
{
	return rAXI_SCALE_WR_Y_VCNT;
}

// 保存scale操作完成后的中断状态
volatile static unsigned int scale_interrupt_status;
// scale middle finish 中断回调函数
static void (*scale_middle_finish_callback) (void *private_data, unsigned int current_line_count);
// scale middle finish 私有数据
static void *scale_middle_finish_private;

static void arkn141_scalar_interrupt_handler (void)
{
	// 检查中断
	unsigned int status = rAXI_SCALE_INT_STATUS;
	unsigned int val;
//	printf ("status=0x%08x\n", status);
	do 
	{
		if(status & (1 << 2))	// write back bresp error
			break;
		
		if(status & (1 << 5))	// AXI SCALE write back middle finish interupt
		{
			// middle finish
			if(scale_middle_finish_callback)
			{
				// 调用middle finish用户自定义回调函数
				// 如，可以触发H264编码任务启动或者其他
				val = rAXI_SCALE_WR_Y_VCNT;
				(*scale_middle_finish_callback) (scale_middle_finish_private, val&0xfff);
//				(*scale_middle_finish_callback) (scale_middle_finish_private, (val>>11)&0xfff);
			}
			// write back middle finish interupt clear
			rAXI_SCALE_CLCD_INT_CLR = (1 << 2);
		}
		
		if(status & (1 << 0))	// write back frame finish interupt
			break;

		// middle finish中断，继续等待 bresp error 或 frame finish 
		return;
	} while (0);
	
	// 存在 write back bresp error 或者 write back frame finish, scale处理过程准备结束
	
	// 保存中断原因
	scale_interrupt_status = rAXI_SCALE_INT_STATUS;
	// 屏蔽scale所有中断
	rAXI_SCALE_INT_CTL = 0;
	// 清除中断
	rAXI_SCALE_CLCD_INT_CLR = 0x07;
	// 唤醒等待scalar完成的任务
	OS_EVENT_Set (&arkn141_scalar_finish_event);
}

// 执行scalar处理，直到图像处理完毕或者异常结束(超时、回写bresp error)
int  arkn141_scalar_process_internal (
					  unsigned char *yuv, // 输入数据
                 unsigned int inwindow_enable,unsigned int owindow_enable,
					  // 开窗坐标(x,y)          开窗大小 宽度与高度
					  int window_x,int window_y,int window_width,int window_height,
  
					  int iwidth, // 输入图像大小  宽度与高度
					  int iheight ,
					  int owidth, // 输出图像大小  宽度与高度
					  int oheight , 
					  int ostride,
					  
					  int source_format, // Video layer source format
					  unsigned char *yout,// 输出数据
					  unsigned char *uout,
					  unsigned char *vout,
					  
					  unsigned int middle_finish_line_count,		// 0     表示关闭middle finish中断
					  															// 非0值 表示开启middle finish中断
					  void (*middle_finish_callback) (void *private_data, unsigned int current_line_count),	
					                                             // middle finish中断回调函数
																				//        此回调函数将在中断上下文中被调用
					  void * middle_finish_private_data          // middle finish中断回调函数的私有数据
					   
					)
{
	int ret = SCALE_RET_OK;
	//int time;
	int do_horz_down_scalar = 0;		// down scalar需要使能"行方向滤波"

	int yuv_format = GetVideoLayerSourceFormat( source_format );
	
	if(middle_finish_line_count >= oheight)
	{
		printf("arkn141_scalar_process middle_finish_line_count > oheight! \r\n");
		return SCALE_RET_INV_PARA;		
	}
	
	// 互斥访问保护
	OS_Use (&arkn141_scalar_access_semid);
	
	
	sys_clk_disable (Scale_xclk_enable);
	sys_clk_disable (Scale_clk_enable);
	sys_soft_reset (softreset_scale);
	sys_clk_enable (Scale_clk_enable);
	sys_clk_enable (Scale_xclk_enable);
	
	
	
	
	// scalar控制器准备
	// 开启scale时钟
#if 0
   rSYS_SOFT_RSTNB  &=~(1<<10);
   rSYS_SOFT_RSTNB  |=(1<<10);
 	__asm ("nop");
 	__asm ("nop");
 	__asm ("nop");
 	__asm ("nop");
#endif   
   // 使能控制器，使能回写
	rAXI_SCALE_EN		= 0x03;
   
	
	// 复位事件，防止超时导致的事件残留影响
	OS_EVENT_Reset (&arkn141_scalar_finish_event);
	
	scale_interrupt_status = 0;
	//time = OS_GetTime();
	do 
	{
		if(middle_finish_line_count)
		{
			// 检查middle finish中断函数
			if(middle_finish_callback == NULL)
			{
				ret = SCALE_RET_INV_PARA;
				break;
			}
		}
		
		// 清除SCALE中断 (write back middle finish, write back bresp error, write back frame finish)
		rAXI_SCALE_CLCD_INT_CLR = 0x07;
		
		// 使能SCALE中断 (write back bresp error, write back frame finish)
		rAXI_SCALE_INT_CTL = (1 << 0) | (1 << 1);
		if(middle_finish_line_count)
		{
			rAXI_SCALE_INT_CTL |= (1 << 2);	// enable write back middle finish
			scale_middle_finish_callback = middle_finish_callback;
			scale_middle_finish_private = middle_finish_private_data;
		}
		else
		{
			scale_middle_finish_callback = NULL;
			scale_middle_finish_private = NULL;
		}
		
		// 安装中断处理向量
		request_irq (SCALE_INT, PRIORITY_FIFTEEN, arkn141_scalar_interrupt_handler);
		
		
		rAXI_SCALE_RESERVED = middle_finish_line_count & 0xfff;
		
		if(source_format == FORMAT_Y_UV422 || source_format == FORMAT_Y_UV420 )
			rAXI_SCALE_CONTROL= 0x0<<29|0x0<<25|0x0<<24|0x8<<20|0x2<<16|0x0<<12|0x0<<8|0x0<<6|0x0<<4|0x0<<3|0x1<<2| yuv_format <<0;
		else if(yuv_format == FORMAT_YUYV )
		{
			if(source_format == FORMAT_YUYV )
				rAXI_SCALE_CONTROL= 0x0<<29|0x0<<25|0x0<<24|0x8<<20|0x2<<16|0x0<<12|0x0<<8|0x0<<6|0x0<<4|0x0<<3|0x0<<2| FORMAT_YUYV <<0;
			else if(source_format == FORMAT_YVYU )
				rAXI_SCALE_CONTROL= 0x0<<29|0x0<<25|0x0<<24|0x8<<20|0x2<<16|0x0<<12|0x0<<8|0x1<<6|0x0<<4|0x0<<3|0x0<<2| FORMAT_YUYV <<0;
			else if(source_format == FORMAT_UYVY )
				rAXI_SCALE_CONTROL= 0x0<<29|0x0<<25|0x0<<24|0x8<<20|0x2<<16|0x0<<12|0x0<<8|0x2<<6|0x0<<4|0x0<<3|0x0<<2| FORMAT_YUYV <<0;
			else if(source_format == FORMAT_VYUY )
				rAXI_SCALE_CONTROL= 0x0<<29|0x0<<25|0x0<<24|0x8<<20|0x2<<16|0x0<<12|0x0<<8|0x3<<6|0x0<<4|0x0<<3|0x0<<2| FORMAT_YUYV <<0;
			else
			{
				printf("yuv_format == FORMAT_YUYV NG\r\n");
				ret = SCALE_RET_INV_PARA;
				break;
			}
		}
		else
			rAXI_SCALE_CONTROL= 0x0<<29|0x0<<25|0x0<<24|0x8<<20|0x2<<16|0x0<<12|0x0<<8|0x0<<6|0x0<<4|0x0<<3|0x0<<2| yuv_format <<0;
		
		if( (iwidth==owidth) &&  (iheight == oheight) &&  !inwindow_enable && !owindow_enable )//test all open
			rAXI_SCALE_CONTROL |= (1<<5);
		
		rAXI_SCALE_VIDEO_SOURCE_SIZE =  iheight<<12|    iwidth<<0;  /*	height<<12 | width */
		rAXI_SCALE_VIDEO_WINDOW_POINT=  window_y<<12|   window_x<<0;   /* y_positong<<12 | x_position */
		rAXI_SCALE_VIDEO_WINDOW_SIZE = (window_height)<<12| (window_width)<<0;  /* High_window_video<<12 | wide_window_video */
		/**Window 表示取的源的位置 以及宽度和高度 **/ /*底部 和右侧 显示错误*/
		
		/* 横轴 必须16点对齐 */
		rAXI_SCALE_VIDEO_SIZE = oheight<<12| owidth<<0; /*scale size */
		
 	  // 源窗口宽度大于目标窗口宽度, 使能行方向滤波
		if( iwidth > owidth )
			do_horz_down_scalar = 1;
		
		if( do_horz_down_scalar == 0 )
			rAXI_SCALE_SCALE_CTL = 0<<9| 0<<8 | 1<<7| 0<<6 | 1<<5 | 0<<4 | 0<<3 | 0<<2 | 0<<1 | 0;
		else
			rAXI_SCALE_SCALE_CTL = 0<<9| 1<<8 | 1<<7| 1<<6 | 1<<5 | 0<<4 | 0<<3 | 0<<2 | 0<<1 | 0;

		rAXI_SCALE_SCALE_VXMOD = 512<<11 | 0<<0;/*V_xmod_odd_init<<11 | V_xmod_even_init */
		if(do_horz_down_scalar)		// 裁剪左侧重复的5列
			rAXI_SCALE_SCALE_CTL0 = 5<<18 | ((window_width)*1024/(owidth + 5))<<0; /*left cut number in line scaler<<18 | hfz */
		else
			rAXI_SCALE_SCALE_CTL0 = 0<<18 | ((window_width)*1024/owidth)<<0; /*left cut number in line scaler<<18 | hfz */
		rAXI_SCALE_SCALE_CTL1 = 0<<18 | ((window_height)*1024/(oheight))<<0; /* Up cut line number in veritical scaler <<18 | vfz */
		rAXI_SCALE_RIGHT_BOTTOM_CUT_NUM = 0<<8  | 0<<0; /* right_cut_num<<8 | bottom_cut_num */
		
		/* 写回ram数据的数据每行点数 */
		rAXI_SCALE_WB_DATA_HSIZE_RAM = ostride;/*The number of the horizontal pix in write back ram, 
												it is equal or bigger than the wide of scale, x16 */
	
		// Source :YUV FORMAT Address set
		scale_arkn141_set_source_addr(yuv ,iwidth, iheight, source_format );

		scale_arkn141_set_writeback_addr((UINT32)yout , (UINT32)uout,(UINT32) vout );
		
		// 触发scalar转换
		scale_arkn141_writeback_start();
		
		//检查中断完成情况，超时则返回 ！
		// 等待中断处理程序返回 Scale 完成 
#ifdef CONFIG_ARK1960_ASIC
		if(OS_EVENT_WaitTimed (&arkn141_scalar_finish_event, 1000))
#else
		if(OS_EVENT_WaitTimed (&arkn141_scalar_finish_event, 8000))
#endif
		{
			ret = SCALE_RET_TIMEOUT ;
			printf ("\t\tscalar timeout, status=0x%08x\n", rAXI_SCALE_INT_STATUS);
		}
		// 检查hw error
		else if(scale_interrupt_status & (1 << 2))
		{
			ret = SCALE_RET_HW_ERROR;
			printf ("\t\tscalar hwerror, status=0x%08x\n", rAXI_SCALE_INT_STATUS);
		}
		
	} while (0);
	
	// 关闭 Scale控制器及回写
	rAXI_SCALE_EN		= 0x00;	

	// 关闭scale时钟	
	
	// 禁止scalar中断
	request_irq (SCALE_INT, PRIORITY_FIFTEEN, NULL);

	// 解除互斥访问保护
	OS_Unuse (&arkn141_scalar_access_semid);
	//time = OS_GetTime()-time;
	//printf("Scale waste time:%dms\n",time);
	
	return ret;
}// 用于支持 Frame 中断

#define	TWO_COLUME		// 每次处理2列数据

#define	SRAM_BASE	0x300000

static void process_2_column_asm (unsigned char *src_ram, unsigned char *dst_ddr, int width, int height)
{
	// r0 --> src_ram
	// r1 --> dst_ddr
	// r2 --> width
	// r3 --> height
	// r4 --> j = width/2
	// r5 --> data1
	// r6 --> data2
	// r7 --> data3
	// r8 --> data4
	// r9 --> sdata
	// r10 --> sdata2
	// lr  --> temp
	// r11 --> src_ram
	
	asm ("PUSH      {r4-r11, lr}");
	asm ("mov		 r11,	r0");
	asm ("ASR       r4, r2, #1");			// r4 = r2 >> 1
	asm ("cmp		 r4,	#1");				// 
	asm ("blt		 process_2_column_0"); 
	
	asm ("process_2_column_1:");
	
	// sdata = *(unsigned short *)src;
	asm ("LDRH     r9, 	[r0]");		
	// src +=  width;
	asm ("add		r0,	r0,	r2");
	asm ("LDRH		r10,	[r0]");
	asm ("add		r0,	r0,	r2");
		  
	// data1  = (unsigned char)sdata ;
	asm ("UXTB		r5, r9");
	// data3  = (unsigned char)(sdata >> 8) ;
	asm ("UXTB		r7, r9, ROR #8");
	
	//		data1 |= (sdata & 0xFF) << 8;
	//		data3 |= (sdata & 0xFF00);
	asm ("AND		r9,  r10, #255");		// 0xff		
	asm ("AND		r10, r10, #65280");	// 0xff00
	asm ("ORR      r5, r5, r9, LSL #8");
	asm ("ORR      r7, r7, r10");
	
	//		sdata = *(unsigned short *)src;
	//		data1 |= (sdata & 0xFF) << 16;
	//		data3 |= (sdata & 0xFF00) << 8;
	asm ("LDRH     r9, 	[r0]");	
	// src +=  width;
	asm ("add		r0,	r0,	r2");
	asm ("LDRH		r10,	[r0]");
	asm ("add		r0,	r0,	r2");
	asm ("AND		lr,	r9,	#255");
	asm ("orr		r5,	r5,	lr,	LSL #16");
	asm ("AND		lr,	r9,	#65280");
	asm ("orr		r7,	r7,	lr,	LSL #8");
	//		data1 |= (sdata & 0xFF) << 24;
	//		data3 |= (sdata & 0xFF00) << 16;
	asm ("and		lr,	r10,	#255");
	asm ("orr		r5,	r5,	lr,	LSL #24");
	asm ("and		lr,	r10,	#65280");
	asm ("orr		r7,	r7,	lr,	LSL #16");
	
	
	//		sdata = *(unsigned short *)src;
	//		data2  = (unsigned char)sdata ;
	//		data4  = (unsigned char)(sdata >> 8) ;
	//		src +=  width;
	//		sdata = *(unsigned short *)src;
	//		data2 |= (sdata & 0xFF) << 8;
	//		data4 |= (sdata & 0xFF00);
	//		src +=  width;
	asm ("LDRH     r9, 	[r0]");	
	// src +=  width;
	asm ("add		r0,	r0,	r2");
	asm ("LDRH		r10,	[r0]");
	// src +=  width;
	asm ("add		r0,	r0,	r2");
	// data2  = (unsigned char)sdata ;
	asm ("UXTB		r6, r9");
	// data4  = (unsigned char)(sdata >> 8) ;
	asm ("UXTB		r8, r9, ROR #8");
	asm ("AND		r9,  r10, #255");		// 0xff		
	asm ("AND		r10, r10, #65280");	// 0xff00
	asm ("ORR      r6, r6, r9, LSL #8");
	asm ("ORR      r8, r8, r10");
	
	//		sdata = *(unsigned short *)src;
	//		data2 |= (sdata & 0xFF) << 16;
	//		data4 |= (sdata & 0xFF00) << 8;
	//		src +=  width;
	//		sdata = *(unsigned short *)src;
	//		data2 |= (sdata & 0xFF) << 24;
	//		data4 |= (sdata & 0xFF00) << 16;
	asm ("LDRH     r9, 	[r0]");	
	// src +=  width;
	asm ("add		r0,	r0,	r2");
	asm ("LDRH		r10,	[r0]");
	asm ("AND		lr,	r9,	#255");
	asm ("orr		r6,	r6,	lr,	LSL #16");
	asm ("AND		lr,	r9,	#65280");
	asm ("orr		r8,	r8,	lr,	LSL #8");
	
	asm ("and		lr,	r10,	#255");
	asm ("orr		r6,	r6,	lr,	LSL #24");
	asm ("and		lr,	r10,	#65280");
	asm ("orr		r8,	r8,	lr,	LSL #16");

	//		*(unsigned int *)dst_ddr = data1;
	//		*(unsigned int *)(dst_ddr+4) = data2;
	//		dst_ddr += height;		// 指向DDR的下一行
	asm ("stm		r1,	{r5, r6}");	
	asm ("add		r1,	r1,	r3");
	//		*(unsigned int *)dst_ddr = data3;
	//		*(unsigned int *)(dst_ddr+4) = data4;
	//		dst_ddr += height;		// 指向DDR的下一行
	asm ("stm		r1,	{r7, r8}");	
	asm ("add		r1,	r1,	r3");
	
	
	// src_ram += 2;		// 指向下一列的数据
	asm ("add		 r11,  r11, #2");
	asm ("mov		r0,	r11");
	
	asm ("subs		 r4,  r4, #1");
	asm ("bne		 process_2_column_1");
	
	asm ("process_2_column_0: POP       {r4-r11, pc}");
}

static void process_2_column (unsigned char *src_ram, unsigned char *dst_ddr, int width, int height)
{
		int j;
		// 每次处理相邻的2列
		for (j = 0; j < width/2; j ++)
		{
			// 每次读取片内SRAM中位于不同行的2列(8个字节数据)(相同列), 将其写入到连续的DDR地址(8个列数据)
			unsigned int data1, data2, data3, data4;
			unsigned short sdata;
			unsigned char *src = src_ram;
			sdata = *(unsigned short *)src;
			data1  = (unsigned char)sdata ;
			data3  = (unsigned char)(sdata >> 8) ;
			src +=  width;
			sdata = *(unsigned short *)src;
			data1 |= (sdata & 0xFF) << 8;
			data3 |= (sdata & 0xFF00);
			src +=  width;
			sdata = *(unsigned short *)src;
			data1 |= (sdata & 0xFF) << 16;
			data3 |= (sdata & 0xFF00) << 8;
			src +=  width;
			sdata = *(unsigned short *)src;
			data1 |= (sdata & 0xFF) << 24;
			data3 |= (sdata & 0xFF00) << 16;
			src +=  width;
			sdata = *(unsigned short *)src;
			data2  = (unsigned char)sdata ;
			data4  = (unsigned char)(sdata >> 8) ;
			src +=  width;
			sdata = *(unsigned short *)src;
			data2 |= (sdata & 0xFF) << 8;
			data4 |= (sdata & 0xFF00);
			src +=  width;
			sdata = *(unsigned short *)src;
			data2 |= (sdata & 0xFF) << 16;
			data4 |= (sdata & 0xFF00) << 8;
			src +=  width;
			sdata = *(unsigned short *)src;
			data2 |= (sdata & 0xFF) << 24;
			data4 |= (sdata & 0xFF00) << 16;
			//src +=  width;
			src_ram += 2;		// 指向下一列的数据
			
			*(unsigned int *)dst_ddr = data1;
			*(unsigned int *)(dst_ddr+4) = data2;
			dst_ddr += height;		// 指向DDR的下一行
			*(unsigned int *)dst_ddr = data3;
			*(unsigned int *)(dst_ddr+4) = data4;
			dst_ddr += height;		// 指向DDR的下一行
		}	
}

static void process_cbcr (unsigned char *src_ram, unsigned char *dst_ddr, unsigned int width, unsigned int height)
{
#if 1
	asm ("PUSH      {r4-r8, lr}");
	asm ("LSRS      lr, r2, #1");
	asm ("BEQ       process_cbcr_0");
	asm ("ADD       r12, r2, r0");
	
	asm ("process_cbcr_1:");
	asm ("LDRH      r4, [r0], #0x2");
	asm ("LDRH      r5, [r12]");
	asm ("SUBS      lr, lr, #1");
	asm ("ORR       r4, r4, r5, LSL #16");
	asm ("ADD       r5, r2, r12");
	asm ("LDRH      r6, [r5]");
	asm ("ADD       r7, r2, r5");
	asm ("LDRH      r5, [r7]");
	asm ("ADD       r12, r12, #2");
	asm ("ORR       r5, r6, r5, LSL #16");
	asm ("ADD       r6, r2, r7");
	asm ("LDRH      r7, [r6]");
	asm ("ADD       r6, r2, r6");
	asm ("LDRH      r8, [r6]");
	asm ("ADD       r6, r2, r6");
	asm ("ORR       r7, r7, r8, LSL #16");
	asm ("LDRH      r8, [r6]");
	asm ("LDRH      r6, [r2, +r6]");
	//asm ("STR       r4, [r1]");
	//asm ("STR       r5, [r1, #0x4]");
	//asm ("ORR       r6, r8, r6, LSL #16");
	asm ("ORR       r8, r8, r6, LSL #16");
	//asm ("STR       r7, [r1, #0x8]");
	//asm ("STR       r6, [r1, #0xc]");
	asm ("stm		 r1,	{r4, r5, r7, r8}");
	asm ("ADD       r1, r3, r1");
	asm ("BNE       process_cbcr_1");
	
	asm ("process_cbcr_0: POP       {r4-r8, pc}");
	
#else
	unsigned int j;
		for (j = 0; j < width/2; j ++)	// CbCr只有Y的1/2
		{
			// 每次读取片内SRAM中位于不同行的8个字节数据(相同列), 将其写入到连续的DDR地址(8个列数据)
			unsigned int data1, data2, data3, data4;
			unsigned char *src = (unsigned char *)src_ram;		
			data1  = *(unsigned short *)src;	// 每次读取一对CbCr
			src +=  width;		// 指向下一行cbcr
			data1 |= (*(unsigned short *)src) << 16;
			src +=  width;
			data2  = *(unsigned short *)src;	// 每次读取一对CbCr
			src +=  width;
			data2 |= (*(unsigned short *)src) << 16;
			src +=  width;
			data3  = *(unsigned short *)src;	// 每次读取一对CbCr
			src +=  width;
			data3 |= (*(unsigned short *)src) << 16;
			src +=  width;
			data4  = *(unsigned short *)src;	// 每次读取一对CbCr
			src +=  width;
			data4 |= (*(unsigned short *)src) << 16;
			//src +=  width;
			src_ram += 2;		// 指向下一列的数据
			
			*(unsigned int *)dst_ddr = data1;
			*(unsigned int *)(dst_ddr+4) = data2;
			*(unsigned int *)(dst_ddr+8) = data3;
			*(unsigned int *)(dst_ddr+12) = data4;
			dst_ddr += height;		// 指向DDR的下一行
			
		}	
#endif
}

extern void dma_memcpy (unsigned char *dst_buf, unsigned char *src_buf, unsigned int len);

//#define	DMA_MEMCPY		// // 实测DMA效率比memcpy效率慢

#define	DMA_M2M_COPY_CHANNEL		3

// 为避免矩阵转置(W*H --> H*W)中DDR访问导致的海量bank Active/bank PreCharge开销(每个字节的拷贝均可能引入Bank Active/bank PreCharge)
// 使用片内SRAM为中间储存环节, 每次从DDR复制8行数据到SRAM, 
// 然后将这8行数据转置, 写入到DDR.
// 循环知道所有数据处理完成
void rotate_degree90 (unsigned char *y_uv420_src, 
						  unsigned char *y_uv420_dst,
						  int width, // 输入图像大小  宽度与高度
						  int height, 
						  int o_width, // 输入图像大小  宽度与高度
						  int o_height 
						 )
{
	int i, j;
	int count;
	unsigned char *y_src, *cbcr_src;
	unsigned char *y_dst, *cbcr_dst;
	
#ifdef DMA_M2M_COPY
	unsigned int DmaCh;
	DmaCh = dma_request_dma_channel (DMA_M2M_COPY_CHANNEL);
#endif
	
	if(!(height & 0x0F))
	{
		assert (!(height & 0x0F));
	}
	if(!(width & 0x0F))
	{
		assert (!(width & 0x0F));
	}
	// 每次读8行
	count = height / 8;
	
	// 处理Y数据
	y_src = y_uv420_src;
	y_dst = y_uv420_dst;
	for (i = 0; i < count; i ++)
	{
		// 每次读取8行数据到SRAM
#ifdef DMA_M2M_COPY
		if(DmaCh != ALL_CHANNEL_RUNNING)
			dma_m2m_copy (DmaCh, y_src, (char *)SRAM_BASE, 8 * width);
		else
#endif
			
#ifdef DMA_MEMCPY
			dma_memcpy ((char *)SRAM_BASE, y_src, 8 * width);			
#else
			memcpy ((char *)SRAM_BASE, y_src, 8 * width);
#endif	
		y_src += 8 * width;
		
		// 按列处理
		unsigned char *src_ram, *dst_ddr;
		src_ram = (unsigned char *)SRAM_BASE;
		dst_ddr = y_dst;
		
		y_dst += 8;	// 移动到下一个8列
		
		
#ifdef TWO_COLUME
		
#if 1
		//process_2_column (src_ram, dst_ddr, width, height);
		process_2_column_asm (src_ram, dst_ddr, width, height);
#else
		// 每次处理相邻的2列
		for (j = 0; j < width/2; j ++)
		{
			// 每次读取片内SRAM中位于不同行的2列(8个字节数据)(相同列), 将其写入到连续的DDR地址(8个列数据)
			unsigned int data1, data2, data3, data4;
			unsigned short sdata;
			unsigned char *src = src_ram;
			sdata = *(unsigned short *)src;
			data1  = (unsigned char)sdata ;
			data3  = (unsigned char)(sdata >> 8) ;
			src +=  width;
			sdata = *(unsigned short *)src;
			data1 |= (sdata & 0xFF) << 8;
			data3 |= (sdata & 0xFF00);
			src +=  width;
			sdata = *(unsigned short *)src;
			data1 |= (sdata & 0xFF) << 16;
			data3 |= (sdata & 0xFF00) << 8;
			src +=  width;
			sdata = *(unsigned short *)src;
			data1 |= (sdata & 0xFF) << 24;
			data3 |= (sdata & 0xFF00) << 16;
			src +=  width;
			sdata = *(unsigned short *)src;
			data2  = (unsigned char)sdata ;
			data4  = (unsigned char)(sdata >> 8) ;
			src +=  width;
			sdata = *(unsigned short *)src;
			data2 |= (sdata & 0xFF) << 8;
			data4 |= (sdata & 0xFF00);
			src +=  width;
			sdata = *(unsigned short *)src;
			data2 |= (sdata & 0xFF) << 16;
			data4 |= (sdata & 0xFF00) << 8;
			src +=  width;
			sdata = *(unsigned short *)src;
			data2 |= (sdata & 0xFF) << 24;
			data4 |= (sdata & 0xFF00) << 16;
			//src +=  width;
			src_ram += 2;		// 指向下一列的数据
			
			*(unsigned int *)dst_ddr = data1;
			*(unsigned int *)(dst_ddr+4) = data2;
			dst_ddr += height;		// 指向DDR的下一行
			*(unsigned int *)dst_ddr = data3;
			*(unsigned int *)(dst_ddr+4) = data4;
			dst_ddr += height;		// 指向DDR的下一行
		}
#endif
		
#else
		// 每次处理1列
		for (j = 0; j < width; j ++)
		{
			// 每次读取片内SRAM中位于不同行的1列(8个字节数据)(相同列), 将其写入到连续的DDR地址(8个列数据)
			unsigned int data1, data2;
			unsigned char *src = src_ram;
			data1  = *src;
			src +=  width;
			data1 |= *src << 8;
			src +=  width;
			data1 |= *src << 16;
			src +=  width;
			data1 |= *src << 24;
			src +=  width;
			data2  = *src;
			src +=  width;
			data2 |= *src << 8;
			src +=  width;
			data2 |= *src << 16;
			src +=  width;
			data2 |= *src << 24;
			//src +=  width;
			src_ram += 1;		// 指向下一列的数据
			
			*(unsigned int *)dst_ddr = data1;
			*(unsigned int *)(dst_ddr+4) = data2;
			dst_ddr += height;		// 指向DDR的下一行
		}
#endif
	}
	

	// 处理UV数据
	cbcr_src = y_uv420_src + width * height;
	cbcr_dst = y_uv420_dst + width * height;
	for (i = 0; i < count/2; i ++)		// CbCr只有Y的1/2
	{
		// 每次读取8行数据到SRAM
#ifdef DMA_M2M_COPY
		if(DmaCh != ALL_CHANNEL_RUNNING)
			dma_m2m_copy (DmaCh, cbcr_src, (char *)SRAM_BASE, 8 * width);
		else
#endif
#ifdef DMA_MEMCPY		// 实测DMA效率比memcpy效率慢
			dma_memcpy ((char *)SRAM_BASE, cbcr_src, 8 * width);
#else
			memcpy ((char *)SRAM_BASE, cbcr_src, 8 * width);
#endif
		cbcr_src += 8 * width;
		
		// 按列处理
		unsigned char *src_ram, *dst_ddr;
		src_ram = (unsigned char *)SRAM_BASE;
		dst_ddr = cbcr_dst;
		
		cbcr_dst += 8*2;	// 移动到下一个8列(每列2个元素, CbCr)
		
		
#if 1
		process_cbcr (src_ram, dst_ddr, width, height);
#else
		for (j = 0; j < width/2; j ++)	// CbCr只有Y的1/2
		{
			// 每次读取片内SRAM中位于不同行的8个字节数据(相同列), 将其写入到连续的DDR地址(8个列数据)
			unsigned int data1, data2, data3, data4;
			unsigned char *src = (unsigned char *)src_ram;		
			data1  = *(unsigned short *)src;	// 每次读取一对CbCr
			src +=  width;		// 指向下一行cbcr
			data1 |= (*(unsigned short *)src) << 16;
			src +=  width;
			data2  = *(unsigned short *)src;	// 每次读取一对CbCr
			src +=  width;
			data2 |= (*(unsigned short *)src) << 16;
			src +=  width;
			data3  = *(unsigned short *)src;	// 每次读取一对CbCr
			src +=  width;
			data3 |= (*(unsigned short *)src) << 16;
			src +=  width;
			data4  = *(unsigned short *)src;	// 每次读取一对CbCr
			src +=  width;
			data4 |= (*(unsigned short *)src) << 16;
			//src +=  width;
			src_ram += 2;		// 指向下一列的数据
			
			*(unsigned int *)dst_ddr = data1;
			*(unsigned int *)(dst_ddr+4) = data2;
			*(unsigned int *)(dst_ddr+8) = data3;
			*(unsigned int *)(dst_ddr+12) = data4;
			dst_ddr += height;		// 指向DDR的下一行
			
		}
#endif
	}
	
#ifdef DMA_M2M_COPY
	if(DmaCh != ALL_CHANNEL_RUNNING)
		dma_release_channel (DmaCh);
#endif
	
#if defined(DMA_MEMCPY) || defined(DMA_M2M_COPY)
	dma_flush_range ((unsigned int)y_uv420_dst, width * height * 3/2 + (unsigned int)y_uv420_dst);
#else
	// 非DMA方式
	dma_flush_range ((unsigned int)y_uv420_dst, width * height * 3/2 + (unsigned int)y_uv420_dst);
#endif
	
}

static int scale_inited = 0;		
void  arkn141_scale_init()
{
	if(scale_inited)
		return;
	
	OS_EnterRegion();
	
	scale_inited = 1;
	
	// SYS_AHB_CLK_EN
	// 27	R/W	1	Scale_clk_enable
	//             1: enable
	//             0: disable
	// 24	R/W	1	Scale_xclk_enable
	//             1: enable
	//             0: disable
	
	unsigned int val;
	
	// 关闭scale时钟
	XM_lock ();
	val = rSYS_AHB_CLK_EN;
	val &= ~((1 << 27) | (1 << 24));
	rSYS_AHB_CLK_EN = val;
	XM_unlock ();
	
	// reset
	XM_lock ();
   rSYS_SOFT_RSTNB  &= ~(1<<10);
	XM_unlock ();
	delay (10);
	XM_lock ();
   rSYS_SOFT_RSTNB  |=  (1<<10);
	XM_unlock ();
	
	// SYS_DEVICE_CLK_CFG2
	// 13-11	R/W	0x0	Scale clk div
	//                   Scale clk = int_scale_clk/div
	// 10-8	R/W	0x4	Int_scale_clk_sel
	//                   3'b0000: Clk_240m
	//                   3'b0001: Syspll_clk
	//                   3'b0010: audpll_clk
	//                   3'b0100: Clk_24m
	// 切换到Syspll_clk, 
	XM_lock ();
	val = rSYS_DEVICE_CLK_CFG2;
	val &= ~((0x7 << 11) | (0x7 << 8));
#if CZS_USB_01
	val |=  ((0x1 << 11) | (0x0 << 8));
#else
	//val |=  ((0x1 << 11) | (0x1 << 8));
	val |=  ((0x1 << 11) | (0x2 << 8));
#endif
	rSYS_DEVICE_CLK_CFG2 = val;
	XM_unlock ();
	
	// 开启scale时钟
	XM_lock ();
	val = rSYS_AHB_CLK_EN;
	val |=  ((1 << 27) | (1 << 24));
	rSYS_AHB_CLK_EN = val;
	XM_unlock ();
	

	/*
	rAXI_SCALE_INT_CTL 说明 ：
		bit.2	R/W	0	AXI scale write back middle interrupt mask
						0: mask
						1: enable
		bit.1	R/W	0	AXI SCALE write back bresp error interrupt mask
						0: mask
						1: enable
		bit.0	R/W	0	AXI SCALE write back frame finish interupt mask
						0: mask
						1: enable
	*/
	unsigned int status;
#if Use_Scale_Int_Mid_Num
	status = ((1<<2)|(1<<0)|(1 << 1));
#else
	status = (1<<0);
#endif
	
	Scale_Int_init( status );

	scale_arkn141_open();
	
	OS_LeaveRegion();
	
	printf ("scalar_clk = %d\n", arkn141_get_clks(ARKN141_CLK_SCALAR));
}

void  arkn141_scale_exit (void)
{
	unsigned int val;
	if(scale_inited == 0)
		return;
	
	OS_EnterRegion();
	
	scale_arkn141_close ();
	
	// 关闭scale时钟
	XM_lock ();
	val = rSYS_AHB_CLK_EN;
	val &= ~((1 << 27) | (1 << 24));
	rSYS_AHB_CLK_EN = val;
	XM_unlock ();
	
	scale_inited = 1;
	
	OS_LeaveRegion();
}


//#define	_TIME_EVALUATE_

#define	DCACHE_USE		// 使用Cache算法, 明显改善速度

#define	ROT_W		352	// 中间转置矩阵的宽度
								//		这个中间转置矩阵宽度会影响整个转置算法的效率及最终图像的质量
								//		越大, 图像效果越好, 计算量及时间越大.
								//		越小, 图像效果越差, 计算量及时间变小.
								//	初步评估结论 (参考下面的比较数据, 单位为微秒)
								//		352, 具有较好的抗锯齿效果及较佳的计算时间	

// 400 NO DCACHE
//	scalar_1st_ticket = 7705
//	rotate_1st_ticket = 20566
//	scalar_2nd_ticket = 609
//	rotate_2nd_ticket = 11327
//	400 DCACHE		基本无锯齿
//	scalar_1st_ticket = 7705
//	rotate_1st_ticket = 15441
//	scalar_2nd_ticket = 791
//	rotate_2nd_ticket = 9354
// 352 DCACHE		轻微锯齿
// scalar_1st_ticket = 7696
// rotate_1st_ticket = 13026
// scalar_2nd_ticket = 691
// rotate_2nd_ticket = 9356
// 320 DCACHE		较明显的锯齿
// scalar_1st_ticket = 7693
// rotate_1st_ticket = 12872
// scalar_2nd_ticket = 654
// rotate_2nd_ticket = 9346


// 352
// scalar_1st_ticket = 7373
// rotate_1st_ticket = 9690
// scalar_2nd_ticket = 631
// rotate_2nd_ticket = 6999


static int do_isp_scalar_anti_aliasing_enhance = 0;
static unsigned int isp_scalar_anti_aliasing_enhance_image_width;
static unsigned int isp_scalar_anti_aliasing_enhance_image_height;
static unsigned char *isp_scalar_anti_aliasing_enhance_1st_rotate_buffer;		// 第一次旋转
static unsigned char *isp_scalar_anti_aliasing_enhance_2nd_scalar_buffer;		// 第二次水平缩放
static unsigned char *isp_scalar_anti_aliasing_enhance_1st_rotate_bus_address;		// 第一次旋转
static unsigned char *isp_scalar_anti_aliasing_enhance_2nd_scalar_bus_address;		// 第二次水平缩放

// scalar缩放抗锯齿增强处理
int  arkn141_isp_scalar_anti_aliasing_enhance (
					  unsigned char *isp_scalar_anti_aliasing_enhance_1st_scalar_buffer, // 输入数据
					  int owidth, // 输出图像大小  宽度与高度
					  int oheight , 
					  int rot_w,		// 转置矩阵大小
					  
					  int source_format, // Video layer source format
					  unsigned char *yout// 输出数据
					)
{
	int ret;
#ifdef _TIME_EVALUATE_	
	XMINT64 ticket = XM_GetHighResolutionTickCount ();
	static XMINT64 scalar_2nd_ticket = 0, rotate_1st_ticket = 0, rotate_2nd_ticket = 0;
	// rotate_1st_ticket = 10908
	// scalar_2nd_ticket = 745
	// rotate_2nd_ticket = 8185	
	
	static int scalar_count = 0;
#endif
	
	
	// 其他尺寸使用缩放抗锯齿增强机制
	OS_Use (&arkn141_scalar_access_semid);
	if(do_isp_scalar_anti_aliasing_enhance)
	{
		// 检查已分配的资源是否匹配. 若不匹配, 重新分配资源
		if(isp_scalar_anti_aliasing_enhance_image_width != owidth || isp_scalar_anti_aliasing_enhance_image_height != oheight)
		{
			if(isp_scalar_anti_aliasing_enhance_1st_rotate_buffer)
			{
				kernel_free (isp_scalar_anti_aliasing_enhance_1st_rotate_buffer);
				isp_scalar_anti_aliasing_enhance_1st_rotate_buffer = NULL;
			}
			if(isp_scalar_anti_aliasing_enhance_2nd_scalar_buffer)
			{
				kernel_free (isp_scalar_anti_aliasing_enhance_2nd_scalar_buffer);
				isp_scalar_anti_aliasing_enhance_2nd_scalar_buffer = NULL;
			}
			do_isp_scalar_anti_aliasing_enhance = 0;
			isp_scalar_anti_aliasing_enhance_image_width = 0;
			isp_scalar_anti_aliasing_enhance_image_height = 0;
		}
	}
	
	if(do_isp_scalar_anti_aliasing_enhance == 0)
	{
		isp_scalar_anti_aliasing_enhance_image_width = owidth;
		isp_scalar_anti_aliasing_enhance_image_height = oheight;
		
		isp_scalar_anti_aliasing_enhance_1st_rotate_buffer = kernel_malloc (rot_w * owidth * 3/2);		// Y_UV420
		isp_scalar_anti_aliasing_enhance_2nd_scalar_buffer = kernel_malloc (oheight * owidth * 3/2);	// Y_UV420
		
		if(isp_scalar_anti_aliasing_enhance_1st_rotate_buffer)
		{
			dma_inv_range ((unsigned int)isp_scalar_anti_aliasing_enhance_1st_rotate_buffer, (unsigned int)isp_scalar_anti_aliasing_enhance_1st_rotate_buffer + rot_w * owidth * 3/2);
			isp_scalar_anti_aliasing_enhance_1st_rotate_bus_address = (unsigned char *)vaddr_to_page_addr ((unsigned int)isp_scalar_anti_aliasing_enhance_1st_rotate_buffer);
		}
		if(isp_scalar_anti_aliasing_enhance_2nd_scalar_buffer)
		{
			dma_inv_range ((unsigned int)isp_scalar_anti_aliasing_enhance_2nd_scalar_buffer, (unsigned int)isp_scalar_anti_aliasing_enhance_2nd_scalar_buffer + oheight * owidth * 3/2);
			isp_scalar_anti_aliasing_enhance_2nd_scalar_bus_address = (unsigned char *)vaddr_to_page_addr ((unsigned int)isp_scalar_anti_aliasing_enhance_2nd_scalar_buffer);
		}
		
#ifdef _TIME_EVALUATE_	
		scalar_2nd_ticket = 0;
		rotate_1st_ticket = 0;
		rotate_2nd_ticket = 0;
		scalar_count = 0;
#endif
		do_isp_scalar_anti_aliasing_enhance = 1;
	}
	
	if(	
			isp_scalar_anti_aliasing_enhance_1st_rotate_buffer == NULL
		|| isp_scalar_anti_aliasing_enhance_2nd_scalar_buffer == NULL)
	{
		OS_Unuse (&arkn141_scalar_access_semid);
		return -1;
	}

#ifdef _TIME_EVALUATE_	
	scalar_count ++;
#endif
	
#ifdef DCACHE_USE
	dma_inv_range ((unsigned int)isp_scalar_anti_aliasing_enhance_1st_scalar_buffer, (unsigned int)isp_scalar_anti_aliasing_enhance_1st_scalar_buffer + owidth * rot_w * 3/2);
	rotate_degree90 (isp_scalar_anti_aliasing_enhance_1st_scalar_buffer, isp_scalar_anti_aliasing_enhance_1st_rotate_buffer, owidth, rot_w, rot_w, owidth);
#else
	rotate_degree90 (scalar_anti_aliasing_enhance_1st_scalar_bus_address, scalar_anti_aliasing_enhance_1st_rotate_bus_address, owidth, rot_w, rot_w, owidth);
#endif
	
#ifdef _TIME_EVALUATE_	
	rotate_1st_ticket += XM_GetHighResolutionTickCount() - ticket;
	ticket = XM_GetHighResolutionTickCount();
#endif
	
#ifdef DCACHE_USE
	dma_flush_range ((unsigned int)isp_scalar_anti_aliasing_enhance_1st_rotate_buffer, (unsigned int)isp_scalar_anti_aliasing_enhance_1st_rotate_buffer + owidth * rot_w * 3/2);
	ret = arkn141_scalar_process_internal (isp_scalar_anti_aliasing_enhance_1st_rotate_buffer, 
										0, 0,
										0, 0, rot_w, owidth,
										rot_w, owidth,
										oheight, owidth, oheight,
										FORMAT_Y_UV420,
										isp_scalar_anti_aliasing_enhance_2nd_scalar_buffer,
										isp_scalar_anti_aliasing_enhance_2nd_scalar_buffer + oheight * owidth,
										NULL,
										0, 0, 0);	
	
#else
	ret = arkn141_scalar_process_internal (scalar_anti_aliasing_enhance_1st_rotate_bus_address, 
										0, 0,
										0, 0, rot_w, owidth,
										rot_w, owidth,
										oheight, owidth, oheight,
										FORMAT_Y_UV420,
										scalar_anti_aliasing_enhance_2nd_scalar_bus_address,
										scalar_anti_aliasing_enhance_2nd_scalar_bus_address + oheight * owidth,
										NULL,
										0, 0, 0);
#endif
	
	if(ret < 0)
		goto scaler_exit;
	
#ifdef _TIME_EVALUATE_	
	scalar_2nd_ticket += XM_GetHighResolutionTickCount() - ticket;
	
	ticket = XM_GetHighResolutionTickCount();
#endif
	
#ifdef DCACHE_USE
	dma_inv_range ((unsigned int)isp_scalar_anti_aliasing_enhance_2nd_scalar_buffer, (unsigned int)isp_scalar_anti_aliasing_enhance_2nd_scalar_buffer + oheight * owidth * 3/2);
	rotate_degree90 (isp_scalar_anti_aliasing_enhance_2nd_scalar_buffer, yout, oheight, owidth, owidth, oheight);
#else
	rotate_degree90 (scalar_anti_aliasing_enhance_2nd_scalar_bus_address, yout, oheight, owidth, owidth, oheight);
#endif
	dma_flush_range ((unsigned int)yout, (unsigned int)yout + owidth * oheight * 3/2);

#ifdef _TIME_EVALUATE_	
	rotate_2nd_ticket += XM_GetHighResolutionTickCount() - ticket;
	
	if(scalar_count && scalar_count % 200 == 0)
	{
		printf ("rotate_1st_ticket = %d\n", (unsigned int)(rotate_1st_ticket / scalar_count));
		printf ("scalar_2nd_ticket = %d\n", (unsigned int)(scalar_2nd_ticket / scalar_count));
		printf ("rotate_2nd_ticket = %d\n", (unsigned int)(rotate_2nd_ticket / scalar_count));
		
	}
#endif
	
scaler_exit:
	OS_Unuse (&arkn141_scalar_access_semid);
	return ret;
	
}

static int do_scalar_anti_aliasing_enhance = 0;
static unsigned int scalar_anti_aliasing_enhance_image_width;
static unsigned int scalar_anti_aliasing_enhance_image_height;
static unsigned char *scalar_anti_aliasing_enhance_1st_scalar_buffer;		// 第一次缩放
static unsigned char *scalar_anti_aliasing_enhance_1st_rotate_buffer;		// 第一次旋转
static unsigned char *scalar_anti_aliasing_enhance_2nd_scalar_buffer;		// 第二次水平缩放
static unsigned char *scalar_anti_aliasing_enhance_1st_scalar_bus_address;		// 第一次缩放
static unsigned char *scalar_anti_aliasing_enhance_1st_rotate_bus_address;		// 第一次旋转
static unsigned char *scalar_anti_aliasing_enhance_2nd_scalar_bus_address;		// 第二次水平缩放

// 执行scalar处理，直到图像处理完毕或者异常结束(超时、回写bresp error)
int  arkn141_scalar_process (
					  unsigned char *yuv, // 输入数据
                 unsigned int inwindow_enable,unsigned int owindow_enable,
					  // 开窗坐标(x,y)          开窗大小 宽度与高度
					  int window_x,int window_y,int window_width,int window_height,
  
					  int iwidth, // 输入图像大小  宽度与高度
					  int iheight ,
					  int owidth, // 输出图像大小  宽度与高度
					  int oheight,   
					  int ostride,
					  
					  int source_format, // Video layer source format
					  unsigned char *yout,// 输出数据
					  unsigned char *uout,
					  unsigned char *vout,
					  
					  unsigned int middle_finish_line_count,		// 0     表示关闭middle finish中断
					  															// 非0值 表示开启middle finish中断
					  void (*middle_finish_callback) (void *private_data, unsigned int current_line_count),	
					                                             // middle finish中断回调函数
																				//        此回调函数将在中断上下文中被调用
					  void * middle_finish_private_data          // middle finish中断回调函数的私有数据
					   
					)
{
	int ret;
	
#ifdef _TIME_EVALUATE_
	XMINT64 ticket = XM_GetHighResolutionTickCount ();
	static XMINT64 scalar_1st_ticket = 0, scalar_2nd_ticket = 0, rotate_1st_ticket = 0, rotate_2nd_ticket = 0;
	static int scalar_count = 0;
#endif
	
	// VGA以上尺寸直接使用硬件scalar, 不做软件抗锯齿修复. 
	// 	这个尺寸以上的图像缩小所导致的锯齿较轻微.
	//	其他情况(开窗)也不做软件修复
	if(	owidth >= 640 && oheight >= 480
		|| inwindow_enable 
		|| owindow_enable
		|| window_x 
		|| window_y
		|| window_width != iwidth
		|| window_height != iheight
		)
	{
old_process:
		return arkn141_scalar_process_internal (yuv, inwindow_enable, owindow_enable,
															 window_x, window_y, window_width, window_height,
															 iwidth, iheight, 
															 owidth, oheight, ostride,
															 source_format, yout, uout, vout,
															 middle_finish_line_count,
															 middle_finish_callback, middle_finish_private_data);
	}
	
	// 其他尺寸使用修复机制
	OS_Use (&arkn141_scalar_access_semid);
	if(do_scalar_anti_aliasing_enhance)
	{
		// 检查已分配的资源是否匹配. 若不匹配, 重新分配资源
		if(scalar_anti_aliasing_enhance_image_width != owidth || scalar_anti_aliasing_enhance_image_height != oheight)
		{
			if(scalar_anti_aliasing_enhance_1st_scalar_buffer)
			{
				kernel_free (scalar_anti_aliasing_enhance_1st_scalar_buffer);
				scalar_anti_aliasing_enhance_1st_scalar_buffer = NULL;
			}
			if(scalar_anti_aliasing_enhance_1st_rotate_buffer)
			{
				kernel_free (scalar_anti_aliasing_enhance_1st_rotate_buffer);
				scalar_anti_aliasing_enhance_1st_rotate_buffer = NULL;
			}
			if(scalar_anti_aliasing_enhance_2nd_scalar_buffer)
			{
				kernel_free (scalar_anti_aliasing_enhance_2nd_scalar_buffer);
				scalar_anti_aliasing_enhance_2nd_scalar_buffer = NULL;
			}
			do_scalar_anti_aliasing_enhance = 0;
			scalar_anti_aliasing_enhance_image_width = 0;
			scalar_anti_aliasing_enhance_image_height = 0;
		}
	}
	
	if(do_scalar_anti_aliasing_enhance == 0)
	{
		scalar_anti_aliasing_enhance_image_width = owidth;
		scalar_anti_aliasing_enhance_image_height = oheight;
		
		scalar_anti_aliasing_enhance_1st_scalar_buffer = kernel_malloc (owidth * ROT_W * 3/2);		// Y_UV420
		scalar_anti_aliasing_enhance_1st_rotate_buffer = kernel_malloc (ROT_W * owidth * 3/2);		// Y_UV420
		scalar_anti_aliasing_enhance_2nd_scalar_buffer = kernel_malloc (oheight * owidth * 3/2);	// Y_UV420
		
		if(scalar_anti_aliasing_enhance_1st_scalar_buffer)
		{
			dma_inv_range ((unsigned int)scalar_anti_aliasing_enhance_1st_scalar_buffer, (unsigned int)scalar_anti_aliasing_enhance_1st_scalar_buffer + owidth * ROT_W * 3/2);
			scalar_anti_aliasing_enhance_1st_scalar_bus_address = (unsigned char *)vaddr_to_page_addr ((unsigned int)scalar_anti_aliasing_enhance_1st_scalar_buffer);
		}
		if(scalar_anti_aliasing_enhance_1st_rotate_buffer)
		{
			dma_inv_range ((unsigned int)scalar_anti_aliasing_enhance_1st_rotate_buffer, (unsigned int)scalar_anti_aliasing_enhance_1st_rotate_buffer + ROT_W * owidth * 3/2);
			scalar_anti_aliasing_enhance_1st_rotate_bus_address = (unsigned char *)vaddr_to_page_addr ((unsigned int)scalar_anti_aliasing_enhance_1st_rotate_buffer);
		}
		if(scalar_anti_aliasing_enhance_2nd_scalar_buffer)
		{
			dma_inv_range ((unsigned int)scalar_anti_aliasing_enhance_2nd_scalar_buffer, (unsigned int)scalar_anti_aliasing_enhance_2nd_scalar_buffer + oheight * owidth * 3/2);
			scalar_anti_aliasing_enhance_2nd_scalar_bus_address = (unsigned char *)vaddr_to_page_addr ((unsigned int)scalar_anti_aliasing_enhance_2nd_scalar_buffer);
		}
		
#ifdef _TIME_EVALUATE_
		scalar_1st_ticket = 0;
		scalar_2nd_ticket = 0;
		rotate_1st_ticket = 0;
		rotate_2nd_ticket = 0;
		scalar_count = 0;
#endif
		
		do_scalar_anti_aliasing_enhance = 1;
	}
	
	if(	scalar_anti_aliasing_enhance_1st_scalar_buffer == NULL 
		||	scalar_anti_aliasing_enhance_1st_rotate_buffer == NULL
		|| scalar_anti_aliasing_enhance_2nd_scalar_buffer == NULL)
	{
		OS_Unuse (&arkn141_scalar_access_semid);
		goto old_process;
	}

#ifdef _TIME_EVALUATE_	
	scalar_count ++;
#endif
	ret = arkn141_scalar_process_internal (yuv, 
										0, 0,
										0, 0, window_width, window_height,
										iwidth, iheight,
										owidth, ROT_W,
										ostride,
										FORMAT_Y_UV420,
										scalar_anti_aliasing_enhance_1st_scalar_bus_address,
										scalar_anti_aliasing_enhance_1st_scalar_bus_address + owidth * ROT_W,
										0,
										0, 0, 0);
	if(ret < 0)
		goto scaler_exit;
#ifdef _TIME_EVALUATE_	
	scalar_1st_ticket += XM_GetHighResolutionTickCount() - ticket;
	ticket = XM_GetHighResolutionTickCount();
#endif
	
#ifdef DCACHE_USE
	dma_inv_range ((unsigned int)scalar_anti_aliasing_enhance_1st_scalar_buffer, (unsigned int)scalar_anti_aliasing_enhance_1st_scalar_buffer + owidth * ROT_W * 3/2);
	rotate_degree90 (scalar_anti_aliasing_enhance_1st_scalar_buffer, scalar_anti_aliasing_enhance_1st_rotate_buffer, owidth, ROT_W, ROT_W, owidth);
#else
	rotate_degree90 (scalar_anti_aliasing_enhance_1st_scalar_bus_address, scalar_anti_aliasing_enhance_1st_rotate_bus_address, owidth, ROT_W, ROT_W, owidth);
#endif
	
#ifdef _TIME_EVALUATE_	
	rotate_1st_ticket += XM_GetHighResolutionTickCount() - ticket;
	ticket = XM_GetHighResolutionTickCount();
#endif
	
#ifdef DCACHE_USE
	dma_flush_range ((unsigned int)scalar_anti_aliasing_enhance_1st_rotate_buffer, (unsigned int)scalar_anti_aliasing_enhance_1st_rotate_buffer + owidth * ROT_W * 3/2);
	ret = arkn141_scalar_process_internal (scalar_anti_aliasing_enhance_1st_rotate_buffer, 
										0, 0,
										0, 0, ROT_W, owidth,
										ROT_W, owidth,
										oheight, owidth,
										oheight,
										FORMAT_Y_UV420,
										scalar_anti_aliasing_enhance_2nd_scalar_buffer,
										scalar_anti_aliasing_enhance_2nd_scalar_buffer + oheight * owidth,
										NULL,
										0, 0, 0);	
	
#else
	ret = arkn141_scalar_process_internal (scalar_anti_aliasing_enhance_1st_rotate_bus_address, 
										0, 0,
										0, 0, ROT_W, owidth,
										ROT_W, owidth,
										oheight, owidth,
										oheight,
										FORMAT_Y_UV420,
										scalar_anti_aliasing_enhance_2nd_scalar_bus_address,
										scalar_anti_aliasing_enhance_2nd_scalar_bus_address + oheight * owidth,
										NULL,
										0, 0, 0);
#endif
	
	if(ret < 0)
		goto scaler_exit;
	
#ifdef _TIME_EVALUATE_	
	scalar_2nd_ticket += XM_GetHighResolutionTickCount() - ticket;
	
	ticket = XM_GetHighResolutionTickCount();
#endif
	
#ifdef DCACHE_USE
	dma_inv_range ((unsigned int)scalar_anti_aliasing_enhance_2nd_scalar_buffer, (unsigned int)scalar_anti_aliasing_enhance_2nd_scalar_buffer + oheight * owidth * 3/2);
	rotate_degree90 (scalar_anti_aliasing_enhance_2nd_scalar_buffer, yout, oheight, owidth, owidth, oheight);
#else
	rotate_degree90 (scalar_anti_aliasing_enhance_2nd_scalar_bus_address, yout, oheight, owidth, owidth, oheight);
#endif
	dma_flush_range ((unsigned int)yout, (unsigned int)yout + owidth * oheight * 3/2);

#ifdef _TIME_EVALUATE_	
	
	rotate_2nd_ticket += XM_GetHighResolutionTickCount() - ticket;
	
	if(scalar_count && scalar_count % 200 == 0)
	{
		printf ("scalar_1st_ticket = %d\n", (unsigned int)(scalar_1st_ticket / scalar_count));
		printf ("rotate_1st_ticket = %d\n", (unsigned int)(rotate_1st_ticket / scalar_count));
		printf ("scalar_2nd_ticket = %d\n", (unsigned int)(scalar_2nd_ticket / scalar_count));
		printf ("rotate_2nd_ticket = %d\n", (unsigned int)(rotate_2nd_ticket / scalar_count));
		
	}
#endif
	
scaler_exit:
	OS_Unuse (&arkn141_scalar_access_semid);
	return ret;
	
}