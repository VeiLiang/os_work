//****************************************************************************
//
//	Copyright (C) 2011 Zhuo YongHong
//
//	Author	ZhuoYongHong
//
//	File name: xm_gif.h
//	  GIF解码
//
//	Revision history
//
//		2011.05.27	ZhuoYongHong Initial version
//
//****************************************************************************

#include <xm_type.h>
#include <xm_dev.h>
#include <xm_base.h>
#include <xm_file.h>
#include <xm_gdi.h>
#include <xm_user.h>
#include "gif.h"
#include <string.h>
#include <xm_malloc.h>
#include <assert.h>
#include <xm_osd_framebuffer.h>

#define	_XM_ARGB32BPP_DEVICE_

static int _GetOneByte(XMMAPFILE * pFile);

// 最大GIF图片尺寸
#define	MAX_GIF_WIDTH	640
#define	MAX_GIF_HEIGHT	480

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

#if defined(_XM_RGB16BPP_DEVICE_)

static void _read_color_map (XMMAPFILE *pFile, WORD *lpColorMap, int cbColorMap)
{
	BYTE rgb[3];
	while(cbColorMap > 0)
	{
		// 每次处理一个RGB三原色
		XM_MapFileRead (pFile, rgb, 3);
		*lpColorMap = RGB8882RGB565(rgb[0], rgb[1], rgb[2]);
		lpColorMap ++;
		cbColorMap --;
	}
}

#elif defined(_XM_ARGB32BPP_DEVICE_)

static void _read_color_map (XMMAPFILE *pFile, DWORD *lpColorMap, int cbColorMap)
{
	BYTE rgb[3];
	while(cbColorMap > 0)
	{
		// 每次处理一个RGB三原色
		XM_MapFileRead (pFile, rgb, 3);
		*lpColorMap = RGB8882ARGB32(rgb[0], rgb[1], rgb[2]);
		lpColorMap ++;
		cbColorMap --;
	}
}

#else

#error please define _XM_RGB16BPP_DEVICE_ or _XM_ARGB32BPP_DEVICE_

#endif

static int _CheckGifAndGetBasicInfo (XMGIF * pGifInfo)
{
	UCHAR	ucTempBuffer[8];
	PGIF_COLORMAP	pColorMap = &pGifInfo->globalColorMap;
	XMMAPFILE *pFile = pGifInfo->gifFile;

	if(XM_MapFileSeek (pFile, 0, XM_SEEK_SET) < 0)
		return IMG_BAD_FILE;

	// 读取GIF文件头
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

	// 读信息
	if( XM_MapFileRead (pFile, ucTempBuffer, 7) != 7 )
		return IMG_BAD_FILE;

	pGifInfo->usSreenWidth = (USHORT)BYTE_TO_SHORT(ucTempBuffer[0], ucTempBuffer[1]);
	pGifInfo->usSreenHeight = (USHORT)BYTE_TO_SHORT(ucTempBuffer[2], ucTempBuffer[3]);
	// 图片太大
	if( pGifInfo->usSreenWidth > MAX_GIF_WIDTH || pGifInfo->usSreenHeight > MAX_GIF_HEIGHT )
		return IMG_BAD_FILE;

	if( ucTempBuffer[4] & 0x80 ) // 设置了全局 Color Map
	{
		DWORD unColorTable = 0x00000007 & ucTempBuffer[4];

		unColorTable = 1 << (unColorTable+1);
		//_memset (pColorMap->MapData, 0, 256*2);
#if defined(_XM_RGB16BPP_DEVICE_)
		_read_color_map (pFile, (WORD*)pColorMap->MapData, unColorTable);
#else
		_read_color_map (pFile, (DWORD*)pColorMap->MapData, unColorTable);
#endif
		pColorMap->unMapSize = (WORD)(unColorTable << 1);
	}
	return IMG_OPER_SUCCESS;
}


typedef	struct	tagDE_CODE
{
	// 以下数据结构不可以调整每个成员的大小及位置（已使用汇编优化）
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
#if defined(_XM_RGB16BPP_DEVICE_)
	WORD *	MapData;			//	32 12
#elif defined(_XM_ARGB32BPP_DEVICE_)
	DWORD *	MapData;			//	32 12
#endif

	ULONG		scale;			// 36	44	缩放系数

	ULONG		trans_color;	//	40	38	46
	ULONG		scale_mask;		//	44	40	48


	ULONG		gif_cache_sx;		// 44	cache起始X坐标
	ULONG		gif_cache_sy;		// 48	cache起始Y坐标
	ULONG		gif_cache_count;	// 52
	ULONG		rx;					// 56	48	40	当前帧右边绝对坐标(已放大scale倍)
	ULONG		dx;					// 60	44	36	当前X绝对坐标(已放大scale倍)
	ULONG		dy;					// 64	46	38 当前Y绝对坐标(已放大scale倍)
	ULONG		gif_cache_lx;		// 68	cache最后一次写入坐标


	ULONG		lx;					// 72	50	42 当前帧左侧绝对坐标(已放大scale倍)

} DE_CODE, *PDE_CODE;


