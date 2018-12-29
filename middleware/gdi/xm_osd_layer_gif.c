//****************************************************************************
//
//	Copyright (C) 2011 Zhuo YongHong
//
//	Author	ZhuoYongHong
//
//	File name: xm_gif.h
//	  GIF����
//
//	Revision history
//
//		2011.05.27	ZhuoYongHong Initial version
//
//****************************************************************************

#include <xm_type.h>
#include <xm_base.h>
#include <xm_file.h>
#include <xm_heap_malloc.h>
#include "xm_osd_layer_gif.h"
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include "hw_osd_layer.h"
#include "xm_osd_layer.h"
#include <xm_printf.h>
#include <xm_osd_framebuffer.h>
#include <xm_user.h>



static int _GetOneByte(XMMAPFILE * pFile);

// ���GIFͼƬ�ߴ�
#define	MAX_GIF_WIDTH	1024
#define	MAX_GIF_HEIGHT	1024

#define	BYTE_TO_SHORT(a, b)	(USHORT)((((b)&0xFF) << 8) | ((a)&0xFF))

__no_init static XMGIF GifInfo;

static void int_memset (void *addr, int c, int size)
{
	int *dst = (int *)addr;
#if defined(WIN32)
	assert ( (size % 4) == 0);
#endif
	size >>= 2;

	while(size > 0)
	{
		*dst = c;
		dst ++;
		size --;
	}
}

#define RGB8882ARGB32(r,g,b)			((DWORD)((r  << 16) | (g << 8) | b ))

static void _read_color_map (XMMAPFILE *pFile, DWORD *lpColorMap, int cbColorMap)
{
	BYTE rgb[3];
	while(cbColorMap > 0)
	{
		// ÿ�δ���һ��RGB��ԭɫ
		XM_MapFileRead (pFile, rgb, 3);
		*lpColorMap = RGB8882ARGB32(rgb[0], rgb[1], rgb[2]);
		lpColorMap ++;
		cbColorMap --;
	}
}


static int _CheckGifAndGetBasicInfo (XMGIF * pGifInfo)
{
	UCHAR	ucTempBuffer[8];
	PGIF_COLORMAP	pColorMap = &pGifInfo->globalColorMap;
	XMMAPFILE *pFile = pGifInfo->gifFile;

	if(XM_MapFileSeek (pFile, 0, XM_SEEK_SET) < 0)
		return IMG_BAD_FILE;

	// ��ȡGIF�ļ�ͷ
	if(XM_MapFileRead (pFile, ucTempBuffer, 6) != 6)
		return IMG_BAD_FILE;
	
	if( ucTempBuffer[0] != 'G' || ucTempBuffer[1] != 'I' || ucTempBuffer[2] != 'F' 
		|| ucTempBuffer[3] != '8' || ucTempBuffer[5] != 'a' )
		return IMG_BAD_FILE;
	
	// Read Version
	if( ucTempBuffer[4] == '7' )
		pGifInfo->usVersion = VERSION_87;
	else if( ucTempBuffer[4] == '9' )
		pGifInfo->usVersion = VERSION_89;
	else
		return IMG_BAD_FILE;

	// ����Ϣ
	if( XM_MapFileRead (pFile, ucTempBuffer, 7) != 7 )
		return IMG_BAD_FILE;

	pGifInfo->usSreenWidth = (USHORT)BYTE_TO_SHORT(ucTempBuffer[0], ucTempBuffer[1]);
	pGifInfo->usSreenHeight = (USHORT)BYTE_TO_SHORT(ucTempBuffer[2], ucTempBuffer[3]);
	// ͼƬ̫��
	if( pGifInfo->usSreenWidth > MAX_GIF_WIDTH || pGifInfo->usSreenHeight > MAX_GIF_HEIGHT )
		return IMG_BAD_FILE;

	if( ucTempBuffer[4] & 0x80 ) // ������ȫ�� Color Map
	{
		DWORD unColorTable = 0x00000007 & ucTempBuffer[4];

		unColorTable = 1 << (unColorTable+1);
		//_memset (pColorMap->MapData, 0, 256*2);
		_read_color_map (pFile, (DWORD*)pColorMap->MapData, unColorTable);
		pColorMap->unMapSize = (WORD)(unColorTable << 1);
	}
	return IMG_OPER_SUCCESS;
}


