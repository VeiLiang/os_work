//****************************************************************************
//
//	Copyright (C) 2010 ShenZhen ExceedSpace
//
//	Author	ZhuoYongHong
//
//	File name: app_romimages.c
//	ROM图像资源管理, 仅支持单线程操作
//
//	Revision history
//
//		2012.09.01	ZhuoYongHong Initial version
//											
//
//****************************************************************************
/*
*/
#include <string.h>
#include <xm_user.h>
#include <xm_base.h>
#include <xm_rom.h>
#include <xm_printf.h>
#include <xm_queue.h>
#include <xm_image.h>
#include <stdio.h>
#include <xm_heap_malloc.h>
#include <xm_flash_space_define.h>


#define	MAX_ROMIMAGE_COUNT	32//64		// 最大Cache缓存的IMAGE资源个数


static int romimage_count;		// 已使用的cache资源个数
static XM_ROMIMAGE romimage_fifo[MAX_ROMIMAGE_COUNT];		// cache资源对象池
static queue_s romimage_head;		// 使用链表
static queue_s romimage_free;		// 空闲链表

void XM_RomImageInit (void)
{
	int i;
	romimage_count = 0;
	queue_initialize (&romimage_head);
	queue_initialize (&romimage_free);
	for (i = 0; i < MAX_ROMIMAGE_COUNT; i ++)
	{
		romimage_fifo[i].lpImage = NULL;
		romimage_fifo[i].dwImageBase = 0;
		queue_insert ((queue_s *)&romimage_fifo[i], &romimage_free);
	}
}

void XM_RomImageExit (void)
{
	XM_RomImageFreeAll ();
}

// 打开一个ROM资源指向的只读图像句柄
// dwImageBase  ROM资源的字节偏移
// dwImageSize  ROM资源的字节大小
static XM_ROMIMAGE *_RomImageOpenImage (DWORD dwImageBase, DWORD dwImageSize)
{
	XM_ROMIMAGE *romimage;

	// 检索是否已打开
	romimage = NULL;
	// 遍历已打开IMAGE资源链表
	if(!queue_empty(&romimage_head))
	{
		// 非空链表
		queue_s *next = queue_next (&romimage_head);
		while(next != &romimage_head)
		{
			// 比较ROM资源偏移
			if(((XM_ROMIMAGE *)next)->dwImageBase == dwImageBase)
			{
				romimage = (XM_ROMIMAGE *)next;
				break;
			}
			next = queue_next (next);
		}
	}

	if(romimage)
	{
		// 已打开，移到队尾。
		// 队尾的单元最后替换(最近未使用替换策略)
		queue_delete ((queue_s *)romimage);
		// 移到队尾
		queue_insert ((queue_s *)romimage, &romimage_head);
	}
	else
	{
		// 未打开
		XM_IMAGE *lpImage = XM_RomImageCreate (dwImageBase, dwImageSize, XM_OSD_LAYER_FORMAT_ARGB888);
		if(lpImage == NULL)
		{
			XM_printf ("_RomImageOpenImage failed, base=0x%08x, size=0x%08x, memory busy\n", dwImageBase, dwImageSize);
			return NULL;
		}

		if(romimage_count < MAX_ROMIMAGE_COUNT)
		{
			// 存在空闲项
			// 取出空闲池的第一个单元并从双向链表中断开
			romimage = (XM_ROMIMAGE *)queue_next (&romimage_free);
			queue_delete ((queue_s *)romimage);

			romimage->dwImageBase = dwImageBase;
			romimage->lpImage = lpImage;
			// 插入到已使用队列的尾部
			queue_insert ((queue_s *)romimage, &romimage_head);
			romimage_count ++;
		}
		else
		{
		    // XM_printf("MAX_ROMIMAGE_COUNT free \n ");
			// 需要替换最近未使用的项
			// 替换已使用队列的队首单元(最近未使用替换策略)
			romimage = (XM_ROMIMAGE *)queue_next (&romimage_head);
			queue_delete ((queue_s *)romimage);
			// 释放原来的Image资源
			XM_ImageDelete (romimage->lpImage);
			romimage->dwImageBase = dwImageBase;
			romimage->lpImage = lpImage;
			// 插入到已使用队列的队尾
			queue_insert ((queue_s *)romimage, &romimage_head);
		}
	}
	return romimage;
}

// 
static XM_ROMIMAGE * RomImageFreeOneImageFromLink (void)
{
	XM_ROMIMAGE *romimage;
	// 检索是否已打开
	romimage = NULL;
	// 遍历已打开IMAGE资源链表
	if(!queue_empty(&romimage_head))
	{
		// 非空链表
		// 删除队首资源
		romimage = (XM_ROMIMAGE *)queue_delete_next (&romimage_head);
		if(romimage->lpImage == 0 || romimage->dwImageBase == 0)
		{
			printf ("Error romimage\n");
		}
		XM_ImageDelete (romimage->lpImage);
		romimage->lpImage = 0;
		romimage->dwImageBase = 0;
		// 插入到已使用队列的尾部
		queue_insert ((queue_s *)romimage, &romimage_head);
	}
	return romimage;
}

