//****************************************************************************
//
//	Copyright (C) 2008 ShenZhen ExceedSpace
//
//	Author	ZhuoYongHong
//
//	File name: xm_tester_script.c
//	  测试脚本
//
//	Revision history
//
//		2008.03.02	ZhuoYongHong Initial version
//
//****************************************************************************
#include "xm_type.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <ctype.h>
#include "xm_dev.h"
#include "xm_base.h"
#include "xm_file.h"
#include "rtos.h"

#include "xm_tester.h"

#include "wintest.h"
#include "xm_key.h"

#define	MAPFILE

#ifdef _WINDOWS
#pragma warning(disable : 4996)
#endif

#define	MAX_INLINE		0x200

static int _Loop		(void);
static int _EndLoop	(void);

#define	MAX_PATH	127

static const TESTER_COMMAND Commands[] = {
	{	"[LOOP]",			_Loop			},		/*	Define beginning of block for loop*/
	{	"[ENDLOOP]",		_EndLoop		},		/*	Define end of block for loop*/
	{	NULL,					NULL			}
};


static LOOPINFO	TestLoopStack[MAX_STACKSTAGE];	/*	Loop position recoder*/
static int			TestStackTop = MAX_STACKSTAGE;

__no_init static char			InLine[MAX_INLINE];
__no_init static char			KeyName[MAX_INLINE];
__no_init static char			LogName[MAX_INLINE];
static char	*		pInLine;

static WCHAR		WcLine[MAX_CHAR+1] = {0};
static WCHAR *		pWcLine        = NULL;

static void *		fpScript			= NULL;
#ifdef _WINDOWS
static void *		fpLog				= NULL;
#endif

static int			ScriptLineNo	= 0;
//static int			ScriptLines		= 0;
static int			ScriptEOF		= 1;
static char			scriptFile[MAX_PATH];

static int			ScriptState		= SCRIPT_TEST;

static KEYS			Keys[MAX_KEYS];
static int			KeyCount;

static int			CodePage;

static DWORD dwKeyDown = 0;
static DWORD dwModifier= 0;

XMMAPFILE			MapFile;
__no_init static char szMapData[0x8000];		// 最大脚本32K
static int cbMapData = 0;


static void * open_map_file (char *filename)
{
	void *fp;
	fp = XM_fopen (filename, "rb");
	if(fp == NULL)
		return fp;

	cbMapData = XM_fread (szMapData, 1, sizeof(szMapData), fp);
	XM_fclose (fp);
	fp = NULL;
	if(cbMapData == 0)
		return fp;

	if(XM_MapFileOpen (szMapData, cbMapData, &MapFile) == FALSE)
	{
		cbMapData = 0;
		return fp;
	}
	fp = &MapFile;
	return fp;
}

static void close_map_file (void *mapfile)
{
	if(mapfile)
	{
		XM_MapFileClose (mapfile);
	}
	cbMapData = 0;
}

char *  fgets_map_file (char *string, int n, void *stream )
{
	int ch;
	char *s = string;
	int size = 0;
	while(n > 0)
	{
		unsigned char buf[1];
		LONG to_read = XM_MapFileRead (stream, buf, 1);
		if(to_read <= 0)
		{
			*string = '\0';
			break;
		}
		ch = buf[0];

	//	ch = fgetc ((FILE *)stream);
		if(ch < 0)
		{
			*string = '\0';
			break;
		}
		else if(ch == 0x0A)
		{
			*string = 0x0A;
			string ++;
			n --;
			size ++;
			break;
		}
		*string = (char)ch;
		string ++;
		size ++;
		n --;
	}

	if(n > 0)
		*string = '\0';

	if(size)
		return s;
	else
		return NULL;
}


static void	_Init (void)
{
	TestStackTop = MAX_STACKSTAGE;
	memset (InLine, 0, sizeof(InLine));
	fpScript			= NULL;
	//ScriptLines		= 0;
	ScriptLineNo	= 0;
	ScriptEOF		= FALSE;
	pInLine			= InLine;
	memset (scriptFile, 0, sizeof(scriptFile));

	dwKeyDown = 0;
	dwModifier= 0;

}