typedef	struct	tagDE_CODE
{
	// �������ݽṹ�����Ե���ÿ����Ա�Ĵ�С��λ�ã���ʹ�û���Ż���
	UCHAR		*work_stack;	// 0
	UCHAR		*stack_top;		//	4
	UCHAR		*ucBuffer;		//	8
	USHORT	usClearCode;	// 12 32
	USHORT	usEndCode;		// 14	34

	USHORT	usCurBit;		// 16
	USHORT	usLastBit;		// 18	
	USHORT	usAvailable;	// 20
	USHORT	usCodeSize;		// 22
	USHORT	usCodeMask;		// 24	
	USHORT	usOldCode;		// 26
	USHORT	usFirstCode;	//	28	
	USHORT	usInputCode;	// 30
	DWORD *	MapData;			//	32 12

//	ULONG		scale;			// 36	44	����ϵ��

	ULONG		trans_color;	//	40	38	46
//	ULONG		scale_mask;		//	44	40	48


	ULONG		rx;					// 56	48	40	��ǰ֡�ұ߾�������(�ѷŴ�scale��)
	ULONG		dx;					// 60	44	36	��ǰX��������(�ѷŴ�scale��)
	ULONG		dy;					// 64	46	38 ��ǰY��������(�ѷŴ�scale��)


	ULONG		lx;					// 72	50	42 ��ǰ֡����������(�ѷŴ�scale��)

} DE_CODE, *PDE_CODE;


#define NULL_CODE  0x0000FFFF




static int _GetOneByte(XMMAPFILE * pFile)
{
	BYTE byte = 0;
	if(XM_MapFileRead (pFile, &byte, 1) != 1)
		return -1;
	return  byte;
}

//---Խ��һϵ������
static void _PassData (XMMAPFILE * pFile)
{
	int	nBytes = _GetOneByte(pFile);

	while( nBytes > 0 )
	{
		XM_MapFileSeek (pFile, nBytes, XM_SEEK_CUR);
		nBytes = _GetOneByte (pFile);
	}
}


static XMGIF*	_gif_open (XMMAPFILE *pFile)
{
	int	nReturn;
	XMGIF *pGifInfo = &GifInfo;
	
	if(pFile == NULL || pGifInfo == NULL)
		return NULL;

	int_memset (pGifInfo, 0, sizeof(XMGIF));

	pGifInfo->gifFile = pFile;

	//pGifInfo->unNowPicture = 1;	//Ĭ����ʾ��һ��ͼƬ

	nReturn = _CheckGifAndGetBasicInfo (pGifInfo);
	if( nReturn != IMG_OPER_SUCCESS )
	{
		return NULL;
	}

	pGifInfo->gifContext.nFirstOffset = XM_MapFileTell (pGifInfo->gifFile);

	//������һ��Ĭ�ϵ�
	pGifInfo->gifContext.usDelayTime = 0;
	pGifInfo->gifContext.usNeedBrushBk = 0;
	pGifInfo->gifContext.usTransColor = 0xFFF;

	pGifInfo->usPictureCount = 0;
	
	return pGifInfo;
}

static int	_gif_close (XMGIF *pGifInfo)
{
	if(pGifInfo == NULL)
		return (-1);

	return IMG_OPER_SUCCESS;
}




