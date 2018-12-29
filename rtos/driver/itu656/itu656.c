#include "RTOS.H"
#include "ark1960.h"
#include "irqs.h"
#include <xm_queue.h>
#include "lcd.h"
#include "itu656.h"
#include <assert.h>

/*
问题点集中：
o	1.   以PAL制 场开始 ，只有偶 场中断出现 ，奇场没有中断
o	2.		检查是否会出现写地址出现死锁,地址必须对齐
o  3.   定时器，过慢，统计数据有错误
   4.   切换buffer是否出错
*/


//#define  SLICE_MODE
#define  ITU656_SUPPORT_POP_ERROR
#define  ITU656_SWITCH_BUFFER   // 切换buffer
#define  ITU656_SUPPORT_LCD_DISPLAY //itu656 是否支持 LCD


//#define  limit_num_field   // 只运行到 指定数量的缓存


// 定义了多少个Buffer
#define  ITU656_BUF_MAX    5
#define  ITU656_BUF_BAK    1024

// 以下是一些测试用变量
static unsigned int oddline = 0;
static unsigned int evenline = 0;
static unsigned int fieldcount = 0;
static unsigned int slicecount = 0;
static unsigned int sliceline = 600;

static unsigned int frameline = 0;



static unsigned char *ITU656buffer;



static itudata_s  itu656data[ITU656_BUF_MAX]={0};
/*display_itu656 :当前已经完成的帧数据 需要应用到显示 */
static itudata_s  *display_itu656 =  NULL ;//display_itu656

static itu656_window_s itu656_window={0};

