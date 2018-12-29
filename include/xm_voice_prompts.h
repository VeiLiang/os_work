#ifndef _XM_VOICE_PROMPTS_H_
#define _XM_VOICE_PROMPTS_H_

#if defined (__cplusplus)
	extern "C"{
#endif

// 系统语音ID定义
enum {
	XM_VOICE_ID_BEEP_KEYBOARD = 1,			// 按键Beep音
	XM_VOICE_ID_ONEKEY_PHOTOGRAPH,			// 拍照咔擦声
	XM_VOICE_ID_ALARM,							// 报警声
	XM_VOICE_ID_STARTUP,							// 录像启动声

	
	XM_VOICE_ID_COUNT,


};

// 语音提示任务初始化
// 0		成功
// < 0	失败
int XM_voice_prompts_init (void);

// 语音提示任务结束
// 0		成功
// < 0	失败
int XM_voice_prompts_exit (void);

// 插入一个新的提示语音到语音播放器
// 0		成功
// < 0	失败
int XM_voice_prompts_insert_voice (unsigned int voice_id);

// 删除播放队列中指定语音ID的提示语音
// 0		成功
// < 0	失败
int XM_voice_prompts_remove_voice (unsigned int voice_id);

// 删除播放队列中的所有提示语音
// 0		成功
// < 0	失败
int XM_voice_prompts_remove_all_voice (void);


// 获取提示语音的文本信息
const char *XM_voice_prompts_get_text (unsigned int voice_id);

#if defined (__cplusplus)
	}
#endif		/* end of __cplusplus */

#endif	// _XM_VOICE_PROMPTS_H_
