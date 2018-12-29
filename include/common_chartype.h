  //****************************************************************************
  //
  //	Copyright (C) 2010 ShenZhen ExceedSpace
  //
  //****************************************************************************
  /****************************************************************************
  * Description: 
  * 
  * NOTE:
  * 
  * MEMO:
  * 
  * Revision history:	
  *        Author            Date                       Cause
  *     ------------  ------------------  -------------------------------------
  *  1. Chengxinhui   2005-3-25 15:03:25  Created.
  *
  ****************************************************************************/
#ifndef __COMMON_CHARTYPE_H__
#define __COMMON_CHARTYPE_H__

#include <xm_type.h>
#include <common_chardef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct	tagSysCharRecord
{
	WCHAR	mStartCode;		// The start code of char.
	WCHAR	mEndCode;		// The end code of char.
}	TSysCharRecord;

XMBOOL IsChineseChar(WCHAR wc);
XMBOOL IsEnglishChar(WCHAR wc);
XMBOOL IsJapaneseChar(WCHAR wc);
XMBOOL IsTHAIChar(WCHAR wc);
XMBOOL IsSpaceChar(WCHAR wc);

//
// 下面的结构用于描述某一连续字符编码的字符类型。
//
typedef union unCharType
{
	struct tagObj
	{
		BYTE	mCharType;		// Character type. 类型的主要特性。
		BYTE	m2ndType;		// 用作第二类型的定义。
		WORD	mAttr;			// 用作特殊属性的定义。保留未用。
	}	mo;
	DWORD		mdw;
}	TCharType;
typedef struct tagCharTypeTbl
{
	WCHAR			mStartCode;		// Start code.
	WCHAR			mEndCode;		// End code.
	TCharType	moType;
}	TCharTypeTbl;
typedef struct tagCharTypeParam
{
	int		mSysRcdTotal;
	int		mYCodeRcdTotal;
	const TCharTypeTbl		*mpSys;
	const TCharTypeTbl		*mpYCode;
}	TCharTypeParam;
/*
//
// Wordwrap char type defined.
//
	1. WWCT_NORMAL为缺省类型，因此在表中没有进行定义。
	2. 由于一个拼音是一个独立的单元，而且只能与小写字母联合使用，
		因此定义为英语类型。
	3. 有几个字符具有多种特性：
		. (0x002e, 0xff0e): 当用作英语标点时，是前禁则，而用作小数点时，则与数字构成一个整体。
		, (0x002c, 0xff0c): 用作标点符号时是前禁则，而用作数字分段符时，则与数字构成一个整体。
		对这类字符，定义时定义为禁则，在数据中再对此字符进行单独判断。
	4.	2006-5-18 15:34:56 Chengxinhui  由于需要手工编制命令文件，因此下表各枚举值固定。
*/
typedef enum enumWwCharType
{
	WWCT_NULL			= 0x00,		// Terminate flag(0x0000) type.
	WWCT_CTRL			= 0x01,		// Control char in unicode spec.
	WWCT_ENTER			= 0x02,		// Enter. 0x0a
	WWCT_FBD_BGN		= 0x03,		// Forbid the beginning of line.
	WWCT_FBD_END		= 0x04,		// Forbid the end of line.
	WWCT_SPACE			= 0x05,		// White char(Space). 前禁则。
	WWCT_D_QUOT			= 0x06,		// " (English, ASCII) Double quotation mark.
	WWCT_S_QUOT			= 0x07,		// ' (English, ASCII) Single quotation mark.
	WWCT_DIGIT			= 0x08,		// 0 - 9 (English, ASCII) Digit character.
	WWCT_LETTER			= 0x09,		// A-Z, a-z English letter. （包括汉语拼音）
	WWCT_SYMBOL			= 0x0a,		// General symbol. 使用定义系统的控制码。
	WWCT_MARK			= 0x0b,		// Tag mark in MyPalm system.
	WWCT_BPMF			= 0x0c,		// Bopomofo.
	WWCT_CHN				= 0x0d,		// Chinese character. All CJK.
	WWCT_KOR				= 0x0e,		// Hangul. Korean
	WWCT_JPN				= 0x0f,		// Japanese character.
	WWCT_NORMAL			= 0x10,		// Normal char type. 允许以独立的单元，缺省的类型定义。
											// 因此没有定义的类型即为正常类型。
	WWCT_HI_SURR		= 0x11,		// High Surrogates.
	WWCT_PRI_HI_SURR	= 0x12,		// Private High Surrogates.
	WWCT_LO_SURR		= 0x13,		// Low Surrogates.
	// Color control.
	WWCT_COLOR			= 0x14,		//	Color control code.
	// %
	WWCT_PERCENT		= 0x15,		// 0x25 '%'
	// 
	WWCT_HIDE			= 0x16,		// Hide character type.
	// 字符的第二类型的定义：
	WWCT_2_DOT			= 0x17,		// . (English, ASCII) Full stop.
	WWCT_2_COMMA		= 0x18,		// , (English, ASCII) Comma.
	// 2006-6-23 16:01:11 Chengxinhui  
	WWCT_HYPHEN			= 0x19,		// hyphen: 0x2d(-)
	WWCT_SYLLABLE		= 0x1a,		// Syllable: 0xb7
	WWCT_CURRENCY		= 0x1b,		// Currency: 0x24, 0xa3, 0xa5, 0x20a0 - 0x20cf, 
	WWCT_FONT_TYPE		= 0x1c,		// 字体类型。

	WWCT_PAGEID			= 0x1d,
	WWCT_RUBY			= 0x1e,
	WWCT_PICK			= 0x1f,		// Pick unit.
	WWCT_SLASH			= 0x20,		// 0x2f, ff0f, slash
	WWCT_PAREN			= 0x21,		// Parenthesis
   WWCT_WORDWRAP     = 0x22,     // Word warp unit .
   WWCT_NOTPICK      = 0x23,     // not be pick
   WWCT_ARABIC       = 0x24,     // Aribic 
	WWCT_TOTAL			= 0x30		// 允许定义字符类型的个数。
}	TWwCharType;

