#include "hardware.h"
#include "xm_dev.h"
#include  "lcd.h"
#include  <stdio.h>
#include <math.h>
#include "hw_osd_layer.h"
#include "rtos.h"
#include "xm_base.h"


static const unsigned char osd_minimum_moving_h_pixels_clr[XM_OSD_LAYER_FORMAT_COUNT] = 
{
	0xf,		// YUV420
	0xf,		// ARGB888
	0xf,		// RGB565
	0xf,		// ARGB454
	0xf		//
};

// ��ֱ������С�ƶ����ص�λ
static const unsigned char osd_minimum_moving_v_pixels[XM_OSD_LAYER_FORMAT_COUNT] = 
{
	2,		// YUV420
	2,		// ARGB888
	2,		// RGB565
	2,		// ARGB454
	2
};
static const unsigned char osd_minimum_moving_v_pixels_clr[XM_OSD_LAYER_FORMAT_COUNT] = 
{
	0x1,	// YUV420
	0x1,		// ARGB888
	0x1,		// RGB565
	0x1,		// ARGB454
	0x1
};



//#pragma inline
void LCD_Layer_Set(int Layer) 
{
   switch(Layer)
   {
   case 0:
      LCD_OSD_0_PARAM0_reg |= (1<<1);
      break;
   case 1:
      LCD_OSD_1_PARAM0_reg |= (1<<1);
      break;
   case 2:
      LCD_OSD_2_PARAM0_reg |= (1<<1);
      break;
   default:
		break;
   }
	/*int val;
	val = *(volatile unsigned *)(LCD_BASE + 23 * 4+Layer*24);
	val |=(1<<1);
	*(volatile unsigned *)(LCD_BASE + 23 * 4+Layer*24)=val;*/
}

//#pragma inline
void LCD_Layer_Clr(int Layer) 
{
   switch ( Layer )
   {
   case 0:
      LCD_OSD_0_PARAM0_reg &= ~(1<<1);
      break;
   case 1:
      LCD_OSD_1_PARAM0_reg &= ~(1<<1);
      break;
   case 2:
      LCD_OSD_2_PARAM0_reg &= ~(1<<1);
      break;
   default:
		break;
   }
	/*val = *(volatile unsigned *)(LCD_BASE + 23 * 4+Layer*24);
	val &= ~(1<<1);
	*(volatile unsigned *)(LCD_BASE + 23 * 4+Layer*24)=val;*/
}

// IAR ��������
#pragma inline
void LCD_OSD_Update(int layer )
{
	LCD_cpu_osd_coef_syn_reg = (1<<layer) ;
//	LCD_INTR_CLR_reg = 1;
}


void LCD_Layer_Close(int layer )
{
	// �ȴ�֡ˢ�½���
	LCD_INTR_CLR_reg = 0xFFFFFFFF;
	while(!(LCD_STATUS_reg & 0x01));		// bit1: lcd_done_intr(ÿ�������ж�) ,
	LCD_Layer_Clr( layer );
	LCD_cpu_osd_coef_syn_reg = (1 << layer);
	while(LCD_cpu_osd_coef_syn_reg & (1 << layer));
	
	/*
	do{
		if(LCD_STATUS_reg&0x1)
		{
			LCD_Layer_Clr( layer );
			LCD_OSD_Update( layer );
			break;
		}
	}while(1);
	*/
}

unsigned int LCD_layer_State (int layer)
{
	if(layer == 0)
		return (LCD_OSD_0_PARAM0_reg & (1 << 1)) ? 1 : 0;
	else if(layer == 1)
		return (LCD_OSD_1_PARAM0_reg & (1 << 1)) ? 1 : 0;
	else
		return (LCD_OSD_2_PARAM0_reg & (1 << 1)) ? 1 : 0;
}

/*
   LCD_Layer_Open  ����ʹ�����ж����� ����Ϊ��ʱ 
*/
void LCD_Layer_Open(int layer )
{
	unsigned int ticket_to_timeout = XM_GetTickCount() + 90;
	do
	{
		if(LCD_STATUS_reg&0x1)
		{
			break;
		}
		
		if(XM_GetTickCount() >= ticket_to_timeout)
			break;
	}while(1);
	LCD_Layer_Set( layer );
	LCD_OSD_Update( layer );
}

void LCD_Set_background_color(unsigned char r, unsigned char g, unsigned char b )
{
	int val = LCD_PARAM2_reg;
	val &= ~0xFFFFFF;
	val |=(r<<16)|(g<<8)|(b<<0);
	LCD_PARAM2_reg = val;
}

//void LCD_Layer_MoveLeft(int Layer,int Offset , int coeff )
void LCD_Layer_MoveLeft(int Layer,int Offset )
{
	int val= *(volatile unsigned *)(LCD_BASE + 28 * 4+Layer*24);
	val &= ~(0xFFF<<12);
	val |= (Offset<<12);
	*(volatile unsigned *)(LCD_BASE + 28 * 4+Layer*24) = val;
}

