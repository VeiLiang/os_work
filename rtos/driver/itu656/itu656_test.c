#include "lcd.h"
#include "irqs.h"
#include "ark1960.h"
#include "itu656.h"
#include  "rtos.h"

void test_ITU656in_frame_on_and_off( itu656_window_s *window )
{ 
	int rdata;
	int lastno;
	int osd_layer = 2 ;
	unsigned char *addr ;
	unsigned int time = 0;
	unsigned int is_open = 1;
	itudata_s *data;
   unsigned int osd_w,osd_h;
   unsigned int limittime = 2001;
   
   osd_w = window->real_w;
   osd_h = window->real_h  ;

	printf("ITU656 in test_ITU656in_frame_on_and_off !\n");
	ITU656in_buffer_init(osd_w,osd_h );// malloc memory
	
	lcd_for_itu656( osd_layer , osd_w,osd_h );
	ITU656_reset_frame( window );
	
	while( 1 )
	{
		unsigned int a,b,c,d,e,f,val;
		unsigned int count ;
		// ��ʾ��ǰ�� FIFO�л��ж������ַ���� 

		count = (rITU656IN_MODULE_STATUS>>29);
		if( count <= 3  )
		{
			ITU656_fill_DataAddress( count );
		}
		time ++; 
		if( time >= limittime ) 
		{
         printf("%d\n",OS_GetTime32() );
			irq_disable( ITU656_INT );//
         printf("close itu656 \n");
			while(1)
         {
            if( ITU656in_disable()  )
            {
               ITU656in_UnSelPad();
               rITU656IN_ICR  = 0x3ff;
               
   
               OS_Delay(1000);
               printf("open itu656 \n");
               limittime = rand()&0x1fff;
               if(limittime<400)
                  limittime = 1000;
               printf("time set:%d\n", limittime);
               ITU656_reset_frame();
               time = 0;
               break;
            }
			}
		}
		OS_Delay(1);
	}
}



void test_ITU656in_field_on_and_off( itu656_window_s *window )
{ 
	int rdata;
	int lastno;
	int osd_layer = 2 ;
	unsigned char *addr ;
	unsigned int time = 0 ;
	itudata_s *data ;
   unsigned int osd_w , osd_h ;
   
   unsigned int limittime=2000;
   
   osd_w = window->real_w;//720;
   osd_h = window->real_h;//576;

	printf("ITU656 in test_ITU656in_field_on_and_off!\n");
	ITU656in_buffer_init( osd_w , osd_h ) ;// malloc memory
	lcd_for_itu656( osd_layer , osd_w , osd_h );
	ITU656_reset_field();

	while( 1 )
	{
		unsigned int a,b,c,d,e,f,val;
		unsigned int count ;
		// ��ʾ��ǰ�� FIFO�л��ж������ַ���� 

		count = (rITU656IN_MODULE_STATUS>>29);
		if( count <= 3 )
		{
			ITU656_fill_DataAddress( count );
		}

		time ++; 

		if( time >= limittime ) 
		{
         printf("%d\n",OS_GetTime32());
         limittime = rand()&0x1fff;
         if(limittime<400)
            limittime = 1000;
         
			irq_disable( ITU656_INT );//
		//	rITU656IN_ENABLE_REG &=  0xfffffffe ;
			while(1)
			{
				if( ITU656in_disable()  )
				{
					ITU656in_UnSelPad();
					rITU656IN_ICR  = 0x3ff;
					printf("close itu656 \n");
	
					OS_Delay(2000);
               printf("time set:%d\n", limittime);
					printf("open itu656 \n");
					ITU656_reset_field();
					time = 0;
					break;
				}
			}
		}
		OS_Delay(1);
	}
}

void test_ITU656in_Note()
{
   printf("ITU656in ��Ҫʹ�ÿ�����������ʾ��λ\n");
   printf("�����ܷ� ������display_itu656 �Ƿ�Ϊ��\n");
}



