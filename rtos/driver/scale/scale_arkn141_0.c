/*
	scale.c
*/
#include "irqs.h"
#include "ark1960.h"
#include "scale_arkn141.h"
#include "RTOS.H"
#include <ark1960_testcase.h>
#include <xmfile.h>
#include <xmtype.h>
#include <xmbase.h>
#include "types.h"
#include <stdio.h>
#include <assert.h>

static OS_RSEMA arkn141_scalar_access_semid;	// ��������ź���
static OS_EVENT arkn141_scalar_finish_event;	// ����¼�

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
#if SCALE_FORMAT_YUV420   //֧��YUV420   UV���ݷֿ�	
   rAXI_SCALE_WB_DEST_VADDR = Vaddr;
#endif
}



void scale_arkn141_writeback_start()
{
   delay(50);
   rAXI_SCALE_WB_START = 0x01;/*һ����ʼ�Ĵ��������������ش���ʱ��ʼһ����scale��������д������*/
   delay(50);
   rAXI_SCALE_WB_START = 0x00;
}

//#pragma inline
void  scale_arkn141_open()
{
   //soft reset 
   rSYS_SOFT_RSTNB  &=~(1<<10);
   rSYS_SOFT_RSTNB  |=(1<<10);
 	__asm ("nop");
 	__asm ("nop");
 	__asm ("nop");
 	__asm ("nop");
   
	// ���������ź���������scalar����
	OS_CREATERSEMA (&arkn141_scalar_access_semid);
	// ����¼�
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
	// Scale �ж�
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
	// ʹ��SCALE�ж�
	// ���SCALE�ж�
	rAXI_SCALE_INT_CTL = status;
	//rAXI_SCALE_INT_CTL = 0;
	
	rAXI_SCALE_CLCD_INT_CLR = status;
   
	OS_DI();
	irq_disable ( SCALE_INT );
	request_irq(SCALE_INT, PRIORITY_FIFTEEN, ScaleIntHander);
	OS_EI();
   
	printf("Scale_Int_init IRQ\r\n");
	
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
	֧�ֿ���д��

���� 19������ 
*/

int  scalar_arkn141_useint( scale_s *scale )
{
	int status=0;
	int timeoutMax = ((scale->owidth*scale->oheight)>>10); // 944 ��ȵȲ����ж� ���ӳ�ʱ�� 
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
	/**Window ��ʾȡ��Դ��λ�� �Լ���Ⱥ͸߶� **/ /*�ײ� ���Ҳ� ��ʾ����*/
	
	/* ���� ����16����� */
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
   
	/* д��ram���ݵ�����ÿ�е��� */
   rAXI_SCALE_WB_DATA_HSIZE_RAM = scale->ostride;/*The number of the horizontal pix in write back ram, 
											it is equal or bigger than the wide of scale, x16 */

	// Source :YUV FORMAT Address set
	scale_arkn141_set_source_addr(scale->yuv ,scale->inwidth, scale->inheight, scale->source_format );
	
	scale_arkn141_set_writeback_addr((UINT32)scale->yout , (UINT32)scale->uout , (UINT32) scale->vout ) ;
	time = OS_GetTime();
	irq_enable(SCALE_INT);
	scale_arkn141_writeback_start();
	
	//����ж�����������ʱ�򷵻� ��
	// �ȴ��жϴ�����򷵻� Scale ��� 
	// �����߼� ����������λ֡�жϣ� ����MID_num�жϲŻ����
	
	if( scale->Midline&0xfff ) 
	{
		printf("MidNum:%d \r\n", scale->Midline );
		if(OS_WaitCSemaTimed(&SEMAScaleMidNum, timeoutMax ) == 0 )
		{
			ret = SCALE_RET_TIMEOUT;
			//����������� �ȹر� Scale
			status = rAXI_SCALE_INT_STATUS&0x30;
			rAXI_SCALE_EN		= 0x00;		
			printf("MidNum: status: 0x%08x  \r\n",status );
			printf("Scale timeout.time:%dms\r\n", timeoutMax );
			// ֻ�����Mid_num 
			// ��������жϣ����ж�����ᱻ����ģ�������Ϊû�н����жϣ������ֹ����
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
		//����������� �ȹر� Scale
		status = rAXI_SCALE_INT_STATUS&0x3;
		rAXI_SCALE_EN		= 0x00;		
		printf("Scale timeout.time:%dms\r\n", timeoutMax );
		printf("Frame status: 0x%08x \r\n",rAXI_SCALE_INT_STATUS );
		// ֻ�����Frmae int
		// ��������жϣ����ж�����ᱻ����ģ�������Ϊû�н����жϣ������ֹ����
		rAXI_SCALE_CLCD_INT_CLR = (1<<0);;
	}
	rAXI_SCALE_EN		= 0x0;
	
	irq_disable(SCALE_INT);
	time = OS_GetTime() - time;
	printf("Scale waste time : %d ms\n",time);
	return ret;
}// ����֧�� Slice�ж�

/*
	��ѯ��ʽ���� Scale 
*/
int  scalar_arkn141_usereq(
					  unsigned char *yuv, // ��������
					  // ��������(x,y)          ������С �����߶�
					  int window_x,int window_y,int window_width,int window_height,
  
					  int iwidth, // ����ͼ���С  �����߶�
					  int iheight ,
					  int owidth, // ���ͼ���С  �����߶�
					  int oheight , 
                 int ostride ,
					  
					  int source_format, // Video layer source format
					  unsigned char *yout,// �������
					  unsigned char *uout,
					  unsigned char *vout,
                 int Midline
					)
{
	int status=0;
	int timeout;
	//int timeoutMax = ((owidth*oheight)>>8); // 944 ��ȵȲ����ж� ���ӳ�ʱ�� 
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
	/**Window ��ʾȡ��Դ��λ�� �Լ���Ⱥ͸߶� **/ /*�ײ� ���Ҳ� ��ʾ����*/
	
	/*ˮƽ ���� ����16����� */
	rAXI_SCALE_VIDEO_SIZE = oheight<<12| owidth<<0; /*scale size */
   rAXI_SCALE_SCALE_CTL = 0<<9| 1<<8 | 1<<7| 0<<6 | 1<<5 | 0<<4 | 0<<3 | 0<<2 | 0<<1 | 0;

	rAXI_SCALE_SCALE_VXMOD = 512<<11 | 0<<0;/*V_xmod_odd_init<<11 | V_xmod_even_init */
   rAXI_SCALE_SCALE_CTL0 = 0<<18 | ((window_width)*1024/owidth)<<0; /*left cut number in line scaler<<18 | hfz */
   rAXI_SCALE_SCALE_CTL1 = 0<<18 | ((window_height)*1024/oheight)<<0; /* Up cut line number in veritical scaler <<18 | vfz */
   rAXI_SCALE_RIGHT_BOTTOM_CUT_NUM = 0<<8  | 0<<0; /* right_cut_num<<8 | bottom_cut_num */
   
	/* д��ram���ݵ�����ÿ�е��� */
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
	// ʹ�ò�ѯ��ʽ ����Scale 
	
	printf("Not Use_Scale_Int\r\n");
	// ʹ�ò�ѯ��ʽ
	// ��ʹ���жϵķ�ʽ���Scale 
   if( Midline )
   {
      while( 1 )
      {//  ���� mid_line
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
	{// ���� ֡ ����ж�
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
			// �ȹر� Scale
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

// ��ȡ��ǰ��middle finish����
unsigned int arkn141_scalar_get_current_middle_line_count (void)
{
	return rAXI_SCALE_WR_Y_VCNT;
}

// ����scale������ɺ���ж�״̬
volatile static unsigned int scale_interrupt_status;
// scale middle finish �жϻص�����
static void (*scale_middle_finish_callback) (void *private_data, unsigned int current_line_count);
// scale middle finish ˽������
static void *scale_middle_finish_private;

static void arkn141_scalar_interrupt_handler (void)
{
	// ����ж�
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
				// ����middle finish�û��Զ���ص�����
				// �磬���Դ���H264��������������������
				val = rAXI_SCALE_WR_Y_VCNT;
//				(*scale_middle_finish_callback) (scale_middle_finish_private, val&0xfff);
//				(*scale_middle_finish_callback) (scale_middle_finish_private, (val>>11)&0xfff);
			}
			// write back middle finish interupt clear
			rAXI_SCALE_CLCD_INT_CLR = (1 << 2);
		}
		
		if(status & (1 << 0))	// write back frame finish interupt
			break;

		// middle finish�жϣ������ȴ� bresp error �� frame finish 
		return;
	} while (0);
	
	// ���� write back bresp error ���� write back frame finish, scale�������׼������
	
	// �����ж�ԭ��
	scale_interrupt_status = rAXI_SCALE_INT_STATUS;
	// ����scale�����ж�
	rAXI_SCALE_INT_CTL = 0;
	// ����ж�
	rAXI_SCALE_CLCD_INT_CLR = 0x07;
	// ���ѵȴ�scalar��ɵ�����
	OS_EVENT_Set (&arkn141_scalar_finish_event);
}

// ִ��scalar����ֱ��ͼ������ϻ����쳣����(��ʱ����дbresp error)
int  arkn141_scalar_process (
					  unsigned char *yuv, // ��������
                 unsigned int inwindow_enable,unsigned int owindow_enable,
					  // ��������(x,y)          ������С �����߶�
					  int window_x,int window_y,int window_width,int window_height,
  
					  int iwidth, // ����ͼ���С  �����߶�
					  int iheight ,
					  int owidth, // ���ͼ���С  �����߶�
					  int oheight ,    
					  
					  int source_format, // Video layer source format
					  unsigned char *yout,// �������
					  unsigned char *uout,
					  unsigned char *vout,
					  
					  unsigned int middle_finish_line_count,		// 0     ��ʾ�ر�middle finish�ж�
					  															// ��0ֵ ��ʾ����middle finish�ж�
					  void (*middle_finish_callback) (void *private_data, unsigned int current_line_count),	
					                                             // middle finish�жϻص�����
																				//        �˻ص����������ж��������б�����
					  void * middle_finish_private_data          // middle finish�жϻص�������˽������
					   
					)
{
	int ret = SCALE_RET_OK;
	//int time;
    int tmp=0;

	int yuv_format = GetVideoLayerSourceFormat( source_format );
	
	if(middle_finish_line_count >= oheight)
	{
		printf("arkn141_scalar_process middle_finish_line_count > oheight! \r\n");
		return SCALE_RET_INV_PARA;		
	}
	
	// ������ʱ���
	OS_Use (&arkn141_scalar_access_semid);
	
	// scalar������׼��
	// ����scaleʱ��
#if 0
   rSYS_SOFT_RSTNB  &=~(1<<10);
   rSYS_SOFT_RSTNB  |=(1<<10);
 	__asm ("nop");
 	__asm ("nop");
 	__asm ("nop");
 	__asm ("nop");
#endif   
   // ʹ�ܿ�������ʹ�ܻ�д
	rAXI_SCALE_EN		= 0x03;
   
	
	// ��λ�¼�����ֹ��ʱ���µ��¼�����Ӱ��
	OS_EVENT_Reset (&arkn141_scalar_finish_event);
	
	scale_interrupt_status = 0;
	//time = OS_GetTime();
	do 
	{
		if(middle_finish_line_count)
		{
			// ���middle finish�жϺ���
			if(middle_finish_callback == NULL)
			{
				ret = SCALE_RET_INV_PARA;
				break;
			}
		}
		
		// ���SCALE�ж� (write back middle finish, write back bresp error, write back frame finish)
		rAXI_SCALE_CLCD_INT_CLR = 0x07;
		
		// ʹ��SCALE�ж� (write back bresp error, write back frame finish)
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
		
		// ��װ�жϴ�������
		request_irq (SCALE_INT, PRIORITY_FIFTEEN, arkn141_scalar_interrupt_handler);
		
		
		rAXI_SCALE_RESERVED = middle_finish_line_count & 0xfff;
#define scale_CONTROL_default        (0x0<<29|0x0<<25|0x0<<24|0x8<<20|0x2<<16|0x0<<12|0x0<<8|0x0<<4|0x0<<3 )
		if(source_format == FORMAT_Y_UV422 || source_format == FORMAT_Y_UV420 )
		{
          //rAXI_SCALE_CONTROL    = 0x0<<29|0x0<<25|0x0<<24|0x8<<20|0x2<<16|0x0<<12|0x0<<8|0x0<<6|0x0<<4|0x0<<3|0x1<<2| yuv_format <<0;
           rAXI_SCALE_CONTROL    = scale_CONTROL_default | 0x1<<2 | yuv_format <<0;
        }		
        else if(yuv_format == FORMAT_YUYV )
		{
            tmp = scale_CONTROL_default | FORMAT_YUYV;
			if(source_format == FORMAT_YUYV )
				rAXI_SCALE_CONTROL= tmp ;
			else if(source_format == FORMAT_YVYU )
				rAXI_SCALE_CONTROL= tmp | 0x1<<6  ;
			else if(source_format == FORMAT_UYVY )
				rAXI_SCALE_CONTROL= tmp | 0x2<<6  ;
			else if(source_format == FORMAT_VYUY )
				rAXI_SCALE_CONTROL= tmp | 0x3<<6  ;
			else
			{
				printf("yuv_format == FORMAT_YUYV NG\r\n");
				ret = SCALE_RET_INV_PARA;
				break;
			}
		}
		else
			rAXI_SCALE_CONTROL   = scale_CONTROL_default | yuv_format <<0;
		
		if( (iwidth==owidth) &&  (iheight == oheight) &&  !inwindow_enable && !owindow_enable )//test all open
			rAXI_SCALE_CONTROL |= (1<<5);
		
		rAXI_SCALE_VIDEO_SOURCE_SIZE =  iheight<<12|    iwidth<<0;  /*	height<<12 | width */
		rAXI_SCALE_VIDEO_WINDOW_POINT=  window_y<<12|   window_x<<0;   /* y_positong<<12 | x_position */
		rAXI_SCALE_VIDEO_WINDOW_SIZE = (window_height)<<12| (window_width)<<0;  /* High_window_video<<12 | wide_window_video */
		/**Window ��ʾȡ��Դ��λ�� �Լ���Ⱥ͸߶� **/ /*�ײ� ���Ҳ� ��ʾ����*/
		
		/* ���� ����16����� */
		rAXI_SCALE_VIDEO_SIZE = oheight<<12| owidth<<0; /*scale size */
		if( (iwidth==owidth) &&  (iheight == oheight) &&  !inwindow_enable && !owindow_enable )
			rAXI_SCALE_SCALE_CTL = 0<<9| 0<<8 | 1<<7| 0<<6 | 1<<5 | 0<<4 | 0<<3 | 0<<2 | 0<<1 | 0;
		else// test only open
			rAXI_SCALE_SCALE_CTL = 0<<9| 1<<8 | 1<<7| 1<<6 | 1<<5 | 0<<4 | 0<<3 | 0<<2 | 0<<1 | 0;

		rAXI_SCALE_SCALE_VXMOD = 512<<11 | 0<<0;/*V_xmod_odd_init<<11 | V_xmod_even_init */
		rAXI_SCALE_SCALE_CTL0 = 0<<18 | ((window_width<<10)/owidth); /*left cut number in line scaler<<18 | hfz */
		rAXI_SCALE_SCALE_CTL1 = 0<<18 | ((window_height<<10)/(oheight)); /* Up cut line number in veritical scaler <<18 | vfz */
		rAXI_SCALE_RIGHT_BOTTOM_CUT_NUM = 0<<8  | 0<<0; /* right_cut_num<<8 | bottom_cut_num */
		
		/* д��ram���ݵ�����ÿ�е��� */
		rAXI_SCALE_WB_DATA_HSIZE_RAM = owidth;/*The number of the horizontal pix in write back ram, 
												it is equal or bigger than the wide of scale, x16 */
	
		// Source :YUV FORMAT Address set
		scale_arkn141_set_source_addr(yuv ,iwidth, iheight, source_format );

		scale_arkn141_set_writeback_addr((UINT32)yout , (UINT32)uout,(UINT32) vout );
		
		// ����scalarת��
		scale_arkn141_writeback_start();
		
		//����ж�����������ʱ�򷵻� ��
		// �ȴ��жϴ�����򷵻� Scale ��� 
#ifdef CONFIG_ARK1960_ASIC
		if(OS_EVENT_WaitTimed (&arkn141_scalar_finish_event, 1000))
#else
		if(OS_EVENT_WaitTimed (&arkn141_scalar_finish_event, 8000))
#endif
		{
			ret = SCALE_RET_TIMEOUT ;
			printf ("\t\tscalar timeout, status=0x%08x\n", rAXI_SCALE_INT_STATUS);
		}
		// ���hw error
		else if(scale_interrupt_status & (1 << 2))
		{
			ret = SCALE_RET_HW_ERROR;
			printf ("\t\tscalar hwerror, status=0x%08x\n", rAXI_SCALE_INT_STATUS);
		}
		
	} while (0);
	
	// �ر� Scale����������д
	rAXI_SCALE_EN		= 0x00;	

	// �ر�scaleʱ��	
	
	// ��ֹscalar�ж�
	request_irq (SCALE_INT, PRIORITY_FIFTEEN, NULL);

	// ���������ʱ���
	OS_Unuse (&arkn141_scalar_access_semid);
	//time = OS_GetTime()-time;
	//printf("Scale waste time:%dms\n",time);
	
	return ret;
}// ����֧�� Frame �ж�


#include "dma.h"
#define ROTATE_COPY_DMA_CTL_L 	((0<<28)\
										|(0<<27)\
										|(0<<25)\
										|(0<<23)\
	/*TT_FC*/						|(M2M_DMAC<<20)\
	/*DST_SCATTER_EN*/			|(1<<18)\
	/*SRC_GATHER_EN */			|(0<<17)\
	/*SRC_MSIZE*/					|(DMA_BURST_SIZE_1<<14)\
	/*DEST_MSIZE*/					|(DMA_BURST_SIZE_1<<11)\
	/*SINC*/							|(0<<9)\
	/*DINC*/							|(0<<7)\
	/*SRC_TR_WIDTH*/				|(DMA_WIDTH_32<<4)\
	/*DST_TR_WIDTH*/				|(DMA_WIDTH_8<<1)\
	/*INT_EN*/						|(1<<0))

#define ROTATE_COPY_DMA_CTL_H 	(0<<12)

// 17 R/W 0x1
//	Bus Lock Bit. When active, the AHB bus master
//	signal hlock is asserted for the duration specified in
//	CFGx.LOCK_B_L.
#define ROTATE_COPY_DMA_CFG_L		((0<<31)\
										|(0<<30)\
										|(1<<17)\
										|(0<<8))


#define ROTATE_COPY_DMA_CFG_H		((0<<12)\
										|(1<<6)\
										|(1<<5)\
										|(1<<2)\
										|(1<<1))

