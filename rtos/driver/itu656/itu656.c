#include "RTOS.H"
#include "ark1960.h"
#include "irqs.h"
#include <xm_queue.h>
#include "lcd.h"
#include "itu656.h"
#include <assert.h>

/*
����㼯�У�
o	1.   ��PAL�� ����ʼ ��ֻ��ż ���жϳ��� ���泡û���ж�
o	2.		����Ƿ�����д��ַ��������,��ַ�������
o  3.   ��ʱ����������ͳ�������д���
   4.   �л�buffer�Ƿ����
*/


//#define  SLICE_MODE
#define  ITU656_SUPPORT_POP_ERROR
#define  ITU656_SWITCH_BUFFER   // �л�buffer
#define  ITU656_SUPPORT_LCD_DISPLAY //itu656 �Ƿ�֧�� LCD


//#define  limit_num_field   // ֻ���е� ָ�������Ļ���


// �����˶��ٸ�Buffer
#define  ITU656_BUF_MAX    5
#define  ITU656_BUF_BAK    1024

// ������һЩ�����ñ���
static unsigned int oddline = 0;
static unsigned int evenline = 0;
static unsigned int fieldcount = 0;
static unsigned int slicecount = 0;
static unsigned int sliceline = 600;

static unsigned int frameline = 0;



static unsigned char *ITU656buffer;



static itudata_s  itu656data[ITU656_BUF_MAX]={0};
/*display_itu656 :��ǰ�Ѿ���ɵ�֡���� ��ҪӦ�õ���ʾ */
static itudata_s  *display_itu656 =  NULL ;//display_itu656

static itu656_window_s itu656_window={0};

void ITU656in_SelPad( void )
{
	//ʹ�� pad:itu_b 
	/* ITU656 padlist : ʹ��9����: itu_b_clk + ������ITU_B:0-7  */
	unsigned int val;
	val = rSYS_PAD_CTRL01 ;
	val &= ~((0x7<<15)|(0x7<<24)|(0x7<<27));
	val |=((1<<15)|(1<<24)|(1<<27)); // clk  din0 din1
	rSYS_PAD_CTRL01 = val ;
	val = rSYS_PAD_CTRL02;
	val &= 0xfffc0000;
	// =  0b 00 1001 0010 0100 1001 (from din2 to din7) 6 pin
	val |= 0x9249;  
	rSYS_PAD_CTRL02 = val;
}

void ITU601in_SelPad()
{
	unsigned int val;
	
	val = rSYS_PAD_CTRL01 ;
	val &=0xc0007fff;
	//     ituclk| vsync | hsync | din0  | din1
	val |=(1<<15)|(1<<18)|(1<<21)|(1<<24)|(1<<27) ;
	rSYS_PAD_CTRL01 = val;
   
	val = rSYS_PAD_CTRL02 ;
	val &=0xfffc0000;
	//     din2 | din3 | din4 | din5 | din6  | din7
	val |=(1<<0)|(1<<3)|(1<<6)|(1<<9)|(1<<12)|(1<<15) ;
	rSYS_PAD_CTRL02 = val;
   
}


void 	ITU601in_SelPad_clk()
{
   unsigned int val;
	
	val = rSYS_PAD_CTRL01 ;
	val &=0xc0007fff;
	//     ituclk| vsync | hsync | din0  | din1
	val |= (1<<15) ;
	rSYS_PAD_CTRL01 = val;
}

void ITU656in_UnSelPad( void )
{
	//ʹ�� pad:itu_b 
	/* ITU656 padlist : ʹ��9����: itu_b_clk + ������ITU_B:0-7  */
	unsigned int val;
	val = rSYS_PAD_CTRL01 ;
	//            clk | din0    |  din1
	//val &= ~((0x7<<15)|(0x7<<24)|(0x7<<27));
   
   val &= ~((0x0<<15)|(0x7<<24)|(0x7<<27));
	rSYS_PAD_CTRL01 = val ;
   
	val = rSYS_PAD_CTRL02;
	val &= 0xfffc0000;
	// = 0b 00 1001 0010 0100 1001 (from din2 to din7) 6 pin
	//val |= 0x9249;  
	rSYS_PAD_CTRL02 = val;
}

void ITU601in_UnSelPad()
{
	unsigned int val;
	
	val = rSYS_PAD_CTRL01 ;
	val &=0xc0007fff;
	//     ituclk| vsync | hsync | din0  | din1
//	val |=(1<<15)|(1<<18)|(1<<21)|(1<<24)|(1<<27) ;
	rSYS_PAD_CTRL01 = val;
   
	val = rSYS_PAD_CTRL02 ;
	val &=0xfffc0000;
	//     din2 | din3 | din4 | din5 | din6  | din7
//	val |=(1<<0)|(1<<3)|(1<<6)|(1<<9)|(1<<12)|(1<<15) ;
	rSYS_PAD_CTRL02 = val;
   
}

itu656_window_s *ITU656_Get_Window()
{
   return &itu656_window;
}

void ITU656_Set_Window( itu656_window_s *window )
{
   rITU656IN_H_CUT  = ((window->Left_cut&0x7ff)<<16) | (window->right_cut&0x7ff) ;
	rITU656IN_V_CUT  = ((window->up_cut&0x7ff)  <<16) | (window->down_cut &0x7ff) ;
}

