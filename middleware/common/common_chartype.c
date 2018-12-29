 /***************************************************************************
  *
  *   Created:   2005-3-25 15:03:25
  *   Author:    Chengxinhui
  *
  ****************************************************************************
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
#include <xm_type.h>
#include <common_chartype.h>
#include <common_languageid.h>

static XMBOOL sctCharInTable(WCHAR wcFinding, BYTE bank, const TSysCharRecord *pRcdTbl, SHORT nRcdTotal)
{
	SHORT	nStart, nEnd, nAver;
	WCHAR	wcTmp;
	XMBOOL	bFound		= 0;
	
	nStart	= 0;
	nEnd		= (SHORT)(nRcdTotal - 1);
	while (nStart <= nEnd)
	{
		nAver	= (SHORT)((nStart + nEnd) >> 1);
		wcTmp	= pRcdTbl[nAver].mStartCode;
		if (wcTmp > wcFinding)
		{
			nEnd	= (SHORT)(nAver - 1);
		}
		else if (wcTmp < wcFinding)
		{
			if (pRcdTbl[nAver].mEndCode >= wcFinding)
			{
				bFound	= 1;
				break;
			}
			else
			{
				nStart	= (SHORT)(nAver + 1);
			}
		}
		else	// (wcTmp == wcFinding)
		{
			bFound	= 1;
			break;
		}
	}
	return bFound;
}

/*
static SHORT findCharInTable(WCHAR wcFinding, const TCharTypeTbl *pRcdTbl, SHORT nRcdTotal)
{
	SHORT	nStart, nEnd, nAver, nTgtItem;
	WCHAR	wcTmp;
	
	nTgtItem	= -1;
	nStart	= 0;
	nEnd		= (SHORT)(nRcdTotal - 1);
	while (nStart <= nEnd)
	{
		nAver	= (SHORT)((nStart + nEnd) >> 1);
		wcTmp	= pRcdTbl[nAver].mStartCode;
		if (wcTmp > wcFinding)
		{
			nEnd	= (SHORT)(nAver - 1);
		}
		else if (wcTmp < wcFinding)
		{
			if (pRcdTbl[nAver].mEndCode >= wcFinding)
			{
				nTgtItem	= nAver;
				break;
			}
			else
			{
				nStart	= (SHORT)(nAver + 1);
			}
		}
		else	// (wcTmp == wcFinding)
		{
			nTgtItem	= nAver;
			break;
		}
	}

	return nTgtItem;
}*/
/*
	非汉字区域：
	0xa500 - 0xabff	unassign
	0xac00 - 0xd7ff	Hangul
	0xd800 - 0xdfff	Surrogates
	0xe000 - 0xf8ff	Private area
	
	0xf900 - 0xfaff	CJK Compatibility Ideographs.
	0xfb00 - 0xffff
	特殊的汉字：
	0x3007	数字0
	0x25cb	这是一个符号，暂时加入，因为来不及修改数据。

*/
static const TSysCharRecord	gwcNonChineseKeyChar[]	=
{
	{0x0000, 0x25ca},
	{0x25cc,	0x3006},
	{0x3008, 0x33ff},
	{0x9fa6, 0xf8ff},
	{0xfb00, 0xffff},
};

XMBOOL IsChineseChar(WCHAR wc)
{
//	return !sctCharInTable(wc, gwcNonChineseKeyChar, sizeof(gwcNonChineseKeyChar) / sizeof(gwcNonChineseKeyChar[0]));
	if (((wc >= 0x3400) && (wc <= 0x9fff))
		|| ((wc >= 0xe000) && (wc <= 0xfaff)))
	{
		return 1;
	}
	else if ((wc >= 0xd800) && (wc <= 0xdfff))
		return 1;
	else
		return 0;
}

