#include <xm_proj_define.h>
#include "hardware.h"
#include <string.h>
#include "common.h"
#include "sensorlib.h"
#include "SensorCommon.h"
#include "descript.h"
#include "descript_def.h"
#include "gpio_function.h"
#include "irqs.h"
#include <stdio.h>
#include "rtos.h"
#include "xm_printf.h"

extern void delay(unsigned int time);

extern STD_DEV_DSCR xdata gsStdDevDesc;


void camera_device_usb_init (void);

static BYTE _reg_brightness;
static BYTE _reg_contrast;
static BYTE _reg_hue;
static BYTE _reg_saturation;

static int usb_di_count;
// 关闭USB中断
void disable_usb_int(void)
{
	usb_di_count ++;
	if(usb_di_count > 1)
	{
		XM_printf ("usb_di_count=%d\n", usb_di_count);
	}
	OS_IncDI();
	irq_mask (USB_INT);
	OS_DecRI();
}

// 开启USB中断
void enable_usb_int(void)
{
	usb_di_count --;
	if(usb_di_count < 0)
	{
		XM_printf ("usb_di_count=%d\n", usb_di_count);
	}
	if(usb_di_count == 0)
	{
		OS_IncDI();
		irq_unmask (USB_INT);
		OS_DecRI();
	}
}



unsigned char reg_brightness_r(void)
{
	return _reg_brightness;
}

unsigned char reg_contrast_r (void)
{
	return _reg_contrast;
}

unsigned char reg_hue_r (void)
{
	return _reg_hue;
}

unsigned char reg_saturation_r (void)
{
	return _reg_saturation;
}

void reg_brightness_w(unsigned char v)
{
	_reg_brightness = v;
}

void reg_contrast_w(unsigned char v)
{
	_reg_contrast = v;
}

void reg_hue_w(unsigned char v)
{
	_reg_hue = v;
}

void reg_saturation_w(unsigned char v)
{
	_reg_saturation = v;
}


void camera_device_usb_init (void)
{
	uint16_t temp;

	// MGC_LinuxCd* pThis;
	// pThis = MGC_HdrcInitController((void *)USB_BASE);
	// MGC_HdrcStart(pThis);
	/* disable interrupts */
	MGC_Write8(MGC_O_HDRC_INTRUSBE, 0);
	MGC_Write16(MGC_O_HDRC_INTRTXE, 0);
	MGC_Write16(MGC_O_HDRC_INTRRXE, 0);
	/* off */
	MGC_Write8(MGC_O_HDRC_DEVCTL, 0);
	
	/*  flush pending interrupts */
	temp = MGC_Read8(MGC_O_HDRC_INTRUSB);
	temp = MGC_Read16(MGC_O_HDRC_INTRTX);
	temp = MGC_Read16(MGC_O_HDRC_INTRRX);
	
	MGC_Write8(MGC_O_HDRC_INTRUSBE, 0xff); /* don't enable suspend! */	

	/* TODO: always set ISOUPDATE in POWER (periph mode) and leave it on! */
	MGC_Write8(MGC_O_HDRC_TESTMODE, 0);

	/* enable high-speed/low-power and start session */
	MGC_Write8(MGC_O_HDRC_POWER,
	MGC_M_POWER_SOFTCONN | MGC_M_POWER_HSENAB | MGC_M_POWER_ISOUPDATE);

	temp = MGC_Read8(MGC_O_HDRC_POWER) ;
	temp = temp &(~0x40) ;
	MGC_Write8(MGC_O_HDRC_POWER,	temp );
	delay(100) ;
	
	MGC_Write8(MGC_O_HDRC_POWER,
	MGC_M_POWER_SOFTCONN | MGC_M_POWER_HSENAB | MGC_M_POWER_ISOUPDATE);

	/* enable high-speed/low-power and start session & suspend IM host*/
	MGC_Write8(MGC_O_HDRC_DEVCTL, MGC_M_DEVCTL_SESSION);	
	
	reg_usb_Index = 0x00;		
	temp = MGC_Read8(0x1F) ;
	//XM_printf ("ConfigData=%02x\n", temp);
}

extern void usb_dma_interrupt (void);


#include "cpuClkCounter.h"
#include "sys_soft_reset.h"
void pccamera_init (void)
{
	// USB PHY初始化
#if 1
	//sys_clk_disable (h2xdma_xclk_enable);
	sys_clk_disable (h2xusb_xclk_enable);
	//sys_clk_disable (h2xdma_hclk_enable);
	sys_clk_disable (h2xusb_hclk_enable);
	sys_clk_disable (usb_hclk_enable);
	sys_clk_disable (USB_PHY_clk_enable);
	sys_clk_disable (usb_12m_enable);
	
	sys_soft_reset (softreset_usbphy);
	sys_soft_reset (softreset_usb);
	sys_soft_reset (softreset_h2xusb);
	//sys_soft_reset (softreset_h2xdma);
	
	sys_clk_enable (usb_12m_enable);
	sys_clk_enable (USB_PHY_clk_enable);
	sys_clk_enable (h2xusb_xclk_enable);
	//sys_clk_enable (h2xdma_hclk_enable);
	sys_clk_enable (h2xusb_hclk_enable);
	sys_clk_enable (usb_hclk_enable);
	
#endif
	
	camera_device_usb_init ();

	device_init();
	
	
	sensor_lib_init();


#if MASK_VER_2
	pHs_AltSet_addr = Hs_AltSet;
	pFs_AltSet_addr = Fs_AltSet;
#endif

	initSetFlicker();	//初始化DSP FILICKER处理

	(*pFunc_AudioInit)();	//前面已留AUDIO参数外挂，此处再添加函数外挂以防万一

	update_video_cur_attribute();	//更新当前属性值，应放在SENSOR初始化之后进行

	set_dsp_default();		//将默认的属性值写入寄存器中
	
	request_irq(USB_INT, 15, usb_interrupt_rom);
	
	request_irq(USBDMA_INT, 15, usb_dma_interrupt);


	// USB Connect
	reg_usb_Power |= bmBIT6;
}

void pccamera_process (void)
{
	BYTE idata bIdxTemp;
	if (gsEp0Status.bState != EP0_IDLE)
	{
		disable_usb_int();		
		bIdxTemp = reg_usb_Index;
		reg_usb_Index = 0;
	
		if (gsEp0Status.bState == EP0_CMD)
		{
			usb_proc_cmd ();
		}

		if (gsEp0Status.bState == EP0_TX)
		{
			usb_ep0_transfer();	//结束状态: Tx  或Idle
		}

		reg_usb_Index = bIdxTemp;
		enable_usb_int();		
	}
}