#define NULL_CODE  0x0000FFFF


//
static int _OutputGIF(register PDE_CODE pDeCode, ULONG *prefix_code, xm_osd_framebuffer_t framebuffer )
{
	/*临时数据*/
	ULONG		usCode;
	USHORT	usInCode;
	/*需要回写数据*/
	UCHAR		*work_stack;
	USHORT	usCurBit, usAvailable, usCodeSize;
	USHORT	usCodeMask, usOldCode, usFirstCode;
	/*只读数据*/
	USHORT	usClearCode;
	UCHAR		*stack_top;

	/*初始数据*/
	/*经常改变的*/
	work_stack = pDeCode->work_stack;
	usCurBit = pDeCode->usCurBit;
	usAvailable = pDeCode->usAvailable;
	usOldCode = pDeCode->usOldCode;
	usFirstCode = pDeCode->usFirstCode;
	/*偶尔改变的*/
	usCodeSize = pDeCode->usCodeSize;
	usCodeMask = pDeCode->usCodeMask;

	usClearCode = pDeCode->usClearCode;
	stack_top = pDeCode->stack_top;

	while ( usCurBit+usCodeSize <= pDeCode->usLastBit )	
	{
		// 读取一个编码并保存在usCode
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
				else	//	2 字对齐
				{
					usCode = (ULONG) *(USHORT*)(pBuf);
					usCode |= ( (*(pBuf+2)) << 16 );
				}
			}
			else	//	4 字对齐的
			{
				usCode = *(ULONG*)pBuf;
			}
			//end of use usNowBytes

			usCode >>= (usCurBit&7);
			usCode = (USHORT)(usCode & usCodeMask);
		}

		usCurBit = (USHORT)(usCurBit + usCodeSize);

		if ( usCode == pDeCode->usEndCode )
			return (-1);	// 解码结束返回
		else if ( usCode == usClearCode )
		{
			usCodeSize = (USHORT)(pDeCode->usInputCode + 1);
			usCodeMask = (USHORT)((1<<usCodeSize) - 1);
			usAvailable = (USHORT)(usClearCode + 2);
			usOldCode = NULL_CODE;

			continue;
		}
		else if ( usCode > usAvailable )
			return (-1);		// 解码出错返回

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
	
		{
			/*只对非交错图*/
			while( work_stack > stack_top )
			{
				BYTE c_index;
				
				-- work_stack;
				if( (pDeCode->dy & pDeCode->scale_mask) == 0)
				{
					c_index = *work_stack;
					// 检查是否透明色
					if(c_index == pDeCode->trans_color)
					{
						// 透明色，过滤
					}
					else 
					{
						XMCOLOR clr;
						unsigned int off_x, off_y;
						unsigned char *off_addr;

						off_x = pDeCode->dx >> pDeCode->scale;
						off_y = pDeCode->dy >> pDeCode->scale;

						clr = pDeCode->MapData[c_index];
#ifdef LCD_ROTATE_90
#if VERT_REVERSE
						off_addr = framebuffer->address + (off_x) * framebuffer->height * 4 + ( (framebuffer->height - 1 - off_y) << 2);
#else
						off_addr = framebuffer->address + off_x * framebuffer->height * 4 + (off_y << 2);
#endif
#else
						off_addr = framebuffer->address + off_y * framebuffer->stride + (off_x << 2);
#endif				
						if(framebuffer->format == XM_OSD_LAYER_FORMAT_ARGB888)
						{
							*(unsigned int *)(off_addr) = clr | 0xFF000000;
						}
						else if(framebuffer->format == XM_OSD_LAYER_FORMAT_RGB565)
						{
							*(unsigned short *)(off_addr) = (unsigned short)clr;
						}

					}
				}	// if( (pDeCode->dy & pDeCode->scale_mask) == 0)

				pDeCode->dx ++;
				if(pDeCode->dx == pDeCode->rx)
				{
					pDeCode->dx = pDeCode->lx;
					pDeCode->dy ++;
				}

			}	// while( work_stack > stack_top )
		
			// 此处将像素写入过程的所有中间变量保存到结构体中
		}

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