int dma_rotate_copy (unsigned char ch, unsigned char *src_buf, unsigned char *dst_buf, unsigned int len, 
							unsigned int destination_scatter_count, 
							unsigned int destination_scatter_interval)
{
	UINT32 DmaCh;
	UINT32 DmaRequestTime = 0;
	UINT32 srcGather, dstScatter;
	do{
		DmaCh = dma_request_dma_channel(ch);
		if((DmaRequestTime++)>50000)
		{
			printf ("SD request dma channel %d failed!\n", ch);
			return 1;
		}
	}while(DmaCh == ALL_CHANNEL_RUNNING);

	//printf ("dma_m2m_copy ch=%d\n", DmaCh);
	//dma_clr_int(DmaCh);
    dma_clr_trans(DmaCh);
	dma_clr_block(DmaCh);
	dma_clr_dst_trans(DmaCh);
	dma_clr_src_trans(DmaCh);
	dma_clr_err(DmaCh);
	
	srcGather = 0;
	dstScatter =  (destination_scatter_count << 20) 
					| (destination_scatter_interval << 0)
					;
	dma_cfg_channel(DmaCh, 
					(UINT32)src_buf,
					(UINT32)dst_buf,
					0,
					ROTATE_COPY_DMA_CTL_L,
					ROTATE_COPY_DMA_CTL_H|(len/4),
					ROTATE_COPY_DMA_CFG_L|(DmaCh<<5), (ROTATE_COPY_DMA_CFG_H),
					srcGather,
					dstScatter);
	
	//printf ("dma_start_channel\n");
	dma_start_channel(DmaCh);
	
	while(!dma_transfer_over(DmaCh))
	{
	}
	
	//printf ("end of dma_start_channel\n");

	dma_clr_trans(DmaCh);
	dma_clr_block(DmaCh);
	dma_clr_dst_trans(DmaCh);
	dma_clr_src_trans(DmaCh);
	dma_clr_err(DmaCh);
	
	while(rDMA_CHEN_L & (1<<DmaCh))
	{
		
	}
	
	//printf ("wait for DMA stop\n");
	
	dma_release_channel(DmaCh);

	return 0;	
}

