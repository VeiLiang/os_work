//****************************************************************************
//
//	Copyright (C) 2012 ZhuoYongHong
//
//	Author	ZhuoYongHong
//
//	File name: app_menuoption.h
//	  通用Menu选项列表选择窗口(只能选择一个选项)
//
//	Revision history
//
//		2012.09.16	ZhuoYongHong Initial version
//
//****************************************************************************

#ifndef _XM_APP_MENUOPTION_H_
#define _XM_APP_MENUOPTION_H_

// 菜单选项回调函数
typedef void (*FPMENUOPTIONCB) (void *lpUserData, int nMenuOptionSelect);

typedef struct _tagAPPMENUOPTION {
	DWORD				dwWindowTextId;		// 列表窗口标题栏的标题资源ID
													// 0xFFFFFFFF表示无该资源
	DWORD				dwWindowIconId;		// 列表窗口标题栏的图标资源ID
													// 0xFFFFFFFF表示无该资源
	WORD				wMenuOptionCount;		// 菜单项选项个数
	WORD				wMenuOptionSelect;	// 当前的选择项
	DWORD*			lpdwMenuOptionId;		// 菜单项选项资源ID数组
	FPMENUOPTIONCB	fpMenuOptionCB;		// 菜单选项回调函数
	VOID *			lpUserData;				// 用户私有数据
} APPMENUOPTION;

VOID AP_OpenMenuOptionView (APPMENUOPTION *lpAppMenuOption);

#endif	// #ifndef _XM_APP_MENUOPTION_H_

