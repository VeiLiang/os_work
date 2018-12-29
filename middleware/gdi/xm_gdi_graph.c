//****************************************************************************
//
//	Copyright (C) 2010 ShenZhen ExceedSpace
//
//	Author	ZhuoYongHong
//
//	File name: gdi_graph.c
//	  基本画图函数
//
//	Revision history
//
//		2010.09.11	ZhuoYongHong Initial version
//
//****************************************************************************

#include <xm_type.h>
#include <xm_user.h>
#include <xm_gdi.h>
#include <xm_assert.h>


void XM_line (HANDLE hWnd, XMCOORD x1, XMCOORD y1, XMCOORD x2, XMCOORD y2, BYTE dash, XMCOLOR color)
{
	if(x1 == x2)
	{
		// 画竖线
		if(dash == 0)
			dash = 0xFF;
		XM_DrawVLine (hWnd, x1, y1, (XMCOORD)(y2 - y1), dash, color);
	}
	else if(y1 == y2)
	{	
		// 画横线
		if(dash == 0)
			dash = 0xFF;
		XM_DrawHLine (hWnd, x1, y1, (XMCOORD)(x2 - x1), dash, color);
	}
	else
	{
		XM_ASSERT (0);
	}
}

void XM_rectangle (HANDLE hWnd, XMCOORD x1, XMCOORD y1, XMCOORD x2, XMCOORD y2, XMCOLOR pencolor, XMCOLOR bkbrush)
{
	if(pencolor == (XMCOLOR)(-1) && bkbrush == (XMCOLOR)(-1))
	{
		return;
	}
	else if(bkbrush == (XMCOLOR)(-1))
	{
		XM_line (hWnd, x1, y1, x2, y1, 0, pencolor);
		XM_line (hWnd, x2, y1, x2, y2, 0, pencolor);
		XM_line (hWnd, x1, y2, x2, y2, 0, pencolor);
		XM_line (hWnd, x1, y1, x1, y2, 0, pencolor);
	}
	else if(pencolor == (XMCOLOR)(-1))
	{
		/*
		XMRECT rect;

		rect.left = x1;
		rect.top = y1;
		rect.right = x2;
		rect.bottom = y2;*/
		XM_FillRect (hWnd, x1, y1, x2, y2, bkbrush);
	}
	else
	{
		XM_Rectangle (hWnd, x1, y1, x2, y2, bkbrush);
	}
}

