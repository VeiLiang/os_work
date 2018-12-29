//****************************************************************************
//
//	Copyright (C) 2012 ZhuoYongHong
//
//	Author	ZhuoYongHong
//
//	File name: xm_video_task.h
//	  ��Ƶ����
//
//	Revision history
//
//		2012.09.06	ZhuoYongHong Initial version
//
//****************************************************************************
#ifndef _XM_VIDEO_TASK_H_
#define _XM_VIDEO_TASK_H_

#if defined (__cplusplus)
	extern "C"{
#endif

// ��Ƶ����ƴ��ģʽ
enum {
	XMSYS_ASSEMBLY_MODE_FRONT_ONLY = 0,			// ǰ��ȫ�� (ǰ������ͷȫ��)
	XMSYS_ASSEMBLY_MODE_REAL_ONLY,				// ����ȫ�� (ǰ������ͷȫ��)
	XMSYS_ASSEMBLY_MODE_FRONT_REAL,				// ǰ����, ���л�(ǰ������ͷΪ��, ��������ͷΪ��)
	XMSYS_ASSEMBLY_MODE_REAL_FRONT,				// ǰ������, ���л�(ǰ������ͷΪ��, ��������ͷΪ��)
	XMSYS_ASSEMBLY_MODE_ISP601_ONLY,
	XMSYS_ASSEMBLY_MODE_ITU656_ONLY,
	XMSYS_ASSEMBLY_MODE_USB_ONLY,
	XMSYS_ASSEMBLY_MODE_ISP601_ITU656,
	XMSYS_ASSEMBLY_MODE_ALL_DISPLAY_Left,
	XMSYS_ASSEMBLY_MODE_ALL_DISPLAY_Right,
	XMSYS_ASSEMBLY_MODE_ALL_DISPLAY_UP,
	XMSYS_ASSEMBLY_MODE_ALL_DISPLAY_Vert,
	XMSYS_ASSEMBLY_MODE_COUNT
};

// ��Ƶϵͳ�����ʼ��
int XMSYS_VideoInit (void);

// ��Ƶϵͳ�����˳�
int XMSYS_VideoExit (void);

// ������Ƶ����ƴ��ģʽ
// ����ֵ
//	0		�ɹ�
// -1		ʧ��
int XMSYS_VideoSetImageAssemblyMode (unsigned int image_assembly_mode);

// ���ص�ǰ��Ƶͼ��ƴ��ģʽ
unsigned int XMSYS_VideoGetImageAssemblyMode (void);


#if defined (__cplusplus)
	}
#endif		/* end of __cplusplus */

#endif	// _XM_VIDEO_TASK_H_