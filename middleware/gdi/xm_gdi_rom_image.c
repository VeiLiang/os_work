//****************************************************************************
//
//	Copyright (C) 2010 ShenZhen ExceedSpace
//
//	Author	ZhuoYongHong
//
//	File name: app_romimages.c
//	ROMͼ����Դ����, ��֧�ֵ��̲߳���
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


#define	MAX_ROMIMAGE_COUNT	32//64		// ���Cache�����IMAGE��Դ����


static int romimage_count;		// ��ʹ�õ�cache��Դ����
static XM_ROMIMAGE romimage_fifo[MAX_ROMIMAGE_COUNT];		// cache��Դ�����
static queue_s romimage_head;		// ʹ������
static queue_s romimage_free;		// ��������

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

// ��һ��ROM��Դָ���ֻ��ͼ����
// dwImageBase  ROM��Դ���ֽ�ƫ��
// dwImageSize  ROM��Դ���ֽڴ�С
static XM_ROMIMAGE *_RomImageOpenImage (DWORD dwImageBase, DWORD dwImageSize)
{
	XM_ROMIMAGE *romimage;

	// �����Ƿ��Ѵ�
	romimage = NULL;
	// �����Ѵ�IMAGE��Դ����
	if(!queue_empty(&romimage_head))
	{
		// �ǿ�����
		queue_s *next = queue_next (&romimage_head);
		while(next != &romimage_head)
		{
			// �Ƚ�ROM��Դƫ��
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
		// �Ѵ򿪣��Ƶ���β��
		// ��β�ĵ�Ԫ����滻(���δʹ���滻����)
		queue_delete ((queue_s *)romimage);
		// �Ƶ���β
		queue_insert ((queue_s *)romimage, &romimage_head);
	}
	else
	{
		// δ��
		XM_IMAGE *lpImage = XM_RomImageCreate (dwImageBase, dwImageSize, XM_OSD_LAYER_FORMAT_ARGB888);
		if(lpImage == NULL)
		{
			XM_printf ("_RomImageOpenImage failed, base=0x%08x, size=0x%08x, memory busy\n", dwImageBase, dwImageSize);
			return NULL;
		}

		if(romimage_count < MAX_ROMIMAGE_COUNT)
		{
			// ���ڿ�����
			// ȡ�����гصĵ�һ����Ԫ����˫�������жϿ�
			romimage = (XM_ROMIMAGE *)queue_next (&romimage_free);
			queue_delete ((queue_s *)romimage);

			romimage->dwImageBase = dwImageBase;
			romimage->lpImage = lpImage;
			// ���뵽��ʹ�ö��е�β��
			queue_insert ((queue_s *)romimage, &romimage_head);
			romimage_count ++;
		}
		else
		{
		    // XM_printf("MAX_ROMIMAGE_COUNT free \n ");
			// ��Ҫ�滻���δʹ�õ���
			// �滻��ʹ�ö��еĶ��׵�Ԫ(���δʹ���滻����)
			romimage = (XM_ROMIMAGE *)queue_next (&romimage_head);
			queue_delete ((queue_s *)romimage);
			// �ͷ�ԭ����Image��Դ
			XM_ImageDelete (romimage->lpImage);
			romimage->dwImageBase = dwImageBase;
			romimage->lpImage = lpImage;
			// ���뵽��ʹ�ö��еĶ�β
			queue_insert ((queue_s *)romimage, &romimage_head);
		}
	}
	return romimage;
}

// 
static XM_ROMIMAGE * RomImageFreeOneImageFromLink (void)
{
	XM_ROMIMAGE *romimage;
	// �����Ƿ��Ѵ�
	romimage = NULL;
	// �����Ѵ�IMAGE��Դ����
	if(!queue_empty(&romimage_head))
	{
		// �ǿ�����
		// ɾ��������Դ
		romimage = (XM_ROMIMAGE *)queue_delete_next (&romimage_head);
		if(romimage->lpImage == 0 || romimage->dwImageBase == 0)
		{
			printf ("Error romimage\n");
		}
		XM_ImageDelete (romimage->lpImage);
		romimage->lpImage = 0;
		romimage->dwImageBase = 0;
		// ���뵽��ʹ�ö��е�β��
		queue_insert ((queue_s *)romimage, &romimage_head);
	}
	return romimage;
}

// ��һ��ROM��Դָ���ֻ��ͼ����
// dwImageBase  ROM��Դ���ֽ�ƫ��
// dwImageSize  ROM��Դ���ֽڴ�С
XM_IMAGE *XM_RomImageOpen (DWORD dwImageBase, DWORD dwImageSize)
{
	int loop;
	XM_ROMIMAGE *romimage = _RomImageOpenImage (dwImageBase, dwImageSize);
	if(romimage)
		return romimage->lpImage;
	
	// ��ʧ��
	// ��������׵�Ԫ(���δʹ��)��ʼ, �ͷŲ���������Դ, Ȼ�����·���
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

// ��ָ�����ڵ�ָ���������Rom Image
int XM_RomImageDraw (	DWORD dwImageBase, DWORD dwImageSize, 
								HANDLE hWnd, 
								XMRECT *lpRect, DWORD dwFlag
							)
{
	int osd_image_x, osd_image_y;
	XM_IMAGE *image = XM_RomImageOpen (dwImageBase, dwImageSize);
	// �����Ƿ��Ѵ�
	if(image == NULL)
		return (-1);

	if(dwFlag == XMGIF_DRAW_POS_CENTER)
	{
		// ��PNGͼƬ��λ����ʾ������м�λ��
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
		// ��PNGͼƬ��λ����ʾ��������Ͻ�
		osd_image_x = lpRect->left;
		osd_image_y = lpRect->top;
	}

	XM_ImageDisplay (image, hWnd, osd_image_x, osd_image_y);

	return 0;	
}


// �ͷ�����ROM Image��Դ
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

// ��ROM��Դ(ͼ���ʽԴ����ΪPNG/GIF��ʽ)����IMAGE����
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

// ��ȡPNG/GIFͼ��ĳߴ�
// ����ֵ
// 0     �ɹ�
// < 0   ʧ��
int XM_GetRomImageSize (DWORD dwImageBase, DWORD dwImageSize, XMSIZE *lpSize)
{
#if ROM_SPI
	unsigned int rom_address;
	unsigned char *rom_buffer;
	unsigned char *lpImageBase;
	int ret = -1;
	
	// 512����, spi��ȡ�ٶ����
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