//ִ�д� ����ǰ�������Ѿ������� Global Color Map, �ļ��Ѿ���λ�����ݿ�
//���Ҵ˺��� �� �ļ��� �����Ժܸ�...
/*
	; ��ʾ�ļ�������
	, ��ʾͼ����Ϣ
	! ��ʾע����Ϣ
*/
// ����GIFͼ�εĵ�ǰ֡��������
static int gif_decode_frame (XMGIF * pGifInfo, BYTE bDecodeFlag)
{
	XMMAPFILE* pFile = pGifInfo->gifFile;
	int		nCharacter;
	UCHAR		ucTempBuff[10];
	PGIF_IMGDATA pImgData = &pGifInfo->imgData;

	pImgData->usHaveColorMap = 0;

	if(bDecodeFlag == XMGIF_FLAG_DECODE_NORMAL)
	{

	}
	else if(bDecodeFlag == XMGIF_FLAG_DECODE_BEGIN)
	{
		XM_MapFileSeek (pGifInfo->gifFile, pGifInfo->gifContext.nFirstOffset, XM_SEEK_SET);
	}
	else
		return (-1);

	for(;;)
	{
		nCharacter = _GetOneByte(pFile);

		if( nCharacter == ',' )
		{
			// ��ȡ��ǰ֡��λ����Ϣ
			if( XM_MapFileRead (pGifInfo->gifFile, ucTempBuff, 9) != 9 )
				return IMG_BAD_FILE;
			pImgData->usPosLeft = (USHORT)BYTE_TO_SHORT(ucTempBuff[0], ucTempBuff[1]);
			pImgData->usPosTop = (USHORT)BYTE_TO_SHORT(ucTempBuff[2], ucTempBuff[3]);
			pImgData->usNowWidth = (USHORT)BYTE_TO_SHORT(ucTempBuff[4], ucTempBuff[5]);
			pImgData->usNowHeight = (USHORT)BYTE_TO_SHORT(ucTempBuff[6], ucTempBuff[7]);
			
			if( ucTempBuff[8] & 0x80 )
			{
				// ��ȡ�ֲ���ɫ��
				int unColorTable = ucTempBuff[8] & 0x07;

				unColorTable =  1 << (unColorTable+1);
				//_memset (pImgData->localColorMap.MapData, 0, 256*2);
				//_read_color_map (pGifInfo->gifFile, (WORD*)pImgData->localColorMap.MapData, unColorTable);
				_read_color_map (pGifInfo->gifFile, (DWORD*)pImgData->localColorMap.MapData, unColorTable);
				pImgData->localColorMap.unMapSize = (WORD)(unColorTable << 1);

				pImgData->usHaveColorMap = 1;
			}
			pImgData->usInterlace = (USHORT)((ucTempBuff[8] & 0x40) ? 1 : 0);
			if(pImgData->usInterlace)
			{
				// ��֧�ֽ����ʽ
				return IMG_BAD_FILE;
			}

			return IMG_OPER_SUCCESS;
		}
		else if( nCharacter == '!' )// ingore all comment
		{
			if( pGifInfo->usVersion/* == VERSION_89*/ )
			{
				if( _GetOneByte(pFile) == 0xF9 )
				{
					UCHAR	ucControl;
					int	nLen = _GetOneByte(pFile);
					if(nLen > 10)
						return IMG_BAD_FILE;

					if( XM_MapFileRead (pFile, ucTempBuff, nLen) != nLen )
						return IMG_BAD_FILE;
					pGifInfo->gifContext.usDelayTime = BYTE_TO_SHORT(ucTempBuff[1], ucTempBuff[2]);
					ucControl = ucTempBuff[0];
					if( ucControl & 0x01 )	//��͸��ɫ
					{
						pGifInfo->gifContext.usTransColor = ucTempBuff[3];
					}
					else
					{
						pGifInfo->gifContext.usTransColor = NO_TRANS_COLOR;
					}
					ucControl >>= 2;
					ucControl &= 0x07;
					if( ucControl >= 2 )
					{
						pGifInfo->gifContext.usNeedBrushBk = 1;
					}
					else
					{
						pGifInfo->gifContext.usNeedBrushBk = 0;
					}
				}//end of image control
			}
			else
			{
				XM_MapFileSeek (pFile, 1, XM_SEEK_CUR);
			}
			_PassData (pFile);
		}
		else if( nCharacter == ';' )
		{
			return IMG_OPER_FINISH;
		}
		else if( nCharacter == 0 )
			continue;
		else
		{
			continue;
		}	
	}

	return IMG_OPER_SUCCESS;
}







