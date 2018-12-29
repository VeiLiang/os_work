//****************************************************************************
//
//	Copyright (C) 2012 ShenZhen ExceedSpace
//
//	Author	ZhuoYongHong
//
//	File name: xm_osd_layer_animate.h
//	  constant��macro & basic typedef definition of animate interface of OSD layer
//
//	Revision history
//
//		2014.02.27	ZhuoYongHong Initial version
//
//****************************************************************************
#ifndef _XM_OSD_LAYER_ANIMATE_H_
#define _XM_OSD_LAYER_ANIMATE_H_

#if defined (__cplusplus)
	extern "C"{
#endif

// OSD Layer�ƶ�������
#define	XM_OSDLAYER_MOVING_FROM_LEFT_TO_RIGHT		1
#define	XM_OSDLAYER_MOVING_FROM_RIGHT_TO_LEFT		2
#define	XM_OSDLAYER_MOVING_FROM_TOP_TO_BOTTOM		3
#define	XM_OSDLAYER_MOVING_FROM_BOTTOM_TO_TOP		4

// OSD Layer����ģʽ����
#define	XM_OSDLAYER_ANIMATE_MODE_VIEW_CREATE		1		// ���Ӵ����� (�Ե������Ƴ�)
#define	XM_OSDLAYER_ANIMATE_MODE_VIEW_REMOVE		2		// �Ӵ�����   (�Զ���������)
#define	XM_OSDLAYER_ANIMATE_MODE_ALERT_CREATE		3		// �Ի��򴴽� (͸��Ч��)
#define	XM_OSDLAYER_ANIMATE_MODE_ALERT_REMOVE		4		// �Ի��򵯳� (͸��Ч��)
#define	XM_OSDLAYER_ANIMATE_MODE_EVENT_CREATE		5		// ��Ϣ������ (�Զ������Ƴ�)
#define	XM_OSDLAYER_ANIMATE_MODE_EVENT_REMOVE		6		// ��Ϣ������ (�Ե���������)



// OSD���Ӵ��ƶ� (���߳�֧��)
void XM_osd_layer_animate (
									unsigned int animating_mode,		// OSD�ƶ�ģʽ
									unsigned int moving_direction,	// �ƶ�����
									HANDLE old_top_view,					// �ϵĶ���ͼ���
									HANDLE new_top_view					// �µĶ���ͼ���
									);

#if defined (__cplusplus)
	}
#endif		/* end of __cplusplus */

#endif	// _XM_OSD_LAYER_ANIMATE_H_
