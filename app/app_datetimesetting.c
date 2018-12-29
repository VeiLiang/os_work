//****************************************************************************
//
//	Copyright (C) 2012 ZhuoYongHong
//
//	Author	ZhuoYongHong
//
//	File name: app_datetimesetting.c
//	  ʱ�����ô���
//
//	Revision history
//
//		2012.09.19	ZhuoYongHong Initial version
//
//****************************************************************************
#include "app.h"

#include "app.h"
#include "app_menuoptionview.h"
#include "app_menuid.h"


#define	OSD_AUTO_HIDE_TIMEOUT			10000	


// ����λ�ú�
enum {
	POS_YEAR = 0,
	POS_MONTH,
	POS_DAY,
	POS_HOUR,
	POS_MINUTE,
	POS_SECOND
};

#define	DATETIMESETTING_COMMAND_NEXT		              0
#define	DATETIMESETTING_COMMAND_OK			          1
#define	DATETIMESETTING_COMMAND_CANCEL	              2
#define	DATETIMESETTING_COMMAND_YEAR_UP	              3
#define	DATETIMESETTING_COMMAND_YEAR_DOWN	          4
#define	DATETIMESETTING_COMMAND_MONTH_UP	          5
#define	DATETIMESETTING_COMMAND_MONTH_DOWN	          6
#define	DATETIMESETTING_COMMAND_DAY_UP	              7
#define	DATETIMESETTING_COMMAND_DAY_DOWN	          8
#define	DATETIMESETTING_COMMAND_HOUR_UP	              9
#define	DATETIMESETTING_COMMAND_HOUR_DOWN	       	  10
#define	DATETIMESETTING_COMMAND_MINUTE_UP	          11
#define	DATETIMESETTING_COMMAND_MINUTE_DOWN	       	  12
#define	DATETIMESETTING_COMMAND_SECOND_UP	       	  13
#define	DATETIMESETTING_COMMAND_SECOND_DOWN	       	  14


// ȱʡ�������԰�ť����(3����ť)(ȷ�ϡ���һ����ȡ��)
#define	DATETIMESETTINGBTNCOUNT_DEFAULT		2
static const XMBUTTONINFO dateTimeSettingBtn_DEFAULT[DATETIMESETTINGBTNCOUNT_DEFAULT] = {
	{	
		VK_AP_MENU,		DATETIMESETTING_COMMAND_OK,		AP_ID_COMMON_MENU,	AP_ID_BUTTON_OK
	},
	
	{	
		VK_AP_SWITCH,	DATETIMESETTING_COMMAND_NEXT,		AP_ID_COMMON_MODE,	AP_ID_BUTTON_TIME_NEXT
	},
	/*
	{	
		VK_AP_MODE,		DATETIMESETTING_COMMAND_CANCEL,	AP_ID_COMMON_OK,	AP_ID_BUTTON_CANCEL
	},
	*/
};
#define	DATETIMESETTINGTPBTNCOUNT	12
static const TPBUTTONINFO dateTimeSettingTpBt[DATETIMESETTINGTPBTNCOUNT] = {
	
	{	
		40,80,88,180,VK_AP_UP,		DATETIMESETTING_COMMAND_YEAR_UP,	AP_ID_DATESETTING_UP,	AP_ID_DATESETTING_UP
	},
	{	
		170,80,218,180,VK_AP_UP,	    DATETIMESETTING_COMMAND_MONTH_UP,	AP_ID_DATESETTING_UP,	AP_ID_DATESETTING_UP
	},
	{	
		270,80,318,180,VK_AP_UP,		DATETIMESETTING_COMMAND_DAY_UP,	AP_ID_DATESETTING_UP,	AP_ID_DATESETTING_UP
	},
	{	
		340,80,388,180,VK_AP_UP,	    DATETIMESETTING_COMMAND_HOUR_UP,	AP_ID_DATESETTING_UP,	AP_ID_DATESETTING_UP
	},
	{	
		440,80,488,180,VK_AP_UP,		DATETIMESETTING_COMMAND_MINUTE_UP,	AP_ID_DATESETTING_UP,	AP_ID_DATESETTING_UP
	},
	{	
		540,80,588,180,VK_AP_UP,		DATETIMESETTING_COMMAND_SECOND_UP,	AP_ID_DATESETTING_UP,	AP_ID_DATESETTING_UP
	},
	{	
		40,140,88,376,VK_AP_DOWN,	DATETIMESETTING_COMMAND_YEAR_DOWN,	AP_ID_DATESETTING_DOWN,	AP_ID_DATESETTING_DOWN
	},
	{	
		170,140,218,376,VK_AP_DOWN,	DATETIMESETTING_COMMAND_MONTH_DOWN,	AP_ID_DATESETTING_DOWN,	AP_ID_DATESETTING_DOWN
	},
	{	
		270,140,318,376,VK_AP_DOWN,	DATETIMESETTING_COMMAND_DAY_DOWN,	AP_ID_DATESETTING_DOWN,	AP_ID_DATESETTING_DOWN
	},
	{	
		340,140,388,376,VK_AP_DOWN,	DATETIMESETTING_COMMAND_HOUR_DOWN,	AP_ID_DATESETTING_DOWN,	AP_ID_DATESETTING_DOWN
	},
	{	
		440,140,488,376,VK_AP_DOWN,  DATETIMESETTING_COMMAND_MINUTE_DOWN,	AP_ID_DATESETTING_DOWN,	AP_ID_DATESETTING_DOWN
	},
	{	
		540,140,588,376,VK_AP_DOWN,	DATETIMESETTING_COMMAND_SECOND_DOWN,	AP_ID_DATESETTING_DOWN,	AP_ID_DATESETTING_DOWN
	},
};