static int _GetOneByte(XMMAPFILE * pFile)
{
	BYTE byte = 0;
	if(XM_MapFileRead (pFile, &byte, 1) != 1)
		return -1;
	return  byte;
}

//---越过一系列数据
static void _PassData (XMMAPFILE * pFile)
{
	int	nBytes = _GetOneByte(pFile);

	while( nBytes > 0 )
	{
		XM_MapFileSeek (pFile, nBytes, XM_SEEK_CUR);
		nBytes = _GetOneByte (pFile);
	}
}


XMGIF*	XM_GifOpen (XMMAPFILE *pFile)
{
	int	nReturn;
	XMGIF *pGifInfo = &GifInfo;
	
	if(pFile == NULL || pGifInfo == NULL)
		return NULL;

	int_memset (pGifInfo, 0, sizeof(XMGIF));

	pGifInfo->gifFile = pFile;

	pGifInfo->unNowPicture = 1;	//默认显示第一幅图片

	nReturn = _CheckGifAndGetBasicInfo (pGifInfo);
	if( nReturn != IMG_OPER_SUCCESS )
	{
		return NULL;
	}

	pGifInfo->gifContext.nFirstOffset = XM_MapFileTell (pGifInfo->gifFile);

	//是设置一个默认的
	pGifInfo->gifContext.usDelayTime = 0;
	pGifInfo->gifContext.usNeedBrushBk = 0;
	pGifInfo->gifContext.usTransColor = 0xFFF;

	pGifInfo->usPictureCount = 0;
	
	return pGifInfo;
}

int	XM_GifClose (XMGIF *pGifInfo)
{
	if(pGifInfo == NULL)
		return (-1);

	return IMG_OPER_SUCCESS;
}