static unsigned int itu656_frame=0;

void Reset_itu656_frame()
{
   itu656_frame = 0;
}
unsigned int return_itu656_frame()
{
   return itu656_frame;
}


int int_count=0;

static char bufferno = 0;
/*******************************************************************************
*void ITU656_fill_DataAddress(int count )
*    ����Ϊ�� rITU656IN_MODULE_STATUS �Ĵ����ж�����FIFO��ʣ������ݸ���
*    һ��ȡֵ��Χ��0 1 2 3 4 ��5����ֵ ����ʾ��ַ�Ĵ����е�ַ��ʣ�¶�����
*    ����Ϊ 0 ʱ�򣬸ú���������������Y UV��ַ��ÿ��������ֵ����Y��ַ��UV��ַ
*    ����Ϊ 4 ʱ�򣬸ú���������д�κε�ַ 
********************************************************************************/
void ITU656_fill_DataAddress(int count )
{
	unsigned int addr = rITU656IN_DRAM_Y_ADDR;
	// ��ʾ��ǰ�� FIFO�л��ж������ַ���� 
	//unsigned int count = (rITU656IN_MODULE_STATUS>>29);
	unsigned int val;
	int MAXfill =0;
	itudata_s *data = display_itu656;
//	printf("addr=0x%08x\n",	addr);
	
	//������д�ĸ�����
	MAXfill = 4-count ;
	if( MAXfill == 0 )
			return;	
	if( MAXfill > ITU656_BUF_MAX )
		MAXfill = ITU656_BUF_MAX-1 ;

	if( (count +1) < ITU656_BUF_MAX )
	{
#ifdef ITU656_SWITCH_BUFFER
		int no = 0;
		// ÿ���������Ԫ���� next,����������һ��������Ԫ����prev������������һ��
		// ��λ����ǰ���FIFO����������β��λ��
		// ��������Ҫ����FIFO�ĵ�һ����Ԫ��

		//1. ��λ����ǰFIFOλ��
		for( int no = 0; no < ITU656_BUF_MAX; no ++  )
		{
			if( data->Ydata == addr )
			{
            // ��ǰ�Ѿ���ɵ�֡���ݣ�����������ʾ  
       //     display_itu656 = data->prev;
            break;
         }
			data = data->next;//��һ������FIFO����Ԫ��
		}
		// no =0;
		//2. ����count���� ��λ�� Ҫ �����������ݵ�λ�� 
		// ITU656_BUF_MAX-(count+1) ��ʾ��ǰ���е���Ԫ�� 
		for( int no = 0 ; no < ITU656_BUF_MAX-(count+1) ; no ++  )
			data = data->prev;//��һ������FIFO����Ԫ��
#endif
		
		//3. ����Ŀǰ��ʣ��Ŀ��е�ַ
		for( int no = 0; no < MAXfill ; no ++  )
		{
#ifdef ITU656_SWITCH_BUFFER
			rITU656IN_DRAM_Y_ADDR = data->Ydata;
			rITU656IN_DRAM_Y_ADDR = data->Pbprdata; 
			if(data->Ydata <0x80000000 && data->Pbprdata<0x80000000)
			{
				printf("data->Ydata < 0x80000000 \n");
			}
			data = data->next;
#else
			rITU656IN_DRAM_Y_ADDR = itu656data[0].Ydata;
			rITU656IN_DRAM_Y_ADDR = itu656data[0].Pbprdata;
#endif
		//	printf("Fill addr 0x%08x 0x%08x\n", data->Ydata ,data->Pbprdata );
//			printf("Fill 0x%08x\n", data->Ydata   );
			

		}
	//	printf("fill end.\n");
	}

}
/*******************************************************************************
*  ITU656_DataAddress_Reg_Init(  )
*  ���ڸ�λ֮��ĵ�һ�� ����ַFIFO����ַ����
********************************************************************************/
static void ITU656_DataAddress_Reg_Init(  )
{
	//��λ�� ��һ�β�������������������������Buffer�ĵ�ַ 
	itudata_s *data = itu656data;
	//����Ŀǰ��ʣ��Ŀ��е�ַ
	for( int no = 0; no < 4 ; no ++  )
	{
		rITU656IN_DRAM_Y_ADDR = data->Ydata;
		rITU656IN_DRAM_Y_ADDR = data->Pbprdata; 
  //    printf("Y:0x%08x\n",data->Ydata);
  //    printf("U:0x%08x\n",data->Pbprdata);
		data = data->next;
	}
//	printf("push fifo=%d Mod-status:0x%08x ISR:0x%08x\n",(rITU656IN_MODULE_STATUS>>29), rITU656IN_MODULE_STATUS, rITU656IN_ISR );
}

