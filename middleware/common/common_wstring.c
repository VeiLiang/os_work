//****************************************************************************
//
//	Copyright (C) 2010 ShenZhen ExceedSpace
//
//	Author	ZhuoYongHong
//
//	File name: common_wstring.c
//	  宽字符串操作函数
//
//	Revision history
//
//		2010.09.09	ZhuoYongHong Initial version
//
//****************************************************************************
#include <common_wstring.h>

//======================================================================
// Function Name:		WCHAR* wstrchr	(cuint *wString, WCHAR wCode)
// Purpose      :		find first code in a special WCHAR string
// Parameter    :		wString	: pointer of a WCHAR string
//							code		: code to find
// Return       :		NULL if not found, or pointer of first match code
// Remarks      :
// Change Log   :
//                Author       Date       Description
//              -----------------------------------------------
//======================================================================
WCHAR* wstrchr	(const WCHAR *wString, WCHAR wCode)
{
	if (!wString)
		return NULL;

	while(*wString)
	{
		if(*wString == wCode)
			return (WCHAR*)wString;
		wString++;
	}
	return NULL;
}

//======================================================================
// Function Name:		WCHAR* wstrrchr (const WCHAR *wString, WCHAR wCode)
// Purpose      :		find  last code in a special WCHAR string
// Parameter    :		wString	: pointer of a WCHAR string
//							code		: code to find
// Return       :		NULL if not found, or pointer of last match code
// Remarks      :
// Change Log   :
//                Author       Date       Description
//              -----------------------------------------------
//======================================================================
WCHAR* wstrrchr (const WCHAR *wString, WCHAR wCode)
{
	const WCHAR *pwStr;

	if (!wString)
		return NULL;

	pwStr = NULL;
	while(*wString)
	{
		if(*wString == wCode)
			pwStr = wString;
		wString++;
	}
	return (WCHAR*)pwStr;
}

WCHAR* wstrstr	(const WCHAR *wString, const WCHAR *wstrSet)
{
	WCHAR *cp = (WCHAR *) wString;
	WCHAR *s1, *s2;
	
	if ( !*wstrSet )
		return((WCHAR *)wString);
	
	while (*cp)
	{
		s1 = cp;
		s2 = (WCHAR *) wstrSet;
		
		while ( *s1 && *s2 && !(*s1-*s2) )
			s1++, s2++;
		
		if (!*s2)
			return(cp);
		
		cp++;
	}
	
	return(NULL);
}

/*******************************************************************
 * Function name: wstrnstr
 * Description  : 在源字符串wString中查找目标字符串wstrSet。
 * Return type  : WCHAR* : 所找到的目标字符串位置。
 * Argument     : const WCHAR *wString : 用于查找的源字符串数据。
 * Argument     : const WCHAR *wstrSet : 需要查找的目标字符串。
 * Argument     : uint uMaxSize : 允许在wString中查找的最大长度。
 *  ----------------------------------------------------------------
 *          Log       *    Author    *   Casue                      
 *  ----------------------------------------------------------------
 *  2006-12-26 15:05:25    Chengxinhui   Created
 * 
 ******************************************************************/
WCHAR* wstrnstr(const WCHAR *wString, const WCHAR *wstrSet, WORD uMaxSize)
{
	WCHAR *cp = (WCHAR *) wString;
	WCHAR *s1, *s2;
	DWORD	uSetLen;
	
	if (!*wstrSet)
		return ((WCHAR *)wString);
	
	s2 = (WCHAR *) wstrSet;
	while (*s2)
		s2 ++;
	uSetLen	= s2 - wstrSet;
	while (*cp && (uMaxSize >= uSetLen))
	{
		s1 = cp;
		s2 = (WCHAR *)wstrSet;
		
		while (*s1 && *s2 && !(*s1 - *s2))
			s1 ++, s2 ++;
		
		if (!*s2)
			return(cp);
		
		cp ++;
		uMaxSize --;
	}
	
	return(NULL);
}

