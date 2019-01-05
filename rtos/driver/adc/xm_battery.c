//****************************************************************************
//
//	Copyright (C) 2010 ShenZhen ExceedSpace
//
//	Author	ZhuoYongHong
//
//	File name: xm_touch_drv.c
//	  �����¼�����
//
//	Revision history
//
//		2010.09.02	ZhuoYongHong Initial version
//
//****************************************************************************

#include <xm_type.h>
#include <xm_user.h>
#include <xm_dev.h>
#include <xm_base.h>
#include <xm_key.h>
#include <xm_printf.h>



typedef struct {
	unsigned int battery;
}BATTERY_EVENT;

#define	BATTERY_BUFFER_SIZE	0x1F		/* maximum key strobe buffer */


static BATTERY_EVENT		BatteryBuffer[BATTERY_BUFFER_SIZE+1];		// ѭ������
static volatile	BYTE	   	BatteryBPos;			// ѭ�������ײ�
static volatile	BYTE		BatteryEPos;			// ѭ������β��
static volatile	BYTE		BatteryInit = 0;		// ��ʶ���������Ƿ��ѳ�ʼ��


// Ͷ��Ӳ�������¼�, ���ⲿӲ����������
// ���뵽��β
// Ͷ�ݴ����¼�����Ϣ����
// point		����λ��
// type		�����¼�����
// ����ֵ����
//		1		�¼�Ͷ�ݵ��¼�������гɹ�
//		0		�¼�Ͷ�ݵ��¼��������ʧ��
unsigned char XM_BatteryEventProc(unsigned int batteryvalue)
{
	BATTERY_EVENT *bat;

	XM_lock ();
	if(BatteryInit == 0)
	{
		XM_unlock ();
		return 0;
	}

	// ����Զ������Ƿ���ڡ��Զ������½�ֹ��������
	//if(TesterSendCommand(TESTER_CMD_TEST, NULL))
	//	return 0;

	// ����������
	if( ((BatteryEPos + 1) & BATTERY_BUFFER_SIZE) == (BatteryBPos & BATTERY_BUFFER_SIZE) )
	{
		XM_unlock ();
		return 0;
	}

	bat = BatteryBuffer + (BatteryEPos & BATTERY_BUFFER_SIZE);
	bat->battery = batteryvalue;
	BatteryEPos ++;

	XM_wakeup ();

	XM_unlock ();

	return 1;
}

XMBOOL XM_BatteryDriverGetEvent(unsigned int *batteryvalue)
{
	BATTERY_EVENT *bat;
	XMBOOL ret = FALSE;
	
	if(BatteryInit == 0)
		return FALSE;
	
	XM_lock ();	// �����������
	if( BatteryBPos != BatteryEPos )
	{
		ret = TRUE;
		bat = BatteryBuffer + (BatteryBPos & BATTERY_BUFFER_SIZE);

		*batteryvalue = bat->battery;

		BatteryBPos ++;
	}
	XM_unlock ();
	
	return ret;
}


void XM_BatteryDriverOpen(void)
{
	XM_lock ();
	
	BatteryBPos = BatteryEPos = 0;
	BatteryInit = 1;
		
	XM_unlock ();
}


void XM_BatteryDriverClose (void)
{
	XM_lock ();
	BatteryInit = 0;
	BatteryBPos = BatteryEPos = 0;
	XM_unlock ();
}


XMBOOL XM_BatteryDriverPoll(void)
{
	XMBOOL ret = FALSE;

	XM_lock();	// �����������
	if( BatteryEPos != BatteryBPos )	/* Key Event buffer empty */
		ret = TRUE;
	XM_unlock();

	return ret;
}