// 2	ǿ��ϵͳʱ�����ö������ԣ�2����ť��ȷ�ϡ���һ�� 
#define	DATETIMESETTINGBTNCOUNT_FORCED		2
static const XMBUTTONINFO dateTimeSettingBtn_FORCED[DATETIMESETTINGBTNCOUNT_FORCED] = {
	{	
		VK_AP_MENU,		DATETIMESETTING_COMMAND_OK,		AP_ID_COMMON_MENU,	AP_ID_BUTTON_OK
	},
	{	
		VK_AP_SWITCH,	DATETIMESETTING_COMMAND_NEXT,		AP_ID_COMMON_MODE,	AP_ID_BUTTON_TIME_NEXT
	},
};

typedef struct tagDATETIMESETTINGDATA {
	int					nCurItem;					// ��ǰѡ��Ĳ˵���

	XMSYSTEMTIME		dateTime;					// ��ǰ���õ�ʱ��

	XMBUTTONCONTROL		btnControl;				// ��ť�ؼ�
	TPBUTTONCONTROL		TPbtnControl;				// ��ť�ؼ�
	XMTITLEBARCONTROL 	titleControl;				// ����ؼ�
	XM_IMAGE *			pImageUp;
	XM_IMAGE *			pImageDown;

} DATETIMESETTINGDATA;


VOID DateTimeSettingOnEnter (XMMSG *msg)
{
	if(msg->wp == 0)
	{
		// ����δ��������һ�ν���
		DATETIMESETTINGDATA *dateTimeSettingData;
		//APPROMRES *AppRes;
	
		// ����˽�����ݾ��
		dateTimeSettingData = XM_calloc (sizeof(DATETIMESETTINGDATA));
		if(dateTimeSettingData == NULL)
		{
			XM_printf ("dateTimeSettingData XM_calloc failed\n");
			// ʧ�ܷ��ص������ߴ���
			XM_PullWindow (0);
			return;
		}

		// ��ʼ��˽������
		dateTimeSettingData->nCurItem = POS_YEAR;		// ָ�����������

		// ��ȡ��ǰϵͳʱ����Ϊʱ�����õĳ�ʼֵ����ʱ���Ѷ�ʧ(RTC����)��ϵͳ���Զ�����Ϊĳ��ȱʡʱ�䡣��20130101
		XM_GetLocalTime (&dateTimeSettingData->dateTime);

		// ��ť�ؼ���ʼ��
		if(msg->lp == APP_DATETIMESETTING_CUSTOM_FORCED)
		{
			AP_ButtonControlInit (&dateTimeSettingData->btnControl, DATETIMESETTINGBTNCOUNT_FORCED, 
				XMHWND_HANDLE(DateTimeSetting), dateTimeSettingBtn_FORCED);
		}
		else
		{
			AP_ButtonControlInit (&dateTimeSettingData->btnControl, DATETIMESETTINGBTNCOUNT_DEFAULT, 
				XMHWND_HANDLE(DateTimeSetting), dateTimeSettingBtn_DEFAULT);
		}
		// ����ؼ���ʼ��
		AP_TitleBarControlInit (&dateTimeSettingData->titleControl, XMHWND_HANDLE(DateTimeSetting), 
														AP_NULLID, AP_ID_SETTING_TIME_TITLE);

		dateTimeSettingData->pImageUp = XM_RomImageCreate (ROM_T18_COMMON_UP_26_PNG, 
			ROM_T18_COMMON_UP_26_PNG_SIZE, XM_OSD_LAYER_FORMAT_ARGB888);
		dateTimeSettingData->pImageDown = XM_RomImageCreate (ROM_T18_COMMON_DOWN_26_PNG, 
			ROM_T18_COMMON_DOWN_26_PNG_SIZE, XM_OSD_LAYER_FORMAT_ARGB888);

	    // ��ť�ؼ���ʼ��
		//AP_TpButtonControlInit (&dateTimeSettingData->TPbtnControl, DATETIMESETTINGTPBTNCOUNT, XMHWND_HANDLE(DateTimeSetting), dateTimeSettingTpBt);
		
		// ���ô��ڵ�˽�����ݾ��
		XM_SetWindowPrivateData (XMHWND_HANDLE(DateTimeSetting), dateTimeSettingData);
	}
	else
	{
		// �����ѽ�������ǰ���ڴ�ջ�лָ�
		XM_printf ("DateTimeSetting Pull\n");
	}
	
	// ������ʱ�������ڲ˵�������
	// ����x��Ķ�ʱ��
	XM_SetTimer (XMTIMER_DATETIMESETTING, OSD_AUTO_HIDE_TIMEOUT);
}

