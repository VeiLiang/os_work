#include "hardware.h"
#include "RTOS.h"		// OS头文件
#include "FS.h"
#include <xm_type.h>
#include <xm_queue.h>
#include <assert.h>
#include <string.h>
#include <xm_printf.h>
#include <xm_key.h>
#include <xm_user.h>
#include <xm_file.h>
#include "xm_core.h"
#include "xm_power.h"
#include "xm_h264_cache_file.h"
#include "xm_message_socket.h"
#include "arkn141_codec.h"
#include "arkn141_jpeg_codec.h"
#include "xm_uvc_socket.h"
#include "xm_video_task.h"
#include <xm_videoitem.h>
#include "xm_app_menudata.h"


static int XM_ImageItemGetInfo (char *image_name, unsigned int *image_time);


#define	JPEGCODEC_TIMEOUT					2000	//2000毫秒
#define	XMSYS_ONE_KEY_PHOTOGRAPH_EVENT		0x01

static OS_TASK TCB_OneKeyPhotographTask;
__no_init static OS_STACKPTR int StackOneKeyPhotographTask[XMSYS_ONE_KEY_PHOTOGRAPH_TASK_STACK_SIZE/4];/* Task stacks */

static OS_RSEMA jpegcodec_sema;			// 互斥保护
static OS_MAILBOX jpegcodec_mailbox;		// 一字节邮箱。
static char jpegcodec_mailbuf;					// 一个字节的邮箱

static char photograph_file[64];		// 解码显示的照片文件名
static unsigned char takephoto_flag;

// codec返回应答码给服务请求者
static void jpegcodec_reply_response_message (char response_code)
{
	// 清除信箱的内容，防止阻塞
	OS_Use(&jpegcodec_sema);
	OS_ClearMB (&jpegcodec_mailbox);
	OS_PutMail1 (&jpegcodec_mailbox, &response_code);
	OS_Unuse(&jpegcodec_sema);
}

// 清除缓存的应答消息
static void jpegcodec_clear_response_message (void)
{
	OS_Use(&jpegcodec_sema);
	OS_ClearMB (&jpegcodec_mailbox);
	OS_Unuse(&jpegcodec_sema);
}

// 将编码后的JPEG码流保存到文件
static int save_photo (unsigned char channel, char *jpeg_buffer, int jpeg_length)
{
	int r;
	char Filename[VIDEOITEM_MAX_FULL_PATH_NAME];
	XMSYSTEMTIME create_time;
	
	XM_GetLocalTime (&create_time);
	
	// 20180609 使用新的VideoItem API(XM_VideoItemCreateVideoItemHandleEx)获得照片的全路径名
	// 检查一键拍照目录是否存在
	memset (Filename, 0, sizeof(Filename));
	do
	{
		HANDLE hVideoItem = XM_VideoItemCreateVideoItemHandleEx (channel, &create_time, XM_FILE_TYPE_PHOTO, XM_VIDEOITEM_CLASS_NORMAL, XM_VIDEOITEM_VOLUME_0);
		if(hVideoItem)
		{
			XMVIDEOITEM *pVideoItem = AP_VideoItemGetVideoItemFromHandle (hVideoItem);
			if(pVideoItem)
			{
				XM_VideoItemGetVideoFilePath(pVideoItem, channel, Filename, sizeof(Filename));
			}
		}	
	} while (0);	
	if(Filename[0] == '\0')
	{
		XM_printf ("can't open file to save photo\n");
		return -1;
	}

	XM_printf(">>>>>save_photo, Filename:%s\r\n", Filename);
	// 锁定, 阻止de-mount
	r = -1;
	if( XMSYS_file_system_block_write_access_lock () == 0 )
	//int lock_ret = XMSYS_file_system_block_write_access_lock_timeout (150);
	// 文件删除时会阻塞XMSYS_file_system_block_write_access_lock_timeout，而文件删除(后台线程)又与Cache文件系统抢夺文件系统控制权，
	// 等待时间加长，减少系统忙时锁定失败的可能性
#if 0
	int lock_ret = XMSYS_file_system_block_write_access_lock_timeout (1000);
	if(lock_ret == (-2))	// 超时
	{
		// 锁定失败, 超时
		XM_printf ("onekey photo (%s) lock timeout\n", Filename);
		return -1;
	}
	else if(lock_ret == 0)
#endif
	{
		FS_FILE *pFile;
		pFile = FS_FOpen(Filename, "wb");
		if(pFile)
		{
			// 20180609 取消64KB对其
			int size = jpeg_length; 
			U32 ret = FS_Write (pFile, (void *)jpeg_buffer, size);
			
			FS_FClose (pFile);
			FS_CACHE_Clean (Filename);	
			//XM_printf ("JPEG end\n");
			if(ret == size)
			{
				r = 0;
			}
			else
			{
				// 通知主机，一键拍照异常
				XM_printf ("onekey photo (%s) NG\n", Filename);
				r = -1;
			}
		}
		else
		{
			// 通知主机，一键拍照异常
			XM_printf ("onekey photo (%s) NG\n", Filename);
			r = -1;
		}
		
	} 
	XMSYS_file_system_block_write_access_unlock ();
	
	if(r == 0)
	{
		XM_VideoItemUpdateVideoItemFromFileName (Filename, jpeg_length, &create_time);
		
		// 通知主机, 新增文件
		unsigned int create_time = 0;
		XM_printf ("onekey photo (%s) OK\n", Filename);
		XM_ImageItemGetInfo (Filename, &create_time);
		XMSYS_UvcSocketFileListUpdate (channel, 1, (unsigned char *)Filename, create_time, 0, 1);
	}
	
	return r;
}

