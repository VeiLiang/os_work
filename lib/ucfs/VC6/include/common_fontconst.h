 /***************************************************************************
  *
  *   Created:   2004-12-16 18:03:14
  *   Author:    Chengxinhui
  *
  ****************************************************************************
  * Description: 定义系统字体的有关常数。
  * 
  * NOTE:　所定义的常数不能随意修改！因为这些常数关联的的模块很多，涉及到所有
  *		与字体相关的代码。
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
#define	UNI_MAP_PLANE_TOTAL			0x11		// UNICODE标准把所有的字符分为11个“位面”Map Plane。
// <Cxh, 2002-9-21 10:03:10> Set the system font to unicode.
// 定义与Unicode码相对应的Ascii字体
// MWFS: MWFONT_STYLE

//
// 1、系统支持多种字体。不同的字体是通过字体名称来识别的，字体名称最多为8个ASCII字符。
//	2、系统缺省字体为“宋体”。
// 3、由于我们使用的字体是经过编码的，因此，我们需要为每一种字体定义一个“字体类型”ID。
// 4、必须保证“字体类型”ID和字体名称一一对应。
// 5、最多支持127种“字体类型”。“字体类型”ID值不允许超过127。
//
enum enumFontType
{
	FONT_TYPE_SIMSUN	= 0,	// 宋体
	FONT_TYPE_MINCHO	= 1,	// 日文字体
	FONT_TYPE_MINGLIU	= 2,	// 繁体中文
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
// 字体风格是用16位表示。
//
// 字体风格的实现有两种途径：一是字体数据的支持，另一种方式是通过软件来实现。
// 对于下划线、单删除线、双删除线这类方式是通过软件来实现，而且这个通过软件
// 来实现的风格可以和其它风格进行组合。
// 因此，字体风格的定义比较特殊：低4位可以定义16种有字体数据支持的字体风格，但
// 最多只允许定义6种风格。高位则用来定义由软件来实现的风格，每一个位定义一种风格，
//	最多可定义12种风格。
// // <Cxh, 2005-2-22 9:39:34> 1、改变字体风格的定义：“斜体”风格类同于“下划线”、
// “穿心线”风格，由软件来实现。
// 2、为了扩展的方便，由数据支持的字体风格类型由一种增加到三种。
// 3、对于没有的风格字体，全部使用正常体来替换。
//
*/
enum enumFontStyle
{
	FONT_STYLE_NORMAL			= 0x0000,	// Normal
	FONT_STYLE_BOLD			= 0x0001,	// Bold
	FONT_STYLE_ITALIC			= 0x0002,	// Italic
	FONT_STYLE_BOLDITALIC	= 0x0003,	// Bold and italic.
	FONT_STYLE_FIXED			= 0x0004,	// Fixed width
	FONT_STYLE_CUSTOM_0		= 0x0005,	// 自定义风格0。在AS032中定义为FT。
	FONT_STYLE_CUSTOM_1		= 0x0006,	// 自定义风格1。在AS032中定义为JT。
//; 	FONT_STYLE_TOTAL	= 6,	// 由字体数据所支持的字体风格的总数。此常数固定，不允许再改变。2005-12-30 13:51:16 Chengxinhui  
	FONT_STYLE_TOTAL			= 0x0007,	
	// 由字体数据所支持的字体风格的总数。此常数固定，不允许再改变。2005-12-30 13:51:16 Chengxinhui  
	// 2006-7-2 12:42:00 Chengxinhui  增加字体风格数目。
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
// 定义字体风格的名称。
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
// 系统支持255种字体大小。
// Defined the font size and font size index no.
// 系统最多支持4种大小的字体。其中12、16、24这三种字体是不允许改变的，而且系统必须
// 有12、16得有这两种字体才能运行，其中第4种字体的大小不作限制。
// <Cxh, 2005-1-20 10:36:25> 由于点阵字体的使用需要大量的资源，因此字体大小种类只能是3个，
// 有字体数据支持的字体风格的总数为6种。
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