static int _output_gif (register PDE_CODE pDeCode, ULONG *prefix_code, 									
									unsigned char *osd_layer_buffer, 
									unsigned int osd_layer_format,
									unsigned int osd_layer_width, unsigned int osd_layer_height,
									unsigned int osd_layer_stride,
									unsigned int osd_image_x, unsigned int osd_image_y,
									XM_COLORREF transparent_color,
									unsigned int alpha
									)
{
	/*��ʱ����*/
	ULONG		usCode;
	USHORT	usInCode;
	/*��Ҫ��д����*/
	UCHAR		*work_stack;
	USHORT	usCurBit, usAvailable, usCodeSize;
	USHORT	usCodeMask, usOldCode, usFirstCode;
	/*ֻ������*/
	USHORT	usClearCode;
	UCHAR		*stack_top;

	unsigned char* buffer;


	/*��ʼ����*/
	/*�����ı��*/
	work_stack = pDeCode->work_stack;
	usCurBit = pDeCode->usCurBit;
	usAvailable = pDeCode->usAvailable;
	usOldCode = pDeCode->usOldCode;
	usFirstCode = pDeCode->usFirstCode;
	/*ż���ı��*/
	usCodeSize = pDeCode->usCodeSize;
	usCodeMask = pDeCode->usCodeMask;

	usClearCode = pDeCode->usClearCode;
	stack_top = pDeCode->stack_top;

	while ( usCurBit+usCodeSize <= pDeCode->usLastBit )	
	{
		// ��ȡһ�����벢������usCode
		{
			USHORT	usNowBytes;
			UCHAR		*pBuf;

			usNowBytes = (USHORT)(usCurBit >> 3);
			pBuf = pDeCode->ucBuffer + usNowBytes;
			if( usNowBytes & 0x03 )
			{
				if( usNowBytes & 0x01 )
				{
					usCode = (ULONG)*(pBuf);
					usCode |= ( (*(USHORT*)(pBuf+1)) << 8 );
				}
				else	//	2 �ֶ���
				{
					usCode = (ULONG) *(USHORT*)(pBuf);
					usCode |= ( (*(pBuf+2)) << 16 );
				}
			}
			else	//	4 �ֶ����
			{
				usCode = *(ULONG*)pBuf;
			}
			//end of use usNowBytes

			usCode >>= (usCurBit&7);
			usCode = (USHORT)(usCode & usCodeMask);
		}

		usCurBit = (USHORT)(usCurBit + usCodeSize);

		if ( usCode == pDeCode->usEndCode )
			return (-1);	// �����������
		else if ( usCode == usClearCode )
		{
			usCodeSize = (USHORT)(pDeCode->usInputCode + 1);
			usCodeMask = (USHORT)((1<<usCodeSize) - 1);
			usAvailable = (USHORT)(usClearCode + 2);
			usOldCode = NULL_CODE;

			continue;
		}
		else if ( usCode > usAvailable )
			return (-1);		// ���������

		usInCode = (USHORT)usCode;
		if( usCode >= usAvailable )
		{
			*work_stack++ = (UCHAR) usFirstCode;
			usCode = usOldCode;
		}
		while (usCode >= usClearCode)
		{
			ULONG	rc = prefix_code[usCode];
			*work_stack++ = (UCHAR)rc;
			usCode = (USHORT)(rc >> 16);
		}
		usFirstCode = (USHORT)usCode;
		
		*work_stack++ = (UCHAR) usFirstCode;

		if( usOldCode != NULL_CODE )
		{
			ULONG	rc;
			rc = (usOldCode<<16) | (usFirstCode);
			prefix_code[usAvailable] = rc;
			usAvailable++;
			
			if (((usAvailable & usCodeMask) == 0) && (usAvailable < MAX_LZW_TABLE))
			{
				usCodeSize++;
				usCodeMask = (USHORT)(usCodeMask + usAvailable);
			}
			
		}
		usOldCode = usInCode;
	
		/*ֻ�Էǽ���ͼ*/
		while( work_stack > stack_top )
		{
			BYTE c_index;
			unsigned int clr;
			
			-- work_stack;
			c_index = *work_stack;
			
			// ����Ƿ�͸��ɫ
			if(c_index == pDeCode->trans_color && transparent_color == 0xFFFFFFFF)
			{
				// ͸��ɫ�Ҳ���䱳��
			}
			else 
			{
				unsigned int x, y;
				x = pDeCode->dx;
				y = pDeCode->dy;
				if(c_index == pDeCode->trans_color)	// ͸��ɫ
					clr = transparent_color;
				else
					clr = pDeCode->MapData[c_index];
				if(osd_layer_format == XM_OSD_LAYER_FORMAT_ARGB888)
				{
					//unsigned int d_rgb;
					//unsigned int a_a;
					//unsigned int a_r, a_g, a_b; 
					//unsigned int a, r, g, b;
					buffer = (unsigned char *)osd_layer_buffer;
					buffer += y * osd_layer_stride + x * 4;
					*(unsigned int *)buffer = (alpha << 24) | clr;

					/*
					r = clr & 0xFF;
					g = r;
					b = r;
					a = 0xFF - r;
							
					d_rgb = *(unsigned int *)buffer;
					a_a = (d_rgb >> 24);
					a_r = ( ((d_rgb >> 16) & 0xFF) * (255 - a) + r * a) / 255;
					a_g = ( ((d_rgb >> 8 ) & 0xFF) * (255 - a) + g * a) / 255;
					a_b = ( (d_rgb & 0xFF)  * (255 - a) + b * a) / 255;
					*(unsigned int *)buffer = (unsigned int)((a_r << 16) | (a_g << 8) | a_b | (a_a << 24));
					*/
					
				}
				else if(osd_layer_format == XM_OSD_LAYER_FORMAT_RGB565)
				{
					buffer = (unsigned char *)osd_layer_buffer;
					buffer += y * osd_layer_stride + x * 2;
					*(unsigned short *)buffer = (unsigned short)ARGB888_TO_RGB565(clr);
				}
				else if(osd_layer_format == XM_OSD_LAYER_FORMAT_ARGB454)
				{
					buffer = (unsigned char *)osd_layer_buffer;
					buffer += y * osd_layer_stride + x * 2;
					*(unsigned short *)buffer = (unsigned short)((((clr >> 16) & 0xF0) << 5) 
						| (((clr >> 8) & 0xF8) << 1) 
						| ((clr & 0xFF) >> 4) 
						| ((alpha >> 5) << 13));
				}
			}
			
			pDeCode->dx ++;
			if(pDeCode->dx == pDeCode->rx)
			{
				pDeCode->dx = pDeCode->lx;
				pDeCode->dy ++;
			}
			
		}	// while( work_stack > stack_top )
		
		// �˴�������д����̵������м�������浽�ṹ����

	}//end of inside while...--------------------------------------

	pDeCode->usCodeSize = usCodeSize;
	pDeCode->usCodeMask = usCodeMask;
	pDeCode->work_stack = work_stack;
	pDeCode->usCurBit = usCurBit;
	pDeCode->usAvailable = usAvailable;
	pDeCode->usOldCode = usOldCode;
	pDeCode->usFirstCode = usFirstCode;
	return 1;
}

