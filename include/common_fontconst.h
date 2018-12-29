 /***************************************************************************
  *
  *   Created:   2004-12-16 18:03:14
  *   Author:    Chengxinhui
  *
  ****************************************************************************
  * Description: ����ϵͳ������йس�����
  * 
  * NOTE:��������ĳ������������޸ģ���Ϊ��Щ���������ĵ�ģ��ܶ࣬�漰������
  *		��������صĴ��롣
  * 
  * MEMO:
  * 
  * Revision history:	
  *        Author            Date                       Cause
  *     ------------  ------------------  -------------------------------------
  *  1. Chengxinhui   2004-12-16 18:03:14  Created.
  *
  ****************************************************************************/

#ifndef __COMMON_FONTCONST_H__
#define __COMMON_FONTCONST_H__

#ifdef __cplusplus
extern "C" {
#endif

#define	FONT_NAME		"Name"
#define	FONT_ID			"ID"

#define	FONT_BUILTIN_TOTAL	STOCK_FONT_TOTAL	/* number of compiled-in fonts*/
#define	UNI_MAP_PLANE_TOTAL			0x11		// UNICODE��׼�����е��ַ���Ϊ11����λ�桱Map Plane��
// <Cxh, 2002-9-21 10:03:10> Set the system font to unicode.
// ������Unicode�����Ӧ��Ascii����
// MWFS: MWFONT_STYLE

//
// 1��ϵͳ֧�ֶ������塣��ͬ��������ͨ������������ʶ��ģ������������Ϊ8��ASCII�ַ���
//	2��ϵͳȱʡ����Ϊ�����塱��
// 3����������ʹ�õ������Ǿ�������ģ���ˣ�������ҪΪÿһ�����嶨��һ�����������͡�ID��
// 4�����뱣֤���������͡�ID����������һһ��Ӧ��
// 5�����֧��127�֡��������͡������������͡�IDֵ��������127��
//
enum enumFontType
{
	FONT_TYPE_SIMSUN	= 0,	// ����
	FONT_TYPE_MINCHO	= 1,	// ��������
	FONT_TYPE_MINGLIU	= 2,	// ��������
	FONT_TYPE_TOTAL
};
// FTYN: Font TYpe Name.
//; ; #define	FTYN_NAME			"Type"
//; ; #define	FTYN_SONGTI			"SongTi"
//; ; #define	FTYN_KAITI			"KaiTi"
//; ; #define	FTYN_XINGSHU		"XingShu"
//; ; #define	FTYN_CAOSHU			"CaoShu"


/*
// 
// ����������16λ��ʾ��
//
// �������ʵ��������;����һ���������ݵ�֧�֣���һ�ַ�ʽ��ͨ�������ʵ�֡�
// �����»��ߡ���ɾ���ߡ�˫ɾ�������෽ʽ��ͨ�������ʵ�֣��������ͨ�����
// ��ʵ�ֵķ����Ժ�������������ϡ�
// ��ˣ�������Ķ���Ƚ����⣺��4λ���Զ���16������������֧�ֵ������񣬵�
// ���ֻ������6�ַ�񡣸�λ�����������������ʵ�ֵķ��ÿһ��λ����һ�ַ��
//	���ɶ���12�ַ��
// // <Cxh, 2005-2-22 9:39:34> 1���ı�������Ķ��壺��б�塱�����ͬ�ڡ��»��ߡ���
// �������ߡ�����������ʵ�֡�
// 2��Ϊ����չ�ķ��㣬������֧�ֵ�������������һ�����ӵ����֡�
// 3������û�еķ�����壬ȫ��ʹ�����������滻��
//
*/
enum enumFontStyle
{
	FONT_STYLE_NORMAL			= 0x0000,	// Normal
	FONT_STYLE_BOLD			= 0x0001,	// Bold
	FONT_STYLE_ITALIC			= 0x0002,	// Italic
	FONT_STYLE_BOLDITALIC	= 0x0003,	// Bold and italic.
	FONT_STYLE_FIXED			= 0x0004,	// Fixed width
	FONT_STYLE_CUSTOM_0		= 0x0005,	// �Զ�����0����AS032�ж���ΪFT��
	FONT_STYLE_CUSTOM_1		= 0x0006,	// �Զ�����1����AS032�ж���ΪJT��
//; 	FONT_STYLE_TOTAL	= 6,	// ������������֧�ֵ���������������˳����̶����������ٸı䡣2005-12-30 13:51:16 Chengxinhui  
	FONT_STYLE_TOTAL			= 0x0007,	
	// ������������֧�ֵ���������������˳����̶����������ٸı䡣2005-12-30 13:51:16 Chengxinhui  
	// 2006-7-2 12:42:00 Chengxinhui  ������������Ŀ��
	FONT_BASIC_STYLE_MASK	= 0x00f,
	FONT_SPEC_STYLE_START			= 0x0010,
	FONT_SPEC_STYLE_ITALIC			= 0x0010,
	FONT_SPEC_STYLE_INVERT			= 0x0020,
	FONT_SPEC_STYLE_UNDERLINE		= 0x0040,	// Underline
	FONT_SPEC_STYLE_STRIKE			= 0x0080,	// Strikethrough
	FONT_SPEC_STYLE_DB_STRIKE		= 0x0100,	// Double strikethrough
	FONT_SPEC_STYLE_BORDOR			= 0x0200,	// Char border.
	FONT_SPEC_STYLE_SHADING			= 0x0400,	// Char shading.
	FONT_SPEC_STYLE_MASK		= (~FONT_BASIC_STYLE_MASK)
};
//
// ��������������ơ�
//
#define	FSTN_NAME			"Style"
#define	FSTN_NORMAL			"Normal"
#define	FSTN_BOLD			"Bold"
#define	FSTN_ITALIC			"Italic"
#define	FSTN_BOLDITALIC	"BoldItal"
#define	FSTN_FIXED			"Fixed"
#define	FSTN_CUSTOM_0		"Custom0"
#define	FSTN_CUSTOM_1		"Custom1"

//
// ϵͳ֧��255�������С��
// Defined the font size and font size index no.
// ϵͳ���֧��4�ִ�С�����塣����12��16��24�����������ǲ�����ı�ģ�����ϵͳ����
// ��12��16��������������������У����е�4������Ĵ�С�������ơ�
// <Cxh, 2005-1-20 10:36:25> ���ڵ��������ʹ����Ҫ��������Դ����������С����ֻ����3����
// ����������֧�ֵ������������Ϊ6�֡�
#define	FONT_SIZE_TOTAL				3		
#define	FONT_SIZE_12					12
#define	FONT_SIZE_16					16
#define	FONT_SIZE_24					24
#define	FONT_SIZE_MIN					FONT_SIZE_12
// Define	FONT_SIZE_CUSTOM			8-256
#define	FSZN_NAME		"Size"
#define	FSZN_12			"12x12"
#define	FSZN_16			"16x16"
#define	FSZN_24			"24x24"
#define	FSZN_CUSTOM		"Custom"

enum enumFontSizeIndex
{
	FONT_SIZE_INDEX_START	= 0,
	FONT_SIZE_INDEX_12	= 0,
	FONT_SIZE_INDEX_16,
	FONT_SIZE_INDEX_24,
	FONT_SIZE_INDEX_END
};


#ifdef __cplusplus
}
#endif

#endif //__COMMON_FONTCONST_H__
