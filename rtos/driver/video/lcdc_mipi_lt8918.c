#include "hardware.h"
#include "gpioi2c.h"
#include "lcd.h"
#include <stdio.h>
#include <math.h>
#include "hw_osd_layer.h"
#include <gpio.h>
#include "pwm.h"
#include "xm_dev.h"
#include "rtos.h"


#ifdef _SUPPORT_LCD_MIPI_LT8918_

#define I2C_RETRY_NUM			3

static unsigned int DEV_ADDR = 0x40;

static unsigned char ReadI2C_Byte(unsigned char address)
{
	 int ret = -1;
	 int retries = 0;
	 unsigned char dat=0;

	 while(retries < I2C_RETRY_NUM)
	 {
	 	ret = lt8918_i2c_read_bytes (DEV_ADDR, &address, 1, &dat, 1);
		if(ret == 1)
			break;

		retries++;
	 }
	 if(retries >= I2C_RETRY_NUM)
	 {
		XM_printf("%s i2c read dev:%x addr:%x error\n",__func__,DEV_ADDR,address);
		ret = -1;
	 }	
	 	
	 if(ret < 0)
	 {
		 return 0xFF;
	 }
	 else
	 {
		 return dat;
	 }
}


static unsigned char WriteI2C_Byte(unsigned char address, unsigned char data)
{
	int ret = -1;
	int retries = 0;

	while(retries < I2C_RETRY_NUM)
	{
		ret = lt8918_i2c_write_bytes (DEV_ADDR, &address, 1, &data, 1);
		if(ret == 0)
			break;
		
		retries++;
	}
	
	if(retries >= I2C_RETRY_NUM)
	{
		XM_printf("%s i2c write dev:%x addr:%x error\n",__func__,DEV_ADDR,address);
		ret = -1;
	}
	
	if(ret < 0)
	{
		return 0xFF;
	}
	else
	{
		return 0;
	}
}


static char ReadI2C_ByteN(unsigned char address,char *rxdata, int length)
{
	 int ret = 0;
	 int retries = 0;

	 while(retries < I2C_RETRY_NUM)
	 {
	 	ret = lt8918_i2c_read_bytes (DEV_ADDR, &address, 1, rxdata, length);
		if(ret == length)
			break;

		retries++;
	 }
	 if(retries >= I2C_RETRY_NUM)
	 {
		XM_printf("%s i2c read dev:%x addr:%x error\n",__func__,DEV_ADDR,address);
		ret = -1;
	 }	
	 return ret;
}


void SetRegisterBank(unsigned char adr)//lt8918 reg_group select
{
	WriteI2C_Byte(0xff,adr);
}


void DcsLongPktWrite(unsigned char DataID,unsigned char Len)
{
	u8 i = 0;
	SetRegisterBank(0x83);
	WriteI2C_Byte(0x40,0x04);

	WriteI2C_Byte(0x32,0x0E);
	WriteI2C_Byte(0x33,(Len+6));
	WriteI2C_Byte(0x34,DataID);
	WriteI2C_Byte(0x34,Len);
	WriteI2C_Byte(0x34,0x00);

	for(i=0;i<Len;i++)
	{
		WriteI2C_Byte(0x34,0x00);
	}
	WriteI2C_Byte(0x32,0x00);
}

void DcsShortPktWrite(unsigned char DataID,unsigned char Data0,unsigned char Data1)
{   
	SetRegisterBank(0x83);
	WriteI2C_Byte(0x40,0x04);

	WriteI2C_Byte(0x32,0x0C);
	WriteI2C_Byte(0x33,0x04);
	WriteI2C_Byte(0x34,DataID);
	WriteI2C_Byte(0x34,Data0);
	WriteI2C_Byte(0x34,Data1);
	WriteI2C_Byte(0x32,0x00);
}


void Lt8918_CfgI2cEnable(void)
{
	SetRegisterBank(0x60);
	WriteI2C_Byte(0xee,0x01);//cfg iic enable
}

void Lt8918_GpioEnable(void)
{
	SetRegisterBank(0x70);
	WriteI2C_Byte(0x42,0x09);//Digital test output enable:
	WriteI2C_Byte(0x44,0xff);//GPIO7~GPIO0 output enable:
	WriteI2C_Byte(0x46,0xff);//GPIO7~GPIO0 output driving capability:
}

void Lt8918_GpoConfig(void)
{
	SetRegisterBank(0x80);
	WriteI2C_Byte(0x70,0x20);//0100 = MIPI TX dcs test output;
	WriteI2C_Byte(0x71,0x00);//GPO4~7 output select 00 = Blk_test_out[3];
	WriteI2C_Byte(0x72,0x00);//GPO0~3 output select 00 = Blk_test_out[3];
}

void Lt8918_DesscPllInit(void)//Bypass Mode
{
	SetRegisterBank(0x70);
	WriteI2C_Byte(0x38,0x00);
	//b7= RG_DESSC_PLL_REF_SEL :1 = Pix clock as reference.
	//b5= RG_DESSC_PLL_PD :1 = Power down.
	SetRegisterBank(0x80);
	WriteI2C_Byte(0xa1,0x00);
	//b7 PLL divide ratio source selction :1 = PLL divide ratio from DIV_RATIO_EXT.
	//b6:0 Set divider ratio of acr audio pll
}