VOID DateTimeSettingOnLeave (XMMSG *msg)
{
	// ɾ����ʱ��
	XM_KillTimer (XMTIMER_DATETIMESETTING);
	
	if (msg->wp == 0)
	{
		// �����˳������״ݻ١�
		// ��ȡ˽�����ݾ��
		DATETIMESETTINGDATA *dateTimeSettingData = (DATETIMESETTINGDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(DateTimeSetting));
		// �ͷ����з������Դ
		if(dateTimeSettingData)
		{
			// ��ť�ؼ��˳�����
			AP_ButtonControlExit (&dateTimeSettingData->btnControl);
			// ����ؼ��˳�����
			AP_TitleBarControlExit (&dateTimeSettingData->titleControl);

			XM_ImageDelete (dateTimeSettingData->pImageUp);
			XM_ImageDelete (dateTimeSettingData->pImageDown);

			// �ͷ�˽�����ݾ��
			XM_free (dateTimeSettingData);
		}
		// ���ô��ڵ�˽�����ݾ��Ϊ��
		XM_SetWindowPrivateData (XMHWND_HANDLE(DateTimeSetting), NULL);
		XM_printf ("DateTimeSetting Exit\n");
	}
	else
	{
		// ��ת���������ڣ���ǰ���ڱ�ѹ��ջ�С�
		// �ѷ������Դ����
		XM_printf ("DateTimeSetting Push\n");
	}
}