/*-----------------------------------------------------------------------*/
/*|  procedure for reserve word [LOOP]                                  |*/
/*-----------------------------------------------------------------------*/
int _Loop (void)
{
	// 规避sscanf函数固有的%s格式的缓冲区溢出bug
	char		Tmp[MAX_INLINE];
	ULONG		LoopCnt;

	i64_t	LoopPos[2];

	/*	Get loop counter.*/
	sscanf  (InLine, "%s %ld", Tmp, &LoopCnt);
	
	if(ScriptState == SCRIPT_TEST)	// 快速测试使用
		LoopCnt = 1;

	/*	Get current file position.*/
#ifdef MAPFILE
	{
		LONG pos = XM_MapFileTell (fpScript);
		if(pos == (-1))
			return TESTERSYSERR_INVALIDSCRIPTLINE;
		LoopPos[0] = pos;
	}
#else
	XM_fgetpos (fpScript, &LoopPos[0]);
#endif

	--TestStackTop;
	if (TestStackTop < 0)
	{
		if(ScriptState == SCRIPT_TEST)
		{
#ifdef _WINDOWS
			char msg[0x1000];
			sprintf ( msg, "\t Nesting Level is greater than %d.\n", MAX_STACKSTAGE );
			if(fpLog)
			{
				//fprintf (fpLog, "%s", msg);
				XM_fwrite (msg, 1, strlen(msg), fpLog);
			}
#endif
		}
		else
		{
			//assert (0);
		}
		return TESTERSYSERR_INVALIDSCRIPTLINE;
	}

	/*	Save loop position in stack.*/
	TestLoopStack[TestStackTop].LoopCnt = LoopCnt;
	TestLoopStack[TestStackTop].LoopPos = LoopPos[0];
	return 0;
}

/*-----------------------------------------------------------------------*/
/*|  Procedure for reserve word [ENDLOOP]                               |*/
/*-----------------------------------------------------------------------*/
static int _EndLoop (void)
{
	i64_t	LoopPos[2];

	if (TestStackTop == MAX_STACKSTAGE)
	{
		return 0;
	}
	
	LoopPos[0] = TestLoopStack[TestStackTop].LoopPos;
	LoopPos[1] = 0;

	/*	goto back to the position specified by [LOOP] and Dec the counter*/
	if (--TestLoopStack[TestStackTop].LoopCnt)
	{
#ifdef MAPFILE
		XM_MapFileSeek (fpScript, (LONG)LoopPos[0], XM_SEEK_SET);
#else
		XM_fsetpos (fpScript, &LoopPos[0]);
#endif
	}
	else
		++TestStackTop;

	return 0;
}

// 语法定义
// [char] 字符编码 字符串: (GB2312, BIG5, SJIS, ASCII)
// [ENDCHAR]
// 
int IsHexCode (unsigned char ch)
{
	if(	(ch >= '0' && ch <= '9')
		||	(ch >= 'a' && ch <= 'z')
		||	(ch >= 'A' && ch <= 'Z')
		)
		return 1;
	else
		return FALSE;
}