void Lt8918_DesscFreqSet(void)
{//refer to <LT8918_PLL_Setting.xls> sheet1: DeSSCPLL Setting
	SetRegisterBank(0x80);
	WriteI2C_Byte(0xa5,0xca);
	WriteI2C_Byte(0xa9,0x91);
	WriteI2C_Byte(0xaa,0x18);
	WriteI2C_Byte(0xab,0x7e);
	WriteI2C_Byte(0xac,0x7c);
	WriteI2C_Byte(0xad,0x00);
	WriteI2C_Byte(0xad,0x02);
}

void Lt8918_MLRXInit(void)
{	
	SetRegisterBank(0x70);//RX PHY Analog Init
	WriteI2C_Byte(0x01,0x80);
	WriteI2C_Byte(0x03,0x48);
	WriteI2C_Byte(0x04,0xa2);
	WriteI2C_Byte(0x0c,0x80);
	WriteI2C_Byte(0x13,0x80);
	WriteI2C_Byte(0x18,0x50);	
	WriteI2C_Byte(0x38,0xb0);	
}
void Lt8918_MLRX_BT1120_Init(void)
{	
	SetRegisterBank(0x80);//40 ff 80 00
	WriteI2C_Byte(0x02,0x70);//40 02 70 00  //r/g swap
	WriteI2C_Byte(0x03,0x02);//40 03 02 00  //p:02; i:06  1920X1080P
	WriteI2C_Byte(0x04,0x07);//40 04 07 00  //h_act[15:8] 
	WriteI2C_Byte(0x05,0x80);//40 05 80 00  //h_act[7:0]   1920
	WriteI2C_Byte(0x06,0x00);//40 06 00 00  //h_fp[15:8]  
	WriteI2C_Byte(0x07,0x10);//40 07 58 00  //h_fp[7:0]	88
	WriteI2C_Byte(0x08,0x00);//40 08 00 00  //h_hw[15:8]  
	WriteI2C_Byte(0x09,0x3E);//40 09 2C 00  //h_hw[7:0]	44
	WriteI2C_Byte(0x0A,0x00);//40 0a 00 00  //v_fp[15:8]  
	WriteI2C_Byte(0x0B,0x09);//40 0b 04 00  //v_fp[7:0]	4
	WriteI2C_Byte(0x0C,0x00);//40 0c 00 00  //v_vw[15:8]  
	WriteI2C_Byte(0x0D,0x06);//40 0d 05 00  //v_vw[7:0]	5
	WriteI2C_Byte(0x0E,0x00);//40 0e 00 00  //2nd_v_hset
	WriteI2C_Byte(0x0F,0x00);//40 0f 00 00  //2nd_v_hset
	//40 00 10 ff
	WriteI2C_Byte(0x6A,0x01);//40 6a 01 00  //422to444
	WriteI2C_Byte(0x6B,0x40);//40 6b 40 00  //444to rgb
	
}

void Lt8918_TxPllConfig(void)
{//refer to <LT8918_PLL_Setting.xls> sheet1: TXPLL Setting
	SetRegisterBank(0x70);
	WriteI2C_Byte(0x30,0x02);
	WriteI2C_Byte(0x31,0x2c);
	WriteI2C_Byte(0x33,0x22);  //pclk->60M
	//WriteI2C_Byte(0x33,0x18);  //pclk->40M
	WriteI2C_Byte(0x34,0x02);
}
void Lt8918_OutputConfig(void)
{
	SetRegisterBank(0x70);
	WriteI2C_Byte(0x24,0x44);    
	WriteI2C_Byte(0x23,0x2f);
	WriteI2C_Byte(0x23,0xaf);
	WriteI2C_Byte(0x23,0x9f);   
}

void Lt8918_InputConfig(void)
{
	SetRegisterBank(0x80);
	//WriteI2C_Byte(0x02,0x70);
	WriteI2C_Byte(0x02,0x00);
    //b4:6 RGB channel swap select: 111 = D[23:0] = RGB.
	WriteI2C_Byte(0x03,0xc0); //bit[7:6] V/H polarity adjust enable
	WriteI2C_Byte(0x33,0x2c);
}

void Lt8918_PtnDataConfig(void)
{
	SetRegisterBank(0x80);//Tx pattern
	WriteI2C_Byte(0xbe,0xc0);
	WriteI2C_Byte(0xbf,0x3f);//Pattern style:blue=01 green=0x02 red=0x04 H_gray=0x37 V_gray=0x3f
	WriteI2C_Byte(0xc0,0xff);//PTN_DATA_VALUE
	WriteI2C_Byte(0xc1,0x00);//De_dly[15:8]
	WriteI2C_Byte(0xc2,0x32);//De_dly[7:0]=hbp+hs 
	WriteI2C_Byte(0xc3,0x32);//De_top=vbp+vs
	WriteI2C_Byte(0xc4,0x01);//Hactive[15:8] 
	WriteI2C_Byte(0xc5,0x90);//Hactive[7:0]
	WriteI2C_Byte(0xc6,0x06);//Vactive[15:8]  
	WriteI2C_Byte(0xc7,0x40);//Vactive[7:0]
	WriteI2C_Byte(0xc8,0x02);//Htotal[15:8]  
	WriteI2C_Byte(0xc9,0x12);//Htotal[7:0]  
	WriteI2C_Byte(0xca,0x06);//Vtotal[15:8] 
	WriteI2C_Byte(0xcb,0x90);//Vtotal[7:0] 
	WriteI2C_Byte(0xcc,0x00);//hs[15:8] 
	WriteI2C_Byte(0xcd,0x1e);//hs[7:0] 
	WriteI2C_Byte(0xce,0x14);//vs 
}

