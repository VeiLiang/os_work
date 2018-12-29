 /***************************************************************************
  *
  *   Created:   2004-12-23 19:35:55
  *   Author:    Chengxinhui
  *
  ****************************************************************************
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
  *
  ****************************************************************************/
#include <xm_type.h>
#include <common_fontconst.h>
#include <common_chardef.h>
#include <common_languageid.h>


// 2005-11-4 14:54:33    Chengxinhui 修改字体基本风格，Italic由“特殊风格”改为“基本风格”。
static const WORD gwFntCtrl2Style[0x0040]	=
{
	// 0x00
	// Basically style.
	FONT_STYLE_BOLD,			FONT_STYLE_BOLD,					// 0x00	-> SC_CONTROL_AREA_S			(unsigned short)0xa900
	FONT_STYLE_ITALIC,		FONT_STYLE_ITALIC,				// 0x02
	FONT_STYLE_BOLDITALIC,	FONT_STYLE_BOLDITALIC,			// 0x04
	FONT_STYLE_FIXED,			FONT_STYLE_FIXED,					// 0x06
	FONT_STYLE_CUSTOM_0,		FONT_STYLE_CUSTOM_0,				// 0x08
//; //	FONT_STYLE_CUSTOM_1,		FONT_STYLE_CUSTOM_1,
//; //	FONT_STYLE_CUSTOM_2,		FONT_STYLE_CUSTOM_2,
	// Basically style reserve.
//; ; //	FONT_STYLE_NORMAL,		FONT_STYLE_NORMAL,
	FONT_STYLE_NORMAL,		FONT_STYLE_NORMAL,				// 0x0a
	FONT_STYLE_NORMAL,		FONT_STYLE_NORMAL,				// 0x0c
	FONT_STYLE_NORMAL,		FONT_STYLE_NORMAL,				// 0x0e
	// 0x10
	// Special style.
//; ; //	FONT_SPEC_STYLE_ITALIC,		FONT_SPEC_STYLE_ITALIC,
	FONT_STYLE_NORMAL,		FONT_STYLE_NORMAL,				// 0x10
	FONT_SPEC_STYLE_UNDERLINE,	FONT_SPEC_STYLE_UNDERLINE,	// 0x12
	FONT_SPEC_STYLE_STRIKE,		FONT_SPEC_STYLE_STRIKE,
	FONT_SPEC_STYLE_DB_STRIKE,	FONT_SPEC_STYLE_DB_STRIKE,
	FONT_SPEC_STYLE_BORDOR,		FONT_SPEC_STYLE_BORDOR,
	FONT_SPEC_STYLE_SHADING,	FONT_SPEC_STYLE_SHADING,
	// Special style reserve.
	FONT_STYLE_NORMAL,		FONT_STYLE_NORMAL,
	FONT_STYLE_NORMAL,		FONT_STYLE_NORMAL,
	// 0x20
	FONT_STYLE_NORMAL,		FONT_STYLE_NORMAL,
	FONT_STYLE_NORMAL,		FONT_STYLE_NORMAL,
	FONT_STYLE_NORMAL,		FONT_STYLE_NORMAL,
	FONT_STYLE_NORMAL,		FONT_STYLE_NORMAL,
	FONT_STYLE_NORMAL,		FONT_STYLE_NORMAL,
	FONT_STYLE_NORMAL,		FONT_STYLE_NORMAL,
	FONT_STYLE_NORMAL,		FONT_STYLE_NORMAL,
	FONT_STYLE_NORMAL,		FONT_STYLE_NORMAL,
	// 0x30
	FONT_STYLE_NORMAL,		FONT_STYLE_NORMAL,
	FONT_STYLE_NORMAL,		FONT_STYLE_NORMAL,
	FONT_STYLE_NORMAL,		FONT_STYLE_NORMAL,
	FONT_STYLE_NORMAL,		FONT_STYLE_NORMAL,
	FONT_STYLE_NORMAL,		FONT_STYLE_NORMAL,
	FONT_STYLE_NORMAL,		FONT_STYLE_NORMAL,
	FONT_STYLE_NORMAL,		FONT_STYLE_NORMAL,
	FONT_STYLE_NORMAL,		FONT_STYLE_NORMAL,
};

