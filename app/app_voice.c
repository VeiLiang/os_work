//****************************************************************************
//
//	Copyright (C) 2012 ZhuoYongHong
//
//	Author	ZhuoYongHong
//
//	File name: app_voice.c
//	  ������ʾ����
//
//	Revision history
//
//		2012.09.06	ZhuoYongHong Initial version
//
//****************************************************************************
#include <string.h>
#include "rtos.h"		// ������غ���
#include <xm_user.h>
#include <xm_base.h>
#include "app_voice.h"
#include <xm_semaphore.h>
#include <xm_printf.h>
#include <xm_speech.h>
#include <xm_mci.h>
#include <xm_rom.h>
#include "app_menuid.h"
#include "app.h"
#include "rom.h"
#include <assert.h>
#include <xm_heap_malloc.h>
#include "xm_voice_prompts.h"
#include "xm_proj_define.h"

#define	XMSYS_VOICEPROMPTS_TASK_STACK_SIZE					0x2000
#define	XMSYS_VOICEPROMPTS_TASK_PRIORITY					200	

static OS_TASK TCB_VoicePromptsTask;
static OS_STACKPTR int StackVoicePromptsTask[XMSYS_VOICEPROMPTS_TASK_STACK_SIZE/4];          /* Task stacks */


#define	MAX_VOICE	8
// ������ʾ����
#define	PCM_BUFFER_SIZE	(80)

// ϵͳ����ID����
// ÿ��������Ӧ������(�����������ǲ�������)
static const char *voice_text[] = {
	/*	XM_VOICE_ID_BEEP_KEYBOARD,*/					"����Beep��",
	/* XM_VOICE_ID_ONEKEY_PHOTOGRAPH,*/					"�����ǲ���",
	/* XM_VOICE_ID_ALARM, */							"������",
	/* XM_VOICE_ID_STARTUP, */							"¼��������",

	"��Ч����ID",
};

static const APPROMRES VoiceID2ROM[XM_VOICE_ID_COUNT] = {
	{ROM_T18_VOICE_COMMON_XM_VOICE_ID_KEYBOARD_SPH, ROM_T18_VOICE_COMMON_XM_VOICE_ID_KEYBOARD_SPH_SIZE},
	{ROM_T18_VOICE_COMMON_XM_VOICE_ID_ONEKEY_PHOTOGRAPH_SPH, ROM_T18_VOICE_COMMON_XM_VOICE_ID_ONEKEY_PHOTOGRAPH_SPH_SIZE},
	{ROM_T18_VOICE_COMMON_XM_VOICE_ID_ALARM_SPH, ROM_T18_VOICE_COMMON_XM_VOICE_ID_ALARM_SPH_SIZE},
	{ROM_T18_VOICE_COMMON_XM_VOICE_ID_STARTUP_SPH, ROM_T18_VOICE_COMMON_XM_VOICE_ID_STARTUP_SPH_SIZE},

};


static unsigned int voice_array[MAX_VOICE];
static int voice_count;		// 
static int voice_index;		// ��һ����ʾ��������������

static unsigned int speech_id;	// ���ڲ��ŵ�����ID 

// ��������ź���
static const char *voice_prompts_sema_name = "_voice_prompts_sema_";
static void * voice_prompts_sema = NULL;
static const char *voice_prompts_count_sema_name = "_voice_prompts_count_sema_";
static void * voice_prompts_count_sema = NULL;

const APPROMRES *voice_id_2_rom_res (unsigned int voice_id)
{
	if(voice_id == 0 || voice_id > XM_VOICE_ID_COUNT)
		return NULL;
	else
		return &VoiceID2ROM[voice_id - 1];
}

// ��ȡ��ʾ�������ı���Ϣ
const char *XM_voice_prompts_get_text (unsigned int voice_id)
{
	if(voice_id == 0 || voice_id > XM_VOICE_ID_COUNT)
		return NULL;
	return voice_text[voice_id - 1];
}