void Lt8918_TxDPhyConfig(void)
{
	SetRegisterBank(0x83); //MIPI TX DPHY Init
	/*
	WriteI2C_Byte(0x41,0x08);//DPHY_CK_POST
	WriteI2C_Byte(0x42,0x08);//DPHY_CK_TRAIL
	WriteI2C_Byte(0x43,0x08);//DPHY_CK_PRPR
	WriteI2C_Byte(0x44,0x08);//DPHY_CK_ZERO
	*/
	WriteI2C_Byte(0x46,0x04);//DPHY_LPX Unit tByteClk
	WriteI2C_Byte(0x47,0x08);//DPHY_HS_PRPR
	WriteI2C_Byte(0x48,0x0a);//DPHY_HS_TRAIL
	WriteI2C_Byte(0x4a,0x20);//this Value must >Reg0x46+Reg0x47
	WriteI2C_Byte(0x40,0x00);
}


void Lt8918_TxPrtclConfig(void)
{
	SetRegisterBank(0x83); //MIPI TX
	WriteI2C_Byte(0x12,0x00);//可以通过读取0x832d判断设置是否正确，配置正确bit[3；2]不会被置1
	WriteI2C_Byte(0x13,0xc8);//delay_cnt 1line=800 2line=600 3line=400 4line=200
	WriteI2C_Byte(0x14,0x03);//VSA
	WriteI2C_Byte(0x15,0x07);//VBP
	WriteI2C_Byte(0x16,0x06);//Vact_H  Vactive高8位
	WriteI2C_Byte(0x17,0x40);//Vact_L  Vactive低8位
	WriteI2C_Byte(0x18,0x83);//VFP
	WriteI2C_Byte(0x19,0x1e);//PKT_HSA
	WriteI2C_Byte(0x1a,0x1e);//PKT_HBP
	WriteI2C_Byte(0x1b,0x1e);//PKT_HFP
	WriteI2C_Byte(0x1c,0x01);//Hact_H	Hactive高8位
	WriteI2C_Byte(0x1d,0x90);//Hact_L	Hactive低8位
	WriteI2C_Byte(0x10,0x18);//video mode & clk set ,details please refer to reglist spec
	WriteI2C_Byte(0x1f,0x00);
	WriteI2C_Byte(0x11,0x0c);//lines & video moe,details please refer to reglist spec
}
void Lt8918_CSI_TxPrtclConfig(void)
{
	SetRegisterBank(0x80);
	WriteI2C_Byte(0x6A,0x0A);//RGB2YCbCr422

	SetRegisterBank(0x83); //MIPI TX
	WriteI2C_Byte(0x12,0x00);//可以通过读取0x832d判断设置是否正确，配置正确bit[3；2]不会被置1
	WriteI2C_Byte(0x13,0xc8);//delay_cnt 1line=800 2line=600 3line=400 4line=200
	WriteI2C_Byte(0x14,0x02);//csi=vs+vbp-1
	WriteI2C_Byte(0x15,0x04);//csi=0x01 vbp
	WriteI2C_Byte(0x16,0x07);//Vact_H  Vactive高8位
	WriteI2C_Byte(0x17,0x80);//Vact_L  Vactive低8位
	WriteI2C_Byte(0x18,0x04);//csi=0x01 Vfp
	WriteI2C_Byte(0x19,0x01);//Ptr_HBlank	csi=0x01
	WriteI2C_Byte(0x1a,0x01);//Ptr_HBlank	csi=0x01
	WriteI2C_Byte(0x1b,0x01);//Ptr_HBlank	csi=0x01
	WriteI2C_Byte(0x1c,0x04);//Hact_H	Hactive高8位
	WriteI2C_Byte(0x1d,0x38);//Hact_L	Hactive低8位
	WriteI2C_Byte(0x10,0x58);//video mode & clk set ,details please refer to reglist spec
	WriteI2C_Byte(0x1f,0x04);//YUV16
	WriteI2C_Byte(0x11,0x0c);//(1LANE=0X1C) (2LANE=0X2C) (3LANE=0X3C) (4LANE=0X0C)
	
}


/*
//以下是通过I2C_Debug直接操作，寄存器值请参考寄存器列表
//Video Check
40 ff 80 00
40 43 01 ff//vs=4=0x04         		reg=0x8043
40 44 02 ff//hs=8=0x0008	   		reg=0x8044\45
40 46 01 ff//vbp=16=0x10	   		reg=0x8046
40 47 01 ff//vfp=16=0x10	   		reg=0x8047
40 48 02 ff//hbp=32=0x0020	   		reg=0x8048/9
40 4a 02 ff//hfp=120=0x0078			reg=0x804a/b
40 4c 02 ff//Vtotal=1956=0x07a4		reg=0x804c/d
40 4e 02 ff//Htotal=1360=0x0550		reg=0x804e/f
40 50 02 ff//Vact=1920=0x0780		reg=0x8050/1
40 52 02 ff//Hact=1200=0x04b0		reg=0x8052/3
//读PixClk的值=0x0294de=169.18Mhz	
40 ff 80 00
40 33 0c 00							//5'h0c = Ad_pix_clk
40 30 03 ff							reg=0x8030/1/2
*/

