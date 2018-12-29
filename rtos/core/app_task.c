#include <stdio.h>
#include "RTOS.h"		// OS头文件
#include "FS.h"
#include "hardware.h"
#include "xm_core.h"
#include <xm_user.h>
#include <xm_base.h>
#include <xm_dev.h>
#include <xm_rom.h>
#include <xm_dev.h>
#include "xm_malloc.h"
#include "xm_icon_manage.h"
#include "xm_voice_prompts.h"
#include "xm_videoitem.h"
#include "xm_h264_codec.h"
#include <xm_heap_malloc.h>
#include "rom.h"
#include "xm_h264_cache_file.h"
#include "xm_systemupdate.h"
#include "xm_flash_space_define.h"
#include "ssi.h"

extern void winmem_init (void);
extern void winmem_exit (void);

static OS_RSEMA iram_access_semaphore;		// 保护IRAM共享访问


static OS_TASK TCB_AppTask;
__no_init static OS_STACKPTR int StackAppTask[XMSYS_APP_TASK_STACK_SIZE/4];          /* Task stacks */

#ifndef _WINDOWS

#if ROM_SPI		// ROM保存在SPI Flash中，未加载到DDR
static DWORD  rom_offset;		// SPI中的偏移
static DWORD  rom_length;
XMBOOL XM_RomInit (void)
{
	XMBOOL ret = 0;
	XM_SYSTEMUPDATE_INFO *info = kernel_malloc (sizeof(XM_SYSTEMUPDATE_INFO));
	rom_offset = 0;
	rom_length = 0;
	do
	{
		if(info == NULL)
			break;
		if(Spi_Read (XM_FLASH_RTOS_BASE, (UINT8 *)info, MAX_SYSTEMUPDATE_HEADER_SIZE) == 0)	// io error
			break;
		if(info->id != XMSYS_SYSTEMUPDATE_ID)
			break;
		//if(info->version != XMSYS_SYSTEMUPDATE_VERSION_1_0)	// 1.0支持ROM.BIN
		//	break;
		if(info->rom_bin_info.id != XMSYS_ROMBIN_INFO_ID)
			break;
		rom_offset = info->rom_bin_info.binary_offset;
		rom_length = info->rom_bin_info.binary_length;
		ret = 1;
	} while(0);
	if(info)
		kernel_free (info);
	return ret;
}

XMBOOL XM_RomExit (void)
{
	rom_offset = 0;
	rom_length = 0;
	return 1;
}


// 根据ROM偏移获取ROM的句柄地址
HANDLE XM_RomAddress (DWORD dwOffset)
{
	if(rom_offset == 0)
		return NULL;
	if(rom_length == 0 || dwOffset >= rom_length)
		return NULL;
	return (HANDLE)(rom_offset + dwOffset + XM_FLASH_RTOS_BASE);
}

// 根据ROM偏移获取ROM的字节大小
DWORD XM_RomLength (DWORD dwOffset)
{
	unsigned int spi_offset;
	DWORD length;
	DWORD size;
	unsigned char data[4];
	if(rom_offset == 0)
		return 0;
	if(rom_length == 0 || dwOffset >= rom_length)
		return 0;
	if(dwOffset < 4)
		return 0;
	spi_offset = rom_offset + dwOffset + XM_FLASH_RTOS_BASE - 4;
	if(Spi_Read (spi_offset, (UINT8 *)data, 4) == 0)	// io error
		return 0;
	size = *data | (data[1] << 8) | (data[2] << 16) | (data[3] << 24);
	return size;
}
#else				// ROM保存在RAM (DDR)中

// 定义包含ROM数据的section
// 同时需要--image_input中定义_ROM_SECTION_
#pragma section="_ROM_SECTION_"

char *rom_buffer;
static DWORD  rom_length;
extern U32 _ROM_BIN_;
XMBOOL XM_RomInit (void)
{
	rom_buffer = (char *)&_ROM_BIN_;
	rom_length = __section_size("_ROM_SECTION_");
	//printf ("rom_buffer=%08x, rom_length=%d\n", rom_buffer, rom_length);
	return 1;
}

XMBOOL XM_RomExit (void)
{
	return 1;

}


// 根据ROM偏移获取ROM的句柄地址
HANDLE XM_RomAddress (DWORD dwOffset)
{
	if(dwOffset >= rom_length)
		return NULL;

	return (HANDLE)(rom_buffer + dwOffset);
}

// 根据ROM偏移获取ROM的字节大小
DWORD XM_RomLength (DWORD dwOffset)
{
	unsigned char *addr;
	DWORD size;
	if(dwOffset >= rom_length)
		return 0;
	if(dwOffset < 4)
		return 0;
	addr = (unsigned char *)(rom_buffer + dwOffset - 4);
	size = *addr | (addr[1] << 8) | (addr[2] << 16) | (addr[3] << 24);
	return size;
}
#endif	// #if ROM_SPI

