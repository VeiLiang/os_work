//****************************************************************************
//
//	Copyright (C) 2010 ShenZhen ExceedSpace
//
//	Author	ZhuoYongHong
//
//	File name: gui_res.h
//	  constant��macro, data structure��function protocol definition of resource interface 
//
//	Revision history
//
//		2010.09.09	ZhuoYongHong Initial version
//
//****************************************************************************
#ifndef _GDI_RES_H_
#define _GDI_RES_H_

#include <xmtype.h>

#define	MAX_RC_NAME			8
#define	MAX_RES_NAME		16

#define	RES_STRING			0
#define	RES_BITMAP			1
#define	RES_FONT				2
#define	RES_MENU				3
#define	RES_BINARY			4		// unknown binary type resource
#define	RES_DDB				5		// �豸���λͼ��ʽ



// ��Դ(RC)���ݽṹ����
// һ������˳�����32����Դ��
// һ������ҳֻ�ܶ���һ����ԴRC
typedef struct tagRC_HEAD {			// �ṹ��СΪ16�ֽ�
	char		rc_name[MAX_RC_NAME];	// 0	rc����
	u16_t		code_page;					//	8	language code
	u16_t		res_count;					// 10	��RC�ж������Դ����(����Դ�ڵ����)
	u32_t		node_offset;				// 12	��Դ�ڵ��(RES_NODE)ƫ��
} RC_HEAD;


//resource 
typedef struct tagRES_HEAD {

	char 		res_name[MAX_RES_NAME];	// ��Դ����
	u16_t		res_id;						// ��Դ���
	u16_t		res_type;					// ��Դ����
	u32_t		res_offset;					// ��Դ�����ļ�ƫ��
	u32_t		res_size;					// ��Դ���ݳ���
	u32_t		reserved;					// �����ֶ�, Ϊ0		
} RES_HEAD;

#endif	// _GDI_RES_H_