#include <stdio.h>
#include <stdlib.h>
#include "RTOS.h"		// OS头文件
#include "hardware.h"
#include "xm_flash_space_define.h"
#include "xm_core.h"
#include "sdmmc.h"
#include <xm_dev.h>
#include "fs.h"
#include <xm_videoitem.h>
#include "arkn141_scale.h"
#include "arkn141_isp.h"
#include "arkn141_codec.h"
#include "xm_itu656_in.h"
#include "ark1960_testcase.h"
#include "xm_isp_scalar.h"
#include "xm_key.h"
#include "xm_mci.h"
#include "ssi.h"
#include "xm_systemupdate.h"
#include "xm_h264_cache_file.h"
#include "xm_message_socket.h"
#include "xm_uvc_socket.h"
#include "CdrRequest.h"
#include "xm_h264_codec.h"
#include "xm_printf.h"
#include "xm_led.h"
#include "watchdog.h"
#include "hw_osd_layer.h"
#include "xm_file_manager_task.h"

typedef	unsigned int uint32_t;

extern void XMSYS_AesInit(void);
extern uint8_t MUSB_InitSystem(uint32_t dwBsrPriority);
extern void uvc_init(void);
extern void usb_mem_init (void);
extern void XMSYS_RecycleTaskInit(void);

// 主任务
OS_STACKPTR int StackMain[XMSYS_MAIN_TASK_STACK_SIZE/4];          /* Task stacks */
OS_TASK TCBMAIN;                        /* Task-control-blocks */


#pragma data_alignment=4096
__no_init static unsigned int _FileSystemCache[_XMSYS_FS_CACHE_SIZE_/4];


static void JLink_System_Update (void)
{
	XM_printf(">>>>>JLink_System_Update....\r\n");
#ifdef JLINK_DISABLE	

#else	
	char *cmp = NULL;
	char *mem;	// = (char *)0x80000000;
	// 检查SRAM中的JLink系统升级标识
	// "SPIWRITE" (0x53 0x50 0x49 0x57 0x52 0x49 0x54 0x45)
	if( *(u32_t *)0x0300008 == 0x53504957 && *(u32_t *)0x030000C == 0x52495445 )
	{
		printf ("JLink_System_Update\n");
		*(u32_t *)0x0300008 = 0;
		*(u32_t *)0x030000C = 0;
		// write into SPI-Flash
		// NandFlash引导方式下, 读写SPI-Flash前需要对NandFlash复位(禁止其功能), 并将Pin脚设置为IO或SPI功能. 这样SPI才能正确访问
		//SpiSelectPad();
		mem = kernel_malloc (XM_FLASH_RTOS_SIZE);
		if(mem)
		{
			Spi_Read (XM_FLASH_RTOS_BASE, (UINT8 *)mem, 1024); 	// 1024 header
			memcpy (mem + 1024, (char *)0x80000000, XM_FLASH_RTOS_SIZE - 1024);
		}

		printf ("Write spi ...\n");
		if(mem)
			Spi_Write (XM_FLASH_RTOS_BASE, (UINT8 *)mem, XM_FLASH_RTOS_SIZE);
		printf ("Write spi finish ...\n");
		
		cmp = kernel_malloc (XM_FLASH_RTOS_SIZE);
		if(cmp)
		{
			memset (cmp, 0, XM_FLASH_RTOS_SIZE);
			dma_clean_range ((unsigned int)cmp, XM_FLASH_RTOS_SIZE + (unsigned int)cmp);
			Spi_Read (XM_FLASH_RTOS_BASE, (UINT8 *)cmp, XM_FLASH_RTOS_SIZE);
			if(memcmp (cmp, mem, XM_FLASH_RTOS_SIZE) == 0)
			{
				printf ("SPI Burn OK\n");
			}
			else
			{
				printf ("SPI Burn NG\n");
			}
			kernel_free (cmp);
		}
		if(mem)
			kernel_free (mem);
	}
#endif
}