//======================================================================
// Function Name:		int  wstrcmp	(const WCHAR *wString1, const WCHAR *wString2)
// Purpose      :		indicates the lexicographic relation of string1 to string2
// Parameter    :		wString1	: string1
//							wString2	: string2
// Return       :		-1 : string1 less than string2
//							0	: string1 identical to string2
//							1  : string1 greater than string2
// Remarks      :
// Change Log   :
//                Author       Date       Description
//              -----------------------------------------------
// <Cxh, 2005-1-8 20:41:26> Modified the type of the return value.
//======================================================================
SHORT  wstrcmp	(const WCHAR *wString1, const WCHAR *wString2)
{
	if (!wString1 || !wString2)
		return 0;

	while(*wString1 && *wString2)
	{
		if(*wString1 > *wString2)
			return 1;
		else if(*wString1 < *wString2)
			return -1;
		wString1++;
		wString2++;
	}
	if(*wString1 != 0)
		return 1;
	else if(*wString2 != 0)
		return -1;
	return 0;
}

//======================================================================
// Function Name:		int  wstrncmp	(const WCHAR *wString1, const WCHAR *wString2, uint nCount);
// Purpose      :		indicates the lexicographic relation of string1 to string2 in special length
// Parameter    :		wString1	: string1
//							wString2	: string2
//							nCount	: given length
// Return       :		-1 : string1 less than string2
//							0	: string1 identical to string2
//							1  : string1 greater than string2
// Remarks      :
// Change Log   :
//                Author       Date       Description
//              -----------------------------------------------
// <Cxh, 2005-1-8 20:42:38> Modified the type of the return value.
//======================================================================
SHORT  wstrncmp	(const WCHAR *wString1, const WCHAR *wString2, WORD nCount)
{
	if (!wString1 || !wString2)
		return 0;

//; ; //	for(; nCount>0; nCount--)
	while (nCount > 0)
	{
		if(*wString1 > *wString2)
			return 1;
		else if(*wString1 < *wString2)
			return -1;
      else if(!(*wString1) && !(*wString2))
         break;

		wString1++;
		wString2++;

		nCount --;
	}
	return 0;
}

//======================================================================
// Function Name:		WCHAR* wstrcpy(WCHAR *wstrDest, const WCHAR *wstrSource)
// Purpose      :		copy source string to destine string
// Parameter    :		wstrDest		: destine string pointer
//							wstrSource	: source string pointer
// Return       :		destine string pointer
// Remarks      :
// Change Log   :
//                Author       Date       Description
//              -----------------------------------------------
//======================================================================
WCHAR* wstrcpy(WCHAR *wstrDest, const WCHAR *wstrSource)
{
	WCHAR *pwStr;

	if (!wstrDest || !wstrSource)
		return NULL;

	pwStr = wstrDest;
	while(*wstrSource)
   {
		*pwStr = *wstrSource;
      pwStr++;
      wstrSource++;
   }
	*pwStr = 0;
	return wstrDest;
}

//======================================================================
// Function Name:		WCHAR* wstrncpy	(WCHAR *wstrDest, const WCHAR *wstrSource, uint nCount);
// Purpose      :		copy source string to destine string in special length
// Parameter    :		wstrDest		: destine string pointer
//							wstrSource	: source string pointer
//							nCount		: length to copy
// Return       :		destine string pointer
// Remarks      :
// Change Log   :
//                Author       Date       Description
//              -----------------------------------------------
//======================================================================
WCHAR* wstrncpy	(WCHAR *wstrDest, const WCHAR *wstrSource, WORD nCount)
{
	WCHAR *pwStr;

	if (!wstrDest || !wstrSource)
		return NULL;

	pwStr = wstrDest;
	for(; nCount>0 && *wstrSource; nCount --)
   {
		*pwStr = *wstrSource;
      pwStr++;
      wstrSource++;
   }
	*pwStr = 0;

	return wstrDest;
}