void XM_frame (HANDLE hWnd, XMCOORD x1, XMCOORD y1, XMCOORD x2, XMCOORD y2, BYTE style)
{
	XMRECT rect;

	rect.left = x1;
	rect.top = y1;
	rect.right = x2;
	rect.bottom = y2;

	switch(style)
	{
		case GUI_SIMPLE_FRAME:
			break;

		case GUI_FRAME:
			XM_Draw3DRect (hWnd, &rect, XM_GetSysColor(XM_COLOR_3DFACE), XM_GetSysColor(XM_COLOR_3DDKSHADOW));
			XM_InflateRect (&rect, 1, 1);
			XM_Draw3DRect (hWnd, &rect, XM_GetSysColor(XM_COLOR_3DHILIGHT), XM_GetSysColor(XM_COLOR_3DSHADOW));
			break;

		case GUI_GRAY_FRAME:
			XM_rectangle (hWnd, x1, y1, x2, y2, (XMCOLOR)(-1), XM_GetSysColor(XM_COLOR_BTNFACE));
			XM_Draw3DRect (hWnd, &rect, XM_GetSysColor(XM_COLOR_3DFACE), XM_GetSysColor(XM_COLOR_3DDKSHADOW));
			XM_InflateRect (&rect, 1, 1);
			XM_Draw3DRect (hWnd, &rect, XM_GetSysColor(XM_COLOR_3DHILIGHT), XM_GetSysColor(XM_COLOR_3DSHADOW));
			break;

		case GUI_WHITE_FRAME:
			XM_rectangle (hWnd, x1, y1, x2, y2, (XMCOLOR)(-1), XM_COLOR_WHITE);
			XM_Draw3DRect (hWnd, &rect, XM_GetSysColor(XM_COLOR_3DFACE), XM_GetSysColor(XM_COLOR_3DDKSHADOW));
			XM_InflateRect (&rect, 1, 1);
			XM_Draw3DRect (hWnd, &rect, XM_GetSysColor(XM_COLOR_3DHILIGHT), XM_GetSysColor(XM_COLOR_3DSHADOW));
			break;

		case GUI_SUNKEN_FRAME:
			XM_Draw3DRect (hWnd, &rect, XM_GetSysColor(XM_COLOR_3DSHADOW), XM_GetSysColor(XM_COLOR_3DHILIGHT));
			XM_InflateRect (&rect, 1, 1);
			XM_Draw3DRect (hWnd, &rect, XM_GetSysColor(XM_COLOR_3DDKSHADOW), XM_GetSysColor(XM_COLOR_3DFACE));
			break;

		case GUI_GRAYSUNKEN_FRAME:
			XM_rectangle (hWnd, x1, y1, x2, y2, (XMCOLOR)(-1), XM_GetSysColor(XM_COLOR_BTNFACE));
			XM_Draw3DRect (hWnd, &rect, XM_GetSysColor(XM_COLOR_3DSHADOW), XM_GetSysColor(XM_COLOR_3DHILIGHT));
			XM_InflateRect (&rect, 1, 1);
			XM_Draw3DRect (hWnd, &rect, XM_GetSysColor(XM_COLOR_3DDKSHADOW), XM_GetSysColor(XM_COLOR_3DFACE));
			break;

		case GUI_WHITESUNKEN_FRAME:
			XM_rectangle (hWnd, x1, y1, x2, y2, (XMCOLOR)(-1), XM_COLOR_WHITE);
			XM_Draw3DRect (hWnd, &rect, XM_GetSysColor(XM_COLOR_3DSHADOW), XM_GetSysColor(XM_COLOR_3DHILIGHT));
			XM_InflateRect (&rect, 1, 1);
			XM_Draw3DRect (hWnd, &rect, XM_GetSysColor(XM_COLOR_3DDKSHADOW), XM_GetSysColor(XM_COLOR_3DFACE));
			break;

		case GUI_GROUP_FRAME:
			//上横线。
			XM_line (hWnd, x1, y1, x2, y1, 0, XM_GetSysColor(XM_COLOR_3DSHADOW));
			XM_line (hWnd, (XMCOORD)(x1+1), (XMCOORD)(y1+1), (XMCOORD)(x2-1), (XMCOORD)(y1+1), 0, XM_GetSysColor(XM_COLOR_3DHILIGHT));
			//右竖线。
			XM_line (hWnd, x2, y1, x2, y2, 0, XM_GetSysColor(XM_COLOR_3DSHADOW));
			XM_line (hWnd, (XMCOORD)(x2+1), y1, (XMCOORD)(x2+1), (XMCOORD)(y2+1), 0, XM_GetSysColor(XM_COLOR_3DHILIGHT));
			//下横线。
			XM_line (hWnd, x2, y2, x1, y2, 0, XM_GetSysColor(XM_COLOR_3DSHADOW));
			XM_line (hWnd, (XMCOORD)(x2+1), (XMCOORD)(y2+1), x1, (XMCOORD)(y2+1), 0, XM_GetSysColor(XM_COLOR_3DHILIGHT));
			//左竖线。
			XM_line (hWnd, x1, y2, x1, y1, 0, XM_GetSysColor(XM_COLOR_3DSHADOW));
			XM_line (hWnd, (XMCOORD)(x1+1), (XMCOORD)(y2-1), (XMCOORD)(x1+1), (XMCOORD)(y1+1), 0, XM_GetSysColor(XM_COLOR_3DHILIGHT));
			break;

		case GUI_FLANGE_FRAME:
			//上横线。
			XM_line (hWnd, x1, y1, x2, y1, 0, XM_GetSysColor(XM_COLOR_3DHILIGHT));
			XM_line (hWnd, (XMCOORD)(x1+1), (XMCOORD)(y1+1), (XMCOORD)(x2-1), (XMCOORD)(y1+1), 0, XM_GetSysColor(XM_COLOR_3DFACE));
			XM_line (hWnd, (XMCOORD)(x1+2), (XMCOORD)(y1+2), (XMCOORD)(x2-2), (XMCOORD)(y1+2), 0, XM_GetSysColor(XM_COLOR_3DSHADOW));
			XM_line (hWnd, (XMCOORD)(x1+3), (XMCOORD)(y1+3), (XMCOORD)(x2-3), (XMCOORD)(y1+3), 0, XM_GetSysColor(XM_COLOR_3DDKSHADOW));
			//右竖线。
			XM_line (hWnd, x2, y1, x2, y2, 0, XM_GetSysColor(XM_COLOR_3DDKSHADOW));
			XM_line (hWnd, (XMCOORD)(x2-1), (XMCOORD)(y1+1), (XMCOORD)(x2-1), (XMCOORD)(y2-1), 0, XM_GetSysColor(XM_COLOR_3DSHADOW));
			XM_line (hWnd, (XMCOORD)(x2-2), (XMCOORD)(y1+2), (XMCOORD)(x2-2), (XMCOORD)(y2-2), 0, XM_GetSysColor(XM_COLOR_3DFACE));
			XM_line (hWnd, (XMCOORD)(x2-3), (XMCOORD)(y1+3), (XMCOORD)(x2-3), (XMCOORD)(y2-3), 0, XM_GetSysColor(XM_COLOR_3DHILIGHT));
			//下横线。
			XM_line (hWnd, x2, y2, x1, y2, 0, XM_GetSysColor(XM_COLOR_3DDKSHADOW));
			XM_line (hWnd, (XMCOORD)(x2-1), (XMCOORD)(y2-1), (XMCOORD)(x1+1), (XMCOORD)(y2-1), 0, XM_GetSysColor(XM_COLOR_3DSHADOW));
			XM_line (hWnd, (XMCOORD)(x2-2), (XMCOORD)(y2-2), (XMCOORD)(x1+2), (XMCOORD)(y2-2), 0, XM_GetSysColor(XM_COLOR_3DFACE));
			XM_line (hWnd, (XMCOORD)(x2-3), (XMCOORD)(y2-3), (XMCOORD)(x1+3), (XMCOORD)(y2-3), 0, XM_GetSysColor(XM_COLOR_3DHILIGHT));
			//左竖线。
			XM_line (hWnd, x1, y2, x1, y1, 0, XM_GetSysColor(XM_COLOR_3DHILIGHT));
			XM_line (hWnd, (XMCOORD)(x1+1), (XMCOORD)(y2-1), (XMCOORD)(x1+1), (XMCOORD)(y1+1), 0, XM_GetSysColor(XM_COLOR_3DFACE));
			XM_line (hWnd, (XMCOORD)(x1+2), (XMCOORD)(y2-2), (XMCOORD)(x1+2), (XMCOORD)(y1+2), 0, XM_GetSysColor(XM_COLOR_3DSHADOW));
			XM_line (hWnd, (XMCOORD)(x1+3), (XMCOORD)(y2-3), (XMCOORD)(x1+3), (XMCOORD)(y1+3), 0, XM_GetSysColor(XM_COLOR_3DDKSHADOW));
			break;

		default:
			break;
	}
}