void SetWwCharTypeTbl(TCharTypeParam *pPar, const TCharTypeTbl *pSysCharType, int nRecordTotal);
void SetWwYCodeCharTypeTbl(TCharTypeParam *pPar, const TCharTypeTbl *pYCodeCharType, int nRecordTotal);
/*******************************************************************
 * Function name: InitWordwrapCharTypeTbl
 * Description  : 
		在使用系统字符类型函数时，必须被初始化，这个函数可以被多次调用。
		如果参数为空，则使用系统缺省的定义。
		注意：所定义的字符类型表必须按升幂排序（即A－Z顺序）。
 * Return type  : int : MP_OK is success, otherwise failure.  
 * Argument     : TCharTypeTbl *pSysCharType : 系统字符类型表。
 * Argument     : int nSysCharRcdNum
 * Argument     : TCharTypeTbl *pYCodeCharType : 外码字符类型表。
  * Argument     : int nYCodeRcdNum
*  ----------------------------------------------------------------
 *          Log       *    Author    *   Casue                      
 *  ----------------------------------------------------------------
 *  2005-3-25 14:30:49    Chengxinhui   Created
 * 
 ******************************************************************/
int InitWordwrapCharTypeTbl(TCharTypeParam *pPar, TCharTypeTbl *pSysCharType, int nSysCharRcdNum, TCharTypeTbl *pYCodeCharType, int nYCodeRcdNum);
/*******************************************************************
 * Function name: AnalysisWordwrapCharType
 * Description  : 
 * Return type  : int : The char total, include the terminate flag.
 * Argument     : const WCHAR *pwsStr
 * Argument     : WORD *pwCharType
 *  ----------------------------------------------------------------
 *          Log       *    Author    *   Casue                      
 *  ----------------------------------------------------------------
 *  2005-3-25 22:03:54    Chengxinhui   Created
 * 
 ******************************************************************/
int AnalysisWordwrapCharType(TCharTypeParam *pPar, const WCHAR *pwsStr, TCharType *poCharType);
//
/////////////////////////////////////////////////////////////////////////////////////
//
//
//
// Word pick char type defined.
//
typedef enum enumPickCharType
{
	PKCT_NULL		= 0x00,		// Terminate flag(0x0000) type.
	PKCT_CHN			= 0x01,		// Chinese character. All CJK.
	PKCT_ENG			= 0x02,		// English character.
	PKCT_KOR			= 0x03,		// Korean 
	PKCT_JPN			= 0x04,		// Japanese character.
	PKCT_PINYIN		= 0x05,		// Chinese Pinyin.
	PKCT_L_PAREN	= 0x06,		// '('
	PKCT_R_PAREN	= 0x07,		// ')'
	PKCT_HYPHEN		= 0x08,		// '-'
	PKCT_DOT			= 0x09,		// '.'
	PKCT_S_QUOT		= 0x0a,		// '			Single quote.Apostrophe quote.
	PKCT_D_QUOT		= 0x0b,		// '			Double quote.
	PKCT_SEPA		= 0x0c,		// Separator. Include: control code, ' '.
	PKCT_COLOR		= 0x0d,		// 
	PKCT_PICK		= 0x0e,		// 允许PICK的单元
	PKCT_NOTPICK	= 0x0f,		// 不允许PICK的单元
	PKCT_WORDID		= 0x10,		// WORDID
	PKCT_FBD_BGN	= 0x11,		// 前禁则
	PKCT_FBD_END	= 0x12,		// 后禁则
	PKCT_SYL_MARK	= 0x13,		// Syllable mark(音节符号)
	PKCT_HI_SURROGATE	= 0x14,	// High Surrogate.
	PKCT_LO_SURROGATE	= 0x15,	// Low Surrogate.

	PKCT_RUBY		= 0x16,		// 
	PKCT_PAGEID		= 0x17,		// Page ID.

	PKCT_FONT_TYPE	= 0x18,		// 
	PKCT_ARABIC 	= 0x19,		// 
	PKCT_CYRILLIC	= 0x1a,		// Cyrillic (Russia)
	PKCT_TIBETAN	= 0x1b,		// tibetan

	PKCT_TOTAL		= 0x20		// 允许定义字符类型的个数。
}	TPickCharType;

