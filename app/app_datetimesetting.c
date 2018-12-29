//****************************************************************************
//
//	Copyright (C) 2012 ZhuoYongHong
//
//	Author	ZhuoYongHong
//
//	File name: app_datetimesetting.c
//	  时间设置窗口
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


// 定义位置宏
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


// 缺省定制属性按钮定义(3个按钮)(确认、下一个、取消)
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

// 2	强制系统时间设置定制属性，2个按钮，确认、下一个 
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
	int					nCurItem;					// 当前选择的菜单项

	XMSYSTEMTIME		dateTime;					// 当前设置的时间

	XMBUTTONCONTROL		btnControl;				// 按钮控件
	TPBUTTONCONTROL		TPbtnControl;				// 按钮控件
	XMTITLEBARCONTROL 	titleControl;				// 标题控件
	XM_IMAGE *			pImageUp;
	XM_IMAGE *			pImageDown;

} DATETIMESETTINGDATA;


VOID DateTimeSettingOnEnter (XMMSG *msg)
{
	if(msg->wp == 0)
	{
		// 窗口未建立，第一次进入
		DATETIMESETTINGDATA *dateTimeSettingData;
		//APPROMRES *AppRes;
	
		// 分配私有数据句柄
		dateTimeSettingData = XM_calloc (sizeof(DATETIMESETTINGDATA));
		if(dateTimeSettingData == NULL)
		{
			XM_printf ("dateTimeSettingData XM_calloc failed\n");
			// 失败返回到调用者窗口
			XM_PullWindow (0);
			return;
		}

		// 初始化私有数据
		dateTimeSettingData->nCurItem = POS_YEAR;		// 指向年份设置项

		// 获取当前系统时间作为时间设置的初始值。若时间已丢失(RTC掉电)，系统会自动设置为某个缺省时间。如20130101
		XM_GetLocalTime (&dateTimeSettingData->dateTime);

		// 按钮控件初始化
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
		// 标题控件初始化
		AP_TitleBarControlInit (&dateTimeSettingData->titleControl, XMHWND_HANDLE(DateTimeSetting), 
														AP_NULLID, AP_ID_SETTING_TIME_TITLE);

		dateTimeSettingData->pImageUp = XM_RomImageCreate (ROM_T18_COMMON_UP_26_PNG, 
			ROM_T18_COMMON_UP_26_PNG_SIZE, XM_OSD_LAYER_FORMAT_ARGB888);
		dateTimeSettingData->pImageDown = XM_RomImageCreate (ROM_T18_COMMON_DOWN_26_PNG, 
			ROM_T18_COMMON_DOWN_26_PNG_SIZE, XM_OSD_LAYER_FORMAT_ARGB888);

	    // 按钮控件初始化
		//AP_TpButtonControlInit (&dateTimeSettingData->TPbtnControl, DATETIMESETTINGTPBTNCOUNT, XMHWND_HANDLE(DateTimeSetting), dateTimeSettingTpBt);
		
		// 设置窗口的私有数据句柄
		XM_SetWindowPrivateData (XMHWND_HANDLE(DateTimeSetting), dateTimeSettingData);
	}
	else
	{
		// 窗口已建立，当前窗口从栈中恢复
		XM_printf ("DateTimeSetting Pull\n");
	}
	
	// 创建定时器，用于菜单的隐藏
	// 创建x秒的定时器
	XM_SetTimer (XMTIMER_DATETIMESETTING, OSD_AUTO_HIDE_TIMEOUT);
}