#define	SRAM_BASE	0x300000

// Ϊ�������ת��(W*H --> H*W)��DDR���ʵ��µĺ���bank Active/bank PreCharge����(ÿ���ֽڵĿ�������������Bank Active/bank PreCharge)
// ʹ��Ƭ��SRAMΪ�м䴢�滷��, ÿ�δ�DDR����8�����ݵ�SRAM, 
// Ȼ����8������ת��, д�뵽DDR.
// ѭ��֪���������ݴ������
void rotate_degree90_(unsigned char *y_uv420_src, 
						  unsigned char *y_uv420_dst,
						  int width, // ����ͼ���С  �����߶�
						  int height, 
						  int o_width, // ����ͼ���С  �����߶�
						  int o_height 
						 )
{
	int i, j;
	int count;
	unsigned char *y_src, *cbcr_src;
	unsigned char *y_dst, *cbcr_dst;
	assert (!(height & 0x0F));
	assert (!(width & 0x0F));
	// ÿ�ζ�8��
	count = height / 8;
	
	// ����Y����
	y_src = y_uv420_src;
	y_dst = y_uv420_dst;
	for (i = 0; i < count; i ++)
	{
		// ÿ�ζ�ȡ8�����ݵ�SRAM
		memcpy ((char *)SRAM_BASE, y_src, 8 * width);
		y_src += 8 * width;
		
		// ���д���
		unsigned char *src_ram, *dst_ddr;
		src_ram = (unsigned char *)SRAM_BASE;
		dst_ddr = y_dst;
		
		y_dst += 8;	// �ƶ�����һ��8��
		
		
		for (j = 0; j < width; j ++)
		{
			// ÿ�ζ�ȡƬ��SRAM��λ�ڲ�ͬ�е�8���ֽ�����(��ͬ��), ����д�뵽������DDR��ַ(8��������)
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
			src_ram += 1;		// ָ����һ�е�����
			
			*(unsigned int *)dst_ddr = data1;
			*(unsigned int *)(dst_ddr+4) = data2;
			dst_ddr += height;		// ָ��DDR����һ��
			
		}
	}

	// ����UV����
	cbcr_src = y_uv420_src + width * height;
	cbcr_dst = y_uv420_dst + width * height;
	for (i = 0; i < count/2; i ++)		// CbCrֻ��Y��1/2
	{
		// ÿ�ζ�ȡ8�����ݵ�SRAM
		memcpy ((char *)SRAM_BASE, cbcr_src, 8 * width);
		cbcr_src += 8 * width;
		
		// ���д���
		unsigned char *src_ram, *dst_ddr;
		src_ram = (unsigned char *)SRAM_BASE;
		dst_ddr = cbcr_dst;
		
		cbcr_dst += 8*2;	// �ƶ�����һ��8��(ÿ��2��Ԫ��, CbCr)
		
		
		for (j = 0; j < width/2; j ++)	// CbCrֻ��Y��1/2
		{
			// ÿ�ζ�ȡƬ��SRAM��λ�ڲ�ͬ�е�8���ֽ�����(��ͬ��), ����д�뵽������DDR��ַ(8��������)
			unsigned int data1, data2, data3, data4;
			unsigned char *src = (unsigned char *)src_ram;		
			data1  = *(unsigned short *)src;	// ÿ�ζ�ȡһ��CbCr
			src +=  width;		// ָ����һ��cbcr
			data1 |= (*(unsigned short *)src) << 16;
			src +=  width;
			data2  = *(unsigned short *)src;	// ÿ�ζ�ȡһ��CbCr
			src +=  width;
			data2 |= (*(unsigned short *)src) << 16;
			src +=  width;
			data3  = *(unsigned short *)src;	// ÿ�ζ�ȡһ��CbCr
			src +=  width;
			data3 |= (*(unsigned short *)src) << 16;
			src +=  width;
			data4  = *(unsigned short *)src;	// ÿ�ζ�ȡһ��CbCr
			src +=  width;
			data4 |= (*(unsigned short *)src) << 16;
			//src +=  width;
			src_ram += 2;		// ָ����һ�е�����
			
			*(unsigned int *)dst_ddr = data1;
			*(unsigned int *)(dst_ddr+4) = data2;
			*(unsigned int *)(dst_ddr+8) = data3;
			*(unsigned int *)(dst_ddr+12) = data4;
			dst_ddr += height;		// ָ��DDR����һ��
			
		}
	}
	
}

