//****************************************************************************
//
//	Copyright (C) 2010 ShenZhen ExceedSpace
//
//
//	File name: xmsystemchardef.h
//	  constant，macro & basic typedef definition of system char 
//
//
//****************************************************************************
 /***************************************************************************
  * Description: Defined system special characters code.
  *		SC: System Char.
  * 
  *		 vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
  * NOTE: !!!! DON'T MODIFY THESE DEFINED, MAY ADD !!!!
  *		 ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
  *		
  * MEMO:
  * 
  * Revision history:	
  *        Author            Date                       Cause
  *     ------------  ------------------  -------------------------------------
  *  1. Chengxinhui   2004-12-23 19:35:55  Created.
  *  2. <Cxh, 2005-4-13 14:43:55> 取消“保留区域”的使用。在原来的方案中，IPA和系统符号
		使用的是保留区域，现在改为使用“私有区域”。另外，保留两套IPA的空间。
		Control code:  0xe000 - 0xe0ff (Total: 0x100, 256)
		IPA1: 0xe100 - 0xe1ff (0x100, 256)
		IPA2: 0xe200 - 0xe2ff (0x100, 256)
		System symbol: 0xe300 - 0xe3ff (Total: 0x100, 256)
		Reserve area:  0xe400 - 0xe7ff (Total: 0x400, 1024)
		Common YCODE:  0xe800 - 0xe87f (Total: 0x80, 128)
		Private YCODE: 0xe880 - 0xf8ff (Total: 0x1080, 4224)
	3.0.	2006-5-2 16:34:26 Chengxinhui  修改私有区域的定义。
	3.1.	Unicode标准的私有码区域定义：0xe000 - 0xf8ff
	3.2.	取消IPA1, IPA2, System symbol, Reserve area。
	3.3.	私有码区域从0xe100开始，其中Common YCODE: 0xe100 - 0xe17f,
			Private YCODE: 0xe180 - 0xe2ff。
	3.4.	0xe300 - 0xf8ff转变为系统的标准区段，用于保存汉字，而且能够通过输入法输入此汉字。
	2006-5-17 22:34:12 Chengxinhui  重新定义系统私有区域。
	10.	Unicode的私有区域全部保留，不再用作系统的私有区域。
	20.	系统的私有区域改用0xa900-0xab00这个区段。
	30.	系统控制码区域：0xa900 - 0xa9ff
	40.	私有码区域从0xaa00开始，其中Common YCODE: 0xaa00 - 0xaa7f,
			Private YCODE: 0xab80 - 0xabff。
	<Cxh, 2008-7-30 17:40:51> 
	10. Reserve area: 0x1a00 - 0x1aff 被重新定义为上标、下标、方框数字
	0x1b00 - 0x1bff: 方框、圆圈数字
	0x1c00 - 0x1cff: 特殊字符
  *
  ****************************************************************************/

#ifndef __XM_SYSTEMCHARCODE_H__
#define __XM_SYSTEMCHARCODE_H__

#include <xm_type.h>