/*******************************************************************************
* ITU656in_close()
*  �ر� itu656 �����ź�
*  ���� 0�� ��ǰ���ܹر�
*  ���� 1:  ��ǰ���Թر�
* ��lcd���������ʹ��
********************************************************************************/
unsigned int ITU656in_disable( )
{
#define  status_val    ((0xf<<24)|(0x7<<20)|(0x3<<16))
	/* ����״̬��Ϊ���ʱ�򣬲�����и�λ�������κβ��� */
	/* 
		Axi_state:4bit:bit.27:bit.24
		Aw_state: 3bit:bit.22:bit.20
		W_state : 2bit:bit.17:bit.16
	*/
   if( (rITU656IN_ISR & (1<<3)) )
   {
      rITU656IN_IMR = 0;
      rITU656IN_ENABLE_REG &=  0xfffffffe ;
   }
	//
	if( (rITU656IN_MODULE_STATUS & status_val ) == 0 )  
	{
      return 1;
   }
	else
	{
      printf("itu656 status = 0x%08x\n", rITU656IN_MODULE_STATUS );
      return 0;
   }
}


itudata_s *ITU656in_GetDispAddress()
{
#ifdef ITU656_SWITCH_BUFFER
	return display_itu656;
#else
	return &itu656data[0];
#endif
}


void output_register_YCbcr_addr()
{
	unsigned int addrY = rITU656IN_DRAM_Y_ADDR;
	unsigned int addrCbcr = rITU656IN_DRAM_CBCR_ADDR;
	printf("  Y:0x%08x\n", addrY );
	printf("  Pbpr:0x%08x\n", addrCbcr );
}

void set_display_itu656_addr()
{
   unsigned int addr;
   unsigned int no;
   addr = rITU656IN_DRAM_Y_ADDR ;
   // һ��ֻ��Ҫִ��һ��
   for(no=0 ;no < ITU656_BUF_MAX ; no++ )
   {
      if(display_itu656->next->Ydata == addr)
         break;

      display_itu656 = display_itu656->next ; 
   }
   
    
}


void ITU656in_int_fieldhandler()
{// ��ģʽ ����
	unsigned int rdata = rITU656IN_ISR;
	unsigned int val ;
	unsigned int itu656_int_mask ;
	
	//printf("int %x\n",rdata);
	rITU656IN_ICR  = rdata; 
#ifdef limit_num_field
	if(fieldcount >= ITU656_BUF_MAX )
	{
		rITU656IN_ENABLE_REG =0;
		rITU656IN_IMR = 0;
		return ;
	}
#endif
	//  ��һ�λ�����ʽ�л���ʱ�򣬾��ܽ��� 1<<7 �ж� ��debounce��
	if( rdata &(1<<7))
	{
		//printf("int %x\n",rdata);
		// field       //bit.7:line num changed //bit.9:slice 
#ifndef SLICE_MODE
		itu656_int_mask =  (1<<0) | (1<<3) | (1<<7)    ;
#else
		itu656_int_mask =  (1<<0) | (1<<3) | (1<<7) |(1<<9)   ;
		rITU656IN_SLICE_PIXEL_NUM = 720*sliceline;//�жϲ��� ��д��д����һ��Ч��
#endif
#ifdef ITU656_SUPPORT_POP_ERROR
		itu656_int_mask |=  (1<<10)|(1<<2)|(1<<8) ;//bit.8 field start //bit.10=pop error //bit.2: push error 
		rITU656IN_IMR = itu656_int_mask;;
#else
		rITU656IN_IMR = itu656_int_mask;
#endif
	//	rITU656IN_IMR = 0x3ff;
    	rITU656IN_TOTAL_PIX = ((rITU656IN_LINE_NUM_PER_FIELD-10)&0x7fe)*(rITU656IN_PIX_NUM_PER_LINE&0xfff)/2;

      // 20150515 �°汾����
      rITU656IN_DATA_OUT_LINE_NUM_PER_FIELD = (rITU656IN_LINE_NUM_PER_FIELD-10)&0x7ff;

		rITU656IN_ENABLE_REG |= ( 1<<0 );
	}
	

	if( rdata & 1 )// odd field
	{
		//ITU656_fill_DataAddress( 3 );//����һ����ַ
		unsigned int count = (rITU656IN_MODULE_STATUS>>29);
//		printf("fifo=%d\n",count);

		set_display_itu656_addr();
      
		oddline += ( rITU656IN_LINE_NUM_PER_FIELD&0x7ff  );
		fieldcount ++;
      itu656_frame ++;
	}

	if (rdata & (1<<3))// even field : use for frame
	{
		//ITU656_fill_DataAddress( 3 );//����һ����ַ
		unsigned int count = (rITU656IN_MODULE_STATUS>>29);

      set_display_itu656_addr();

		evenline += (  rITU656IN_LINE_NUM_PER_FIELD&0x7ff  );
		fieldcount ++;
      
      itu656_frame ++;
	}
#ifdef ITU656_SUPPORT_POP_ERROR
	if (rdata & (1<<10))//bit.10=pop error 
	{
		unsigned int count = (rITU656IN_MODULE_STATUS>>29);
		printf("pop error: %d\n",count );
	}
	if (rdata & (1<<2))//bit.10=pop error 
	{
		unsigned int count = (rITU656IN_MODULE_STATUS>>29);
		printf("push error:%d\n",count );
      
	}
	if( rdata & (1<<8) )// field start 
	{
	}
#endif
#ifdef SLICE_MODE
	if( rdata & (1<<9) )
	{
		slicecount ++;
	}
#endif

	
}