void Lt8918_TxPrtclConfig_roll( void )
{
	u8	vsa	   = 0;
	u8	hsa	   = 0;
	u8	vbp	   = 0;
	u8	vfp	   = 0;
	u16	hbp	   = 0;
	u16	hfp	   = 0;
	u16	Vtotal = 0;
	u16	Htotal = 0;
	u16	Vact = 0;
	u16	Hact = 0;
	u32	PixClk = 0;
	char buf[3];
	int ret;
	
	SetRegisterBank(0x80 );//read the ttlrgb input vblank dynamic timing 
	vsa	= ReadI2C_Byte(0x43 );
	hsa	= ReadI2C_Byte(0x44 );
	vbp	= ReadI2C_Byte(0x46 );
	vfp	= ReadI2C_Byte(0x47 );
	
	XM_printf("vsa =%x\n",vsa);
	XM_printf("hsa =%x\n",hsa);
	XM_printf("vbp =%x\n",vbp);
	XM_printf("vfp =%x\n",vfp);

	ReadI2C_ByteN(0x48,buf,2);
	hbp = buf[0];
	hbp = (hbp<<8)|buf[1];
    XM_printf("hbp=%x\n",hbp);

	ReadI2C_ByteN(0x4a,buf,2);
	hfp = buf[0];
	hfp = (hfp<<8)|buf[1];
    XM_printf("hfp=%x\n",hfp);

	ReadI2C_ByteN(0x4c,buf,2);
	Vtotal = buf[0];
	Vtotal = (Vtotal<<8)|buf[1];
    XM_printf("Vtotal=%x\n",Vtotal);

	ReadI2C_ByteN(0x4e,buf,2);
	Htotal = buf[0];
	Htotal = (Htotal<<8)|buf[1];
    XM_printf("Htotal=%x\n",Htotal);

	ReadI2C_ByteN(0x50,buf,2);
	Vact = buf[0];
	Vact = (Vact<<8)|buf[1];
    XM_printf("Vact=%x\n",Vact);

	ReadI2C_ByteN(0x52,buf,2);
	Hact = buf[0];
	Hact = (Hact<<8)|buf[1];
    XM_printf("Hact=%x\n",Hact);

	WriteI2C_Byte(0x33, 0x0c );
	ReadI2C_ByteN(0x30,buf,3);
	PixClk = buf[0];
	PixClk = (PixClk<<8)|buf[1];
	PixClk = (PixClk<<8)|buf[2];
    XM_printf("PixClk=%x\n",PixClk);

	WriteI2C_Byte(0x33, 0x09 );
	ReadI2C_ByteN(0x30,buf,3);
	PixClk = buf[0];
	PixClk = (PixClk<<8)|buf[1];
	PixClk = (PixClk<<8)|buf[2];
    XM_printf("tx mipi PixClk=%x\n",PixClk);
	
	/*
	SetRegisterBank( 0x83 ); //MIPI TX	
	WriteI2C_Byte( 0x14, vsa );//vblank dynamic timing=vsa
	WriteI2C_Byte( 0x15, vbp );//vblank dynamic timing=vbp	
	WriteI2C_Byte( 0x18, vfp );//vblank dynamic timing=vfp
	*/

	SetRegisterBank(0x83); //MIPI TX	
	ret	= ReadI2C_Byte(0x2d);
	XM_printf("check ret =%x\n",ret);
}


void LT8918_ScreenInitial(void)
{
	//DCS Command Init
	SetRegisterBank(0x70);
	WriteI2C_Byte(0x23,0xa0);
	WriteI2C_Byte(0x29,0x81);
	WriteI2C_Byte(0x38,0x00);
	WriteI2C_Byte(0x34,0x49);
	WriteI2C_Byte(0x35,0x80);
	SetRegisterBank(0x80); 
	WriteI2C_Byte(0x11,0x00); 
		
	//DCS Command Set Start
	DcsShortPktWrite(0x05,0x11,0x00); //exit sleep mode	
	delay1ms(150);
	DcsShortPktWrite(0x05,0x29,0x00); //set display on
}


void Lt8918Reset()
{
	// cd0_pad/GPIO80
	unsigned int val;
	XM_lock();
	val = rSYS_PAD_CTRL08;
	val &= ~(0x03 << 0);
	rSYS_PAD_CTRL08 = val;
	SetGPIOPadDirection (GPIO80, euOutputPad);
	XM_unlock();
	
	XM_lock ();	
	SetGPIOPadData (GPIO80, euDataHigh);
	XM_unlock();	
	OS_Delay (100);
	XM_lock ();	
	SetGPIOPadData (GPIO80, euDataLow);
	XM_unlock();	
	OS_Delay (100);
	XM_lock ();	
	SetGPIOPadData (GPIO80, euDataHigh);
	XM_unlock();	
}


