//****************************************************************************
//
//	Copyright (C) 2012 ZhuoYongHong
//
//	Author	ZhuoYongHong
//
//	File name: system_check.h
//	  ϵͳ�Լ�
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

#define	APP_SYSTEMCHECK_CARD_NOCARD						(-1)		// ��������


#define	APP_SYSTEMCHECK_FILESYSTEM_FSERROR				(-3)		// ���ļ�ϵͳ����쳣��
																			//		���޷�����Ŀ¼��

#define	APP_SYSTEMCHECK_FILESYSTEM_LOWSPACE				(-4)		// ��ʣ��ռ䲻�㣬���ڿ�¼ʱ�䱨����������

#define	APP_SYSTEMCHECK_BACKUPBATTERY_LOWVOLTAGE		(-5)		// ���ݵ�ص͵�ѹ

#define	APP_SYSTEMCHECK_TIMESETTING_INVALID				(-6)		// ϵͳʱ��δ����


// ���ϵ��Լ�(��鿨�Ƿ���ڼ����Ƿ�д����)
int APSYS_CardChecking (void);


// ϵͳʱ�����ü��
int APSYS_TimeSettingChecking (void);

// ���ݵ�ؼ��
int APSYS_BackupBatteryChecking (void);


#if defined (__cplusplus)
	}
#endif		/* end of __cplusplus */

#endif	// #ifndef _XM_APP_SYSTEMCHECK_H_