void LCD_Layer_set_stride(int Layer,int Stride)
{
   unsigned int val;
   unsigned int clr = 0xfffff000;
   unsigned int set = (Stride&0xFFF);
   switch( Layer )
   {
   case 0:
      val = LCD_OSD_0_PARAM5_reg;
      val &= clr;
      val |= set;
      LCD_OSD_0_PARAM5_reg = val;
      break;
   case 1:
      val = LCD_OSD_1_PARAM5_reg;
      val &= clr;
      val |= set;
      LCD_OSD_1_PARAM5_reg = val;
      break;
   case 2:
      val = LCD_OSD_2_PARAM5_reg;
      val &= clr;
      val |= set;
      LCD_OSD_2_PARAM5_reg = val;
      break;
   default:
		break;
   }
	/*int val= *(volatile unsigned *)(LCD_BASE + 28 * 4+Layer*24);
	val &= ~(0xFFF);
	val |= (Stride&0xFFF);
	*(volatile unsigned *)(LCD_BASE + 28 * 4+Layer*24) = val;*/
}

void LCD_Layer_set_left_position(int Layer,int Offset  )
{
   unsigned int val;
   unsigned int clr = 0xff000fff;
   unsigned int set = ((Offset&0xFFF)<<12);
   switch( Layer )
   {
   case 0:
      val = LCD_OSD_0_PARAM5_reg;
      val &= clr;
      val |= set;
      LCD_OSD_0_PARAM5_reg = val;
      break;
   case 1:
      val = LCD_OSD_1_PARAM5_reg;
      val &= clr;
      val |= set;
      LCD_OSD_1_PARAM5_reg = val;
      break;
   case 2:
      val = LCD_OSD_2_PARAM5_reg;
      val &= clr;
      val |= set;
      LCD_OSD_2_PARAM5_reg = val;
      break;
   default:
		break;
   }
	/*int val=*(volatile unsigned *)(LCD_BASE + 28 * 4+Layer*24);
	val &= ~(0xFFF<<12);
	val |= ((Offset&0xFFF)<<12);
	*(volatile unsigned *)(LCD_BASE + 28 * 4+Layer*24) = val;*/
}

//#pragma inline
void LCD_Layer_set_osd_param5(int Layer,int Offset  ,int Stride, unsigned char coeff )
{
   unsigned int val = ((Offset&0xFFF)<<12)|( Stride&0xFFF )|(coeff<<24);
   switch ( Layer )
   {
   case 0:
	   LCD_OSD_0_PARAM5_reg = val ;
      break;  
   case 1:
	   LCD_OSD_1_PARAM5_reg = val ;
      break;  
   case 2:
	   LCD_OSD_2_PARAM5_reg = val ;
      break;
   default:
		break;
   }
	//*(volatile unsigned *)(LCD_BASE + 28 * 4+Layer*24) = ((Offset&0xFFF)<<12)|( Stride&0xFFF )|(coeff<<24);
}

void LCD_Layer_set_brightness_coeff(int Layer,unsigned char coeff  )
{
   unsigned int val;
   switch (Layer)
   {
   case 0:
      val = LCD_OSD_0_PARAM5_reg ;
      val = (val<<8);
      val = ( (coeff)<<24 )|(val>>8);
      LCD_OSD_0_PARAM5_reg = val;
      break;
   case 1:
      val= LCD_OSD_1_PARAM5_reg;
      val = (val<<8);
      val = ( (coeff)<<24 )|(val>>8);
      LCD_OSD_1_PARAM5_reg = val;
      break;
   case 2:
      val= LCD_OSD_2_PARAM5_reg;
      val = (val<<8);
      val = ( (coeff)<<24 )|(val>>8);
      LCD_OSD_2_PARAM5_reg = val;
      break;
   default:
		break;
   }
   /*
	int val=*(volatile unsigned *)(LCD_BASE + 28 * 4+Layer*24);
	val &= ~( 0xFF<<24 );// 8 bit
	val |= ( (coeff)<<24 );
	*(volatile unsigned *)(LCD_BASE + 28 * 4+Layer*24) = val;*/
}

//ÿ��osd֧�����Դ�ֱ��ʣ�argbԴΪ1024��yuv420ԴΪ2048��rgb565ԴΪ2048��rgb454ԴΪ2048
#pragma inline
void LCD_Layer_Move(int Layer,int x , int y )
{// 
	int val= *(volatile unsigned *)(LCD_BASE + 24 * 4+24*Layer);
	val &= ~0xFFFFFF;
	val |= (((x&0xfff)<<0)|((y&0xfff)<<12));
	*(volatile unsigned *)(LCD_BASE + 24 * 4+24*Layer) =val;
}