/*--------------------------------------------------------------------------*/
/*|  Press a Key where Key is a pointer to the name for the key position.	 */
/*|  A greedy match is prefomed to match with the longest name.				 */
/*--------------------------------------------------------------------------*/
static int GetOneKey (char** pKey, unsigned char *modifier)
{
	char				*KeyBuf;
	UINT				i, j, k;
	
re_scan:
	*modifier = 0;

	// skip SPACE and TAB
	while( **pKey && (**pKey == ' ' || **pKey == '\t') )
	{
		(*pKey) ++;
	}

	KeyBuf = *pKey;


	/*	Find the Key sepcified in test line.*/
	for (i = j = 0, k = MAX_KEYS; (int)i < KeyCount && *Keys[i].KeyName; ++i)
	{
		if (Keys[i].KeyLength > j)
		{
			//if (! strncmp (KeyBuf, (char*)Keys[i].KeyName, Keys[i].KeyLength))
			if( ! strncmp (KeyBuf, (char*)Keys[i].KeyName, Keys[i].KeyLength))
			{
				j = Keys[i].KeyLength;
				k = i;
			}
		}

		if (Keys[i].LogLength > j)
		{
			//if (! strncmp (KeyBuf, (char*)Keys[i].KeyName, Keys[i].KeyLength))
			if( ! strncmp (KeyBuf, (char*)Keys[i].LogName, Keys[i].LogLength))
			{
				j = Keys[i].LogLength;
				k = i;
			}
		}

	}

	if (k == MAX_KEYS)
	{
		if( ! strncmp (KeyBuf, "RANDKEY", 7) )
		{
			// 随机按键
			i = (unsigned int)rand ();
			i %=  KeyCount;
			k = i;
			*pKey += 7;
			return  (UINT) Keys[k].KeyValue | KEYDOWN_MASK;
		}
		else if( ! strncmp (KeyBuf, "DELAY", 5) )
		{
			// 延时, 毫秒单位
			unsigned int ms = 0;
			*pKey += 5;
			KeyBuf = *pKey;
			while (*KeyBuf == ' ' || *KeyBuf == '\t')
				KeyBuf ++;
			while ( *KeyBuf >= '0' && *KeyBuf <= '9' )
			{
				ms = ms * 10;
				ms += *KeyBuf - '0';
				KeyBuf ++;
			}

			*pKey = KeyBuf;
			if(ScriptState == SCRIPT_TEST)
			{
				return 0;
			}
			if(ms)
			{
				XM_Delay (ms);
			}

			return 0;
		}
		else if( ! strncmp (KeyBuf, "ALARM", 5) )
		{
			// 闹钟, 秒单位
			void SetAutoPowerUpTime(unsigned int power_off_delay_seconds);


			unsigned int sec = 0;
			*pKey += 5;
			KeyBuf = *pKey;
			while (*KeyBuf == ' ' || *KeyBuf == '\t')
				KeyBuf ++;
			while ( *KeyBuf >= '0' && *KeyBuf <= '9' )
			{
				sec = sec * 10;
				sec += *KeyBuf - '0';
				KeyBuf ++;
			}

			*pKey = KeyBuf;
			if(ScriptState == SCRIPT_TEST)
			{
				return 0;
			}
			if(sec)
			{
				printf ("Alarm %d\n", sec);
				SetAutoPowerUpTime (sec);
			}

			return 0;			
		}
		else if( ! strncmp (KeyBuf, "POWER", 5) )
		{
			// 系统复位指令
			if(ScriptState == SCRIPT_TEST)
			{
				*pKey += 5;
				return 0;
			}
			XM_KeyEventProc (VK_POWER, XMKEY_PRESSED);
			OS_Delay (50);
			return 0;	
		}
		else if( ! strncmp (KeyBuf, "RESET", 5) )
		{
			// 系统复位指令
			if(ScriptState == SCRIPT_TEST)
			{
				*pKey += 5;
				return TESTERSYSERR_ENDOFSCRIPT;
			}
			// XM_ShutDownSystem (SDS_REBOOT);
			// XM_KeyEventPost (VK_POWER, XMKEY_PRESSED);
			// 20180129 zhuoyonghong
			// 增加软件重启功能
			XM_KeyEventPost (VK_REBOOT, XMKEY_PRESSED);	
		}

		if(ScriptState == SCRIPT_TEST)
		{
#ifdef _WINDOWS
			if(fpLog)
			{
				char msg[0x1000];
				sprintf (msg, "\t 非法的脚本语法 --- \"%s\" at \"%s\".\n", InLine, *pKey);
				//fprintf (fpLog, "%s", msg);
				XM_fwrite (msg, 1, strlen(msg), fpLog);
			}
#endif
		}
		else
		{
			// can't reach here
			//assert (0);
		}

		// 中止当前行语法检查，继续下一行脚本检查
		*pInLine = '\0';

		return TESTERSYSERR_INVALIDSCRIPTLINE;
	}
	else
	{
		//*pKey += Keys[k].KeyLength;
		*pKey += j;



		return  (UINT) Keys[k].KeyValue | KEYDOWN_MASK;
	}
}

