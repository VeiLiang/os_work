//****************************************************************************
//
//	Copyright (C) 2012 ZhuoYongHong
//
//	Author	ZhuoYongHong
//
//	File name: app_videolist.h
//	  ��Ƶ�б�ӿ�
//
//	Revision history
//
//		2012.09.06	ZhuoYongHong Initial version
//
//****************************************************************************

#ifndef _XM_APP_VIDEOLIST_H_
#define _XM_APP_VIDEOLIST_H_

#if defined (__cplusplus)
	extern "C"{
#endif

// ��Ƶ���Ŀ¼
#define	APP_ROOTPATH	"e:"
#define	APP_VIDEOPATH	"\\VIDEO_F"

#if defined(PCVER)

#if _XM_FFMPEG_CODEC_ == _XM_FFMPEG_CODEC_MKV_
#define	APP_VIDEOFILE	"\\VIDEO_F\\*.MKV"
#elif _XM_FFMPEG_CODEC_ == _XM_FFMPEG_CODEC_AVI_
#define	APP_VIDEOFILE	"\\VIDEO_F\\*.AVI"
#endif

#else
#define	APP_VIDEOFILE	"\\VIDEO_F\\"
#endif

// \video\20120930084512.mp4
#define	MAX_APP_VIDEOPATH				26	



// ɨ��ָ��ģʽ���е���Ƶ�ļ�
void AP_VideoListScanVideoFileList (XMFILEFIND *fileFind, BYTE mode);

// ��ȡ��Ƶ�б��ļ�����
DWORD AP_VideoListGetVideoFileCount (VOID);

// ��ȡ��Ƶ�б�(ѭ���򱣻���Ƶ�б�)��ָ����ŵ���Ƶ�ļ���
char *AP_VideoListGetVideoFileName (DWORD dwVideoIndex);

// ��ȡ��Ƶ�б�(ѭ���򱣻���Ƶ�б�)��ָ����ŵ���Ƶ�ļ����ļ�����
DWORD AP_VideoListGetVideoFileAttribute (DWORD dwVideoIndex);

// ��ʾ��ʽ��ָ����ŵ���Ƶ�ļ���
char *AP_VideoListFormatVideoFileName (DWORD dwVideoIndex, char *lpDisplayName, int cbDisplayName);

// ����Ƶ�б�(ѭ���򱣻���Ƶ�б�)��ָ����ŵ���Ƶ�ļ�����(����)
XMBOOL AP_VideoListLockVideoFile (DWORD dwVideoIndex);

// ����Ƶ�б�(ѭ���򱣻���Ƶ�б�)��ָ����ŵ���Ƶ�ļ�����(��ѭ������)
XMBOOL AP_VideoListUnlockVideoFile (DWORD dwVideoIndex);

// ��ȡǰһ����Ƶ������, -1��ʾ������
DWORD AP_VideoListGetPrevVideoIndex (DWORD dwVideoIndex);

// ��ȡ��һ����Ƶ������, -1��ʾ������
DWORD AP_VideoListGetNextVideoIndex (DWORD dwVideoIndex);

#if defined (__cplusplus)
	}
#endif		/* end of __cplusplus */


#endif	// #ifndef _XM_APP_VIDEOLIST_H_
