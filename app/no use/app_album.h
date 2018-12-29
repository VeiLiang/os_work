//****************************************************************************
//
//	Copyright (C) 2012 ZhuoYongHong
//
//	Author	ZhuoYongHong
//
//	File name: app_album.h
//	  ��������
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

// ��ȡ��Ƭ�����ݿ�����Ƭ��ĸ���
// image_channel ��Ƭ��ͨ��, 0,1
// ����ֵ
// 0 ��ʾ����Ƭ��
// > 0 ��ʾ�Ѽ�¼����Ƭ�����
int XM_ImageItemGetImageItemCount (unsigned int image_channel);

// ��ȡָ����Ƭͨ�����ļ��б�
// image_channel ��Ƭ��ͨ��
// index ��ȡ�ļ�¼��ʼ���
// count ��ȡ�ļ�¼����(������ʼ���)
// packet_addr ����������ַ
// packet_size ���������ֽڴ�С
// ����ֵ
// < 0 ʧ��
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