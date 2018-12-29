//****************************************************************************
//
//	Copyright (C) 2010 ShenZhen ExceedSpace
//
//	Author	ZhuoYongHong
//
//	File name: app_romimages.c
//	ROM图像资源管理, 仅支持单线程操作
//
//	Revision history
//
//		2012.09.01	ZhuoYongHong Initial version
//											
//
//****************************************************************************
/*
*/
#include <string.h>
#include <xm_user.h>
#include <xm_base.h>
#include <stdio.h>
#include <xm_dev.h>
#include <xm_printf.h>
#include <xm_image.h>

#include "app.h"
#include "app_menuid.h"


// 使用菜单ID, 在指定窗口的指定区域绘制Rom Image
int AP_RomImageDrawByMenuID (	DWORD MenuID,
								HANDLE hWnd, 
								XMRECT *lpRect, DWORD dwFlag
							)
{
#ifdef USE_ROM_SIZE
	APPROMRES *AppRes;
	
	if(MenuID == AP_NULLID)
		return -1;

	AppRes = AP_AppRes2RomRes (MenuID);
	if(AppRes == NULL)
	{
		XM_printf ("Error, No Image Match MenuID(%d)\n", MenuID);
		return (-1);
	}
	return XM_RomImageDraw (AppRes->rom_offset, AppRes->res_length, hWnd, lpRect, dwFlag);
#else
	u32_t RomOffset, RomLength;
	if(MenuID == AP_NULLID)
		return -1;
	RomOffset = AP_AppRes2RomOffset (MenuID);
	if(RomOffset == 0)
	{
		XM_printf ("Error, No Image Match MenuID(%d)\n", MenuID);
		return (-1);
	}
	RomLength = XM_RomLength (RomOffset);
	if(RomLength == 0)
	{
		XM_printf ("Error, No Image Match MenuID(%d)\n", MenuID);
		return (-1);
	}
	return XM_RomImageDraw (RomOffset, RomLength, hWnd, lpRect, dwFlag);
#endif
}

