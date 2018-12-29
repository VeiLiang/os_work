//****************************************************************************
//
//	Copyright (C) 2012 ShenZhen ExceedSpace
//
//	Author	ZhuoYongHong
//
//	File name: xmgdi.h
//	  constant，macro, data structure，function protocol definition of X-Mini's GDI 
//
//	Revision history
//
//		2010.08.31	ZhuoYongHong Initial version
//
//****************************************************************************
#ifndef _XM_GDI_H_
#define _XM_GDI_H_

#include <xm_type.h>
#include "hw_osd_layer.h"
#include "xm_osd_layer.h"
#include "xm_osd_framebuffer.h"

// 宏定义

// 系统颜色数定义
#define XM_TOTAL_SYSTEM_COLOR        22

// 系统颜色定义
#define XM_COLOR_SCROLLBAR           0
#define XM_COLOR_BACKGROUND          1
#define XM_COLOR_ACTIVECAPTION       2
#define XM_COLOR_INACTIVECAPTION     3
#define XM_COLOR_MENU                4
#define XM_COLOR_WINDOW              5
#define XM_COLOR_WINDOWFRAME         6
#define XM_COLOR_MENUTEXT            7
#define XM_COLOR_WINDOWTEXT          8
#define XM_COLOR_CAPTIONTEXT         9	
#define XM_COLOR_ACTIVEBORDER        10
#define XM_COLOR_INACTIVEBORDER      11
#define XM_COLOR_HIGHLIGHT           12
#define XM_COLOR_HIGHLIGHTTEXT       13
#define XM_COLOR_BTNFACE             14
#define XM_COLOR_BTNSHADOW           15
#define XM_COLOR_GRAYTEXT            16
#define XM_COLOR_BTNTEXT             17
#define XM_COLOR_INACTIVECAPTIONTEXT 18
#define XM_COLOR_BTNHIGHLIGHT        19
#define XM_COLOR_3DDKSHADOW          20
#define XM_COLOR_3DLIGHT             21

#define XM_COLOR_DESKTOP            XM_COLOR_3DDKSHADOW// XM_COLOR_BACKGROUND
#define XM_COLOR_3DFACE              XM_COLOR_BTNFACE
#define XM_COLOR_3DSHADOW            XM_COLOR_BTNSHADOW
#define XM_COLOR_3DHIGHLIGHT         XM_COLOR_BTNHIGHLIGHT
#define XM_COLOR_3DHILIGHT           XM_COLOR_BTNHIGHLIGHT
#define XM_COLOR_BTNHILIGHT          XM_COLOR_BTNHIGHLIGHT

#define XM_COLOR_WHITE               (XMCOLOR)(0x8000000F)
#define XM_COLOR_BLACK               (XMCOLOR)(0x80000000)

// XMCOLOR转换到RGB565格式
#define COLOR2RGB565(c)					((WORD)((((c) & 0xf8) << 8) | (((c) & 0xfc00) >> 5) | (((c) & 0xf80000) >> 19)))
//#define COLOR2RGB565(c)					((WORD)((((c) & 0xf8) >> 3) | (((c) & 0xfc00) >> 5) | (((c) & 0xf80000) >> 8)))
#define COLOR2ARGB(c)					((DWORD)((((c) & 0xff) << 16) | (((c) & 0xff00) ) | (((c) & 0xff0000) >> 16)))


// RGB三原色转换到XMCOLOR格式
//#define RGB(r,g,b)						((XMCOLOR)(((BYTE)(r) | ((XMCOLOR)((BYTE)(g))<<8)) | (((XMCOLOR)(BYTE)(b))<<16)))

#define RGB8882RGB565(r,g,b)			((WORD)(((r & 0xf8) << 8) | ((g & 0xfc) << 3) | (b >> 3)))
#define RGB8882ARGB32(r,g,b)			((DWORD)((r  << 16) | (g << 8) | b ))



// 字体风格
#define	XM_FONT_STYLE_NORMAL			0
#define	XM_FONT_STYLE_BOLD			1
#define	XM_FONT_STYLE_ITALIC			2
#define	XM_FONT_STYLE_BOLDITALIC	3

// 字体大小
#define	XM_FONT_SIZE_12				0
#define	XM_FONT_SIZE_16				1
#define	XM_FONT_SIZE_24				2

/* Text alignment flags*/		
#define	XMTF_TOP							0x10	/* align on top*/
#define	XMTF_BASELINE					0x20	/* align on baseline*/
#define	XMTF_BOTTOM						0x40	/* align on bottom*/

// Converting 8-bit YUV to RGB888
void	XM_RGB2YUV (BYTE R, BYTE G, BYTE B, BYTE *Y, BYTE *U, BYTE *V);

// conversion from YUV to RGB
void	XM_YUV2RGB (BYTE Y, BYTE U, BYTE V, BYTE *R, BYTE *G, BYTE *B);

// function protocol definition

// 获取系统颜色
XMCOLOR	XM_GetSysColor	(BYTE index);

// 设置字符显示的文本颜色; 返回值为原来设置的颜色
XMCOLOR XM_SetTextColor (XMCOLOR color);

// 设置字符显示的背景颜色
XMCOLOR XM_SetBkColor (XMCOLOR color);

// 获取当前字符显示的文本颜色
XMCOLOR XM_GetTextColor (void);

// 获取当前字符显示的背景颜色
XMCOLOR XM_GetBkColor (void);


