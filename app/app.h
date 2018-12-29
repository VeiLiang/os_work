//****************************************************************************
//
//	Copyright (C) 2010 ShenZhen ExceedSpace
//
//	Author	ZhuoYongHong
//
//	File name: app.h
//	  D3视窗定义
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


// 窗口外部声明定义

// 桌面(主界面)视图
XMHWND_DECLARE(Desktop)

// 设置视图
XMHWND_DECLARE(MainView)
XMHWND_DECLARE(Photo)
XMHWND_DECLARE(SettingView)
XMHWND_DECLARE(VideoSettingView)
XMHWND_DECLARE(SystemSettingView)
XMHWND_DECLARE(DateTimeSetting)
XMHWND_DECLARE(LangOptionSetting)
XMHWND_DECLARE(WaterMarkSettingView)
// 标志水印(用户自定义水印)载入视图
XMHWND_DECLARE(WaterMarkLogoLoadView)
XMHWND_DECLARE(SystemVersionView)
//声音
XMHWND_DECLARE(VolView)

//显示开机界面
//XMHWND_DECLARE(MovieRec)

//倒车标识图标界面
XMHWND_DECLARE(CarBackLineView)
//颜色设置界面
XMHWND_DECLARE(ColorView)
//功能
XMHWND_DECLARE(FunctionView)
//倒车延时
XMHWND_DECLARE(GuideView)
//倒车标尺
XMHWND_DECLARE(CameraView)
//工程模式
XMHWND_DECLARE(FactoryView)
// 菜单选项视图
XMHWND_DECLARE(MenuOptionView)

// 视频视图
XMHWND_DECLARE(VideoListView)
XMHWND_DECLARE(VideoBackListView)
XMHWND_DECLARE(VideoView)
XMHWND_DECLARE(VideoBackView)

// 相册列表视图
XMHWND_DECLARE(AlbumListView)
XMHWND_DECLARE(AlbumView)

// 回收站视窗
XMHWND_DECLARE(RecycleView)


// 信息显示视图
XMHWND_DECLARE(MessageView)

// 系统自检视图
XMHWND_DECLARE(CardView)


XMHWND_DECLARE(InitSettingView)

XMHWND_DECLARE(VersionView)

// 系统工具
XMHWND_DECLARE(ProcessView)

// ALERT风格视窗
XMHWND_DECLARE(AlertView)
XMHWND_DECLARE(SD_ProcessView)

// OK/CACNEL模式对话框
XMHWND_DECLARE(OkCancelView)

// 循环录像报警视窗
XMHWND_DECLARE(RecycleVideoAlertView)


XMHWND_DECLARE(SoundVolumeView)

XMHWND_DECLARE(SwitchButtonView)

//回放菜单
XMHWND_DECLARE(PlayBackMenu)
XMHWND_DECLARE(PhotoMenu)

// 支持1, 2, 3, 4个Button样式
#define	XM_MAX_BUTTON_COUNT		5// 4
#define	TP_MAX_BUTTON_COUNT		12

typedef struct _tagXMBUTTONINFO {
	WORD				wKey;					// 对应的按键值
	WORD				wCommand;				// 按钮按下时发送的命令
	DWORD				dwLogoId;				// 按钮的Logo资源ID
	DWORD				dwTextId;				// 按钮的文本(Text)资源ID
} XMBUTTONINFO;

typedef struct _tagTPBUTTONINFO {
	   WORD left;		// 矩形左上角X坐标
	WORD top;		// 矩形左上角Y坐标
	WORD right;		// 矩形右下角X坐标
      WORD bottom;	// 矩形右下角Y坐标
	WORD				wKey;					// 对应的按键值
	WORD				wCommand;				// 按钮按下时发送的命令
	DWORD				dwLogoId;				// 按钮的Logo资源ID
	DWORD				ClickdwLogoId;// 选中按钮的Logo资源ID //dwTextId;	
} TPBUTTONINFO;
#define	XMBUTTON_FLAG_HIDE_BACKGROUND		0x00000001		// 隐藏按钮区的背景元素的显示
#define	XMBUTTON_FLAG_HIDE_HORZ_SPLIT		0x00000002		// 隐藏按钮区的顶部水平分割线的显示
#define	XMBUTTON_FLAG_HIDE_VERT_SPLIT		0x00000004		// 隐藏按钮区的按钮之间的垂直分割线的显示

