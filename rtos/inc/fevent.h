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


#define UNUSED_EVENT          -1
#define UNKNOW_EVENT          0
#define KEY_EVENT	          	1
#define TIMER_EVENT	         2
#define BUTTON_PRESS_EVENT    3
#define BUTTON_RELEASE_EVENT  4
#define FMOVE_EVENT           5
#define FREPAINT_EVENT        6
#define PIC_EVENT			  		7
#define CARD_EVENT		      8
#define SEARCH_EVENT          9
#define MUSIC_EVENT           10
#define MEDIA_EVENT           11  
#define BLUETOOTH_EVENT   		12 
#define URGENCY_EVENT			13
#define STANDBY_EVENT			14
#define CAR_BACK_EVENT			15
#define MCU_EVENT					16
#define MODULE_SWITCH_EVENT	17
#define DVD_EVENT		      	18
#define RADIO_EVENT           19
#define TV_EVENT					20
#define IPOD_EVENT		      21
#define ITU656IN_EVENT        22
#define VIDEOITEM_EVENT			23

#define REUPDATA_EVENT      252
#define CONFER_EVENT        253
#define EXIT_EVENT          254
#define ANY_EVENT	        255


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
// 检查相同的消息是否已在系统消息队列中存在.
// 避免某些消息的不断重发(SDCARD_REINIT)导致系统响应迟钝
// 若存在, 返回1
// 不存在, 返回0
int chkEventQueue(fEvent *pEvent);


#ifdef __cplusplus
}
#endif

#endif