void ITU656in_int_framehandler()
{// ֡ģʽ ����
	
	// ��ʱ��ż�����泡״̬�����
	unsigned int itu656_int_mask ;
	unsigned int rdata = rITU656IN_ISR;
	printf("int %x\n",rdata);
	
#ifdef limit_num_field
	if(fieldcount >= ITU656_BUF_MAX )
	{
		rITU656IN_ENABLE_REG =0;
		rITU656IN_IMR = 0;
		return ;
	}
#endif
	
	//  ��һ�λ�����ʽ�л���ʱ�򣬾��ܽ��� 1<<7 �ж� ��debounce��
	if( rdata &(1<<7) )
	{
		// frame  
      rITU656IN_IMR = (1<<8);
    	rITU656IN_TOTAL_PIX = ((rITU656IN_LINE_NUM_PER_FIELD-10)&0x7fe)
                              *(rITU656IN_PIX_NUM_PER_LINE&0xfff)/2;
      // 20150515 �°汾����
      rITU656IN_DATA_OUT_LINE_NUM_PER_FIELD = (rITU656IN_LINE_NUM_PER_FIELD-10)&0x7ff;
	}
	
	// ����֡ģʽ�� Ϊ��ֻ����even�жϣ����ƽ���odd�жϴ���
	if (rdata & (1<<3))// even field : use for frame
	{
		unsigned int count = (rITU656IN_MODULE_STATUS>>29);
      
      // ��õ�ǰ��ɵ� ֡ 
      set_display_itu656_addr();

		frameline += (  rITU656IN_LINE_NUM_PER_FIELD&0x7ff   );
		fieldcount ++ ;
      
      itu656_frame ++;
	}
	
#ifdef ITU656_SUPPORT_POP_ERROR
	if (rdata & (1<<10))//bit.10=pop error 
	{
		unsigned int count = (rITU656IN_MODULE_STATUS>>29);
		printf("pop error=%d\n",count);
	}
#endif
	if (rdata & (1<<2))//bit.2=push error 
	{
		unsigned int count = (rITU656IN_MODULE_STATUS>>29);
      rITU656IN_ICR  = (1<<2); 
		printf("push error=%d Mod-status:0x%08x ISR:0x%08x Y:0x%08x U:0x%08x\n",count, rITU656IN_MODULE_STATUS, rdata ,rITU656IN_DRAM_Y_ADDR, rITU656IN_DRAM_CBCR_ADDR);
	}
	if (rdata & (1<<8))//bit.2=field start
	{
      if( !(rITU656IN_ISR & (1<<11)) )
      {  
         rITU656IN_ENABLE_REG |= ( 1<<0 )|(1<<12);
         rITU656IN_IMR &= ~(1<<8);
#ifndef SLICE_MODE
		   itu656_int_mask =   (1<<3) | (1<<7)   ;// bit.3:even bit //bit.9:slice
#else
		   itu656_int_mask =   (1<<3) | (1<<7) | (1<<9)  ;// bit.3:even bit //bit.9:slice
		   rITU656IN_SLICE_PIXEL_NUM = 720*sliceline;//�жϲ��� ��д��д����һ��Ч��
#endif
#ifdef ITU656_SUPPORT_POP_ERROR
		   itu656_int_mask |=  (1<<10)|(1<<2) ;//bit.10=pop error// bit.2:push error
		   rITU656IN_IMR = itu656_int_mask;;
#else
		   rITU656IN_IMR = itu656_int_mask;
#endif
 //        rITU656IN_IMR |= (1<<10)|(1<<2)|(1<<3) | (1<<7);
      }
	}
#ifdef SLICE_MODE
	if( rdata & (1<<9) )
	{
		slicecount ++;
	}
#endif
	
	rITU656IN_ICR  = rdata; 
}

void ITU656_close()
{
	irq_disable( ITU656_INT );//
	irq_disable( LCD_INT );//
	//rITU656IN_ENABLE_REG &= ~( 1<<0 );
}

void ITU656_reset_soft()
{
#define  ITU656_reset_soft_delay    200
   volatile unsigned int count = ITU656_reset_soft_delay;
   /*sysbase+0x74  bit.19,Ĭ����1 ��reset:����0������1 */
	rSYS_SOFT_RSTNA &= ~(0x1<<19);
   while(count--);


	rSYS_SOFT_RSTNA |= (0x1<<19);
   count = ITU656_reset_soft_delay;
   while(count--);

}