static int _GetGifTotalPicture (XMGIF * pGifInfo)
{
	XMMAPFILE* pFile = pGifInfo->gifFile;
	int	nCharacter;
	LONG	nPicCount = 0;	

	for( ; ;)
	{
		nCharacter = _GetOneByte(pFile);
		if(nCharacter < 0)
			return (-1);

		if( nCharacter == ',' )
		{
			UCHAR	ucTempBuffer[10];

			++nPicCount;

			// pass picture...
			if( XM_MapFileRead (pFile, ucTempBuffer, 10) != 10 )
				return (-1);
			if( ucTempBuffer[8] & 0x80 )
			{
				int unColorTable = ucTempBuffer[8] & 0x07;
				int unMapSize = (1<<(unColorTable+1)) * 3;
				XM_MapFileSeek (pFile, unMapSize, XM_SEEK_CUR);
			}
			_PassData (pFile);
		}
		else if( nCharacter == '!' )// ingore all comment
		{
			if( pGifInfo->usVersion/* == VERSION_89*/ )
			{
				if( _GetOneByte(pFile) == 0xF9 )
				{
					int	nLen = _GetOneByte(pFile);
					XM_MapFileSeek (pFile, nLen, XM_SEEK_CUR);
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
			// 到最后了,
			break;
		}
		else if( nCharacter == 0 )
			continue;
		else
		{
			continue;
		}
	}

	return nPicCount;
}

int XM_GifGetFrameCount(XMGIF * pGifInfo)
{
	LONG			pos;
	int			ret;

	// 保存当前文件偏移
	pos = XM_MapFileTell (pGifInfo->gifFile);
	if(pos < 0)
		return (-1);

	XM_MapFileSeek (pGifInfo->gifFile, pGifInfo->gifContext.nFirstOffset, XM_SEEK_SET);

	ret = _GetGifTotalPicture(pGifInfo);

	// 重置当前文件偏移
	XM_MapFileSeek (pGifInfo->gifFile, pos, XM_SEEK_SET);

	return ret;
}

int XM_GifRenderFrame (XMGIF * pGifInfo, HANDLE hWnd, XMRECT *lpRect)
{
	UCHAR	ucBuf[262+2];		// 4字节对齐
	ULONG *prefix_code;
	UCHAR *ucBuffer;

	XMMAPFILE*	pFile = pGifInfo->gifFile;
	int	y, nReturn;
	int	ret;
	USHORT	usLastBytes;
	DE_CODE	deCode;
	PGIF_COLORMAP	pColorMap;

	int ly;

	xm_osd_framebuffer_t framebuffer = XM_GetWindowFrameBuffer (hWnd);

	prefix_code = pGifInfo->prefix_code;
	ucBuffer = ucBuf+4;


	if(pGifInfo->imgData.usHaveColorMap)	// 存在局部颜色表
		pColorMap = &pGifInfo->imgData.localColorMap;
	else	// 使用缺省的全局颜色表
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

	deCode.scale = (USHORT)pGifInfo->nScale;
	if(deCode.scale == 0)
		deCode.scale_mask = 0x0000;
	else
		deCode.scale_mask = (USHORT)(~(0xFFFF << deCode.scale));

	deCode.lx = (XMCOORD)(pGifInfo->imgData.usPosLeft + (lpRect->left << pGifInfo->nScale));
	deCode.rx = (XMCOORD)(deCode.lx + pGifInfo->imgData.usNowWidth);
	deCode.dx = deCode.lx;
	deCode.dy = (XMCOORD)(pGifInfo->imgData.usPosTop + (lpRect->top << pGifInfo->nScale));

	deCode.trans_color = pGifInfo->gifContext.usTransColor;

	deCode.gif_cache_sx = 0;
	deCode.gif_cache_sy = 0;
	deCode.gif_cache_lx = 0;
	deCode.gif_cache_count = 0;


	ly = deCode.dy + pGifInfo->imgData.usNowHeight;

	ucBuf[264 - 1] = 0;
	ucBuf[264 - 2] = 0;

#if defined(_XM_RGB16BPP_DEVICE_)
	deCode.MapData = (WORD *)pColorMap->MapData;
#elif defined(_XM_ARGB32BPP_DEVICE_)
	deCode.MapData = (DWORD *)pColorMap->MapData;
#endif

	if( pGifInfo->imgData.usInterlace )
	//交错图---------------------------------------------------------------
	{
		// 不支持交错图
		return IMG_BAD_FILE;
	}
	//-------------------------------------------------------------------------------------
	else	/* 非交错图 */
	{
		while ( (int)deCode.dy < ly )
		{
			/* 解数据 */
			nReturn = _OutputGIF (&deCode, prefix_code, framebuffer);
			if( nReturn == 0 )
			{
			}
			else if( nReturn == 1 )
			{
				// 读取解码流数据
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

//执行此 函数前，必须已经读完了 Global Color Map, 文件已经定位到数据块
//而且此函数 对 文件读 依赖性很高...
/*
	; 表示文件结束了
	, 表示图形信息
	! 表示注释信息
*/
// 解码GIF图形的当前帧控制数据
int XM_GifDecodeFrame (XMGIF * pGifInfo, BYTE bDecodeFlag)
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
			// 读取当前帧的位置信息
			if( XM_MapFileRead (pGifInfo->gifFile, ucTempBuff, 9) != 9 )
				return IMG_BAD_FILE;
			pImgData->usPosLeft = (USHORT)BYTE_TO_SHORT(ucTempBuff[0], ucTempBuff[1]);
			pImgData->usPosTop = (USHORT)BYTE_TO_SHORT(ucTempBuff[2], ucTempBuff[3]);
			pImgData->usNowWidth = (USHORT)BYTE_TO_SHORT(ucTempBuff[4], ucTempBuff[5]);
			pImgData->usNowHeight = (USHORT)BYTE_TO_SHORT(ucTempBuff[6], ucTempBuff[7]);
			
			if( ucTempBuff[8] & 0x80 )
			{
				// 读取局部颜色表
				int unColorTable = ucTempBuff[8] & 0x07;

				unColorTable =  1 << (unColorTable+1);
				//_memset (pImgData->localColorMap.MapData, 0, 256*2);
				//_read_color_map (pGifInfo->gifFile, (WORD*)pImgData->localColorMap.MapData, unColorTable);
#if defined(_XM_RGB16BPP_DEVICE_)
				_read_color_map (pFile, (WORD*)pImgData->localColorMap.MapData, unColorTable);
#else
				_read_color_map (pGifInfo->gifFile, (DWORD*)pImgData->localColorMap.MapData, unColorTable);
#endif
				pImgData->localColorMap.unMapSize = (WORD)(unColorTable << 1);

				pImgData->usHaveColorMap = 1;
			}
			pImgData->usInterlace = (USHORT)((ucTempBuff[8] & 0x40) ? 1 : 0);
			if(pImgData->usInterlace)
			{
				// 不支持交叉格式
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
					if( ucControl & 0x01 )	//有透明色
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



