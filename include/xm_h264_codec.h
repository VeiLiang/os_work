#ifndef _XM_H264_CODEC_
#define _XM_H264_CODEC_

#if defined (__cplusplus)
	extern "C"{
#endif
	
// AVI编码使用的视频格式
// video format		
enum {
	ARKN141_VIDEO_FORMAT_1080P_30 = 0,
	ARKN141_VIDEO_FORMAT_720P_30,
#if HONGJING_CVBS
	ARKN141_VIDEO_FORMAT_720X480P,
	ARKN141_VIDEO_FORMAT_640X480P,
	ARKN141_VIDEO_FORMAT_320X240P,
#else	
	ARKN141_VIDEO_FORMAT_720P_60,
#endif
	ARKN141_VIDEO_FORMAT_COUNT
};
		
// H264 Codec工作模式定义
#define	XMSYS_H264_CODEC_MODE_IDLE					0		// H264 Codec空闲模式
#define	XMSYS_H264_CODEC_MODE_RECORD				1		// H264 Codec录像记录模式
#define	XMSYS_H264_CODEC_MODE_PLAY					2		// H264 Codec录像回放模式
		
// H264 Codec工作状态定义
#define	XMSYS_H264_CODEC_STATE_STOP					0		// H264停止状态
#define	XMSYS_H264_CODEC_STATE_WORK					1		// H264工作状态
#define	XMSYS_H264_CODEC_STATE_PAUSE				2		// H264暂停状态(仅录像回放具有)

// 视频播放模式
#define	XM_VIDEO_PLAYBACK_MODE_NORMAL				0		// 正常播放模式 (解码所有I帧及P帧并显示)
#define	XM_VIDEO_PLAYBACK_MODE_FORWARD				1		// 快进播放模式 (仅解码I帧，从头到尾顺序显示)
#define	XM_VIDEO_PLAYBACK_MODE_BACKWARD				2		// 快退播放模式 (仅解码I帧，从尾到头顺序显示)
#define	XM_VIDEO_PLAYBACK_MODE_FRAME				3		// 逐帧播放模式 (每次播放单帧, 等待按键继续下一帧)

// 获取当前H264 Codec的工作模式
int XMSYS_H264CodecGetMode (void);

// 获取H264 Codec当前的工作状态
int XMSYS_H264CodecGetState (void);

// 启动视频录像
// -2 命令执行超时
// -1 命令非法或命令执行失败
// 0  命令已成功执行
int XMSYS_H264CodecRecorderStart (void);

// 停止录像记录
// -2 命令执行超时
// -1 命令非法或命令执行失败
// 0  命令已成功执行
int XMSYS_H264CodecRecorderStop  (void);

// 切换摄像头通道
// 返回值
// -2 命令执行超时
// -1 命令非法或命令执行失败
// 0  命令已成功执行
int XMSYS_H264CodecRecorderSwitchChannel  (unsigned int camera_channel);

// 一键保护
// 1）录像中 将当前录制的视频加锁，并将记录长度延长3分钟。
// 2）回放中 将当前回放的视频加锁
// 返回值
// -2 命令执行超时
// -1 命令非法或命令执行失败
// 0  命令已成功执行
int XMSYS_H264CodecOneKeyProtect (void);


// 播放指定的录像记录
// lpVideoFile			回放视频文件的全路径名
// playback_mode		视频播放模式
// start_point			播放起始点 (毫秒单位)							
// -2 命令执行超时
// -1 命令非法或命令执行失败
// 0  命令已成功执行
int XMSYS_H264CodecPlayerStart (const char *lpVideoFile, unsigned int playback_mode, unsigned int start_point);

// 停止“视频播放”
// -2 命令执行超时
// -1 命令非法或命令执行失败
// 0  命令已成功执行
int XMSYS_H264CodecPlayerStop (void);

// 暂停当前播放播放
// -2 命令执行超时
// -1 命令非法或命令执行失败
// 0  命令已成功执行
int XMSYS_H264CodecPlayerPause (void);

// 恢复播放已暂停的录像记录
void XMSYS_H264CodecPlayerResume (void);

// 启动快进播放
// -2 命令执行超时
// -1 命令非法或命令执行失败
// 0  命令已成功执行
int XMSYS_H264CodecPlayerForward (void);

// 启动快退播放
// -2 命令执行超时
// -1 命令非法或命令执行失败
// 0  命令已成功执行
int XMSYS_H264CodecPlayerBackward (void);

// 启动一键拍照功能
// -2 命令执行超时
// -1 命令非法或命令执行失败
// 0  命令已成功执行
int XMSYS_JpegCodecOneKeyPhotograph (void);

// 逐帧播放
int XMSYS_H264CodecPlayerFrame (void);

// 停止视频编解码器
// -2 命令执行超时
// -1 命令非法或命令执行失败
// 0  命令已成功执行
int XMSYS_H264CodecStop (void);

// 定位到特定时间位置(time_to_play, 单位秒)
// -2 命令执行超时
// -1 命令非法或命令执行失败
// 0  命令已成功执行
int XMSYS_H264CodecPlayerSeek (unsigned int time_to_play);

// 设置AVI编码使用的视频格式
int XMSYS_H264CodecSetVideoFormat (int video_format);

// 获取AVI编码使用的视频格式
int XMSYS_H264CodecGetVideoFormat (void);

// 设置H264编码质量
int XMSYS_H264CodecSetVideoQuality (unsigned int vide_quality);
		
// 检查H264编解码是否忙
// 1 busy
// 0 idle
int XMSYS_H264CodecCheckBusy (void);

// 获取AVI视频的第一帧图像
// 返回值
//	0		成功
// -1		失败
// 仅支持Y_UV420输出
int  h264_decode_avi_stream_thumb_frame (	const char *filename, 				// AVI视频源文件
														unsigned int thumb_width, 			// 缩放尺寸定义
														unsigned int thumb_height, 
														unsigned int thumb_stride,
														unsigned char *thumb_image[3]		// thumb_image[0]	Y
																									// thumb_image[1]	U/UV
																									// thumb_image[2] V
														);

#if defined (__cplusplus)
	}
#endif		/* end of __cplusplus */
		
#endif	// _XM_H264_CODEC_