#define ROTATE_LLI_DMA_CTL_L 	(		\
	/*LLP_SRC_EN*/ 				 (1<<28)\
	/*LLP_DST_EN*/					|(1<<27)\
	/*SMS*/							|(0<<25)\
	/*DMS*/							|(0<<23)\
	/*TT_FC*/						|(M2M_DMAC<<20)\
	/*DST_SCATTER_EN*/			|(1<<18)\
	/*SRC_GATHER_EN */			|(0<<17)\
	/*SRC_MSIZE*/					|(DMA_BURST_SIZE_1<<14)\
	/*DEST_MSIZE*/					|(DMA_BURST_SIZE_1<<11)\
	/*SINC*/							|(0<<9)\
	/*DINC*/							|(0<<7)\
	/*SRC_TR_WIDTH*/				|(DMA_WIDTH_8<<4)\
	/*DST_TR_WIDTH*/				|(DMA_WIDTH_8<<1)\
	/*INT_EN*/						|(1<<0))

#define ROTATE_LLI_DMA_CTL_H 	(0<<12)

#define M2M_COPY_DMA_CFG_L		((0<<31)\
										|(0<<30)\
										|(0<<17)\
										|(0<<8))


#define M2M_COPY_DMA_CFG_H		((0<<12)\
										|(1<<6)\
										|(1<<5)\
										|(1<<2)\
										|(1<<1))