void ITU656_reset_frame( itu656_window_s *window )
{
	unsigned int osd_layer = 2;
	itudata_s *data;
	// clear pad
	ITU656in_UnSelPad();
   ITU601in_SelPad_clk();// ֻѡ�� clk
   // soft reset 
	ITU656_reset_soft();
//   printf("push fifo=%d Mod-status:0x%08x ISR:0x%08x\n",(rITU656IN_MODULE_STATUS>>29), rITU656IN_MODULE_STATUS, rITU656IN_ISR );


	/*Pad ��ѡ�������� ���������źŸ��� ��Ӱ�촦�� */
	
	// SYS_DEVICE_CLK_CFG0
	// 9	R/W	0	Itu_clk_b_sel
	// 				1:itu_clk_b_inv
	//					0:itu_clk_b
	rSYS_DEVICE_CLK_CFG0 |= (0x1<<9);
	
	// SYS_AHB_CLK_EN
	// 18	R/W	1	itu656_xclk_enable
	//					1: enable
	//					0: disable
	rSYS_AHB_CLK_EN |= (1 << 18);
	
	// SYS_PER_CLK_EN
	// 24	R/W	1	Itu_b_clk enable(itu656_clk_in)
	//					1: enable
	//					0: disable
	rSYS_PER_CLK_EN |= (1 << 24);
	
	
	rITU656IN_ModuleEn = 1<<5| 0x1<<1 |0x1;

	printf("Frame MODE\n");
   printf("Function:%s!\n",__FUNCTION__);

	LCD_Layer_Size(osd_layer,720,480);
	
	LCD_Layer_fill_address(osd_layer ,itu656data[0].Ydata, itu656data[0].Pbprdata, 0);

	LCD_cpu_osd_coef_syn_reg = 7 ;

	rITU656IN_INPUT_SEL = 0x01;
	rITU656IN_IMR = 0x0; 
	rITU656IN_ICR = 0x3ff;
	rITU656IN_SLICE_PIXEL_NUM = 0;
	
	//	frame                        //bit.5: 0.field mode or 1.frame mode
	rITU656IN_ENABLE_REG = (0<<13) |(0<<12)| (1<<11)|(1<<6) |(1<<5) | (0<<3) | (1<<1) | 0;

	//                 ����֡�� |֡���в���| �м����ز���
	rITU656IN_DELTA_NUM = 5<<16 | (10<<8)  | 20;
	rITU656IN_TOTAL_PIX = 0;//720*576;
	// �״θ�λ�� Ҫ������������
	ITU656_DataAddress_Reg_Init();


	request_irq(ITU656_INT, PRIORITY_FIFTEEN, ITU656in_int_framehandler);
	
	rITU656IN_IMR =  (1<<7);// �����仯�ж�
	//rITU656IN_IMR = 0x3ff;
	
	rITU656IN_SIZE = 720<<16;  //�������� 0x2d0 = 720
	
	rITU656IN_ENABLE_REG |= (3<<1);// | (1<<0) ;//bit.3:Axi_cmd_max
	
#ifndef ITU656_SWITCH_BUFFER	//�꿪�����壺���û�ж����л�buffer
	rITU656IN_DRAM_Y_ADDR = itu656data[0].Ydata;
	rITU656IN_DRAM_Y_ADDR =  itu656data[0].Pbprdata;
#endif
	ITU656in_SelPad();
//   printf("ITU656in_SelPad \n");

}

void ITU656_reset_field()
{
	unsigned int osd_layer = 2;
	itudata_s *data;
	// clear pad
	ITU656in_UnSelPad();

   ITU601in_SelPad_clk();// ֻѡ�� clk
	ITU656_reset_soft();
	

	rSYS_DEVICE_CLK_CFG0 |= (0x1<<9);
   //rSYS_DEVICE_CLK_CFG0 &= ~(0x1<<9);
	
	rITU656IN_ModuleEn = 1<<5| 0x1<<1 |0x1;


	printf("Field MODE\n");
   printf("Function:%s!\n",__FUNCTION__);
	irq_enable( LCD_INT );
	LCD_Layer_Size(osd_layer,720,288);

	// ��һ����ʾ���õ�һ����ַ��ʼ
	LCD_Layer_fill_address(osd_layer ,itu656data[0].Ydata, itu656data[0].Pbprdata, 0);

	LCD_cpu_osd_coef_syn_reg = 7 ;
	
	rITU656IN_INPUT_SEL = 0x01 ;// //0 : sel itu601 input 1 : select itu656 input
	rITU656IN_IMR = 0x0 ; 
	rITU656IN_ICR = 0x3ff ;
	rITU656IN_SLICE_PIXEL_NUM = 0 ;
	
	// field                        //bit.5: 0.field mode or 1.frame mode
	//rITU656IN_ENABLE_REG = (0<<13) | (1<<11) |(0<<5) | (1<<3) | (1<<1) | 1;
	rITU656IN_ENABLE_REG = (0<<13)|(1<<12)|  (1<<11) |(0<<5) | (0<<3) | (1<<1) | 0;
	
	//                          ֡���в���| �м����ز���
	rITU656IN_DELTA_NUM = 5<<16 | (10<<8) | 20;
	rITU656IN_TOTAL_PIX = 0;//720*576;
	// ���������ַ  Y��UV��ַ��ÿ������
	ITU656_DataAddress_Reg_Init(  );
	
	request_irq(ITU656_INT, PRIORITY_FIFTEEN, ITU656in_int_fieldhandler);

	rITU656IN_IMR =  (1<<7);// �����仯�жϣ�ÿ֡��Ч�б仯��
	//rITU656IN_IMR = 0x3ff;
	
	rITU656IN_SIZE = 720<<16;  //�����п� 0x2d0 = 720
	
	rITU656IN_ENABLE_REG |=  (3<<1) ;// ÿһ��д������� ����
	
#ifndef ITU656_SWITCH_BUFFER	//�꿪�����壺���û�ж����л�buffer
	rITU656IN_DRAM_Y_ADDR = itu656data[0].Ydata;
	rITU656IN_DRAM_Y_ADDR =  itu656data[0].Pbprdata;
#endif
	ITU656in_SelPad();
}