WORD syschrFontCtrlCode2Style (WCHAR wFontCtrlCode)
{
	const WORD*	pFntCtrl2Style;
	WORD	wStyle;

	pFntCtrl2Style = gwFntCtrl2Style;

	wStyle = pFntCtrl2Style[wFontCtrlCode - SC_FONT_STYLE_AREA_S];

	return wStyle;
}

static const BYTE gbyCharType[128]	=
{
	// Font style: 0xa900 - 0xa93f.
	// 0x00 - 0x0f
	SC_DT_FONT_STYLE_S, SC_DT_FONT_STYLE_E, SC_DT_FONT_STYLE_S, SC_DT_FONT_STYLE_E, SC_DT_FONT_STYLE_S, SC_DT_FONT_STYLE_E, SC_DT_FONT_STYLE_S, SC_DT_FONT_STYLE_E, 
	SC_DT_FONT_STYLE_S, SC_DT_FONT_STYLE_E, SC_DT_FONT_STYLE_S, SC_DT_FONT_STYLE_E, SC_DT_FONT_STYLE_S, SC_DT_FONT_STYLE_E, SC_DT_FONT_STYLE_S, SC_DT_FONT_STYLE_E, 
	// 0x10 - 0x1f
	SC_DT_FONT_SPEC_STYLE_S, SC_DT_FONT_SPEC_STYLE_E, SC_DT_FONT_SPEC_STYLE_S, SC_DT_FONT_SPEC_STYLE_E, SC_DT_FONT_SPEC_STYLE_S, SC_DT_FONT_SPEC_STYLE_E, SC_DT_FONT_SPEC_STYLE_S, SC_DT_FONT_SPEC_STYLE_E, 
	SC_DT_FONT_SPEC_STYLE_S, SC_DT_FONT_SPEC_STYLE_E, SC_DT_FONT_SPEC_STYLE_S, SC_DT_FONT_SPEC_STYLE_E, SC_DT_FONT_SPEC_STYLE_S, SC_DT_FONT_SPEC_STYLE_E, SC_DT_FONT_SPEC_STYLE_S, SC_DT_FONT_SPEC_STYLE_E, 
	// 0x20 - 0x2f
	SC_DT_FONT_SPEC_STYLE_S, SC_DT_FONT_SPEC_STYLE_E, SC_DT_FONT_SPEC_STYLE_S, SC_DT_FONT_SPEC_STYLE_E, SC_DT_FONT_SPEC_STYLE_S, SC_DT_FONT_SPEC_STYLE_E, SC_DT_FONT_SPEC_STYLE_S, SC_DT_FONT_SPEC_STYLE_E, 
	SC_DT_FONT_SPEC_STYLE_S, SC_DT_FONT_SPEC_STYLE_E, SC_DT_FONT_SPEC_STYLE_S, SC_DT_FONT_SPEC_STYLE_E, SC_DT_FONT_SPEC_STYLE_S, SC_DT_FONT_SPEC_STYLE_E, SC_DT_FONT_SPEC_STYLE_S, SC_DT_FONT_SPEC_STYLE_E, 
	// 0x30 - 0x3f
	SC_DT_FONT_TYPE_S, SC_DT_FONT_TYPE_E,					SC_DT_FONT_SPEC_STYLE_S, SC_DT_FONT_SPEC_STYLE_E, SC_DT_FONT_SPEC_STYLE_S, SC_DT_FONT_SPEC_STYLE_E, SC_DT_FONT_SPEC_STYLE_S, SC_DT_FONT_SPEC_STYLE_E, 
	SC_DT_FONT_SPEC_STYLE_S, SC_DT_FONT_SPEC_STYLE_E, SC_DT_FONT_SPEC_STYLE_S, SC_DT_FONT_SPEC_STYLE_E, SC_DT_FONT_SPEC_STYLE_S, SC_DT_FONT_SPEC_STYLE_E, SC_DT_FONT_SPEC_STYLE_S, SC_DT_FONT_SPEC_STYLE_E, 
	// Common control code: 0xa940 - 0xa94f.
	// 0x40 - 0x4f
	// Color control code: 0xa940 - 0xa943
	SC_DT_COLOR_S, SC_DT_COLOR_E, SC_DT_COLOR_S, SC_DT_COLOR_E, 
	// 0xa944 - 0xa94f
	SC_DT_COMM_CTRL_S, SC_DT_COMM_CTRL_E, SC_DT_COMM_CTRL_S, SC_DT_COMM_CTRL_E, 
	SC_DT_COMM_CTRL_S, SC_DT_COMM_CTRL_E, SC_DT_COMM_CTRL_S, SC_DT_COMM_CTRL_E, SC_DT_COMM_CTRL_S, SC_DT_COMM_CTRL_E, SC_DT_COMM_CTRL_S, SC_DT_COMM_CTRL_E, 
	// Common control code: 0xa960 - 0xa9ff.
	// 0xa950 - 0xa953
	SC_DT_COMM_CTRL_S, SC_DT_COMM_CTRL_E, SC_DT_COMM_CTRL_S, SC_DT_COMM_CTRL_E, 
	// 0xa954 - 0xa957
	SC_DT_WORDID_S		, SC_DT_WORDID_E	, SC_DT_PAGEID_S, SC_DT_PAGEID_E, 
	// 0xa958 - 0xa95f
	SC_DT_RUBY_S, SC_DT_RUBY_E, SC_DT_RUBYBASE_S, SC_DT_RUBYBASE_E, SC_DT_RUBYTEXT_S, SC_DT_RUBYTEXT_E, 
	// 0xa95e - 0xa95f
	SC_DT_COMM_CTRL_S, SC_DT_COMM_CTRL_E, 
	// 0x60 - 0x6f
	SC_DT_COMM_CTRL_S, SC_DT_COMM_CTRL_E, SC_DT_COMM_CTRL_S, SC_DT_COMM_CTRL_E, SC_DT_COMM_CTRL_S, SC_DT_COMM_CTRL_E, SC_DT_COMM_CTRL_S, SC_DT_COMM_CTRL_E, 
	SC_DT_COMM_CTRL_S, SC_DT_COMM_CTRL_E, SC_DT_COMM_CTRL_S, SC_DT_COMM_CTRL_E, SC_DT_COMM_CTRL_S, SC_DT_COMM_CTRL_E, SC_DT_COMM_CTRL_S, SC_DT_COMM_CTRL_E, 
	// 0x70 - 0x7f
	SC_DT_COMM_CTRL_S, SC_DT_COMM_CTRL_E, SC_DT_COMM_CTRL_S, SC_DT_COMM_CTRL_E, SC_DT_COMM_CTRL_S, SC_DT_COMM_CTRL_E, SC_DT_COMM_CTRL_S, SC_DT_COMM_CTRL_E, 
	SC_DT_COMM_CTRL_S, SC_DT_COMM_CTRL_E, SC_DT_COMM_CTRL_S, SC_DT_COMM_CTRL_E, SC_DT_COMM_CTRL_S, SC_DT_COMM_CTRL_E, SC_DT_BR_WORD,		 SC_DT_COMM_CTRL_E, 
//; ; 	// 0x80 - 0x8f
//; ; 	SC_DT_COMM_CTRL_S, SC_DT_COMM_CTRL_E, SC_DT_COMM_CTRL_S, SC_DT_COMM_CTRL_E, SC_DT_COMM_CTRL_S, SC_DT_COMM_CTRL_E, SC_DT_COMM_CTRL_S, SC_DT_COMM_CTRL_E, 
//; ; 	SC_DT_COMM_CTRL_S, SC_DT_COMM_CTRL_E, SC_DT_COMM_CTRL_S, SC_DT_COMM_CTRL_E, SC_DT_COMM_CTRL_S, SC_DT_COMM_CTRL_E, SC_DT_COMM_CTRL_S, SC_DT_COMM_CTRL_E, 
//; ; 	// 0x90 - 0x9f
//; ; 	SC_DT_COMM_CTRL_S, SC_DT_COMM_CTRL_E, SC_DT_COMM_CTRL_S, SC_DT_COMM_CTRL_E, SC_DT_COMM_CTRL_S, SC_DT_COMM_CTRL_E, SC_DT_COMM_CTRL_S, SC_DT_COMM_CTRL_E, 
//; ; 	SC_DT_COMM_CTRL_S, SC_DT_COMM_CTRL_E, SC_DT_COMM_CTRL_S, SC_DT_COMM_CTRL_E, SC_DT_COMM_CTRL_S, SC_DT_COMM_CTRL_E, SC_DT_COMM_CTRL_S, SC_DT_COMM_CTRL_E, 
//; ; 	// 0xa0 - 0xaf
//; ; 	SC_DT_COMM_CTRL_S, SC_DT_COMM_CTRL_E, SC_DT_COMM_CTRL_S, SC_DT_COMM_CTRL_E, SC_DT_COMM_CTRL_S, SC_DT_COMM_CTRL_E, SC_DT_COMM_CTRL_S, SC_DT_COMM_CTRL_E, 
//; ; 	SC_DT_COMM_CTRL_S, SC_DT_COMM_CTRL_E, SC_DT_COMM_CTRL_S, SC_DT_COMM_CTRL_E, SC_DT_COMM_CTRL_S, SC_DT_COMM_CTRL_E, SC_DT_COMM_CTRL_S, SC_DT_COMM_CTRL_E, 
//; ; 	// 0xb0 - 0xbf
//; ; 	SC_DT_COMM_CTRL_S, SC_DT_COMM_CTRL_E, SC_DT_COMM_CTRL_S, SC_DT_COMM_CTRL_E, SC_DT_COMM_CTRL_S, SC_DT_COMM_CTRL_E, SC_DT_COMM_CTRL_S, SC_DT_COMM_CTRL_E, 
//; ; 	SC_DT_COMM_CTRL_S, SC_DT_COMM_CTRL_E, SC_DT_COMM_CTRL_S, SC_DT_COMM_CTRL_E, SC_DT_COMM_CTRL_S, SC_DT_COMM_CTRL_E, SC_DT_COMM_CTRL_S, SC_DT_COMM_CTRL_E, 
//; ; 	// 0xc0 - 0xcf
//; ; 	SC_DT_COMM_CTRL_S, SC_DT_COMM_CTRL_E, SC_DT_COMM_CTRL_S, SC_DT_COMM_CTRL_E, SC_DT_COMM_CTRL_S, SC_DT_COMM_CTRL_E, SC_DT_COMM_CTRL_S, SC_DT_COMM_CTRL_E, 
//; ; 	SC_DT_COMM_CTRL_S, SC_DT_COMM_CTRL_E, SC_DT_COMM_CTRL_S, SC_DT_COMM_CTRL_E, SC_DT_COMM_CTRL_S, SC_DT_COMM_CTRL_E, SC_DT_COMM_CTRL_S, SC_DT_COMM_CTRL_E, 
//; ; 	// 0xd0 - 0xdf
//; ; 	SC_DT_COMM_CTRL_S, SC_DT_COMM_CTRL_E, SC_DT_COMM_CTRL_S, SC_DT_COMM_CTRL_E, SC_DT_COMM_CTRL_S, SC_DT_COMM_CTRL_E, SC_DT_COMM_CTRL_S, SC_DT_COMM_CTRL_E, 
//; ; 	SC_DT_COMM_CTRL_S, SC_DT_COMM_CTRL_E, SC_DT_COMM_CTRL_S, SC_DT_COMM_CTRL_E, SC_DT_COMM_CTRL_S, SC_DT_COMM_CTRL_E, SC_DT_COMM_CTRL_S, SC_DT_COMM_CTRL_E, 
//; ; 	// 0xa9 - 0xef
//; ; 	SC_DT_COMM_CTRL_S, SC_DT_COMM_CTRL_E, SC_DT_COMM_CTRL_S, SC_DT_COMM_CTRL_E, SC_DT_COMM_CTRL_S, SC_DT_COMM_CTRL_E, SC_DT_COMM_CTRL_S, SC_DT_COMM_CTRL_E, 
//; ; 	SC_DT_COMM_CTRL_S, SC_DT_COMM_CTRL_E, SC_DT_COMM_CTRL_S, SC_DT_COMM_CTRL_E, SC_DT_COMM_CTRL_S, SC_DT_COMM_CTRL_E, SC_DT_COMM_CTRL_S, SC_DT_COMM_CTRL_E, 
//; ; 	// 0xf0 - 0xff
//; ; 	SC_DT_COMM_CTRL_S, SC_DT_COMM_CTRL_E, SC_DT_COMM_CTRL_S, SC_DT_COMM_CTRL_E, SC_DT_COMM_CTRL_S, SC_DT_COMM_CTRL_E, SC_DT_COMM_CTRL_S, SC_DT_COMM_CTRL_E, 
//; ; 	SC_DT_COMM_CTRL_S, SC_DT_COMM_CTRL_E, SC_DT_COMM_CTRL_S, SC_DT_COMM_CTRL_E, SC_DT_COMM_CTRL_S, SC_DT_COMM_CTRL_E, SC_DT_COMM_CTRL_S, SC_DT_COMM_CTRL_E, 
};