static int gif_render_frame (XMGIF * pGifInfo, 
									unsigned char *osd_layer_buffer, 
									unsigned int osd_layer_format,
									unsigned int osd_layer_width, unsigned int osd_layer_height,
									unsigned int osd_layer_stride,
									unsigned int osd_image_x, unsigned int osd_image_y,
									XM_COLORREF transparent_color,
									unsigned int alpha
									)
{
	UCHAR	ucBuf[262+2];		// 4�ֽڶ���
	ULONG *prefix_code;
	UCHAR *ucBuffer;

	XMMAPFILE*	pFile = pGifInfo->gifFile;
	int	y, nReturn;
	int	ret;
	USHORT	usLastBytes;
	DE_CODE	deCode;
	PGIF_COLORMAP	pColorMap;

	int ly;

	prefix_code = pGifInfo->prefix_code;
	ucBuffer = ucBuf+4;


	if(pGifInfo->imgData.usHaveColorMap)	// ���ھֲ���ɫ��
		pColorMap = &pGifInfo->imgData.localColorMap;
	else	// ʹ��ȱʡ��ȫ����ɫ��
		pColorMap = &pGifInfo->globalColorMap;
	
	deCode.stack_top = pGifInfo->work_stack;
	deCode.work_stack = pGifInfo->work_stack;
	deCode.ucBuffer = ucBuffer;
	deCode.usInputCode = (USHORT)_GetOneByte(pFile);
	if( deCode.usInputCode < 2 || deCode.usInputCode >= MAX_LZW_BITS )
		return IMG_BAD_FILE;
	deCode.usCodeSize = (USHORT)(deCode.usInputCode + 1);
	deCode.usClearCode = (USHORT)(1 << deCode.usInputCode);
	deCode.usEndCode = (USHORT)(deCode.usClearCode + 1);
	deCode.usCodeMask = (USHORT)((1<<deCode.usCodeSize) - 1);
	deCode.usAvailable = (USHORT)(deCode.usClearCode + 2);

	for(y=0; y<(int)deCode.usClearCode; ++y)
	{
		prefix_code[y] = (UCHAR)y;
	}

	deCode.usCurBit = 0;
	deCode.usLastBit = 0;
	usLastBytes = 0;
	
	deCode.usOldCode = NULL_CODE;
	deCode.usFirstCode = 0;

	ret = IMG_OPER_SUCCESS;

	// deCode.scale_mask = 0x0000;

	deCode.lx = (XMCOORD)(pGifInfo->imgData.usPosLeft + osd_image_x);
	deCode.rx = (XMCOORD)(deCode.lx + pGifInfo->imgData.usNowWidth);
	deCode.dx = deCode.lx;
	deCode.dy = (XMCOORD)(pGifInfo->imgData.usPosTop + osd_image_y);

	deCode.trans_color = pGifInfo->gifContext.usTransColor;



	ly = deCode.dy + pGifInfo->imgData.usNowHeight;

	ucBuf[264 - 1] = 0;
	ucBuf[264 - 2] = 0;

	deCode.MapData = (DWORD *)pColorMap->MapData;

	if( pGifInfo->imgData.usInterlace )
	//����ͼ---------------------------------------------------------------
	{
		// ��֧�ֽ���ͼ
		return IMG_BAD_FILE;
	}
	//-------------------------------------------------------------------------------------
	else	/* �ǽ���ͼ */
	{
		while ( (int)deCode.dy < ly )
		{
			/* ������ */
			nReturn = _output_gif (&deCode, prefix_code, 
								osd_layer_buffer,
								osd_layer_format,
								osd_layer_width, osd_layer_height,
								osd_layer_stride,
								osd_image_x, osd_image_y,
								transparent_color,
								alpha);
			if( nReturn == 0 )
			{
			}
			else if( nReturn == 1 )
			{
				// ��ȡ����������
				int nCount = _GetOneByte(pFile);
				if( nCount == 0 )
				{
					break;
				}
				if( nCount > 262 )
				{
					ret = IMG_BAD_FILE;
					break;
				}
				if(usLastBytes == 0)
				{
					ucBuffer[0] = 0;
					ucBuffer[1] = 0;
				}
				else
				{
					ucBuffer[0] = ucBuffer[usLastBytes-2];
					ucBuffer[1] = ucBuffer[usLastBytes-1];
				}
				usLastBytes = (USHORT)(nCount+2);
				if( XM_MapFileRead (pFile, &ucBuffer[2], nCount) != nCount )
				{
					ret = IMG_BAD_FILE;
					break;
				}
				
				deCode.usCurBit = (USHORT)(deCode.usCurBit - deCode.usLastBit + 16);
				deCode.usLastBit = (USHORT)(usLastBytes << 3);
			}
			else
			{
				ret = IMG_OTHER_ERROR;
				break;
			}
		}//end of outside while...
	}

	return ret;
}

