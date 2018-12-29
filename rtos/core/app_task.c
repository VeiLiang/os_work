#include <stdio.h>
#include "RTOS.h"		// OSͷ�ļ�
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

static OS_RSEMA iram_access_semaphore;		// ����IRAM�������


static OS_TASK TCB_AppTask;
__no_init static OS_STACKPTR int StackAppTask[XMSYS_APP_TASK_STACK_SIZE/4];          /* Task stacks */

#ifndef _WINDOWS

#if ROM_SPI		// ROM������SPI Flash�У�δ���ص�DDR
static DWORD  rom_offset;		// SPI�е�ƫ��
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
		//if(info->version != XMSYS_SYSTEMUPDATE_VERSION_1_0)	// 1.0֧��ROM.BIN
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


// ����ROMƫ�ƻ�ȡROM�ľ����ַ
HANDLE XM_RomAddress (DWORD dwOffset)
{
	if(rom_offset == 0)
		return NULL;
	if(rom_length == 0 || dwOffset >= rom_length)
		return NULL;
	return (HANDLE)(rom_offset + dwOffset + XM_FLASH_RTOS_BASE);
}

// ����ROMƫ�ƻ�ȡROM���ֽڴ�С
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
#else				// ROM������RAM (DDR)��

// �������ROM���ݵ�section
// ͬʱ��Ҫ--image_input�ж���_ROM_SECTION_
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


// ����ROMƫ�ƻ�ȡROM�ľ����ַ
HANDLE XM_RomAddress (DWORD dwOffset)
{
	if(dwOffset >= rom_length)
		return NULL;

	return (HANDLE)(rom_buffer + dwOffset);
}

// ����ROMƫ�ƻ�ȡROM���ֽڴ�С
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
	OS_SignalEvent(XMSYS_APP_KEYBD_EVENT|XMSYS_APP_TIMER_EVENT, &TCB_AppTask); /* ֪ͨ�¼� */
}
#endif

void XMSYS_AppTask (void)
{
	//printf ("XMSYS_AppTask, %d\n", XM_GetTickCount());
	// 20180129 zhuoyonghong
	// ����˳���
	int exit_code = XM_Main ();
	
	// ֹͣ���Ż�¼���
	//XMSYS_H264CodecStop ();
	
	// �ȴ�codec Cacheˢ�µ��ļ�ϵͳ
	//XMSYS_WaitingForCacheSystemFlush (10000);
	XM_printf ("WaitingForCacheSystemFlush forever\n");
	XMSYS_WaitingForCacheSystemFlush (0x1FFFFFFF);
	
	// �ȴ�����������������, �����������ִ����ϵͳͻȻ�˳����µ��ļ�ϵͳ��
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
	// �����˳�������ػ�ģʽ
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

// �û������ʼ��
void XMSYS_AppInit (void)
{
	// 20180227 
	// iram��������ź���
	OS_CREATERSEMA (&iram_access_semaphore);
	
	// �û��ڴ����ģ���ʼ��
	// 20170308 ��GUI���ڴ�����Ƶ��˴�
	winmem_init ();
	
	// 1 ����ROM
	XM_RomInit ();
	
	// ��Ƶʱ����ַ����û���ʶ��ʼ��
	XM_VideoIconInit ();

	// ��ʾ���������ʼ��
	XM_voice_prompts_init();
	
	// ϵͳ����Ӧ�ó�ʼ������
	XM_AppInit ();
	
	// UI���񴴽�
	OS_CREATETASK(&TCB_AppTask, "AppTask", XMSYS_AppTask, XMSYS_APP_TASK_PRIORITY, StackAppTask);
}

void XMSYS_AppExit (void)
{
	// ϵͳ����Ӧ���˳�����
	XM_AppExit ();
	
	// ���������˳�
	XM_voice_prompts_exit ();

	// �ͷ�icon��Դ
	XM_VideoIconExit();
	
	// �ر�ROM
	XM_RomExit ();
	
	// GUI�ڴ�ϵͳ�ر�
	// 20170308 ��GUI���ڴ�����Ƶ��˴�
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


// �ͷ��û�RAM����
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

// �Ӷ��з����ڴ棬����������ֽ�ȫ����0. ʧ�ܷ���NULL
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


// ��LOGO���벢��ʾ��OSD 0��(��Ƶ��)
// -1 ����ʧ��
//  0 ����ɹ�
int ShowLogo (UINT32 LogoAddr,UINT32 LogoSize)//eason 20170802
{
	xm_osd_framebuffer_t yuv_framebuffer;
	unsigned int w, h;
	unsigned int size;
	char *jpg = NULL;
	int ret = -1;
	u8_t *data[4];

	// ��ȡ��Ƶ��ĳߴ���Ϣ
	w = XM_lcdc_osd_get_width (XM_LCDC_CHANNEL_0, XM_OSD_LAYER_0);
	h = XM_lcdc_osd_get_height (XM_LCDC_CHANNEL_0, XM_OSD_LAYER_0);
	// ����һ���µ���Ƶ���֡������
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
		// ��JPEG�������뵽�´�����֡����
		//�建��,��Ч����
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
		// ����ʧ��
		// ���framebuffer
		XM_osd_framebuffer_clear (yuv_framebuffer, 0, 255, 255, 255);
	}
	// ��framebuffer�ر�, �������Ϊ��ǰ����Ƶ����, ����OSD 0����ʾ
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

	// ��ȡ��Ƶ��ĳߴ���Ϣ
	w = XM_lcdc_osd_get_width (XM_LCDC_CHANNEL_0, XM_OSD_LAYER_0);
	h = XM_lcdc_osd_get_height (XM_LCDC_CHANNEL_0, XM_OSD_LAYER_0);
	// ����һ���µ���Ƶ���֡������
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
		// ��JPEG�������뵽�´�����֡����
		//�建��,��Ч����
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
    // ��ICON����������ʾ,������:
    // XM_osd_layer_initδ��ʼ��,osd_layer_format����δ0,uv���ݲ���,��ɵĽ����ɫ�ʲ���ʾ������
    // ���������: ��Ҫ���������ICON ��ʾ,ICON��ʾ�������(��һ�ε���)XM_osd_framebuffer_close ֮��,
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
		// ����ʧ��
		// ���framebuffer
		XM_osd_framebuffer_clear (yuv_framebuffer, 0, 255, 255, 255);
	}
	// ��framebuffer�ر�, �������Ϊ��ǰ����Ƶ����, ����OSD 0����ʾ
	XM_osd_framebuffer_close (yuv_framebuffer, 0);

	return ret;
}

// ����Ƭ��IRAM������Դ����Ȩ
void enter_region_to_access_iram (void)
{
	//OS_Use (&iram_access_semaphore);
}

// �ͷ�Ƭ��IRAM������Դ����Ȩ
void leave_region_to_access_iram (void)
{
	//OS_Unuse (&iram_access_semaphore);
}