BYTE syschrCode2Type (WCHAR uCode)
{
	BYTE	uType;
	const BYTE *pbyCharType;

	uType	= SC_DT_NORMAL;
	pbyCharType = gbyCharType;

	if (uCode >= SC_CONTROL_AREA_S)
	{
		if (uCode <= SC_CONTROL_AREA_E)
		{
			// 0xa900 - 0xa97f		// 0xa900 - 0xa9ff
			uType	= pbyCharType[uCode - SC_CONTROL_AREA_S];
		}
		// 0xe100 - 0xe7ff: 没有定义的控制码。
		//#define	SC_YCODE_AREA_S			(unsigned short)0xe100
		//#define	SC_YCODE_AREA_E			(unsigned short)0xe2ff
		//#define	SC_YCODE_TOTAL				(SC_YCODE_AREA_E - SC_YCODE_AREA_S + 1)
		else if ((uCode >= SC_YCODE_AREA_S) && (uCode <= SC_YCODE_AREA_E))
		{
			// 0xe800 - 0xf8ff
			uType	= SC_DT_YCODE;
		}
		else if ((uCode >= SC_SURROGATE_AREA_S) && (uCode <= SC_SURROGATE_AREA_E))
		{
			if (uCode < SC_LOW_SUR_AREA_S)
				uType	= SC_DT_HIGH_SUR;
			else
				uType	= SC_DT_LOW_SUR;
		}
		else if (uCode > 0xffee)
		{
			uType	= SC_DT_UNI_CTRL;
		}
		// 0xe300 - 0xffed		Normal character.
	}

	return uType;
}