#define	XMBUTTON_FLAG_HIDE					0x0000FFFF		// 隐藏按钮区的显示

// 按钮控件
typedef struct _tagXMBUTTONCONTROL {
	XMBUTTONINFO 			pBtnInfo[XM_MAX_BUTTON_COUNT];				// 按钮信息
	BYTE						bBtnCount;				// 按钮个数
	BYTE						bBtnClick;				// 0xFF表示没有Button按下
														// 其他值表示当前按下Button的序号
	BYTE						bBtnInited;				// 控件私有数据，表示是否初始化等信息
	BYTE						bBtnEnable;				// 控件使能或禁止标志	
	HANDLE						hWnd;					// 按钮控件所属的窗口句柄
	DWORD						dwBtnBackgroundId;		// 背景色资源ID
	DWORD						dwBtnFlag;				// 按钮控制信息

#if 0
	// 预加载图像资源
	XM_IMAGE *				pImageMenuItemSplitLine;	// 按钮区顶部分隔线
	XM_IMAGE *				pImageButtonBackground;		// 按钮区背景资源
	XM_IMAGE *				pImageButtonSplitLine;		// 按钮区分割竖线

	// 支持1, 2, 3, 4个Button样式
	XM_IMAGE *				pImageLogo[XM_MAX_BUTTON_COUNT];
	XM_IMAGE *				pImageText[XM_MAX_BUTTON_COUNT];				
#endif
} XMBUTTONCONTROL;
// 按钮控件
typedef struct _tagTPBUTTONCONTROL {
	TPBUTTONINFO 			TpBtnInfo[TP_MAX_BUTTON_COUNT];				// 按钮信息
	BYTE						bBtnCount;				// 按钮个数
	BYTE						bBtnClick;				// 0xFF表示没有Button按下
															// 其他值表示当前按下Button的序号
	BYTE						bBtnInited;				// 控件私有数据，表示是否初始化等信息
	BYTE						bBtnEnable;				// 控件使能或禁止标志	
	HANDLE					hWnd;						// 按钮控件所属的窗口句柄
	DWORD						dwBtnBackgroundId;	// 背景色资源ID
	DWORD						dwBtnFlag;				// 按钮控制信息

#if 0
	// 预加载图像资源
	XM_IMAGE *				pImageMenuItemSplitLine;	// 按钮区顶部分隔线
	XM_IMAGE *				pImageButtonBackground;		// 按钮区背景资源
	XM_IMAGE *				pImageButtonSplitLine;		// 按钮区分割竖线

	// 支持1, 2, 3, 4个Button样式
	XM_IMAGE *				pImageLogo[XM_MAX_BUTTON_COUNT];
	XM_IMAGE *				pImageText[XM_MAX_BUTTON_COUNT];				
#endif
} TPBUTTONCONTROL;
XMBOOL AP_ButtonControlInit (XMBUTTONCONTROL *pButtonControl, BYTE bBtnCount, HANDLE hWnd, const XMBUTTONINFO* pButtonInfo);
XMBOOL AP_ButtonControlExit (XMBUTTONCONTROL *pButtonControl);
XMBOOL AP_TpButtonControlInit (TPBUTTONCONTROL *TpButtonControl, BYTE bBtnCount, HANDLE hWnd, const TPBUTTONINFO* pButtonInfo);
XMBOOL AP_TpButtonControlExit (TPBUTTONCONTROL *TpButtonControl);
// 使能或禁止按钮
XMBOOL AP_ButtonControlSetEnable (XMBUTTONCONTROL *pButtonControl, XMBOOL bEnable);

// 按钮消息处理
// 返回值
// TRUE		消息已被按钮控件处理, App无需继续处理
// FALSE		消息未被按钮控件处理, App需要继续处理
XMBOOL AP_VideoplayingButtonControlMessageHandler (XMBUTTONCONTROL *pButtonControl, XMMSG *msg);
XMBOOL AP_ButtonControlMessageHandler (XMBUTTONCONTROL *pButtonControl, XMMSG *msg);
XMBOOL AP_TpButtonControlMessageHandler (TPBUTTONCONTROL *TpButtonControl, XMMSG *msg);
XMBOOL AP_TpButtonControlMessageHandlerDateSet (TPBUTTONCONTROL *TpButtonControl, XMMSG *msg);
// 设置按钮的标志状态, 改变按钮的显示
VOID  AP_ButtonControlSetFlag (XMBUTTONCONTROL *pButtonControl, DWORD Flag);
DWORD AP_ButtonControlGetFlag (XMBUTTONCONTROL *pButtonControl);