/*******************************************************************
 * Function name: IsLatinChar
 * Description  : 
 * 0x0000 -- 0x001f: Control
 *	0x0020 -- 0x007f: Basic Latin
 * 0x0080 -- 0x009f: Control
 * 0x00a0 -- 0x00ff: Latin-1 Supplement
 * 0x0100 -- 0x017f: Latin Extented-A
 * 0x0180 -- 0x024f: Latin Extented-B
 * Return type  : XMBOOL 
 * Argument     : WCHAR wc
 *  ----------------------------------------------------------------
 *          Log       *    Author    *   Casue                      
 *  ----------------------------------------------------------------
 *  2008-9-6 16:25:45    Chengxinhui   Created
 *  
 ******************************************************************/
XMBOOL IsLatinChar(WCHAR wc)
{
	// if (wc >= 0x00c0 && wc <= 0x00f6)
	if (wc >= 0x00c0 && wc <= 0x024f)
		return 1;
	else if (wc >= 0x00f7 && wc <= 0x00ff)
		return 1;
	else if (wc >= 0x0100 && wc <= 0x01ff)
		return 1;
	else if (wc >= 0x0200 && wc <= 0x024f)
		return 1;
	else
		return 0;
}

XMBOOL IsEnglishChar(WCHAR wc)
{
	if (((wc >= 'a') && (wc <= 'z'))
		|| ((wc >= 'A') && (wc <= 'Z'))
		|| ((wc >= 0xff21) && (wc <= 0xff3a))
		|| ((wc >= 0xff41) && (wc <= 0xff5a)))
	{
		return 1;
	}
	else
		return 0;
}

XMBOOL IsJapaneseChar(WCHAR wc)
{
   if (((wc >= 0x3041) && (wc <= 0x309e))
      || ((wc >= 0x30a1) && (wc <= 0x30fe))
      || ((wc >= 0xff66) && (wc <= 0xff9f))
      )
   {
      return 1;
   }
   else
      return 0;
}

XMBOOL IsTHAIChar(WCHAR wc)
{
   if ((wc >= 0x0e00) && (wc <= 0x0e7f))
      return 1;
   else
      return 0;
}

#define ARABIC_PRE_FORM_A_S 0xfb50
#define ARABIC_PRE_FORM_A_E 0xfDFF
#define ARABIC_PRE_FORM_B_S 0xfE70
#define ARABIC_PRE_FORM_B_E 0xfEFF

XMBOOL IsArabicChar(WCHAR wc)
{
   if(((wc >= 0x0600) && (wc <= 0x06ff))
    ||((wc >= 0xfb50) && (wc <= 0xfDFF))
    ||((wc >= 0xfE70) && (wc <= 0xfEFF)))
      return 1;
   else
      return 0;

}

static const TSysCharRecord	gwcSpace[]	=
{
	{0x0020,	0x0020},
	{0x00a0,	0x00a0},
	{0x200a,	0x200b},
	{0x2060,	0x2060},
	{0x3000,	0x3000},
	{0xfeff,	0xfeff}
};

XMBOOL IsSpaceChar(WCHAR wc)
{ 
	return sctCharInTable (wc, 
		0, gwcSpace, 
		sizeof(gwcSpace) / sizeof(gwcSpace[0]));
}

static const TSysCharRecord	gwcPunctuation[]	=
{
	{0x0021, 0x002f},
	{0x003a,	0x0040},
	{0x005b,	0x0060},
	{0x007b, 0x007e},
   {0x00ab, 0x00ab},
   {0x00bb, 0x00bb},
   {0x060c, 0x060c},
   {0x061b, 0x061b},
   {0x061F, 0x061F},
   {0x066a, 0x066d},
   {0x2000,	0x206f},
	{0x3001,	0x3003},
	{0x3008,	0x3011},
	{0x3014,	0x301f},
	{0xff00, 0xff0f},
	{0xff1a, 0xff20},
	{0xff3b, 0xff40},
	{0xff5b, 0xff65}
};

XMBOOL IsPunctuationChar(WCHAR wc)
{
	return sctCharInTable (wc, 
		0, gwcPunctuation,
		sizeof(gwcPunctuation) / sizeof(gwcPunctuation[0]));
}

