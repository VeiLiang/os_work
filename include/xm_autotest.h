//****************************************************************************
//
//	Copyright (C) 2010 Shenzhen Exceedspace Digital Technology Co.,LTD
//
//	Author	ZhuoYongHong
//
//	File name: xm_autotest.c
//	 auto tester interface
//
//	Revision history
//
//		2006.04.25	ZhuoYongHong begin
//		2009.08.01	ZhuoYongHong 加入笔事件支持
//
//****************************************************************************
#ifndef _XM_AUTOTEST_H_
#define _XM_AUTOTEST_H_

#if defined (__cplusplus)
	extern "C"{
#endif

// 自动测试初始化
void xm_autotest_init (void);

// 自动测试启动
// autotest_script_file	脚本文件
// 返回值
//	0		执行成功
// < 0	执行失败
int xm_autotest_run (const char *autotest_script_file);

// 自动测试终止
// 返回值
//	0		执行成功
// < 0	执行失败
int xm_autotest_stop (void);

#if defined (__cplusplus)
	}
#endif		/* end of __cplusplus */

#endif		// _XM_AUTOTEST_H_