// 修改单个按钮的设置
// pButtonControl		按钮控件
// bBtnIndex			需要修改的按钮的索引序号 (序号从0开始)
// pButtonInfo			新的按钮的设置信息
XMBOOL AP_ButtonControlModify (XMBUTTONCONTROL *pButtonControl, BYTE bBtnIndex, const XMBUTTONINFO* pButtonInfo);


// 标题控件
typedef struct _tagXMTITLEBARCONTROL {
	DWORD						dwIconID;		// 图标资源
	DWORD						dwTextID;		// 标题文本资源

	HANDLE					hWnd;				// 按钮控件所属的窗口句柄

#if 0
	XM_IMAGE *				pImageMenuTitleBarBackground;		// 标题背景位图资源
	XM_IMAGE *				pImageMenuItemSplitLine;			// 分割线
	XM_IMAGE *				pImageIcon;								// 图标
	XM_IMAGE *				pImageText;								// 标题文本
#endif
} XMTITLEBARCONTROL;

XMBOOL AP_TitleBarControlInit (XMTITLEBARCONTROL *pTitleBarControl, HANDLE hWnd, DWORD dwIconID, DWORD dwTextID);
XMBOOL AP_TitleBarControlExit (XMTITLEBARCONTROL *pTitleBarControl);
VOID AP_TitleBarControlMessageHandler (XMTITLEBARCONTROL *pTitleBarControl, XMMSG *msg);

// 开关切换控件
// 初始化一个开关切换控件
HANDLE AP_SwitchButtonControlInit (HANDLE hWnd,					// 开关切换控件所属的窗口句柄
									 XMPOINT *lpButtonPoint,		// 开关切换控件显示的位置
									 XMBOOL bInitialState,		// 开关切换控件的初始状态，0 关闭(禁止) 1 开启(使能)
									 VOID (*lpSwitchStateCallback) (VOID *lpPrivateData, unsigned int state),
																		// 状态切换时调用的回调函数
									 VOID *lpPrivateData);

// 关闭开关切换控件
XMBOOL AP_SwitchButtonControlExit (HANDLE hSwitchControl);

unsigned int AP_SwitchButtonControlGetState (HANDLE hSwitchControl);

XMBOOL AP_SwitchButtonControlSetState (HANDLE hSwitchControl, XMBOOL bState);

// 移动开关切换控件
VOID AP_SwitchButtonControlMove (HANDLE hSwitchControl, XMCOORD x, XMCOORD y);			

// 开关切换控件消息处理
VOID AP_SwitchButtonControlMessageHandler (HANDLE hSwitchControl, XMMSG *msg);
						

// 窗口标题显示。若资源未存在，可指定为AP_NULLID "(DWORD)(-1)"
VOID AP_DrawTitlebarControl (HANDLE hWnd, DWORD dwIconId, DWORD dwTextID);



// 窗口行为数据结构定义
typedef struct _tagXMWNDCONTROL {
	int				nTopItem;					// 第一个可视的菜单项
	int				nCurItem;					// 当前选择的菜单项
	int				nItemCount;					// 菜单项个数

	int				nClickedButton;				// -1表示没有Button按下
														// 其他值表示当前按下Button的序号
	
} XMWNDCONTROL;

// 定时器定义
#define	XMTIMER_DESKTOPVIEW					1		// 设置主界面(桌面)下的定时器

#define	XMTIMER_SETTINGVIEW					1		// 设置界面下的定时器
#define	XMTIMER_DATETIMESETTING				1		// 时间设置界面下的定时器，与设置界面共用同一个ID	
#define XMTIMER_VIDEOSETTING             	1       // 录像设置界面下的定时器，与设置界面共用同一个ID	
#define XMTIMER_SYSTEMSETTING            	1       // 系统设置界面下的定时器，与设置界面共用同一个ID	

