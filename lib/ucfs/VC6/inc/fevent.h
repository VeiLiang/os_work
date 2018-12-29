#ifndef __FEVENT_H__
#define __FEVENT_H__
#include "xm_proj_define.h"
#include "rtos.h"

#ifdef __cplusplus
extern "C" {
#endif
	
#define NULL_DEVICE		0
#define SD_DEVICE			(1<<0)
#define USB_DEVICE		(1<<4)
	

typedef struct _fEvent fEvent;
typedef struct _fEventQue fEventQue;

/* display status */
#define   DISPLAY_DEF      				0   	/* display default */
#define   DISPLAY_NOT      				1 	/* don't display */
#define	CHANGE_LANGUAGE				3	/* if change language then reload UI */

#define UNKNOW_WTYPE					0
#define WIDGET_WTYPE					1
#define ROOTWGT_WTYPE					2
#define FWINDOW_WTYPE					3
#define BMPLABEL_WTYPE					4
#define BMPANIMAT_WTYPE				5
#define BMPZOOM_WTYPE					6
#define STRLABEL_WTYPE					7
#define MBMPLABEL_WTYPE				8


//#define BUTTON_WTYPE     				20
#if USED_MENUBOX
#define MENUBOX_WTYPE					9
#endif

#if USED_MENUBAR
#define MENUBAR_WTYPE					10
#endif

#define CALENDAR_WTYPE					11

#define BMPITEM_WTYPE					12
#define TIMEITEM_WTYPE					13
#define LISTGROUP_WTYPE				14
#define THUMBVIEW_WTYPE				15
#define PICDECODER_WTYPE				16
#define REFRESH_WTYPE					17
#define PS_WTYPE						18
#define SHOW_WTYPE						19
#define OSDPAINT_WTYPE					20
#define ROLLLABEL_WTYPE				21
#define SEARCHBAR_WTYPE				22
#define UPGRADEBAR_WTYPE				23
#define FMMID_WTYPE					24
#define FTOP_WTYPE						25
#define BLUETOOTHITEM_WTYPE			26
#define TEXTLIST_WTYPE					27
#define SCROLLBAR_WTYPE				28
#define FLASHBOX_WTYPE					29
#define EDITBOX_WTYPE					30
#define STATICTEXT_WTYPE				31
#define SPIN_WTYPE						32
#define GROUP_WTYPE					33
#define VANSPIN_WTYPE					34
#define PICSPIN_WTYPE					35
#define PICNUMSPIN_WTYPE				36
#define MUSIC_WTYPE					37
#define THUMPICSHOW_WTYPE			38
#define FILEINFO_WTYPE					39
#define FILELIST_WTYPE					40
#define STRINGITEM_WTYPE				41
#define ROLLITEM_WTYPE					42
#define MUSICPLAYFORM_WTYPE			43
#define TIMESPIN_WTYPE					44
#define SOUND_WTYPE					45
#define CBUTTON_WTYPE					46
#define VANBTN_WTYPE					47
#define VANCOMB_WTYPE					48
#define VANLIST_WTYPE					49
#define CTRITEM_WTYPE					50
#define TOUCHCHK_WTYPE				51
#define VOLBAR_WTYPE					52
#define SCRNSAVER_WTYPE				53
#define SAVETIMER_WTYPE				54
#define MPLAYER_WTYPE 					55
#define VIDEO_WTYPE					56
#define BUTTON_WTYPE					57

#define IPODCONTROL_WTYPE				58

#define NUMBERTABLE_WTYPE				59
#define NUMBOX_WTYPE					60

#define IPODLIST_WTYPE					61
#define FLABEL_WTYPE					62
#define MBMPSITEM_WTYPE				63
#define  CTRL_WTYPE						64
#define  SETTIMEBAR_WTYPE				65
#define  TITLEBAR_WTYPE					66
#define  FICONVIEW_WIYPE				67
#define FAVIN_WTYPE					68

#define FSETUPBAR_WTYPE				69
#define FTIMESHOW_WTYPE				70//XJY0518
#define PICCTRL_WTYPE					71//bychen
#define DIR_WTYPE						72//bychen
#define CLOCK_WTYPE					73
#define TEXTVIEW_WTYPE					74
#define LOAD_WTYPE						75
#define BTDIAL_WTYPE					76
#define BTPHONE_WTYPE					77
#define BTAUDIO_WTYPE					78
#define BTSETUP_WTYPE					79
#define BTCALLED_WTYPE					80
#define ICONLIST_WTYPE					81
#define COMBTN_WTYPE					82
#define SETTINGFORM_WTYPE				83
#define FILELISTFORM_WTYPE				84
#define PROGRESSBAR_WTYPE				85
#define SETRADIOFORM_WTYPE			86
#define SETBGPICFORM_WTYPE			87
#define BTDIALFORM_WTYPE			88
#define FMSHOWBAR_WTYPE				89
#define FMFORM_WTYPE					90
#define SCALEPOINTER_WTYPE			91
#define SPECTRUMWIDGET_WTYPE			92
#define BTSETUPFORM_WTYPE				93
#define SETVIDEOFORM_WTYPE			94
#define SETSYSFORM_WTYPE				95
#define SETLANGUAGEFORM_WTYPE		96
#define SETSTDTIMEFORM_WTYPE			97
#define LOADERFORM_WTYPE        			98
#define BOTTOM_WTYPE					99
#define MAINMENUFORM_WTYPE			100
#define SETBLACKFORM_WTYPE			101
#define FMENUTOP_WTYPE				102
#define SETSOUNDFORM_WTYPE			103
#define LINEBAR_WTYPE					104

#define BTPINCODEFORM_WTYPE			105
#define BTBASE_WTYPE					106
#define BTCALLOGFORM_WTYPE			107
#define BTMUSICFORM_WTYPE				108
#define BTMUSICFREQ_WTYPE				109
#define FAVINFORM_WTYPE				110
#define SETSYSINFOFORM_WTYPE			111
#define DVDFORM_WTYPE					112
#define CDFORM_WTYPE					113
#define CDNUMFORM_WTYPE				114
#define CDMUSICFORM_WTYPE				115
#define FICONSCROLLVIEW_WIYPE			116
#define DVDLOADERFORM_WTYPE			117
#define FTEXTSCROLLVIEW_WIYPE			118
#define FINFOWINDOW_WTPYE			119
#define SETTIMEFORM_WTYPE				120
#define SETCARLOGOFORM_WTYPE			121
#define SETSTEERWHEELFORM_WTYPE		122
#define MUSICCOVERFORM_WTYPE			123
#define VIDEOOPTIONFORM_WTYPE		124
#define IPODFORM_WTYPE					125
#define NAVIFORM_WTYPE					126
#define SETFACTORYFORM_WTYPE			127
#define TVFORM_WTYPE                200
#define SLIDELIST_WTYPE				201
#define SLIDESWITCH_WTYPE			202
#define CONTAINER_WTYPE				203		
#define MULBTN_WTYPE				204	
#define SLIDEBAR_WTYPE				205
#define SLIDEMENU_WTYPE				206
#define NUMBERSTR_WTYPE				207
#define RADIO_BTN_WTYPE              220
#define IPOD_MUSICFORM_WTYPE		221
#define IPOD_VIDEOFORM_WTYPE		222


#define UNUSED_EVENT          -1
#define UNKNOW_EVENT          0
//#define KEY_EVENT	          1
#define TIMER_EVENT	          2
#define BUTTON_PRESS_EVENT    3
#define BUTTON_RELEASE_EVENT  4
#define FMOVE_EVENT           5
#define FREPAINT_EVENT        6
#define PIC_EVENT			  7
#define CARD_EVENT		      8
#define SEARCH_EVENT          9
#define MUSIC_EVENT           10
#define MEDIA_EVENT             11  
#define BLUETOOTH_EVENT     12 
#define URGENCY_EVENT		13
#define STANDBY_EVENT		14
#define CAR_BACK_EVENT		15
#define MCU_EVENT			16
#define MODULE_SWITCH_EVENT  17
#define DVD_EVENT		      18
#define RADIO_EVENT             19
#define TV_EVENT			20
#define IPOD_EVENT		      21
#define ITU656IN_EVENT        22

#define REUPDATA_EVENT      252
#define CONFER_EVENT        253
#define EXIT_EVENT          254
#define ANY_EVENT	        255

#define TIMER_EVENT_REGION	50

typedef struct _fTimer_Event 
{
	int event_type;
	void *ptimer;    
}fTimer_Event;


#define KEY_DOWN			0
#define KEY_UP				1
#define KEY_REPEAT			2

typedef struct _fKey_Event
{
	int event_type;
	int key_type;
	int keyCode;
}fKey_Event;


typedef struct _fTouch_Event
{
	int event_type;
	int x;
	int y;
}fTouch_Event;


typedef struct _fAny_Event
{
	int event_type;
	void *pdata;  
}fAny_Event;

typedef struct _fPic_Event 
{
	int event_type;
	int result;
	void *pdata;    
}fPic_Event;

typedef struct _fSearch_Event 
{
	int event_type;
	int result;
	void *pdata;    
}fSearch_Event;

typedef struct _fMedia_Event_t 
{
	int event_type;
	int state_code; // state code from decoder
	void *pdata; 
} fMedia_Event;


typedef struct _fBluetooth_Event_t 
{
	int event_type;
	unsigned int message_code; // message code from bluetooth
	void *pdata; 
} fBluetooth_Event;

typedef struct _fCarBack_Event_t 
{
	int event_type;
	unsigned int code; // message code from car back
	void *pdata; 
} fCarBack_Event;

typedef struct _fMcu_Event_t 
{
	int event_type;
	unsigned char code[16];
	void *pdata; 
} fMcu_Event;

typedef struct _fModuleSwitch_Event_t 
{
	int event_type;
	int nModule;
	void *pdata; 
} fModuleSwitch_Event;

typedef struct _fStandBy_Event_t 
{
	int event_type;
	int standby_code;	//0: enter standby mode, 1: quit standby mode
	void *pdata; 
} fStandBy_Event;


typedef struct _fConfer_Event
{
	int event_type;
	void *pdata;  
}fConfer_Event;


typedef struct _fMove_Event
{
	int event_type;
	int x;
	int y;
	int width;
	int height;  
}fMove_Event;


typedef struct _fRepaint_Event
{
	int event_type;
	void *pdata;
}fRepaint_Event;

typedef struct _fCard_Event 
{
	int event_type;
	int cardType;
	int cardID;
	int plugIn;    
}fCard_Event;

typedef struct _fDVD_Event 
{
	int event_type;
	int msg_type;	//0:出入碟机构消息；1: DVD 机芯通讯消息
	int parameter;
	unsigned char code[16];
	void *pdata;
}fDVD_Event;

typedef struct _fRadio_Event 
{
	int event_type;
	int parameter;
	void *pdata;
}fRadio_Event;


typedef struct _fTV_Event 
{
	int event_type;
	int msg_type;	//0:开始搜索消息 1:搜索结果查询消息	2:停止搜索消息
	int start_freq;
}fTV_Event;

typedef struct _fIpod_Event_t 
{
	int event_type;
	int message_code; 	//msg code from ipod
	int mode;
	void *pdata; 
} fIpod_Event;

typedef struct _fITU656in_Event 
{
	int event_type;
	int msg_type; 	

} fITU656in_Event;

/**************************************/
struct _fEvent
{
	union 
	{
		int fKind;
		fTimer_Event timer_event;
		fKey_Event key_event;
		fAny_Event any_event;
		fMove_Event move_event;
		fConfer_Event confer_event;
		fRepaint_Event repaint_event;  
		fPic_Event  pic_event; 
		fCard_Event    card_event;  
		fSearch_Event  search_event;
		fMedia_Event media_event;
		fTouch_Event touch_event;
		fBluetooth_Event bluetooth_event;
		fStandBy_Event standby_event;
		fMcu_Event	mcu_event;
		fModuleSwitch_Event module_switch_event;
		fDVD_Event    DVD_event;  
		fRadio_Event  Radio_event;
		fTV_Event Tv_event;
		fIpod_Event	Ipod_event;
	}uFEvent;  
};



///////////////////////////////////////////////////


int addEventQueue(fEvent *pEvent);


#ifdef __cplusplus
}
#endif

#endif