struct dw_lli	scalar_rotate_lli[1920];
struct dw_lli	scalar_reverse_rotate_lli[1920];


void dma_transfer_over_IRQHandler (UINT32 ulIRQFactors, UINT32 channel, void *channel_private_data)
{
   int ret = 0;
	int *trans_ret = (int *)channel_private_data;
	do 
	{
		if(ulIRQFactors & ERR_INT_BIT)
		{
			// This interrupt is generated when an ERROR response is received from an AHB slave on the
			// HRESP bus during a DMA transfer. 
			// In addition, the DMA transfer is cancelled and the channel is disabled.
			// DMA �쳣
			ret = 1;
		}
		dma_clr_int(channel);
	} while (0);
	
	//printf ("dma int done!\n");
	*trans_ret = ret;
	
	// ����DMA��������¼�
	Dma_TfrOver_PostSem (channel);
	
}


unsigned int s_status;
unsigned int d_status;
void rotate_degree90 (unsigned char *y_uv420, 
							unsigned char *y_uv420_out,
						  int iwidth, // ����ͼ���С  �����߶�
						  int iheight ,
						  int owidth, // ���ͼ���С  �����߶�
						  int oheight
						 )
{
	// Y
	int i;
	unsigned char *y_src, *cbcr_src;
	unsigned char *y_dst, *cbcr_dst;
	struct dw_lli *lli = scalar_rotate_lli;
    
    
	y_src = y_uv420;
	y_dst = y_uv420_out;


    //׼��Y�� ����
	for (i = 0; i < iheight; i ++ )
	{
        lli->sar = (UINT32)y_src;
		lli->dar = (UINT32)y_dst;
		lli->ctllo = ROTATE_LLI_DMA_CTL_L;
		lli->ctlhi = ROTATE_LLI_DMA_CTL_H | (iheight);//(16*512 / 4);
        if(i != (iheight - 1))
			lli->llp = (UINT32)(&scalar_rotate_lli[0] + i + 1);
		else
			lli->llp = 0;
        lli->sstat = (unsigned int)&s_status;
		lli->dstat = (unsigned int)&d_status;
      //
		//dma_rotate_copy (0, y_src, y_dst, iwidth, 1, owidth - 1);
		y_src += (iwidth);
		y_dst += 1;
	}
    
    int size = iwidth*iheight*3/2;
	dma_flush_range ((UINT32)lli, (UINT32)&lli[iheight]);
	dma_flush_range ((UINT32)y_src, (UINT32)(y_src + size));
	dma_inv_range ((UINT32)y_dst, (UINT32)(y_dst + size));
	
	int trans_ret = 0;
    UINT32 DmaCh; 
    UINT32 DmaRequestTime=0;
    do{
		DmaCh = dma_request_dma_channel( 0 );
		if((DmaRequestTime++)>50000)
		{
			printf("copy request dma channel failed!\n");
			return ;
		}
	}while(DmaCh == ALL_CHANNEL_RUNNING);

	dma_clr_int( DmaCh );

    
	register_changle_IRQHandler (DmaCh, 
										  dma_transfer_over_IRQHandler, 
										  &trans_ret,
										  (1<<TFR_INT_BIT)|(1<<ERR_INT_BIT)
										  );

	    
    dma_cfg_channel (DmaCh,
					 lli[0].sar,
					 lli[0].dar,
					 (UINT32)(&lli[0])&0xfffffffc, 
					lli[0].ctllo,
					lli[0].ctlhi,
					 M2M_COPY_DMA_CFG_L,
					 M2M_COPY_DMA_CFG_H, 
					 0, 
					 (1<<20)|(owidth - 1) );
	
	dma_start_channel (DmaCh);
	
	//return;
	
	int finished = Dma_TfrOver_PendSemEx (DmaCh, 100);
	if(finished == 1 && trans_ret == 0)
		;//printf ("lli dma ok\n");
	else
		printf ("lli dma ng\n");
			  
	

	dma_detect_ch_disable( DmaCh );
	dma_release_channel( DmaCh );
    
    
    //   uv
    //   uv
    //   uv
    //׼��UV�Ĵ��� 
    int iheighthalf = iheight/2;
	cbcr_src = y_uv420 + iwidth * iheight;
	cbcr_dst = y_uv420_out + owidth * oheight;
    lli = scalar_rotate_lli;
	for (i = 0; i < iheighthalf; i ++)
	{
        lli->sar = (UINT32)cbcr_src;
		lli->dar = (UINT32)cbcr_dst;
		lli->ctllo = ROTATE_LLI_DMA_CTL_L;
		lli->ctlhi = ROTATE_LLI_DMA_CTL_H | (iwidth);
        if(i != (iheighthalf - 1))
			lli->llp = (UINT32)(&scalar_rotate_lli[0] + i + 1);
		else
			lli->llp = 0;
        lli->sstat = (unsigned int)&s_status;
		lli->dstat = (unsigned int)&d_status;
        
		//dma_rotate_copy (0, cbcr_src, cbcr_dst, iwidth, 2, owidth - 2);
		cbcr_src += (iwidth);
		cbcr_dst += 2;
	}
    
    trans_ret = 0;
    DmaRequestTime=0;
    do{
		DmaCh = dma_request_dma_channel( 0 );
		if((DmaRequestTime++)>50000)
		{
			printf("copy request dma channel failed!\n");
			return ;
		}
	}while(DmaCh == ALL_CHANNEL_RUNNING);

	dma_clr_int( DmaCh );

    
	register_changle_IRQHandler (DmaCh, 
										  dma_transfer_over_IRQHandler, 
										  &trans_ret,
										  (1<<TFR_INT_BIT)|(1<<ERR_INT_BIT)
										  );

	    
    dma_cfg_channel (DmaCh,
					 lli[0].sar,
					 lli[0].dar,
					 (UINT32)(&lli[0])&0xfffffffc, 
					 lli[0].ctllo,
					 lli[0].ctlhi,
					 M2M_COPY_DMA_CFG_L,
					 M2M_COPY_DMA_CFG_H, 
					 0, 
					 (2<<20)|(owidth - 2) );
	
	dma_start_channel (DmaCh);
	
	//return;
	
	finished = Dma_TfrOver_PendSemEx (DmaCh, 100);
	if(finished == 1 && trans_ret == 0)
		;//printf ("lli dma ok\n");
	else
		printf ("lli dma ng\n");
			  
	dma_inv_range ((UINT32)y_dst, (UINT32)y_dst + size );

	dma_detect_ch_disable( DmaCh );
	dma_release_channel( DmaCh );
    
    
}

