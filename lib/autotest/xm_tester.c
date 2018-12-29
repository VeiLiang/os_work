//****************************************************************************
//
//	Copyright (C) 2010 Shenzhen Exceedspace Digital Technology Co.,LTD
//
//	Author	ZhuoYongHong
//
//	File name: xm_tester.c
//	  tester interface
//
//	Revision history
//
//		2006.04.25	ZhuoYongHong begin
//		2009.08.01	ZhuoYongHong 加入笔事件支持
//
//****************************************************************************
#include "xm_type.h"
#include "xm_base.h"
#include "xm_tester.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "wintest.h"
#include <stdio.h>
#include <assert.h>
#include "xm_dev.h"

#define	MAX_PATH		127

static UINT					testerMode = 0;
static int					testerLive = FALSE;
//static FPCAPTUREBITMAP	fpCaptureBitmap  = (FPCAPTUREBITMAP)NULL;	
static FPAUTOTESTSTOP	fpAutotestStop = (FPAUTOTESTSTOP)NULL;		

static int					KeyRecord = FALSE;
static CHAR					RecordFileName[MAX_PATH] = {0};

//static int					BmpRecord = FALSE;
static CHAR					BitmapFileName[MAX_PATH] = {0};


static int	TesterStart (XMTESTERSTART *lpTesterStartParam)
{
	int		result;
	char *		scriptFile;

	if(testerLive == TRUE)
		return TESTERSYSERR_BUSY;

	if(lpTesterStartParam == NULL)
		return TESTERSYSERR_INVALIDPARAM;

	if(lpTesterStartParam->uTestMode == TESTER_MODE_SCRIPT)
	{
		testerMode = lpTesterStartParam->uTestMode;

		scriptFile = (char *)lpTesterStartParam->lpScriptFile;
		if(scriptFile == NULL)
			return TESTERSYSERR_INVALIDPARAM;
		if(*scriptFile == '\0')
			return TESTERSYSERR_INVALIDPARAM;
		
		result = TesterOpenScript (scriptFile, lpTesterStartParam->lpKeyMapFile);
		if(result < 0)
			return result;

		//fpCaptureBitmap = lpTesterStartParam->fpCaptureBitmap;
		fpAutotestStop	= lpTesterStartParam->fpAutotestStop;
		
		//BmpRecord = FALSE;
	}
	else if(lpTesterStartParam->uTestMode == TESTER_MODE_BITMAP)
	{
		return TESTERSYSERR_INVALIDPARAM;
	}
	else
	{
		return TESTERSYSERR_INVALIDPARAM;
	}
	

	testerLive = TRUE;
	return TESTERSYSERR_NOERROR;
}

static int TesterStop (LONG cause)
{
	if(fpAutotestStop)
	{
		(*fpAutotestStop)(cause);
	}

	//BmpRecord = FALSE;
	memset (BitmapFileName, 0, sizeof(BitmapFileName));
	fpAutotestStop = NULL;

	if(testerLive)
	{
		testerMode = 0;
		testerLive = FALSE;
		//fpCaptureBitmap = NULL;
	}

	return TESTERSYSERR_NOERROR;
}




static int TesterMessage (XMTESTERMESSAGE *pMessage)
{
	int result;
	if(testerLive == FALSE)
		return FALSE;

	switch (testerMode)
	{
		case TESTER_MODE_SCRIPT:
		case TESTER_MODE_BITMAP:
			result = TesterGetMessage (pMessage);
			switch(result)
			{
				case TESTERSYSERR_ENDOFSCRIPT:
					// 保存最后一次屏幕信息
					TesterStop (NTF_TESTER_FINISH);
					return FALSE;

				case TESTERSYSERR_BEGINOFMESSAGE:
					// 获得测试脚本中新的一行键或字符消息
					// 保存屏幕信息
					return TRUE;

				case TESTERSYSERR_NOERROR:
					// 获得测试脚本中的一个键或字符消息
					return TRUE;

				default:		// 自动测试消息异常
					TesterStop (NTF_TESTER_FINISH);
					return FALSE;
			}

		default:
			TesterStop (NTF_TESTER_FINISH);
			return FALSE;
	}
}



#if 0
static int TesterRecord (XMTESTERRECORD *pTesterRecord)
{
	FILE	*fpRecord;

	if(pTesterRecord->key == TESTER_RECORD_ON)
	{
		char logbak_filename[MAX_PATH];

		CHAR *pFileName = (CHAR *)pTesterRecord->reserved;
		if(KeyRecord == TRUE)
			return TESTERSYSERR_BUSY;
		if(pFileName == NULL || *pFileName == '\0')
		{
			return TESTERSYSERR_INVALIDPARAM;
		}

		strcpy (RecordFileName, pFileName);

		// 删除BAK文件
		sprintf (logbak_filename, "%s.bak", pFileName);
		remove (logbak_filename);
		// 将老的LOG文件改名为.bak
		rename (pFileName, logbak_filename);
		//if(remove (RecordFileName) < 0)
		//	return TESTERSYSERR_FAILTOCREATERECORD;
		fpRecord = fopen (RecordFileName, "w");
		if(fpRecord == NULL)
			return TESTERSYSERR_FAILTOCREATERECORD;
		fclose (fpRecord);
		KeyRecord = TRUE;
		return TESTERSYSERR_NOERROR;
	}
	else if(pTesterRecord->key == TESTER_RECORD_OFF)
	{
		KeyRecord = FALSE;
		memset (RecordFileName, 0, sizeof(RecordFileName));
		return TESTERSYSERR_NOERROR;
	}
	else if(pTesterRecord->key >= 0 && pTesterRecord->key <= 255)
	{
		if(KeyRecord)
		{
			const char * lpKeyName = TesterGetKeyName (pTesterRecord->key);
			if(lpKeyName)
			{
				FILE *fpRecord = fopen (RecordFileName, "a+");
				if(fpRecord)
				{
					// 写入键值
					fprintf (fpRecord, "%s", lpKeyName);
					fprintf (fpRecord, "\n");
					fclose (fpRecord);
				}
			}
		}
		return TESTERSYSERR_NOERROR;
	}
	else
		return TESTERSYSERR_NOERROR;
}
#endif

int	XM_TesterSendCommand (UINT uTestCommand, VOID *pCommandParam)
{
	switch(uTestCommand)
	{
		case TESTER_CMD_START:
			return TesterStart ((XMTESTERSTART *)pCommandParam);

		case TESTER_CMD_STOP:
			return TesterStop (NTF_TESTER_STOP);

		case TESTER_CMD_MESSAGE:
			return TesterMessage ((XMTESTERMESSAGE *)pCommandParam);

		case TESTER_CMD_TEST:
			return testerLive;

		//case TESTER_CMD_RECORD:
		//	return TesterRecord ((XMTESTERRECORD *)pCommandParam);

		case TESTER_CMD_KEYMAP:
			if(pCommandParam == NULL)
				return TESTERSYSERR_INVALIDPARAM;
			if(testerLive)
				return TESTERSYSERR_BUSY;
			return TesterKeymap (((XMTESTERKEYMAP *)pCommandParam)->lpKeyMapFile);

		default:
			break;
	}

	return TESTERSYSERR_INVALIDCOMMAND;
}