void XMSYS_OneKeyPhotographTask (void)
{
	OS_U8 os_event;

	unsigned char channel;
	unsigned int channel_count;
	char *jpeg_buffer;
	int jpeg_length;
	int ret;
	unsigned char cnt;
	
	//XM_printf ("XMSYS_OneKeyPhotographTask start\n");
	takephoto_flag  = FALSE;
	while(1)
	{
		// 等待外部事件
		os_event = OS_WaitEvent(XMSYS_ONE_KEY_PHOTOGRAPH_EVENT);

		XM_printf(">>>>>>>XMSYS_OneKeyPhotographTask, os_event:%d\r\n", os_event);
		if(os_event & XMSYS_ONE_KEY_PHOTOGRAPH_EVENT)
		{			
			// 一键拍照		
			XMSYSSENSORPACKET *packet;
			DWORD start_ticket;
			unsigned int w, h;

			for(cnt=0; cnt<2; cnt++)//默认拍照2个通道图片
			{
				channel = cnt;
				
				#if 0 //解决拍照只拍前路的问题
				if(	XMSYS_VideoGetImageAssemblyMode() != XMSYS_ASSEMBLY_MODE_FRONT_ONLY && XMSYS_VideoGetImageAssemblyMode() != XMSYS_ASSEMBLY_MODE_REAL_ONLY)
					return;

				if(XMSYS_VideoGetImageAssemblyMode() == XMSYS_ASSEMBLY_MODE_FRONT_ONLY)
					channel = 0;
				else if(XMSYS_VideoGetImageAssemblyMode() == XMSYS_ASSEMBLY_MODE_REAL_ONLY)
					channel = 1;
				#endif
				
				XMSYS_SensorGetChannelSize(channel, &w, &h);
								
				jpeg_buffer = NULL;
				packet = NULL;
				ret = 0;
				do 
				{
					// ACC下电时拒绝照片写入
					if(xm_power_check_acc_safe_or_not() == 0)
					{
						ret = -1;
						XM_printf ("photo failed, bad acc\n");
						break;
					}
					
					// 分配用于保存JPEG码流的缓冲区
					jpeg_buffer = kernel_malloc (w * h * 3/8);
					if(jpeg_buffer == NULL)
					{
						ret = -1;
						XM_printf ("onekey photo failed, memory busy\n");
						break;
					}
					jpeg_length = w * h * 3/8;
					
					start_ticket = XM_GetTickCount ();
					
					// 获取sensor帧
					while(packet == NULL)
					{
						packet = XMSYS_SensorCreatePacket(channel);
						if(packet)
							break;
							
						// 判断超时
						OS_Delay (1);
						if( (XM_GetTickCount() - start_ticket) > 150 )
						{
							// timeout
							ret = -1;
							XM_printf ("onekey photo failed, sensor busy\n");
							break;
						}
					}
#if 1
		if(packet && AP_GetMenuItem(APPMENUITEM_TIME_STAMP) == AP_SETTING_VIDEO_TIMESTAMP_ON)
		{
			void embed_local_time_into_video_frame (
																		 unsigned int width,	// 视频宽度
																		 unsigned int height,	// 视频高度
																		 unsigned int stride,	// Y_UV行字节长度
																		 char *data				// Y_UV缓冲区
																		);
					
			unsigned int vaddr = page_addr_to_vaddr((unsigned int)packet->buffer);
			unsigned char *virtual_address = (unsigned char *)vaddr;
			// 嵌入时间戳信息
			// 使用虚地址(Cache)优化算法
			embed_local_time_into_video_frame (packet->width, packet->height, 
																  packet->width, 
																  //packet->buffer
																  (char *)virtual_address
																);		
					
		}
#endif


					
				} while (0);
				
				if(jpeg_buffer && packet)
				{
					// JPEG编码
					if(arkn141_codec_open(ARKN141_CODEC_TYPE_JPEG_ENCODER) == 0)
					{
						char qLevel = 5;
						
						if(AP_GetMenuItem(APPMENU_IMAGE_QUALITY) == AP_SETTING_VIDEO_QUALITY_HIGH)
							qLevel = 9;
						else if(AP_GetMenuItem(APPMENU_IMAGE_QUALITY) == AP_SETTING_VIDEO_QUALITY_GOOD)
							qLevel = 7;
						
						ret = arkn141_jpeg_encode(packet->buffer, 
												packet->buffer + packet->width * packet->height,
												NULL,
												ARKN141_YUV_FRAME_Y_UV420,
												qLevel,
												packet->width, 
												packet->height,
												(unsigned char *)jpeg_buffer, 
												&jpeg_length);
						if(ret != 0)
						{
							XM_printf ("onekey_photo failed, jarkn141_jpeg_encode NG, ret=%d\n", ret);
						}
						
						arkn141_codec_close (ARKN141_CODEC_TYPE_JPEG_ENCODER);
					}
					else
					{
						XM_printf ("onekey_photo failed, arkn141_codec_open NG\n");
						ret = -1;
					}
				}
				
				// 释放Sensor图像
				if(packet)
				{
					// 重新回收sensor帧
					XMSYS_SensorRecyclePacket(channel, packet);
					packet = NULL;
				}
				
				if(ret == 0 && jpeg_buffer && jpeg_length)
				{
					ret = save_photo(channel, jpeg_buffer, jpeg_length);//保存图片
				}

				if(jpeg_buffer)
				{
					kernel_free (jpeg_buffer);
					jpeg_buffer = NULL;
				}
			}
			// 应答请求线程 成功
			jpegcodec_reply_response_message( (ret == 0) ? 0 : (-1) );		
			XM_printf(">>>>>>>>>photo ok..........\r\n");
		}
	}
}