#define	XMTIMER_VIDEOLISTVIEW				1		// 视频列表视图的定时器		
#define	XMTIMER_VIDEOVIEW					1		// 视频播放视图的定时器
#define XMTIMER_OPTIONVIEW					1       //选择视图的定时器
#define XMTIMER_VERSIONVIEW              	1       //版本视图的定时器
#define XMTIMER_PROCESSVIEW              	1      
#define	XMTIMER_MESSAGEVIEW					2		// 信息显示视图的定时器	
#define	XMTIMER_GIFANIMATING				3		// GIF动画播放定时器
#define	XMTIMER_ALBUMLISTVIEW				1		// 相册列表视图的定时器		
#define XMTIMER_VERSION						1

// *********** 窗口菜单位置宏定义 *************************
#define	APP_TITLEBAR_HEIGHT				42		// 窗口标题控件区高度
#define	APP_BUTTON_HEIGHT					60		// 窗口按钮控件区高度


// *********** 标题栏区位置定义宏 **************
// 标题栏区位置定义(开始位置)
#define	APP_POS_TITLEBAR_X					0
#define	APP_POS_TITLEBAR_Y					0
// 标题栏图标位置
#define	APP_POS_TITLEBAR_ICON_X				2
#define	APP_POS_TITLEBAR_ICON_Y				6
// 标题栏标题位置
#define	APP_POS_TITLEBAR_TEXT_X				22                                               
#define	APP_POS_TITLEBAR_TEXT_Y				4

// *********** 菜单项区位置定义宏 **************
#define	APP_POS_MENU_X							0
#define	APP_POS_MENU_Y							42//48

// 菜单项为5个的水平分割线开始位置
#define	APP_POS_ITEM5_SPLITLINE_X			0
#define	APP_POS_ITEM5_SPLITLINE_Y			42//30		
#define	APP_POS_ITEM5_LINEHEIGHT				63
#define	APP_POS_ITEM5_LINEHEIGHT_DATESETTING		30

#define	APP_POS_ITEM6_LINEHEIGHT			25

// 菜单项目区位置定义(第一项的开始位置)
// 菜单名字位置
#define	APP_POS_ITEM5_MENUNAME_X			22
#define	APP_POS_ITEM5_MENUNAME_Y			55//45//38 文字跟分界线居中
// 菜单值域位置
#define	APP_POS_ITEM5_MENUDATA_X			166
#define	APP_POS_ITEM5_MENUDATA_Y			38
// 菜单项标记位置
#define	APP_POS_ITEM5_MENUFLAG_X			300


// 媒体文件列表位置定义
#define	APP_POS_MEDIA_TYPE_X					8					// 类型标志
#define	APP_POS_MEDIA_NAME_X					(8+24+16)		// 媒体名称位置
#define	APP_POS_MEDIA_CHANNEL_X				(320 - 8 - 24)	// 媒体通道位置


// *********** 按钮区位置定义宏 **************
// Button区的位置
#define	APP_POS_BUTTON_X						0
#define	APP_POS_BUTTON_Y						180

// Button区分割线的位置
#define	APP_POS_BUTTON_SPLITLINE_Y			192

#define	APP_POS_BUTTON_LOGO_Y				194

#define	APP_POS_BUTTON_TEXT_Y				220


// 锁定图标位置
#define	APP_POS_KEY_X							280


// 视频播放器Toolbar按钮定义
#define	APP_POS_VIDEOPLAY_TOOLBAR_X		0
#define	APP_POS_VIDEOPLAY_TOOLBAR_Y		200
#define	APP_POS_VIDEOPLAY_TIME_X			74
#define	APP_POS_VIDEOPLAY_TIME_y			204
#define	APP_POS_VIDEOPLAY_PREV_BTN_X		100
#define	APP_POS_VIDEOPLAY_PLAY_BTN_X		160
#define	APP_POS_VIDEOPLAY_NEXT_BTN_X		220

#define	APP_POS_VIDEOPLAY_BTN_Y				229


// “设置”菜单的菜单项个数
#define	APP_DISPLAY_ITEM_COUNT				5

#define	APP_MENUOPTIONVIEW_ITEM_COUNT			5

#define  APP_WATERMARKSETTING_ITEM_COUNT		4	// 水印设置菜单项项目数量