char* script_gets_key (char* buf, size_t n, void* fp)
{
	char *ret;
	unsigned char *pbuf;

//	memset (buf, 0, n);

	pbuf = (unsigned char *)buf;

	ret = XM_fgets (buf, n, fp);
	if (ret != buf)	
		return NULL;

	while (*pbuf && *pbuf != '\n')
		++pbuf;

	// remove the spaces of tail
	*pbuf = '\0';	// remove 0x0a 0x09
	pbuf --;   
	while (isspace(*pbuf) && (pbuf >= (unsigned char *)buf))
	{
		*pbuf-- = 0;
	}

	return ret;
}


/*-----------------------------------------------------------------------*/
/*|  script_gets                                                        |*/
/*|                                                                     |*/
/*|  get strings from sequence file.                                    |*/
/*-----------------------------------------------------------------------*/
char* script_gets (char* buf, size_t n, void* fp)
{
	char *ret;
	unsigned char *pbuf;

//	memset (buf, 0, n);

	pbuf = (unsigned char *)buf;

#ifdef MAPFILE
	ret =	fgets_map_file (buf, n, fp);
#else
	ret = XM_fgets (buf, n, fp);
#endif
	if (ret != buf)	
		return NULL;

	while (*pbuf && *pbuf != '\n')
		++pbuf;

	// remove the spaces of tail
	*pbuf = '\0';	// remove 0x0a 0x09
	pbuf --;   
	while (isspace(*pbuf) && (pbuf >= (unsigned char *)buf))
	{
		*pbuf-- = 0;
	}

	return ret;
}


/*-----------------------------------------------------------------------*/
/*|  AutoMessage                                                        |*/
/*|                                                                     |*/
/*|  Main function of auto test module. it will process the sequence    |*/
/*|  file and return the message.                                       |*/
/*-----------------------------------------------------------------------*/
int AutoMessage (unsigned char *Modifier)
{
	UINT		i;
	int		key;
	int		mask = 0;

	*Modifier = 0;

	// 检测是否存在字符消息
	if(pWcLine && *pWcLine)
	{	
		
		mask = CHAR_MASK;
		if(pWcLine == WcLine)	// 处理新序列行
		{
			mask |= SEQBEGIN_MASK;
			*pInLine = '\0';	// 强制取新序列行数据
		}

		// skip SPACE & TAB
		while(*pWcLine && (*pWcLine == ' ' || *pWcLine == '\t'))
		{
			*pWcLine ++;
		}

		if(*pWcLine)
		{
			USHORT CharCode = *pWcLine;
			pWcLine ++;
			return CharCode | mask;	
		}
		else
		{
			*pInLine = '\0';	// 强制取新序列行数据
			mask = 0;
		}
	}

	/*	If there are string wait for process in buffer. call PressKey*/
	/*	to analysis it and get the key code.*/
	/*	else, get the next line from sequence file. if sequence file*/
	/*	reach end of file. open the next file.*/

	if (!*pInLine)
	{
		mask = SEQBEGIN_MASK;
		while (fpScript)
		{
			if ( script_gets (InLine, sizeof( InLine ), fpScript) )
			{
				// skip the spaces and tabs of head
				pInLine = InLine;
				while(*pInLine && (*pInLine == ' ' || *pInLine == '\t'))
				{
					pInLine ++;
				}

				if (*pInLine == '\0')
					continue;	/*	empty line.*/

				if (*pInLine == ':')
					continue;	/*	comment line, ignore it.*/

				if(*pInLine == '[')		// command prefix
				{
					// command line
					for (i = 0; Commands[i].DoIt; ++i)
					{
						if(!strncmp (Commands[i].CommandCode, pInLine, strlen(Commands[i].CommandCode)))
						{	
							//	command callback
							(*Commands[i].DoIt) ();
							break;
						}
					}

					if (Commands[i].DoIt)
					{
						continue;
					}

				}

				ScriptLineNo ++;
				break;
			}
			else
			{
				return TESTERSYSERR_ENDOFSCRIPT;
			}
		}
	}

	/*	Get the key code.*/
	key = (UINT)GetOneKey (&pInLine, Modifier);
	if(key < 0)
		return key;

	return key | mask;
}