void LCD_Layer_h_position(int Layer,int x )
{//��3������Ϊ ��  ͸����
	int val = *(volatile unsigned *)(LCD_BASE + 24 * 4+24*Layer);
	val &= ~0xFFF;
	val |= (x&0xFFF);
	*(volatile unsigned *)(LCD_BASE + 24 * 4+24*Layer) = val ;
}

void LCD_Layer_v_position(int Layer, int y )
{//��3������Ϊ ��  ͸����
	int val = *(volatile unsigned *)(LCD_BASE + 24 * 4+24*Layer);
	val &= ~(0xFFF<<12);
	val |= ((y&0xFFF)<<12);
	*(volatile unsigned *)(LCD_BASE + 24 * 4+24*Layer) = val;
}


void LCD_Layer_set_global_coeff(int Layer, unsigned char coeff )
{//��3������Ϊ ��  ͸����
   int val;
   switch (Layer)
   {
   case 0:
      val = LCD_OSD_0_PARAM1_reg ;
	   val &= ~(0xFF <<24);
	   val |= ((coeff) <<24);
	   LCD_OSD_0_PARAM1_reg = val;
      break;
   case 1:
      val = LCD_OSD_1_PARAM1_reg;
      val &= ~(0xFF <<24);
      val |= ((coeff) <<24);
      LCD_OSD_1_PARAM1_reg = val;
      break;
   case 2:
      val = LCD_OSD_2_PARAM1_reg ;
      val &= ~(0xFF <<24);
      val |= ((coeff) <<24);
      LCD_OSD_2_PARAM1_reg = val;
      break;
   default:
		break;
   }
	/*int val = *(volatile unsigned *)(LCD_BASE + 24 * 4+24*Layer);//0x60
	val &= ~(0xFF <<24);
	val |= ((coeff) <<24);
	*(volatile unsigned *)(LCD_BASE + 24 * 4+24*Layer) = val;*/
}
//#pragma inline
static void LCD_Layer_set_osd_param1(int Layer,int x , int y , unsigned char coeff )
{
   unsigned int val = (((x&0xfff)<<0)|((y&0xfff)<<12)|( (coeff) <<24) );
   switch ( Layer )
   {
   case 0:
      LCD_OSD_0_PARAM1_reg = val;
      break;
   case 1:
      LCD_OSD_1_PARAM1_reg = val;
      break;
   case 2:
      LCD_OSD_2_PARAM1_reg = val;
      break;
   default:
		break;
   }
	//*(volatile unsigned *)(LCD_BASE + 24 * 4+24*Layer) = (((x&0xfff)<<0)|((y&0xfff)<<12)|( (coeff) <<24) );
}

//ÿ��osd֧�����Դ�ֱ��ʣ�argbԴΪ1024��yuv420ԴΪ2048��rgb565ԴΪ2048��rgb454ԴΪ2048
//���ò��С ��� �߶�
//#pragma inline
void LCD_Layer_Size(int Layer,int x,int y )
{
   int data ;
   //int clr = 0xc000003f;/*31 30 ..... 5 4 3 2 1 0*/
   int clr = (~((0xfff<<6)|(0xfff<<18)));
   int set = (((x&0xfff)<<6)|((y&0xfff)<<18));
   switch ( Layer )
   {
   case 0:
      data = LCD_OSD_0_PARAM0_reg ;
      //data &= ~((0xfff<<6)|(0xfff<<18));// 12 bit 
      data &= clr ;// 12+12 bit 
      data |= set;
      LCD_OSD_0_PARAM0_reg = data ;
      break;
   case 1:
      data = LCD_OSD_1_PARAM0_reg ;
      //data &= ~((0xfff<<6)|(0xfff<<18));// 12 bit 
      //data |=(((x&0xfff)<<6)|((y&0xfff)<<18));
      data &= clr ;// 12+12 bit 
      data |= set;
      LCD_OSD_1_PARAM0_reg = data;
      break;
   case 2:
   	data = LCD_OSD_2_PARAM0_reg ;
	   //data &= ~((0xfff<<6)|(0xfff<<18));// 12 bit 
	   //data |=(((x&0xfff)<<6)|((y&0xfff)<<18)) ;
      data &= clr ;// 12+12 bit 
      data |= set;
	   LCD_OSD_2_PARAM0_reg = data ;
      break;
   default:
		break;
   }
}

//ÿ��osd֧�����Դ�ֱ��ʣ�argbԴΪ1024��yuv420ԴΪ2048��rgb565ԴΪ2048��rgb454ԴΪ2048
void LCD_Layer_set_width(int Layer,int x )
{
	int data = *(volatile unsigned *)(LCD_BASE + 23 * 4+24*Layer);
	data &= ~(0xfff<<6);// 12 bit 
	data |=((x&0xfff)<<6);
	*(volatile unsigned *)(LCD_BASE + 23 * 4+24*Layer)=data;
}