#ifdef __cplusplus
extern "C" {
#endif

#define	SOLUTION_3			1

/*
	// 语系：
of the countries or peoples using languages developed from Latin, eg France, Italy, Portugal, Spain 拉丁语系国家或民族的（如法兰西﹑ 意大利﹑ 葡萄牙﹑ 西班牙）: 


 */




/* Unicode characters defined:
* 0x0000 -- 0x001f: Control
* 0x0020 -- 0x007f: Basic Latin
* 0x0080 -- 0x009f: Control
* 0x00a0 -- 0x00ff: Latin-1 Supplement
* 0x0100 -- 0x017f: Latin Extented-A
* 0x0180 -- 0x024f: Latin Extented-B
*/
#define	SC_Control_S		0x0000
#define	SC_Control_E		0x001f
#define	SC_Control_1_S		0x0080
#define	SC_Control_1_E		0x009f
#define	SC_Latin_S			0x0020	// Latin: 拉丁文, 拉丁语, 拉丁语族的
#define	SC_Latin_E			0x007f
#define	SC_Latin_1_S		0x00a0
#define	SC_Latin_1_E		0x00ff
#define	SC_Latin_A_S		0x0100
#define	SC_Latin_A_E		0x017f
#define	SC_Latin_B_S		0x0180
#define	SC_Latin_B_E		0x024f
#define	SC_IPA_S				0x0250	// IPA Extensions
#define	SC_IPA_E				0x02af
#define	SC_SML_S				0x02b0	// Spacing Modifier Letters
#define	SC_SML_E				0x02ff
#define	SC_CDM_S				0x0300	// Combining Diacritical Marks
#define	SC_CDM_E				0x036f
#define	SC_Greek_S			0x0370	// Greek 希腊语
#define	SC_Greek_E			0x03ff
#define	SC_Cyrillic_S		0x0400	// Cyrillic(西里尔字母的): 0x0400 - 0x04ff
#define	SC_Cyrillic_E		0x052f	// Cyrillic Supplement: 0x0500 - 0x052f
#define	SC_Armenian_S		0x0530	// Armenian 亚美尼亚的
#define	SC_Armenian_E		0x058f
#define	SC_Hebrew_S			0x0590	// Hebrew 希伯来人
#define	SC_Hebrew_E			0x05ff
#define	SC_Arabic_S			0x0600	// Arabic 阿拉伯的
#define	SC_Arabic_E			0x06ff
#define	SC_Syriac_S			0x0700	// Syriac 古叙利亚语
#define	SC_Syriac_E			0x074f
#define	SC_Thaana_S			0x0780	// Thaana
#define	SC_Thaana_E			0x07bf
#define	SC_Devanagari_S	0x0900	// Devanagari: 梵文：梵语和许多现代印度语言书写用的字母系统
#define	SC_Devanagari_E	0x097f
#define	SC_Bengali_S		0x0980	// Bengali: 孟加拉的：孟加拉或其人民、语言或文化的或与之相关的
#define	SC_Bengali_E		0x09ff
#define	SC_Gurmukhi_S		0x0a00	// Gurmukhi: 果鲁穆奇语( 即旁遮普语)(亦作Gurumukhi)
#define	SC_Gurmukhi_E		0x0a7f
#define	SC_Gujarati_S		0x0a80	// Gujarati: 古吉特拉语：吉特拉地区使用的印度语言
#define	SC_Gujarati_E		0x0aff
#define	SC_Oriya_S			0x0b00	// Oriya: 奥里雅语：印度东部奥里雅州的一种印度语
#define	SC_Oriya_E			0x0b7f
#define	SC_Tamil_S			0x0b80	// Tamil: 泰米尔人：一支居住在印度南部和斯里兰卡北部的德拉威人的成员
#define	SC_Tamil_E			0x0bff
#define	SC_Telugu_S			0x0c00	// Telugu: 泰卢固语：印度中部使用的德拉威语
#define	SC_Telugu_E			0x0c7f
#define	SC_Kannada_S		0x0c80	// Kannada: 卡纳达语：印度南部一地区迈索尔的主要的达罗
#define	SC_Kannada_E		0x0cff
#define	SC_Malayalam_S		0x0d00	// Malayalam: 马拉雅拉姆语：印度西南马拉巴尔海岸的喀拉拉邦所使用的一种达罗毗荼语
#define	SC_Malayalam_E		0x0d7f
#define	SC_Sinhala_S		0x0d80	// Sinhala: 僧伽罗语
#define	SC_Sinhala_E		0x0dff
#define	SC_Thai_S			0x0e00	// Thai: 泰国人, 泰国语
#define	SC_Thai_E			0x0e7f
#define	SC_Lao_S				0x0e80	// Lao: (=Laotian)老挝人, 老挝语
#define	SC_Lao_E				0x0eff
#define	SC_Tibetan_S		0x0f00	// Tibetan: 西藏语, 西藏人, 藏族人
#define	SC_Tibetan_E		0x0fff
#define	SC_Myanmar_S		0x1000	// Myanmar: 缅甸[东南亚国家](即Burma)
#define	SC_Myanmar_E		0x109f
#define	SC_Georgian_S		0x10a0	// Georgian: 乔治亚州人, 乔治亚人
#define	SC_Georgian_E		0x10ff
#define	SC_HangulJamo_S	0x1100	// Hangul Jamo: 
#define	SC_HangulJamo_E	0x11ff
#define	SC_Ethiopic_S		0x1200	// Ethiopic: 埃塞俄比亚的
#define	SC_Ethiopic_E		0x12ff
//
//
#define	SC_Latin_EA_S		0x1e00	// Latin Extended Additional: 
#define	SC_Latin_EA_E		0x1eff
#define	SC_Greek_E_S		0x1f00	// Latin Extended Additional: 
#define	SC_Greek_E_E		0x1fff

// Hangul Jamo: 0x1100 - 0x11ff
// Hangul Compatibility Jamo: 0x3130 - 0x318f
// Hangul: 0xac00 - 0xd7af
#define SC_Hangul_Jamo_S	0x1100
#define SC_Hangul_Jamo_E	0x11ff
#define SC_Hangul_C_Jamo_S	0x3130
#define SC_Hangul_C_Jamo_E	0x318f
#define SC_Hangul_S			0xac00
#define SC_Hangul_E			0xd7af

#define	SC_BR					0x0a
#define	SC_VOICE_FLAG		0x1c80		// 发音标志
#define	SC_SHIFT_FLAG		0x1c84		// Shift标志。
#define	SC_PHONETIC_S		0x1c00		// 英语音标开始标志字符。
#define	SC_PHONETIC_E		0x1c01		// 英语音标开始标志字符。
#define	SC_PINYIN_S			0x1c02		// 汉语拼音开始标志字符。
#define	SC_PINYIN_E			0x1c03		// 汉语拼音开始标志字符。
#define	SC_REPLACING		0x1c04		// 可替换字符。

// 20100120 ZhuoYongHong
// 增加日文假名注音标志字符
#define SC_KANA_S				0x1c10		// 日文假名注音开始标志字符
#define SC_KANA_E				0x1c11		// 日文假名注音结束标志字符

	// 在辞典中，可能是用0x7e，在处理过的数据中，必须把这个字符用SC_REPLACING替换。
// 10. Surrogate area
#define	SC_HIGH_SUR_AREA_S		(unsigned short)0xd800
#define	SC_HIGH_SUR_AREA_E		(unsigned short)0xdbff
#define	SC_LOW_SUR_AREA_S			(unsigned short)0xdc00
#define	SC_LOW_SUR_AREA_E			(unsigned short)0xdfff
#define	SC_SURROGATE_AREA_S		SC_HIGH_SUR_AREA_S
#define	SC_SURROGATE_AREA_E		SC_LOW_SUR_AREA_E
// 20. IPA code area: 0x0800 - 0x087f. 
/*
	IPA分散在Latin字母中，而且有些与其它的字母共用，
	因此再定义这个统一的区间两者相互补充。
	2006-3-11 11:56:12 Chengxinhui  音标、符号区域暂时未用。
*/
//; ; // 2006-5-2 16:43:38 Chengxinhui  
//; ; #ifndef SOLUTION_3
//; ; #define	SC_IPA_AREA_S				(unsigned short)0xe100
//; ; #define	SC_IPA_AREA_E				(unsigned short)0xe1ff
//; ; #endif
// 30. System symbol code area: 0x0880 - 0x08ff
//; ; // 2006-5-2 16:43:50 Chengxinhui  
//; ; #ifndef SOLUTION_3
//; ; #define	SC_SYMBOL_AREA_S			(unsigned short)0xe300
//; ; #define	SC_SYMBOL_AREA_E			(unsigned short)0xe3ff
//; ; #endif
// 40. System control code area: 0x1c00 - 0x1dff
// 50. 第三次方案：
// System control code area: 0xa900 - 0xa97f
/*
	注意：所有的控制码是必须成对使用，其中“偶数”编码是开始，“奇数”编码是结束。
	另外，所有的控制码类型是“前禁则”或“后禁则”。
*/
#define	SC_CONTROL_AREA_S				(unsigned short)0xa900		// 0xe000
#define	SC_CONTROL_AREA_E				(unsigned short)0xa97f		// 0xe0ff
// 41. Font basicly style code.
#define	SC_FONT_STYLE_AREA_S			(unsigned short)0xa900
#define	SC_FONT_STYLE_AREA_E			(unsigned short)0xa93f
#define	SC_FONT_BOLD_START			(unsigned short)(0xa900+0x00)	// Bold
#define	SC_FONT_BOLD_END				(unsigned short)(0xa900+0x01)
#define	SC_FONT_ITALIC_START			(unsigned short)(0xa900+0x02)	// Italic
#define	SC_FONT_ITALIC_END			(unsigned short)(0xa900+0x03)
#define	SC_FONT_BOLDITALIC_START	(unsigned short)(0xa900+0x04)	// Bold
#define	SC_FONT_BOLDITALIC_END		(unsigned short)(0xa900+0x05)
#define	SC_FONT_FIX_START				(unsigned short)(0xa900+0x06)	// Fixed 
#define	SC_FONT_FIX_END				(unsigned short)(0xa900+0x07)
#define	SC_FONT_CUSTOM_0_START		(unsigned short)(0xa900+0x08)	// Custom.
#define	SC_FONT_CUSTOM_0_END			(unsigned short)(0xa900+0x09)
#define	SC_FONT_CUSTOM_1_START		(unsigned short)(0xa900+0x0a)	// Custom.
#define	SC_FONT_CUSTOM_1_END			(unsigned short)(0xa900+0x0b)
#define	SC_FONT_CUSTOM_2_START		(unsigned short)(0xa900+0x0c)	// Custom.
#define	SC_FONT_CUSTOM_2_END			(unsigned short)(0xa900+0x0d)
// 42. Font special style.
#define	SC_FONT_SEECIAL_STYLE		(unsigned short)(0xa900+0x10)
//; ; //#define	SC_FONT_ITALIC_START		(unsigned short)(0xa900+0x10)	// Italic
//; ; //#define	SC_FONT_ITALIC_END		(unsigned short)(0xa900+0x11)
#define	SC_FONT_UNDERLINE_START		(unsigned short)(0xa910+0x02)	// Underline
#define	SC_FONT_UNDERLINE_END		(unsigned short)(0xa910+0x03)
#define	SC_FONT_STRIKE_START			(unsigned short)(0xa910+0x04)	// Strikethought
#define	SC_FONT_STRIKE_END			(unsigned short)(0xa910+0x05)
#define	SC_FONT_DB_STRIKE_START		(unsigned short)(0xa910+0x06)	// Double strikethought
#define	SC_FONT_DB_STRIKE_END		(unsigned short)(0xa910+0x07)
#define	SC_FONT_BORDER_START			(unsigned short)(0xa910+0x08)	// Border
#define	SC_FONT_BORDER_END			(unsigned short)(0xa910+0x09)
#define	SC_FONT_SHADING_START		(unsigned short)(0xa910+0x0a)	// Shading
#define	SC_FONT_SHADING_END			(unsigned short)(0xa910+0x0b)

// 用于设置不同的字体。在SC_FONT_TYPE_S紧跟一个16位的数据，表示不同类型的字体。
// SC_FONT_TYPE_E表示所设置的字体结束，需要恢复前一种字体。
#define	SC_FONT_TYPE_S					(unsigned short)(0xa930)
#define	SC_FONT_TYPE_E					(unsigned short)(0xa931)

// 42. Control code defined.
#define	SC_COMM_CTRL_START		(unsigned short)(0xa940)
#define	SC_COMM_CTRL_END			(unsigned short)(0xa97f)
#define	SC_COLOR_FORE_START		(unsigned short)(0xa940 + 0x00)	// 颜色开始：紧跟R、G、B的值，每一个值用一个wchar描述。
#define	SC_COLOR_FORE_END			(unsigned short)(0xa940 + 0x01)
#define	SC_COLOR_BACK_START		(unsigned short)(0xa940 + 0x02)
#define	SC_COLOR_BACK_END			(unsigned short)(0xa940 + 0x03)

#define	SC_PICK_START				(unsigned short)(0xa950 + 0x00)	// Pick start mark.
#define	SC_PICK_END					(unsigned short)(0xa950 + 0x01)	// Pick end mark.
#define	SC_NOTPICK_START			(unsigned short)(0xa950 + 0x02)	// Don't allow pick.
#define	SC_NOTPICK_END				(unsigned short)(0xa950 + 0x03)
#define	SC_WORDID_START			(unsigned short)(0xa950 + 0x04)
#define	SC_WORDID_END				(unsigned short)(0xa950 + 0x05)
#define	SC_PAGEID_START			(unsigned short)(0xa950 + 0x06)
#define	SC_PAGEID_END				(unsigned short)(0xa950 + 0x07)
#define	SC_RUBY_START			   (unsigned short)(0xa950 + 0x08)
#define	SC_RUBY_END				   (unsigned short)(0xa950 + 0x09)
#define	SC_RUBYBASE_START			(unsigned short)(0xa950 + 0x0A)
#define	SC_RUBYBASE_END			(unsigned short)(0xa950 + 0x0B)
#define	SC_RUBYTEXT_START			(unsigned short)(0xa950 + 0x0C)
#define	SC_RUBYTEXT_END			(unsigned short)(0xa950 + 0x0D)
#define	SC_WORDWARP_START			(unsigned short)(0xa950 + 0x0E)
#define	SC_WORDWARP_END			(unsigned short)(0xa950 + 0x0F)

#define	SC_SUBBLOCK_START			(unsigned short)(0xa960 + 0x00)	// 子块定义
#define	SC_SUBBLOCK_END			(unsigned short)(0xa960 + 0x01)
// 定义如下
// SC_SUBBLOCK_START 子块24位地址  SC_SUBBLOCK_END

#define	SC_VOICE_START				(unsigned short)(0xa960 + 0x02)	// 语音定义
#define	SC_VOICE_END				(unsigned short)(0xa960 + 0x03)
// 定义如下
// SC_VOICE_START 语言编码16位(参考common_languageid.h) 语音索引编码24位  SC_VOICE_END

#define	SC_LANG_START				(unsigned short)(0xa960 + 0x04)	// 语言定义，用于说明被包含文字的语言类型，用于跳查或TTS
#define	SC_LANG_END					(unsigned short)(0xa960 + 0x05)	//   如英语、马语均使用26个字母。可用SC_LANG_XXX包含马语的文字
// 定义如下
// SC_LANG_START  纯文本  SC_LANG_END

#define	SC_BR_WORD					(unsigned short)0xa97e

// 0xa980 - 0xa9ff: 没有定义。

//; ; #ifndef	SOLUTION_3
// 50. YCode code area: 0xe800 - 0xf8ff
//; ; #define	SC_YCODE_AREA_S			(unsigned short)0xe800
//; ; #define	SC_YCODE_AREA_E			(unsigned short)0xf8ff
//; ; #define	SC_YCODE_TOTAL				(SC_YCODE_AREA_E - SC_YCODE_AREA_S + 1)
//; ; #else
// 50. YCode code area: 0xa980 - 0xa9ff
#define	SC_YCODE_AREA_S			(unsigned short)0xa980
#define	SC_YCODE_AREA_E			(unsigned short)0xabff
#define	SC_YCODE_TOTAL				(SC_YCODE_AREA_E - SC_YCODE_AREA_S + 1)
//; ; #endif
//; ; #ifndef	SOLUTION_3
//; ; // 51. Common defined: 0xe800 - 0xe87f. 这个区段统一定义，每一个码值具有相同的含义，
//; ; //		并且具有相同的宽度，宽度为48，而且所有字体宽度相同。如：0xe800定义为图标。
//; ; #define	SC_YCODE_COMM_S			(unsigned short)0xe800
//; ; #define	SC_YCODE_COMM_E			(unsigned short)(0xe87f)
//; ; #define	SC_LOGO						(unsigned short)(0xe800+0x00)		// 本国语LOGO
//; ; #define	SC_LOGO_FOREIGN			(unsigned short)(0xe800+0x01)		// 外国语LOGO
//; ; #define	SC_LOGO_WIDTH				48		// 
//; ; // 52. Y-Code private area.
//; ; #define	SC_YCODE_PRIVATE_S		(unsigned short)(0xe880)
//; ; #define	SC_YCODE_PRIVATE_E		SC_YCODE_AREA_E
//; ; #else
// 51. Common defined: 0xa980 - 0xa9ff. 这个区段统一定义，每一个码值具有相同的含义，
//		并且具有相同的宽度，宽度为48，而且所有字体宽度相同。如：0xa980定义为图标。
//		此区域编码的字符由外码字库定义，其中0xa980对应外码字库的0x0000。
#define	SC_YCODE_COMM_S			(unsigned short)0xa980
#define	SC_YCODE_COMM_E			(unsigned short)(0xa9ff)
#define	SC_LOGO						(unsigned short)(0xa980+0x00)		// 本国语LOGO
#define	SC_LOGO_FOREIGN			(unsigned short)(0xa980+0x01)		// 外国语LOGO
//		定义正文特征标志，每一个标志4个字符。

#define	SC_LOGO_WIDTH				48		// 
// 52. Y-Code private area.
#define	SC_YCODE_PRIVATE_S		(unsigned short)(0xaa00)
#define	SC_YCODE_PRIVATE_E		SC_YCODE_AREA_E
//; ; #endif

// 100. System symbol: 0xe300 - 0xe3ff (Total: 0x100, 256)
/*
	1、在系统字符中，诸如“例句”、“词性”这类引导字符，不同的数据厂商会有不同的定义，
因此这些字符需要在外码中进行定义。同时也定义为系统字符，是为自己产品的使用，这样便于
统一界面风格。
	2、需要定义的字符有：Reference, Example, Grammar, 
*/
#ifndef	SOLUTION_3
#define	SC_SYMBOL_START			SC_SYMBOL_AREA_S
//#define	SC_SYMBOL_REFEER			
#endif

//
// Defined char code type.
// 类型编号小于SC_DT_VIRTUAL的有相应的字体数据，否则没有相应的字体数据，只是一个标志。
// 有字体数据的编码，则有相应的宽度、高度、点阵数据。
// SC_DT: System Char Display Type.
//

#define	SC_DT_REAL							(BYTE)0x00
#define	SC_DT_NORMAL						(BYTE)0x00
#define	SC_DT_YCODE							(BYTE)0x01
#define	SC_DT_HIGH_SUR						(BYTE)0x02
#define	SC_DT_LOW_SUR						(BYTE)0x03

#define	SC_DT_VIRTUAL						(BYTE)0x80
#define	SC_DT_FONT_STYLE_S				(BYTE)0x80
#define	SC_DT_FONT_STYLE_E				(BYTE)0x81
#define	SC_DT_FONT_SPEC_STYLE_S			(BYTE)0x82
#define	SC_DT_FONT_SPEC_STYLE_E			(BYTE)0x83

#define	SC_DT_COLOR_S						(BYTE)0x84	// 字义字符串的颜色（背景、前景），能否扩展到其它对象？
#define	SC_DT_COLOR_E						(BYTE)0x85	// 字义字符串的颜色（背景、前景），能否扩展到其它对象？
#define	SC_DT_WORDID_S						(BYTE)0x86	// 隐藏控制字符，由此标志定义的字符不显示。
#define	SC_DT_WORDID_E						(BYTE)0x87	// 
#define	SC_DT_FONT_TYPE_S					(BYTE)0x88
#define	SC_DT_FONT_TYPE_E					(BYTE)0x89
#define	SC_DT_PAGEID_S						(BYTE)0x8a
#define	SC_DT_PAGEID_E						(BYTE)0x8b
#define	SC_DT_RUBY_S						(BYTE)0x8c
#define	SC_DT_RUBY_E						(BYTE)0x8d
#define	SC_DT_RUBYBASE_S					(BYTE)0x8e
#define	SC_DT_RUBYBASE_E					(BYTE)0x8f
#define	SC_DT_RUBYTEXT_S					(BYTE)0x90
#define	SC_DT_RUBYTEXT_E					(BYTE)0x91

#define	SC_DT_BR_WORD						(BYTE)0xed
#define	SC_DT_COMM_CTRL_S					(BYTE)0xee
#define	SC_DT_COMM_CTRL_E					(BYTE)0xef
#define	SC_DT_UNI_CTRL						(BYTE)0xf0	// Unicode编码的控制字符。


BYTE syschrCode2Type (WCHAR uCode);

WORD syschrFontCtrlCode2Style (WCHAR wFontCtrlCode);



#ifdef __cplusplus
}
#endif

#endif //__SYSTEMCHARCODE_H__