void SetPickCharTypeTbl(TCharTypeParam *pPar, const TCharTypeTbl *pSysCharType, int nRecordTotal);
void SetPickYCodeCharTypeTbl(TCharTypeParam *pPar, const TCharTypeTbl *pYCodeCharType, int nRecordTotal);
int InitPickCharTypeTbl(TCharTypeParam *pPar, TCharTypeTbl *pSysCharType, int nSysCharRcdNum, TCharTypeTbl *pYCodeCharType, int nYCodeRcdNum);
int AnalysisPickCharType(TCharTypeParam *pPar, const WCHAR *pwsStr, TCharType *poCharType);

/*******************************************************************
 * Function name: PunctU16ToAsc
 * Description  : 标点符号的转换：从Utf16到Ascii。
 * Return type  : int : 错误码，MP_OK表示转换成功，否则转换失败。
 * Argument     : WCHAR wcU16
 * Argument     : char *cAsc
 *  ----------------------------------------------------------------
 *          Log       *    Author    *   Casue                      
 *  ----------------------------------------------------------------
 *  2005-8-29 11:05:31    Chengxinhui   Created
 * 
 ******************************************************************/
int PunctU16ToAsc(WCHAR wcU16, char *cAsc);

typedef enum enumPinyinCharType
{
	PYCT_NULL		= 0x00,		// Terminate flag(0x0000) type.
	PYCT_ENG			= 0x01,		// English character.
	PYCT_PINYIN		= 0x02,		// āáǎàōóǒòêēéěèīíǐìūúǔùǖǘǚǜü
	PYCT_SPACE		= 0x03,		// 
	PYCT_L_PAREN	= 0x04,		// '('
	PYCT_R_PAREN	= 0x05,		// ')'
	PYCT_SEPA		= 0x06		// Separator. Include: control code, ' '.
}	TPinyinCharType;

int AnalysisPinyinCharType(const WCHAR *pwsStr, TCharType *poCharType);

//
// 校正标点类型转换。
//
typedef enum enumRevisionPunctCharType
{
	RPCT_NULL	= 0,
	RPCT_ENG	,		// 英文字母
	RPCT_ASCII,		// ASCII字符
	RPCT_DIGIT,		// 英文数字
//	RPCT_PUNCT,		// 标点符号，包括英文、中文标点
	RPCT_PREFIX,	// 可以作为一个英文单元的“前缀”，如：(，[，{，（，｛，［
	RPCT_SUFFIX,	// 可以作为一个英文单元的“后缀”，如：)，]，}，），｝，］
	RPCT_PINYIN,	// 汉语拼音
	RPCT_SEPA		// 分隔符
}	TRevisionPunctCharType;

int RevisionPunctCharType(const WCHAR *pwsStr, TCharType *poCharType);

XMBOOL IsPunctuationChar(WCHAR wc);
XMBOOL IsLatinChar(WCHAR wc);
XMBOOL IsArabicChar(WCHAR wc);


/*******************************************************************
 * Function name: SysChar2LangID
 * Description  : 
 * Return type  : WORD : 返回语言ID，在LanguageID.h中定义。
 * Argument     : WCHAR wcChar
 *  ----------------------------------------------------------------
 *          Log       *    Author    *   Casue                      
 *  ----------------------------------------------------------------
 *  2006-12-30 15:26:42    Chengxinhui   Created
 * 
 ******************************************************************/
WORD SysChar2LangID(WCHAR wcChar);

#ifdef __cplusplus
}
#endif

#endif //__COMMON_SYSTEMCHARTYPE_H__