void ITU656in_buffer_init(unsigned int width, unsigned int height )
{
	unsigned int  val = 0 ;
	unsigned char *frame ;
   unsigned int size = width*height ;
   unsigned int yuvsize = size*3/2 ;
   printf("buffer width:%d height:%d\n" ,  width , height ) ;

	ITU656buffer = (unsigned char *)malloc(yuvsize*ITU656_BUF_MAX + ITU656_BUF_BAK); //622080 = 720*576*3/2
	if( !ITU656buffer )
		printf("ITU656 malloc buffer failured!\n");

	frame = (unsigned char*)(((unsigned int)ITU656buffer+ITU656_BUF_BAK)&0xffffff00);
   memset( frame , 0 , yuvsize*ITU656_BUF_MAX ) ;

	itu656data[val].Ydata =  (unsigned int )frame;
	itu656data[val].Pbprdata = (unsigned int )(frame+ size );//414720 = 720*576 //+ offset Ydata area

	printf("malloc:0x%08x 0x%08x %d\n",itu656data[val].Ydata, itu656data[val].Pbprdata  , val);
	
	for(val = 1 ; val < ITU656_BUF_MAX ; val ++ )
	{
		frame +=  yuvsize;//622080 = 720*576*3/2
		itu656data[val].Ydata =  (unsigned int )frame;
		itu656data[val].Pbprdata = (unsigned int )(frame+ size );//414720 = 720*576
		itu656data[val-1].next = &itu656data[val];
		itu656data[val].prev = &itu656data[val-1];
		
		printf("malloc:0x%08x 0x%08x %d\n",itu656data[val].Ydata, itu656data[val].Pbprdata  , val);
		if( !frame   )
			printf("ITU656 malloc memory failured !\n");
	}
	
	itu656data[0].prev = &itu656data[ITU656_BUF_MAX-1];
	itu656data[ITU656_BUF_MAX-1].next = &itu656data[0];
	
	printf("malloc:0x%08x end\n", (unsigned int ) (frame+yuvsize) ) ;
   
   
   if( !display_itu656 )
      display_itu656 = itu656data;
}

void ITU656in_Config_field( itu656_window_s *window )
{ 
	int rdata;
	int lastno;
	int osd_layer = 2 ;
	unsigned char *addr ;
	unsigned int time = 0;
   unsigned int osd_w,osd_h;
	itudata_s *data;
	bufferno = 0 ;
	oddline = 0;
	evenline = 0;
	frameline = 0;
	fieldcount = 0;
   
   //ָ�� OSD �Ŀ�� �߶� 
   osd_w = window->real_w ;
   osd_h = window->real_h ;

	slicecount = 0;
   printf("Function:%s!\n",__FUNCTION__);
	ITU656in_buffer_init(osd_w,osd_h );// malloc memory
	lcd_for_itu656(osd_layer, osd_w,osd_h);
   
	ITU656_reset_field();
   
	assert(display_itu656);
   
	while( 1 )
	{
		unsigned int a,b,c,d,e,f,val;
		unsigned int count ;
		
#if 0
		if( fieldcount >= ITU656_BUF_MAX )
		{
			irq_disable( ITU656_INT );
			while( 1 );
		}
#endif
	// ��ʾ��ǰ�� FIFO�л��ж������ַ���� 

		count = (rITU656IN_MODULE_STATUS>>29);
		if( count <= 3 )
		{
			ITU656_fill_DataAddress( count );
		}
		OS_Delay(1);
	}

}

