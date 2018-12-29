//****************************************************************************
//
//	Copyright (C) 2012 Zhuo YongHong
//
//	Author	ZhuoYongHong
//
//	File name: xm_file.c
//	  constant，macro & basic typedef definition of file
//
//	Revision history
//
//		2011.05.26	ZhuoYongHong Initial version
//
//****************************************************************************
#include <xm_type.h>
#include <xm_base.h>
#include <xm_file.h>
#include <string.h>
#include <xm_printf.h>

XMBOOL	XM_MapFileOpen (const VOID *base, LONG size, XMMAPFILE *lpMapFile)
{
	if(lpMapFile == NULL || base == NULL || size <= 0)
		return FALSE;

	lpMapFile->id = XM_MAPFILEID;
	lpMapFile->base = base;
	lpMapFile->size = size;
	lpMapFile->offset = 0;
	return TRUE;
}

#ifndef _ARM_ASM_
void map_memcpy (BYTE *dst, const BYTE *src, int len)
{
	if(dst == NULL || src == NULL || len == 0)
		return;
	if((unsigned int)src & 0x01)
	{
		*dst = (BYTE)*src;
		dst ++;
		src ++;
		len --;
	}
	if((unsigned int)src & 0x02 && len >= 2)
	{
		USHORT us = *(USHORT *)src;
		src += 2;
		*dst ++ = (BYTE)us;
		*dst ++ = (BYTE)(us >> 8);
		len -= 2;
	}
	while(len >= 4)
	{
		ULONG ul = *(ULONG *)src;
		src += 4;
		
		if( ((unsigned int)dst) & 3 )
		{
			if( ((unsigned int)dst) & 1 )
			{
				*dst ++ = (BYTE)ul;
				*(USHORT *)dst = (USHORT)(ul >> 8);
				dst += 2;
				*dst ++ = (BYTE)(ul >> 24);
			}
			else
			{
				*(USHORT *)dst = (USHORT)ul;
				dst += 2;
				*(USHORT *)dst = (USHORT)(ul >> 16);
				dst += 2;
			}
		}
		else
		{
			*(ULONG *)dst = ul;
			dst += 4;
		}
		/*
		*dst ++ = (char)ul;
		*dst ++ = (char)(ul >> 8);
		*dst ++ = (char)(ul >> 16);
		*dst ++ = (char)(ul >> 24);*/
		len -= 4;
	}


	while(len)
	{
		*dst = (BYTE)*src;
		dst ++;
		src ++;
		len --;
	}
}
#else
extern void map_memcpy (BYTE *dst, const BYTE *src, int len);
#endif

// 从内存映射文件当前读指针位置读取字节数为cbBuffer的内容到缓冲区lpBuffer
// 返回值 < 0    表示错误
//        = 0    表示达到文件尾部
//        其他值 表示读取的字节数
LONG		XM_MapFileRead	(XMMAPFILE *lpMapFile, VOID *lpBuffer, LONG cbBuffer)
{
	LONG to_read;
	if(lpMapFile == NULL || lpMapFile->id != XM_MAPFILEID)
		return (-1);
	to_read = lpMapFile->size - lpMapFile->offset;
	if(to_read > cbBuffer)
		to_read = cbBuffer;
	if(to_read)
	{
		map_memcpy ((BYTE *)lpBuffer, ((const BYTE *)lpMapFile->base) + lpMapFile->offset, to_read);
		lpMapFile->offset += to_read;
	}
	return to_read;
}

// 将读指针定位到指定的位置,
// 返回值 < 0		表示错误
//        其他值	设置前的读指针位置
LONG		XM_MapFileSeek (XMMAPFILE *lpMapFile, LONG offset, LONG whence)
{
	LONG ret;
	LONG pos;
	if(lpMapFile == NULL || lpMapFile->id != XM_MAPFILEID)
		return (-1);
	if(whence == XM_SEEK_SET)
	{
		pos = offset;
	}
	else if(whence == XM_SEEK_CUR)
	{
		pos = lpMapFile->offset + offset;
	}
	else if(whence == XM_SEEK_END)
	{
		pos = lpMapFile->size + offset;
	}
	else
	{
		return (-1);
	}
	
	if(pos < 0 || pos > lpMapFile->size)
		return (-1);

	ret = lpMapFile->offset;
	lpMapFile->offset = pos;

	return ret;
}

// 获取内存映射文件当前的读指针
LONG		XM_MapFileTell	(XMMAPFILE *lpMapFile)
{
	if(lpMapFile == NULL || lpMapFile->id != XM_MAPFILEID)
		return (-1);
	return lpMapFile->offset;
}

// 关闭内存映射文件
XMBOOL	XM_MapFileClose (XMMAPFILE *lpMapFile)
{
	if(lpMapFile == NULL || lpMapFile->id != XM_MAPFILEID)
		return FALSE;
	lpMapFile->id = 0;
	return TRUE;
}