void InitLt8918State(void)
{
	char buf[2];
	unsigned int chipid;
	
    XM_printf("Init Lt8918\n");
	
	Lt8918Reset();
	
	Lt8918_CfgI2cEnable();//enable i2c config
	
	ReadI2C_ByteN(0x00,buf,2);
	chipid = buf[0];
	chipid = (chipid<<8)|buf[1];
    XM_printf("CHIP LT8918 ID =%x\n",chipid);
	
	LT8918_ScreenInitial();//MIPI Line0 DCS Init
	
	#if 1//TTL RGB Input
	//Lt8918_GpoConfig();//LT8918 GPIO Config
	Lt8918_MLRXInit();//config TTL input
	Lt8918_InputConfig();
	Lt8918_TxPllConfig();//refer to <LT8918_PLL_Setting.xls> sheet1: TXPLL Setting
	Lt8918_OutputConfig();	
	Lt8918_TxDPhyConfig();
	Lt8918_TxPrtclConfig();//MIPI  Protocol Config,for TTLinput the setting shoule be volatile
	#else//Pattern 
	Lt8918_DesscPllInit();
	Lt8918_DesscFreqSet();//refer to <LT8918_PLL_Setting.xls> sheet1: DeSSCPLL Setting
	Lt8918_TxPllConfig();//refer to <LT8918_PLL_Setting.xls> sheet1: TXPLL Setting
	Lt8918_OutputConfig();
	Lt8918_PtnDataConfig();//Pattern Timing Config
	Lt8918_TxDPhyConfig();
	Lt8918_TxPrtclConfig();//MIPI  Protocol Config,for Pattern the setting must be fixed
	while(1);//for pattern test
	#endif
}


#define VS_START       (21)
#define HS_START		(1)

#define HD				(400)
#define VD				(1600)

#define VBP		 		(7)
#define VFP	 	 		(110)
#define VPW		 		(3)
#define TV				(VS_START+VPW+VBP+VD+VFP)

#define HBP				(26)
#define HFP		 		(184)
#define HPW	       		(5)
#define TH				(HS_START+HPW+HBP+HD+HFP)