// ����һ���µ���ʾ����������������
// 0		�ɹ�
// < 0	ʧ��
int XM_voice_prompts_insert_voice (unsigned int voice_id)
{
	// ��ȡ�����ź���
	int voice_free;	// ����λ��
	int i;
	int ret;
	int loop = 0;		// ����VC����������
	const char *text;

	if(voice_id == 0)
		return -1;

	if(speech_id && speech_id == voice_id)
		return 0;

	text = XM_voice_prompts_get_text (voice_id);
	if(text)
	{
		XM_printf ("voice (%s)\n", text);
	}
	
    #if 0  //eason
	if(AP_GetMenuItem (APPMENUITEM_VOICE_PROMPTS) == AP_SETTING_VOICE_PROMPTS_OFF)
		return -1;
    #endif
	
	// ��������
	if(XM_WaitSemaphore (voice_prompts_sema) == 0)
		return -1;

	ret = 0;
	do 
	{
		// ɨ���Ƿ��Ѵ�����ͬ������
		for (i = 0; i < voice_count; i ++)
		{
			if(voice_array[(voice_index + i) & (MAX_VOICE - 1)] == voice_id)
			{
				// ��ͬ����ƥ��
				break;
			}
		}

		if(i != voice_count)		// �ҵ�ƥ��
			break;

		// �������Ƿ�����
		if(voice_count < MAX_VOICE)
		{
			voice_free = (voice_index + voice_count) & (MAX_VOICE - 1);
			voice_array[voice_free] = voice_id;
			voice_count ++;
			// �ۼӼ����ź���
			XM_SignalSemaphore (voice_prompts_count_sema);
		}
		else
		{
			ret = -1;
		}
	} while (loop);

	// ����
	XM_SignalSemaphore (voice_prompts_sema);
	
	return ret;
}

// ɾ�����Ŷ�����ָ������ID����ʾ����
// 0		�ɹ�
// < 0	ʧ��
int XM_voice_prompts_remove_voice (unsigned int voice_id)
{
	int ret;
	int i;
	int loop = 0;		// ����VC����������
	if(voice_id == 0)
		return -1;
	// ��������
	if(XM_WaitSemaphore (voice_prompts_sema) == 0)
		return -1;

	ret = 0;
	do 
	{
		if(voice_count == 0)
			break;

		// ɨ���Ƿ��Ѵ�����ͬ������
		for (i = 0; i < voice_count; i ++)
		{
			if(voice_array[(voice_index + i) & (MAX_VOICE - 1)] == voice_id)
			{
				// ��ͬ����ƥ��
				break;
			}
		}

		if(i == voice_count)		// δ�ҵ�ƥ��
		{
			break;
		}

		for (; i < (voice_count - 1); i ++)
		{
			voice_array[(voice_index + i) & (MAX_VOICE - 1)] = voice_array[(voice_index + i + 1) & (MAX_VOICE - 1)];
		}
		voice_array[(voice_index + i) & (MAX_VOICE - 1)] = 0;

	} while (loop);

	// ����
	XM_SignalSemaphore (voice_prompts_sema);

	return ret;
}

// ɾ�����Ŷ����е�������ʾ����
int XM_voice_prompts_remove_all_voice (void)
{
	// ��������
	if(XM_WaitSemaphore (voice_prompts_sema) == 0)
		return -1;

	memset (voice_array, 0, sizeof(voice_array));
	voice_index = 0;
	voice_count = 0;

	// ����
	XM_SignalSemaphore (voice_prompts_sema);
	return 0;
}


static int voice_decode (char **stream_curr_pointer, char *decode_buffer)
{
	int ret;
	int size = 0;	// �ѽ�����������ֽڴ�С
	
	// ������������Packet�����͵������豸
	while(size != (PCM_BUFFER_SIZE*2*2))
	{
		ret =  speech_decode (	(char **)stream_curr_pointer, 
										(short *)(decode_buffer + size), 
										0x00002000|0x20000000);
		if(ret == 0)
		{
			// ������������ȷ
			size += PCM_BUFFER_SIZE*2;
			if(size == PCM_BUFFER_SIZE*2*2)
				break;
			continue;
		}
		else
		{
			// ����������������쳣
			break;
		}
	}
	return size;
}