/*******************************************************************
 * Function name: syschrSurrogate2Code
 * Description  : 把一个UNICODE的代理对转换为一个字符编码。
						调用者必须保证wcHiSurr和wcLoSurr满足要求。
 * Return type  : int : 错误码，MP_OK表示正确，否则表示错误。
 * Argument     : wchar wcHiSurr : 代理对的高位部分，0xd800 - 0xdbff
 * Argument     : wchar wcLoSurr : 代理对的低位部分，0xdc00 - 0xdfff
 * Argument     : uint *puCode : 字符编码，如果错误，设置为0x7f，表示错误。
 *  ----------------------------------------------------------------
 *          Log       *    Author    *   Casue                      
 *  ----------------------------------------------------------------
 *  2006-8-31 10:12:30    Chengxinhui   Created
 * 
 ******************************************************************/
VOID syschrSurrogate2Code (WCHAR wcHiSurr, WCHAR wcLoSurr, DWORD *puCode)
{
	WORD	uHi, uHi1, uHi2;
	DWORD	uCode;

	uHi	= (WORD)(wcHiSurr - SC_HIGH_SUR_AREA_S);
	uHi1	= (WORD)(uHi >> 6);		// uHi / 0x40
	uHi2	= (WORD)(uHi & 0x3f);	// uHi

	uCode	= (uHi1 + 1) << 16;	// * 0x10000
	uCode	+= (uHi2 << 10);	// uHi2 * 1024
	uCode	+= (wcLoSurr - SC_LOW_SUR_AREA_S);

	*puCode	= uCode;
}

