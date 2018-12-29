//****************************************************************************
//
//	Copyright (C) 2012 Zhuo YongHong
//
//	Author	ZhuoYongHong
//
//	File name: xm_file.c
//	  constant��macro & basic typedef definition of file
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

// ���ڴ�ӳ���ļ���ǰ��ָ��λ�ö�ȡ�ֽ���ΪcbBuffer�����ݵ�������lpBuffer
// ����ֵ < 0    ��ʾ����
//        = 0    ��ʾ�ﵽ�ļ�β��
//        ����ֵ ��ʾ��ȡ���ֽ���
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

// ����ָ�붨λ��ָ����λ��,
// ����ֵ < 0		��ʾ����
//        ����ֵ	����ǰ�Ķ�ָ��λ��
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

// ��ȡ�ڴ�ӳ���ļ���ǰ�Ķ�ָ��
LONG		XM_MapFileTell	(XMMAPFILE *lpMapFile)
{
	if(lpMapFile == NULL || lpMapFile->id != XM_MAPFILEID)
		return (-1);
	return lpMapFile->offset;
}

// �ر��ڴ�ӳ���ļ�
XMBOOL	XM_MapFileClose (XMMAPFILE *lpMapFile)
{
	if(lpMapFile == NULL || lpMapFile->id != XM_MAPFILEID)
		return FALSE;
	lpMapFile->id = 0;
	return TRUE;
}

