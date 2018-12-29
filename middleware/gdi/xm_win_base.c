//****************************************************************************
//
//	Copyright (C) 2010 ShenZhen ExceedSpace
//
//	Author	ZhuoYongHong
//
//	File name: xm_win_base.c
//	  ����������
//
//	Revision history
//
//		2010.09.01	ZhuoYongHong Initial version
//
//****************************************************************************
#include <string.h>
#include <xm_type.h>
#include <xm_user.h>
#include <xm_base.h>
#include <xm_dev.h>
#include <xm_rom.h>
#include <xm_dev.h>
#include "xm_device.h"
#include "xm_voice_prompts.h"
#include "xm_app_menudata.h"
// Beep����(�簴����)
XMBOOL XM_Beep (DWORD dwBeepID)
{
	if((dwBeepID == XM_BEEP_KEYBOARD)&&(AppMenuData.key == AP_SETTING_KEY_ON))
		XM_voice_prompts_insert_voice (XM_VOICE_ID_BEEP_KEYBOARD);

	return 1;
}