/*******************************************************************************
static void ITU656IspCalculateTask()
// ���� lcd ֡��

// itu656 ֡��

// ÿ�� 4s ִ��һ��
*******************************************************************************/
#if 0
static OS_TASK TCB_ITU656CalculateTask;
OS_STACKPTR int StackITU656Calculate[0x200/4];   
static void ITU656IspCalculateTask()
{
   unsigned int lcd_frame  ;
   unsigned int itu656_frame ;
   unsigned int start_time,end_time=0;
   unsigned int time;
   float seconds;
   float lcd_rate,itu656_rate;
   Reset_lcd_frame();
   Reset_itu656_frame();
   while( 1 )
   {
      end_time = OS_GetTime32();
      //��ȡlcd rate 
      lcd_frame = return_lcd_frame();
      Reset_lcd_frame();
      
      // ��ȡitu656 �ٶ�
      itu656_frame = return_itu656_frame();
      Reset_itu656_frame();
      
      time =  end_time-start_time;
      
      start_time = OS_GetTime32();
      
      if(time)
      {
         seconds = (float)(time/1000);
         lcd_rate = (float)(lcd_frame/seconds);
         itu656_rate = (float)(itu656_frame/seconds);
      }
      else
      {
         lcd_rate = 0;
         itu656_rate =0;
      }
      
      printf("itu656 time:%f \n",seconds );
      printf("itu656 lcd_rate:%f/s lcd_frame:%d\n", lcd_rate,lcd_frame);
      printf("itu656 itu656_rate:%f/s itu656_frame:%d\n", itu656_rate,itu656_frame);
      OS_Delay(4000);
   }
}
#endif

void  test_ITU656in()
{
	UART1_RX_init();
	
   itu656_window_s *window = ITU656_Get_Window();
#if 1 // ������
   window->Left_cut  = 0;
   window->right_cut = 0;
   window->up_cut    = 0;
   window->down_cut  = 0;
	window->width     = 320;
	window->height    = 240 ;
 //  window->width     = 720;// ������
  // window->height    = 576 ;// ������
	// window->height    = 480 ;// ������
   window->real_w    = window->width  - window->Left_cut - window->right_cut;
   window->real_h    = window->height - window->up_cut   - window->down_cut;
#else // ���� 
   // ʵ�ʲ����У������±ߣ�û����Ч��
   window->Left_cut  = 0;
   window->right_cut = 0;
   window->up_cut    = 32;
   window->down_cut  = 64;
   window->width     = 720;// ������
   window->height    = 576 ;// ������
   window->real_w    = window->width  - window->Left_cut - window->right_cut;
   window->real_h    = window->height - window->up_cut   - window->down_cut;
#endif
   rITU656IN_INPUT_SEL = 0; //0 : sel itu601 input 1 : select itu656 input
   add_ispCalculateTask();
   
   test_ITU656in_Note();
   
   ITU656_Set_Window( window );
   //rITU656IN_H_CUT  = 0;//((window->Left_cut&0x7ff)<<16) | (window->right_cut&0x7ff) ;
	//rITU656IN_V_CUT  =0;
   
   //���� ͳ�� ���߲��� �������ܵ�
   // ����lcd֡�ٶȡ�itu656֡�� 
//   OS_CREATETASK(&TCB_ITU656CalculateTask, "TCB_ITU656CalculateTask", ITU656IspCalculateTask, 255, StackITU656Calculate );	
   
	//1. ѡ�� ֡ģʽ �л������
//	test_ITU656in_frame_on_and_off( window );return;
	
	//2. ѡ�� ��ģʽ �л������
	// 
//	test_ITU656in_field_on_and_off(window);return;
	
	//3. ����Ϊ�� ģʽ ������ʾ 
	// �жϺ���������дdizhi 
//	ITU656in_Config_field( window );return;
	
	//4. ����Ϊ֡ ģʽ ������ʾ 
	//ITU656in_Config_frame( window );return;
   
   // 5. ѡ�� 656 ����601 ֱͨ����   �����ҿ��� 
   // rITU656IN_INPUT_SEL = 0; //0 : sel itu601 input 1 : select itu656 input
   // rITU656IN_H_CUT = 
   // rITU656IN_V_CUT = 
   window->Left_cut  = 0;//8*4;
   window->right_cut = 0;	//8*4;
   window->up_cut    = 0;	//8;
   window->down_cut  = 0;	//8;
   window->width     = 320;
   window->height    = 240 ;
   window->real_w    = window->width  - window->Left_cut - window->right_cut;
   window->real_h    = window->height - window->up_cut   - window->down_cut;
   
   ITU601in_Config_bypass( window );
   return;
}