// 一键拍照任务初始化
void XMSYS_OneKeyPhotographInit (void)
{
	// 初始化Motion JPEG硬件	
	OS_CREATEMB(&jpegcodec_mailbox, 1, 1, &jpegcodec_mailbuf);	
	OS_CREATERSEMA(&jpegcodec_sema); /* Creates resource semaphore */
	
	OS_CREATETASK(&TCB_OneKeyPhotographTask, "OneKeyPhoto", XMSYS_OneKeyPhotographTask, XMSYS_ONE_KEY_PHOTOGRAPH_TASK_PRIORITY, StackOneKeyPhotographTask);
}

// 一键拍照任务结束
void XMSYS_OneKeyPhotographExit (void)
{
	
}


// 启动一键拍照功能
// -2 命令执行超时
// -1 命令非法或命令执行失败
// 0  命令已成功执行
// 暂时未完整实现，此处仅简单实现
int XMSYS_JpegCodecOneKeyPhotograph (void)
{
	char response_code;
	int ret = -1;
	
	// 强制清除缓存的信箱信息，避免阻塞
	jpegcodec_clear_response_message ();

	takephoto_flag = TRUE;
	OS_SignalEvent(XMSYS_ONE_KEY_PHOTOGRAPH_EVENT, &TCB_OneKeyPhotographTask);

	OS_Delay(1000);
	// 等待JPEG Codec线程应答
	if(OS_GetMailTimed(&jpegcodec_mailbox, &response_code, JPEGCODEC_TIMEOUT) == 0)
	{
		// codec已应答，获得消息
		if(response_code == 0)
			ret = 0;
		else
			ret = -1;
		//ret = (int)response_code;
		XM_printf(">>>>>>>>>>>>>>>>>>>>>>>>>>OneKeyPhotograph resp=%d\n", ret);
	}
	else
	{
		// 超时
		XM_printf (">>>>>>>>>>>>>>>>>>>>>>>>>>OneKeyPhotograph timeout\n");
		ret = -2;
	}	
	
	takephoto_flag = FALSE;
	return ret;
}

static int XM_ImageItemGetInfo (char *image_full_path_name, unsigned int *image_time)
{
	DWORD dwCreateTime = 0;
	U32 TimeStamp;
	FS_FILETIME FileTime;
	
	if(image_full_path_name == NULL || *image_full_path_name == '\0')
		return -1;
	
	if(FS_GetFileTime (image_full_path_name, &TimeStamp) != 0)
		return -1;
	FS_TimeStampToFileTime (TimeStamp, &FileTime);
	dwCreateTime  = FileTime.Second & 0x3F;
	dwCreateTime |= (FileTime.Minute & 0x3F) << 6;
	dwCreateTime |= (FileTime.Hour & 0x1F) << 12;
	dwCreateTime |= (FileTime.Day & 0x1F) << 17;
	dwCreateTime |= (FileTime.Month & 0x0F) << 22;
	dwCreateTime |= ((FileTime.Year - 1980 + 2010) & 0x1F) << 26;
	*image_time = dwCreateTime;
	return 0;
}

unsigned char get_takephoto_flag(void)
{
	return takephoto_flag;
}