VOID DateTimeSettingOnLeave (XMMSG *msg)
{
	// 删除定时器
	XM_KillTimer (XMTIMER_DATETIMESETTING);
	
	if (msg->wp == 0)
	{
		// 窗口退出，彻底摧毁。
		// 获取私有数据句柄
		DATETIMESETTINGDATA *dateTimeSettingData = (DATETIMESETTINGDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(DateTimeSetting));
		// 释放所有分配的资源
		if(dateTimeSettingData)
		{
			// 按钮控件退出过程
			AP_ButtonControlExit (&dateTimeSettingData->btnControl);
			// 标题控件退出过程
			AP_TitleBarControlExit (&dateTimeSettingData->titleControl);

			XM_ImageDelete (dateTimeSettingData->pImageUp);
			XM_ImageDelete (dateTimeSettingData->pImageDown);

			// 释放私有数据句柄
			XM_free (dateTimeSettingData);
		}
		// 设置窗口的私有数据句柄为空
		XM_SetWindowPrivateData (XMHWND_HANDLE(DateTimeSetting), NULL);
		XM_printf ("DateTimeSetting Exit\n");
	}
	else
	{
		// 跳转到其他窗口，当前窗口被压入栈中。
		// 已分配的资源保留
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
	float scale_factor;		// 水平缩放因子

	DATETIMESETTINGDATA *dateTimeSettingData;

	dateTimeSettingData = (DATETIMESETTINGDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(DateTimeSetting));
	if(dateTimeSettingData == NULL)
		return;

	// 显示标题栏
	XM_GetDesktopRect (&rc);
	XM_FillRect (XMHWND_HANDLE(DateTimeSetting), rc.left, rc.top, rc.right, rc.bottom, XM_GetSysColor(XM_COLOR_DESKTOP));

	old_alpha = XM_GetWindowAlpha (XMHWND_HANDLE(DateTimeSetting));
	XM_SetWindowAlpha (XMHWND_HANDLE(DateTimeSetting), 255);

	// 计算水平缩放因子(UI按320X240规格设计)
	scale_factor = (float)((rc.right - rc.left + 1) / 320.0);

	// --------------------------------------
	//
	// ********* 1 显示标题栏区信息 *********
	//
	// --------------------------------------
	//AP_DrawTitlebarControl (XMHWND_HANDLE(DateTimeSetting), AP_NULLID, AP_ID_SETTING_TIME_TITLE);
	// 处理标题控件的显示。
	// 若存在标题控件，必须调用AP_TitleControlMessageHandler执行标题控件显示
	AP_TitleBarControlMessageHandler (&dateTimeSettingData->titleControl, msg);

	// --------------------------------------
	//
	// ********* 2 显示菜单项区 *************
	//
	// --------------------------------------
	rect = rc;
	// 格式2： 输出日期 "2012 / 09 / 16"
	AP_FormatDataTime (&dateTimeSettingData->dateTime, APP_DATETIME_FORMAT_2, text, sizeof(text));
	AP_TextGetStringSize (text, strlen(text), &size);
	x = (XMCOORD)( (rc.left + (rc.right - rc.left + 1 - 2*size.cx) / 2));
	y = (XMCOORD)(scale_factor * (APP_POS_ITEM5_MENUNAME_Y + APP_POS_ITEM5_LINEHEIGHT_DATESETTING * 1));
    //y-=50;
	// 格式4： 输出时分秒 "10 : 22 : 20"
	AP_FormatDataTime (&dateTimeSettingData->dateTime, APP_DATETIME_FORMAT_4, text2, sizeof(text2));
	AP_TextGetStringSize (text2, strlen(text2), &size);
	x2 = x+size.cx+80;//(XMCOORD)( (rc.left + (rc.right - rc.left + 1 - size.cx) / 2));
	y2 = y;//(XMCOORD)(scale_factor * (APP_POS_ITEM5_MENUNAME_Y + APP_POS_ITEM5_LINEHEIGHT_DATESETTING * 3));
    
	// 格式2： 输出日期 "2012 / 09 / 16"
	AP_TextOutDataTimeString (XMHWND_HANDLE(DateTimeSetting), x, y, text, strlen(text));
	// 格式4： 输出时分秒 "10 : 22 : 20"
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

	// 处理按钮控件的显示。
	// 若存在按钮控件，必须调用AP_ButtonControlMessageHandler执行按钮控件显示
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
		case 0:	// 年 (2012 ~ 2049)
			v = dateTime->wYear + diff;
			if(v < 2012)
				v = 2049;
			else if(v > 2049)
				v = 2012;
			dateTime->wYear = (WORD)v;
			
			// 调整2月
			if( 	dateTime->wYear % 4 == 0  && dateTime->wYear % 100 != 0   
				|| dateTime->wYear % 400 == 0 )
			{
				_month[1] = 29;
			}
			// 检查日期
			if(dateTime->bDay > (BYTE)_month[dateTime->bMonth - 1])
				dateTime->bDay = (BYTE)_month[dateTime->bMonth - 1];
						
			break;

		case 1:	// 月	(1 ~ 12)
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
			
			// 检查日期
			if(dateTime->bDay > (BYTE)_month[dateTime->bMonth - 1])
				dateTime->bDay = (BYTE)_month[dateTime->bMonth - 1];
			
			break;
		}

		case 2:	// 日 (1 ~ 31)
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

		case 3:	// 时 (0 ~ 23)
			v = dateTime->bHour + diff;
			if(v < 0)
				v = 23;
			else if(v > 23)
				v = 0;
			dateTime->bHour = (BYTE)v;
			break;

		case 4:	// 分 (0 ~ 59)
			v = dateTime->bMinute + diff;
			if(v < 0)
				v = 59;
			else if(v > 59)
				v = 0;
			dateTime->bMinute = (BYTE)v;
			break;

		case 5:	// 秒 (0 ~ 59)
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
	
	// 按键音
	XM_Beep (XM_BEEP_KEYBOARD);
	
	switch(msg->wp)
	{
		case VK_AP_MENU:		// 菜单键
		case VK_AP_MODE:
		case VK_AP_SWITCH:
			// 在"时间设置"窗口中，MENU及MODE键被定义为按钮操作。
			// 此处将这两个键事件交给按钮控件进行缺省处理，完成必要的显示效果处理
			AP_ButtonControlMessageHandler (&dateTimeSettingData->btnControl, msg);
			XM_SetTimer (XMTIMER_DATETIMESETTING, OSD_AUTO_HIDE_TIMEOUT);
			break;
			
        #if 1
		case VK_AP_UP:		
			AP_ButtonControlMessageHandler (&dateTimeSettingData->btnControl, msg);
			AdjustDateTime (&dateTimeSettingData->dateTime, dateTimeSettingData->nCurItem, 1);
			// 刷新
			XM_InvalidateWindow ();
			XM_UpdateWindow ();
			break;

		case VK_AP_DOWN:
			AP_ButtonControlMessageHandler (&dateTimeSettingData->btnControl, msg);
			AdjustDateTime (&dateTimeSettingData->dateTime, dateTimeSettingData->nCurItem, -1);
			// 刷新
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
		case VK_AP_MENU:		// 菜单键
		case VK_AP_MODE:		// 切换到“行车记录”状态
		case VK_AP_SWITCH:
			// 在"时间设置"窗口中，MENU及MODE键被定义为按钮操作。
			// 此处将这两个键事件交给按钮控件进行缺省处理，完成必要的显示效果处理
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
		case DATETIMESETTING_COMMAND_NEXT:		// 菜单键
			dateTimeSettingData->nCurItem ++;
			if(dateTimeSettingData->nCurItem > 5)
				dateTimeSettingData->nCurItem = 0;
			// 刷新
			XM_InvalidateWindow ();
			XM_UpdateWindow ();
			break;

		case DATETIMESETTING_COMMAND_OK:
			// 以下为"时间设置"窗口的私有行为过程
			// 返回到桌面
			// 将当前时间设置为系统时间
			XM_SetLocalTime (&dateTimeSettingData->dateTime);
			XM_PullWindow (0);
			break;

		case DATETIMESETTING_COMMAND_CANCEL:
			// 返回到桌面
			XM_PullWindow (0);
			break;
			
		case DATETIMESETTING_COMMAND_YEAR_UP:	
			dateTimeSettingData->nCurItem=0;	
			AP_ButtonControlMessageHandler (&dateTimeSettingData->btnControl, msg);
			AdjustDateTime (&dateTimeSettingData->dateTime, dateTimeSettingData->nCurItem, 1);
			// 刷新
			XM_InvalidateWindow ();
			XM_UpdateWindow ();
			XM_SetTimer (XMTIMER_DATETIMESETTING, OSD_AUTO_HIDE_TIMEOUT);
			break;

	    case DATETIMESETTING_COMMAND_YEAR_DOWN:
			dateTimeSettingData->nCurItem=0;	  	
			AP_ButtonControlMessageHandler (&dateTimeSettingData->btnControl, msg);
			AdjustDateTime (&dateTimeSettingData->dateTime, dateTimeSettingData->nCurItem, -1);
			// 刷新
			XM_InvalidateWindow ();
			XM_UpdateWindow ();
			XM_SetTimer (XMTIMER_DATETIMESETTING, OSD_AUTO_HIDE_TIMEOUT);
			break;
			
		case DATETIMESETTING_COMMAND_MONTH_UP:	
			dateTimeSettingData->nCurItem=1;	
			AP_ButtonControlMessageHandler (&dateTimeSettingData->btnControl, msg);
			AdjustDateTime (&dateTimeSettingData->dateTime, dateTimeSettingData->nCurItem, 1);
			// 刷新
			XM_InvalidateWindow ();
			XM_UpdateWindow ();
			XM_SetTimer (XMTIMER_DATETIMESETTING, OSD_AUTO_HIDE_TIMEOUT);
			break;

        case DATETIMESETTING_COMMAND_MONTH_DOWN:
			dateTimeSettingData->nCurItem=1;	  	
			AP_ButtonControlMessageHandler (&dateTimeSettingData->btnControl, msg);
			AdjustDateTime (&dateTimeSettingData->dateTime, dateTimeSettingData->nCurItem, -1);
			// 刷新
			XM_InvalidateWindow ();
			XM_UpdateWindow ();
			XM_SetTimer (XMTIMER_DATETIMESETTING, OSD_AUTO_HIDE_TIMEOUT);
			break;
		
		case DATETIMESETTING_COMMAND_DAY_UP:	
			dateTimeSettingData->nCurItem=2;	
			AP_ButtonControlMessageHandler (&dateTimeSettingData->btnControl, msg);
			AdjustDateTime (&dateTimeSettingData->dateTime, dateTimeSettingData->nCurItem, 1);
			// 刷新
			XM_InvalidateWindow ();
			XM_UpdateWindow ();
			XM_SetTimer (XMTIMER_DATETIMESETTING, OSD_AUTO_HIDE_TIMEOUT);
			break;

        case DATETIMESETTING_COMMAND_DAY_DOWN:
			dateTimeSettingData->nCurItem=2;	  	
			AP_ButtonControlMessageHandler (&dateTimeSettingData->btnControl, msg);
			AdjustDateTime (&dateTimeSettingData->dateTime, dateTimeSettingData->nCurItem, -1);
			// 刷新
			XM_InvalidateWindow ();
			XM_UpdateWindow ();
			XM_SetTimer (XMTIMER_DATETIMESETTING, OSD_AUTO_HIDE_TIMEOUT);
			break;
		
		case DATETIMESETTING_COMMAND_HOUR_UP:	
			dateTimeSettingData->nCurItem=3;	
			AP_ButtonControlMessageHandler (&dateTimeSettingData->btnControl, msg);
			AdjustDateTime (&dateTimeSettingData->dateTime, dateTimeSettingData->nCurItem, 1);
			// 刷新
			XM_InvalidateWindow ();
			XM_UpdateWindow ();
			XM_SetTimer (XMTIMER_DATETIMESETTING, OSD_AUTO_HIDE_TIMEOUT);
			break;

        case DATETIMESETTING_COMMAND_HOUR_DOWN:
			dateTimeSettingData->nCurItem=3;	  	
			AP_ButtonControlMessageHandler (&dateTimeSettingData->btnControl, msg);
			AdjustDateTime (&dateTimeSettingData->dateTime, dateTimeSettingData->nCurItem, -1);
			// 刷新
			XM_InvalidateWindow ();
			XM_UpdateWindow ();
			XM_SetTimer (XMTIMER_DATETIMESETTING, OSD_AUTO_HIDE_TIMEOUT);
			break;
		
		case DATETIMESETTING_COMMAND_MINUTE_UP:	
			dateTimeSettingData->nCurItem=4;	
			AP_ButtonControlMessageHandler (&dateTimeSettingData->btnControl, msg);
			AdjustDateTime (&dateTimeSettingData->dateTime, dateTimeSettingData->nCurItem, 1);
			// 刷新
			XM_InvalidateWindow ();
			XM_UpdateWindow ();
			XM_SetTimer (XMTIMER_DATETIMESETTING, OSD_AUTO_HIDE_TIMEOUT);
			break;

        case DATETIMESETTING_COMMAND_MINUTE_DOWN:
			dateTimeSettingData->nCurItem=4;	  	
			AP_ButtonControlMessageHandler (&dateTimeSettingData->btnControl, msg);
			AdjustDateTime (&dateTimeSettingData->dateTime, dateTimeSettingData->nCurItem, -1);
			// 刷新
			XM_InvalidateWindow ();
			XM_UpdateWindow ();
			XM_SetTimer (XMTIMER_DATETIMESETTING, OSD_AUTO_HIDE_TIMEOUT);
			break;
		
		case DATETIMESETTING_COMMAND_SECOND_UP:	
			dateTimeSettingData->nCurItem=5;	
			AP_ButtonControlMessageHandler (&dateTimeSettingData->btnControl, msg);
			AdjustDateTime (&dateTimeSettingData->dateTime, dateTimeSettingData->nCurItem, 1);
			// 刷新
			XM_InvalidateWindow ();
			XM_UpdateWindow ();
			XM_SetTimer (XMTIMER_DATETIMESETTING, OSD_AUTO_HIDE_TIMEOUT);
			break;

        case DATETIMESETTING_COMMAND_SECOND_DOWN:
			dateTimeSettingData->nCurItem=5;	  	
			AP_ButtonControlMessageHandler (&dateTimeSettingData->btnControl, msg);
			AdjustDateTime (&dateTimeSettingData->dateTime, dateTimeSettingData->nCurItem, -1);
			// 刷新
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

	// 返回到桌面
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
// 消息处理函数定义
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

// 桌面视窗定义
#ifdef __ICCARM__
#pragma data_alignment=32
#endif
XMHWND_DEFINE(0, 0, LCD_XDOTS, LCD_YDOTS, DateTimeSetting, 0, 0, 0, XM_VIEW_DEFAULT_ALPHA, HWND_VIEW)