// 初始化系统基本服务
static void InitSystemBasicService (void)
{
	//printf ("InitSystemBasicService Init\n");
	
	// 文件系统及Cache
	//printf ("FS_Init\n");
	FS_Init ();
	FS_AssignCache ("", _FileSystemCache, sizeof(_FileSystemCache), FS_CACHE_RW);
	//FS_CACHE_SetMode ("", FS_SECTOR_TYPE_MASK_DIR|FS_SECTOR_TYPE_MASK_MAN, FS_CACHE_MODE_WT);
	FS_CACHE_SetMode("", FS_SECTOR_TYPE_MASK_DIR|FS_SECTOR_TYPE_MASK_MAN, FS_CACHE_MODE_WB);
	// 加入长文件名支持
	FS_FAT_SupportLFN ();
		
	// USB Host
#ifdef _XMSYS_FS_UDISK_SUPPORT_	
//	MUSB_InitSystem(100);
#endif
	
#ifdef _XMSYS_FS_SDMMC_SUPPORT_	
//	SDCard_Module_CardCheck ();
#endif

	
	//printf ("InitSystemBasicService Success\n");
}

// 产品板禁止jlink pin
static void jlink_disable (void)
{
#ifdef JLINK_DISABLE	

	XM_printf(">>>>>>>>>>>>>>>>>>jlink_disable define.....\r\n");

	//设置jlink作为IO口,
	unsigned int val;
	// pad_ctl9
	// [21:20]	uartrxd1	uartrxd1_pad	GPIO110	uart1_UARTRXD/ssp1_clk	tck
	// [23:22]	uarttxd1	uarttxd1_pad	GPIO111	uart1_UARTTXD/ssp1_cs	tms
	// [25:24]	uartrxd2	uartrxd2_pad	GPIO2	uart2_UARTRXD/ssp1_rxd	tdi
	// [27:26]	uarttxd2	uarttxd2_pad	GPIO3	uart2_UARTTXD/ssp1_txd	tdo
	val = rSYS_PAD_CTRL09 ;
	val &= ~((3 << 20) | (3 << 22) | (3 << 24) | (3 << 26));
	rSYS_PAD_CTRL09 = val;	
	
	// pad_ctla
	// [5:4]	gpio0	gpio0_pad	GPIO0	sen_clk_out	in_ntrst
	val = rSYS_PAD_CTRL0A ;
	val &= ~(3 << 4);
	rSYS_PAD_CTRL0A = val;	

	//设置这两个脚为串口
	// pad_ctl9
	// [25:24]	uartrxd2	uartrxd2_pad	GPIO2	uart2_UARTRXD/ssp1_rxd	tdi
	// [27:26]	uarttxd2	uarttxd2_pad	GPIO3	uart2_UARTTXD/ssp1_txd	tdo
	val = rSYS_PAD_CTRL09 ;
	val &= ~((3 << 24) | (3 << 26));
	val |= ((1 << 24) | (1 << 26));
	rSYS_PAD_CTRL09 = val;	
	
#else
	XM_printf(">>>>>>>>>>>>>>>>>>jlink_disable no define.....\r\n");

	//定义JLINK pin
	unsigned int val;
	// pad_ctl9
	// [21:20]	uartrxd1	uartrxd1_pad	GPIO110	uart1_UARTRXD/ssp1_clk	tck
	// [23:22]	uarttxd1	uarttxd1_pad	GPIO111	uart1_UARTTXD/ssp1_cs	tms
	// [25:24]	uartrxd2	uartrxd2_pad	GPIO2	uart2_UARTRXD/ssp1_rxd	tdi
	// [27:26]	uarttxd2	uarttxd2_pad	GPIO3	uart2_UARTTXD/ssp1_txd	tdo
	
	val = rSYS_PAD_CTRL09 ;
	val |= ((2 << 20) | (2 << 22) | (2 << 24) | (2 << 26));
	rSYS_PAD_CTRL09 = val;	

	// pad_ctla
	// [5:4]	gpio0	gpio0_pad	GPIO0	sen_clk_out	in_ntrst
	val = rSYS_PAD_CTRL0A ;
	val |= (2 << 4);
	rSYS_PAD_CTRL0A = val;
#endif

}