// 打开一个ROM资源指向的只读图像句柄
// dwImageBase  ROM资源的字节偏移
// dwImageSize  ROM资源的字节大小
XM_IMAGE *XM_RomImageOpen (DWORD dwImageBase, DWORD dwImageSize)
{
	int loop;
	XM_ROMIMAGE *romimage = _RomImageOpenImage (dwImageBase, dwImageSize);
	if(romimage)
		return romimage->lpImage;
	
	// 打开失败
	// 从链表队首单元(最近未使用)开始, 释放并回收其资源, 然后重新分配
	//printf ("XM_RomImageOpen Recycle...\n");
	loop = 32;
	while (romimage == NULL && loop > 0)
	{
		if(RomImageFreeOneImageFromLink () == NULL)
			break;
		romimage = _RomImageOpenImage (dwImageBase, dwImageSize);
		loop --;
	}
	if(romimage == NULL)
	{
		printf ("XM_RomImageOpen Recycle NG\n");	
		return NULL;
	}
	else
	{
		printf ("XM_RomImageOpen Recycle OK\n");	
		return romimage->lpImage;
	}
}

// 在指定窗口的指定区域绘制Rom Image
int XM_RomImageDraw (	DWORD dwImageBase, DWORD dwImageSize, 
								HANDLE hWnd, 
								XMRECT *lpRect, DWORD dwFlag
							)
{
	int osd_image_x, osd_image_y;
	XM_IMAGE *image = XM_RomImageOpen (dwImageBase, dwImageSize);
	// 检索是否已打开
	if(image == NULL)
		return (-1);

	if(dwFlag == XMGIF_DRAW_POS_CENTER)
	{
		// 将PNG图片定位在显示区域的中间位置
		int temp = (lpRect->right - lpRect->left + 1);
		temp -= image->width;
		osd_image_x = lpRect->left + (temp) / 2;
		temp = (lpRect->bottom - lpRect->top + 1);
		temp -= image->height;
		osd_image_y = lpRect->top + ( temp ) / 2;

		if(osd_image_x < 0 || osd_image_y < 0)
		{
			XM_printf ("AP_RomImageDraw NG, osd_image_x = %d, osd_image_y = %d\n", osd_image_x, osd_image_y);
			return -1;
		}
	}
	else if(dwFlag == XMGIF_DRAW_POS_RIGHT)
	{
	    osd_image_x = lpRect->right - image->width;
	    osd_image_y = lpRect->top + (lpRect->bottom - lpRect->top - image->height) / 2;
	}
	else //if(dwFlag == XMGIF_DRAW_POS_LEFTTOP)
	{
		// 将PNG图片定位在显示区域的左上角
		osd_image_x = lpRect->left;
		osd_image_y = lpRect->top;
	}

	XM_ImageDisplay (image, hWnd, osd_image_x, osd_image_y);

	return 0;	
}


// 释放所有ROM Image资源
void XM_RomImageFreeAll (void)
{
	while(!queue_empty (&romimage_head))
	{
		XM_ROMIMAGE *romimage = (XM_ROMIMAGE *)queue_next(&romimage_head);
		queue_delete ((queue_s *)romimage);
		XM_ImageDelete (romimage->lpImage);
		romimage->lpImage = NULL;
		romimage->dwImageBase = 0;
		queue_insert ((queue_s *)romimage, &romimage_free);
	}
	romimage_count = 0;
}

extern int Spi_Read(UINT32 addr, UINT8 *buffer, UINT32 numBytes);

// 从ROM资源(图像格式源数据为PNG/GIF格式)创建IMAGE对象
XM_IMAGE * XM_RomImageCreate (DWORD dwImageBase, DWORD dwImageSize, DWORD dwFormat)
{
#if ROM_SPI
	unsigned int rom_address;
	unsigned char *rom_buffer;
	unsigned char *lpImageBase;
	XM_IMAGE *image = NULL;
	
	rom_address = (unsigned int)XM_RomAddress (dwImageBase);
	if(rom_address == NULL)
		return NULL;
	
	rom_buffer = (unsigned char *)XM_heap_malloc ( dwImageSize );
	if(rom_buffer == NULL)
		return NULL;
	if(Spi_Read ((unsigned int)rom_address, rom_buffer, dwImageSize) == 0)
	{
		XM_heap_free (rom_buffer);
		return NULL;
	}
	
	image = XM_ImageCreate (rom_buffer, dwImageSize, dwFormat); 
	
	XM_heap_free (rom_buffer);
	return image;
#else
	HANDLE rom_address = XM_RomAddress (dwImageBase);
	if(rom_address == NULL)
		return NULL;
	return XM_ImageCreate (rom_address, dwImageSize, dwFormat);
	
#endif
}

// 获取PNG/GIF图像的尺寸
// 返回值
// 0     成功
// < 0   失败
int XM_GetRomImageSize (DWORD dwImageBase, DWORD dwImageSize, XMSIZE *lpSize)
{
#if ROM_SPI
	unsigned int rom_address;
	unsigned char *rom_buffer;
	unsigned char *lpImageBase;
	int ret = -1;
	
	// 512对齐, spi读取速度最快
	rom_address = (unsigned int)XM_RomAddress (dwImageBase);
	if(rom_address == NULL)
		return ret;
	
	rom_buffer = (unsigned char *)XM_heap_malloc (dwImageSize );
	if(rom_buffer == NULL)
		return ret;
	if(Spi_Read ((unsigned int)rom_address, rom_buffer, dwImageSize) == 0)
	{
		XM_heap_free (rom_buffer);
		return ret;
	}
	
	ret = XM_GetImageSize (rom_buffer, dwImageSize, lpSize);	
	XM_heap_free (rom_buffer);
	return ret;
#else
	HANDLE rom_address = XM_RomAddress (dwImageBase);
	if(rom_address == NULL)
		return -1;
	return XM_GetImageSize (rom_address, dwImageSize, lpSize);
	
#endif	
}