// 可同时判断按键或触笔
int AutoMessageEx (unsigned char *Modifier, XMPOINT *point, int *action)
{
	UINT		i;
	int		key;
	int		mask = 0;

	// 规避sscanf函数固有的%s格式的缓冲区溢出bug
	char	szKey[MAX_INLINE];
	int		x;
	int		y;
	int		act;

	*Modifier = 0;

	// 检测是否存在字符消息
	if(pWcLine && *pWcLine)
	{	
		
		mask = CHAR_MASK;
		if(pWcLine == WcLine)	// 处理新序列行
		{
			mask |= SEQBEGIN_MASK;
			*pInLine = '\0';	// 强制取新序列行数据
		}

		// skip SPACE & TAB
		while(*pWcLine && (*pWcLine == ' ' || *pWcLine == '\t'))
		{
			*pWcLine ++;
		}

		if(*pWcLine)
		{
			USHORT CharCode = *pWcLine;
			pWcLine ++;
			return CharCode | mask;	
		}
		else
		{
			*pInLine = '\0';	// 强制取新序列行数据
			mask = 0;
		}
	}

	/*	If there are string wait for process in buffer. call PressKey*/
	/*	to analysis it and get the key code.*/
	/*	else, get the next line from sequence file. if sequence file*/
	/*	reach end of file. open the next file.*/

	if (!*pInLine)
	{
		mask = SEQBEGIN_MASK;
		while (fpScript)
		{
			if ( script_gets (InLine, sizeof( InLine ), fpScript) )
			{

				// skip the spaces and tabs of head
				pInLine = InLine;
				while(*pInLine && (*pInLine == ' ' || *pInLine == '\t'))
				{
					pInLine ++;
				}

				if (*pInLine == '\0')
					continue;	/*	empty line.*/

				if (*pInLine == ':')
					continue;	/*	comment line, ignore it.*/
				// 是否为触笔
				memset (szKey, 0, sizeof(szKey));
				if (sscanf (InLine, "%s (%d,%d) %d", szKey, &x, &y, &act) == 4)
				{// 触笔动作独立处理
					(*point).x = x;
					(*point).y = y;
					*action = act;
					while (*pInLine)
					{
						pInLine++;
					}
					return TESTERSYSERR_PENSCRIPT;
				}

				if(*pInLine == '[')		// command prefix
				{
					// command line
					for (i = 0; Commands[i].DoIt; ++i)
					{
						if(!strncmp (Commands[i].CommandCode, pInLine, strlen(Commands[i].CommandCode)))
						{	
							//	command callback
							(*Commands[i].DoIt) ();
							break;
						}
					}


					if (Commands[i].DoIt)
					{
						continue;
					}

				}

				ScriptLineNo ++;
				break;
			}
			else
			{
				return TESTERSYSERR_ENDOFSCRIPT;
			}
		}
	}

	/*	Get the key code.*/
	key = (UINT)GetOneKey (&pInLine, Modifier);
	if(key < 0)
		return key;

	return key | mask;
}