int XM_lcdc_load_gif_image (unsigned char *osd_layer_buffer, 
									unsigned int osd_layer_format,
									unsigned int osd_layer_width, unsigned int osd_layer_height,
									unsigned int osd_layer_stride,
									unsigned int osd_image_x, unsigned int osd_image_y,
									const char *gif_image_buff, unsigned int gif_image_size,
									XM_COLORREF transparent_color,
									unsigned int alpha)
{
	XMMAPFILE	MapFile, *pMapFile;
	XMGIF		*pGifInfo;
	int			ret = IMG_OPER_SUCCESS;
	
	if(gif_image_buff == NULL || gif_image_size == 0)
		return IMG_BAD_PARM;;
	if(	osd_layer_format == XM_OSD_LAYER_FORMAT_YUV420
		||	osd_layer_format == XM_OSD_LAYER_FORMAT_Y_UV420)
	{
		XM_printf ("can't load gif image into YUV420 or Y_UV420 OSD layer\n");
		return IMG_BAD_PARM;
	}

	pMapFile = &MapFile;
	int_memset (pMapFile, 0, sizeof(MapFile));

	while (pMapFile)
	{
		if(XM_MapFileOpen (gif_image_buff, gif_image_size, pMapFile) == 0)
		{
			ret = IMG_BAD_FILE;
			break;
		}
		pGifInfo = _gif_open (pMapFile);
		if(pGifInfo == NULL)
		{
			XM_MapFileClose (pMapFile);
			ret = IMG_BAD_FILE;
			break;
		}
		// ���GIF�Ƿ����������ʾ
		if(pGifInfo->usSreenWidth > (osd_layer_width - osd_image_x))
		{
			_gif_close (pGifInfo);
			XM_MapFileClose (pMapFile);
			ret = IMG_IMAGE_TOO_LARGE;
			XM_printf ("GIF image's width(%d) exceed osd layer's size(%d)\n", 
				pGifInfo->usSreenWidth, (osd_layer_width - osd_image_x));
			break;
		}
		if(pGifInfo->usSreenHeight > (osd_layer_height - osd_image_y))
		{
			_gif_close (pGifInfo);
			XM_MapFileClose (pMapFile);
			ret = IMG_IMAGE_TOO_LARGE;
			XM_printf ("GIF image's height(%d) exceed osd layer's size(%d)\n", 
				pGifInfo->usSreenHeight, (osd_layer_height - osd_image_y));
			break;
		}

		if(gif_decode_frame (pGifInfo, XMGIF_FLAG_DECODE_BEGIN) < 0)
		{
			_gif_close (pGifInfo);
			XM_MapFileClose (pMapFile);
			ret = IMG_OTHER_ERROR;
			break;
		}

		gif_render_frame (pGifInfo, 
			osd_layer_buffer,
			osd_layer_format,
			osd_layer_width, osd_layer_height,
			osd_layer_stride,
			osd_image_x, osd_image_y,
			transparent_color,
			alpha);
		
		_gif_close (pGifInfo);
		XM_MapFileClose (pMapFile);
		
		ret = IMG_OPER_SUCCESS;
		break;
	}
	return ret;
}

