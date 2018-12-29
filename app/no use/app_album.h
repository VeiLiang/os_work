//****************************************************************************
//
//	Copyright (C) 2012 ZhuoYongHong
//
//	Author	ZhuoYongHong
//
//	File name: app_album.h
//	  相册杂项函数
//
//	Revision history
//
//		2013.09.22	ZhuoYongHong Initial version
//
//****************************************************************************
#ifndef _APP_ALBUM_H_
#define _APP_ALBUM_H_

#if defined (__cplusplus)
	extern "C"{
#endif

// 读取照片项数据库中照片项的个数
// image_channel 照片项通道, 0,1
// 返回值
// 0 表示无照片项
// > 0 表示已记录的照片项个数
int XM_ImageItemGetImageItemCount (unsigned int image_channel);

// 读取指定照片通道的文件列表
// image_channel 照片项通道
// index 读取的记录开始序号
// count 读取的记录条数(包含开始序号)
// packet_addr 包缓冲区地址
// packet_size 包缓冲区字节大小
// 返回值
// < 0 失败
// = 0 
int XM_ImageItemGetImageItemList (unsigned int image_channel, 
											 unsigned int index, 
											 unsigned int count,
											 void *packet_addr, 
											 int packet_size
											);




#if defined (__cplusplus)
	}
#endif		/* end of __cplusplus */

#endif	// _APP_ALBUM_H_