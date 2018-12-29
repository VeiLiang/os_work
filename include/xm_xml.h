//****************************************************************************
//
//	Copyright (C) 2012 ZhuoYongHong
//
//	Author	ZhuoYongHong
//
//	File name: xmxml.h
//	  constant，macro & basic typedef definition of XML Parser
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

// 宏定义
#define	XMXML_ID							0x4C4D5858	// 'XXML'
#define	XMXML_VERSION					0x00100010	// 1010



// 数据结构定义
typedef struct tag_XMXML_HEADER {
	DWORD		id;				//	0	数据头标识
	DWORD		version;			// 4	数据版本

	DWORD		tag_count;		// 8	标记的个数
	DWORD		tag_entry;		// 12	标记数据的入口（相对于二进制XML头结构开始的偏移）

	DWORD		attr_count;		// 16	属性的个数
	DWORD		attr_entry;		// 20	属性数据的入口（相对于二进制XML头结构开始的偏移）
	
	DWORD		data_entry;		// 24	值入口(CDATA及属性值)（相对于二进制XML头结构开始的偏移）
	DWORD		data_count;		// 28	值数据的字节大小

	BYTE		rev[512-32];	// 32 保留字段
} XMXML_HEADER;

typedef struct tag_XMXML_ATTR_STRUCT {
	DWORD		attr_id;				// 属性的ID
	DWORD		attr_value_len;	// 属性值的字节大小
	DWORD		attr_next;			// 指向下一个属性结点
	DWORD		attr_tag;			// TAG的索引值
	DWORD		attr_value;			// 属性值在域的偏移
} XMXML_ATTR_STRUCT;

typedef struct tag_XMXML_TAG_STRUCT {
	DWORD		tag_id;
	DWORD		tag_attr;
	DWORD		tag_parent;
	DWORD		tag_child;
	DWORD		tag_prev;
	DWORD		tag_next;
	DWORD		tag_value;		// 指向CDATA. CDATA以字符0结束
	DWORD		tag_value_len;
} XMXML_TAG_STRUCT;

// XML初始化
void	XM_XmlInit (void);

// XML关闭
void	XM_XmlExit (void);

// 根据XML偏移获取XML的句柄(内存)地址
HANDLE XM_XmlAddress (DWORD dwOffset);

// 获取XML数据的根节点
XMXML_TAG_STRUCT *XM_XmlRoot (HANDLE hXml);

// 获取TAG结点的下一个兄弟节点
XMXML_TAG_STRUCT *XM_XmlNext (HANDLE hXml, XMXML_TAG_STRUCT *tag);

// 获取TAG结点的前一个兄弟节点
XMXML_TAG_STRUCT *XM_XmlPrev (HANDLE hXml, XMXML_TAG_STRUCT *tag);

// 获取TAG结点的父节点
XMXML_TAG_STRUCT *XM_XmlParent (HANDLE hXml, XMXML_TAG_STRUCT *tag);

// 获取TAG结点的第一个子节点
XMXML_TAG_STRUCT *XM_XmlChild (HANDLE hXml, XMXML_TAG_STRUCT *tag);

// 获取TAG结点的第一个属性子节点
XMXML_ATTR_STRUCT *XM_XmlAttr (HANDLE hXml, XMXML_TAG_STRUCT *tag);

// 获取ATTR结点的下一个属性节点
XMXML_ATTR_STRUCT *XM_XmlNextAttr (HANDLE hXml, XMXML_ATTR_STRUCT *attr);

// 获取TAG结点的结点ID
#define	XM_XmlTagID(tag)		(tag)->tag_id

// 获取ATTR结点的属性ID
#define	XM_XmlAttrID(attr)	(attr)->attr_id

XMXML_TAG_STRUCT *XM_XmlLocate (HANDLE hXml, XMXML_TAG_STRUCT *tag, DWORD tag_id);


#if defined (__cplusplus)
	}
#endif		/* end of __cplusplus */

#endif	// _XM_XML_H_