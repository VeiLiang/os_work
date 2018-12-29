//****************************************************************************
//
//	Copyright (C) 2012 ShenZhen ExceedSpace
//
//	Author	ZhuoYongHong
//
//	File name: xm_osd_layer_animate.h
//	  constant，macro & basic typedef definition of animate interface of OSD layer
//
//	Revision history
//
//		2014.02.27	ZhuoYongHong Initial version
//
//****************************************************************************
#ifndef _XM_OSD_LAYER_ANIMATE_H_
#define _XM_OSD_LAYER_ANIMATE_H_

#if defined (__cplusplus)
	extern "C"{
#endif

// OSD Layer移动方向定义
#define	XM_OSDLAYER_MOVING_FROM_LEFT_TO_RIGHT		1
#define	XM_OSDLAYER_MOVING_FROM_RIGHT_TO_LEFT		2
#define	XM_OSDLAYER_MOVING_FROM_TOP_TO_BOTTOM		3
#define	XM_OSDLAYER_MOVING_FROM_BOTTOM_TO_TOP		4

// OSD Layer动画模式定义
#define	XM_OSDLAYER_ANIMATE_MODE_VIEW_CREATE		1		// 新视窗创建 (自底向上移出)
#define	XM_OSDLAYER_ANIMATE_MODE_VIEW_REMOVE		2		// 视窗弹出   (自顶向下收起)
#define	XM_OSDLAYER_ANIMATE_MODE_ALERT_CREATE		3		// 对话框创建 (透明效果)
#define	XM_OSDLAYER_ANIMATE_MODE_ALERT_REMOVE		4		// 对话框弹出 (透明效果)
#define	XM_OSDLAYER_ANIMATE_MODE_EVENT_CREATE		5		// 消息窗创建 (自顶向下移出)
#define	XM_OSDLAYER_ANIMATE_MODE_EVENT_REMOVE		6		// 消息窗弹出 (自底向上收起)



// OSD层视窗移动 (单线程支持)
void XM_osd_layer_animate (
									unsigned int animating_mode,		// OSD移动模式
									unsigned int moving_direction,	// 移动方向
									HANDLE old_top_view,					// 老的顶视图句柄
									HANDLE new_top_view					// 新的顶视图句柄
									);

#if defined (__cplusplus)
	}
#endif		/* end of __cplusplus */

#endif	// _XM_OSD_LAYER_ANIMATE_H_