//======================================================================
// Function Name:		WCHAR* wstrcat	(WCHAR *wstrDest, const WCHAR *wstrSource)
// Purpose      :		link source string to the end of destine string
// Parameter    :		wstrDest		: destine string pointer
//							wstrSource	: source string pointer
// Return       :		destine string pointer
// Remarks      :
// Change Log   :
//                Author       Date       Description
//              -----------------------------------------------
//======================================================================
WCHAR* wstrcat	(WCHAR *wstrDest, const WCHAR *wstrSource)
{
	WCHAR *pwStr;

	if (!wstrDest || !wstrSource)
		return NULL;

	pwStr = wstrDest;
	while(*pwStr)pwStr++;
	while(*wstrSource)
   {
		*pwStr = *wstrSource;
      pwStr++;
      wstrSource++;
   }
	*pwStr = 0;
	return wstrDest;
}

//======================================================================
// Function Name:		WCHAR* wstrncat	(WCHAR *wstrDest, const WCHAR *wstrSource, uint nCount)
// Purpose      :		link source string to the end of destine string in special length
// Parameter    :		wstrDest		: destine string pointer
//							wstrSource	: source string pointer
//							nCount		: length to link
// Return       :		destine string pointer
// Remarks      :
// Change Log   :
//                Author       Date       Description
//              -----------------------------------------------
//======================================================================
WCHAR* wstrncat	(WCHAR *wstrDest, const WCHAR *wstrSource, WORD nCount)
{
	WCHAR *pwStr;

	if (!wstrDest || !wstrSource)
		return NULL;

	pwStr = wstrDest;
	while(*pwStr)pwStr++;
	for(; nCount>0 && *wstrSource ; nCount --)
   {
		*pwStr = *wstrSource;
      pwStr++;
      wstrSource++;
   }
	*pwStr = 0;
	return wstrDest;
}
/*******************************************************************
 * Function name: wstrappend
 * Description  : 
 * Return type  : WCHAR* 
 * Argument     : WCHAR *pDst
 * Argument     : WCHAR wc
 *  ----------------------------------------------------------------
 *          Log       *    Author    *   Casue                      
 *  ----------------------------------------------------------------
 *  2004-12-12 18:34:03    Chengxinhui   Created
 * 
 ******************************************************************/
WCHAR* wstrappend(WCHAR *pDst, WCHAR wc)
{
	WCHAR	*pTmp;
	if (!pDst)
		return NULL;
	pTmp	= pDst;
	while (*pTmp)
		pTmp ++;
	*pTmp ++	= wc;
	*pTmp		= 0;
	return pDst;
}
//======================================================================
// Function Name:		uint  wstrlen	(const WCHAR *wString);
// Purpose      :		Get the length of given string
// Parameter    :		wString	: string given
// Return       :		length of string
// Remarks      :
// Change Log   :
//                Author       Date       Description
//              -----------------------------------------------
//======================================================================
WORD  wstrlen	(const WCHAR *wString)
{
	WORD nLen	= 0;
	const WCHAR	*pws;

	if (wString)
	{
		pws	= wString;
		while (*wString)
		{
			wString ++;
		}
		nLen	= (WORD)(wString - pws);
	}
	return nLen;
}

//======================================================================
// Function Name:		WCHAR* wstrrev	(WCHAR *wString)
// Purpose      :		Reverse a string
// Parameter    :		wString	: string to reverse
// Return       :		pointer of the new string
// Remarks      :
// Change Log   :
//                Author       Date       Description
//              -----------------------------------------------
//======================================================================
WCHAR* wstrrev	(WCHAR *wString)
{
	WCHAR *pwHead, *pwTail;
	WCHAR wTmp;

	if (!wString)
		return NULL;

	pwHead = wString;
	pwTail = wString;
	while(*pwTail)pwTail++;
	pwTail --;

	while(pwHead < pwTail)
	{
		wTmp = *pwHead;
		*pwHead++ = *pwTail;
		*pwTail-- = wTmp;
	}
	return wString;
}

