//****************************************************************************
//
//	Copyright (C) 2010 ShenZhen ExceedSpace
//
//	Author	ZhuoYongHong
//
//	File name: app_processview.h
//	  过程操作视窗(格式化、系统升级、恢复出厂设置)
//
//	  过程操作视窗一般由固定的过程操作组成, 如下
//      1) 操作初始化阶段
//         显示即将执行的动作信息, 提示用户如何操作来进入到下一个过程阶段. 如
//	           按"确认"键开始系统升级, 按"取消"键返回. 
//      2) 操作确认阶段
//         显示警告信息, 提示如何操作来继续或放弃该操作. 如
//            重新按下"确认"键执行系统升级, 升级过程请勿断电. 按"取消"键返回.
//      3) 操作命令执行状态
//         命令执行中, 显示反馈信息或等待动画告知用户当前操作的状态. 如
//            系统正在升级! 请勿断电或关机!
//      4) 操作完成状态
//         命令执行完毕, 显示成功或者失败的信息
//
//	Revision history
//
//		2013.09.01	ZhuoYongHong Initial version
//
//****************************************************************************

#ifndef _XM_APP_PROCESS_VIEW_API_H_
#define _XM_APP_PROCESS_VIEW_API_H_

#if defined (__cplusplus)
	extern "C"{
#endif

// 菜单选项回调函数
#define PROCESS_FAIL		(-1)
#define PROCESS_INITIAL	(0)

#define	PROCESS_STATE_INITIAL			0	// 操作初始化状态
#define	PROCESS_STATE_CONFIRM			1	// 操作确认中状态
#define	PROCESS_STATE_WORKING			2	// 操作执行中状态
#define	PROCESS_STATE_SUCCESS			3	// 操作成功状态
#define	PROCESS_STATE_FAILURE			4	// 操作失败状态


typedef int (*FPOPERATIONCB)		(void *);		// 过程操作函数
typedef void (*FPSYSTEMEVENTCB)	(XMMSG *msg, int process_state);		// 系统事件回调函数

// 过程视窗类型
enum {
	AP_PROCESS_VIEW_FORMAT = 0,			// 格式化过程视窗
	AP_PROCESS_VIEW_SYSTEMUPDATE,		// 系统升级过程视窗
	AP_PROCESS_VIEW_RESTORESETTING,		// 恢复出厂设置过程视窗
	AP_PROCESS_VERSION,					//版本号显示
	AP_PROCESS_VIEW_COUNT
};

typedef struct tagAP_PROCESSINFO {
	int					type;						// 视窗类型
	APPMENUID			Title;						// 窗口标题
	APPMENUID			DispItem[APP_DISPLAY_ITEM_COUNT];				// 提示信息
	int					nDispItemNum;				// 提示信息数
	int					nMaxProgress;				// 最大进度值
	FPOPERATIONCB		fpStartProcess;				// 开始处理过程回调函数
	FPOPERATIONCB		fpQueryProgress;			// 处理进程回调函数
	FPOPERATIONCB		fpEndProcess;				// 结束处理过程回调函数
	FPSYSTEMEVENTCB	fpSystemEventCb;				// 系统事件处理回调函数
	void *				lpPrivateData;				// 过程私有数据
} AP_PROCESSINFO;



#if defined (__cplusplus)
	}
#endif		/* end of __cplusplus */

#endif	// #ifndef _XM_APP_MENUDATA_H_


