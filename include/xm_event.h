//****************************************************************************
//
//	Copyright (C) 2012 ShenZhen ExceedSpace
//
//	Author	ZhuoYongHong
//
//	File name: xm_event.h
//	  constant��macro & basic typedef definition of event
//
//	Revision history
//
//		2010.09.01	ZhuoYongHong Initial version
//
//****************************************************************************
#ifndef _XM_EVENT_H_
#define _XM_EVENT_H_

#include <xm_type.h>



// "һ������"֪ͨ�¼�ID
enum {
	XM_EVENT_ONEKEYPHOTO_CREATE,				// �Ѵ������һ��һ�����յ���Ƭ
																//		����֪ͨUI�㣬�ײ��ļ������仯
	XM_EVENT_ONEKEYPHOTO_DELETE,				// ��ɾ�����һ��һ�����յ���Ƭ
																//		����֪ͨUI�㣬�ײ��ļ������仯
	//XM_APP_MESSAGE_ONEKEYPHOTO_MOREITEMS,		//	һ�����յ���Ƭ̫��	
}; 

typedef struct tagXM_ONEKEYPHOTO_EVENT {
	unsigned int	event;						// ����Ƶ���¼�ID��
} XM_ONEKEYPHOTO_EVENT;

// ��ϵͳͶ��һ������֪ͨ�¼�
XMBOOL	XM_PostOneKeyPhotoEvent (UINT OneKeyEvent, 
											 VOID *lpOneKeyEventParam, 
											 UINT cbOneKeyEventParam);


// ��ϵͳͶ����Ƶ��֪ͨ�ġ���Ƶ���¼�ID��
enum {
	XM_EVENT_VIDEOITEM_CREATE,					// �Ѵ������һ���µ���Ƶ��¼�ļ�
	XM_EVENT_VIDEOITEM_DELETE,					// ��ɾ�����һ����Ƶ��¼�ļ�
};

typedef struct tagXM_VIDEOITEM_EVENT {
	unsigned int	event;						// ����Ƶ���¼�ID��
	unsigned char	channel;						// �����¼���ͨ����
	unsigned char	file_name[32];				// ��Ƶ���ļ���
} XM_VIDEOITEM_EVENT;

XMBOOL	XM_PostVideoItemEvent (UINT VideoItemEvent, 
										  VOID *lpVideoItemEventParam, 
										  UINT cbVideoItemEventParam);


#endif	// _XM_EVENT_H_