VOID DateTimeSettingOnPaint (XMMSG *msg)
{
	XMRECT rc, rect;
	XMCOORD x, y;
	XMCOORD x2, y2;
	unsigned int old_alpha;
	char text[32];
	char text2[32];
	XMSIZE size;
	int arrow_cx, arrow_cy;
	float scale_factor;		// ˮƽ��������

	DATETIMESETTINGDATA *dateTimeSettingData;

	dateTimeSettingData = (DATETIMESETTINGDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(DateTimeSetting));
	if(dateTimeSettingData == NULL)
		return;

	// ��ʾ������
	XM_GetDesktopRect (&rc);
	XM_FillRect (XMHWND_HANDLE(DateTimeSetting), rc.left, rc.top, rc.right, rc.bottom, XM_GetSysColor(XM_COLOR_DESKTOP));

	old_alpha = XM_GetWindowAlpha (XMHWND_HANDLE(DateTimeSetting));
	XM_SetWindowAlpha (XMHWND_HANDLE(DateTimeSetting), 255);

	// ����ˮƽ��������(UI��320X240������)
	scale_factor = (float)((rc.right - rc.left + 1) / 320.0);

	// --------------------------------------
	//
	// ********* 1 ��ʾ����������Ϣ *********
	//
	// --------------------------------------
	//AP_DrawTitlebarControl (XMHWND_HANDLE(DateTimeSetting), AP_NULLID, AP_ID_SETTING_TIME_TITLE);
	// �������ؼ�����ʾ��
	// �����ڱ���ؼ����������AP_TitleControlMessageHandlerִ�б���ؼ���ʾ
	AP_TitleBarControlMessageHandler (&dateTimeSettingData->titleControl, msg);

	// --------------------------------------
	//
	// ********* 2 ��ʾ�˵����� *************
	//
	// --------------------------------------
	rect = rc;
	// ��ʽ2�� ������� "2012 / 09 / 16"
	AP_FormatDataTime (&dateTimeSettingData->dateTime, APP_DATETIME_FORMAT_2, text, sizeof(text));
	AP_TextGetStringSize (text, strlen(text), &size);
	x = (XMCOORD)( (rc.left + (rc.right - rc.left + 1 - 2*size.cx) / 2));
	y = (XMCOORD)(scale_factor * (APP_POS_ITEM5_MENUNAME_Y + APP_POS_ITEM5_LINEHEIGHT_DATESETTING * 1));
    //y-=50;
	// ��ʽ4�� ���ʱ���� "10 : 22 : 20"
	AP_FormatDataTime (&dateTimeSettingData->dateTime, APP_DATETIME_FORMAT_4, text2, sizeof(text2));
	AP_TextGetStringSize (text2, strlen(text2), &size);
	x2 = x+size.cx+80;//(XMCOORD)( (rc.left + (rc.right - rc.left + 1 - size.cx) / 2));
	y2 = y;//(XMCOORD)(scale_factor * (APP_POS_ITEM5_MENUNAME_Y + APP_POS_ITEM5_LINEHEIGHT_DATESETTING * 3));
    
	// ��ʽ2�� ������� "2012 / 09 / 16"
	AP_TextOutDataTimeString (XMHWND_HANDLE(DateTimeSetting), x, y, text, strlen(text));
	// ��ʽ4�� ���ʱ���� "10 : 22 : 20"
	AP_TextOutDataTimeString (XMHWND_HANDLE(DateTimeSetting), x2, y2, text2, strlen(text2));

	
	#if 1
	if(dateTimeSettingData->pImageUp)
	{
		arrow_cx = dateTimeSettingData->pImageUp->width;
		arrow_cy = dateTimeSettingData->pImageUp->height;
	}
	else
	{
		arrow_cx = 0;
		arrow_cy = 0;
	}


	if(dateTimeSettingData->nCurItem <= 2)
	{
		// 2012 / 09 / 16
		rect.top = (XMCOORD)(scale_factor * (APP_POS_ITEM5_MENUNAME_Y + APP_POS_ITEM5_LINEHEIGHT_DATESETTING * 1 - arrow_cy - 4));
		if(dateTimeSettingData->nCurItem == 0)		// 2012
			rect.left = (XMCOORD)( (x + (4 * DATETIME_CHAR_WIDTH - arrow_cx)/2));
		else if(dateTimeSettingData->nCurItem == 1)		// 09
			rect.left = (XMCOORD)( (x + 7 * DATETIME_CHAR_WIDTH + (2 * DATETIME_CHAR_WIDTH - arrow_cx)/2));
		else if(dateTimeSettingData->nCurItem == 2)		// 16
			rect.left = (XMCOORD)( (x + 12 * DATETIME_CHAR_WIDTH + (2 * DATETIME_CHAR_WIDTH - arrow_cx)/2));

				  
	}
	else
	{
		rect.top = (XMCOORD)(scale_factor * (APP_POS_ITEM5_MENUNAME_Y + APP_POS_ITEM5_LINEHEIGHT_DATESETTING * 1 - arrow_cy - 4));//3 //  3
		// 10 : 22 : 20
		if(dateTimeSettingData->nCurItem == 3)		// 10
			rect.left = (XMCOORD)( (x2 + (2 * DATETIME_CHAR_WIDTH - arrow_cx)/2));
		else if(dateTimeSettingData->nCurItem == 4)		// 22
			rect.left = (XMCOORD)( (x2 + 5 * DATETIME_CHAR_WIDTH + (2 * DATETIME_CHAR_WIDTH - arrow_cx)/2));
		else if(dateTimeSettingData->nCurItem == 5)		// 16
			rect.left = (XMCOORD)( (x2 + 10 * DATETIME_CHAR_WIDTH + (2 * DATETIME_CHAR_WIDTH - arrow_cx)/2));
             
	}
        
//	XM_DrawImageEx(XM_RomAddress(ROM_T18_COMMON_UP_26_PNG), ROM_T18_COMMON_UP_26_PNG_SIZE, 
//		XMHWND_HANDLE(DateTimeSetting),
//		&rect, XMGIF_DRAW_POS_LEFTTOP);
	XM_ImageDisplay (dateTimeSettingData->pImageUp, XMHWND_HANDLE(DateTimeSetting), rect.left, rect.top);

	rect.top = rect.top + (XMCOORD)(scale_factor * (size.cy + 4 + 4 + arrow_cy));
//	XM_DrawImageEx(XM_RomAddress(ROM_T18_COMMON_DOWN_26_PNG), ROM_T18_COMMON_DOWN_26_PNG_SIZE, 
//		XMHWND_HANDLE(DateTimeSetting),
//		&rect, XMGIF_DRAW_POS_LEFTTOP);
	XM_ImageDisplay (dateTimeSettingData->pImageDown, XMHWND_HANDLE(DateTimeSetting), rect.left, rect.top);
    #endif

	// ����ť�ؼ�����ʾ��
	// �����ڰ�ť�ؼ����������AP_ButtonControlMessageHandlerִ�а�ť�ؼ���ʾ
	AP_ButtonControlMessageHandler (&dateTimeSettingData->btnControl, msg);
    AP_TpButtonControlMessageHandler (&dateTimeSettingData->TPbtnControl, msg);
	XM_SetWindowAlpha (XMHWND_HANDLE(DateTimeSetting), (unsigned char)old_alpha);

}