/*******************************************************************
 * Function name: syschrSurrogate2CodeEx
 * Description  : 把一个UNICODE的代理对转换为一个字符编码。
						首先验证wcHiSurr和wcLoSurr是否满足要求。
 * Return type  : int : 错误码，MP_OK表示正确，否则表示错误。
 * Argument     : wchar wcHiSurr : 代理对的高位部分，0xd800 - 0xdbff
 * Argument     : wchar wcLoSurr : 代理对的低位部分，0xdc00 - 0xdfff
 * Argument     : uint *puCode : 字符编码，如果错误，设置为0x7f，表示错误。
 *  ----------------------------------------------------------------
 *          Log       *    Author    *   Casue                      
 *  ----------------------------------------------------------------
 *  2006-8-31 10:38:24    Chengxinhui   Created
 * 
 ******************************************************************/
XMBOOL syschrSurrogate2CodeEx (WCHAR wcHiSurr, WCHAR wcLoSurr, DWORD *puCode)
{
	WORD	uHi, uHi1, uHi2;
	DWORD	uCode;

	if ((wcHiSurr >= SC_HIGH_SUR_AREA_S) && (wcHiSurr <= SC_HIGH_SUR_AREA_E))
	{
		if ((wcLoSurr >= SC_LOW_SUR_AREA_S) && (wcLoSurr <= SC_LOW_SUR_AREA_E))
		{
			uHi	= (WORD)(wcHiSurr - SC_HIGH_SUR_AREA_S);
			uHi1	= (WORD)(uHi >> 6);		// uHi / 0x40
			uHi2	= (WORD)(uHi & 0x3f);	// uHi
			uCode	= (uHi1 + 1) << 16;	// * 0x10000;
			uCode	+= (uHi2 << 10);	// uHi2 * 1024
			uCode	+= (wcLoSurr - SC_LOW_SUR_AREA_S);

			*puCode	= uCode;
			return 1;
		}
	}

	*puCode	= 0x7f;
	return 0;
}