#define	APP_VIDEOLIST_ITEM_COUNT			5
#define	APP_ALBUMLIST_ITEM_COUNT			5

#define	APP_RECYCLELIST_ITEM_COUNT			5


// 
#define	APP_FORECHANNEL_STREAMBITRATE		0x100000		// 前视通道平均码流(每秒)
#define	APP_BACKCHANNEL_STREAMBITRATE		0xC0000		// 后视通道平均码流(每秒)

#define	APP_STREAMBITRATE						(APP_FORECHANNEL_STREAMBITRATE + APP_BACKCHANNEL_STREAMBITRATE)

#define	APP_FORCED_ALARM_RECORDTIME		10				// 强制报警时间10分钟

// 在指定位置开始输出日期字符串，返回输出的X坐标结束位置(即下一个字符开始输出的X坐标位置)
XMCOORD AP_TextOutDataTimeString (HANDLE hWnd, XMCOORD x, XMCOORD y, char *text, int size);

XMBOOL AP_TextGetStringSize (char *text, int size, XMSIZE *pSize);

XMCOORD AP_TextOutWhiteDataTimeString (HANDLE hWnd, XMCOORD x, XMCOORD y, char *text, int size);

// 输出日期时间

// 格式1： 输出时间及日期 2012/09/16 10:22
#define	APP_DATETIME_FORMAT_1				1
// 格式2： 输出日期 2012/09/16
#define	APP_DATETIME_FORMAT_2				2
// 格式3： 输出时分 10:22
#define	APP_DATETIME_FORMAT_3				3
// 格式4： 输出时分秒 10:22:20
#define	APP_DATETIME_FORMAT_4				4
// 格式5： 输出日期 09/16 10:22
#define	APP_DATETIME_FORMAT_5				5
// 格式6： 输出月份/日期 09/16
#define	APP_DATETIME_FORMAT_6				6
#define   APP_DATETIME_FORMAT_7                        7
#define	DATETIME_CHAR_WIDTH 15//10

// 格式化系统时间字符串
int AP_FormatDataTime (XMSYSTEMTIME *lpSystemTime, DWORD dwType, char *lpTextBuffer, int cbTextBuffer);

void AP_TextOutDateTime (HANDLE hWnd, XMCOORD x, XMCOORD y, XMSYSTEMTIME *lpSystemTime, DWORD dwType);

VOID AP_VideoFileNameToDisplayName (const char *lpVideoFileName, char *lpDisplayName);

// ALERT视窗缺省背景色及ALPHA值定义
#define	APP_ALERT_BKGCOLOR		0xB0404040
#define	APP_ALERT_BKGALPHA		1.0f

// 系统时间设置窗口定制属性, 暂定2种
// 1	系统时间设置缺省定制属性，3个按钮，确认、下一个、取消
// 2	强制系统时间设置定制属性，2个按钮，确认、下一个 
#define	APP_DATETIMESETTING_CUSTOM_DEFAULT		HWND_CUSTOM_DEFAULT
#define	APP_DATETIMESETTING_CUSTOM_FORCED		(HWND_CUSTOM_DEFAULT+1)




#define	APP_CARD_OPERATION_MODE			0			// 卡操作模式
#define	APP_DEMO_OPERATION_MODE			1			// 无卡操作模式(演示模式)


void AppInit (void);
void AppExit (void);

void XM_AppInitStartupTicket (void);
// 获取APP启动时间
DWORD AppGetStartupTicket (void);

// 标记卡重新检查
void APPMarkCardChecking (int stop_record);

// 处理一键拍照
void AP_OnekeyPhotograph (void);

// 视频列表模式定义(循环、保护)
#define	VIDEOLIST_ALL_VIDEO_MODE				0		// 所有录像列表浏览模式
#define	VIDEOLIST_PROTECT_VIDEO_MODE			1		// 保护录像列表浏览模式
#define	VIDEOLIST_CIRCULAR_VIDEO_MODE			2		// 循环录像列表浏览模式

void AP_AlbumInit (void);

// 获取相册中照片的数量
// 返回值
// =  0  相册为空
// >  0  相册中的照片数量
unsigned int AP_AlbumGetPhotoCount (void);

// 根据照片索引值获取照片对应的名称(显示名称)
const char *AP_AlbumGetPhotoName (unsigned int index);