// 获取电池的当前电压(毫伏)
unsigned int XM_GetBatteryVoltage (void);

#include "gpio.h"


//#define	RTC_WAKEUP_TEST

static void MainTask(void)
{
	// 检查JLink系统升级标志是否存在. 若存在, 将ROM写入到SPI或NFC
	JLink_System_Update ();
	
#if  defined(_XMSYS_FS_SDMMC_SUPPORT_) || defined(_XMSYS_FS_NANDFLASH_SUPPORT_)
	// SD卡文件系统初始化
	//InitSystemBasicService ();	
	
	// USB Host
#ifdef _XMSYS_FS_UDISK_SUPPORT_	
	MUSB_InitSystem(100);
#endif
	
#ifdef _XMSYS_FS_SDMMC_SUPPORT_	
	SDCard_Module_CardCheck ();
#endif
	
#endif

#if HDMI_720P
	hdmi_main(); 
#endif
	
	//fs_device_cache_file_test ("mmc", 0);while(1);
	//fs_device_cache_file_test ("usb", 0); while(1);
	
	
	/*
	while(1)
	{
		SetGPIOPadData(33, euDataHigh);
		//OS_Delay (1);
		delay (10000);
		SetGPIOPadData(33, euDataLow);
		//OS_Delay (1);
		delay (10000);
		
	}*/
	
	//TestAlarm (); while(1);
	
	
	//void arkn141_jpeg_decode_test (void);
	//arkn141_jpeg_decode_test();

	//pccamera_init (); while(1);

	//TestRTC (); while(1);		// RTC测试
	
	//printf ("ISP TEST BY RAW DATA\n");
	
	//xm_i2c_test (); while(1);
	
	//xm_uart_test (); while(1);
	
	//wdt_test (); while(1);
	
	//adc_dac_test (); while(1);
	
	//while(1);
	//xm_timer_test(); while(1);
	//fs_sd_sector_read_test (0);
	//fs_sd1_sector_read_test (0);		// SD Card 1已测试, OK
	
	//h264codec_RecorderStart ();
	
	//XMSYS_H264CodecRecorderStart ();
	/*
	unsigned int avi_video_format = XMSYS_H264CodecGetVideoFormat();
	if(avi_video_format == ARKN141_VIDEO_FORMAT_1080P_30)
	{
		isp_set_video_width (1920);
		isp_set_video_height (1080);
		isp_set_sensor_bit (ARKN141_ISP_SENSOR_BIT_12);
		XMSYS_H264CodecSetFrameRate (30);
	}
	else if(avi_video_format == ARKN141_VIDEO_FORMAT_720P_30)
	{
		isp_set_video_width (1280);
		isp_set_video_height (720);
		isp_set_sensor_bit (ARKN141_ISP_SENSOR_BIT_10);
		XMSYS_H264CodecSetFrameRate (30);
	}
	else if(avi_video_format == ARKN141_VIDEO_FORMAT_720P_60)
	{
		isp_set_video_width (1280);
		isp_set_video_height (720);
		isp_set_sensor_bit (ARKN141_ISP_SENSOR_BIT_10);
		XMSYS_H264CodecSetFrameRate (60);
	}
	else
	{
		isp_set_video_width (1920);
		isp_set_video_height (1080);
		isp_set_sensor_bit (ARKN141_ISP_SENSOR_BIT_12);		
		XMSYS_H264CodecSetFrameRate (30);
	}*/
	
	//XMSYS_SensorStart();
	
	// 开启watchdog, 保护isp启动过程
	hw_watchdog_init (2.0);
	
	OS_Delay (400);
	
#if CZS_USB_01
#else
	//LCD_set_enable (1);
#endif
    if(0/*!Get_USB_CONNECT_COMPUTER()*/) 
    {
        //save_DatafromSPI();
        uvc_init();
    	//set_default_video_resolution();
    	//set_default_lcd_display_resolution();
    	usb_mem_init();	//usb malloc && free 使用私有的堆空间。
    	MUSB_InitSystem(254);
    }

#ifdef RTC_WAKEUP_TEST
	// RTC唤醒测试
	OS_Delay (60*2*1000);		// 2分钟
	SetAutoPowerUpTime (3*60);		// 3分钟
	//XM_KeyEventProc (VK_POWER, XMKEY_PRESSED);
	XM_ShutDownSystem (SDS_POWEROFF);
#endif

	// 周期性设置并清除watchdog, 保护异常重新启动
	while(1)
	{
		/*
		{
			if(XM_GetFmlDeviceCap (DEVCAP_VIDEO_REC) == DEVCAP_VIDEO_REC_START)
			{
				// 录像状态下每隔1秒交替 显示 "录像中" / 隐藏 "录像中"
				xm_led_on(XM_LED_1,ON);
				hw_watchdog_init (2.0);
				OS_Delay (500);
				xm_led_on(XM_LED_1,OFF);
				hw_watchdog_init (2.0);
				OS_Delay (500);
			}
			else
			{
				xm_led_on(XM_LED_1,ON);
				hw_watchdog_init (2.0);
				OS_Delay (100);
			}
		}
		*/
		
		hw_watchdog_init (2.0);
		OS_Delay (100);
		
		/*
		//xm_led_on (XM_LED_1, 1);	// on
		OS_Delay (100);
		//xm_led_on (XM_LED_1, 0);	// off
		//OS_Delay (100);
		hw_watchdog_init (2.0);
		
		//printf ("battery voltage = %d\n",  XM_GetBatteryVoltage ());
		*/
	}
}


