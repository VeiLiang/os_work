//****************************************************************************
//
//	Copyright (C) 2010 ShenZhen ExceedSpace
//
//
//	File name: xmsystemchardef.h
//	  constant��macro & basic typedef definition of system char 
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
  *  2. <Cxh, 2005-4-13 14:43:55> ȡ�����������򡱵�ʹ�á���ԭ���ķ����У�IPA��ϵͳ����
		ʹ�õ��Ǳ����������ڸ�Ϊʹ�á�˽�����򡱡����⣬��������IPA�Ŀռ䡣
		Control code:  0xe000 - 0xe0ff (Total: 0x100, 256)
		IPA1: 0xe100 - 0xe1ff (0x100, 256)
		IPA2: 0xe200 - 0xe2ff (0x100, 256)
		System symbol: 0xe300 - 0xe3ff (Total: 0x100, 256)
		Reserve area:  0xe400 - 0xe7ff (Total: 0x400, 1024)
		Common YCODE:  0xe800 - 0xe87f (Total: 0x80, 128)
		Private YCODE: 0xe880 - 0xf8ff (Total: 0x1080, 4224)
	3.0.	2006-5-2 16:34:26 Chengxinhui  �޸�˽������Ķ��塣
	3.1.	Unicode��׼��˽���������壺0xe000 - 0xf8ff
	3.2.	ȡ��IPA1, IPA2, System symbol, Reserve area��
	3.3.	˽���������0xe100��ʼ������Common YCODE: 0xe100 - 0xe17f,
			Private YCODE: 0xe180 - 0xe2ff��
	3.4.	0xe300 - 0xf8ffת��Ϊϵͳ�ı�׼���Σ����ڱ��溺�֣������ܹ�ͨ�����뷨����˺��֡�
	2006-5-17 22:34:12 Chengxinhui  ���¶���ϵͳ˽������
	10.	Unicode��˽������ȫ����������������ϵͳ��˽������
	20.	ϵͳ��˽���������0xa900-0xab00������Ρ�
	30.	ϵͳ����������0xa900 - 0xa9ff
	40.	˽���������0xaa00��ʼ������Common YCODE: 0xaa00 - 0xaa7f,
			Private YCODE: 0xab80 - 0xabff��
	<Cxh, 2008-7-30 17:40:51> 
	10. Reserve area: 0x1a00 - 0x1aff �����¶���Ϊ�ϱꡢ�±ꡢ��������
	0x1b00 - 0x1bff: ����ԲȦ����
	0x1c00 - 0x1cff: �����ַ�
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
	// ��ϵ��
of the countries or peoples using languages developed from Latin, eg France, Italy, Portugal, Spain ������ϵ���һ�����ģ��編�����p ������p �������p ��������: 


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
#define	SC_Latin_S			0x0020	// Latin: ������, ������, ���������
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
#define	SC_Greek_S			0x0370	// Greek ϣ����
#define	SC_Greek_E			0x03ff
#define	SC_Cyrillic_S		0x0400	// Cyrillic(�������ĸ��): 0x0400 - 0x04ff
#define	SC_Cyrillic_E		0x052f	// Cyrillic Supplement: 0x0500 - 0x052f
#define	SC_Armenian_S		0x0530	// Armenian �������ǵ�
#define	SC_Armenian_E		0x058f
#define	SC_Hebrew_S			0x0590	// Hebrew ϣ������
#define	SC_Hebrew_E			0x05ff
#define	SC_Arabic_S			0x0600	// Arabic ��������
#define	SC_Arabic_E			0x06ff
#define	SC_Syriac_S			0x0700	// Syriac ����������
#define	SC_Syriac_E			0x074f
#define	SC_Thaana_S			0x0780	// Thaana
#define	SC_Thaana_E			0x07bf
#define	SC_Devanagari_S	0x0900	// Devanagari: ���ģ����������ִ�ӡ��������д�õ���ĸϵͳ
#define	SC_Devanagari_E	0x097f
#define	SC_Bengali_S		0x0980	// Bengali: �ϼ����ģ��ϼ��������������Ի��Ļ��Ļ���֮��ص�
#define	SC_Bengali_E		0x09ff
#define	SC_Gurmukhi_S		0x0a00	// Gurmukhi: ��³������( ����������)(����Gurumukhi)
#define	SC_Gurmukhi_E		0x0a7f
#define	SC_Gujarati_S		0x0a80	// Gujarati: �ż����������������ʹ�õ�ӡ������
#define	SC_Gujarati_E		0x0aff
#define	SC_Oriya_S			0x0b00	// Oriya: �������ӡ�ȶ����������ݵ�һ��ӡ����
#define	SC_Oriya_E			0x0b7f
#define	SC_Tamil_S			0x0b80	// Tamil: ̩�׶��ˣ�һ֧��ס��ӡ���ϲ���˹�����������ĵ������˵ĳ�Ա
#define	SC_Tamil_E			0x0bff
#define	SC_Telugu_S			0x0c00	// Telugu: ̩¬���ӡ���в�ʹ�õĵ�������
#define	SC_Telugu_E			0x0c7f
#define	SC_Kannada_S		0x0c80	// Kannada: ���ɴ��ӡ���ϲ�һ��������������Ҫ�Ĵ���
#define	SC_Kannada_E		0x0cff
#define	SC_Malayalam_S		0x0d00	// Malayalam: ��������ķ�ӡ�����������Ͷ������Ŀ���������ʹ�õ�һ�ִ�����ݱ��
#define	SC_Malayalam_E		0x0d7f
#define	SC_Sinhala_S		0x0d80	// Sinhala: ɮ٤����
#define	SC_Sinhala_E		0x0dff
#define	SC_Thai_S			0x0e00	// Thai: ̩����, ̩����
#define	SC_Thai_E			0x0e7f
#define	SC_Lao_S				0x0e80	// Lao: (=Laotian)������, ������
#define	SC_Lao_E				0x0eff
#define	SC_Tibetan_S		0x0f00	// Tibetan: ������, ������, ������
#define	SC_Tibetan_E		0x0fff
#define	SC_Myanmar_S		0x1000	// Myanmar: ���[�����ǹ���](��Burma)
#define	SC_Myanmar_E		0x109f
#define	SC_Georgian_S		0x10a0	// Georgian: ����������, ��������
#define	SC_Georgian_E		0x10ff
#define	SC_HangulJamo_S	0x1100	// Hangul Jamo: 
#define	SC_HangulJamo_E	0x11ff
#define	SC_Ethiopic_S		0x1200	// Ethiopic: ��������ǵ�
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
#define	SC_VOICE_FLAG		0x1c80		// ������־
#define	SC_SHIFT_FLAG		0x1c84		// Shift��־��
#define	SC_PHONETIC_S		0x1c00		// Ӣ�����꿪ʼ��־�ַ���
#define	SC_PHONETIC_E		0x1c01		// Ӣ�����꿪ʼ��־�ַ���
#define	SC_PINYIN_S			0x1c02		// ����ƴ����ʼ��־�ַ���
#define	SC_PINYIN_E			0x1c03		// ����ƴ����ʼ��־�ַ���
#define	SC_REPLACING		0x1c04		// ���滻�ַ���

