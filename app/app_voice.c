//****************************************************************************
//
//	Copyright (C) 2012 ZhuoYongHong
//
//	Author	ZhuoYongHong
//
//	File name: app_voice.c
//	  语言提示功能
//
//	Revision history
//
//		2012.09.06	ZhuoYongHong Initial version
//
//****************************************************************************
#include <string.h>
#include "rtos.h"		// 任务相关函数
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
// 语音提示任务
#define	PCM_BUFFER_SIZE	(80)

// 系统语音ID定义
// 每个语音对应的文字(按键及拍照咔擦声例外)
static const char *voice_text[] = {
	/*	XM_VOICE_ID_BEEP_KEYBOARD,*/					"按键Beep音",
	/* XM_VOICE_ID_ONEKEY_PHOTOGRAPH,*/					"拍照咔擦声",
	/* XM_VOICE_ID_ALARM, */							"报警声",
	/* XM_VOICE_ID_STARTUP, */							"录像启动声",

	"无效语音ID",
};

static const APPROMRES VoiceID2ROM[XM_VOICE_ID_COUNT] = {
	{ROM_T18_VOICE_COMMON_XM_VOICE_ID_KEYBOARD_SPH, ROM_T18_VOICE_COMMON_XM_VOICE_ID_KEYBOARD_SPH_SIZE},
	{ROM_T18_VOICE_COMMON_XM_VOICE_ID_ONEKEY_PHOTOGRAPH_SPH, ROM_T18_VOICE_COMMON_XM_VOICE_ID_ONEKEY_PHOTOGRAPH_SPH_SIZE},
	{ROM_T18_VOICE_COMMON_XM_VOICE_ID_ALARM_SPH, ROM_T18_VOICE_COMMON_XM_VOICE_ID_ALARM_SPH_SIZE},
	{ROM_T18_VOICE_COMMON_XM_VOICE_ID_STARTUP_SPH, ROM_T18_VOICE_COMMON_XM_VOICE_ID_STARTUP_SPH_SIZE},

};


static unsigned int voice_array[MAX_VOICE];
static int voice_count;		// 
static int voice_index;		// 第一个提示语音的数组索引

static unsigned int speech_id;	// 正在播放的语音ID 

// 互斥访问信号量
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

// 获取提示语音的文本信息
const char *XM_voice_prompts_get_text (unsigned int voice_id)
{
	if(voice_id == 0 || voice_id > XM_VOICE_ID_COUNT)
		return NULL;
	return voice_text[voice_id - 1];
}

// 插入一个新的提示语音到语音播放器
// 0		成功
// < 0	失败
int XM_voice_prompts_insert_voice (unsigned int voice_id)
{
	// 获取互斥信号量
	int voice_free;	// 空闲位置
	int i;
	int ret;
	int loop = 0;		// 避免VC编译器报警
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
	
	// 互斥锁定
	if(XM_WaitSemaphore (voice_prompts_sema) == 0)
		return -1;

	ret = 0;
	do 
	{
		// 扫描是否已存在相同的语音
		for (i = 0; i < voice_count; i ++)
		{
			if(voice_array[(voice_index + i) & (MAX_VOICE - 1)] == voice_id)
			{
				// 相同语音匹配
				break;
			}
		}

		if(i != voice_count)		// 找到匹配
			break;

		// 检查队列是否已满
		if(voice_count < MAX_VOICE)
		{
			voice_free = (voice_index + voice_count) & (MAX_VOICE - 1);
			voice_array[voice_free] = voice_id;
			voice_count ++;
			// 累加计数信号量
			XM_SignalSemaphore (voice_prompts_count_sema);
		}
		else
		{
			ret = -1;
		}
	} while (loop);

	// 解锁
	XM_SignalSemaphore (voice_prompts_sema);
	
	return ret;
}

// 删除播放队列中指定语音ID的提示语音
// 0		成功
// < 0	失败
int XM_voice_prompts_remove_voice (unsigned int voice_id)
{
	int ret;
	int i;
	int loop = 0;		// 避免VC编译器报警
	if(voice_id == 0)
		return -1;
	// 互斥锁定
	if(XM_WaitSemaphore (voice_prompts_sema) == 0)
		return -1;

	ret = 0;
	do 
	{
		if(voice_count == 0)
			break;

		// 扫描是否已存在相同的语音
		for (i = 0; i < voice_count; i ++)
		{
			if(voice_array[(voice_index + i) & (MAX_VOICE - 1)] == voice_id)
			{
				// 相同语音匹配
				break;
			}
		}

		if(i == voice_count)		// 未找到匹配
		{
			break;
		}

		for (; i < (voice_count - 1); i ++)
		{
			voice_array[(voice_index + i) & (MAX_VOICE - 1)] = voice_array[(voice_index + i + 1) & (MAX_VOICE - 1)];
		}
		voice_array[(voice_index + i) & (MAX_VOICE - 1)] = 0;

	} while (loop);

	// 解锁
	XM_SignalSemaphore (voice_prompts_sema);

	return ret;
}

