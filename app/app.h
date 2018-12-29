//****************************************************************************
//
//	Copyright (C) 2010 ShenZhen ExceedSpace
//
//	Author	ZhuoYongHong
//
//	File name: app.h
//	  D3�Ӵ�����
//
//	Revision history
//
//		2010.09.06	ZhuoYongHong Initial version
//
//****************************************************************************

#ifndef _XSPACE_APP_H_
#define _XSPACE_APP_H_

#include <xm_proj_define.h>
#include <xm_user.h>
#include <xm_base.h>
#include <xm_gdi.h>
#include <xm_key.h>
#include <common_wstring.h>
#include <xm_malloc.h>
#include <xm_file.h>
#include <xm_rom.h>
#include <xm_assert.h>
#include <xm_dev.h>
#include <xm_printf.h>
#include <xm_image.h>
#include <xm_config.h>
#include <rom.h>
#include <xm_voice_prompts.h>
#include "xm_app_menudata.h"
#include <xm_canvas.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "xm_recycle.h"
#include <xm_videoitem.h>
#include <common_string.h>

#if defined (__cplusplus)
	extern "C"{
#endif


#define	_XM_SYS_PROTECT_VIDEO_LIST_ENABLE_


// �����ⲿ��������

// ����(������)��ͼ
XMHWND_DECLARE(Desktop)

// ������ͼ
XMHWND_DECLARE(MainView)
XMHWND_DECLARE(Photo)
XMHWND_DECLARE(SettingView)
XMHWND_DECLARE(VideoSettingView)
XMHWND_DECLARE(SystemSettingView)
XMHWND_DECLARE(DateTimeSetting)
XMHWND_DECLARE(LangOptionSetting)
XMHWND_DECLARE(WaterMarkSettingView)
// ��־ˮӡ(�û��Զ���ˮӡ)������ͼ
XMHWND_DECLARE(WaterMarkLogoLoadView)
XMHWND_DECLARE(SystemVersionView)
//����
XMHWND_DECLARE(VolView)

//��ʾ��������
//XMHWND_DECLARE(MovieRec)

//������ʶͼ�����
XMHWND_DECLARE(CarBackLineView)
//��ɫ���ý���
XMHWND_DECLARE(ColorView)
//����
XMHWND_DECLARE(FunctionView)
//������ʱ
XMHWND_DECLARE(GuideView)
//�������
XMHWND_DECLARE(CameraView)
//����ģʽ
XMHWND_DECLARE(FactoryView)
// �˵�ѡ����ͼ
XMHWND_DECLARE(MenuOptionView)

// ��Ƶ��ͼ
XMHWND_DECLARE(VideoListView)
XMHWND_DECLARE(VideoBackListView)
XMHWND_DECLARE(VideoView)
XMHWND_DECLARE(VideoBackView)

// ����б���ͼ
XMHWND_DECLARE(AlbumListView)
XMHWND_DECLARE(AlbumView)

// ����վ�Ӵ�
XMHWND_DECLARE(RecycleView)


// ��Ϣ��ʾ��ͼ
XMHWND_DECLARE(MessageView)

// ϵͳ�Լ���ͼ
XMHWND_DECLARE(CardView)


XMHWND_DECLARE(InitSettingView)

XMHWND_DECLARE(VersionView)

// ϵͳ����
XMHWND_DECLARE(ProcessView)

// ALERT����Ӵ�
XMHWND_DECLARE(AlertView)
XMHWND_DECLARE(SD_ProcessView)

// OK/CACNELģʽ�Ի���
XMHWND_DECLARE(OkCancelView)

// ѭ��¼�񱨾��Ӵ�
XMHWND_DECLARE(RecycleVideoAlertView)


XMHWND_DECLARE(SoundVolumeView)

XMHWND_DECLARE(SwitchButtonView)

//�طŲ˵�
XMHWND_DECLARE(PlayBackMenu)
XMHWND_DECLARE(PhotoMenu)

// ֧��1, 2, 3, 4��Button��ʽ
#define	XM_MAX_BUTTON_COUNT		5// 4
#define	TP_MAX_BUTTON_COUNT		12

typedef struct _tagXMBUTTONINFO {
	WORD				wKey;					// ��Ӧ�İ���ֵ
	WORD				wCommand;				// ��ť����ʱ���͵�����
	DWORD				dwLogoId;				// ��ť��Logo��ԴID
	DWORD				dwTextId;				// ��ť���ı�(Text)��ԴID
} XMBUTTONINFO;

typedef struct _tagTPBUTTONINFO {
	   WORD left;		// �������Ͻ�X����
	WORD top;		// �������Ͻ�Y����
	WORD right;		// �������½�X����
      WORD bottom;	// �������½�Y����
	WORD				wKey;					// ��Ӧ�İ���ֵ
	WORD				wCommand;				// ��ť����ʱ���͵�����
	DWORD				dwLogoId;				// ��ť��Logo��ԴID
	DWORD				ClickdwLogoId;// ѡ�а�ť��Logo��ԴID //dwTextId;	
} TPBUTTONINFO;
#define	XMBUTTON_FLAG_HIDE_BACKGROUND		0x00000001		// ���ذ�ť���ı���Ԫ�ص���ʾ
#define	XMBUTTON_FLAG_HIDE_HORZ_SPLIT		0x00000002		// ���ذ�ť���Ķ���ˮƽ�ָ��ߵ���ʾ
#define	XMBUTTON_FLAG_HIDE_VERT_SPLIT		0x00000004		// ���ذ�ť���İ�ť֮��Ĵ�ֱ�ָ��ߵ���ʾ

#define	XMBUTTON_FLAG_HIDE					0x0000FFFF		// ���ذ�ť������ʾ

// ��ť�ؼ�
typedef struct _tagXMBUTTONCONTROL {
	XMBUTTONINFO 			pBtnInfo[XM_MAX_BUTTON_COUNT];				// ��ť��Ϣ
	BYTE						bBtnCount;				// ��ť����
	BYTE						bBtnClick;				// 0xFF��ʾû��Button����
														// ����ֵ��ʾ��ǰ����Button�����
	BYTE						bBtnInited;				// �ؼ�˽�����ݣ���ʾ�Ƿ��ʼ������Ϣ
	BYTE						bBtnEnable;				// �ؼ�ʹ�ܻ��ֹ��־	
	HANDLE						hWnd;					// ��ť�ؼ������Ĵ��ھ��
	DWORD						dwBtnBackgroundId;		// ����ɫ��ԴID
	DWORD						dwBtnFlag;				// ��ť������Ϣ

#if 0
	// Ԥ����ͼ����Դ
	XM_IMAGE *				pImageMenuItemSplitLine;	// ��ť�������ָ���
	XM_IMAGE *				pImageButtonBackground;		// ��ť��������Դ
	XM_IMAGE *				pImageButtonSplitLine;		// ��ť���ָ�����

	// ֧��1, 2, 3, 4��Button��ʽ
	XM_IMAGE *				pImageLogo[XM_MAX_BUTTON_COUNT];
	XM_IMAGE *				pImageText[XM_MAX_BUTTON_COUNT];				
#endif
} XMBUTTONCONTROL;
// ��ť�ؼ�
typedef struct _tagTPBUTTONCONTROL {
	TPBUTTONINFO 			TpBtnInfo[TP_MAX_BUTTON_COUNT];				// ��ť��Ϣ
	BYTE						bBtnCount;				// ��ť����
	BYTE						bBtnClick;				// 0xFF��ʾû��Button����
															// ����ֵ��ʾ��ǰ����Button�����
	BYTE						bBtnInited;				// �ؼ�˽�����ݣ���ʾ�Ƿ��ʼ������Ϣ
	BYTE						bBtnEnable;				// �ؼ�ʹ�ܻ��ֹ��־	
	HANDLE					hWnd;						// ��ť�ؼ������Ĵ��ھ��
	DWORD						dwBtnBackgroundId;	// ����ɫ��ԴID
	DWORD						dwBtnFlag;				// ��ť������Ϣ

#if 0
	// Ԥ����ͼ����Դ
	XM_IMAGE *				pImageMenuItemSplitLine;	// ��ť�������ָ���
	XM_IMAGE *				pImageButtonBackground;		// ��ť��������Դ
	XM_IMAGE *				pImageButtonSplitLine;		// ��ť���ָ�����

	// ֧��1, 2, 3, 4��Button��ʽ
	XM_IMAGE *				pImageLogo[XM_MAX_BUTTON_COUNT];
	XM_IMAGE *				pImageText[XM_MAX_BUTTON_COUNT];				
#endif
} TPBUTTONCONTROL;
XMBOOL AP_ButtonControlInit (XMBUTTONCONTROL *pButtonControl, BYTE bBtnCount, HANDLE hWnd, const XMBUTTONINFO* pButtonInfo);
XMBOOL AP_ButtonControlExit (XMBUTTONCONTROL *pButtonControl);
XMBOOL AP_TpButtonControlInit (TPBUTTONCONTROL *TpButtonControl, BYTE bBtnCount, HANDLE hWnd, const TPBUTTONINFO* pButtonInfo);
XMBOOL AP_TpButtonControlExit (TPBUTTONCONTROL *TpButtonControl);
// ʹ�ܻ��ֹ��ť
XMBOOL AP_ButtonControlSetEnable (XMBUTTONCONTROL *pButtonControl, XMBOOL bEnable);

// ��ť��Ϣ����
// ����ֵ
// TRUE		��Ϣ�ѱ���ť�ؼ�����, App�����������
// FALSE		��Ϣδ����ť�ؼ�����, App��Ҫ��������
XMBOOL AP_VideoplayingButtonControlMessageHandler (XMBUTTONCONTROL *pButtonControl, XMMSG *msg);
XMBOOL AP_ButtonControlMessageHandler (XMBUTTONCONTROL *pButtonControl, XMMSG *msg);
XMBOOL AP_TpButtonControlMessageHandler (TPBUTTONCONTROL *TpButtonControl, XMMSG *msg);
XMBOOL AP_TpButtonControlMessageHandlerDateSet (TPBUTTONCONTROL *TpButtonControl, XMMSG *msg);
// ���ð�ť�ı�־״̬, �ı䰴ť����ʾ
VOID  AP_ButtonControlSetFlag (XMBUTTONCONTROL *pButtonControl, DWORD Flag);
DWORD AP_ButtonControlGetFlag (XMBUTTONCONTROL *pButtonControl);

// �޸ĵ�����ť������
// pButtonControl		��ť�ؼ�
// bBtnIndex			��Ҫ�޸ĵİ�ť��������� (��Ŵ�0��ʼ)
// pButtonInfo			�µİ�ť��������Ϣ
XMBOOL AP_ButtonControlModify (XMBUTTONCONTROL *pButtonControl, BYTE bBtnIndex, const XMBUTTONINFO* pButtonInfo);


// ����ؼ�
typedef struct _tagXMTITLEBARCONTROL {
	DWORD						dwIconID;		// ͼ����Դ
	DWORD						dwTextID;		// �����ı���Դ

	HANDLE					hWnd;				// ��ť�ؼ������Ĵ��ھ��

#if 0
	XM_IMAGE *				pImageMenuTitleBarBackground;		// ���ⱳ��λͼ��Դ
	XM_IMAGE *				pImageMenuItemSplitLine;			// �ָ���
	XM_IMAGE *				pImageIcon;								// ͼ��
	XM_IMAGE *				pImageText;								// �����ı�
#endif
} XMTITLEBARCONTROL;

XMBOOL AP_TitleBarControlInit (XMTITLEBARCONTROL *pTitleBarControl, HANDLE hWnd, DWORD dwIconID, DWORD dwTextID);
XMBOOL AP_TitleBarControlExit (XMTITLEBARCONTROL *pTitleBarControl);
VOID AP_TitleBarControlMessageHandler (XMTITLEBARCONTROL *pTitleBarControl, XMMSG *msg);

// �����л��ؼ�
// ��ʼ��һ�������л��ؼ�
HANDLE AP_SwitchButtonControlInit (HANDLE hWnd,					// �����л��ؼ������Ĵ��ھ��
									 XMPOINT *lpButtonPoint,		// �����л��ؼ���ʾ��λ��
									 XMBOOL bInitialState,		// �����л��ؼ��ĳ�ʼ״̬��0 �ر�(��ֹ) 1 ����(ʹ��)
									 VOID (*lpSwitchStateCallback) (VOID *lpPrivateData, unsigned int state),
																		// ״̬�л�ʱ���õĻص�����
									 VOID *lpPrivateData);

// �رտ����л��ؼ�
XMBOOL AP_SwitchButtonControlExit (HANDLE hSwitchControl);

unsigned int AP_SwitchButtonControlGetState (HANDLE hSwitchControl);

XMBOOL AP_SwitchButtonControlSetState (HANDLE hSwitchControl, XMBOOL bState);

// �ƶ������л��ؼ�
VOID AP_SwitchButtonControlMove (HANDLE hSwitchControl, XMCOORD x, XMCOORD y);			

// �����л��ؼ���Ϣ����
VOID AP_SwitchButtonControlMessageHandler (HANDLE hSwitchControl, XMMSG *msg);
						

// ���ڱ�����ʾ������Դδ���ڣ���ָ��ΪAP_NULLID "(DWORD)(-1)"
VOID AP_DrawTitlebarControl (HANDLE hWnd, DWORD dwIconId, DWORD dwTextID);



// ������Ϊ���ݽṹ����
typedef struct _tagXMWNDCONTROL {
	int				nTopItem;					// ��һ�����ӵĲ˵���
	int				nCurItem;					// ��ǰѡ��Ĳ˵���
	int				nItemCount;					// �˵������

	int				nClickedButton;				// -1��ʾû��Button����
														// ����ֵ��ʾ��ǰ����Button�����
	
} XMWNDCONTROL;

// ��ʱ������
#define	XMTIMER_DESKTOPVIEW					1		// ����������(����)�µĶ�ʱ��

#define	XMTIMER_SETTINGVIEW					1		// ���ý����µĶ�ʱ��
#define	XMTIMER_DATETIMESETTING				1		// ʱ�����ý����µĶ�ʱ���������ý��湲��ͬһ��ID	
#define XMTIMER_VIDEOSETTING             	1       // ¼�����ý����µĶ�ʱ���������ý��湲��ͬһ��ID	
#define XMTIMER_SYSTEMSETTING            	1       // ϵͳ���ý����µĶ�ʱ���������ý��湲��ͬһ��ID	

#define	XMTIMER_VIDEOLISTVIEW				1		// ��Ƶ�б���ͼ�Ķ�ʱ��		
#define	XMTIMER_VIDEOVIEW					1		// ��Ƶ������ͼ�Ķ�ʱ��
#define XMTIMER_OPTIONVIEW					1       //ѡ����ͼ�Ķ�ʱ��
#define XMTIMER_VERSIONVIEW              	1       //�汾��ͼ�Ķ�ʱ��
#define XMTIMER_PROCESSVIEW              	1      
#define	XMTIMER_MESSAGEVIEW					2		// ��Ϣ��ʾ��ͼ�Ķ�ʱ��	
#define	XMTIMER_GIFANIMATING				3		// GIF�������Ŷ�ʱ��
#define	XMTIMER_ALBUMLISTVIEW				1		// ����б���ͼ�Ķ�ʱ��		
#define XMTIMER_VERSION						1

// *********** ���ڲ˵�λ�ú궨�� *************************
#define	APP_TITLEBAR_HEIGHT				42		// ���ڱ���ؼ����߶�
#define	APP_BUTTON_HEIGHT					60		// ���ڰ�ť�ؼ����߶�


// *********** ��������λ�ö���� **************
// ��������λ�ö���(��ʼλ��)
#define	APP_POS_TITLEBAR_X					0
#define	APP_POS_TITLEBAR_Y					0
// ������ͼ��λ��
#define	APP_POS_TITLEBAR_ICON_X				2
#define	APP_POS_TITLEBAR_ICON_Y				6
// ����������λ��
#define	APP_POS_TITLEBAR_TEXT_X				22                                               
#define	APP_POS_TITLEBAR_TEXT_Y				4

// *********** �˵�����λ�ö���� **************
#define	APP_POS_MENU_X							0
#define	APP_POS_MENU_Y							42//48

// �˵���Ϊ5����ˮƽ�ָ��߿�ʼλ��
#define	APP_POS_ITEM5_SPLITLINE_X			0
#define	APP_POS_ITEM5_SPLITLINE_Y			42//30		
#define	APP_POS_ITEM5_LINEHEIGHT				63
#define	APP_POS_ITEM5_LINEHEIGHT_DATESETTING		30

#define	APP_POS_ITEM6_LINEHEIGHT			25

// �˵���Ŀ��λ�ö���(��һ��Ŀ�ʼλ��)
// �˵�����λ��
#define	APP_POS_ITEM5_MENUNAME_X			22
#define	APP_POS_ITEM5_MENUNAME_Y			55//45//38 ���ָ��ֽ��߾���
// �˵�ֵ��λ��
#define	APP_POS_ITEM5_MENUDATA_X			166
#define	APP_POS_ITEM5_MENUDATA_Y			38
// �˵�����λ��
#define	APP_POS_ITEM5_MENUFLAG_X			300


// ý���ļ��б�λ�ö���
#define	APP_POS_MEDIA_TYPE_X					8					// ���ͱ�־
#define	APP_POS_MEDIA_NAME_X					(8+24+16)		// ý������λ��
#define	APP_POS_MEDIA_CHANNEL_X				(320 - 8 - 24)	// ý��ͨ��λ��


// *********** ��ť��λ�ö���� **************
// Button����λ��
#define	APP_POS_BUTTON_X						0
#define	APP_POS_BUTTON_Y						180

// Button���ָ��ߵ�λ��
#define	APP_POS_BUTTON_SPLITLINE_Y			192

#define	APP_POS_BUTTON_LOGO_Y				194

#define	APP_POS_BUTTON_TEXT_Y				220


// ����ͼ��λ��
#define	APP_POS_KEY_X							280


// ��Ƶ������Toolbar��ť����
#define	APP_POS_VIDEOPLAY_TOOLBAR_X		0
#define	APP_POS_VIDEOPLAY_TOOLBAR_Y		200
#define	APP_POS_VIDEOPLAY_TIME_X			74
#define	APP_POS_VIDEOPLAY_TIME_y			204
#define	APP_POS_VIDEOPLAY_PREV_BTN_X		100
#define	APP_POS_VIDEOPLAY_PLAY_BTN_X		160
#define	APP_POS_VIDEOPLAY_NEXT_BTN_X		220

#define	APP_POS_VIDEOPLAY_BTN_Y				229


// �����á��˵��Ĳ˵������
#define	APP_DISPLAY_ITEM_COUNT				5

#define	APP_MENUOPTIONVIEW_ITEM_COUNT			5

#define  APP_WATERMARKSETTING_ITEM_COUNT		4	// ˮӡ���ò˵�����Ŀ����

#define	APP_VIDEOLIST_ITEM_COUNT			5
#define	APP_ALBUMLIST_ITEM_COUNT			5

#define	APP_RECYCLELIST_ITEM_COUNT			5


// 
#define	APP_FORECHANNEL_STREAMBITRATE		0x100000		// ǰ��ͨ��ƽ������(ÿ��)
#define	APP_BACKCHANNEL_STREAMBITRATE		0xC0000		// ����ͨ��ƽ������(ÿ��)

#define	APP_STREAMBITRATE						(APP_FORECHANNEL_STREAMBITRATE + APP_BACKCHANNEL_STREAMBITRATE)

#define	APP_FORCED_ALARM_RECORDTIME		10				// ǿ�Ʊ���ʱ��10����

// ��ָ��λ�ÿ�ʼ��������ַ��������������X�������λ��(����һ���ַ���ʼ�����X����λ��)
XMCOORD AP_TextOutDataTimeString (HANDLE hWnd, XMCOORD x, XMCOORD y, char *text, int size);

XMBOOL AP_TextGetStringSize (char *text, int size, XMSIZE *pSize);

XMCOORD AP_TextOutWhiteDataTimeString (HANDLE hWnd, XMCOORD x, XMCOORD y, char *text, int size);

// �������ʱ��

// ��ʽ1�� ���ʱ�估���� 2012/09/16 10:22
#define	APP_DATETIME_FORMAT_1				1
// ��ʽ2�� ������� 2012/09/16
#define	APP_DATETIME_FORMAT_2				2
// ��ʽ3�� ���ʱ�� 10:22
#define	APP_DATETIME_FORMAT_3				3
// ��ʽ4�� ���ʱ���� 10:22:20
#define	APP_DATETIME_FORMAT_4				4
// ��ʽ5�� ������� 09/16 10:22
#define	APP_DATETIME_FORMAT_5				5
// ��ʽ6�� ����·�/���� 09/16
#define	APP_DATETIME_FORMAT_6				6
#define   APP_DATETIME_FORMAT_7                        7
#define	DATETIME_CHAR_WIDTH 15//10

// ��ʽ��ϵͳʱ���ַ���
int AP_FormatDataTime (XMSYSTEMTIME *lpSystemTime, DWORD dwType, char *lpTextBuffer, int cbTextBuffer);

void AP_TextOutDateTime (HANDLE hWnd, XMCOORD x, XMCOORD y, XMSYSTEMTIME *lpSystemTime, DWORD dwType);

VOID AP_VideoFileNameToDisplayName (const char *lpVideoFileName, char *lpDisplayName);

// ALERT�Ӵ�ȱʡ����ɫ��ALPHAֵ����
#define	APP_ALERT_BKGCOLOR		0xB0404040
#define	APP_ALERT_BKGALPHA		1.0f

// ϵͳʱ�����ô��ڶ�������, �ݶ�2��
// 1	ϵͳʱ������ȱʡ�������ԣ�3����ť��ȷ�ϡ���һ����ȡ��
// 2	ǿ��ϵͳʱ�����ö������ԣ�2����ť��ȷ�ϡ���һ�� 
#define	APP_DATETIMESETTING_CUSTOM_DEFAULT		HWND_CUSTOM_DEFAULT
#define	APP_DATETIMESETTING_CUSTOM_FORCED		(HWND_CUSTOM_DEFAULT+1)




#define	APP_CARD_OPERATION_MODE			0			// ������ģʽ
#define	APP_DEMO_OPERATION_MODE			1			// �޿�����ģʽ(��ʾģʽ)


void AppInit (void);
void AppExit (void);

void XM_AppInitStartupTicket (void);
// ��ȡAPP����ʱ��
DWORD AppGetStartupTicket (void);

// ��ǿ����¼��
void APPMarkCardChecking (int stop_record);

// ����һ������
void AP_OnekeyPhotograph (void);

// ��Ƶ�б�ģʽ����(ѭ��������)
#define	VIDEOLIST_ALL_VIDEO_MODE				0		// ����¼���б����ģʽ
#define	VIDEOLIST_PROTECT_VIDEO_MODE			1		// ����¼���б����ģʽ
#define	VIDEOLIST_CIRCULAR_VIDEO_MODE			2		// ѭ��¼���б����ģʽ

void AP_AlbumInit (void);

// ��ȡ�������Ƭ������
// ����ֵ
// =  0  ���Ϊ��
// >  0  ����е���Ƭ����
unsigned int AP_AlbumGetPhotoCount (void);

// ������Ƭ����ֵ��ȡ��Ƭ��Ӧ������(��ʾ����)
const char *AP_AlbumGetPhotoName (unsigned int index);

unsigned int AP_AlbumGetPhotoChannel (unsigned int index);


// ������Ƭ����ֵ��ȡ��Ƭ��Ӧ���ļ���
// index   ��Ƭ����ֵ
// ����ֵ
// < 0  ʧ��
//   0  �ɹ�
int AP_AlbumGetPhotoFileName (unsigned int index, char *lpFileName, int cbFileName);

// ������Ƭ����ֵ����Ƭ�������ɾ��
// index   ��Ƭ����ֵ
// ����ֵ
// < 0     ʧ��
//   0     �ɹ�
int AP_AlbumDeletePhoto (unsigned int index);

// ɾ����������е���Ƭ
// ����ֵ
// < 0     ʧ��
//   0     �ɹ�
int AP_AlbumDeleteAllPhoto (void);

// ��Ƶ��ѡ����Ϣ
#define	AP_USER_VIDEO_ITEM_SELECT			1

// ��ȡ��Ƶ�б�(ѭ���򱣻���Ƶ�б�)��ָ����ŵ���Ƶ�ļ���
char *AP_VideoListGetVideoFileName (DWORD dwVideoFileIndex);
char *AP_VideoListGetVideoDisplayName (DWORD dwVideoIndex, char *lpDisplayName, int cbDisplayName);


// ����¼���б��Ӵ��ľ۽���
XMBOOL AP_VideoListViewSetFocusItem (UINT uFocusItem);

// ȷ����ͼ��ȷ�Ϻ���ûص�������ɲ��������ݲ��������ʾ�ɹ���ʧ��
// dwViewTitleID ��Ϣ�ı�ID
// dwViewMessageID ��Ϣ�ı�ID
// fpMenuOptionCB �ص�����
VOID AP_OpenComfirmView (DWORD dwViewTitleID, DWORD dwViewMessageID, void *fpMenuOptionCB);
// ��ʾ��Ϣ��ͼ
// dwViewTitleID ��Ϣ�ı�ID
// dwViewMessageID ��Ϣ�ı�ID
// dwAutoCloseTime ָ���Զ��ر�ʱ��(1/10�뵥λ), (-1) ��ֹ�Զ��ر�
VOID AP_OpenMessageViewEx (DWORD dwViewTitleID, DWORD dwViewMessageID, DWORD dwAutoCloseTime, XMBOOL bPopTopView);

// ��ʾ��Ϣ��ͼ(ʹ��ȱʡ���⡢3�����Զ��ر�)
VOID AP_OpenMessageView (DWORD dwViewMessageID);

const char *AP_TextPromptsVoice (unsigned short Voice);


// �򿪸�ʽ���Ӵ�
// bPushView 
//	1 --> ��ʾ�������Ӵ�ѹ�뵽�Ӵ�ջ
// 0 --> ��ʾ�����ص������Ӵ���ִ����󣬷��ص�����
VOID AP_OpenFormatView (XMBOOL bPushView);

// ��ϵͳ�����Ӵ�
// push_view_or_pull_view	
//		1	push view, ��ϵͳ�����Ӵ�ѹ�뵽��ǰ�Ӵ�ջ��ջ��
//		0	pull view, ����ǰ�Ӵ�ջ�������Ӵ�����, Ȼ���ٽ�ϵͳ�����Ӵ�ѹ�뵽�Ӵ�ջ��ջ��
VOID AP_OpenSystemUpdateView (unsigned int push_view_or_pull_view);

// Ͷ��ϵͳ�¼�����Ϣ����
VOID AP_PostSystemEvent (unsigned int event);


// ��ʾ��Ϣ��ͼ
// 0 ��ͼ����������ͼ��ʾʧ��
// 1 ��ͼ��������ʾ
XMBOOL XM_OpenOkCancelView ( 
								 DWORD dwInfoTextID,				// ��Ϣ�ı���ԴID
																		//		��0ֵ��ָ����ʾ������Ϣ����ԴID
								 DWORD dwImageID,					// ͼƬ��Ϣ��ԴID
																		//		��0ֵ��ָ����ʾͼƬ��Ϣ����ԴID	

								 DWORD dwButtonCount,			// ��ť������1����2
																		//		������һ����ťʱ��
																		//		OK��ť��ʹ��VK_F1(Menu)
																		//		Cancel������ʹ��VK_F2(Mode)
								 DWORD dwButtonNormalID[],		//	��ť��ԴID
																		//		0 
																		//			��ʾû�ж���Button��OkCancel��Ϊһ����Ϣ��ʾ���ڡ�
																		//			���������ʱ���Զ��ر�
																		//		����ֵ
																		//			��ʾButton������Ϣ����ԴID��OkCancel��Ҫ��ʾһ����ť
								 DWORD dwButtonPressedID[],	//	��ť������ԴID
																		//		0
																		//			��ʾû�ж������Դ��
																		//			��ť����ʱʹ��dwButtonNormalID��Դ
																		//			*** ��û���ṩ��ť����Ч����ͼƬ������ͼƬ���ȣ��䰵��
																		//				 �����ɰ���Ч��
																		//		����ֵ
																		//			��ʾButton����ʱ������Ϣ����ԴID
								 DWORD dwBackgroundColor,		// ����ɫ
																		//		0
																		//			��ʾʹ��ȱʡ����ɫ
																		//		����ֵ
																		//			��ʾ��Ч��䱳��ɫ������ɫ��ָ��Alpha����
								 float fAutoCloseTime,			//	ָ���Զ��ر�ʱ�� (�뵥λ)��
																		//		0.0	��ʾ��ֹ�Զ��ر�
								 float fViewAlpha,				// ��Ϣ��ͼ���Ӵ�Alpha���ӣ�0.0 ~ 1.0
																		//		0.0	��ʾȫ͸
																		//		1.0	��ʾȫ����
								 FPOKCANCELCB okcancelcb,		// ��ť����Ļص�����
																		//		// ��ť����Ļص�����������Ϊ XM_COMMAND_OK ���� XM_COMMAND_CANCEL
								 DWORD dwAlignOption,			//	��ͼ����ʾ����������Ϣ
																		//		���OSD��ʾ��ԭ��(OSD��Ч��������Ͻ�)
																		//		XM_VIEW_ALIGN_CENTRE	
																		//			�Ӵ����ж���(���OSD��ʾ����)
																		//		XM_VIEW_ALIGN_BOTTOM
																		//			�Ӵ��ײ����ж���(���OSD��ʾ����)
								 DWORD dwOption					// OKCANCEL��ͼ�Ŀ���ѡ��
																		//		XM_OKCANCEL_OPTION_ENABLE_POPTOPVIEW
																		//			����ջ���Ӵ�ʹ��
								 );


#define	AP_RECYCLEVIDEOALERTVIEW_OPTION_TABLE			0x00000000	// ������
#define	AP_RECYCLEVIDEOALERTVIEW_OPTION_GRAPH			0x00000001	// ͼ�����

// ѭ��¼�񱨾���Ϣ��ʾ�Ӵ�
//
//	----- ��ͼ����ģʽ֧�� ( XM_VIEW_ALIGN_CENTRE �� XM_VIEW_ALIGN_BOTTOM ) -----
//
//	1) ��ֱ����/ˮƽ������ʾ XM_VIEW_ALIGN_CENTRE
//	2) �ײ�����/ˮƽ������ʾ XM_VIEW_ALIGN_BOTTOM
//
//  ---- UIԪ�ز���(Layout) ----
//
// 1) ��ʾ��������Ϣ���ݡ���ͼƬ��Ϣ���ݡ�
//		�հ���
//		������Ϣ����
//		�հ���
//		ͼƬ��Ϣ����
//		�հ���
//
// 2) ����ʾ����Ϣ���ݡ�
//		�հ���
//		��Ϣ����
//		�հ���

// ��ʾ��Ϣ��ͼ
// 0 ��ͼ����������ͼ��ʾʧ��
// 1 ��ͼ��������ʾ
XMBOOL XM_OpenRecycleVideoAlertView ( 
								 DWORD dwTitleID,					// ������ԴID

								 DWORD dwBackgroundColor,		// ����ɫ
																		//		0
																		//			��ʾʹ��ȱʡ����ɫ
																		//		����ֵ
																		//			��ʾ��Ч��䱳��ɫ������ɫ��ָ��Alpha����
								 float fAutoCloseTime,			//	ָ���Զ��ر�ʱ�� (�뵥λ)��
																		//		0.0	��ʾ��ֹ�Զ��ر�
								 float fViewAlpha,				// ��Ϣ��ͼ���Ӵ�Alpha���ӣ�0.0 ~ 1.0
																		//		0.0	��ʾȫ͸
																		//		1.0	��ʾȫ����

								 DWORD dwAlignOption,			//	��ͼ����ʾ����������Ϣ
																		//		���OSD��ʾ��ԭ��(OSD��Ч��������Ͻ�)
																		//		XM_VIEW_ALIGN_CENTRE	
																		//			�Ӵ����ж���(���OSD��ʾ����)
																		//		XM_VIEW_ALIGN_BOTTOM
																		//			�Ӵ��ײ����ж���(���OSD��ʾ����)
								 DWORD dwOption					// RECYCLEVIDEOALERTVIEW��ͼ�Ŀ���ѡ��
								 );




// ʹ�ò˵�ID, ��ָ�����ڵ�ָ���������Rom Image
int AP_RomImageDrawByMenuID (	DWORD MenuID,
								HANDLE hWnd, 
								XMRECT *lpRect, DWORD dwFlag
							);

// ��ǰ����Ƶ������ͼ
VOID AP_OpenVideoView (BYTE mode, WORD wVideoFileIndex);
// �򿪺�����Ƶ������ͼ
VOID AP_OpenVideoBackView (BYTE mode, WORD wVideoFileIndex);

int AppLocateItem (HANDLE hwnd, int item_count, int item_height, int top_item, int x, int y);
int AppLocateItem1 (HANDLE hwnd, int item_count, int item_height, int top_item, int x, int y);

#if defined (__cplusplus)
	}
#endif		/* end of __cplusplus */

#endif	// #ifndef _XSPACE_APP_H_w