void dump_itu656_regs (void)
{
	printf ("ITU656_BASE + 0x000 = 0x%08x\n", (*(volatile unsigned int *)(ITU656_BASE + 0x000)));
	printf ("ITU656_BASE + 0x124 = 0x%08x\n", (*(volatile unsigned int *)(ITU656_BASE + 0x124)));
	printf ("ITU656_BASE + 0x128 = 0x%08x\n", (*(volatile unsigned int *)(ITU656_BASE + 0x128)));
	printf ("ITU656_BASE + 0x12c = 0x%08x\n", (*(volatile unsigned int *)(ITU656_BASE + 0x12c)));
	printf ("ITU656_BASE + 0x8f4 = 0x%08x\n", (*(volatile unsigned int *)(ITU656_BASE + 0x8f4)));
	printf ("ITU656_BASE + 0x8f8 = 0x%08x\n", (*(volatile unsigned int *)(ITU656_BASE + 0x8f8)));
	printf ("ITU656_BASE + 0x8fc = 0x%08x\n", (*(volatile unsigned int *)(ITU656_BASE + 0x8fc)));
	printf ("ITU656_BASE + 0x900 = 0x%08x\n", (*(volatile unsigned int *)(ITU656_BASE + 0x900)));
	printf ("ITU656_BASE + 0x904 = 0x%08x\n", (*(volatile unsigned int *)(ITU656_BASE + 0x904)));
	printf ("ITU656_BASE + 0x930 = 0x%08x\n", (*(volatile unsigned int *)(ITU656_BASE + 0x930)));
	printf ("ITU656_BASE + 0x934 = 0x%08x\n", (*(volatile unsigned int *)(ITU656_BASE + 0x934)));
	printf ("ITU656_BASE + 0x938 = 0x%08x\n", (*(volatile unsigned int *)(ITU656_BASE + 0x938)));
	printf ("ITU656_BASE + 0x94c = 0x%08x\n", (*(volatile unsigned int *)(ITU656_BASE + 0x94c)));
	printf ("ITU656_BASE + 0x950 = 0x%08x\n", (*(volatile unsigned int *)(ITU656_BASE + 0x950)));
	printf ("ITU656_BASE + 0x954 = 0x%08x\n", (*(volatile unsigned int *)(ITU656_BASE + 0x954)));
	printf ("ITU656_BASE + 0x958 = 0x%08x\n", (*(volatile unsigned int *)(ITU656_BASE + 0x958)));
	printf ("ITU656_BASE + 0x95c = 0x%08x\n", (*(volatile unsigned int *)(ITU656_BASE + 0x95c)));
	printf ("ITU656_BASE + 0x960 = 0x%08x\n", (*(volatile unsigned int *)(ITU656_BASE + 0x960)));
	printf ("ITU656_BASE + 0x964 = 0x%08x\n", (*(volatile unsigned int *)(ITU656_BASE + 0x964)));
}

void ITU656in_Config_frame( itu656_window_s *window )
{ 
	int rdata;
	int lastno;
	int osd_layer = 2 ;
	unsigned char *addr ;
	unsigned int time = 0;
	unsigned int is_open = 1;
   unsigned int osd_w,osd_h;
	itudata_s *data;
   
   //ָ�� OSD �Ŀ�� �߶� 
   osd_w = window->real_w;//720;
   osd_h = window->real_h;//576;
   
	bufferno = 0 ;
	oddline = 0;
	evenline = 0;
	frameline = 0;
	fieldcount = 0;

	slicecount = 0;
	printf("Function:%s!\n",__FUNCTION__);
	ITU656in_buffer_init(osd_w,osd_h );// malloc memory
#ifdef ITU656_SUPPORT_LCD_DISPLAY
   lcd_for_itu656(osd_layer,osd_w,osd_h);
#endif 
	ITU656_reset_frame( window );
   
	
   assert(display_itu656);
	
	dump_itu656_regs();


	while( 1 )
	{
		unsigned int count ;
		// ��ʾ��ǰ�� FIFO�л��ж������ַ���� 

		count = (rITU656IN_MODULE_STATUS>>29);
		if( count <= 3  )
		{
			ITU656_fill_DataAddress( count );
		}
		OS_Delay(1);
	}

}


void ITU601in_int_fieldhandler()
{// ��ģʽ ����
	unsigned int rdata = rITU656IN_ISR;
	unsigned int val ;
	unsigned int itu656_int_mask ;
	
	//printf("int %x\n",rdata);
	rITU656IN_ICR  = rdata; 


	if( rdata & 1 )// odd field
	{
		//ITU656_fill_DataAddress( 3 );//����һ����ַ
		unsigned int count = (rITU656IN_MODULE_STATUS>>29);
	  //printf("fifo=%d\n",count);
	 //	output_register_YCbcr_addr();
	//	printf(" \n");
		
		oddline += ( rITU656IN_LINE_NUM_PER_FIELD&0x7ff  );
		fieldcount ++;

	}

	if (rdata & (1<<3))// even field : use for frame
	{
		//ITU656_fill_DataAddress( 3 );//����һ����ַ
		unsigned int count = (rITU656IN_MODULE_STATUS>>29);
	//	printf("***\n");
	//	printf("fifo=%d\n",count);
	//	output_register_YCbcr_addr();

		evenline += (  rITU656IN_LINE_NUM_PER_FIELD&0x7ff  );
		fieldcount ++;
	}
#ifdef ITU656_SUPPORT_POP_ERROR
	if (rdata & (1<<10))//bit.10=pop error 
	{
		unsigned int count = (rITU656IN_MODULE_STATUS>>28);
		printf("pop error\n");
	}
	if (rdata & (1<<2))//bit.10=pop error 
	{
  //    push_error++;
		;printf("push error\n");
	}
	if( rdata & (1<<8) )// field start 
	{
	}
#endif
#ifdef SLICE_MODE
	if( rdata & (1<<9) )
	{
		slicecount ++;
	}
#endif

	
}

