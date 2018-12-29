//****************************************************************************
//
//	Copyright (C) 2004-2005 Zhuo YongHong
//
//	Author	ZhuoYongHong
//
//	File name: xm_xml.c
//	  This is XML Parser toolset
//
//	Revision history
//
//		2010.06.01	ZhuoYongHong add XML
//
//****************************************************************************

#include <xm_xml.h>
#include <xm_rom.h>
#include <xm_lang.h>
#include <common_wstring.h>

// XML��ʼ��
void	XM_XmlInit (void)
{
}

// XML�ر�
void	XM_XmlExit (void)
{
}

// ��ȡXML���ݵĸ��ڵ�
XMXML_TAG_STRUCT *XM_XmlRoot (HANDLE hXml)
{
	XMXML_HEADER *header = (XMXML_HEADER *)hXml;
	XMXML_TAG_STRUCT *root;
	if(header == NULL)
		return NULL;
	if(header->id != XMXML_ID)
		return NULL;
	if(header->version != XMXML_VERSION)
		return NULL;

	// ����������Ϊ���ڵ�
	root = (XMXML_TAG_STRUCT *)(((char *)hXml) + header->tag_entry);
	return root;
}

// ��ȡTAG������һ���ֵܽڵ�
XMXML_TAG_STRUCT *XM_XmlNext (HANDLE hXml, XMXML_TAG_STRUCT *tag)
{
	XMXML_HEADER *header = (XMXML_HEADER *)hXml;
	XMXML_TAG_STRUCT *tag_entry;
	if(tag->tag_next == 0)
		return NULL;
	tag_entry = (XMXML_TAG_STRUCT *)(((char *)hXml) + header->tag_entry);

	return (XMXML_TAG_STRUCT *)(tag_entry + tag->tag_next - 1);
}

// ��ȡTAG����ǰһ���ֵܽڵ�
XMXML_TAG_STRUCT *XM_XmlPrev (HANDLE hXml, XMXML_TAG_STRUCT *tag)
{
	XMXML_HEADER *header = (XMXML_HEADER *)hXml;
	XMXML_TAG_STRUCT *tag_entry;
	if(tag->tag_prev == 0)
		return NULL;
	tag_entry = (XMXML_TAG_STRUCT *)(((char *)hXml) + header->tag_entry);
	return (XMXML_TAG_STRUCT *)(tag_entry + tag->tag_prev - 1);
}

// ��ȡTAG���ĸ��ڵ�
XMXML_TAG_STRUCT *XM_XmlParent (HANDLE hXml, XMXML_TAG_STRUCT *tag)
{
	XMXML_HEADER *header = (XMXML_HEADER *)hXml;
	XMXML_TAG_STRUCT *tag_entry;
	if(tag->tag_parent == 0)
		return NULL;
	tag_entry = (XMXML_TAG_STRUCT *)(((char *)hXml) + header->tag_entry);
	return (XMXML_TAG_STRUCT *)(tag_entry + tag->tag_parent - 1);
}

// ��ȡTAG���ĵ�һ���ӽڵ�
XMXML_TAG_STRUCT *XM_XmlChild (HANDLE hXml, XMXML_TAG_STRUCT *tag)
{
	XMXML_HEADER *header = (XMXML_HEADER *)hXml;
	XMXML_TAG_STRUCT *tag_entry;
	if(tag->tag_child == 0)
		return NULL;
	tag_entry = (XMXML_TAG_STRUCT *)(((char *)hXml) + header->tag_entry);
	return (XMXML_TAG_STRUCT *)(tag_entry + tag->tag_child - 1);
}

// ��ȡTAG���ĵ�һ�������ӽڵ�
XMXML_ATTR_STRUCT *XM_XmlAttr (HANDLE hXml, XMXML_TAG_STRUCT *tag)
{
	XMXML_HEADER *header = (XMXML_HEADER *)hXml;
	XMXML_ATTR_STRUCT *attr_entry;
	if(tag->tag_attr == 0)
		return NULL;
	attr_entry = (XMXML_ATTR_STRUCT *)(((char *)hXml) + header->attr_entry);
	return (XMXML_ATTR_STRUCT *)(attr_entry + tag->tag_attr - 1);
}

// ��ȡATTR������һ�����Խڵ�
XMXML_ATTR_STRUCT *XM_XmlNextAttr (HANDLE hXml, XMXML_ATTR_STRUCT *attr)
{
	XMXML_HEADER *header = (XMXML_HEADER *)hXml;
	XMXML_ATTR_STRUCT *attr_entry;
	if(attr->attr_next == 0)
		return NULL;
	attr_entry = (XMXML_ATTR_STRUCT *)(((char *)hXml) + header->attr_entry);
	return (XMXML_ATTR_STRUCT *)(attr_entry + attr->attr_next - 1);
}

// ��ָ�������ӽڵ��в���ָ��TAG ID���ӽڵ�
XMXML_TAG_STRUCT *XM_XmlLocate (HANDLE hXml, XMXML_TAG_STRUCT *tag, DWORD tag_id)
{
	XMXML_TAG_STRUCT *child = XM_XmlChild (hXml, tag);
	while(child)
	{
		if(child->tag_id == tag_id)
			return child;
		child = XM_XmlNext (hXml, child);
	}
	return NULL;
}