void ITU656in_SelPad( void )
{
	//使用 pad:itu_b 
	/* ITU656 padlist : 使用9根线: itu_b_clk + 数据线ITU_B:0-7  */
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
	//使用 pad:itu_b 
	/* ITU656 padlist : 使用9根线: itu_b_clk + 数据线ITU_B:0-7  */
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
*    参数为从 rITU656IN_MODULE_STATUS 寄存器中读出的FIFO中剩余的数据个数
*    一般取值范围在0 1 2 3 4 这5个数值 ，表示地址寄存器中地址还剩下多少组
*    参数为 0 时候，该函数可以填入四组Y UV地址，每组两个数值，即Y地址，UV地址
*    参数为 4 时候，该函数不能填写任何地址 
********************************************************************************/
void ITU656_fill_DataAddress(int count )
{
	unsigned int addr = rITU656IN_DRAM_Y_ADDR;
	// 表示当前的 FIFO中还有多少组地址可用 
	//unsigned int count = (rITU656IN_MODULE_STATUS>>29);
	unsigned int val;
	int MAXfill =0;
	itudata_s *data = display_itu656;
//	printf("addr=0x%08x\n",	addr);
	
	//最多可填写四个数据
	MAXfill = 4-count ;
	if( MAXfill == 0 )
			return;	
	if( MAXfill > ITU656_BUF_MAX )
		MAXfill = ITU656_BUF_MAX-1 ;

	if( (count +1) < ITU656_BUF_MAX )
	{
#ifdef ITU656_SWITCH_BUFFER
		int no = 0;
		// 每次填入的链元素是 next,而遍历到第一个空闲链元素是prev方向，两个方向不一样
		// 定位到当前填进FIFO的数据链表尾部位置
		// 遍历到需要填充进FIFO的第一个链元素

		//1. 定位到当前FIFO位置
		for( int no = 0; no < ITU656_BUF_MAX; no ++  )
		{
			if( data->Ydata == addr )
			{
            // 当前已经完成的帧数据，可以用来显示  
       //     display_itu656 = data->prev;
            break;
         }
			data = data->next;//下一个填入FIFO的链元素
		}
		// no =0;
		//2. 利用count参数 定位到 要 最先填入数据的位置 
		// ITU656_BUF_MAX-(count+1) 表示当前空闲的链元素 
		for( int no = 0 ; no < ITU656_BUF_MAX-(count+1) ; no ++  )
			data = data->prev;//下一个填入FIFO的链元素
#endif
		
		//3. 填入目前还剩余的空闲地址
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
*  用于复位之后的第一次 给地址FIFO填充地址数据
********************************************************************************/
static void ITU656_DataAddress_Reg_Init(  )
{
	//复位后， 第一次不管任意条件，首先填入四组Buffer的地址 
	itudata_s *data = itu656data;
	//填入目前还剩余的空闲地址
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
*  关闭 itu656 输入信号
*  返回 0： 当前不能关闭
*  返回 1:  当前可以关闭
* 与lcd控制器配合使用
********************************************************************************/
unsigned int ITU656in_disable( )
{
#define  status_val    ((0xf<<24)|(0x7<<20)|(0x3<<16))
	/* 三个状态不为零的时候，不许进行复位和其他任何操作 */
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
   // 一般只需要执行一次
   for(no=0 ;no < ITU656_BUF_MAX ; no++ )
   {
      if(display_itu656->next->Ydata == addr)
         break;

      display_itu656 = display_itu656->next ; 
   }
   
    
}


void ITU656in_int_fieldhandler()
{// 场模式 处理
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
	//  第一次或者制式切换的时候，均能进入 1<<7 中断 （debounce）
	if( rdata &(1<<7))
	{
		//printf("int %x\n",rdata);
		// field       //bit.7:line num changed //bit.9:slice 
#ifndef SLICE_MODE
		itu656_int_mask =  (1<<0) | (1<<3) | (1<<7)    ;
#else
		itu656_int_mask =  (1<<0) | (1<<3) | (1<<7) |(1<<9)   ;
		rITU656IN_SLICE_PIXEL_NUM = 720*sliceline;//中断不开 ，写不写数据一样效果
#endif
#ifdef ITU656_SUPPORT_POP_ERROR
		itu656_int_mask |=  (1<<10)|(1<<2)|(1<<8) ;//bit.8 field start //bit.10=pop error //bit.2: push error 
		rITU656IN_IMR = itu656_int_mask;;
#else
		rITU656IN_IMR = itu656_int_mask;
#endif
	//	rITU656IN_IMR = 0x3ff;
    	rITU656IN_TOTAL_PIX = ((rITU656IN_LINE_NUM_PER_FIELD-10)&0x7fe)*(rITU656IN_PIX_NUM_PER_LINE&0xfff)/2;

      // 20150515 新版本配置
      rITU656IN_DATA_OUT_LINE_NUM_PER_FIELD = (rITU656IN_LINE_NUM_PER_FIELD-10)&0x7ff;

		rITU656IN_ENABLE_REG |= ( 1<<0 );
	}
	

	if( rdata & 1 )// odd field
	{
		//ITU656_fill_DataAddress( 3 );//填入一个地址
		unsigned int count = (rITU656IN_MODULE_STATUS>>29);
//		printf("fifo=%d\n",count);

		set_display_itu656_addr();
      
		oddline += ( rITU656IN_LINE_NUM_PER_FIELD&0x7ff  );
		fieldcount ++;
      itu656_frame ++;
	}

	if (rdata & (1<<3))// even field : use for frame
	{
		//ITU656_fill_DataAddress( 3 );//填入一个地址
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
{// 帧模式 处理
	
	// 此时，偶场与奇场状态都会出
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
	
	//  第一次或者制式切换的时候，均能进入 1<<7 中断 （debounce）
	if( rdata &(1<<7) )
	{
		// frame  
      rITU656IN_IMR = (1<<8);
    	rITU656IN_TOTAL_PIX = ((rITU656IN_LINE_NUM_PER_FIELD-10)&0x7fe)
                              *(rITU656IN_PIX_NUM_PER_LINE&0xfff)/2;
      // 20150515 新版本配置
      rITU656IN_DATA_OUT_LINE_NUM_PER_FIELD = (rITU656IN_LINE_NUM_PER_FIELD-10)&0x7ff;
	}
	
	// 避免帧模式下 为了只进入even中断，限制进入odd中断处理
	if (rdata & (1<<3))// even field : use for frame
	{
		unsigned int count = (rITU656IN_MODULE_STATUS>>29);
      
      // 求得当前完成的 帧 
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
		   rITU656IN_SLICE_PIXEL_NUM = 720*sliceline;//中断不开 ，写不写数据一样效果
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
   /*sysbase+0x74  bit.19,默认是1 ，reset:先配0，再配1 */
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
   ITU601in_SelPad_clk();// 只选中 clk
   // soft reset 
	ITU656_reset_soft();
//   printf("push fifo=%d Mod-status:0x%08x ISR:0x%08x\n",(rITU656IN_MODULE_STATUS>>29), rITU656IN_MODULE_STATUS, rITU656IN_ISR );


	/*Pad 脚选择放在最后 避免垃圾信号干扰 ，影响处理 */
	
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

	//                 消颤帧数 |帧间行差异| 行间像素差异
	rITU656IN_DELTA_NUM = 5<<16 | (10<<8)  | 20;
	rITU656IN_TOTAL_PIX = 0;//720*576;
	// 首次复位后 要填入四组数据
	ITU656_DataAddress_Reg_Init();


	request_irq(ITU656_INT, PRIORITY_FIFTEEN, ITU656in_int_framehandler);
	
	rITU656IN_IMR =  (1<<7);// 行数变化中断
	//rITU656IN_IMR = 0x3ff;
	
	rITU656IN_SIZE = 720<<16;  //配置行数 0x2d0 = 720
	
	rITU656IN_ENABLE_REG |= (3<<1);// | (1<<0) ;//bit.3:Axi_cmd_max
	
#ifndef ITU656_SWITCH_BUFFER	//宏开关意义：如果没有定义切换buffer
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

   ITU601in_SelPad_clk();// 只选中 clk
	ITU656_reset_soft();
	

	rSYS_DEVICE_CLK_CFG0 |= (0x1<<9);
   //rSYS_DEVICE_CLK_CFG0 &= ~(0x1<<9);
	
	rITU656IN_ModuleEn = 1<<5| 0x1<<1 |0x1;


	printf("Field MODE\n");
   printf("Function:%s!\n",__FUNCTION__);
	irq_enable( LCD_INT );
	LCD_Layer_Size(osd_layer,720,288);

	// 第一次显示都用第一个地址开始
	LCD_Layer_fill_address(osd_layer ,itu656data[0].Ydata, itu656data[0].Pbprdata, 0);

	LCD_cpu_osd_coef_syn_reg = 7 ;
	
	rITU656IN_INPUT_SEL = 0x01 ;// //0 : sel itu601 input 1 : select itu656 input
	rITU656IN_IMR = 0x0 ; 
	rITU656IN_ICR = 0x3ff ;
	rITU656IN_SLICE_PIXEL_NUM = 0 ;
	
	// field                        //bit.5: 0.field mode or 1.frame mode
	//rITU656IN_ENABLE_REG = (0<<13) | (1<<11) |(0<<5) | (1<<3) | (1<<1) | 1;
	rITU656IN_ENABLE_REG = (0<<13)|(1<<12)|  (1<<11) |(0<<5) | (0<<3) | (1<<1) | 0;
	
	//                          帧间行差异| 行间像素差异
	rITU656IN_DELTA_NUM = 5<<16 | (10<<8) | 20;
	rITU656IN_TOTAL_PIX = 0;//720*576;
	// 填入四组地址  Y、UV地址，每组两个
	ITU656_DataAddress_Reg_Init(  );
	
	request_irq(ITU656_INT, PRIORITY_FIFTEEN, ITU656in_int_fieldhandler);

	rITU656IN_IMR =  (1<<7);// 行数变化中断（每帧有效行变化）
	//rITU656IN_IMR = 0x3ff;
	
	rITU656IN_SIZE = 720<<16;  //配置行宽 0x2d0 = 720
	
	rITU656IN_ENABLE_REG |=  (3<<1) ;// 每一次写入命令的 总数
	
#ifndef ITU656_SWITCH_BUFFER	//宏开关意义：如果没有定义切换buffer
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
   
   //指定 OSD 的宽度 高度 
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
	// 表示当前的 FIFO中还有多少组地址可用 

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
   
   //指定 OSD 的宽度 高度 
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
		// 表示当前的 FIFO中还有多少组地址可用 

		count = (rITU656IN_MODULE_STATUS>>29);
		if( count <= 3  )
		{
			ITU656_fill_DataAddress( count );
		}
		OS_Delay(1);
	}

}


void ITU601in_int_fieldhandler()
{// 场模式 处理
	unsigned int rdata = rITU656IN_ISR;
	unsigned int val ;
	unsigned int itu656_int_mask ;
	
	//printf("int %x\n",rdata);
	rITU656IN_ICR  = rdata; 


	if( rdata & 1 )// odd field
	{
		//ITU656_fill_DataAddress( 3 );//填入一个地址
		unsigned int count = (rITU656IN_MODULE_STATUS>>29);
	  //printf("fifo=%d\n",count);
	 //	output_register_YCbcr_addr();
	//	printf(" \n");
		
		oddline += ( rITU656IN_LINE_NUM_PER_FIELD&0x7ff  );
		fieldcount ++;

	}

	if (rdata & (1<<3))// even field : use for frame
	{
		//ITU656_fill_DataAddress( 3 );//填入一个地址
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
   
   //　开窗之后　的　实际大小　
   unsigned int real_w    = window->real_w;
   unsigned int real_h    = window->real_h;
   
	itudata_s *data;
	// clear pad
	ITU601in_UnSelPad();
   
   ITU656_reset_soft();
   
	irq_disable( LCD_INT );
   //  分配的空间 也使用 
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

	// 第一次显示都用第一个地址开始
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
	
	//                          帧间行差异| 行间像素差异
	rITU656IN_DELTA_NUM = 5<<16 | (10<<8) |     20;
	
	// 填入四组地址  Y、UV地址，每组两个
	ITU656_DataAddress_Reg_Init(  );
	
	request_irq(ITU656_INT, PRIORITY_FIFTEEN, ITU601in_int_fieldhandler );
	
	//配置
   rITU656IN_SIZE = (window->width)<<16; //wide
   rITU656IN_DATA_OUT_LINE_NUM_PER_FIELD = window->height;
   rITU656IN_TOTAL_PIX = (window->width-((rITU656IN_H_CUT>>16)&0x7ff)- ((rITU656IN_H_CUT)&0x7ff) )*
      (window->height-((rITU656IN_V_CUT>>16)&0x7ff)- ((rITU656IN_V_CUT)&0x7ff) ) ;//720*576; 总有效点数 
   printf("%d\n",  (window->width-((rITU656IN_H_CUT>>16)&0x7ff)- ((rITU656IN_H_CUT)&0x7ff) ));
   printf("%d\n",  (window->height-((rITU656IN_V_CUT>>16)&0x7ff)- ((rITU656IN_V_CUT)&0x7ff) ));
   printf("width:%d\n",  rITU656IN_SIZE>>16);
   printf("height:%d\n", rITU656IN_DATA_OUT_LINE_NUM_PER_FIELD);
   printf("TOTAL_PIX:%d\n",  rITU656IN_TOTAL_PIX );
   

   
   rITU656IN_INPUT_CTL |= (1<<15);
	
	rITU656IN_ENABLE_REG |=  (3<<1) ;
   rITU656IN_ENABLE_REG |= ( 1<<0 );
	
#ifndef ITU656_SWITCH_BUFFER	//宏开关意义：如果没有定义切换buffer
	rITU656IN_DRAM_Y_ADDR = itu656data[0].Ydata;
	rITU656IN_DRAM_Y_ADDR =  itu656data[0].Pbprdata;
#endif
   ITU601in_SelPad();
}

void ITU601in_Config_bypass( itu656_window_s *window )
{
   // 5. 选择 656 还是601 直通输入   ，左右开窗 
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
		
	// 检查当前的 FIFO中还有多少组地址可用 
		count = (rITU656IN_MODULE_STATUS>>29);
		if( count <= 3 )
			ITU656_fill_DataAddress( count );
		
	}
}