static int LoadKeyMap (const char * lpKeyMapFile)
{
	int	i;
	char	*Cp;
	UINT	KeyValue;
	void *fpKeyMap;
	int	Success = 1;

	// 20090801 ZhuoYongHong
	// 修改键映射文件路径
	if(lpKeyMapFile == NULL || *lpKeyMapFile == '\0')
		return FALSE;

	if((fpKeyMap = XM_fopen(lpKeyMapFile, "r")) == NULL)
		return FALSE;

	memset (Keys, 0, sizeof(Keys));
	KeyCount = 0;

	while (fpKeyMap)
	{
		if (!script_gets_key (InLine, sizeof( InLine ), fpKeyMap))
			break;

		if(InLine[0] == '\0')
			continue;

		Cp = InLine;
		while(*Cp && (*Cp == ' ' || *Cp == '\t'))
			Cp ++;
		if(*Cp == ':')	// cpmment
			continue;

		memset (&Keys[KeyCount], 0, sizeof(Keys[KeyCount]));

		/*	Get the connection information*/
		// 规避sscanf函数固有的%s格式的缓冲区溢出bug
		// i = sscanf (Cp, "%s %x %s", (char *)&Keys[KeyCount].KeyName, &KeyValue, (char *)&Keys[KeyCount].LogName);
		KeyName[0] = 0;
		LogName[0] = 0;
		i = sscanf (Cp, "%s %x %s", (char *)KeyName, &KeyValue, (char *)LogName);
		if (i != 2 && i != 3)
		{
			Success = FALSE;
			break;
		}
		
		if(KeyValue > 0xFF)
		{
			Success = FALSE;
			break;
		}
		if(KeyName[0])
		{
			if(strlen(KeyName) >= sizeof(Keys[KeyCount].KeyName))
			{
				Success = FALSE;
				break;
			}
			strcpy ((char *)Keys[KeyCount].KeyName, KeyName);
		}
		if(LogName[0])
		{
			if(strlen(LogName) >= sizeof(Keys[KeyCount].LogName))
			{
				Success = FALSE;
				break;
			}
			strcpy ((char *)Keys[KeyCount].LogName, LogName);
		}

		if(i == 3)
		{
			// comment check
			if(Keys[KeyCount].LogName[0] == ':')
			{
				memset (Keys[KeyCount].LogName, 0, sizeof(Keys[KeyCount].LogName));
			}
		}

		Keys[KeyCount].KeyValue = (UCHAR)KeyValue;
		Keys[KeyCount].KeyLength = (UCHAR)strlen ((char*)Keys[KeyCount].KeyName);
		Keys[KeyCount].LogLength = (UCHAR)strlen ((char*)Keys[KeyCount].LogName);

		++KeyCount;
	}

	if(Success == FALSE)
	{
		memset (Keys, 0, sizeof(Keys));
		KeyCount = 0;
	}

	XM_fclose (fpKeyMap);
	return Success;
}


// 增加触笔脚本
static int CheckScriptFileEx (const char * lpScriptFile)
{
#ifdef _WINDOWS
	char log_file[512];
#endif
	XMTESTERMESSAGE Message;
	int ScriptError;

	// 设置脚本检查状态
	ScriptState		= SCRIPT_TEST;

#ifdef MAPFILE
	fpScript = open_map_file ((char *)lpScriptFile);
	if(fpScript == NULL)
		return FALSE;

#else
	fpScript = XM_fopen (lpScriptFile, "r");
	if(fpScript == NULL)
		return FALSE;
#endif

#ifdef _WINDOWS
	sprintf (log_file, "%s.log", lpScriptFile);
	fpLog = XM_fopen (log_file, "w");
	if(fpLog == NULL)
	{

#ifdef MAPFILE
		close_map_file (fpScript);
#else
		XM_fclose (fpScript);
#endif
		return FALSE;
	}
#endif
	
	ScriptError = FALSE;
	// 开始语法检查
	while(fpScript)
	{
		int result = TesterGetMessage (&Message);
		if(result == TESTERSYSERR_ENDOFSCRIPT)
			break;
		if(result == TESTERSYSERR_BEGINOFMESSAGE)
			continue;
		if(result < 0)
		{
			ScriptError = 1;
		}
	}
	
#ifdef MAPFILE
	close_map_file (fpScript);
	fpScript = NULL;
#else
	XM_fclose (fpScript);
	fpScript = NULL;
#endif
	
#ifdef _WINDOWS
	XM_fclose (fpLog);
	fpLog = NULL;
#endif

	if(ScriptError)
		return FALSE;
	return 1;
}

int TesterOpenScript (const char * lpScriptFile, const char * lpKeyMapFile)
{
	
	_Init ();
	if(LoadKeyMap(lpKeyMapFile) == FALSE)
		return TESTERSYSERR_INVALIDKEYMAPFILE;

	_Init ();
	strcpy (scriptFile, lpScriptFile);
//	if(CheckScriptFile(lpScriptFile) == FALSE)
	if(CheckScriptFileEx(lpScriptFile) == FALSE)
	{
		memset (scriptFile, 0, sizeof(scriptFile));
		return TESTERSYSERR_INVALIDSCRIPTLINE;
	}

	_Init ();
	
	ScriptState		= SCRIPT_MAKE;
	strcpy (scriptFile, lpScriptFile);
//	scriptFile		= lpScriptFile;
	fpScript			= NULL;
	cbMapData = 0;
//	fpScript = fopen (lpScriptFile, "r");

	return TESTERSYSERR_NOERROR;
}