//ÿ��osd֧�����Դ�ֱ��ʣ�argbԴΪ1024��yuv420ԴΪ2048��rgb565ԴΪ2048��rgb454ԴΪ2048
void LCD_Layer_set_height(int Layer,int y )
{
	int data = *(volatile unsigned *)(LCD_BASE + 23 * 4+24*Layer);
	data &= ~(0xfff<<18);// 12 bit 
	data |=((y&0xfff)<<18);
	*(volatile unsigned *)(LCD_BASE + 23 * 4+24*Layer)=data;
}

//ʹ�ò� alpha ����
void LCD_Set_Layer_Alpha(int Layer )
{// ��ʾ ͸�����ӣ�Ϊ1��ʾ�����ӣ�0��ʾͼ��a����
   switch ( Layer )
   {
   case 0:
      LCD_OSD_0_PARAM0_reg |= (1<<0);
      break;
   case 1:
      LCD_OSD_1_PARAM0_reg |= (1<<0);
      break;
   case 2:
      LCD_OSD_2_PARAM0_reg |= (1<<0);
      break;
   }
	/*int data = *(volatile unsigned *)(LCD_BASE + 23 * 4+ 24*Layer );
	data |= (1<<0);
	*(volatile unsigned *)(LCD_BASE + 23 * 4+ 24*Layer ) = data;*/
}
//ʹ�����ݵ�alpha����
void LCD_Set_Src_Alpha(int Layer )
{
   switch ( Layer )
   {
   case 0:
      LCD_OSD_0_PARAM0_reg &= ~(1<<0);
      break;
   case 1:
      LCD_OSD_1_PARAM0_reg &= ~(1<<0);
      break;
   case 2:
      LCD_OSD_2_PARAM0_reg &= ~(1<<0);
      break;
   default:
		break;
   }
	/*int data = (*(volatile unsigned *)(LCD_BASE + 23 * 4+ 24*Layer ));
	data &= ~(0x1<<0);
	*(volatile unsigned *)(LCD_BASE + 23 * 4+ 24*Layer ) = data;*/
}





//#pragma inline
void LCD_Layer_fill_address(unsigned int Layer ,unsigned int Y,unsigned int U,unsigned int V )
{
   switch (Layer)
   {
   case 0:
      LCD_OSD_0_PARAM2_reg = Y;
      LCD_OSD_0_PARAM3_reg = U;
      LCD_OSD_0_PARAM4_reg = V;
      break;
   case 1:
      LCD_OSD_1_PARAM2_reg = Y;
      LCD_OSD_1_PARAM3_reg = U;
      LCD_OSD_1_PARAM4_reg = V;
      break;
   case 2:
      LCD_OSD_2_PARAM2_reg = Y;
      LCD_OSD_2_PARAM3_reg = U;
      LCD_OSD_2_PARAM4_reg = V;
      break;
   default:
		break;
   }
}

/*******************************************************************************
* lcd_set_source_addr
* ������������ݸ�ʽ���Լ����ݿ�ʼλ�ã����� YUV ���Ե�ֵ
*  
********************************************************************************/

void lcd_calculate_yuv_addr(unsigned char *frame, unsigned int w,unsigned h, int format, 
											unsigned int *yin, unsigned int *uin, unsigned int *vin  )
{
	*yin = (int)frame;
	if(    format == XM_OSD_LAYER_FORMAT_YUV420 )
	{
		*uin = (int)(frame+w*h);
		*vin = (int)(frame+w*h+w*h/4);
	}
	else if(   format ==  XM_OSD_LAYER_FORMAT_Y_UV420 )
	{
		*uin = (int)(frame+w*h);
		*vin = 0;
	}
	else if(   format == XM_OSD_LAYER_FORMAT_ARGB888   
			  || format == XM_OSD_LAYER_FORMAT_RGB565 
			  || format == XM_OSD_LAYER_FORMAT_ARGB454  )
	{
		*uin =0;
		*vin =0;
	}
	else
	{
		printf("lcd format error !\n");
		return ;
	}
}

