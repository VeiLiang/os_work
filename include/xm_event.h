//****************************************************************************
//
//	Copyright (C) 2012 ShenZhen ExceedSpace
//
//	Author	ZhuoYongHong
//
//	File name: xm_event.h
//	  constant，macro & basic typedef definition of event
//
//	Revision history
//
//		2010.09.01	ZhuoYongHong Initial version
//
//****************************************************************************
#ifndef _XM_EVENT_H_
#define _XM_EVENT_H_

#include <xm_type.h>



// "一键拍照"通知事件ID
enum {
	XM_EVENT_ONEKEYPHOTO_CREATE,				// 已创建完成一张一键拍照的照片
																//		用于通知UI层，底层文件发生变化
	XM_EVENT_ONEKEYPHOTO_DELETE,				// 已删除完成一张一键拍照的照片
																//		用于通知UI层，底层文件发生变化
	//XM_APP_MESSAGE_ONEKEYPHOTO_MOREITEMS,		//	一键拍照的照片太多	
}; 

typedef struct tagXM_ONEKEYPHOTO_EVENT {
	unsigned int	event;						// “视频项事件ID”
} XM_ONEKEYPHOTO_EVENT;

// 向系统投递一键拍照通知事件
XMBOOL	XM_PostOneKeyPhotoEvent (UINT OneKeyEvent, 
											 VOID *lpOneKeyEventParam, 
											 UINT cbOneKeyEventParam);


// 向系统投递视频项通知的“视频项事件ID”
enum {
	XM_EVENT_VIDEOITEM_CREATE,					// 已创建完成一个新的视频记录文件
	XM_EVENT_VIDEOITEM_DELETE,					// 已删除完成一个视频记录文件
};

typedef struct tagXM_VIDEOITEM_EVENT {
	unsigned int	event;						// “视频项事件ID”
	unsigned char	channel;						// 触发事件的通道号
	unsigned char	file_name[32];				// 视频项文件名
} XM_VIDEOITEM_EVENT;

XMBOOL	XM_PostVideoItemEvent (UINT VideoItemEvent, 
										  VOID *lpVideoItemEventParam, 
										  UINT cbVideoItemEventParam);


#endif	// _XM_EVENT_H_