unsigned int AP_AlbumGetPhotoChannel (unsigned int index);


// 根据照片索引值获取照片对应的文件名
// index   照片索引值
// 返回值
// < 0  失败
//   0  成功
int AP_AlbumGetPhotoFileName (unsigned int index, char *lpFileName, int cbFileName);

// 根据照片索引值将相片从相册中删除
// index   照片索引值
// 返回值
// < 0     失败
//   0     成功
int AP_AlbumDeletePhoto (unsigned int index);

// 删除相册中所有的照片
// 返回值
// < 0     失败
//   0     成功
int AP_AlbumDeleteAllPhoto (void);

// 视频项选择消息
#define	AP_USER_VIDEO_ITEM_SELECT			1

// 获取视频列表(循环或保护视频列表)中指定序号的视频文件名
char *AP_VideoListGetVideoFileName (DWORD dwVideoFileIndex);
char *AP_VideoListGetVideoDisplayName (DWORD dwVideoIndex, char *lpDisplayName, int cbDisplayName);


// 设置录像列表视窗的聚焦项
XMBOOL AP_VideoListViewSetFocusItem (UINT uFocusItem);

// 确认视图，确认后调用回调函数完成操作，依据操作结果显示成功与失败
// dwViewTitleID 信息文本ID
// dwViewMessageID 信息文本ID
// fpMenuOptionCB 回调函数
VOID AP_OpenComfirmView (DWORD dwViewTitleID, DWORD dwViewMessageID, void *fpMenuOptionCB);
// 显示信息视图
// dwViewTitleID 信息文本ID
// dwViewMessageID 信息文本ID
// dwAutoCloseTime 指定自动关闭时间(1/10秒单位), (-1) 禁止自动关闭
VOID AP_OpenMessageViewEx (DWORD dwViewTitleID, DWORD dwViewMessageID, DWORD dwAutoCloseTime, XMBOOL bPopTopView);

// 显示信息视图(使用缺省标题、3秒钟自动关闭)
VOID AP_OpenMessageView (DWORD dwViewMessageID);

const char *AP_TextPromptsVoice (unsigned short Voice);


// 打开格式化视窗
// bPushView 
//	1 --> 表示将调用视窗压入到视窗栈
// 0 --> 表示不返回到调用视窗，执行完后，返回到桌面
VOID AP_OpenFormatView (XMBOOL bPushView);

// 打开系统升级视窗
// push_view_or_pull_view	
//		1	push view, 将系统升级视窗压入到当前视窗栈的栈顶
//		0	pull view, 将当前视窗栈顶部的视窗弹出, 然后再将系统升级视窗压入到视窗栈的栈顶
VOID AP_OpenSystemUpdateView (unsigned int push_view_or_pull_view);

// 投递系统事件到消息队列
VOID AP_PostSystemEvent (unsigned int event);


// 显示信息视图
// 0 视图参数错误，视图显示失败
// 1 视图创建并显示
XMBOOL XM_OpenOkCancelView ( 
								 DWORD dwInfoTextID,				// 信息文本资源ID
																		//		非0值，指定显示文字信息的资源ID
								 DWORD dwImageID,					// 图片信息资源ID
																		//		非0值，指定显示图片信息的资源ID	

								 DWORD dwButtonCount,			// 按钮个数，1或者2
																		//		当超过一个按钮时，
																		//		OK按钮，使用VK_F1(Menu)
																		//		Cancel按键，使用VK_F2(Mode)
								 DWORD dwButtonNormalID[],		//	按钮资源ID
																		//		0 
																		//			表示没有定义Button，OkCancel仅为一个信息提示窗口。
																		//			按任意键或定时器自动关闭
																		//		其他值
																		//			表示Button文字信息的资源ID，OkCancel需要显示一个按钮
								 DWORD dwButtonPressedID[],	//	按钮按下资源ID
																		//		0
																		//			表示没有定义该资源。
																		//			按钮按下时使用dwButtonNormalID资源
																		//			*** 若没有提供按钮按下效果的图片，调节图片亮度（变暗）
																		//				 来生成按下效果
																		//		其他值
																		//			表示Button按下时文字信息的资源ID
								 DWORD dwBackgroundColor,		// 背景色
																		//		0
																		//			表示使用缺省背景色
																		//		其他值
																		//			表示有效填充背景色，背景色可指定Alpha分量
								 float fAutoCloseTime,			//	指定自动关闭时间 (秒单位)，
																		//		0.0	表示禁止自动关闭
								 float fViewAlpha,				// 信息视图的视窗Alpha因子，0.0 ~ 1.0
																		//		0.0	表示全透
																		//		1.0	表示全覆盖
								 FPOKCANCELCB okcancelcb,		// 按钮命令的回调函数
																		//		// 按钮命令的回调函数，参数为 XM_COMMAND_OK 或者 XM_COMMAND_CANCEL
								 DWORD dwAlignOption,			//	视图的显示对齐设置信息
																		//		相对OSD显示的原点(OSD有效区域的左上角)
																		//		XM_VIEW_ALIGN_CENTRE	
																		//			视窗居中对齐(相对OSD显示区域)
																		//		XM_VIEW_ALIGN_BOTTOM
																		//			视窗底部居中对齐(相对OSD显示区域)
								 DWORD dwOption					// OKCANCEL视图的控制选项
																		//		XM_OKCANCEL_OPTION_ENABLE_POPTOPVIEW
																		//			弹出栈顶视窗使能
								 );