VOID XM_idle (VOID)
{
	//printf ("XM_idle\n");
	OS_WaitEvent(	XMSYS_APP_KEYBD_EVENT
					|	XMSYS_APP_TIMER_EVENT
					);
}

VOID XM_wakeup (VOID)
{
	OS_SignalEvent(XMSYS_APP_KEYBD_EVENT|XMSYS_APP_TIMER_EVENT, &TCB_AppTask); /* 通知事件 */
}
#endif

void XMSYS_AppTask (void)
{
	//printf ("XMSYS_AppTask, %d\n", XM_GetTickCount());
	// 20180129 zhuoyonghong
	// 检查退出码
	int exit_code = XM_Main ();
	
	// 停止播放或录像过
	//XMSYS_H264CodecStop ();
	
	// 等待codec Cache刷新到文件系统
	//XMSYS_WaitingForCacheSystemFlush (10000);
	XM_printf ("WaitingForCacheSystemFlush forever\n");
	XMSYS_WaitingForCacheSystemFlush (0x1FFFFFFF);
	
	// 等待回收任务正常结束, 避免回收任务执行中系统突然退出导致的文件系统损害
	XM_printf ("WaitingForRecycleMonitor forever\n");
	unsigned int ticket_to_timeout = XM_GetTickCount() + 0x1FFFFFFF;
	while(XM_VideoItemCheckRecycleMonitorBusy())
	{
		if(ticket_to_timeout <= XM_GetTickCount())
		{
			break;
		}
	}
	XM_printf ("WaitingForRecycleMonitor end\n");
	
	xm_close_volume_service ("mmc:0:");
	
	// 20180129 zhuoyonghong
	// 根据退出码决定关机模式
	if(exit_code == XMEXIT_REBOOT)
	{
		XM_ShutDownSystem (SDS_REBOOT);
	}
	else
	{
		XM_ShutDownSystem (SDS_POWERACC);
	}
	// XM_ShutDownSystem (SDS_POWEROFF);
	// XM_ShutDownSystem (SDS_POWERACC);
}

// 用户任务初始化
void XMSYS_AppInit (void)
{
	// 20180227 
	// iram共享访问信号量
	OS_CREATERSEMA (&iram_access_semaphore);
	
	// 用户内存分配模块初始化
	// 20170308 将GUI堆内存分配移到此处
	winmem_init ();
	
	// 1 开启ROM
	XM_RomInit ();
	
	// 视频时间戳字符及用户标识初始化
	XM_VideoIconInit ();

	// 提示语音任务初始化
	XM_voice_prompts_init();
	
	// 系统调用应用初始化过程
	XM_AppInit ();
	
	// UI任务创建
	OS_CREATETASK(&TCB_AppTask, "AppTask", XMSYS_AppTask, XMSYS_APP_TASK_PRIORITY, StackAppTask);
}

void XMSYS_AppExit (void)
{
	// 系统调用应用退出过程
	XM_AppExit ();
	
	// 语音任务退出
	XM_voice_prompts_exit ();

	// 释放icon资源
	XM_VideoIconExit();
	
	// 关闭ROM
	XM_RomExit ();
	
	// GUI内存系统关闭
	// 20170308 将GUI堆内存分配移到此处
	winmem_exit ();
}

#ifndef _WINDOWS


void *	XM_heap_debug_malloc	(int size, char *file, int line)
{
	void *mem = XM_malloc (size);
#if HEAP_DEBUG
	OS_EnterRegion ();
	if(mem)
	{
		printf ("h_malloc  0x%08x size 0x%08x file=%s, line=%d\n", (unsigned int)(mem), size, file, line);
		//printf ("h_malloc  0x%08x size 0x%08x\n", (unsigned int)(mem), size);
	}
	OS_LeaveRegion ();
#endif
	return mem;
	//return OS_malloc (size);
	//return kernel_malloc (size);
}


// 释放用户RAM到堆
void XM_heap_debug_free	(void *mem, char *file, int line)
{
	//if(mem)
	//	kernel_free (mem);
#if HEAP_DEBUG
	OS_EnterRegion ();
	//printf ("h_free 0x%08x, file=%s, line=%d\n", (unsigned int)(mem), file, line);
	if(mem)
	{
		printf ("h_free 0x%08x\n", (unsigned int)(mem));
	}
	OS_LeaveRegion ();
#endif
	if(mem)
		XM_free (mem);
	//if(mem)
	//	OS_free (mem);
}

