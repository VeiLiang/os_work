#include "common.h"
#include "descript.h"
#include "requests.h"
#include "sensorlib.h"
#include <stdio.h>

void UvcSetCur()
{
	switch (LSB(gsUsbRequest.wIndex))
	{
		case VIDEO_CONTROL :
			video_control_set_cur();
			break;
		case VIDEO_STREAMING :
			video_stream_set_cur();
			break;
		case AUDIO_CONTROL :
			(*pFunc_pFunc_AudioCtrl_SetCur)();	//audio_control_set_cur();
			break;
		case AUDIO_ENDPOINT :	//����AS Sampling Frequency Control
			(*pFunc_pFunc_AudioEpCtrl_SetCur)();	//audio_endpoint_control_set_cur();
			break;
		default :
			gbError = TRUE;
			break;
	}
}

void UvcGetCur()
{
	switch (LSB(gsUsbRequest.wIndex))
	{
		case VIDEO_CONTROL :
			video_control_get_cur();
			break;
		case VIDEO_STREAMING :
			video_stream_get_cur();
			break;
		case AUDIO_CONTROL :
			audio_control_get_cur();
			break;
		default :
			gbError = TRUE;
			break;
	}	
}

void UvcGetMin()
{
	switch (LSB(gsUsbRequest.wIndex))
	{
		case VIDEO_CONTROL :
			video_control_get_min();
			break;
		case VIDEO_STREAMING :
			video_stream_get_cur();			//hally
			break;
		case AUDIO_CONTROL :
			audio_control_get_min();
			break;
		default :
			gbError = TRUE;
			break;
	}	
}

void UvcGetMax()
{
	switch (LSB(gsUsbRequest.wIndex))
	{
		case VIDEO_CONTROL :
			video_control_get_max();
			break;
		case VIDEO_STREAMING :
			video_stream_get_cur();			//hally���˴�һ��Ҫ��ӣ�����WIN7��Vista��Ӧ��
			break;
		case AUDIO_CONTROL :
			audio_control_get_max();
			break;
		default :
			gbError = TRUE;
			break;
	}	
}

void UvcGetRes()
{
	switch (LSB(gsUsbRequest.wIndex))
	{
		case VIDEO_CONTROL :
			video_control_get_res();
			break;
		case AUDIO_CONTROL :
			audio_control_get_res();
			break;
		default :
			gbError = TRUE;
			break;
	}	
}

void UvcGetInfo()
{
	switch (LSB(gsUsbRequest.wIndex))
	{
		case VIDEO_CONTROL :
			video_control_get_info();
			break;
		default :
			gbError = TRUE;
			break;
	}
		
}

void UvcGetLen()
{
	switch (LSB(gsUsbRequest.wIndex))
	{
		case VIDEO_CONTROL :
			video_control_get_len();
			break;
		default :
			gbError = TRUE;
			break;
	}	
}

void UvcGetDef()
{
	switch (LSB(gsUsbRequest.wIndex))
	{
		case VIDEO_CONTROL :
			video_control_get_def();
			break;
		default :
			gbError = TRUE;
			break;
	}
}

/*
0. �� bmRequestType ���� usb, class, vendor
1. �� bRequest ����set_cur, get_cur, get_min ��
2. �� LSB(wIndex) ���� vc,vs, ac, as
3. �� MSB(wIndex) ���� ��unit
4. �� MSB(wValue) ���� ��control
*/
void uvc_request()
{
	gbVideoError = 0x00;
	
	switch (gsUsbRequest.bRequest)
	{
		case SET_CUR :
			UvcSetCur();		
			break;
		case GET_CUR :				//��ȡ��ǰֵ
			UvcGetCur();
			break;
		case GET_MIN :
			UvcGetMin();
			break;
		case GET_MAX :
			UvcGetMax();
			break;
		case GET_RES :
			UvcGetRes();
			break;
		case GET_INFO :				//��ȡ����
			UvcGetInfo();
			break;
		case GET_LEN :
			UvcGetLen();
			break;
		case GET_DEF :				//��ȡĬ��ֵ
			UvcGetDef();
			break;
		default :
			printf ("gbError, uvc_request %x\n", gsUsbRequest.bRequest);
			gbError = TRUE;
			break;		
	}

	gbVideoErrorLast = gbVideoError;	//�ݴ�Video Error,  �Ա���ѯ
}