void ITU601_reset_field_bypass( itu656_window_s *window )
{
	unsigned int osd_layer = 2;
   
   //������֮�󡡵ġ�ʵ�ʴ�С��
   unsigned int real_w    = window->real_w;
   unsigned int real_h    = window->real_h;
   
	itudata_s *data;
	// clear pad
	ITU601in_UnSelPad();
   
   ITU656_reset_soft();
   
	irq_disable( LCD_INT );
   //  ����Ŀռ� Ҳʹ�� 
	ITU656in_buffer_init( real_w ,  real_h ) ;// malloc memory
   
	rSYS_DEVICE_CLK_CFG0 |= (0x1<<9);
	
	unsigned int Itu_clk_b_dly = 0;
	unsigned int reg = rSYS_DEVICE_CLK_CFG3;
	reg &= ~(0x7F << 23);
	reg |= (Itu_clk_b_dly << 23);
	rSYS_DEVICE_CLK_CFG3 = reg;
	
	rITU656IN_ModuleEn = 0<<5| 0x1<<1 |0x1 | (1 << 4);


	printf("Function:%s \nField MODE:\n", __FUNCTION__ );
	//irq_enable( LCD_INT );

	// ��һ����ʾ���õ�һ����ַ��ʼ
   lcd_for_itu656( osd_layer, real_w , real_h  );
	LCD_Layer_fill_address(osd_layer ,itu656data[0].Ydata, itu656data[0].Pbprdata, 0);


	LCD_cpu_osd_coef_syn_reg = 7 ;
	
	rITU656IN_INPUT_SEL = 0x0 ; //select -> itu 601 in , bypass 
	rITU656IN_IMR = (1<<0)|(1<<3)  ; // bit.0 odd int    bit.3  even int 
	rITU656IN_ICR = 0x3ff ;
	rITU656IN_SLICE_PIXEL_NUM = 0 ;
   
   // open window setting
   rITU656IN_H_CUT = ((window->Left_cut&0x7ff)<<16)|(window->right_cut&0x7ff );
	rITU656IN_V_CUT = ((window->up_cut&0x7ff)<<16)|(window->down_cut&0x7ff );
   
	// field                        //bit.5: 0.field mode or 1.frame mode
	rITU656IN_ENABLE_REG = (0<<13)|(1<<12)|  (1<<11) |(0<<5) |(0<<4) | (0<<3) | (1<<1) | 0;
	
	//                          ֡���в���| �м����ز���
	rITU656IN_DELTA_NUM = 5<<16 | (10<<8) |     20;
	
	// ���������ַ  Y��UV��ַ��ÿ������
	ITU656_DataAddress_Reg_Init(  );
	
	request_irq(ITU656_INT, PRIORITY_FIFTEEN, ITU601in_int_fieldhandler );
	
	//����
   rITU656IN_SIZE = (window->width)<<16; //wide
   rITU656IN_DATA_OUT_LINE_NUM_PER_FIELD = window->height;
   rITU656IN_TOTAL_PIX = (window->width-((rITU656IN_H_CUT>>16)&0x7ff)- ((rITU656IN_H_CUT)&0x7ff) )*
      (window->height-((rITU656IN_V_CUT>>16)&0x7ff)- ((rITU656IN_V_CUT)&0x7ff) ) ;//720*576; ����Ч���� 
   printf("%d\n",  (window->width-((rITU656IN_H_CUT>>16)&0x7ff)- ((rITU656IN_H_CUT)&0x7ff) ));
   printf("%d\n",  (window->height-((rITU656IN_V_CUT>>16)&0x7ff)- ((rITU656IN_V_CUT)&0x7ff) ));
   printf("width:%d\n",  rITU656IN_SIZE>>16);
   printf("height:%d\n", rITU656IN_DATA_OUT_LINE_NUM_PER_FIELD);
   printf("TOTAL_PIX:%d\n",  rITU656IN_TOTAL_PIX );
   

   
   rITU656IN_INPUT_CTL |= (1<<15);
	
	rITU656IN_ENABLE_REG |=  (3<<1) ;
   rITU656IN_ENABLE_REG |= ( 1<<0 );
	
#ifndef ITU656_SWITCH_BUFFER	//�꿪�����壺���û�ж����л�buffer
	rITU656IN_DRAM_Y_ADDR = itu656data[0].Ydata;
	rITU656IN_DRAM_Y_ADDR =  itu656data[0].Pbprdata;
#endif
   ITU601in_SelPad();
}

void ITU601in_Config_bypass( itu656_window_s *window )
{
   // 5. ѡ�� 656 ����601 ֱͨ����   �����ҿ��� 
   // rITU656IN_INPUT_SEL = 0;
   // rITU656IN_H_CUT = 
   // rITU656IN_V_CUT = 
	int rdata;
	int lastno;
	int osd_layer = 2 ;
	unsigned char *addr ;
	unsigned int time = 0;
	itudata_s *data;
	bufferno = 0 ;
	oddline = 0;
	evenline = 0;
	frameline = 0;
	fieldcount = 0;

	slicecount = 0;
	printf("FUNCTION:%s() in test!\n",__FUNCTION__ );
	
	
   
	ITU601_reset_field_bypass( window );
	
	dump_itu656_regs();
	
	while( 1 )
	{
		unsigned int a,b,c,d,e,f,val;
		unsigned int count ;
		
	// ��鵱ǰ�� FIFO�л��ж������ַ���� 
		count = (rITU656IN_MODULE_STATUS>>29);
		if( count <= 3 )
			ITU656_fill_DataAddress( count );
		
	}
}