static void AdjustDateTime (XMSYSTEMTIME *dateTime, int nItem, int diff)
{
	UINT16  _month[12] = {31,28,31,30,31,30,31,31,30,31,30,31};
	int v;
	switch (nItem)
	{
		case 0:	// �� (2012 ~ 2049)
			v = dateTime->wYear + diff;
			if(v < 2012)
				v = 2049;
			else if(v > 2049)
				v = 2012;
			dateTime->wYear = (WORD)v;
			
			// ����2��
			if( 	dateTime->wYear % 4 == 0  && dateTime->wYear % 100 != 0   
				|| dateTime->wYear % 400 == 0 )
			{
				_month[1] = 29;
			}
			// �������
			if(dateTime->bDay > (BYTE)_month[dateTime->bMonth - 1])
				dateTime->bDay = (BYTE)_month[dateTime->bMonth - 1];
						
			break;

		case 1:	// ��	(1 ~ 12)
		{
			if( 	dateTime->wYear % 4 == 0  && dateTime->wYear % 100 != 0   
				|| dateTime->wYear % 400 == 0 )
			{
				_month[1] = 29;
			}

			v = dateTime->bMonth + diff;
			if(v < 1)
				v = 12;
			else if(v > 12)
				v = 1;
			dateTime->bMonth = (BYTE)v;
			
			// �������
			if(dateTime->bDay > (BYTE)_month[dateTime->bMonth - 1])
				dateTime->bDay = (BYTE)_month[dateTime->bMonth - 1];
			
			break;
		}

		case 2:	// �� (1 ~ 31)
		{
			if( 	dateTime->wYear % 4 == 0  && dateTime->wYear % 100 != 0   
				|| dateTime->wYear % 400 == 0 )
			{
				_month[1] = 29;
			}
		
			v = dateTime->bDay + diff;
			if(v < 1)
				v = _month[dateTime->bMonth - 1];
			else if(v > _month[dateTime->bMonth - 1])
				v = 1;
			
			dateTime->bDay = (BYTE)v;
			break;
		}

		case 3:	// ʱ (0 ~ 23)
			v = dateTime->bHour + diff;
			if(v < 0)
				v = 23;
			else if(v > 23)
				v = 0;
			dateTime->bHour = (BYTE)v;
			break;

		case 4:	// �� (0 ~ 59)
			v = dateTime->bMinute + diff;
			if(v < 0)
				v = 59;
			else if(v > 59)
				v = 0;
			dateTime->bMinute = (BYTE)v;
			break;

		case 5:	// �� (0 ~ 59)
			v = dateTime->bSecond + diff;
			if(v < 0)
				v = 59;
			else if(v > 59)
				v = 0;
			dateTime->bSecond = (BYTE)v;
			break;
	}
}

VOID DateTimeSettingOnKeyDown (XMMSG *msg)
{
	DATETIMESETTINGDATA *dateTimeSettingData;
	dateTimeSettingData = (DATETIMESETTINGDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(DateTimeSetting));
	if(dateTimeSettingData == NULL)
		return;
	
	// ������
	XM_Beep (XM_BEEP_KEYBOARD);
	
	switch(msg->wp)
	{
		case VK_AP_MENU:		// �˵���
		case VK_AP_MODE:
		case VK_AP_SWITCH:
			// ��"ʱ������"�����У�MENU��MODE��������Ϊ��ť������
			// �˴������������¼�������ť�ؼ�����ȱʡ������ɱ�Ҫ����ʾЧ������
			AP_ButtonControlMessageHandler (&dateTimeSettingData->btnControl, msg);
			XM_SetTimer (XMTIMER_DATETIMESETTING, OSD_AUTO_HIDE_TIMEOUT);
			break;
			
        #if 1
		case VK_AP_UP:		
			AP_ButtonControlMessageHandler (&dateTimeSettingData->btnControl, msg);
			AdjustDateTime (&dateTimeSettingData->dateTime, dateTimeSettingData->nCurItem, 1);
			// ˢ��
			XM_InvalidateWindow ();
			XM_UpdateWindow ();
			break;

		case VK_AP_DOWN:
			AP_ButtonControlMessageHandler (&dateTimeSettingData->btnControl, msg);
			AdjustDateTime (&dateTimeSettingData->dateTime, dateTimeSettingData->nCurItem, -1);
			// ˢ��
			XM_InvalidateWindow ();
			XM_UpdateWindow ();
			break;
        #endif
		
		default:
			AP_ButtonControlMessageHandler (&dateTimeSettingData->btnControl, msg);
			XM_SetTimer (XMTIMER_DATETIMESETTING, OSD_AUTO_HIDE_TIMEOUT);
			break;
	}

}