int AP_VoicePromptsTask (void) 
{
	// �ȴ�����
	unsigned int voice_id;
	int ret;
	int size;
	XMWAVEFORMAT WaveFormat;
	char *stream_curr_pointer;
	char *stream_last_pointer;
	short	pcm_buffer[PCM_BUFFER_SIZE*4];	// ����2֡������, ÿ֡80��������

	const APPROMRES *AppRes;

	//XM_printf ("AP_VoicePromptsTask Enter\n");

	while (XM_WaitSemaphore (voice_prompts_count_sema))
	{
		if(XM_WaitSemaphore (voice_prompts_sema) == 0)
			break;

		//assert (voice_count > 0);

		// ��ȡ�����ŵ�����ID
		if(voice_count == 0)
		{
			XM_SignalSemaphore (voice_prompts_sema);
			continue;
		}
		voice_id = voice_array[voice_index];
		voice_array[voice_index] = 0;
		voice_index = (voice_index + 1) & (MAX_VOICE - 1);
		voice_count --;

		XM_SignalSemaphore (voice_prompts_sema); 

		if (voice_id == 0)
			continue;

		// ��ȡ�������ݵ�����ƫ�Ƽ��ֽڳ���
		AppRes = voice_id_2_rom_res (voice_id);
		if(AppRes == 0)
			continue;

		// ��WAVE�����豸
		// ����������ʽ (��������8K������16Bitģʽ)
		WaveFormat.wChannels = 1;
		WaveFormat.wBitsPerSample = 16;
		WaveFormat.dwSamplesPerSec = 8000;
		ret = XM_WaveOpen (XMMCIDEVICE_PLAY, &WaveFormat);
		if(ret < 0)
		{
			XM_printf ("XM_WaveOpen NG, VoicePromptsTask end\n");
			continue;
		}

		speech_id = voice_id;

		// ��λ����������
		#if ROM_SPI
		unsigned int address;
		char *buffer;
		
		address = (unsigned int)XM_RomAddress (AppRes->rom_offset);
		if(address == NULL)
			return -1;
		
		buffer = (char *)XM_heap_malloc (AppRes->res_length );
		if(buffer == NULL)
			return -1;

		if(Spi_Read ((unsigned int)address, buffer, AppRes->res_length) == 0)
		{
			XM_heap_free (buffer);
			return -1;
		}
		stream_curr_pointer = buffer;
		#else
		stream_curr_pointer = XM_RomAddress(AppRes->rom_offset);
		#endif
		
		if(stream_curr_pointer == NULL)
		{
			continue;
		}
		stream_last_pointer = stream_curr_pointer + AppRes->res_length;
		// ֪ͨ������������ԭʼ�����������ݻ�������ַ���������ֽڴ�С
		speech_init ((char *)stream_curr_pointer, stream_last_pointer - stream_curr_pointer);
	
		// ��������ֱ����������/�������
		while (stream_curr_pointer !=  stream_last_pointer)
		{
			// ��������
			size = voice_decode (&stream_curr_pointer, (char *)pcm_buffer);
			if(size == 0)
			{
				// ��������������쳣
				break;
			}

			// ��������
			ret = XM_WaveWrite (XMMCIDEVICE_PLAY, (WORD *)pcm_buffer, size/2);
			if(ret != 0)
			{
				// ��������ʧ��
				break;
			}
		}

		#if ROM_SPI
		XM_heap_free (buffer);
		#endif
		
		// ������ϻ��쳣
	
		// �ر���Ƶ�豸
		XM_WaveClose (XMMCIDEVICE_PLAY);

		speech_id = 0;

		// �ӳ�200����
		XM_Sleep (200);
	}

	//XM_printf ("AP_VoicePromptsTask Leave\n");

	return 0;
}

void  XMSYS_VoicePromptsTask (void)
{
	AP_VoicePromptsTask ();

	//XM_voice_prompts_exit ();

	// ��ֹ��ǰ����
	OS_Terminate (NULL);
}



// ������ʾ�����ʼ��
int XM_voice_prompts_init (void)
{
	// ������������ź���
	voice_count = 0;
	voice_index = 0;
	memset (voice_array, 0, sizeof(voice_array));

	speech_id = 0;

	voice_prompts_sema = XM_CreateSemaphore (voice_prompts_sema_name, 1);
	if(voice_prompts_sema == NULL)
	{
		XM_printf ("AP_VoicePromptsInit NG, create semaphore %s failed\n", voice_prompts_sema_name);
		return -1;
	}

	// �����������ź���
	voice_prompts_count_sema = XM_CreateSemaphore (voice_prompts_count_sema_name, 0);
	if(voice_prompts_count_sema == NULL)
	{
		// �ر��źŵ�
		XM_CloseSemaphore (voice_prompts_sema);
		// ɾ���źŵ�
		XM_DeleteSemaphore (voice_prompts_sema);
		voice_prompts_sema = NULL;
		XM_printf ("AP_VoicePromptsInit NG, create semaphore %s failed\n", voice_prompts_count_sema_name);
		return -1;
	}

	OS_CREATETASK(&TCB_VoicePromptsTask, "VoicePromptsTask", XMSYS_VoicePromptsTask, XMSYS_VOICEPROMPTS_TASK_PRIORITY, StackVoicePromptsTask);
	
	return 0;
}

// ������ʾ�������
int XM_voice_prompts_exit (void)
{
	if(XM_WaitSemaphore (voice_prompts_sema) == 0)
		return -1;


	if(voice_prompts_count_sema)
	{
		// �ر��źŵ�
		XM_CloseSemaphore (voice_prompts_count_sema);
		// ɾ���źŵ�
		XM_DeleteSemaphore (voice_prompts_count_sema);
		voice_prompts_count_sema = NULL;
	}

	if(voice_prompts_sema)
	{
		// �ر��źŵ�
		XM_CloseSemaphore (voice_prompts_sema);
		// ɾ���źŵ�
		XM_DeleteSemaphore (voice_prompts_sema);
		voice_prompts_sema = NULL;
	}

	return 0;

}