// 画点
// hWnd 视窗或控件的句柄
// hWnd == NULL, x,y坐标为绝对坐标
// hWnd != NULL, x,y坐标为相对于视窗左上角原点的坐标
void  XM_SetPixel  (HANDLE hWnd, XMCOORD x, XMCOORD y, XMCOLOR color);

// 画线
// hWnd 视窗或控件的句柄
// hWnd == NULL, x1,y1,x2,y2坐标为绝对坐标
// hWnd != NULL, x1,y1,x2,y2坐标为相对于视窗左上角原点的坐标
void XM_DrawLine (
	HANDLE hWnd,
	XMCOORD x1, 
	XMCOORD y1, 
	XMCOORD x2, 
	XMCOORD y2,
	XMCOLOR color
);

// 画横线(点划线、虚线、实线绘制)
// hWnd 视窗或控件的句柄
// hWnd == NULL, x,y坐标为绝对坐标
// hWnd != NULL, x,y坐标为相对于视窗左上角原点的坐标
void XM_DrawHLine (
	HANDLE hWnd,
	XMCOORD x, 
	XMCOORD y, 
	XMCOORD size, 
	BYTE mask,
	XMCOLOR color
);

// 画竖线(点划线、虚线、实线绘制)
// hWnd 视窗或控件的句柄
// hWnd == NULL, x,y坐标为绝对坐标
// hWnd != NULL, x,y坐标为相对于视窗左上角原点的坐标
void XM_DrawVLine (
	HANDLE hWnd,
	XMCOORD x, 
	XMCOORD y, 
	XMCOORD size, 
	BYTE mask,
	XMCOLOR color
);

// 使用指定的颜色填充矩形
// hWnd 视窗或控件的句柄
// hWnd == NULL, 矩形坐标为绝对坐标
// hWnd != NULL, 矩形坐标为相对于视窗左上角原点的坐标
void XM_FillRect (HANDLE hWnd, XMCOORD left, XMCOORD top, XMCOORD right, XMCOORD bottom, XMCOLOR color);

// 画矩形边框
// hWnd 视窗或控件的句柄
// hWnd == NULL, 矩形坐标为绝对坐标
// hWnd != NULL, 矩形坐标为相对于视窗左上角原点的坐标
void XM_Rectangle (HANDLE hWnd, XMCOORD left, XMCOORD top, XMCOORD right, XMCOORD bottom, XMCOLOR color);

// 画聚焦边框
// hWnd 视窗或控件的句柄
// hWnd == NULL, 矩形坐标为绝对坐标
// hWnd != NULL, 矩形坐标为相对于视窗左上角原点的坐标
void XM_FocusRect (HANDLE hWnd, XMCOORD left, XMCOORD top, XMCOORD right, XMCOORD bottom, XMCOLOR color);

// 画3D效果的矩形边框
void XM_Draw3DRect (HANDLE hWnd, XMRECT* lpRect, XMCOLOR clrTopLeft, XMCOLOR clrBottomRight);


// 设置字体风格(N/B/I/BI), 返回值为原来的风格
BYTE XM_SetFontStyle	(BYTE style);

// 设置字体大小(12/16/24)，返回值为原来的字体大小。
BYTE XM_SetFontSize	(BYTE size);

VOID XM_SetFontScaleRatio (BYTE ScaleRatio);

BYTE XM_GetFontScaleRatio (VOID);



/************************************************************************************************
 * XM_frame()
 *
 * -- 函数功能:	画区域框。
 *
 * -- 返回值:
 *
 * -- 参数表:
 *    hdc:			DC句柄；
 *    x1,y1:		区域框左上角位置；
 *    x2,y2:		区域框右下角位置；
 *    style:		区域框风格：
 *                      GUI_SIMPLE_FRAME         - 不做显示；
 *                      GUI_FRAME                - 透明突出区域；
 *                      GUI_GRAY_FRAME           - 灰底突出区域；
 *                      GUI_WHITE_FRAME          - 白底突出区域。
 *                      GUI_FLANGE_FRAME         - 厚边框突出区域；
 *                      GUI_SUNKEN_FRAME         - 白底凹陷区域。
 *                      GUI_GRAYSUNKEN_FRAME     - 灰底凹陷区域；
 *                      GUI_WHITESUNKEN_FRAME    - 白底凹陷区域。
 *
 *************************************************************************************************/

#define GUI_SIMPLE_FRAME						0x01
#define GUI_FRAME									0x02
#define GUI_GROUP_FRAME							0x03
#define GUI_GRAY_FRAME							0x04
#define GUI_WHITE_FRAME							0x05
#define GUI_FLANGE_FRAME						0x06
#define GUI_SUNKEN_FRAME						0x07
#define GUI_GRAYSUNKEN_FRAME					0x08
#define GUI_WHITESUNKEN_FRAME					0x09

void XM_line (HANDLE hWnd, XMCOORD x1, XMCOORD y1, XMCOORD x2, XMCOORD y2, BYTE dash, XMCOLOR color);
void XM_frame (HANDLE hWnd, XMCOORD x1, XMCOORD y1, XMCOORD x2, XMCOORD y2, BYTE style);
void XM_rectangle (HANDLE hWnd, XMCOORD x1, XMCOORD y1, XMCOORD x2, XMCOORD y2, XMCOLOR pencolor, XMCOLOR bkbrush);

#define XM_focus	XM_FocusRect


#endif	// _XM_GDI_H_
