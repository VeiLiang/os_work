//****************************************************************************
//
//	Copyright (C) 2012 ZhuoYongHong
//
//	Author	ZhuoYongHong
//
//	File name: xm_recycle.h
//	  ����վ����
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
	unsigned char rev;			// �����ֶ�, Ϊ0
	char item_name[9];			// �ļ���
} XM_RECYCLEITEM;


// ��ȡ����վ�ɻ�����Ŀ�ĸ���
// ����ֵ
// 0 ��ʾ�޻��յ���Ŀ
// > 0 ��ʾ�ɻ��յ���Ŀ����
unsigned int XM_RecycleGetItemCount (void);


// ��ȡָ��������ʶ�Ļ�����
XM_RECYCLEITEM* XM_RecycleGetItem (unsigned int index);

// ��ԭָ��������ʶ���ļ�
// ����ֵ
//   -1  ʧ��
//   0   �ɹ�
int XM_RecycleRestoreFile (unsigned int index);

// ��һ���ļ����뵽����վ
// channel	ͨ����
// type		�ļ�����, XM_FILE_TYPE_PHOTO / XM_FILE_TYPE_VIDEO
//
// ����ֵ
//   -1  ʧ��
//   0   �ɹ�
int XM_RecycleDeleteFile (unsigned int channel, unsigned int type, const char *file_name);



// ����վ��ʼ��
void XM_RecycleInit (void);


// ����վ�ر�, �ͷŷ������Դ
void XM_RecycleExit (void);

// �������վ�е������ļ�
// 0   �ɹ�
// < 0 ʧ��
int XM_RecycleCleanFile (char *recycle_path);

#if defined (__cplusplus)
	}
#endif		/* end of __cplusplus */

#endif	// _XM_RECYCLE_ITEM_H_