/*******************************************************************
 * Function name: Dword2Ws
 * Description  : Convert a Uint to WCHAR string.
 *						Each a nibble convert a WCHAR character.
 * Return type  : void 
 * Argument     : unsigned short *pwcsDst
 * Argument     : dword dwSrc
 * Argument     : uint uWidth : the number of the nibble.
 *  ----------------------------------------------------------------
 *          Log       *    Author    *   Casue                      
 *  ----------------------------------------------------------------
 *  2004-12-21 14:42:24    Chengxinhui   Created
 * 
 ******************************************************************/
void dword2wstr (WCHAR *pwsDst, DWORD dwSrc, WORD uWidth)
{
	DWORD dwTmp	= dwSrc;
	WORD uActualWidth	= 0;

	if (!pwsDst)
		return;

	while (dwTmp)
	{
		uActualWidth ++;
		dwTmp	>>= 4;
	}
	if (uActualWidth > uWidth)
		uWidth = uActualWidth;
	while (uWidth > 0)
	{
		uWidth --;
		dwTmp	= (dwSrc >> (uWidth << 2));
		dwTmp	&= 0x0f;
		if (dwTmp <= 9)
		{
			*pwsDst	= (unsigned short)('0' + dwTmp);
		}
		else	// a - f
		{
			*pwsDst	= (unsigned short)('a' + (dwTmp - 10));
		}
		pwsDst ++;
	}
	*pwsDst	= 0;
}

/*******************************************************************
 * Function name: Ws2Dword
 * Description  : Conver a WCHAR string to a uint
 * Return type  : uint 
 * Argument     : const unsigned short *pwsSrc
 *  ----------------------------------------------------------------
 *          Log       *    Author    *   Casue                      
 *  ----------------------------------------------------------------
 *  2004-12-22 10:16:57    Chengxinhui   Created
 * 
 ******************************************************************/
DWORD wstr2dword (const WCHAR *pwsSrc)
{
	DWORD	dwTmp, dwDst	= 0;
	WORD	uCounter = 0;
	WCHAR	wc;
	
	if (!pwsSrc)
	{
		return (DWORD)(-1);
	}

	while (*pwsSrc)
	{
		wc	= *pwsSrc;

		if ((wc >= '0') && (wc <= '9'))
			dwTmp	= wc - '0';
		else if ((wc >= 'a') && (wc <= 'f'))
			dwTmp	= 10 + (wc - 'a');
		else if ((wc >= 'A') && (wc <= 'F'))
			dwTmp	= 10 + (wc - 'A');
		else
			dwTmp	= 0;
		
		dwDst	<<= 4;
		dwDst	|= (dwTmp & 0x0f);
			
		pwsSrc ++;
		uCounter ++;
		if (uCounter > 8)
			break;	// Don't report error.
	}
	return dwDst;
}

/*******************************************************************
 * Function name: asc_str2wstr
 * Description  : 转换一个仅包含ASCII字符的单字节字符串到双字节串。
 * Return type  : wchar* 
 * Argument     : wchar *wstrDst
 * Argument     : char *strSrc
 *  ----------------------------------------------------------------
 *          Log       *    Author    *   Casue                      
 *  ----------------------------------------------------------------
 *  2004-11-27 11:09:12    Chengxinhui   Created
 * 
 ******************************************************************/
WCHAR* astr2wstr(WCHAR *wstrDst, const CHAR *strSrc)
{
	WCHAR	*pret	= wstrDst;

	if (!wstrDst || !strSrc)
		return NULL;

	while (*strSrc)
	{
		*wstrDst ++	= *strSrc++;
	}
	*wstrDst = 0;
	return pret;
}
/*******************************************************************
 * Function name: asc_wstr2str
 * Description  : 转换一个仅包含ASCII字符的双字节字符串到单字节字符串。
 * Return type  : char* 
 * Argument     : char *strDst
 * Argument     : const wchar *wstrSrc
 *  ----------------------------------------------------------------
 *          Log       *    Author    *   Casue                      
 *  ----------------------------------------------------------------
 *  2005-8-24 16:27:58    Chengxinhui   Created
 * 
 ******************************************************************/
