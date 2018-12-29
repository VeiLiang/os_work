//****************************************************************************
//
//	Copyright (C) 2012 ShenZhen ExceedSpace
//
//	Author	ZhuoYongHong
//
//	File name: xmwidget.h
//	  constant，macro & basic typedef definition of widget
//
//	Revision history
//
//		2010.09.10	ZhuoYongHong Initial version
//
//****************************************************************************
#ifndef _XM_WIDGET_H_
#define _XM_WIDGET_H_

#include <xm_type.h>

// 定义控件总个数
#define	XM_WIDGET_COUNT			16

// 定义控件WIDGET的类型
#define	XM_WIDGET_NULL				0x00			// 
#define	XM_WIDGET_BUTTON			0x01			// 按钮		
#define	XM_WIDGET_CHECKBOX		0x02			// 复选框
#define	XM_WIDGET_RADIO			0x03			// 圆形按钮
#define	XM_WIDGET_SLIDER			0x04			// 滑行控件
#define	XM_WIDGET_LIST				0x05			// 列表
#define	XM_WIDGET_SCROLLBAR		0x06			// 滚动条
#define	XM_WIDGET_IMAGE			0x07			// 图片控件
#define	XM_WIDGET_EDIT				0x08			// 单行编辑框
#define	XM_WIDGET_STATIC			0x09			// 静态文本显示框
#define	XM_WIDGET_TITLE			0x0A			// 普通视窗标题栏控件(显示标题、时间、电池等信息)
#define	XM_WIDGET_IME				0x0B			// 输入法控件
#define	XM_WIDGET_INPUTBAR		0x0C			// 单行输入编辑框(字典使用)

// 控件用户数据结构定义

// 子控件“标题”的用户数据结构
typedef struct tagXM_TITLEUSERDATA {
	WCHAR *lpTitle;	// 标题串的指针
	CHAR	cbTitle;		// 标题串的字符个数
} XM_TITLEUSERDATA;

// 子控件“滑动条”的用户数据结构
typedef struct tagXM_SLIDERUSERDATA {
	XMCOORD ticks;		// 刻度总数
	XMCOORD cursor;		// 游标位置
} XM_SLIDERUSERDATA;


// 控件消息处理函数外部声明
XMWDGTPROC_DECLARE(BUTTON)
XMWDGTPROC_DECLARE(CHECKBOX)
XMWDGTPROC_DECLARE(RADIO)
XMWDGTPROC_DECLARE(SLIDER)
XMWDGTPROC_DECLARE(LIST)
XMWDGTPROC_DECLARE(SCROLLBAR)
XMWDGTPROC_DECLARE(IMAGE)
XMWDGTPROC_DECLARE(EDIT)
XMWDGTPROC_DECLARE(STATIC)
XMWDGTPROC_DECLARE(TITLE)
XMWDGTPROC_DECLARE(IME)
XMWDGTPROC_DECLARE(INPUTBAR)

// 调用控件的消息处理函数
// bWidgetType 控件WIDGET的类型
// hWidget  控件的句柄
VOID XM_WidgetProc (const XMWDGT *pWidget, BYTE bWidgetFlag, VOID *pUserData, XMMSG *msg);

// 设置Button控件的文本并使用控件当前状态进行显示，因此需要在控件显示前将控件状态正确设置
VOID XM_ButtonSetText (HANDLE hWidget, BYTE bWidgetFlag, WCHAR *lpText, BYTE cbText);

VOID XM_CheckBoxSetText (HANDLE hWidget, BYTE bWidgetFlag, WCHAR *lpText, BYTE cbText);

VOID XM_RadioSetText (HANDLE hWidget, BYTE bWidgetFlag, WCHAR *lpText, BYTE cbText);

VOID XM_SliderSetText (HANDLE hWidget, BYTE bWidgetFlag, WCHAR *lpText, BYTE cbText);

VOID XM_EditSetText (HANDLE hWidget, BYTE bWidgetFlag, WCHAR *lpText, BYTE cbText);

VOID XM_StaticSetText (HANDLE hWidget, BYTE bWidgetFlag, WCHAR *lpText, BYTE cbText);

VOID XM_TitleSetText (HANDLE hWidget, BYTE bWidgetFlag, WCHAR *lpText, BYTE cbText);

// 设置视窗中控件的状态
XMBOOL XM_SetWidgetFlag (BYTE bWidgetIndex, BYTE bWidgetFlag);

// 获取视窗中控件的状态
BYTE XM_GetWidgetFlag (BYTE bWidgetIndex);

// 设置视窗中具有输入焦点的控件，bWidgetIndex为视窗中的控件索引
XMBOOL XM_SetFocus (BYTE bWidgetIndex);

// 获取视窗中具有输入焦点的控件索引
BYTE XM_GetFocus (VOID);

// 设置视窗中控件的选择状态
XMBOOL XM_SetSelect (BYTE bWidgetIndex, XMBOOL bSelect);

// 设置视窗中控件的使能状态
XMBOOL XM_SetEnable (BYTE bWidgetIndex, XMBOOL bEnable);

// 隐藏或显示控件
XMBOOL XM_SetVisual (BYTE bWidgetIndex, XMBOOL bVisual);


#endif	// _XM_WIDGET_H_