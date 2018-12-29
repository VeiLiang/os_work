//****************************************************************************
//
//	Copyright (C) 2012 ZhuoYongHong
//
//	Author	ZhuoYongHong
//
//	File name: xm_recycle.h
//	  回收站函数
//
//	Revision history
//
//		2012.09.22	ZhuoYongHong Initial version
//
//****************************************************************************
#ifndef _XM_RECYCLE_H_
#define _XM_RECYCLE_H_

#if defined (__cplusplus)
	extern "C"{
#endif

typedef struct tagXM_RECYCLEITEM {
	unsigned char channel;		// channel
	unsigned char file_type;	// video or photo
	unsigned char rev;			// 保留字段, 为0
	char item_name[9];			// 文件名
} XM_RECYCLEITEM;


// 读取回收站可回收项目的个数
// 返回值
// 0 表示无回收的项目
// > 0 表示可回收的项目个数
unsigned int XM_RecycleGetItemCount (void);


// 获取指定索引标识的回收项
XM_RECYCLEITEM* XM_RecycleGetItem (unsigned int index);

// 还原指定索引标识的文件
// 返回值
//   -1  失败
//   0   成功
int XM_RecycleRestoreFile (unsigned int index);

// 将一个文件放入到回收站
// channel	通道号
// type		文件类型, XM_FILE_TYPE_PHOTO / XM_FILE_TYPE_VIDEO
//
// 返回值
//   -1  失败
//   0   成功
int XM_RecycleDeleteFile (unsigned int channel, unsigned int type, const char *file_name);



// 回收站初始化
void XM_RecycleInit (void);


// 回收站关闭, 释放分配的资源
void XM_RecycleExit (void);

// 清除回收站中的所有文件
// 0   成功
// < 0 失败
int XM_RecycleCleanFile (char *recycle_path);

#if defined (__cplusplus)
	}
#endif		/* end of __cplusplus */

#endif	// _XM_RECYCLE_ITEM_H_