// 20100120 ZhuoYongHong
// �������ļ���ע����־�ַ�
#define SC_KANA_S				0x1c10		// ���ļ���ע����ʼ��־�ַ�
#define SC_KANA_E				0x1c11		// ���ļ���ע��������־�ַ�

	// �ڴǵ��У���������0x7e���ڴ�����������У����������ַ���SC_REPLACING�滻��
// 10. Surrogate area
#define	SC_HIGH_SUR_AREA_S		(unsigned short)0xd800
#define	SC_HIGH_SUR_AREA_E		(unsigned short)0xdbff
#define	SC_LOW_SUR_AREA_S			(unsigned short)0xdc00
#define	SC_LOW_SUR_AREA_E			(unsigned short)0xdfff
#define	SC_SURROGATE_AREA_S		SC_HIGH_SUR_AREA_S
#define	SC_SURROGATE_AREA_E		SC_LOW_SUR_AREA_E
// 20. IPA code area: 0x0800 - 0x087f. 
/*
	IPA��ɢ��Latin��ĸ�У�������Щ����������ĸ���ã�
	����ٶ������ͳһ�����������໥���䡣
	2006-3-11 11:56:12 Chengxinhui  ���ꡢ����������ʱδ�á�
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
// 50. �����η�����
// System control code area: 0xa900 - 0xa97f
/*
	ע�⣺���еĿ������Ǳ���ɶ�ʹ�ã����С�ż���������ǿ�ʼ���������������ǽ�����
	���⣬���еĿ����������ǡ�ǰ���򡱻򡰺���򡱡�
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

// �������ò�ͬ�����塣��SC_FONT_TYPE_S����һ��16λ�����ݣ���ʾ��ͬ���͵����塣
// SC_FONT_TYPE_E��ʾ�����õ������������Ҫ�ָ�ǰһ�����塣
#define	SC_FONT_TYPE_S					(unsigned short)(0xa930)
#define	SC_FONT_TYPE_E					(unsigned short)(0xa931)

// 42. Control code defined.
#define	SC_COMM_CTRL_START		(unsigned short)(0xa940)
#define	SC_COMM_CTRL_END			(unsigned short)(0xa97f)
#define	SC_COLOR_FORE_START		(unsigned short)(0xa940 + 0x00)	// ��ɫ��ʼ������R��G��B��ֵ��ÿһ��ֵ��һ��wchar������
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

#define	SC_SUBBLOCK_START			(unsigned short)(0xa960 + 0x00)	// �ӿ鶨��
#define	SC_SUBBLOCK_END			(unsigned short)(0xa960 + 0x01)
// ��������
// SC_SUBBLOCK_START �ӿ�24λ��ַ  SC_SUBBLOCK_END

#define	SC_VOICE_START				(unsigned short)(0xa960 + 0x02)	// ��������
#define	SC_VOICE_END				(unsigned short)(0xa960 + 0x03)
// ��������
// SC_VOICE_START ���Ա���16λ(�ο�common_languageid.h) ������������24λ  SC_VOICE_END

#define	SC_LANG_START				(unsigned short)(0xa960 + 0x04)	// ���Զ��壬����˵�����������ֵ��������ͣ����������TTS
#define	SC_LANG_END					(unsigned short)(0xa960 + 0x05)	//   ��Ӣ������ʹ��26����ĸ������SC_LANG_XXX�������������
// ��������
// SC_LANG_START  ���ı�  SC_LANG_END

#define	SC_BR_WORD					(unsigned short)0xa97e

// 0xa980 - 0xa9ff: û�ж��塣

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
//; ; // 51. Common defined: 0xe800 - 0xe87f. �������ͳһ���壬ÿһ����ֵ������ͬ�ĺ��壬
//; ; //		���Ҿ�����ͬ�Ŀ�ȣ����Ϊ48������������������ͬ���磺0xe800����Ϊͼ�ꡣ
//; ; #define	SC_YCODE_COMM_S			(unsigned short)0xe800
//; ; #define	SC_YCODE_COMM_E			(unsigned short)(0xe87f)
//; ; #define	SC_LOGO						(unsigned short)(0xe800+0x00)		// ������LOGO
//; ; #define	SC_LOGO_FOREIGN			(unsigned short)(0xe800+0x01)		// �����LOGO
//; ; #define	SC_LOGO_WIDTH				48		// 
//; ; // 52. Y-Code private area.
//; ; #define	SC_YCODE_PRIVATE_S		(unsigned short)(0xe880)
//; ; #define	SC_YCODE_PRIVATE_E		SC_YCODE_AREA_E
//; ; #else
// 51. Common defined: 0xa980 - 0xa9ff. �������ͳһ���壬ÿһ����ֵ������ͬ�ĺ��壬
//		���Ҿ�����ͬ�Ŀ�ȣ����Ϊ48������������������ͬ���磺0xa980����Ϊͼ�ꡣ
//		�����������ַ��������ֿⶨ�壬����0xa980��Ӧ�����ֿ��0x0000��
#define	SC_YCODE_COMM_S			(unsigned short)0xa980
#define	SC_YCODE_COMM_E			(unsigned short)(0xa9ff)
#define	SC_LOGO						(unsigned short)(0xa980+0x00)		// ������LOGO
#define	SC_LOGO_FOREIGN			(unsigned short)(0xa980+0x01)		// �����LOGO
//		��������������־��ÿһ����־4���ַ���

#define	SC_LOGO_WIDTH				48		// 
// 52. Y-Code private area.
#define	SC_YCODE_PRIVATE_S		(unsigned short)(0xaa00)
#define	SC_YCODE_PRIVATE_E		SC_YCODE_AREA_E
//; ; #endif

// 100. System symbol: 0xe300 - 0xe3ff (Total: 0x100, 256)
/*
	1����ϵͳ�ַ��У����硰���䡱�������ԡ����������ַ�����ͬ�����ݳ��̻��в�ͬ�Ķ��壬
�����Щ�ַ���Ҫ�������н��ж��塣ͬʱҲ����Ϊϵͳ�ַ�����Ϊ�Լ���Ʒ��ʹ�ã���������
ͳһ������
	2����Ҫ������ַ��У�Reference, Example, Grammar, 
*/
#ifndef	SOLUTION_3
#define	SC_SYMBOL_START			SC_SYMBOL_AREA_S
//#define	SC_SYMBOL_REFEER			
#endif

//
// Defined char code type.
// ���ͱ��С��SC_DT_VIRTUAL������Ӧ���������ݣ�����û����Ӧ���������ݣ�ֻ��һ����־��
// ���������ݵı��룬������Ӧ�Ŀ�ȡ��߶ȡ��������ݡ�
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

#define	SC_DT_COLOR_S						(BYTE)0x84	// �����ַ�������ɫ��������ǰ�������ܷ���չ����������
#define	SC_DT_COLOR_E						(BYTE)0x85	// �����ַ�������ɫ��������ǰ�������ܷ���չ����������
#define	SC_DT_WORDID_S						(BYTE)0x86	// ���ؿ����ַ����ɴ˱�־������ַ�����ʾ��
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
#define	SC_DT_UNI_CTRL						(BYTE)0xf0	// Unicode����Ŀ����ַ���


BYTE syschrCode2Type (WCHAR uCode);

WORD syschrFontCtrlCode2Style (WCHAR wFontCtrlCode);



#ifdef __cplusplus
}
#endif

#endif //__SYSTEMCHARCODE_H__