VOID DateTimeSettingOnKeyUp (XMMSG *msg)
{
	DATETIMESETTINGDATA *dateTimeSettingData;
	dateTimeSettingData = (DATETIMESETTINGDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(DateTimeSetting));
	if(dateTimeSettingData == NULL)
		return;

	switch(msg->wp)
	{
		case VK_AP_MENU:		// �˵���
		case VK_AP_MODE:		// �л������г���¼��״̬
		case VK_AP_SWITCH:
			// ��"ʱ������"�����У�MENU��MODE��������Ϊ��ť������
			// �˴������������¼�������ť�ؼ�����ȱʡ������ɱ�Ҫ����ʾЧ������
			AP_ButtonControlMessageHandler (&dateTimeSettingData->btnControl, msg);
			break;

		default:
			AP_ButtonControlMessageHandler (&dateTimeSettingData->btnControl, msg);
			break;
	}
}

VOID DateTimeSettingOnCommand (XMMSG *msg)
{
	DATETIMESETTINGDATA *dateTimeSettingData;
	dateTimeSettingData = (DATETIMESETTINGDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(DateTimeSetting));
	if(dateTimeSettingData == NULL)
		return;

	switch(msg->wp)
	{
		case DATETIMESETTING_COMMAND_NEXT:		// �˵���
			dateTimeSettingData->nCurItem ++;
			if(dateTimeSettingData->nCurItem > 5)
				dateTimeSettingData->nCurItem = 0;
			// ˢ��
			XM_InvalidateWindow ();
			XM_UpdateWindow ();
			break;

		case DATETIMESETTING_COMMAND_OK:
			// ����Ϊ"ʱ������"���ڵ�˽����Ϊ����
			// ���ص�����
			// ����ǰʱ������Ϊϵͳʱ��
			XM_SetLocalTime (&dateTimeSettingData->dateTime);
			XM_PullWindow (0);
			break;

		case DATETIMESETTING_COMMAND_CANCEL:
			// ���ص�����
			XM_PullWindow (0);
			break;
			
		case DATETIMESETTING_COMMAND_YEAR_UP:	
			dateTimeSettingData->nCurItem=0;	
			AP_ButtonControlMessageHandler (&dateTimeSettingData->btnControl, msg);
			AdjustDateTime (&dateTimeSettingData->dateTime, dateTimeSettingData->nCurItem, 1);
			// ˢ��
			XM_InvalidateWindow ();
			XM_UpdateWindow ();
			XM_SetTimer (XMTIMER_DATETIMESETTING, OSD_AUTO_HIDE_TIMEOUT);
			break;

	    case DATETIMESETTING_COMMAND_YEAR_DOWN:
			dateTimeSettingData->nCurItem=0;	  	
			AP_ButtonControlMessageHandler (&dateTimeSettingData->btnControl, msg);
			AdjustDateTime (&dateTimeSettingData->dateTime, dateTimeSettingData->nCurItem, -1);
			// ˢ��
			XM_InvalidateWindow ();
			XM_UpdateWindow ();
			XM_SetTimer (XMTIMER_DATETIMESETTING, OSD_AUTO_HIDE_TIMEOUT);
			break;
			
		case DATETIMESETTING_COMMAND_MONTH_UP:	
			dateTimeSettingData->nCurItem=1;	
			AP_ButtonControlMessageHandler (&dateTimeSettingData->btnControl, msg);
			AdjustDateTime (&dateTimeSettingData->dateTime, dateTimeSettingData->nCurItem, 1);
			// ˢ��
			XM_InvalidateWindow ();
			XM_UpdateWindow ();
			XM_SetTimer (XMTIMER_DATETIMESETTING, OSD_AUTO_HIDE_TIMEOUT);
			break;

        case DATETIMESETTING_COMMAND_MONTH_DOWN:
			dateTimeSettingData->nCurItem=1;	  	
			AP_ButtonControlMessageHandler (&dateTimeSettingData->btnControl, msg);
			AdjustDateTime (&dateTimeSettingData->dateTime, dateTimeSettingData->nCurItem, -1);
			// ˢ��
			XM_InvalidateWindow ();
			XM_UpdateWindow ();
			XM_SetTimer (XMTIMER_DATETIMESETTING, OSD_AUTO_HIDE_TIMEOUT);
			break;
		
		case DATETIMESETTING_COMMAND_DAY_UP:	
			dateTimeSettingData->nCurItem=2;	
			AP_ButtonControlMessageHandler (&dateTimeSettingData->btnControl, msg);
			AdjustDateTime (&dateTimeSettingData->dateTime, dateTimeSettingData->nCurItem, 1);
			// ˢ��
			XM_InvalidateWindow ();
			XM_UpdateWindow ();
			XM_SetTimer (XMTIMER_DATETIMESETTING, OSD_AUTO_HIDE_TIMEOUT);
			break;

        case DATETIMESETTING_COMMAND_DAY_DOWN:
			dateTimeSettingData->nCurItem=2;	  	
			AP_ButtonControlMessageHandler (&dateTimeSettingData->btnControl, msg);
			AdjustDateTime (&dateTimeSettingData->dateTime, dateTimeSettingData->nCurItem, -1);
			// ˢ��
			XM_InvalidateWindow ();
			XM_UpdateWindow ();
			XM_SetTimer (XMTIMER_DATETIMESETTING, OSD_AUTO_HIDE_TIMEOUT);
			break;
		
		case DATETIMESETTING_COMMAND_HOUR_UP:	
			dateTimeSettingData->nCurItem=3;	
			AP_ButtonControlMessageHandler (&dateTimeSettingData->btnControl, msg);
			AdjustDateTime (&dateTimeSettingData->dateTime, dateTimeSettingData->nCurItem, 1);
			// ˢ��
			XM_InvalidateWindow ();
			XM_UpdateWindow ();
			XM_SetTimer (XMTIMER_DATETIMESETTING, OSD_AUTO_HIDE_TIMEOUT);
			break;

        case DATETIMESETTING_COMMAND_HOUR_DOWN:
			dateTimeSettingData->nCurItem=3;	  	
			AP_ButtonControlMessageHandler (&dateTimeSettingData->btnControl, msg);
			AdjustDateTime (&dateTimeSettingData->dateTime, dateTimeSettingData->nCurItem, -1);
			// ˢ��
			XM_InvalidateWindow ();
			XM_UpdateWindow ();
			XM_SetTimer (XMTIMER_DATETIMESETTING, OSD_AUTO_HIDE_TIMEOUT);
			break;
		
		case DATETIMESETTING_COMMAND_MINUTE_UP:	
			dateTimeSettingData->nCurItem=4;	
			AP_ButtonControlMessageHandler (&dateTimeSettingData->btnControl, msg);
			AdjustDateTime (&dateTimeSettingData->dateTime, dateTimeSettingData->nCurItem, 1);
			// ˢ��
			XM_InvalidateWindow ();
			XM_UpdateWindow ();
			XM_SetTimer (XMTIMER_DATETIMESETTING, OSD_AUTO_HIDE_TIMEOUT);
			break;

        case DATETIMESETTING_COMMAND_MINUTE_DOWN:
			dateTimeSettingData->nCurItem=4;	  	
			AP_ButtonControlMessageHandler (&dateTimeSettingData->btnControl, msg);
			AdjustDateTime (&dateTimeSettingData->dateTime, dateTimeSettingData->nCurItem, -1);
			// ˢ��
			XM_InvalidateWindow ();
			XM_UpdateWindow ();
			XM_SetTimer (XMTIMER_DATETIMESETTING, OSD_AUTO_HIDE_TIMEOUT);
			break;
		
		case DATETIMESETTING_COMMAND_SECOND_UP:	
			dateTimeSettingData->nCurItem=5;	
			AP_ButtonControlMessageHandler (&dateTimeSettingData->btnControl, msg);
			AdjustDateTime (&dateTimeSettingData->dateTime, dateTimeSettingData->nCurItem, 1);
			// ˢ��
			XM_InvalidateWindow ();
			XM_UpdateWindow ();
			XM_SetTimer (XMTIMER_DATETIMESETTING, OSD_AUTO_HIDE_TIMEOUT);
			break;

        case DATETIMESETTING_COMMAND_SECOND_DOWN:
			dateTimeSettingData->nCurItem=5;	  	
			AP_ButtonControlMessageHandler (&dateTimeSettingData->btnControl, msg);
			AdjustDateTime (&dateTimeSettingData->dateTime, dateTimeSettingData->nCurItem, -1);
			// ˢ��
			XM_InvalidateWindow ();
			XM_UpdateWindow ();
			XM_SetTimer (XMTIMER_DATETIMESETTING, OSD_AUTO_HIDE_TIMEOUT);
			break;
	}
}