int XM_lcdc_load_gif_image_file (unsigned char *osd_layer_buffer, 
									unsigned int osd_layer_format,
									unsigned int osd_layer_width, unsigned int osd_layer_height,
									unsigned int osd_layer_stride,
									unsigned int osd_image_x, unsigned int osd_image_y,
									const char *gif_image_file,
									XM_COLORREF transparent_color,
									unsigned int alpha)
{
	unsigned int file_size;
	char *gif_image = NULL;
	int ret = 0;

	void *fp = XM_fopen (gif_image_file, "rb");

	while(fp)
	{	
		file_size = XM_filesize (fp);
		if(file_size == 0)
			break;
		
		gif_image = XM_heap_malloc (file_size);
		if(gif_image == NULL)
		{
			XM_printf ("XM_lcdc_osd_layer_load_gif_image_file %s NG, XM_heap_malloc failed\n", gif_image_file);
			break;
		}

		if(XM_fread (gif_image, 1, file_size, fp) != file_size)
			break;

		ret = XM_lcdc_load_gif_image (osd_layer_buffer, 
												osd_layer_format,
												osd_layer_width, osd_layer_height,
												osd_layer_stride,
												osd_image_x, osd_image_y,
												gif_image, file_size,
												transparent_color,
												alpha);
		if(ret == IMG_OPER_SUCCESS)
		{
			ret = 1;
		}
		else
		{
			ret = 0;
			XM_printf ("XM_lcdc_load_png_image_file %s NG, load failed\n", gif_image_file);
		}
		break;
	} ;

	if(fp)
		XM_fclose (fp);
	if(gif_image)
		XM_heap_free (gif_image);

	return ret;
}


