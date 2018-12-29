//****************************************************************************
//
//	Copyright (C) 2012 ZhuoYongHong
//
//	Author	ZhuoYongHong
//
//	File name: app_FormatSetting.c
//	  ͨ��Menuѡ���б�ѡ�񴰿�(ֻ��ѡ��һ��ѡ��)
//
//	Revision history
//
//		2012.09.16	ZhuoYongHong Initial version
//
//****************************************************************************
#include "app.h"
#include "app_menuid.h"
#include "app_menuoptionview.h"
#include "types.h"

extern BOOL Guide_Camera_Show();
extern BOOL ACC_Select_BiaoCHi_Show;


typedef struct tagBIAOCHIVIEWDATA {
	u8_t				nTopItem;					// ��һ�����ӵĲ˵���
	u8_t				nCurItem;					// ��ǰѡ��Ĳ˵���
	u8_t				nItemCount;					// �˵������
} BIAOCHIVIEWDATA;


VOID BiaoChiViewOnEnter (XMMSG *msg)
{
	XM_printf(">>>>BiaoChiViewOnEnter......\r\n");
    //XM_DisableViewAnimate (XMHWND_HANDLE(CarBackLineView));
	if(msg->wp == 0)
	{
	    BIAOCHIVIEWDATA *BiaoChiViewData = XM_calloc(sizeof(BIAOCHIVIEWDATA));
        APPMENUOPTION *lpBiaoChiView = (APPMENUOPTION *)msg->lp;
		
		if(BiaoChiViewData == NULL)
		{
			XM_printf ("menuOptionViewData XM_calloc failed\n");
			// ʧ�ܷ��ص�����
			XM_PullWindow (0);
			return;
		}
		XM_SetWindowPrivateData (XMHWND_HANDLE(CarBackLineView), BiaoChiViewData);
	}
	else
	{
		// �����ѽ�������ǰ���ڴ�ջ�лָ�
		XM_printf ("FormatSettingView Pull\n");
	}
    //������ʱ�� 500ms ���ڹرս���
    XM_SetTimer (XMTIMER_SETTINGVIEW, 200);
}

VOID BiaoChiViewOnLeave (XMMSG *msg)
{
	XM_printf(">>>>BiaoChiViewOnLeave......\r\n");

    //XM_EnableViewAnimate (XMHWND_HANDLE(CarBackLineView));
	if (msg->wp == 0)
	{
		BIAOCHIVIEWDATA *BiaoChiViewData = (BIAOCHIVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(CarBackLineView));
		if(BiaoChiViewData)
		{
			// �ͷ�˽�����ݾ��
			XM_free (BiaoChiViewData);
			
		}
		XM_printf ("FormatSettingView Exit\n");
	}
	else
	{
		// ��ת���������ڣ���ǰ���ڱ�ѹ��ջ�С�
		// �ѷ������Դ����
		XM_printf ("FormatSettingView Push\n");
	}
}


VOID BiaoChiViewOnPaint (XMMSG *msg)
{
	XMRECT rc,rect;
	unsigned int old_alpha;

	XM_printf(">>>>>>>>BiaoChiViewOnPaint, msg->wp:%x, msg->lp:%x\r\n", msg->wp, msg->lp);
	HANDLE hwnd = XMHWND_HANDLE(CarBackLineView);
	BIAOCHIVIEWDATA *BiaoChiViewData = (BIAOCHIVIEWDATA *)XM_GetWindowPrivateData (hwnd);
	if(BiaoChiViewData == NULL)
		return;

	// ��ʾ������
	XM_GetDesktopRect (&rc); 
	XM_FillRect (hwnd, rc.left, rc.top, rc.right, rc.bottom, XM_GetSysColor(XM_COLOR_DESKTOP));

	old_alpha = XM_GetWindowAlpha (hwnd);
	XM_SetWindowAlpha (hwnd, 255);
    
    rect = rc;
	rect.left = 102;
	rect.top = 230;
	AP_RomImageDrawByMenuID(AP_ID_BUTTON_BACKBACK, hwnd, &rect, XMGIF_DRAW_POS_LEFTTOP);
	
	XM_SetWindowAlpha (hwnd, old_alpha);
}

VOID BiaoChiViewOnKeyDown(XMMSG *msg)
{
	BIAOCHIVIEWDATA *BiaoChiViewData = (BIAOCHIVIEWDATA *)XM_GetWindowPrivateData (XMHWND_HANDLE(CarBackLineView));
	if(BiaoChiViewData == NULL)
		return;

	XM_printf(">>>>>>>>BiaoChiViewOnKeyDown, msg->wp:%x, msg->lp:%x\r\n", msg->wp, msg->lp);
}


VOID BiaoChiViewOnKeyUp(XMMSG *msg)
{
	XM_printf(">>>>>>>>BiaoChiViewOnKeyUp, msg->wp:%x, msg->lp:%x\r\n", msg->wp, msg->lp);
}

VOID BiaoChiViewOnTimer (XMMSG *msg)
{
    // AV �л���Ҫ�˳�
    #if 0
    if((AHD_Status == 0)|| (Manual_Channel == 0)) {
        if(!ACC_Select_BiaoCHi_Show) {
            XM_PullWindow (0);
        }else {
            if(!AHD_Guide_Start()) {
                XM_PullWindow (0);
                ACC_Select_BiaoCHi_Show = FALSE;
            }
        }
    }
    #endif
    if(!Guide_Camera_Show()) {
        XM_PullWindow (0);
        ACC_Select_BiaoCHi_Show = FALSE;
    }
    deskTopStartRecord();
}
// ��Ϣ����������
XM_MESSAGE_MAP_BEGIN (CarBackLineView)
	XM_ON_MESSAGE (XM_PAINT, BiaoChiViewOnPaint)
	XM_ON_MESSAGE (XM_KEYDOWN, BiaoChiViewOnKeyDown)
	XM_ON_MESSAGE (XM_KEYUP, BiaoChiViewOnKeyUp)
	XM_ON_MESSAGE (XM_ENTER, BiaoChiViewOnEnter)
	XM_ON_MESSAGE (XM_LEAVE, BiaoChiViewOnLeave)
	XM_ON_MESSAGE (XM_TIMER, BiaoChiViewOnTimer)
XM_MESSAGE_MAP_END

// �����Ӵ�����
#ifdef __ICCARM__
#pragma data_alignment=32
#endif
XMHWND_DEFINE(0, 0, LCD_XDOTS, LCD_YDOTS, CarBackLineView, 0, 0, 0, 0, HWND_VIEW)
