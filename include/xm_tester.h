//****************************************************************************
//
//	Copyright (C) ZhuoYongHong
//
//	Author	ZhuoYongHong
//
//	File name: xmtester.h
//	  自动测试接口
//
//	Revision history
//
//
//		2010.04.25	ZhuoYongHong first version
//
//***************************************************************************

#ifndef _XM_TESTER_H_
#define _XM_TESTER_H_

#ifdef	__cplusplus
extern "C" {
#endif


// 测试模式定义
#define	TESTER_MODE_SCRIPT				1		//	脚本文件测试模式(屏幕位图截获输出)
#define	TESTER_MODE_BITMAP				2		// 脚本文件测试模式(直接BITMAP位图输出)

// 测试命令定义
#define	TESTER_CMD_START					1		// 测试开始
#define	TESTER_CMD_STOP					2		// 测试中止
#define	TESTER_CMD_PAUSE					3		// 测试暂停
#define	TESTER_CMD_MESSAGE				4		// 获取下一个测试消息(硬件上使用陷阱实现)
#define	TESTER_CMD_TEST					5		// 监测自动测试是否工作
#define	TESTER_CMD_RECORD					6		// 监控键盘输入	
#define	TESTER_CMD_KEYMAP					7		// 加载键盘定义表
#define	TESTER_CMD_PEN						8		// 监控触笔输入

// 测试返回值定义
#define	TESTERSYSERR_NOERROR					0		// 无错误
#define	TESTERSYSERR_INVALIDCOMMAND		(-1)	// 无效命令
#define	TESTERSYSERR_INVALIDPARAM			(-2)	// 无效命令参数
#define	TESTERSYSERR_BUSY						(-3)	// 服务忙
#define	TESTERSYSERR_UNSUPPORT				(-4)	// 未支持特性
#define	TESTERSYSERR_INVALIDSCRIPTFILE	(-5)	// 无效的脚本文件
#define	TESTERSYSERR_INVALIDSCRIPTLINE	(-6)	// 无效的脚本语句行
#define	TESTERSYSERR_ENDOFSCRIPT			(-7)	// 脚本文件结束
#define	TESTERSYSERR_BEGINOFMESSAGE		(-8)	// 消息事件开始
#define	TESTERSYSERR_INVALIDKEYMAPFILE	(-9)	// 无效的键盘映射表文件
#define	TESTERSYSERR_FAILTTOCREATELOG		(-10)	// 不能创建LOG文件
#define	TESTERSYSERR_FAILTOCREATERECORD	(-11)	// 不能创建RECORD文件
#define	TESTERSYSERR_FAILTOCREATEBITMAP	(-12)	// 不能创建BITMAP文件
#define	TESTERSYSERR_PENSCRIPT				(-13)	// 非法的笔事件记录

// 自动测试结束通知代码
#define	NTF_TESTER_FINISH						(LONG)(1)	// 自动测试正常结束
#define	NTF_TESTER_STOP						(LONG)(2)	// 自动测试强制结束
#define	NTF_TESTER_EXCEPTION					(LONG)(3)	// 自动测试异常结束


#define	TESTER_RECORD_ON						((DWORD)(-1))
#define	TESTER_RECORD_OFF						((DWORD)(-2))

// 显示位图捕捉函数
typedef int (*FPCAPTUREBITMAP) (void *lpbm, void * lpbi);
// 自动测试结束回调函数
typedef int (*FPAUTOTESTSTOP)  (UINT uNitifyCode);	
//	uNotifyCode  自动测试结束通知代码
//		


typedef struct _XMTESTERSTART {
	UINT						uTestMode;					// 测试模式
	const char *			lpScriptFile;				// 脚本文件路径
	FPCAPTUREBITMAP		fpCaptureBitmap;			// 位图截取函数或输出位图路径
																//		1)	屏幕位图截获输出
																//			显示位图捕捉函数指针
																//		2)	直接BITMAP位图输出
																//			输出位图路径
	const char *			lpKeyMapFile;				// 键盘定义文件
	FPAUTOTESTSTOP			fpAutotestStop;			// 自动测试结束回调函数
} XMTESTERSTART;

typedef struct _XMTESTERMESSAGE {
	WORD					message;
	WORD					wParam;
	DWORD					lParam;
} XMTESTERMESSAGE;

typedef struct _XMTESTERRECORD {
	DWORD					key;
	DWORD					reserved;
} XMTESTERRECORD;

typedef struct _XMTESTERKEYMAP {
	char *				lpKeyMapFile;
} XMTESTERKEYMAP;

int	XM_TesterSendCommand (UINT uTestCommand, VOID *pCommandParam);

#ifdef	__cplusplus
}
#endif

#endif // _WINTESTER_H_