static void LCD_Init (void)
{
#define      lcd_enable             1
#define      screen_width           HD
#define      rgb_pad_mode           5
#define      direct_enable          0
#define      mem_lcd_enable         0 
#define      range_coeff_y          0
#define      range_coeff_uv         0
#define      lcd_done_intr_enable  1   //使能中断=1 禁止中断=0 lcd_done_intr
#define      dac_for_video          0
#define      dac_for_cvbs           0 

#define      lcd_interlace_flag     0
#define      screen_type            1 //pRGB_i   0  pRGB_p   1   sRGB_p   2  ITU656   3
#define      VSW1_enable            0 
#define      LcdVComp               0
#define      itu_pal_ntsc           0
#define      hsync_ivs              1
#define      vsync_ivs              1
#define      lcd_ac_ivs             0
#define      test_on_flag           0

//#define      back_color           (200<<16)|(100<<8)|(16)
#define      back_color             (0x22<<16)|(0x44<<8)|(0x88 )	// 无背景色
#define      cpu_screen_csn_bit     0
#define      cpu_screen_wen_bit     9

#define      DEN_h_rise             (HFP+HPW+HBP)   // 数据使能信号data_en水平起始位置
#define      DEN_h_fall             (HFP+HPW+HBP+HD) 
#define      DEN0_v_rise            (VFP+VPW+VBP)  // even field data enable é?éy????
#define      DEN0_v_fall            (VFP+VPW+VBP+VD)
#define      DEN1_v_rise            (VFP+VPW+VBP)
#define      DEN1_v_fall            (VFP+VPW+VBP+VD)


// 帧频 = LCD时钟/(CPL * LPS)	
// 每行时钟周期个数	
#define      CPL                      (TH)   // clock cycles per line  max 4096     ;  from register   timing2 16			

// 每屏行数量		
#define      LPS                      (TV)      // Lines per screen value  ;  from register   timing1 0

#define      HSW_rise                (HFP)
#define      HSW_fall                (HFP+HPW)

#define      VSW0_v_rise             (VFP) // Vertical sync width value    ;  from register   timing1 10
#define      VSW0_v_fall             (VFP+VPW) // Vertical sync width value     ;  from register   timing1 10

#define      VSW0_h_rise             (HFP)   
#define      VSW0_h_fall             (HFP+HPW) 

#define      VSW1_v_rise             (VFP)          // Vertical sync width value ;  from register   timing1 10
#define      VSW1_v_fall             (VFP+VPW)            // Vertical sync width value  ;  from register   timing1 10

#define      VSW1_h_rise             (HFP)
#define      VSW1_h_fall             (HFP+HPW)

#define      sav_pos                   288
#define      f1_vblk_start             624
#define      f1_vblk_end               23
#define      f2_vblk_start             311
#define      f2_vblk_end               336
#define      f2_start                  313
#define      f2_end                    625
#define      binary_thres              128
#define      binary_big                240
#define      binary_small              16

//#define      osd_0_global_coeff_enable      1
//#define      osd_0_enable                   1
#define      osd_0_global_coeff_enable      0
#define      osd_0_enable                   0

#define      osd_0_binary_enable            0
#define      osd_0_format                   0//0 : YUV420, 1 : ARGB8888, 2: RGB565, 3: RGB454
#define      osd_0_width                    0
#define      osd_0_height                   0
#define      osd_0_vc1_range_map            0
#define      osd_0_chroma_disable           0
#define      osd_0_h_position               0
#define      osd_0_v_position               0
#define      osd_0_global_coeff             0//设置透明度 0不透明 <~~~> 255 全透明
#define      osd_0_y_addr                   0x80000000//设置为全透明色之后，影响因子osd_0_mult_coef起作用？？
#define      osd_0_u_addr                   0x80000000
#define      osd_0_v_addr                   0x80000000
#define      osd_0_stride                   1280// 源  宽度
#define      osd_0_hoffset                  0
#define      osd_0_mult_coef                0

#define      osd_1_global_coeff_enable      0
#define      osd_1_enable                   0
#define      osd_1_binary_enable            0
#define      osd_1_format                   0//0 : YUV420, 1 : ARGB8888, 2: RGB565, 3: RGB454
#define      osd_1_width                    0
#define      osd_1_height                   0
#define      osd_1_vc1_range_map            0
#define      osd_1_chroma_disable           0
#define      osd_1_h_position               0
#define      osd_1_v_position               0
#define      osd_1_global_coeff             0 //设置透明度
#define      osd_1_y_addr                   0
#define      osd_1_u_addr                  0
#define      osd_1_v_addr                  0
#define      osd_1_stride                  0
#define      osd_1_hoffset                 0
#define      osd_1_mult_coef               0

#define      osd_2_global_coeff_enable      0
#define      osd_2_enable                   0
#define      osd_2_binary_enable            0
#define      osd_2_format                   0
#define      osd_2_width                    0
#define      osd_2_height                   0
#define      osd_2_vc1_range_map            0
#define      osd_2_chroma_disable           0
#define      osd_2_h_position               0
#define      osd_2_v_position               0
#define      osd_2_global_coeff             0 //设置透明度 0不透明 <~~~> 64 全透明
#define      osd_2_y_addr                   0
#define      osd_2_u_addr                  0
#define      osd_2_v_addr                  0
#define      osd_2_stride                  0
#define      osd_2_hoffset                 0
#define      osd_2_mult_coef               0
	unsigned int  lcd_param0, lcd_param1, lcd_param2, lcd_param3 ;
	unsigned int  lcd_param4, lcd_param5, lcd_param6, lcd_param7 ;
	unsigned int  lcd_param8, lcd_param9, lcd_param10, lcd_param11 ;
	unsigned int  lcd_param12, lcd_param13, lcd_param14, lcd_param15 ;
	unsigned int  lcd_param16, lcd_param17, lcd_param18, lcd_param19 ;
	unsigned int  lcd_param20, lcd_param21, lcd_param22  ;
	
	unsigned int  osd_0_param0, osd_0_param1, osd_0_param2, osd_0_param3 , osd_0_param4, osd_0_param5 ;
	unsigned int  osd_1_param0, osd_1_param1, osd_1_param2, osd_1_param3 , osd_1_param4, osd_1_param5 ;
	unsigned int  osd_2_param0, osd_2_param1, osd_2_param2, osd_2_param3 , osd_2_param4, osd_2_param5 ;
	unsigned int  osd_3_param0, osd_3_param1, osd_3_param2, osd_3_param3 , osd_3_param4, osd_3_param5 ;
	unsigned int  osd_4_param0, osd_4_param1, osd_4_param2, osd_4_param3 , osd_4_param4, osd_4_param5 ;
	unsigned int  osd_5_param0, osd_5_param1, osd_5_param2, osd_5_param3 , osd_5_param4, osd_5_param5 ;
	char  stop_lcd = 0 ;
   
   unsigned int addr,regno;
	
	LCD_PARAM0_reg  = (1<<17);
	LCD_PARAM0_reg  = 0;
	LCD_PARAM0_reg  = (1<<17);
	LCD_PARAM0_reg  = 0;//bit 26 enable lcd_done_intr 
	LCD_PARAM0_reg  = (0<<26);
	
	LCD_TVOUT_PARAM3_reg  = (1<<11)|(3<<12) ;
	lcd_param0  = 
		lcd_enable   |
		(screen_width          <<1)| 
		(rgb_pad_mode        <<13)| 
		(direct_enable          <<16)|  
						(mem_lcd_enable   <<17     )|
						(range_coeff_y  <<18    )|
						(range_coeff_uv          <<22    )|
						(lcd_done_intr_enable   <<26     )|
						(dac_for_video   <<27     )|
						(dac_for_cvbs   <<28     )|
						(1<<30)|
						(stop_lcd<<31)  ;
	
	lcd_param1  =
		( lcd_interlace_flag     <<0 ) |
		( screen_type     <<1) |
		( VSW1_enable     <<13) |
		( LcdVComp     <<14) |
						( itu_pal_ntsc     <<15) |
						( hsync_ivs     <<17) |
						( vsync_ivs     <<18) |
						( lcd_ac_ivs     <<19) |
						( 1  <<21 ) |
						( 1 <<23  ) |
						( test_on_flag     <<31)   ;
	
	lcd_param2  =
		( back_color     <<0 ) |
		( cpu_screen_csn_bit     <<24) |
		( cpu_screen_wen_bit     <<28);
   
	lcd_param3  =
		( DEN_h_rise      <<0  ) |  // data enable ?ú????é?éyμ?????
		( DEN_h_fall      <<12 )  ;
	
	lcd_param4  =
		( DEN0_v_rise     <<0  )|  // even field data enable é?éy????
		( DEN0_v_fall     <<12 ) ;
	
	lcd_param5  =
		( DEN1_v_rise     <<0  )|
		( DEN1_v_fall     <<12 ) ;
	
	lcd_param6  =
		(  CPL    <<0  )|               // clock cycles per line  max 4096                                 ;  from register   timing2 16
		(  LPS    <<12 ) ;              // Lines per screen value                                          ;  from register   timing1 0
	
	lcd_param7  =
		(  HSW_rise       <<0  )|
		(  HSW_fall       <<12 ) ;
	
	lcd_param8  =
		(  VSW0_v_rise    <<0  )|            // Vertical sync width value                                       ;  from register   timing1 10
		(  VSW0_v_fall    <<12 ) ;              // Vertical sync width value                                       ;  from register   timing1 10
	
	lcd_param9  =
		(  VSW0_h_rise   <<0  )|   
		(  VSW0_h_fall   <<12 ) ;   
	
	lcd_param10  =
		(  VSW1_v_rise   <<0  )|   
		(  VSW1_v_fall   <<12 ) ;   
	
	lcd_param11  =
		(  VSW1_h_rise    <<0  )|   
		(  VSW1_h_fall    <<12 ) ;   
	
	lcd_param12  = 66 | (129<<9) | (25<<18);
	
	lcd_param13  = 38 |  (74<<9) | (112<<18);
	
	lcd_param14  = 112 | (94<<9) | (18<<18); 
	
	lcd_param15  = 256 | (0<<9) | (394<<18);
	
	lcd_param16  = 256 |  (47<<9) | (118<<18);
	
	lcd_param17  = 256 | (465<<9) | (0<<19);
	
	//  lcd_param15  = 298 | (0<<9) | (409<<18);  
	
	//  lcd_param16  = 299 |  (100<<9) | (208<<18);
	
	//  lcd_param17  = 298 | (517<<9) | (0<<19); 
	
	lcd_param18  =
		(sav_pos    <<0     )   ;
	
	lcd_param19  =
		( f1_vblk_start    <<0  )|
		( f1_vblk_end    <<12 ) ;
	
	lcd_param20  =
		( f2_vblk_start        <<0  ) |
		( f2_vblk_end       <<12 )  ;
	lcd_param21  =
		( f2_start   <<0 )| 
		( f2_end   <<12);
	lcd_param22  =
		( binary_thres   <<16 )|
		( binary_big   <<8 )|
		( binary_small   <<0) ;
	
	LCD_PARAM1_reg     = lcd_param1  ;
	LCD_PARAM2_reg     = lcd_param2  ;
	LCD_PARAM3_reg     = lcd_param3  ;
	LCD_PARAM4_reg     = lcd_param4  ;
	LCD_PARAM5_reg     = lcd_param5  ;
	LCD_PARAM6_reg     = lcd_param6  ;
	LCD_PARAM7_reg     = lcd_param7  ;
	LCD_PARAM8_reg     = lcd_param8  ;
	LCD_PARAM9_reg     = lcd_param9  ;
	LCD_PARAM10_reg    = lcd_param10 ;
	LCD_PARAM11_reg    = lcd_param11 ;
	LCD_PARAM12_reg    = lcd_param12 ;
	LCD_PARAM13_reg    = lcd_param13 ;
	LCD_PARAM14_reg    = lcd_param14 ;
	LCD_PARAM15_reg    = lcd_param15 ;
	LCD_PARAM16_reg    = lcd_param16 ;
	LCD_PARAM17_reg    = lcd_param17 ;
	
	LCD_PARAM18_reg    = lcd_param18 ;
	LCD_PARAM19_reg    = lcd_param19 ;
	LCD_PARAM20_reg    = lcd_param20 ;
	LCD_PARAM21_reg    = lcd_param21 ;
	LCD_PARAM22_reg    = lcd_param22 ;
	//  lcd_param1 = LCD_PARAM1_reg      ;
	
   // addr,regno
   addr = LCD_OSD_BASE;
   for(regno = 0 ;regno<18 ; regno++ )
   {
      *(volatile unsigned int *)(addr)= 0;
      addr += 4;
   }
   
	LCD_PARAM0_reg     = lcd_param0  ;
	//offset :0x70190000 +0x214
	LCD_dithing_reg    = (1<<7) | (1<<15) | 5<<24;// 
}