#define	AP_RECYCLEVIDEOALERTVIEW_OPTION_TABLE			0x00000000	// 表格输出
#define	AP_RECYCLEVIDEOALERTVIEW_OPTION_GRAPH			0x00000001	// 图形输出

// 循环录像报警信息显示视窗
//
//	----- 视图对齐模式支持 ( XM_VIEW_ALIGN_CENTRE 及 XM_VIEW_ALIGN_BOTTOM ) -----
//
//	1) 垂直居中/水平居中显示 XM_VIEW_ALIGN_CENTRE
//	2) 底部对齐/水平居中显示 XM_VIEW_ALIGN_BOTTOM
//
//  ---- UI元素布局(Layout) ----
//
// 1) 显示“文字信息内容”“图片信息内容”
//		空白区
//		文字信息内容
//		空白区
//		图片信息内容
//		空白区
//
// 2) 仅显示“信息内容”
//		空白区
//		信息内容
//		空白区

// 显示信息视图
// 0 视图参数错误，视图显示失败
// 1 视图创建并显示
XMBOOL XM_OpenRecycleVideoAlertView ( 
								 DWORD dwTitleID,					// 标题资源ID

								 DWORD dwBackgroundColor,		// 背景色
																		//		0
																		//			表示使用缺省背景色
																		//		其他值
																		//			表示有效填充背景色，背景色可指定Alpha分量
								 float fAutoCloseTime,			//	指定自动关闭时间 (秒单位)，
																		//		0.0	表示禁止自动关闭
								 float fViewAlpha,				// 信息视图的视窗Alpha因子，0.0 ~ 1.0
																		//		0.0	表示全透
																		//		1.0	表示全覆盖

								 DWORD dwAlignOption,			//	视图的显示对齐设置信息
																		//		相对OSD显示的原点(OSD有效区域的左上角)
																		//		XM_VIEW_ALIGN_CENTRE	
																		//			视窗居中对齐(相对OSD显示区域)
																		//		XM_VIEW_ALIGN_BOTTOM
																		//			视窗底部居中对齐(相对OSD显示区域)
								 DWORD dwOption					// RECYCLEVIDEOALERTVIEW视图的控制选项
								 );




// 使用菜单ID, 在指定窗口的指定区域绘制Rom Image
int AP_RomImageDrawByMenuID (	DWORD MenuID,
								HANDLE hWnd, 
								XMRECT *lpRect, DWORD dwFlag
							);

// 打开前置视频播放视图
VOID AP_OpenVideoView (BYTE mode, WORD wVideoFileIndex);
// 打开后置视频播放视图
VOID AP_OpenVideoBackView (BYTE mode, WORD wVideoFileIndex);

int AppLocateItem (HANDLE hwnd, int item_count, int item_height, int top_item, int x, int y);
int AppLocateItem1 (HANDLE hwnd, int item_count, int item_height, int top_item, int x, int y);

#if defined (__cplusplus)
	}
#endif		/* end of __cplusplus */

#endif	// #ifndef _XSPACE_APP_H_w