/*******************************************************************
 * Function name: syschrCode2Surrogate
 * Description  : 把BMP以外的UNICODE编码转换为代理对。
 * Return type  : int : 如果正确转换，则返回MP_OK，否则转换失败。
						自动进行对uCode的范围进行判断。
 * Argument     : uint uCode (in) : 
 * Argument     : wchar *pwcHiSurr (out) : 
 * Argument     : wchar *pwcLoSurr (out) : 
 *  ----------------------------------------------------------------
 *          Log       *    Author    *   Casue                      
 *  ----------------------------------------------------------------
 *  2006-8-31 10:55:48    Chengxinhui   Created
 * 
 ******************************************************************/
XMBOOL syschrCode2Surrogate (DWORD uCode, WCHAR *pwcHiSurr, WCHAR *pwcLoSurr)
{
	DWORD	uUnit, uHiSurr;

	if ((uCode > 0xffff) && (uCode < 0x110000))
	{
		uHiSurr	= 0xd800;
		//
		uUnit	= uCode >> 16;
		uHiSurr	+= ((uUnit - 1) << 6);
		//
		uUnit	= uCode & 0xffff;
		uHiSurr	+= (uUnit >> 10);
		*pwcHiSurr	= (WCHAR)uHiSurr;
		//
		*pwcLoSurr	= (WCHAR)(0xdc00 + (uUnit	& 0x3ff));
		return 1;
	}
	return 0;
}


/*******************************************************************
 * Function name: syschrClearKey
 * Description  : 1、这个函数用于清理Headword中的级别标志(*go)，上标等一类通用的字符，
						使之可以成为Key。
						2、pwsDst可以等于pwsSrc。
 * Return type  : int : MP_OK
 * Argument     : wchar *pwsDst : 清理后的目标串，不会自动加入0结束标志。
 * Argument     : wchar *pwsSrc : 需要清理的源串，必须有0结束标志。
 *  ----------------------------------------------------------------
 *          Log       *    Author    *   Casue                      
 *  ----------------------------------------------------------------
 *  2007-7-22 17:07:27    Chengxinhui   Created
 *	 // <Cxh, 2008-7-9 21:36:00> 清理日语字符串
 *	 // <Cxh, 2008-12-16 15:36:41> 修改此函数，原函数如下：
 ******************************************************************/
#include "latin.inc"