/*******************************************************************************
* Osd_rgb_order
*  layer   OSD���
*  order   RGB��ɫ���� �����£�
*  0��RGB
*  1��RBG
*  2��GRB
*  3��BRG
*  4��GBR
*  5��BGR
*  ������RGB
********************************************************************************/
void  Osd_rgb_order(int layer, char order )
{
	int value = LCD_OSD_YUV420_config_reg ;
	// order value: from 0 to 5 ,�����ֵΪ7  0x111����
	value &=~ (0x7<<(layer<<2));
	value |=  ((order&0x7)<<(layer<<2));
	LCD_OSD_YUV420_config_reg =   value;
}
/*******************************************************************************
*  LCD_Layer_Set_Mode()
*  �����б�
*  Layer  OSD���  ȡֵ��Χ 0--2 ����֧��3��OSD
*  format OSD����Դ��ʽ��ȡֵ��Χ���Ҽ�ת������鿴=>  XM_OSD_LAYER_FORMAT_YUV420
*  format ȡֵ��Χ 0~~3,��Ӧ֧�� 0:YUV 1:ARGB888 2:RGB565 3:ARGB454
********************************************************************************/
void LCD_Layer_Set_Mode(unsigned int Layer,unsigned char format )
{
	//���� UV������֯��ʽ 
	unsigned int val = LCD_OSD_YUV420_config_reg;
	val &= ~(0x3<<(12+(Layer<<1)));
	if( format == XM_OSD_LAYER_FORMAT_Y_UV420 )
		val |= (0x1<<(12+(Layer<<1)));
		//val |= (0x3<<(12+Layer));// ����Ϊ��ֵ�� ����������ɫ(����UV���ݵ�λ��)  Y_UV420
	LCD_OSD_YUV420_config_reg = val;
	
   if(format == XM_OSD_LAYER_FORMAT_YUV420 || format == XM_OSD_LAYER_FORMAT_Y_UV420  )
		format = 0;
   
	//���� format 
	/*val = *(volatile unsigned *)(LCD_BASE + 23 * 4+ 24*Layer );
	val &= ~(0x3<<3);
	val |= ((format&0x3)<<3);
	*(volatile unsigned *)(LCD_BASE + 23 * 4+ 24*Layer ) = val;*/
   switch (Layer)
   {
   case 0:
      val = LCD_OSD_0_PARAM0_reg;
      val &= ~(0x3<<3);
      val |= ((format&0x3)<<3);
      LCD_OSD_0_PARAM0_reg = val;
      break;
   case 1:
      val = LCD_OSD_1_PARAM0_reg;
      val &= ~(0x3<<3);
      val |= ((format&0x3)<<3);
      LCD_OSD_1_PARAM0_reg = val;
      break;
   case 2:
      val = LCD_OSD_2_PARAM0_reg;
      val &= ~(0x3<<3);
      val |= ((format&0x3)<<3);
      LCD_OSD_2_PARAM0_reg = val;
      break;
   default:
		break;
   }
}

int LCD_Layer_Get_Mode(unsigned int Layer  )
{//  Get  mode
   unsigned int mode;
   unsigned int val;
   switch (Layer)
   {
   case 0:
      val = LCD_OSD_0_PARAM0_reg;
      break;
   case 1:
      val = LCD_OSD_1_PARAM0_reg;
      break;
   case 2:
      val = LCD_OSD_2_PARAM0_reg;
      break;
   default:
		break;
   }
   
   mode = ((val >> 3) & 3);
   if( !mode )// ==0 ��ʾ��YUV ���� ��ʽ 
   {
      //Ȼ���ж��� YUV420 ���� Y_UV420 ?
      if( (LCD_OSD_YUV420_config_reg & (0x1<<(12+(Layer<<1))))  )
         mode = XM_OSD_LAYER_FORMAT_Y_UV420;
   }
	return  mode;
}