static void LCD_SelPad(void)
{
	unsigned int val;
	OS_IncDI();    // Initially disable interrupts
	
	rSYS_PAD_CTRL05 = 1<<27|1<<24|1<<21|1<<18|1<<15|1<<12|1<<9|1<<6|1<<3|1<<0;
	rSYS_PAD_CTRL06 = 1<<27|1<<24|1<<21|1<<18|1<<15|1<<12|1<<9|1<<6|1<<3|1<<0;

    #if 0
	val = rSYS_PAD_CTRL07;
	// lcd_dout[20] lcd_dout[21] lcd_dout[22] lcd_dout[23]
	val &= 0xFF000000;
	val |= 1<<21|1<<18|1<<15|1<<12|1<<9|1<<3|1<<0;
    
	// 	[31]	lcd_d22, 0->lcd panel,输出port；1->cpu panel, 输入,cpu_scr_sel=1					
	val &= ~( (1 << 31) );
	
	rSYS_PAD_CTRL07 = val;
		#endif

	rSYS_SOFT_RSTNA &= ~(1<<17);
	__asm ("nop");
   __asm ("nop");
	rSYS_SOFT_RSTNA |= (1<<17);
   __asm ("nop");
   __asm ("nop");
   
	OS_DecRI();		// enable interrupt
}