CHAR* wstr2astr(CHAR *strDst, const WCHAR *wstrSrc)
{
	CHAR	*pRet	= strDst;
	if (!strDst || !wstrSrc)
		return NULL;
	while (*wstrSrc)
	{
		*strDst	= (CHAR)*wstrSrc;
		strDst ++;
		wstrSrc ++;
	}
	*strDst	= 0;
	return pRet;
}

/////////////////////////////////////////////UTF-8格式转成UNICODE//////////////////
int  UTF8toUTF16 (void *lpUtf8Buf, void *lpUnicodeBuf, int *lpcbUtf8Buf, int cbUnicodeBuf)
{
	int  i;
	unsigned char high,low,temp,temp2,temp3;
	int wlen , len;
	
	len = *lpcbUtf8Buf; 
	wlen = 0;
	for(i=0;i<len && wlen< cbUnicodeBuf ;i++)
	{   
		if((*(unsigned char*)lpUtf8Buf) == 0)
		{
			*(unsigned short*)lpUnicodeBuf = 0x0;  //结束用0x0表示
			*lpcbUtf8Buf = i;
			return wlen;
		}
		if(((*(unsigned char*)lpUtf8Buf)&0x80)==0)
		{
			(*(unsigned short*)lpUnicodeBuf) = (*(unsigned char*)lpUtf8Buf);
		}
		
		else if(((*(unsigned char*)lpUtf8Buf)&0xe0)==0xc0)
		{
			temp=*(unsigned char*)lpUtf8Buf;
			lpUtf8Buf = (unsigned char*)lpUtf8Buf+1;
			high = (unsigned char)((temp&0x1f)>>2);
			low = (unsigned char)(((*(unsigned char*)lpUtf8Buf)&0x3f) | (temp<<6));
			*(unsigned short*)lpUnicodeBuf = (unsigned short)(((unsigned short)low)|(((unsigned short)high)<<8));
			i++;
			//--------------------//如果单字节缓冲区最后一个字节符合0xe0或0xc0(表示此UTF-8码为双字节或三字节)
			if (i >= len) {//代表转换的字节超过了  单字节缓冲区
				i--;
				*(unsigned short*)lpUnicodeBuf = 0;
				*lpcbUtf8Buf = i;
				return wlen;
			}
			//--------------------
		}
		else if(((*(unsigned char*)lpUtf8Buf)&0xe0)==0xe0)
			
		{
			temp=*(unsigned char*)lpUtf8Buf;
			lpUtf8Buf=(unsigned char*)lpUtf8Buf+1;
			temp2=*(unsigned char*)lpUtf8Buf;
			lpUtf8Buf=(unsigned char*)lpUtf8Buf+1;
			temp3=*(unsigned char*)lpUtf8Buf;
			low = (unsigned char)((temp3&0x3f) | ((temp2&0x03)<<6));
			high = (unsigned char)(((temp2&0x3f)>>2) | ((temp&0x0f)<<4));
			*(unsigned short*)lpUnicodeBuf = (unsigned short)(((unsigned short)low) | (((unsigned short)high)<<8));
			i++;	
			i++;
			//--------------------//如果单字节缓冲区最后一个字节符合0xe0或0xc0(表示此UTF-8码为双字节或三字节)
			if (i >= len) 
			{
				//代表转换的字节超过了  单字节缓冲区
				i -= 2;
				*(unsigned short*)lpUnicodeBuf = 0;
				*lpcbUtf8Buf = i;
				return wlen;
			}
			//--------------------
		}
		else//失败，此编码不是UTF8编码，这时将单字节扩展为双字节，写入双字节缓冲，保证双字节缓冲中有东西
		{
			(*(unsigned short*)lpUnicodeBuf) = (*(unsigned char*)lpUtf8Buf);
		}
		
		lpUnicodeBuf=(unsigned short*)lpUnicodeBuf+1;      
		lpUtf8Buf=(unsigned char*)lpUtf8Buf+1;
		++wlen;  		
	}
	*(unsigned short*)lpUnicodeBuf=0;
	*lpcbUtf8Buf = i;	   
	return wlen;
}