/*******************************************************************************
* ��͸����
* global         ȡֵ��Χ0~255  ��ʾ��alpha�仯  
* leftmult       ȡֵ��Χ0-63   ����ɫ ��������һ����ʾ
********************************************************************************/
/*******************************************************************************
* LCD_Move()   �ƶ�ĳOSD�㵽����(x,y)��
* �����б�
* osd_layer         ȡֵ��Χ0~2  ��ʾ���  
* channel_w       ��ʾͨ���Ŀ��
* channel_h       ��ʾͨ���ĸ߶�
* osd_width       ��ʾOSD��Ŀ�ȡ��߶�
* osd_height
* Coeff,           OSD 0������ݳ˻����ɸı�ͼ��RGB��С,ͼ���ɺ�ɫ
* global_coeff,    Blendingϵ�� alpha
* x               ��ʾOSD�㶥����ʾ����� (x,y)
* y 
* osd_layer_format  ��ʾOSD�������Դ��ʽ ��ȡֵ��Χ 0--3 
* osd_y_addr        ��ʾOSD�������YUV��ַ,YUV420��Ҫ��д3����ַ��Y_UV420��Ҫ��д������ַ��������ʽֻ��Ҫ��дY��ַ
* osd_u_addr
* osd_v_addr
********************************************************************************/
//int LCD_OSD_Changed = 0;
/*
      ʵ�ʲ��� lcd.c ���� LCD_Move ��̫��Ĵ�������  
      Ӱ�쵽��ITU601 ���뵽 scale �������ж� ��Ӧ�ܿ� �����ز������� 
*/
void LCD_Move (unsigned int osd_layer,
					int channel_w, int channel_h ,int osd_width,int osd_height, unsigned char Coeff,unsigned char global_coeff,
					int x,int y , int osd_layer_format ,
					unsigned int osd_y_addr,unsigned int osd_u_addr,unsigned int osd_v_addr,unsigned int src_width )
{
	unsigned int reg_h_position, reg_v_position, reg_left_position;
	unsigned int reg_y, reg_u, reg_v=0;
	int reg_height,reg_width;
	unsigned int align_h, align_v;

	reg_width  = osd_width;
	reg_height = osd_height;

	align_h = osd_minimum_moving_h_pixels_clr[osd_layer_format];
	align_v = osd_minimum_moving_v_pixels_clr[osd_layer_format];
   
	if(   osd_layer_format == XM_OSD_LAYER_FORMAT_YUV420 
		|| osd_layer_format == XM_OSD_LAYER_FORMAT_Y_UV420 )
	{
		// YUV420��ʽ
		if(x >= 0) //ˮƽ����
		{
			reg_h_position = x;
			// Ӧ��ˮƽ�ƶ���С����Լ��
			//reg_h_position = (reg_h_position / align_h) * align_h;
			reg_h_position &= ~ align_h;
			reg_left_position = 0;
			if(reg_h_position+reg_width > channel_w ) // �� ��Ļ�ڣ�λ��+OSD��ȡ�=��Ļ���
				reg_width = channel_w-reg_h_position;
		}
		else                                //ˮƽ����
		{
			reg_h_position = 0;
			reg_left_position = - x;
			// Ӧ��ˮƽ�ƶ���С����Լ��
			//reg_left_position = (reg_left_position / align_h) * align_h;
			reg_left_position &= ~ align_h;
         if(osd_width <= reg_left_position)
            reg_width = osd_width-reg_left_position;
         else if( osd_width - reg_left_position < channel_w )
            reg_width = osd_width - reg_left_position;
         else if(reg_h_position+reg_width > channel_w ) // �� ��Ļ�ڣ�λ��+OSD��ȡ�=��Ļ���
				reg_width = channel_w-reg_h_position;//-reg_left_position;
         else 
            reg_width = reg_width-reg_left_position;
		}

		if(y >= 0)
		{
			// OSD��Ķ���λ����ʾ��   (�·�)
			// Ӧ�ô�ֱ������С�ƶ�����Լ��
			reg_v_position = y &~ align_v;
			reg_y = osd_y_addr;
			reg_u = osd_u_addr;
			if(osd_layer_format == XM_OSD_LAYER_FORMAT_YUV420 )
				reg_v = osd_v_addr;
         if( osd_height > channel_h )
            reg_height = osd_height-y;
			else if( reg_v_position+reg_height > channel_h )
				reg_height = channel_h-reg_v_position;//��Ļ�߶�-����߶�;���ٴ�������
		}
		else
		{
			// OSD��Ķ���λ����ʾ���Ķ���֮�� ( �Ϸ� )

			int y_offset = (-y);
			// Ӧ�ô�ֱ������С�ƶ�����Լ��
			//y_offset = (y_offset / align_v) * align_v;
			y_offset &= ~ align_v;//�����ƶ����ƶ����ݲ������Ƶ�Ч���У�����ż��λ���ƶ�
			reg_v_position = 0;
			reg_y = osd_y_addr + (y_offset) * src_width;
         if(osd_layer_format == XM_OSD_LAYER_FORMAT_Y_UV420 )
			   reg_u = osd_u_addr + (((y_offset) * src_width )>>1) ;
         else if(osd_layer_format == XM_OSD_LAYER_FORMAT_YUV420 )
			{
            reg_u = osd_u_addr + (((y_offset) * src_width )>>2) ;
				reg_v = osd_v_addr + (((y_offset) * src_width )>>2) ;
         }			// �޸�OSDƽ��߶�
			if(osd_height+y < channel_h)
		      reg_height += y;
		}
		// ����Ƿ�Ӧ��Լ�����������OSDӲ����ȱ��
	}
	else
	{
		// RGB��ʽ
		if( x >= 0)// ���ұ��ƶ�
		{
			reg_h_position = x;
			// Ӧ��ˮƽ�ƶ���С����Լ��
			//reg_h_position = (reg_h_position / align_h) * align_h;
			reg_h_position &= ~ align_h;
			reg_left_position = 0 ;
			if(reg_h_position+reg_width > channel_w ) // �� ��Ļ�ڣ�λ��+OSD��ȡ�=��Ļ���
				reg_width = reg_width-(reg_h_position+reg_width-channel_w);
		}
		else                                // �����ƶ�
		{
			reg_h_position = 0;
			reg_left_position = - x;
			// Ӧ��ˮƽ�ƶ���С����Լ��
			//reg_left_position = (reg_left_position / align_h) * align_h;
			reg_left_position &= ~  align_h;
         if(osd_width <= reg_left_position)
            reg_width = osd_width-reg_left_position;
         else if( osd_width - reg_left_position < channel_w )
            reg_width = osd_width - reg_left_position;
         else if(reg_h_position+reg_width > channel_w ) // �� ��Ļ�ڣ�λ��+OSD��ȡ�=��Ļ���
				reg_width = channel_w-reg_h_position;//-reg_left_position;
         else 
            reg_width = reg_width-reg_left_position;
		}

		if( y >= 0)
		{
			// Ӧ�ô�ֱ������С�ƶ�����Լ�� (�·�)
			//reg_v_position = (xm_osdlayer->osd_y_position / align_v) * align_v;
			reg_v_position = y &~ align_v;
			//reg_v_position &= ~ align_v;
			reg_y = osd_y_addr;
			reg_u = NULL;
			reg_v = NULL;
			// �޸�OSDƽ��߶�
	//		if( reg_v_position+reg_height > channel_h )
				//reg_height = channel_h-reg_v_position;//��Ļ�߶�-����߶�;���ٴ�������
			if( reg_v_position+reg_height > channel_h )
				reg_height = channel_h-reg_v_position;//��Ļ�߶�-��
		}
		else
		{
			// OSD��Ķ���λ����ʾ���Ķ���֮�� (�Ϸ�)
			int y_offset = -y;
			// Ӧ�ô�ֱ������С�ƶ�����Լ��
			//y_offset = (y_offset / align_v) * align_v;
			//y_offset &= ~ align_v;//����ʡ��
			reg_v_position = 0;
			if(osd_layer_format == XM_OSD_LAYER_FORMAT_ARGB888 )
				reg_y = osd_y_addr + (y_offset) * (osd_width <<2);
			else
				reg_y = osd_y_addr + (y_offset) * (osd_width <<1);
			reg_u = NULL;
			reg_v = NULL;
			// �޸�OSDƽ��߶�
			reg_height += y;
		}
		// ����Ƿ�Ӧ��Լ�����������OSDӲ����ȱ��
	}
	//irq_disable ( LCD_INT );
	//OS_DI();// ��֤������ȼ� �������
	//����Ƿ�����ʾ��������ʾ����
	//  ARGB���� ��Y�����ϱ��뱣������4�������ݣ������´���ʾ��ʧ
	if( reg_width<=0 || reg_height<=4 /*|| reg_left_position <= 2*/ )
	{
		LCD_Layer_Clr ( osd_layer );
		//irq_enable ( LCD_INT );
		//OS_EI();
		return ;
	}
	LCD_Layer_Set_Mode( osd_layer , osd_layer_format );
	
	LCD_Layer_Size(osd_layer,reg_width,reg_height);

	LCD_Layer_set_osd_param1(osd_layer,reg_h_position,reg_v_position, global_coeff );
	//                                                       ����Դ
	LCD_Layer_set_osd_param5(osd_layer, reg_left_position ,src_width , Coeff );

	LCD_Layer_fill_address(osd_layer ,reg_y, reg_u, reg_v);	

	LCD_Layer_Set (osd_layer);
	
	//irq_enable ( LCD_INT );
	//OS_EI();
}



