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

#ifndef _XM_OSD_LAYER_GIF_H_
#define _XM_OSD_LAYER_GIF_H_

#include <xm_type.h>

#if defined (__cplusplus)
	extern "C"{
#endif

// �궨��
// �汾
#define	VERSION_87			1
#define	VERSION_89			2

#define	MAX_IMAGE			4096
#define	NO_TRANS_COLOR		0xFFF

#define	MAX_LZW_BITS	12
#define	MAX_LZW_TABLE	(1<<MAX_LZW_BITS)

//ͼ�β����ķ���ֱ(������)����-----------------------------------------------------------------
#define	IMG_OPER_SUCCESS			0x00000000	/*	�ɹ�	*/
#define	IMG_OPER_FINISH			0x00000001
#define	IMG_OTHER_ERROR			0xFFFFFFFF
#define	IMG_NO_MEMORY				0xFFFFFFF0
#define	IMG_BAD_FILE				0xFFFFFFF1
#define	IMG_BAD_HANDLE				0xFFFFFFF2
#define	IMG_BAD_PARM				0xFFFFFFF3
#define	IMG_WRONG_TYPE				0xFFFFFFF4
#define	IMG_BAD_COMMAND			0xFFFFFFF5
#define	IMG_IMAGE_TOO_LARGE		0xFFFFFFF6

#define	GIF_READ_ON_MEMORY	0x00000010
#define	GIF_READ_ONE_BREAK	0x00000001
#define	GIF_READ_AUTO			0x00000008
#define	GIF_CREATE_DECODE		0x08000000


#define	XMGIF_FLAG_DECODE_NORMAL	1	// �ӵ�ǰλ�ý���
#define	XMGIF_FLAG_DECODE_BEGIN		2	// ����ʼ֡��ʼ����

#define	XMGIF_DRAW_POS_LEFTTOP		1	// ����ʾ��������Ͻ�λ�ÿ�ʼ��ʾ
#define	XMGIF_DRAW_POS_CENTER		2	// ��ʾ���������ʾ


typedef struct tagGIF_COLORMAP
{
	WORD		unMapSize;
	BYTE		MapData[256*4];	// GIFÿ��ͼƬ�������256����ɫ
} GIF_COLORMAP, *PGIF_COLORMAP;

typedef struct tagGIF_IMGDATA
{
	USHORT	usNowWidth;
	USHORT	usNowHeight;
	USHORT	usPosLeft;
	USHORT	usPosTop;

	USHORT	usInterlace;
	USHORT	usHaveColorMap;
	GIF_COLORMAP	localColorMap;

}GIF_IMGDATA, *PGIF_IMGDATA;

typedef struct tagGIF_IMAINFO
{
	int		nFirstOffset;
	USHORT	usDelayTime;
	USHORT	usTransColor;
	USHORT	usNeedBrushBk;
}GIF_IMAINFO, *PGIF_IMAINFO;

typedef struct tagXMGIF
{
	USHORT	usVersion;			// GIF version
	USHORT	usPictureCount;	// the Sum of GIF-File's picture;
	USHORT	usSreenWidth;		// the max width of all GIF-File's picture;
	USHORT	usSreenHeight;

	//int		unNowPicture;		//��ǰ��ʾ��
	
	GIF_COLORMAP	globalColorMap;		// ����ȫ����ɫ��

	GIF_IMGDATA		imgData;
	XMMAPFILE *		gifFile;

	//int				nScale;

	GIF_IMAINFO		gifContext;

	ULONG				prefix_code[MAX_LZW_TABLE];
	BYTE				work_stack[MAX_LZW_TABLE];
} XMGIF;

// �����ⲿ��������






#if defined (__cplusplus)
	}
#endif		/* end of __cplusplus */

#endif	// #ifndef _XM_OSD_LAYER_GIF_H_