static const char* video_path[] = {
#ifdef _WINDOWS
	"G:\\VIDEO_F\\",
	"G:\\VIDEO_B\\"
	"G:\\VIDEO_L\\"
	"G:\\VIDEO_R\\"
#else
	"\\VIDEO_F\\",
	"\\VIDEO_B\\"
	"\\VIDEO_L\\"
	"\\VIDEO_R\\"
#endif
};
#if CZS_USB_01

#else

//#define	_XMSYS_UART_SOCKET_SUPPORT_
//#define	_XMSYS_EDOG_SOCKET_SUPPORT_

#endif
void main(void)
{		
	OS_IncDI();                      /* Initially disable interrupts*/
	
	OS_InitKern();                   /* initialize OS */
	
	// thread safe c system library
	OS_INIT_SYS_LOCKS();   
	
	// 初始化系统时钟、定时器、COM口
	//printf ("OS_InitHW\r\n");
	OS_InitHW();                     /* initialize Hardware for OS    */
	//BSP_Init();                      /* 初始化产品相关的硬件          */
	
	hw_watchdog_init (3.0);

	jlink_disable ();
	PWM_Init(); //PWM 初始化-#20180912# Edison add 

	//XMSYS_AesInit();
	
	// printf ("XM_WaveInit\n");
	XM_WaveInit ();
	
	//printf ("XMSYS_MessageInit\n");
	// 创建系统消息任务
	XMSYS_MessageInit ();
	
	//printf ("InitSystemBasicService\n");
	InitSystemBasicService ();
	
	//printf ("xm_isp_init\n");
	xm_isp_init ();
	
#ifdef _XMSYS_ARK7116_SUPPORT_
	XMSYS_Ark7116Init ();
#endif	

#ifdef _XMSYS_NVP6134C_SUPPORT_
	XMSYS_Nvp6134cInit ();
#endif	

#ifdef _XMSYS_NVP6124B_SUPPORT_
	//XMSYS_Nvp6124bInit ();
#endif	

#ifdef _XMSYS_PR2000_SUPPORT_
	XMSYS_Pr2000Init();
#endif	

#ifdef _XMSYS_PR2000_SUPPORT_2_
	XMSYS_Pr2000Init_2();
#endif

#ifdef _XMSYS_RXCHIP_SUPPORT_
	XMSYS_RxchipInit();
#endif

	xm_itu656_in_init ();

	XMSYS_Itu601ScalerInit();

	// 视频项服务初始化
#if _XM_PROJ_ == _XM_PROJ_2_SENSOR_1080P_CVBS
	XM_VideoItemInit (2, video_path);
#else
	XM_VideoItemInit (1, video_path);
#endif
	
	printf ("XMSYS_SystemUpdateInit\n");
	XMSYS_SystemUpdateInit ();
	
	arkn141_scale_init ();
	
	printf ("XMSYS_CameraInit\n");
	XMSYS_CdrInit ();			
	XMSYS_UvcSocketInit ();
	XMSYS_CameraInit ();		// PC Camera任务初始化
	
	//printf ("XMSYS_H264FileInit\n");
	XMSYS_H264FileInit ();
	
	//printf ("XMSYS_SensorInit\n");
	XMSYS_SensorInit ();		// Sensor 任务初始化
	
	//printf ("XMSYS_JpegInit\n");
	XMSYS_JpegInit ();
	
	// 文件管理器任务初始化
	XMSYS_FileManagerInit ();	

	//printf ("xm_isp_scalar_init\n");
	xm_isp_scalar_init();	// ISP scalar
	
	//printf ("XMSYS_VideoInit\n");
	XMSYS_VideoInit ();

	XMSYS_AppInit ();
	MotionTaskInit();
	
	// HW codec initializing
	//printf ("arkn141_codec_init\n");
	arkn141_codec_init();
	XMSYS_H264CodecInit();//H.264编解码任务初始化

#ifdef _XMSYS_FS_SDMMC_SUPPORT_		// SDMMC卡文件系统支持
#if _XMSYS_FS_SDMMC_TYPE_	== _XMSYS_FS_SDMMC_TYPE_ARKMICRO_
	SDCard_Module_Init();
#endif
	
#endif

    RemoteKeyInit();

	//printf ("XMSYS_OneKeyPhotographInit\n");
	// 一键拍照服务初始化
	XMSYS_OneKeyPhotographInit();
	
	
#ifdef _XMSYS_UART_SOCKET_SUPPORT_
	printf(">>>>XMSYS_MessageSocketInit\r\n");
	// 消息管道服务初始化
	XMSYS_MessageSocketInit ();
#endif
	
#ifdef _XMSYS_EDOG_SOCKET_SUPPORT_
	// 电子狗支持
	void XMSYS_EdogSocketInit (void);
	XMSYS_EdogSocketInit ();
#endif
	
	//printf ("XMSYS_CameraInit\n");
	//XMSYS_CameraInit ();		// PC Camera任务初始化
	
	XMSYS_RecycleTaskInit ();

    XMSYS_DetEventInit();
    
    //XMSYS_AutoTestInit();//关掉自动测试功能
    
	/* You need to create at least one task before calling OS_Start() */
	//OS_CREATETASK(&TCBMAIN, "Main", MainTask, 250, StackMain);
	OS_CREATETASK(&TCBMAIN, "Main", MainTask, 254, StackMain);
	//XMSYS_ACC_Init();//ACC检测
	trig_detect_init();
	printf ("OS_Start\n");
	OS_Start();                      /* Start multitasking            */
}

size_t __write(int handle, const unsigned char *buf, size_t bufSize)
{
	size_t nChars = 0;
	//return 0;
	/* Check for the command to flush all handles */
	if (handle == -1)
	{
		return 0;
	}
	/* Check for stdout and stderr
	(only necessary if FILE descriptors are enabled.) */
	if (handle != 1 && handle != 2)
	{
		return -1;
	}
#if 1
	for (/* Empty */; bufSize > 0; --bufSize)
	{
		uart0_send_char (*buf);
		++buf;
		++nChars;
	}
#else
   putUART0SendStr(buf, nChars = bufSize );//使用中断 调用uart0 传送数据 
#endif
	return nChars;
}

size_t __read(int handle, unsigned char *buf, size_t bufSize)
{
	size_t nChars = 0;
	/* Check for stdin (only necessary if FILE descriptors are enabled) */
	if (handle != 0)
	{
		return -1;
	}
  
	for (/*Empty*/; bufSize > 0; --bufSize)
	{
		unsigned char c = (unsigned char)uart0_getc();
		if(c == '\r' || c == '\n')
			break;
		if (c == 0)
			break;
		uart0_send_char (c);
		*buf++ = c;
		++nChars;
	}
	return nChars;
}
