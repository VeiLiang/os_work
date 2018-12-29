//****************************************************************************
//
//	Copyright (C) 2012 ZhuoYongHong
//
//	Author	ZhuoYongHong
//
//	File name: xmxml.h
//	  constant��macro & basic typedef definition of XML Parser
//
//	Revision history
//
//		2011.06.08	ZhuoYongHong Initial version
//
//****************************************************************************
#ifndef _XM_XML_H_
#define _XM_XML_H_

#include <xm_type.h>

#if defined (__cplusplus)
	extern "C"{
#endif

// �궨��
#define	XMXML_ID							0x4C4D5858	// 'XXML'
#define	XMXML_VERSION					0x00100010	// 1010



// ���ݽṹ����
typedef struct tag_XMXML_HEADER {
	DWORD		id;				//	0	����ͷ��ʶ
	DWORD		version;			// 4	���ݰ汾

	DWORD		tag_count;		// 8	��ǵĸ���
	DWORD		tag_entry;		// 12	������ݵ���ڣ�����ڶ�����XMLͷ�ṹ��ʼ��ƫ�ƣ�

	DWORD		attr_count;		// 16	���Եĸ���
	DWORD		attr_entry;		// 20	�������ݵ���ڣ�����ڶ�����XMLͷ�ṹ��ʼ��ƫ�ƣ�
	
	DWORD		data_entry;		// 24	ֵ���(CDATA������ֵ)������ڶ�����XMLͷ�ṹ��ʼ��ƫ�ƣ�
	DWORD		data_count;		// 28	ֵ���ݵ��ֽڴ�С

	BYTE		rev[512-32];	// 32 �����ֶ�
} XMXML_HEADER;

typedef struct tag_XMXML_ATTR_STRUCT {
	DWORD		attr_id;				// ���Ե�ID
	DWORD		attr_value_len;	// ����ֵ���ֽڴ�С
	DWORD		attr_next;			// ָ����һ�����Խ��
	DWORD		attr_tag;			// TAG������ֵ
	DWORD		attr_value;			// ����ֵ�����ƫ��
} XMXML_ATTR_STRUCT;

typedef struct tag_XMXML_TAG_STRUCT {
	DWORD		tag_id;
	DWORD		tag_attr;
	DWORD		tag_parent;
	DWORD		tag_child;
	DWORD		tag_prev;
	DWORD		tag_next;
	DWORD		tag_value;		// ָ��CDATA. CDATA���ַ�0����
	DWORD		tag_value_len;
} XMXML_TAG_STRUCT;

// XML��ʼ��
void	XM_XmlInit (void);

// XML�ر�
void	XM_XmlExit (void);

// ����XMLƫ�ƻ�ȡXML�ľ��(�ڴ�)��ַ
HANDLE XM_XmlAddress (DWORD dwOffset);

// ��ȡXML���ݵĸ��ڵ�
XMXML_TAG_STRUCT *XM_XmlRoot (HANDLE hXml);

// ��ȡTAG������һ���ֵܽڵ�
XMXML_TAG_STRUCT *XM_XmlNext (HANDLE hXml, XMXML_TAG_STRUCT *tag);

// ��ȡTAG����ǰһ���ֵܽڵ�
XMXML_TAG_STRUCT *XM_XmlPrev (HANDLE hXml, XMXML_TAG_STRUCT *tag);

// ��ȡTAG���ĸ��ڵ�
XMXML_TAG_STRUCT *XM_XmlParent (HANDLE hXml, XMXML_TAG_STRUCT *tag);

// ��ȡTAG���ĵ�һ���ӽڵ�
XMXML_TAG_STRUCT *XM_XmlChild (HANDLE hXml, XMXML_TAG_STRUCT *tag);

// ��ȡTAG���ĵ�һ�������ӽڵ�
XMXML_ATTR_STRUCT *XM_XmlAttr (HANDLE hXml, XMXML_TAG_STRUCT *tag);

// ��ȡATTR������һ�����Խڵ�
XMXML_ATTR_STRUCT *XM_XmlNextAttr (HANDLE hXml, XMXML_ATTR_STRUCT *attr);

// ��ȡTAG���Ľ��ID
#define	XM_XmlTagID(tag)		(tag)->tag_id

// ��ȡATTR��������ID
#define	XM_XmlAttrID(attr)	(attr)->attr_id

XMXML_TAG_STRUCT *XM_XmlLocate (HANDLE hXml, XMXML_TAG_STRUCT *tag, DWORD tag_id);


#if defined (__cplusplus)
	}
#endif		/* end of __cplusplus */

#endif	// _XM_XML_H_