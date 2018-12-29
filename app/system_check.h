//****************************************************************************
//
//	Copyright (C) 2012 ZhuoYongHong
//
//	Author	ZhuoYongHong
//
//	File name: system_check.h
//	  系统自检
//
//	Revision history
//
//		2012.09.16	ZhuoYongHong Initial version
//
//****************************************************************************

#ifndef _XM_APP_SYSTEMCHECK_H_
#define _XM_APP_SYSTEMCHECK_H_

#if defined (__cplusplus)
	extern "C"{
#endif

#define	APP_SYSTEMCHECK_SUCCESS								(0)		// 

#define	APP_SYSTEMCHECK_CARD_NOCARD						(-1)		// 卡不存在


#define	APP_SYSTEMCHECK_FILESYSTEM_FSERROR				(-3)		// 卡文件系统检查异常。
																			//		如无法创建目录、

#define	APP_SYSTEMCHECK_FILESYSTEM_LOWSPACE				(-4)		// 卡剩余空间不足，低于可录时间报警语音门限

#define	APP_SYSTEMCHECK_BACKUPBATTERY_LOWVOLTAGE		(-5)		// 备份电池低电压

#define	APP_SYSTEMCHECK_TIMESETTING_INVALID				(-6)		// 系统时间未设置


// 卡上电自检(检查卡是否存在及卡是否写保护)
int APSYS_CardChecking (void);


// 系统时间设置检查
int APSYS_TimeSettingChecking (void);

// 备份电池检查
int APSYS_BackupBatteryChecking (void);


#if defined (__cplusplus)
	}
#endif		/* end of __cplusplus */

#endif	// #ifndef _XM_APP_SYSTEMCHECK_H_