int TesterCloseScript (void)
{
#ifdef MAPFILE
	close_map_file (fpScript);
	fpScript = NULL;

#else
	if(fpScript)
	{
		XM_fclose (fpScript);
	}
#endif
	_Init ();

	return 1;
}

#define KEYEVENTF_EXTENDEDKEY 0x0001
#define KEYEVENTF_KEYUP       0x0002

#include "xm_user.h"

//#define	XM_KEYDOWN		0x01
										// wp 按键键值，lp 按键状态
										
//#define	XM_KEYUP			0x02
										// wp 按键键值，lp 按键状态

// 可同时处理按键和触笔
int TesterGetMessage (XMTESTERMESSAGE *pTesterMessage)
{
	int	 dwKey;
	unsigned char   chModifier = 0; 	
	// 
//	XMTESTERMESSAGE Message;
	XMPOINT	point;
	int		action;

	if(scriptFile[0] == '\0')
		return TESTERSYSERR_INVALIDSCRIPTLINE;

	if(fpScript == NULL)
	{
#ifdef MAPFILE
		fpScript = open_map_file (scriptFile);
#else
		fpScript = XM_fopen (scriptFile, "r");
#endif
	}

	if(fpScript == NULL)
		return TESTERSYSERR_INVALIDSCRIPTLINE;

	if(ScriptEOF)
	{
		// 自动关闭脚本
		TesterCloseScript ();
		return TESTERSYSERR_ENDOFSCRIPT;
	}

	if(dwKeyDown & KEYDOWN_MASK)
	{
		// 发送WM_KEYUP消息
		pTesterMessage->message = XM_KEYUP;
		pTesterMessage->wParam = (WORD)(dwKeyDown & KEYVALUE_MASK);
		pTesterMessage->lParam = dwModifier;
		dwKeyDown = 0;
		dwModifier = 0;
		return TESTERSYSERR_NOERROR;
	}

	// 可同时处理按键和触笔
	dwKey = AutoMessageEx (&chModifier, &point, &action);

	//dwKey = AutoMessage (&chModifier);
	if(dwKey == TESTERSYSERR_ENDOFSCRIPT)	// 脚本结束
		return TESTERSYSERR_ENDOFSCRIPT;



	if(dwKey < 0)
	{
		return dwKey;
	}

	/*
	// 检测字符消息
	if(dwKey & CHAR_MASK)
	{
		pTesterMessage->message = WM_CHAR;
		pTesterMessage->wParam = (WORD)(dwKey & UNICODE_MASK);
		pTesterMessage->lParam = 0;
		if(dwKey & SEQBEGIN_MASK)
			return TESTERSYSERR_BEGINOFMESSAGE;		// 标记新序列行
		else
			TESTERSYSERR_NOERROR;
	}
	*/


	dwKeyDown = dwKey;
	dwModifier= chModifier;
	if(dwKeyDown & KEYDOWN_MASK)
	{
		// WM_KEYDOWN消息
		pTesterMessage->message = XM_KEYDOWN;
		pTesterMessage->wParam = (WORD)(dwKeyDown & KEYVALUE_MASK);
		pTesterMessage->lParam = chModifier;


		if(dwKeyDown & SEQBEGIN_MASK)
			return TESTERSYSERR_BEGINOFMESSAGE;
		else
			return TESTERSYSERR_NOERROR;
	}
	else
	{
		// 字符消息
		// WM_CHAR
		pTesterMessage->message = 0;
		pTesterMessage->wParam = 0;
		pTesterMessage->lParam = 0;
	}

	return TESTERSYSERR_NOERROR;
}

const char* TesterGetKeyName (DWORD key)
{
	int i;
	for (i = 0; i < KeyCount; i++)
	{
		if(Keys[i].KeyValue == key)
		{
			if(Keys[i].LogLength)
				return (const char *)Keys[i].LogName;
			else
				return (const char *)Keys[i].KeyName;
		}
	}

	return NULL;
}


int TesterKeymap (char * pKeyMapFileName)
{
	if(LoadKeyMap (pKeyMapFileName))
		return TESTERSYSERR_NOERROR;
	else
		return TESTERSYSERR_INVALIDKEYMAPFILE;
}