// 从堆中分配内存，并将分配的字节全部清0. 失败返回NULL
void *	XM_heap_debug_calloc	(int size, char *file, int line)
{
	char *mem = XM_heap_debug_malloc (size, file, line);
	if(mem)
	{
		memset (mem, 0, size);
	}
	return mem;
}
#endif


// 将LOGO解码并显示在OSD 0层(视频层)
// -1 解码失败
//  0 解码成功
int ShowLogo (UINT32 LogoAddr,UINT32 LogoSize)//eason 20170802
{
	xm_osd_framebuffer_t yuv_framebuffer;
	unsigned int w, h;
	unsigned int size;
	char *jpg = NULL;
	int ret = -1;
	u8_t *data[4];

	// 获取视频层的尺寸信息
	w = XM_lcdc_osd_get_width (XM_LCDC_CHANNEL_0, XM_OSD_LAYER_0);
	h = XM_lcdc_osd_get_height (XM_LCDC_CHANNEL_0, XM_OSD_LAYER_0);
	// 创建一个新的视频层的帧缓存句柄
	yuv_framebuffer = XM_osd_framebuffer_create (0,
						XM_OSD_LAYER_0,
						XM_OSD_LAYER_FORMAT_Y_UV420,
						w,
						h,
						NULL,
						0,
						0		// clear_framebuffer_context
						);
	if(yuv_framebuffer == NULL)
	{
		XM_printf ("AlbumView failed, XM_osd_framebuffer_create NG\n");
		return -1;
	}
    //if(ICON_Show)
    //    XM_osd_framebuffer_get_buffer (yuv_framebuffer, data);
    #if 1
	do
	{
		#if ROM_SPI
		unsigned int address;
		unsigned char *buffer;

		address = (unsigned int)XM_RomAddress (LogoAddr);
		if(address == NULL)
			return -1;

		buffer = (unsigned char *)XM_heap_malloc (LogoSize );
		if(buffer == NULL)
			return -1;

		if(Spi_Read ((unsigned int)address, buffer, LogoSize) == 0)
		{
			XM_heap_free (buffer);
			return -1;
		}

	    size = LogoSize;
        jpg=(HANDLE)(buffer);
		#else
		size = LogoSize;
        jpg=(HANDLE)(rom_buffer + LogoAddr);
		#endif


#ifdef LCD_ROTATE_90
		{
			unsigned char *rotate_buffer = kernel_malloc (w * h * 3/2);
			if(rotate_buffer)
			{
				ret = xm_jpeg_decode ((const char *)jpg, size, (unsigned char *)rotate_buffer, w, h);
				if(ret == 0)
				{
					u8_t *data[4];
					XM_osd_framebuffer_get_buffer (yuv_framebuffer, data);
					//rotate_degree90 (	rotate_buffer, data[0], w, h, h, w);
					xm_rotate_90_by_zoom_opt ((u8_t *)rotate_buffer,
						  (u8_t *)rotate_buffer + w*h,
						  w,	h,	w,
						  0, 0, w, h,
						  data[0],
						  data[1],
						  w, h,
						  0,1);
				}
				kernel_free (rotate_buffer);
			}
			else
			{
				ret = -1;
			}
		}
#else
		// 将JPEG码流解码到新创建的帧缓存
		//清缓存,无效数据
		dma_inv_range ((unsigned int)yuv_framebuffer->address,(unsigned int)yuv_framebuffer->address + w * h * 3/2);
		ret = xm_jpeg_decode ((const char *)jpg, size, (unsigned char *)yuv_framebuffer->address, w, h);
#endif
		//kernel_free (jpg);
		jpg = NULL;

		#if ROM_SPI
		XM_heap_free (buffer);
		#endif

		break;
	} while (jpg);
    #endif

    if(ret != 0)
	{
		// 解码失败
		// 清除framebuffer
		XM_osd_framebuffer_clear (yuv_framebuffer, 0, 255, 255, 255);
	}
	// 将framebuffer关闭, 将其更新为当前的视频缓冲, 即在OSD 0层显示
	XM_osd_framebuffer_close (yuv_framebuffer, 0);

	return ret;
}

