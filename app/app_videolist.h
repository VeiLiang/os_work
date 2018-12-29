//****************************************************************************
//
//	Copyright (C) 2012 ZhuoYongHong
//
//	Author	ZhuoYongHong
//
//	File name: app_videolist.h
//	  视频列表接口
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

// 视频浏览目录
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



// 扫描指定模式所有的视频文件
void AP_VideoListScanVideoFileList (XMFILEFIND *fileFind, BYTE mode);

// 获取视频列表文件个数
DWORD AP_VideoListGetVideoFileCount (VOID);

// 获取视频列表(循环或保护视频列表)中指定序号的视频文件名
char *AP_VideoListGetVideoFileName (DWORD dwVideoIndex);

// 获取视频列表(循环或保护视频列表)中指定序号的视频文件的文件属性
DWORD AP_VideoListGetVideoFileAttribute (DWORD dwVideoIndex);

// 显示格式化指定序号的视频文件名
char *AP_VideoListFormatVideoFileName (DWORD dwVideoIndex, char *lpDisplayName, int cbDisplayName);

// 将视频列表(循环或保护视频列表)中指定序号的视频文件锁定(保护)
XMBOOL AP_VideoListLockVideoFile (DWORD dwVideoIndex);

// 将视频列表(循环或保护视频列表)中指定序号的视频文件解锁(可循环覆盖)
XMBOOL AP_VideoListUnlockVideoFile (DWORD dwVideoIndex);

// 获取前一个视频索引号, -1表示不存在
DWORD AP_VideoListGetPrevVideoIndex (DWORD dwVideoIndex);

// 获取下一个视频索引号, -1表示不存在
DWORD AP_VideoListGetNextVideoIndex (DWORD dwVideoIndex);

#if defined (__cplusplus)
	}
#endif		/* end of __cplusplus */


#endif	// #ifndef _XM_APP_VIDEOLIST_H_
