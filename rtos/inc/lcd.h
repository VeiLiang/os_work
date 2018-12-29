#ifndef   _lcd_h_
#define   _lcd_h_

//#define BUZZER_EN//���õĵط�Ҫ�������ͷ�ļ�����Ч,#include  "lcd.h"
//#define VCOM_EN//���õĵط�Ҫ�������ͷ�ļ�����Ч,#include  "lcd.h"

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

	
//��֤32λ����
typedef struct _OSDINFO{
  unsigned char  mode;
  unsigned char  on;
  unsigned char  dirty;
  unsigned char  Coeff;  // 0:��������alpha 1:���ò�Alpha 2:ʹ�úڻ�alpha
  
  unsigned short alpha;
  unsigned short w;      // OSD ��� �߶�    �����ֵ2048 
  unsigned short h;     
  unsigned short set;// ��ʾ ��ǰ���õı������� set =0 û������ set =1 ��ǰ�Ѿ�����
  short x;              // OSD  ���� ����Ϊ����
  
  short y;   
  unsigned short SrcW;   // OSD  ����Դ���  �����ֵ2048  ARGB:1024
  unsigned short SrcH;
  unsigned short Reserved;//�� 
 
  unsigned int  Ysrc;    // OSD  ����Դ��ַ 
  unsigned int  Usrc;    
  unsigned int  Vsrc;
  unsigned int  YUVend; // ������ַ  
  unsigned int  Y_Addr;  //    ������Ҫ
  unsigned int  U_Addr;  //
  unsigned int  V_Addr;  //
} OSDINFO;
	
//�����ƶ���Ҫ����Դ���ݵ�ַ ʹ������
//�����ƶ���Ҫ����ͼ����
//����Ļ�߽��ƶ�ʱ��������size��Ӧ��С
	
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
//ÿ��osd֧�����Դ�ֱ��ʣ�argbԴΪ1024��yuv420ԴΪ2048��rgb565ԴΪ2048��rgb454ԴΪ2048
void LCD_Layer_set_width(int Layer,int x );
//ÿ��osd֧�����Դ�ֱ��ʣ�argbԴΪ1024��yuv420ԴΪ2048��rgb565ԴΪ2048��rgb454ԴΪ2048
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

// ��������ʹ��
void LCD_set_enable (int enable);
int LCD_get_enable (void);


void HW_LCD_Init (void);
// RGB LCD�ر��˳�
void HW_LCD_Exit(void);

// ��������
void HW_LCD_BackLightOn (void);
// �رձ���
void HW_LCD_BackLightOff (void);
// ���ñ��⼶�� (0.0 ~ 1.0)
void HW_LCD_BackLightSetLevel (float level);
// ����ON/OFF״̬�л�
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