//  �Ա�
//  D:\Proj\SCALE��֤\SCALE������֤\TEST\03200240\0960\ 
//  G:\TEST\03200240\0960\ 
#include "fs.h"
void rotate_test_ (unsigned int dst_w, unsigned int dst_h,
						unsigned int rot_w)
{
	int src_w,src_h;
	FS_FILE *fp = NULL;
	unsigned char *raw_image;
	unsigned char *src_image, *dst_image;
	unsigned char *tar_image;
	unsigned char *fin_image;
	int ret;
	char filename[64];
	char path[64];
    XMINT64 ticket = 0;
    XMINT64 scalar_1st_ticket = 0, scalar_2nd_ticket = 0, rotate_1st_ticket = 0, rotate_2nd_ticket = 0;
	//unsigned int 
	printf ("Please Insert volume(%s) to begin\n", "mmc:");
	while(1)
	{
		int volume_status = FS_GetVolumeStatus ("mmc:");
		if(volume_status == FS_MEDIA_IS_PRESENT)
		{
			printf ("Volume(%s) present\n", "mmc:");
			break;
		}
		OS_Delay (100);
	}
	
	sprintf (filename, "\\TEST\\%04d%04d\\", dst_w, dst_h);
	check_and_create_directory (filename);
	sprintf (path, "%s\\%04d\\", filename, rot_w);
	check_and_create_directory (path);
	
	// 16�ֽڶ���
	rot_w = (rot_w + 15) & ~15;
	
	printf ("rotate\n");
	raw_image = OS_malloc (1920*1080*3/2);
	src_image = OS_malloc (dst_w*rot_w*3/2);
	dst_image = OS_malloc (rot_w*dst_w*3/2);
	tar_image = OS_malloc (dst_h*dst_w*3/2);
	fin_image = OS_malloc (dst_w*dst_h*3/2);
	if(raw_image && src_image && dst_image && tar_image && fin_image)
	{
		fp = FS_FOpen ("\\TEST\\19201080.YUV", "rb");
		if(fp)
		{
			size_t size = FS_FRead (raw_image, 1, 1920*1080*3/2, fp);
			if(size != 1920*1080*3/2)
			{
				printf ("read error\n");
			}
			else
			{
				printf ("read ok\n");
			}
			FS_FClose(fp);
		}
		
		printf ("1st scalar\n");
        ticket = XM_GetHighResolutionTickCount ();
		arkn141_scalar_process (raw_image, 
										0, 0,
										0, 0, 1920, 1080,
										1920, 1080,
										dst_w, rot_w,
										FORMAT_Y_UV420,
										src_image,
										src_image + dst_w * rot_w,
										src_image + dst_w * rot_w,
										0, 0, 0);
        scalar_1st_ticket = XM_GetHighResolutionTickCount ()-ticket;
        printf ("scalar_1st_ticket:%d\n",(unsigned int)(scalar_1st_ticket / 1));
        
		sprintf (filename, "%s\\%04d%04d.YUV", path, dst_w, rot_w);
		fp = FS_FOpen (filename, "wb");
		if(fp)
		{
			size_t size = FS_FWrite (src_image, 1, dst_w*rot_w*3/2, fp);
			if(size != dst_w*rot_w*3/2)
			{
				printf ("write error\n");
			}
			else
			{
              printf ("write ok: \n");
			}
			FS_FClose(fp);
		}
		
		printf ("1st rotate\n");
        
        ticket = XM_GetHighResolutionTickCount ();
		rotate_degree90 (src_image, dst_image, dst_w, rot_w, rot_w, dst_w);
		rotate_1st_ticket = XM_GetHighResolutionTickCount ()-ticket;
        printf ("rotate_1st_ticket:%d\n",(unsigned int)(rotate_1st_ticket / 1));
        
		sprintf (filename, "%s\\%04d%04d.YUV", path, rot_w, dst_w);
		fp = FS_FOpen (filename, "wb");
		if(fp)
		{
			FS_FWrite (dst_image, 1, rot_w*dst_w*3/2, fp);
			FS_FClose(fp);
		}
		
		printf ("2nd scalar\n");
        ticket = XM_GetHighResolutionTickCount ();
		arkn141_scalar_process (dst_image, 
										0, 0,
										0, 0, rot_w, dst_w,
										rot_w, dst_w,
										dst_h, dst_w,
										FORMAT_Y_UV420,
										tar_image,
										tar_image + dst_h * dst_w,
										NULL,
										0, 0, 0);
        scalar_2nd_ticket =XM_GetHighResolutionTickCount () -ticket ;
        printf ("scalar_2nd_ticket:%d\n",(unsigned int)(scalar_2nd_ticket / 1) );
        
		sprintf (filename, "%s\\%04d%04d.YUV", path, dst_h, dst_w);
		fp = FS_FOpen (filename, "wb");
		if(fp)
		{
			FS_FWrite (tar_image, 1, dst_h*dst_w*3/2, fp);
			FS_FClose(fp);
		}
		
		printf ("2nd rotate\n");
        
        ticket = XM_GetHighResolutionTickCount ();
		rotate_degree90 (tar_image, fin_image, dst_h, dst_w, dst_w, dst_h);
        rotate_2nd_ticket =XM_GetHighResolutionTickCount () -ticket ;
        printf ("rotate_2nd_ticket:%d\n",(unsigned int)(rotate_2nd_ticket / 1) );
        
		printf ("reverse rotate end\n");
		sprintf (filename, "%s\\%04d%04d.YUV", path, dst_w, dst_h );
		fp = FS_FOpen (filename, "wb");
		if(fp)
		{
			FS_FWrite (fin_image, 1, dst_w*dst_h*3/2, fp);
			FS_FClose(fp);
		}
	}	
	
	printf ("end\n");
	OS_free (raw_image);
	OS_free (src_image);
	OS_free (dst_image);
	OS_free (tar_image);
	OS_free (fin_image);
	printf ("rotate (%d) end\n", rot_w);
}