static void LCD_ClockInit (void)
{
	unsigned int IntTvClkSwitch_sel;
	unsigned int Tvout_lcd_clk_wire_div;
	unsigned int Tvout_lcd_clk;
	unsigned int Tvout_lcd_pixel_clk;
	unsigned int Tvout_lcd_pixel_clk_wire_div;
	unsigned int TVOUT_LCD_OUT_DLY;
	
	// fPCLKSYS 8 30
	// PCLK = 240/9 = 26.7MHz
	// PCLK = 240/15 = 16MHz
	// fps = 16 / (528 * 874) = 34.7
	// fps = 12 / (528 * 874) = 26.0
	// fps = 10 / (528 * 874) = 21.7

	// 2-0	R/W	0x4	IntTvClkSwitch_sel：IntTvClkSwitch
	//							3'b000: clk_240m
	//							3'b001: syspll_clk 
	//							3'b010: audpll_clk
	//							3'b100: clk_24m
	IntTvClkSwitch_sel = 0;
	
	//	7-3	R/W	0x40	Tvout_lcd_clk_wire_div
	//	Tvout_lcd_clk  = IntTvClkSwitch / ( Tvout_lcd_pixel_clk _div ? Tvout_lcd_pixel_clk_div : 1 );
	

	//Tvout_lcd_clk_wire_div = 4;		// 60MHz
	Tvout_lcd_clk_wire_div = 5;		// 48MHz
	//Tvout_lcd_clk_wire_div = 6;		// 40MHz, 显示出现异常

	// 8	R/W	0x0	Tvout_lcd_clk  0 normal  1 inv
	Tvout_lcd_clk = 1;

	// 9	R/W	0x0	Tvout_lcd_pixel_clk 0 normal  1 inv
	Tvout_lcd_pixel_clk = 0;
	
	// 17-15	R/W	0x4	Tvout_lcd_pixel_clk_wire_div
	//							Tvout_lcd_pixel_clk_wire = tvout_lcd_clk / ( Tvout_lcd_pixel_clk_wire_div ? Tvout_lcd_pixel_clk_wire_div : 1 );
	Tvout_lcd_pixel_clk_wire_div = 1;
	
	// 24-18	R/W	0	TVOUT_LCD_OUT DLY
	TVOUT_LCD_OUT_DLY = 0;
	
	// 26.7MHz
	rSYS_LCD_CLK_CFG =	(TVOUT_LCD_OUT_DLY << 18)
							|	(Tvout_lcd_pixel_clk_wire_div << 15)
							|	(Tvout_lcd_pixel_clk << 9)
							|	(Tvout_lcd_clk << 8)
							|	(Tvout_lcd_clk_wire_div << 3)
							|	(IntTvClkSwitch_sel << 0)
							;

	Lcd_ClkEnable ();
}


static void LCD_ClockStop (void)
{
	Lcd_ClkDisable ();
}


void HW_LCD_BackLightOn (void)
{
 	// GPIO31/PWM1
	// pad_ctl3
	// [5:3]	itu_c_vsync_in	itu_c_vsync_in_pad	GPIO31	itu_c_vsync_in	mac_mdio	pwm_out[1]
	unsigned int val;
	
	XM_lock();
	val = rSYS_PAD_CTRL03;
	val &= ~(0x07 << 3);
	rSYS_PAD_CTRL03 = val;
	XM_unlock();
    
	XM_lock ();	
	SetGPIOPadDirection (GPIO31, euOutputPad);
	SetGPIOPadData (GPIO31, euDataHigh);
	XM_unlock();
}

// 关闭背光
void HW_LCD_BackLightOff (void)
{
 	// GPIO31/PWM1
	// pad_ctl3
	// [5:3]	itu_c_vsync_in	itu_c_vsync_in_pad	GPIO31	itu_c_vsync_in	mac_mdio	pwm_out[1]
	unsigned int val;
	
	XM_lock();
	val = rSYS_PAD_CTRL03;
	val &= ~(0x07 << 3);
	rSYS_PAD_CTRL03 = val;
	XM_unlock();

	XM_lock ();	
	SetGPIOPadDirection (GPIO31, euOutputPad);
	SetGPIOPadData (GPIO31, euDataLow);
	XM_unlock();
}


// RGB LCD初始化
void HW_LCD_Init (void)
{	
	LCD_SelPad();
	LCD_ClockInit ();
	GPIO_I2C_Init ();
	InitLt8918State();
	LCD_Init();
}


// RGB LCD关闭退出
void HW_LCD_Exit(void)
{
	int i;
	// Lcd_enable
	//	LCD使能信号，为1时LCD模块工作，为0时，LCD模块不工作。
	OS_EnterRegion ();	// 禁止任务切换
	
	// 等待帧刷新结束
	LCD_INTR_CLR_reg = 0xFFFFFFFF;
	while(!(LCD_STATUS_reg & 0x01));		// bit1: lcd_done_intr(每场结束中断) ,
	// 禁止LCDC
	LCD_PARAM0_reg &= ~1;
	
	OS_LeaveRegion ();	// 禁止任务切换
}


#endif