/*
   LCD_Move_Src �ú��������ֹ�ж� ����Ϊ��Ҫ�������Ӧ���� �����ж����� 
*/
void LCD_Move_Src (unsigned int osd_layer,
					int channel_w, int channel_h ,int osd_width,int osd_height, unsigned char Coeff,unsigned char global_coeff,
					int x,int y , int osd_layer_format ,
					unsigned int osd_y_addr,unsigned int osd_u_addr,unsigned int osd_v_addr,unsigned int src_width )
{
	unsigned int reg_h_position, reg_v_position, reg_left_position;
	unsigned int reg_y, reg_u, reg_v=0;
	int reg_height,reg_width;
	unsigned int align_h, align_v;

	align_h = osd_minimum_moving_h_pixels_clr[osd_layer_format];
	align_v = osd_minimum_moving_v_pixels_clr[osd_layer_format];
   
   x &= ~align_h;
   y &= ~align_v;
      
	if(   osd_layer_format == XM_OSD_LAYER_FORMAT_YUV420 
		|| osd_layer_format == XM_OSD_LAYER_FORMAT_Y_UV420 )
	{
     reg_y =  osd_y_addr+ x+y*src_width;
     
     if(osd_layer_format == XM_OSD_LAYER_FORMAT_YUV420 )
     {
        reg_u =  osd_u_addr + x/2 + y*src_width/4;
        reg_v =  osd_v_addr + x/2 + y*src_width/4;
     }
     else if( osd_layer_format == XM_OSD_LAYER_FORMAT_Y_UV420  )
     {
        reg_u =  osd_u_addr +  x + y*src_width/2;
        reg_v =  0;
     }
	}
	else if( osd_layer_format == XM_OSD_LAYER_FORMAT_ARGB888 )
	{
      reg_y =  osd_y_addr+ (x+y*src_width)*4;
      reg_u =  0;
      reg_v =  0;

	}

	LCD_Layer_Set_Mode( osd_layer , osd_layer_format );
   //����OSD ��� �߶� 
	LCD_Layer_Size(osd_layer , osd_width ,  osd_height );

	LCD_Layer_set_osd_param1(osd_layer,0,0, global_coeff );
	//                                                       ����Դ
	LCD_Layer_set_osd_param5(osd_layer, 0 ,src_width , Coeff );

	if(   osd_layer_format == XM_OSD_LAYER_FORMAT_YUV420   )
	   LCD_Layer_fill_address(osd_layer ,reg_y, reg_u, reg_v);	
      
   else if(osd_layer_format == XM_OSD_LAYER_FORMAT_Y_UV420 )
      LCD_Layer_fill_address(osd_layer ,reg_y, reg_u, 0);	

   else
      LCD_Layer_fill_address(osd_layer ,reg_y, 0, 0);	

	LCD_Layer_Set (osd_layer);
}

