#ifndef   _lcd_h_
#define   _lcd_h_

//#define BUZZER_EN//调用的地方要包含这个头文件在有效,#include  "lcd.h"
//#define VCOM_EN//调用的地方要包含这个头文件在有效,#include  "lcd.h"

#define  XPIX   800
#define  YPIX   180

#if defined(__cplusplus)
extern "C" {     /* Make sure we have C-declarations in C++ programs */
#endif
	
#define  global_coeff_Max_Val    		255
#define  left_mult_coef_Max_Val       63
#define  mult_coef_Max_Val            63
	
#define  Lay0             0
#define  Lay1             1
#define  Lay2             2

typedef struct _itudata_s {
	struct _itudata_s			*prev;
	struct _itudata_s			*next;
	unsigned int   Ydata;
	unsigned int   Pbprdata;
} itudata_s;

	
//保证32位对齐
typedef struct _OSDINFO{
  unsigned char  mode;
  unsigned char  on;
  unsigned char  dirty;
  unsigned char  Coeff;  // 0:采用数据alpha 1:采用层Alpha 2:使用黑化alpha
  
  unsigned short alpha;
  unsigned short w;      // OSD 宽度 高度    最大数值2048 
  unsigned short h;     
  unsigned short set;// 表示 当前设置的备份数据 set =0 没有设置 set =1 当前已经设置
  short x;              // OSD  坐标 可以为负数
  
  short y;   
  unsigned short SrcW;   // OSD  数据源宽度  最大数值2048  ARGB:1024
  unsigned short SrcH;
  unsigned short Reserved;//绷 
 
  unsigned int  Ysrc;    // OSD  数据源地址 
  unsigned int  Usrc;    
  unsigned int  Vsrc;
  unsigned int  YUVend; // 结束地址  
  unsigned int  Y_Addr;  //    上移需要
  unsigned int  U_Addr;  //
  unsigned int  V_Addr;  //
} OSDINFO;
	
//向上移动需要保存源数据地址 使用数据
//向左移动需要保存图像宽度
//向屏幕边界移动时，将数据size相应变小
	
void LCD_Layer_Set(int Layer) ;
void LCD_Layer_Clr(int Layer) ;
void LCD_Layer_MoveLeft(int Layer,int Offset  );
void LCD_Layer_Move(int Layer,int x  ,int y);
void LCD_Layer_fill_address(unsigned int Layer ,unsigned int Y,unsigned int U,unsigned int V );

void LCD_Layer_Size(int Layer,int x,int y );
void LCD_Set_Src_Alpha(int Layer );
void LCD_Set_Layer_Alpha(int Layer );

void LCD_Move (unsigned int osd_layer,
					int channel_w, int channel_h ,int osd_width,int osd_height, unsigned char Coeff,unsigned char global_coeff,
					int x,int y , int osd_layer_format ,
					unsigned int osd_y_addr,unsigned int osd_u_addr,unsigned int osd_v_addr,unsigned int src_width );

void  LCD_Layer_Set_Mode(unsigned int Layer,unsigned char mode );
int  LCD_Layer_Get_Mode(unsigned int Layer  );
void LCD_Layer_ShiftUP(int Layer,int x,int y );
void LCD_Set_SrcWH(int Layer,int weight ,int height );

void LCD_Set_background_color(unsigned char r, unsigned char g, unsigned char b );
void LCD_Layer_set_global_coeff(int Layer, unsigned char coeff );
//每层osd支持最大源分辨率，argb源为1024，yuv420源为2048，rgb565源为2048，rgb454源为2048
void LCD_Layer_set_width(int Layer,int x );
//每层osd支持最大源分辨率，argb源为1024，yuv420源为2048，rgb565源为2048，rgb454源为2048
void LCD_Layer_set_height(int Layer,int y );
void LCD_Layer_h_position(int Layer,int x );
void LCD_Layer_v_position(int Layer, int y );
void LCD_Layer_set_left_position(int Layer,int Offset  );
void LCD_Layer_set_stride(int Layer,int Stride);
void LCD_Layer_set_brightness_coeff(int Layer,unsigned char coeff  );


void LCD_Init(void );
void LCD_SelPad(void );
void LCDIntHander (void);
void LCDIntHander_isp (void);
void LCDIntHander_doublebuffer (void);
void LCDIntHander_itu656 (void);
void LCDIntHander_int_time (void);
void LCDIntHander_cvbs (void);

void LCD_Read_OSDINFO(void);

// 设置屏显使能
void LCD_set_enable (int enable);
int LCD_get_enable (void);


void HW_LCD_Init (void);
// RGB LCD关闭退出
void HW_LCD_Exit(void);

// 开启背光
void HW_LCD_BackLightOn (void);
// 关闭背光
void HW_LCD_BackLightOff (void);
// 设置背光级别 (0.0 ~ 1.0)
void HW_LCD_BackLightSetLevel (float level);
// 背光ON/OFF状态切换
void HW_LCD_BackLightToggleOnOff (void);
void buzzer_off();
void buzzer_init(void);
void buzzer_onoff(unsigned char onff);
void vcom_init (void);
void vcom_adjust (unsigned char adjust_flag);
unsigned char get_buzzer_time_stop(void);

#if defined(__cplusplus)
}                /* Make sure we have C-declarations in C++ programs */
#endif

#endif