static VOID DateTimeSettingOnTimer (XMMSG *msg)
{
	DATETIMESETTINGDATA *dateTimeSettingData;
	dateTimeSettingData = (DATETIMESETTINGDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(DateTimeSetting));
	if(dateTimeSettingData == NULL)
		return;

	// ���ص�����
	XM_PullWindow(XMHWND_HANDLE(Desktop));
}


VOID DateTimeSettingOnTouchDown (XMMSG *msg)
{
    int x,y;
	int index;
	DATETIMESETTINGDATA *dateTimeSettingData;
	
	dateTimeSettingData = (DATETIMESETTINGDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(DateTimeSetting));
	if(dateTimeSettingData == NULL)
		return;
	
	x = LOWORD(msg->lp);
	y = HIWORD(msg->lp);
	//XM_printf("x is %d\n",x);
	//XM_printf("y is %d\n",y);
	msg->lp=MAKELONG(x,y);
	
	if(AP_TpButtonControlMessageHandlerDateSet (&dateTimeSettingData->TPbtnControl, msg))
		return;
	
	if(AP_ButtonControlMessageHandler (&dateTimeSettingData->btnControl, msg))
		return;

	XM_SetTimer (XMTIMER_DATETIMESETTING, OSD_AUTO_HIDE_TIMEOUT);

   /*
	index = AppLocateItem (XMHWND_HANDLE(systemVersionViewData), systemVersionViewData->nItemCount, APP_POS_ITEM5_LINEHEIGHT, settingViewData->nTopItem, LOWORD(msg->lp), HIWORD(msg->lp));
	if(index < 0)
		return;
	systemVersionViewData->nTouchItem = index;
	if(systemVersionViewData->nCurItem != index)
	{
		systemVersionViewData->nCurItem = index;
	
		XM_InvalidateWindow ();
		XM_UpdateWindow ();
	}
    */
}