// 删除播放队列中的所有提示语音
int XM_voice_prompts_remove_all_voice (void)
{
	// 互斥锁定
	if(XM_WaitSemaphore (voice_prompts_sema) == 0)
		return -1;

	memset (voice_array, 0, sizeof(voice_array));
	voice_index = 0;
	voice_count = 0;

	// 解锁
	XM_SignalSemaphore (voice_prompts_sema);
	return 0;
}


static int voice_decode (char **stream_curr_pointer, char *decode_buffer)
{
	int ret;
	int size = 0;	// 已解码的语音包字节大小
	
	// 解码两个语音Packet并发送到播放设备
	while(size != (PCM_BUFFER_SIZE*2*2))
	{
		ret =  speech_decode (	(char **)stream_curr_pointer, 
										(short *)(decode_buffer + size), 
										0x00002000|0x20000000);
		if(ret == 0)
		{
			// 语音包解码正确
			size += PCM_BUFFER_SIZE*2;
			if(size == PCM_BUFFER_SIZE*2*2)
				break;
			continue;
		}
		else
		{
			// 语音包解码结束或异常
			break;
		}
	}
	return size;
}

int AP_VoicePromptsTask (void) 
{
	// 等待语音
	unsigned int voice_id;
	int ret;
	int size;
	XMWAVEFORMAT WaveFormat;
	char *stream_curr_pointer;
	char *stream_last_pointer;
	short	pcm_buffer[PCM_BUFFER_SIZE*4];	// 保存2帧的数据, 每帧80个采样点

	const APPROMRES *AppRes;

	//XM_printf ("AP_VoicePromptsTask Enter\n");

	while (XM_WaitSemaphore (voice_prompts_count_sema))
	{
		if(XM_WaitSemaphore (voice_prompts_sema) == 0)
			break;

		//assert (voice_count > 0);

		// 获取待播放的语音ID
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

		// 获取语音数据的数据偏移及字节长度
		AppRes = voice_id_2_rom_res (voice_id);
		if(AppRes == 0)
			continue;

		// 打开WAVE播放设备
		// 设置语音格式 (单声道、8K采样、16Bit模式)
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

		// 复位语音解码器
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
		// 通知语音解码器，原始语音流的数据缓冲区地址及语音流字节大小
		speech_init ((char *)stream_curr_pointer, stream_last_pointer - stream_curr_pointer);
	
		// 播放语音直到语音解码/播放完毕
		while (stream_curr_pointer !=  stream_last_pointer)
		{
			// 语音解码
			size = voice_decode (&stream_curr_pointer, (char *)pcm_buffer);
			if(size == 0)
			{
				// 语音解码结束或异常
				break;
			}

			// 语音播放
			ret = XM_WaveWrite (XMMCIDEVICE_PLAY, (WORD *)pcm_buffer, size/2);
			if(ret != 0)
			{
				// 语音播放失败
				break;
			}
		}

		#if ROM_SPI
		XM_heap_free (buffer);
		#endif
		
		// 播放完毕或异常
	
		// 关闭音频设备
		XM_WaveClose (XMMCIDEVICE_PLAY);

		speech_id = 0;

		// 延迟200毫秒
		XM_Sleep (200);
	}

	//XM_printf ("AP_VoicePromptsTask Leave\n");

	return 0;
}

void  XMSYS_VoicePromptsTask (void)
{
	AP_VoicePromptsTask ();

	//XM_voice_prompts_exit ();

	// 终止当前任务
	OS_Terminate (NULL);
}



// 语音提示任务初始化
int XM_voice_prompts_init (void)
{
	// 创建互斥访问信号量
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

	// 创建计数器信号量
	voice_prompts_count_sema = XM_CreateSemaphore (voice_prompts_count_sema_name, 0);
	if(voice_prompts_count_sema == NULL)
	{
		// 关闭信号灯
		XM_CloseSemaphore (voice_prompts_sema);
		// 删除信号灯
		XM_DeleteSemaphore (voice_prompts_sema);
		voice_prompts_sema = NULL;
		XM_printf ("AP_VoicePromptsInit NG, create semaphore %s failed\n", voice_prompts_count_sema_name);
		return -1;
	}

	OS_CREATETASK(&TCB_VoicePromptsTask, "VoicePromptsTask", XMSYS_VoicePromptsTask, XMSYS_VOICEPROMPTS_TASK_PRIORITY, StackVoicePromptsTask);
	
	return 0;
}

// 语音提示任务结束
int XM_voice_prompts_exit (void)
{
	if(XM_WaitSemaphore (voice_prompts_sema) == 0)
		return -1;


	if(voice_prompts_count_sema)
	{
		// 关闭信号灯
		XM_CloseSemaphore (voice_prompts_count_sema);
		// 删除信号灯
		XM_DeleteSemaphore (voice_prompts_count_sema);
		voice_prompts_count_sema = NULL;
	}

	if(voice_prompts_sema)
	{
		// 关闭信号灯
		XM_CloseSemaphore (voice_prompts_sema);
		// 删除信号灯
		XM_DeleteSemaphore (voice_prompts_sema);
		voice_prompts_sema = NULL;
	}

	return 0;

}