void syschrClearKey (WCHAR *pwsDst, WCHAR *pwsSrc, WORD wLang)
{
   WCHAR		wc, wcDst;
	const WCHAR		*pwsLatin;

	pwsLatin = gwsLatinTab;
	
   switch(wLang)
   {
		case LANG_ARABIC:
			wc = *pwsSrc;
			while (wc) 
			{
				if (
					((wc >= 0x0621) && (wc <= 0x064a))
					|| ((wc >= 0x066e) && (wc <= 0x06d5))
					|| ((wc >= 0x06fa) && (wc <= 0x06ff))
					)
				{
					*pwsDst ++	= wc;
				}
				pwsSrc ++;
				wc = *pwsSrc;
			}
			break;

		case LANG_CHINESE:
			// pwsLatin	= gwsLatinTab;
			wc = *pwsSrc;
			while (wc)
			{
				if (wc <= 0x024f)
					wcDst = pwsLatin[wc];
				else
					wcDst	= wc;
				// <Cxh, 2009-4-3 14:22:27> 允许输入数字，允许大写字母，小写转换为大写
				if (wcDst >= 0x3400)
				{
					*pwsDst ++	= wcDst;
				}
				else if ((wcDst >= '0') && (wcDst <= '9'))
				{
					*pwsDst ++	= wcDst;
				}
				else if ((wcDst >= 'A') && (wcDst <= 'Z'))
				{
					*pwsDst ++	= wcDst;
				}
				else if ((wcDst >= 'a') && (wcDst <= 'z'))
				{
					wcDst -= ('a' - 'A');
					*pwsDst ++	= wcDst;
				}
				pwsSrc ++;
				wc = *pwsSrc;
			}
			break;

		case LANG_ENGLISH:
		case LANG_FRENCH:
		case LANG_GERMAN:
		case LANG_MALAY:
		case LANG_TURKISH:
			//pwsLatin	= gwsLatinTab;
			wc = *pwsSrc;
			while (wc)
			{
				if (wc <= 0x024f)
					wcDst = pwsLatin[wc];
				else
					wcDst	= wc;
				// <Cxh, 2009-4-3 14:22:27> 允许输入数字
				if ((wcDst >= '0') && (wcDst <= '9'))
				{
					*pwsDst ++	= wcDst;
				}
				else if ((wcDst >= 'A') && (wcDst <= 'Z'))
				{
					wcDst += 'a' - 'A' ;
					*pwsDst ++	= wcDst;
				}
				else if ((wcDst >= 'a') && (wcDst <= 'z'))
				{
					*pwsDst ++	= wcDst;
				}
				pwsSrc ++;
				wc = *pwsSrc;
			}
			break;

		case LANG_JAPANESE :
			wc = *pwsSrc;
			while (wc)
			{
				if ((wc >= 0x3041) && (wc <= 0x30ff))
				{
					*pwsDst ++	= wc;
				}
				pwsSrc ++;
				wc = *pwsSrc;
			}
			break;

		case LANG_KOREAN :
	//	#define SC_Hangul_Jamo_S	0x1100
	//	#define SC_Hangul_Jamo_E	0x11ff
	//	#define SC_Hangul_C_Jamo_S	0x3130
	//	#define SC_Hangul_C_Jamo_E	0x318f
	//	#define SC_Hangul_S			0xac00
	//	#define SC_Hangul_E			0xd7af
			wc = *pwsSrc;
			while (wc)
			{
				if (((wc >= SC_Hangul_Jamo_S) && (wc <= SC_Hangul_Jamo_E))
					|| ((wc >= SC_Hangul_C_Jamo_S) && (wc <= SC_Hangul_C_Jamo_E))
					|| ((wc >= SC_Hangul_S) && (wc <= SC_Hangul_E))
					)
				{
					*pwsDst ++	= wc;
				}
				pwsSrc ++;
				wc = *pwsSrc;
			}
			break;

		case LANG_RUSSIAN:
		case LANG_CYRILLIC:
			wc = *pwsSrc;
			while (wc) 
			{
				if ((wc >= 0x0400) && (wc <= 0x04ff))
				{
					*pwsDst ++	= wc;
				}
				pwsSrc ++;
				wc = *pwsSrc;
			}
			break;

		default:
			wc = *pwsSrc;
			while (wc)
			{
				*pwsDst ++	= wc;
				pwsSrc ++;
				wc = *pwsSrc;
			}
			break;
   }
	
	*pwsDst	= 0;
}