void rotate_test (void)
{
	rotate_test_ (320, 240, 1080);
	rotate_test_ (320, 240, 960);
	rotate_test_ (320, 240, 800);
	rotate_test_ (320, 240, 720);
	rotate_test_ (320, 240, 640);
	rotate_test_ (320, 240, 560);

	rotate_test_ (480, 272, 1080);
	rotate_test_ (480, 272, 960);
	rotate_test_ (480, 272, 800);
	rotate_test_ (480, 272, 720);
	rotate_test_ (480, 272, 640);
	rotate_test_ (480, 272, 560);
	
	rotate_test_ (640, 480, 1080);
	rotate_test_ (640, 480, 960);
	rotate_test_ (640, 480, 800);
	rotate_test_ (640, 480, 720);
	rotate_test_ (640, 480, 640);
	rotate_test_ (640, 480, 560);
}

#if 0

#define ROTATE_LLI_DMA_CTL_L 	(		\
	/*LLP_SRC_EN*/ 				 (1<<28)\
	/*LLP_DST_EN*/					|(1<<27)\
	/*SMS*/							|(0<<25)\
	/*DMS*/							|(0<<23)\
	/*TT_FC*/						|(M2M_DMAC<<20)\
	/*DST_SCATTER_EN*/			|(1<<18)\
	/*SRC_GATHER_EN */			|(0<<17)\
	/*SRC_MSIZE*/					|(DMA_BURST_SIZE_1<<14)\
	/*DEST_MSIZE*/					|(DMA_BURST_SIZE_1<<11)\
	/*SINC*/							|(0<<9)\
	/*DINC*/							|(0<<7)\
	/*SRC_TR_WIDTH*/				|(DMA_WIDTH_32<<4)\
	/*DST_TR_WIDTH*/				|(DMA_WIDTH_8<<1)\
	/*INT_EN*/						|(1<<0))