void LCDSetGAMMA_RGB(float *a,UINT32 * addr )
{
	UINT32 cycle;
	UINT8 val0,val1,val2,val3;

	float roundret;
	printf("a=%f\n",*a);
	for(cycle=0 ; cycle<0x100 ; cycle=cycle+16 )
	{
		//pow(a,b); //a��b�η� 
		roundret = pow( cycle/(255.0f), (float)(*a) )*255.0f;
		val0=(UINT8)(roundret+0.5f);
		roundret = pow( (cycle+4)/(255.0f), (float)(*a) )*255.0f;
		val1=(UINT8)(roundret+0.5f);
		roundret = pow( (cycle+8)/(255.0f), (float)(*a) )*255.0f;
		val2=(UINT8)(roundret+0.5f);
		roundret = pow( (cycle+12)/(255.0f), (float)(*a) )*255.0f;
		val3=(UINT8)(roundret+0.5f);
		
	//	printf ("val0=%d\n", val0);
	//	printf ("val1=%d\n", val1);
	//	printf ("val2=%d\n", val2);
	//	printf ("val3=%d\n", val3);

		*addr = (val0)|(val1<<8)|(val2<<16)|(val3<<24);
		addr++ ;
	}
}
void LCDSetGAMMA(float *r ,float *g ,float *b )
{
	#define  GAMMA_BASE    LCD_BASE
	//��Ҫ���� ���Ƿ�gamm��������
	UINT32 *addr ;
	*((volatile unsigned int *)(GAMMA_BASE + 0x013C)) = 0x3;
	
	addr = (UINT32*)(GAMMA_BASE+0x0140);// custom_r_val01
	LCDSetGAMMA_RGB( r ,addr );
	
	addr = (UINT32*)(GAMMA_BASE+0x0180); // custom_g_val01
	LCDSetGAMMA_RGB( g ,addr );
	
	addr = (UINT32*)(GAMMA_BASE+0x01C0); // custom_b_val01
	LCDSetGAMMA_RGB( b ,addr );
	
//	*((volatile unsigned int *)(GAMMA_BASE + 0x013C)) = 0x2;//ִ����Ϻ󣬲�������gamma
}




/*******************************************************************************
*  void LCD_YUV2RGB_Bypass()
*  ����YUV תRGB ֱͨ 
*  ������bypass ycbcr to rgbģʽ�������Ҫ��VDEֱͨ�Ļ���
*  ����Ҫ��RGB������ת����YCbCr������Ҫ��vde���������ת����RGB��
*  ��Ҫ������ģ�鶼bypass����
*  RGB to YCbCrģ��ֱͨ�ļĴ���Ϊ0x200 bit 29��1��
*  vdeģ��ֱͨ�ļĴ���Ϊ0x210 bit��3:0����8��
*  YCbCr to RGBģ��ֱͨ�ļĴ���Ϊ0x214 bit 27��1.
NOTE:====== 20150618 ֮��İ汾���д˸�������
*********************************************************************************/
void LCD_YUV2RGB_Bypass()
{//
   unsigned int val;
   //0x200 bit 29��1��
   VP_RGB2YCBCR_COEF0_reg |= (1<<29);
   
   // 0x210 bit��3:0����8��
   val = VP_CONTROL_reg;
   val &= 0xfffffff0;
   VP_CONTROL_reg = val;
      
   //0x214 bit 27��1.
   LCD_dithing_reg |= (1<<27);
}

// LCD��ʾ����ʹ��
static int lcd_display_enable = 0;
void LCD_set_enable (int enable)
{
	XM_lock ();
	lcd_display_enable = enable;
	if(lcd_display_enable)
		LCD_PARAM0_reg |= 1;
	else
		LCD_PARAM0_reg &= ~1;
	XM_unlock ();

	if(lcd_display_enable)
	{
		HW_LCD_BackLightOn();
	}
}

int LCD_get_enable (void)
{
	return lcd_display_enable;
}