VOID DateTimeSettingOnTouchUp (XMMSG *msg)
{
       int x,y;
	   DATETIMESETTINGDATA *dateTimeSettingData;
	dateTimeSettingData = (DATETIMESETTINGDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(DateTimeSetting));
	if(dateTimeSettingData == NULL)
		return;
	
	x = LOWORD(msg->lp);
	y = HIWORD(msg->lp);
	msg->lp=MAKELONG(x,y);
	
	if(AP_TpButtonControlMessageHandlerDateSet (&dateTimeSettingData->TPbtnControl, msg))
		return;
	
	if(AP_ButtonControlMessageHandler (&dateTimeSettingData->btnControl, msg))
		return;
	
	#if 0
	if(systemVersionViewData->nTouchItem == -1)
		return;
	systemVersionViewData->nTouchItem = -1;
	XM_PostMessage (XM_COMMAND, systemVersionViewData->btnControl.pBtnInfo[0].wCommand, systemVersionViewData->nCurItem);
    #endif  
}
// ��Ϣ����������
XM_MESSAGE_MAP_BEGIN (DateTimeSetting)
	XM_ON_MESSAGE (XM_PAINT, DateTimeSettingOnPaint)
	XM_ON_MESSAGE (XM_KEYDOWN, DateTimeSettingOnKeyDown)
	XM_ON_MESSAGE (XM_KEYUP, DateTimeSettingOnKeyUp)
	XM_ON_MESSAGE (XM_ENTER, DateTimeSettingOnEnter)
	XM_ON_MESSAGE (XM_LEAVE, DateTimeSettingOnLeave)
	XM_ON_MESSAGE (XM_COMMAND, DateTimeSettingOnCommand)
	XM_ON_MESSAGE (XM_TIMER, DateTimeSettingOnTimer)
	XM_ON_MESSAGE (XM_TOUCHDOWN, DateTimeSettingOnTouchDown)
	XM_ON_MESSAGE (XM_TOUCHUP, DateTimeSettingOnTouchUp)
XM_MESSAGE_MAP_END

// �����Ӵ�����
#ifdef __ICCARM__
#pragma data_alignment=32
#endif
XMHWND_DEFINE(0, 0, LCD_XDOTS, LCD_YDOTS, DateTimeSetting, 0, 0, 0, XM_VIEW_DEFAULT_ALPHA, HWND_VIEW)

