//****************************************************************************
//
//	Copyright (C) ShenZhen ExceedSpace
//
//	Author	ZhuoYongHong
//
//	File name: xm_hybrid_stream.c
//	  ������ӿ�
//
//	Revision history
//
//
//		2013.03.25	ZhuoYongHong first version
//
//***************************************************************************
#include "xm_hybrid_stream.h"
#include "xm_printf.h"
#include "CdrRequest.h"
#include "xm_core.h"

extern void XMSYS_H264CodecForceIFrame (void);


static unsigned int ch_no[XM_HYBRID_CH_COUNT];		// ��¼ÿ��ͨ�����յ�������֡���
static unsigned int hybrid_state;
static OS_RSEMA HybridSema;				// �첽��Ϣ���ʻ���


static const char *ch_name[XM_HYBRID_CH_COUNT] = 
{
	"VIDEO_F",
	"VIDEO_B",
	"VOICE"
};

static const char *type_name[XM_HYBRID_TYPE_COUNT] = 
{
	"I_FRAME",
	"P_FRAME",
	"PCM_8000"
};


int XM_HybridStreamInit (void)
{
	int ch;
	for (ch = 0; ch < XM_HYBRID_CH_COUNT; ch ++)
		ch_no[ch] = 0;
	
	OS_CREATERSEMA (&HybridSema); /* Creates resource semaphore */
	
#if HYBRID_H264_MJPG
	hybrid_state = 1;		// ȱʡʹ�ܻ촫ģʽ
#endif
	return 0;
}


// ǿ����Ƶͨ�����¿�ʼI֡����
// �����˷���֡��ʧʱ, ��������һ֡��I֡����
int XM_HybridStreamForceIFrame (void)
{
	OS_Use (&HybridSema);
	if(hybrid_state)
	{
		XMSYS_H264CodecForceIFrame ();
	}
	OS_Unuse (&HybridSema);
	return 0;
}

// ������ϱ��봫��ģʽ
// ��ÿ��ͨ����֡�����Ϊ0.
int XM_HybridStreamStart (void)
{
	int ch;
	OS_Use (&HybridSema);
	XMSYS_H264CodecForceIFrame ();
	for (ch = 0; ch < XM_HYBRID_CH_COUNT; ch ++)
		ch_no[ch] = 1;
	hybrid_state = 1;
	OS_Unuse (&HybridSema);
	return 0;
}

// ֹͣ���ͨ������ģʽ
int XM_HybridStreamStop (void)
{
	OS_Use (&HybridSema);
	hybrid_state = 0;
	OS_Unuse (&HybridSema);
	return 0;
}

// ����Hybrid��
int XM_HybridInsertHybridStream (unsigned int ch, unsigned int type, char *hybrid_data, unsigned int hybrid_size)
{
	int ret = -1;
	if(hybrid_state == 0)
		return 0;
	
	if(ch >= XM_HYBRID_CH_COUNT)
	{
		XM_printf ("XMSYS_CdrInsertHybridStream Failed, illegal ch (%d)\n", ch);
		return -1;
	}
	if(type >= XM_HYBRID_TYPE_COUNT)
	{
		XM_printf ("XMSYS_CdrInsertHybridStream Failed, illegal type (%d)\n", type);
		return -1;
	}
	if(hybrid_data == NULL || hybrid_size == 0)
	{
		return -1;
	}
	
	OS_Use (&HybridSema);
	ret =  XMSYS_CdrInsertHybridStream (ch, type, ch_no[ch], hybrid_data, hybrid_size);
	if(ret >= 0)
	{
		ch_no[ch] ++;
	}
	OS_Unuse (&HybridSema);
	
	return ret;
}