int ShowLogo_Logo (UINT32 LogoAddr,UINT32 LogoSize,BOOL ICON_Show)//eason 20170802
{
	xm_osd_framebuffer_t yuv_framebuffer;
	unsigned int w, h;
	unsigned int size;
	char *jpg = NULL;
	int ret = -1;
	u8_t *data[4];

	// 获取视频层的尺寸信息
	w = XM_lcdc_osd_get_width (XM_LCDC_CHANNEL_0, XM_OSD_LAYER_0);
	h = XM_lcdc_osd_get_height (XM_LCDC_CHANNEL_0, XM_OSD_LAYER_0);
	// 创建一个新的视频层的帧缓存句柄
	yuv_framebuffer = XM_osd_framebuffer_create (0,
						XM_OSD_LAYER_0,
						XM_OSD_LAYER_FORMAT_Y_UV420,
						w,
						h,
						NULL,
						0,
						0		// clear_framebuffer_context
						);
	if(yuv_framebuffer == NULL)
	{
		XM_printf ("AlbumView failed, XM_osd_framebuffer_create NG\n");
		return -1;
	}
    //if(ICON_Show)
    //    XM_osd_framebuffer_get_buffer (yuv_framebuffer, data);
    #if 1
	do
	{
		#if ROM_SPI
		unsigned int address;
		unsigned char *buffer;

		address = (unsigned int)XM_RomAddress (LogoAddr);
		if(address == NULL)
			return -1;

		buffer = (unsigned char *)XM_heap_malloc (LogoSize );
		if(buffer == NULL)
			return -1;

		if(Spi_Read ((unsigned int)address, buffer, LogoSize) == 0)
		{
			XM_heap_free (buffer);
			return -1;
		}

	    size = LogoSize;
        jpg=(HANDLE)(buffer);
		#else
		size = LogoSize;
        jpg=(HANDLE)(rom_buffer + LogoAddr);
		#endif


#ifdef LCD_ROTATE_90
		{
			unsigned char *rotate_buffer = kernel_malloc (w * h * 3/2);
			if(rotate_buffer)
			{
				ret = xm_jpeg_decode ((const char *)jpg, size, (unsigned char *)rotate_buffer, w, h);
				if(ret == 0)
				{
					u8_t *data[4];
					XM_osd_framebuffer_get_buffer (yuv_framebuffer, data);
					//rotate_degree90 (	rotate_buffer, data[0], w, h, h, w);
					xm_rotate_90_by_zoom_opt ((u8_t *)rotate_buffer,
						  (u8_t *)rotate_buffer + w*h,
						  w,	h,	w,
						  0, 0, w, h,
						  data[0],
						  data[1],
						  w, h,
						  0,1);
				}
				kernel_free (rotate_buffer);
			}
			else
			{
				ret = -1;
			}
		}
#else
		// 将JPEG码流解码到新创建的帧缓存
		//清缓存,无效数据
		//dma_inv_range ((unsigned int)yuv_framebuffer->address,(unsigned int)yuv_framebuffer->address + w * h * 3/2);
		ret = xm_jpeg_decode ((const char *)jpg, size, (unsigned char *)yuv_framebuffer->address, w, h);
#endif
		//kernel_free (jpg);
		jpg = NULL;

		#if ROM_SPI
		XM_heap_free (buffer);
		#endif

		break;
	} while (jpg);
    #endif
    // 把ICON放在这里显示,有问题:
    // XM_osd_layer_init未初始化,osd_layer_format返回未0,uv数据不等,造成的结果是色彩层显示有问题
    // 解决方法是: 不要在这里添加ICON 显示,ICON显示必须放在(第一次调用)XM_osd_framebuffer_close 之后,
	if(0/*ICON_Show*/) {
        XM_OSD_FRAMEBUFFER framebuffer;
    	memset (&framebuffer, 0, sizeof(framebuffer));
    	framebuffer.width = 1024;
    	framebuffer.height = 600;
    	framebuffer.stride = 1024;
    	framebuffer.format = XM_OSD_LAYER_FORMAT_Y_UV420;
    	framebuffer.lcd_channel = 0;
    	framebuffer.osd_layer = XM_OSD_LAYER_0;
    	framebuffer.address = (unsigned char *)yuv_framebuffer->address;
    	//XM_IconDisplay (&framebuffer);
        //dma_flush_range( (unsigned int)yuv_framebuffer->address, yuv_framebuffer->width * yuv_framebuffer->height * 3/2 + (unsigned int)yuv_framebuffer->address);
        //dma_memcpy (data[0], (unsigned char *)yuv_framebuffer->address, w * h *3/2);
    }
    if(ret != 0)
	{
		// 解码失败
		// 清除framebuffer
		XM_osd_framebuffer_clear (yuv_framebuffer, 0, 255, 255, 255);
	}
	// 将framebuffer关闭, 将其更新为当前的视频缓冲, 即在OSD 0层显示
	XM_osd_framebuffer_close (yuv_framebuffer, 0);

	return ret;
}

// 请求片内IRAM共享资源访问权
void enter_region_to_access_iram (void)
{
	//OS_Use (&iram_access_semaphore);
}

// 释放片内IRAM共享资源访问权
void leave_region_to_access_iram (void)
{
	//OS_Unuse (&iram_access_semaphore);
}
