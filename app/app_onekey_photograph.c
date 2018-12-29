//****************************************************************************
//
//	Copyright (C) 2013 ShenZhen ExceedSpace
//
//	Author	ZhuoYongHong
//
//	File name: app_onekey_photograph.c
//	  一键拍照
//
//	Revision history
//
//		2013.05.11	ZhuoYongHong Initial version
//
//****************************************************************************
#include "app.h"


// 处理一键拍照
void AP_OnekeyPhotograph (void)
{
	// 
	XM_PostMessage(XM_SYSTEMEVENT, SYSTEM_EVENT_ONE_KEY_PHOTOGRAPH, 0);
}