// ��ȡGIFͼ��ĳߴ�
int XM_GetGifImageSize (VOID *lpImageBase, DWORD dwImageSize, XMSIZE *lpSize)
{
	XMMAPFILE	MapFile, *pMapFile;
	XMGIF			*pGifInfo;
	int			ret = IMG_OPER_SUCCESS;
	
	if(lpImageBase == NULL || dwImageSize == 0 || lpSize == NULL)
		return IMG_BAD_PARM;

	pMapFile = &MapFile;
	int_memset (pMapFile, 0, sizeof(MapFile));

	while (pMapFile)
	{
		if(XM_MapFileOpen (lpImageBase, dwImageSize, pMapFile) == 0)
		{
			ret = IMG_BAD_FILE;
			break;
		}
		pGifInfo = _gif_open (pMapFile);
		if(pGifInfo == NULL)
		{
			XM_MapFileClose (pMapFile);
			ret = IMG_BAD_FILE;
			break;
		}

		lpSize->cx = pGifInfo->usSreenWidth;
		lpSize->cy = pGifInfo->usSreenHeight;

		_gif_close (pGifInfo);
		XM_MapFileClose (pMapFile);
		
		ret = IMG_OPER_SUCCESS;
		break;
	}
	return ret;
}

int XM_DrawGifImageEx (VOID *lpImageBase, DWORD dwImageSize, HANDLE hWnd, 
						  XMRECT *lpRect, DWORD dwFlag)
{
	//int			nRectWidth, nRectHeight;
	XMRECT		rc;
	int			offset;
	XMSIZE		size;

	xm_osd_framebuffer_t framebuffer = XM_GetWindowFrameBuffer (hWnd);
	if(framebuffer == NULL)
		return -1;

	if(lpImageBase == NULL || dwImageSize == 0 || lpRect == NULL)
		return IMG_BAD_PARM;

	memset (&rc, 0, sizeof(rc));

	// ��ȡGIFͼ��ĳߴ���Ϣ
	if(XM_GetGifImageSize (lpImageBase, dwImageSize, &size) < 0)
		return IMG_BAD_PARM;


	//nRectWidth = lpRect->right - lpRect->left + 1;
	//nRectHeight = lpRect->bottom - lpRect->top + 1;

	rc.left = lpRect->left;
	rc.right = lpRect->right;
	rc.top = lpRect->top;
	rc.bottom = lpRect->bottom;
		
	// ��GIFͼƬ��λ����ʾ������м�λ��
	if(dwFlag == XMGIF_DRAW_POS_CENTER)
	{
		offset = ((rc.right - rc.left + 1) - size.cx ) / 2;
		rc.left = (XMCOORD)(rc.left + offset);
		rc.right = (XMCOORD)(rc.right - offset);
		offset = ((rc.bottom - rc.top + 1) - size.cy) / 2;
		rc.top = (XMCOORD)(rc.top + offset);
		rc.bottom = (XMCOORD)(rc.bottom - offset);
	}
	else
	{
		// ȱʡ���ϽǶ�����ʾ
	}		

	return XM_lcdc_load_gif_image (framebuffer->address,
		framebuffer->format,
		framebuffer->width,
		framebuffer->height,
		framebuffer->stride,
		rc.left,
		rc.top,
		lpImageBase,
		dwImageSize,
		0xFFFFFFFF,
		XM_GetWindowAlpha(hWnd));
}
