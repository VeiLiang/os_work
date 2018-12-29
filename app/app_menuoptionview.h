//****************************************************************************
//
//	Copyright (C) 2012 ZhuoYongHong
//
//	Author	ZhuoYongHong
//
//	File name: app_menuoption.h
//	  ͨ��Menuѡ���б�ѡ�񴰿�(ֻ��ѡ��һ��ѡ��)
//
//	Revision history
//
//		2012.09.16	ZhuoYongHong Initial version
//
//****************************************************************************

#ifndef _XM_APP_MENUOPTION_H_
#define _XM_APP_MENUOPTION_H_

// �˵�ѡ��ص�����
typedef void (*FPMENUOPTIONCB) (void *lpUserData, int nMenuOptionSelect);

typedef struct _tagAPPMENUOPTION {
	DWORD				dwWindowTextId;		// �б��ڱ������ı�����ԴID
													// 0xFFFFFFFF��ʾ�޸���Դ
	DWORD				dwWindowIconId;		// �б��ڱ�������ͼ����ԴID
													// 0xFFFFFFFF��ʾ�޸���Դ
	WORD				wMenuOptionCount;		// �˵���ѡ�����
	WORD				wMenuOptionSelect;	// ��ǰ��ѡ����
	DWORD*			lpdwMenuOptionId;		// �˵���ѡ����ԴID����
	FPMENUOPTIONCB	fpMenuOptionCB;		// �˵�ѡ��ص�����
	VOID *			lpUserData;				// �û�˽������
} APPMENUOPTION;

VOID AP_OpenMenuOptionView (APPMENUOPTION *lpAppMenuOption);

#endif	// #ifndef _XM_APP_MENUOPTION_H_