#define ROTATE_LLI_DMA_CTL_H 	(0<<12)

struct dw_lli	scalar_rotate_lli[1920];
struct dw_lli	scalar_reverse_rotate_lli[1920];

int scalar_rotate ( unsigned char *y_uv420_src, 
						  unsigned char *y_uv420_dst,
						  int iwidth, // ����ͼ���С  �����߶�
						  int iheight ,
						  int owidth, // ���ͼ���С  �����߶�
						  int oheight,
						  struct dw_lli *scalar_lli
						 )
{
	// Y
	int i;
	unsigned char *y_src, *cbcr_src;
	unsigned char *y_dst, *cbcr_dst;
	struct dw_lli *lli = scalar_lli;
	
	int lli_count = iheight;
	
	y_src = y_uv420_src;
	y_dst = y_uv420_dst;
	for (i = 0; i < iheight; i ++)
	{
		lli->sar = y_src;
		lli->dar = y_dst;
		if(i != (lli_count - 1))
			lli->llp = (UINT32)(scalar_lli + i + 1);
		else
			lli->llp = 0;
		lli->ctllo = ROTATE_LLI_DMA_CTL_L;
		lli->ctlhi = ROTATE_LLI_DMA_CTL_H | (MAX_BATCH_TRANS_SECTOR*512 / 4);
		
		dma_rotate_copy (0, y_src, y_dst, iwidth, 1, owidth - 1);
		y_src += iwidth;
		y_dst += 1;
	}
    
	//uv
	cbcr_src = y_uv420 + iwidth * iheight;
	cbcr_dst = y_uv420_out + owidth * oheight;
	for (i = 0; i < iheight/2; i ++)
	{
		dma_rotate_copy (0, cbcr_src, cbcr_dst, iwidth, 2, owidth - 2);
		cbcr_src += iwidth;
		cbcr_dst += 2;
	}
}
#endif