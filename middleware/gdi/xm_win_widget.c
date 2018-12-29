//****************************************************************************
//
//	Copyright (C) 2012 ShenZhen ExceedSpace
//
//	Author	ZhuoYongHong
//
//	File name: xm_win_widget.c
//	  constant£¬macro & basic typedef definition of widget
//
//	Revision history
//
//		2010.09.10	ZhuoYongHong Initial version
//
//****************************************************************************
#include <xm_user.h>
#include <xm_widget.h>

VOID XM_WidgetProc (const XMWDGT *pWidget, BYTE bWidgetFlag, VOID *pUserData, XMMSG *msg)
{
	BYTE bWidgetType;

	if(pWidget == NULL)
		return;
	
	bWidgetType = pWidget->bType;

	switch (bWidgetType)
	{
#if defined(_XM_WIDGET_SUPPORT_)
		case XM_WIDGET_BUTTON:	
			XMWDGTPROC(BUTTON) (pWidget, bWidgetFlag, pUserData, msg);
			break;
		case XM_WIDGET_TITLE:	
			XMWDGTPROC(TITLE) (pWidget, bWidgetFlag, pUserData, msg);
			break;
		case XM_WIDGET_STATIC:	
			XMWDGTPROC(STATIC) (pWidget, bWidgetFlag, pUserData, msg);
			break;
		case XM_WIDGET_SLIDER:	
			XMWDGTPROC(SLIDER) (pWidget, bWidgetFlag, pUserData, msg);
			break;
		case XM_WIDGET_LIST:	
			XMWDGTPROC(LIST) (pWidget, bWidgetFlag, pUserData, msg);
			break;
		case XM_WIDGET_RADIO:	
			XMWDGTPROC(RADIO) (pWidget, bWidgetFlag, pUserData, msg);
			break;
		case XM_WIDGET_CHECKBOX:	
			XMWDGTPROC(CHECKBOX) (pWidget, bWidgetFlag, pUserData, msg);
			break;
		case XM_WIDGET_SCROLLBAR:	
			XMWDGTPROC(SCROLLBAR) (pWidget, bWidgetFlag, pUserData, msg);
			break;
#endif	// _XM_WIDGET_SUPPORT_

		default:
			break;
	}
}



void winwidget_init (void)
{
}