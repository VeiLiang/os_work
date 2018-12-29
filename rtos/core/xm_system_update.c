#include "hardware.h"
#include "xm_flash_space_define.h"		// Flash空间分配定义
#include "xm_core.h"
#include <xm_type.h>
#include <xm_dev.h>
#include <fs.h>
#include <assert.h>
#include <xm_queue.h>
#include "fevent.h"
#include "xm_systemupdate.h"
#include "xm_message_socket.h"
#include "crc16.h"
#include <xm_printf.h>
#include "ssi.h"
#include "xm_key.h"
#include "xm_user.h"
#include "xm_uvc_socket.h"
#include "xm_autotest.h"
#include "xm_core.h"

extern INT32 IsCardExist(UINT32 ulCardID);


#define URTOS_ADDR	XM_FLASH_RTOS_BASE	// 从SPI Flash的4KB地址偏移开始保存 RTOS
#define	PAGE_SIZE	0x1000


#define	XMSYS_SYSTEM_UPDATE_PACKAGE_SIZE		XM_FLASH_RTOS_SIZE			// 最大系统升级包字节大小

static OS_RSEMA systemupdate_sema;			// 升级信息的互斥保护

static volatile unsigned char systemupdate_step;
static volatile unsigned char systemupdate_state;		// 1 升级进行中 0 升级未执行

// 0x00000000表示没有升级版本需要安装
// 0xFFFFFFFF为自动安装版本, 无需用户确认, 用于内部测试
static volatile unsigned int systemupdate_version;		// 待升级的软件版本号

static OS_TIMER UpdateNotificationTimer;
static int notify_system_update_event;			// 定时发送系统升级消息


// 系统升级任务为最低优先级任务(后台任务)
// 卡插入时, 后台检查是否存在系统升级包. 
// 若存在, 投递消息给UI线程提示系统升级包存在.

static unsigned int checksum_int(const unsigned int *buf, int len)
{
	register int counter;
	register unsigned int checksum = 0;
	for( counter = 0; counter < len; counter++)
		checksum += *(unsigned int *)buf++;

	return ((unsigned int)(checksum));
}

static unsigned int UpdatePackagechecksum(unsigned int *s, unsigned int len )
{
	unsigned int count = 0;
	count = checksum_int(s,3);// 0 , 1, 2
	//check_sum不纳入校验和计算   // 3
	count+= checksum_int(s+4,len-4);// 4 , 5, 6....

	return count;
}

// 检查系统升级包(包括系统升级信息结构及二进制数据)(checksum, ID, 长度，CRC)
// 0  --> 升级包数据OK，并将升级信息结构填充到systemupdate_info
// -1 --> 升级包数据异常
static int	XM_CheckSystemUpdatePackage (char *package_buffer, 
											  int package_length
											  )
{
	unsigned int header_checksum;
	int ret=0 ;
	XM_SYSTEMUPDATE_INFO *info = (XM_SYSTEMUPDATE_INFO *)package_buffer;
	// 检查ID
	if(info->id != XMSYS_SYSTEMUPDATE_ID)
	{
		XM_printf ("illegal package ID\n");
		return -1;
	}
	// 检查升级文件格式版本。当前仅支持1.0版本
	if(info->version != XMSYS_SYSTEMUPDATE_VERSION_1_0)
	{
		XM_printf ("packet version 0x(%08x) don't support by current fireware\n", info->version);
		return -1;		
	}
	
	// 数据结构体校验和 每个32bit数据 相加
	header_checksum = UpdatePackagechecksum((unsigned int *)(info) , sizeof(XM_SYSTEMUPDATE_INFO)/4 );
	if(info->checksum != (unsigned int)(0xAAAAAAAA - header_checksum) )
	{	
		XM_printf("packet header checksum is Error!\n");
		return -1;
	}

	//二进制文件 CRC16校验和                                              =640
//	if( info->binary_crc == crc16_ccitt( (char*)package_buffer + info->binary_offset, systemupdate_info->binary_length ))/*错误参数，可导致校验错误*/
	if( info->binary_crc == crc16_ccitt( (char*)package_buffer + info->binary_offset, info->binary_length ))
		XM_printf("data binary_crc is correct.\n");
	else
	{	
		XM_printf("data binary_crc is Error!\n");
		return -1;
	}
		
	return ret;
}

// 检查系统升级包头结构信息
// 0  --> 升级包头结构信息正确
// -1 --> 升级包头结构信息异常
int	XM_CheckSystemUpdatePackageHeader ( XM_SYSTEMUPDATE_INFO *package_buffer )
{
	unsigned int header_checksum;
	int ret=0 ;
	XM_SYSTEMUPDATE_INFO *info = (XM_SYSTEMUPDATE_INFO *)package_buffer;
	// 检查ID
	if(info->id != XMSYS_SYSTEMUPDATE_ID)
	{
		XM_printf ("illegal package ID\n");
		return -1;
	}
	// 检查升级文件格式版本。当前仅支持1.0版本
	if(info->version != XMSYS_SYSTEMUPDATE_VERSION_1_0)
	{
		XM_printf ("packet version 0x(%08x) don't support by current fireware\n", info->version);
		return -1;		
	}
	
	// 数据结构体校验和 每个32bit数据 相加
	header_checksum = UpdatePackagechecksum((unsigned int *)(info) , sizeof(XM_SYSTEMUPDATE_INFO)/4 );
	if(info->checksum != (unsigned int)(0xAAAAAAAA - header_checksum) )
	{	
		XM_printf("packet header checksum is Error!\n");
		return -1;
	}
	
	return ret;
}

static int LoadSystemPackageHeaderFromSpi (char *package, int size)
{
	int ret = -1;
	int loop = 4;
	if(size != MAX_SYSTEMUPDATE_HEADER_SIZE)
		return -1;
	
	OS_Use(&systemupdate_sema); /* Make sure nobody else uses */
	while(loop > 0)
	{
		// 从SPI中读取系统升级包头信息
		if(Spi_Read (URTOS_ADDR, (UINT8 *)package, size) == 1)
		{
			// 检查系统升级包头信息的完备
			if(XM_CheckSystemUpdatePackageHeader ((XM_SYSTEMUPDATE_INFO *)package) == 0)
			{
				ret = 0;
				break;
			}
		}
		loop --;
	}
	OS_Unuse(&systemupdate_sema); /* Make sure nobody else uses */
	return ret;
}

// 获取系统软件版本
// 返回值  
//		-1		失败
//		0		成功
int XMSYS_GetSystemSoftwareVersion (unsigned char sw_version[4])
{
	char *package = NULL;
	//char *package = system_update_header;
	int ret = -1;
	int loop = 4;
	
	OS_Use(&systemupdate_sema); /* Make sure nobody else uses */
	
	while(loop > 0)
	{
		do
		{
			package = OS_malloc (MAX_SYSTEMUPDATE_HEADER_SIZE);
			if(package == NULL)
				break;
			// 从SPI中读取系统升级包头信息
			if(Spi_Read (URTOS_ADDR, (UINT8 *)package, MAX_SYSTEMUPDATE_HEADER_SIZE) == 0)	// io error
				break;
			// 检查系统升级包头信息的完备
			if(XM_CheckSystemUpdatePackageHeader ((XM_SYSTEMUPDATE_INFO *)package))
				break;
			memcpy (sw_version, &(((XM_SYSTEMUPDATE_INFO *)package)->brand_version), 4);
			ret = 0;
		} while(0);
		if(package)
		{
			OS_free (package);
			package = 0;
		}
		if(ret == 0)
			break;
		loop --;
	} 
	
	OS_Unuse(&systemupdate_sema); /* Make sure nobody else uses */
	
	return ret;
}


// 检查系统升级包是否存在
// 0  升级包存在且有效
// -1 升级包不存在或无效
static int system_update_check (void)
{
	// 检查系统升级文件是否存在
	FS_FILE *fp;
	int ret;
	char temp[16];
	char *update_package_info_base = NULL;
	XM_SYSTEMUPDATE_INFO *update_package_info = NULL;
	unsigned int sys_version, cur_version;
	
	XM_SYSTEMUPDATE_INFO *local_system_info = NULL;		// 本地版本信息
	
	memset (temp, 0, sizeof(temp));
	
	systemupdate_version = 0;
	
	OS_Use(&systemupdate_sema); /* Make sure nobody else uses */
	fp = NULL;

	ret = -1;
	do 
	{
		local_system_info = (XM_SYSTEMUPDATE_INFO *)kernel_malloc (sizeof(XM_SYSTEMUPDATE_INFO));
		if(local_system_info == NULL)
		{
			XM_printf ("system_update_check failed, memory busy\n");
			break;
		}
		fp = FS_FOpen("\\update.bin", "rb");
		if(fp == NULL)
			break;
		
		// 检查文件的大小
		unsigned int size = FS_GetFileSize (fp);
		if(size == (unsigned int)(-1))
		{
			// 无法获取文件字节大小
			break;
		}
		if(size >= XMSYS_SYSTEM_UPDATE_PACKAGE_SIZE || size < sizeof(XM_SYSTEMUPDATE_INFO))
		{
			// 超出系统允许的最大升级包字节大小
			XM_printf ("system_update_check failed, illegal size(%d) of \\update.bin great than the maximum size(%d)\n", size, XMSYS_SYSTEM_UPDATE_PACKAGE_SIZE);
			break;
		}

		if(size >= Spi_GetSize())
		{
			// 超出系统允许的最大升级包字节大小
			XM_printf ("XMSYS_system_update failed, file size(%d) of \\update.bin great than the size(%d) of SPI Flash\n", size, Spi_GetSize() );
			break;
		}

		// 读取升级包头并检查完备性
		update_package_info_base = (char *)kernel_malloc ((sizeof(XM_SYSTEMUPDATE_INFO) + 0x3F) & (~0x3F));	// 按cache line对齐分配
		//update_package_info = (XM_SYSTEMUPDATE_INFO *)kernel_malloc (sizeof(XM_SYSTEMUPDATE_INFO));
		//if(update_package_info == NULL)
		if(update_package_info_base)
		{
			update_package_info = (XM_SYSTEMUPDATE_INFO *)((((unsigned int)update_package_info_base) + 0x1F) & (~0x1F));
		}
		else
		{
			update_package_info = NULL;
			XM_printf ("system_update_check failed, memory busy\n");
			break;
		}

		ret = FS_Read(fp, (char *)update_package_info, sizeof(XM_SYSTEMUPDATE_INFO));
		if(ret != sizeof(XM_SYSTEMUPDATE_INFO))
		{
			XM_printf ("system_update_check failed, file io error\n");
			ret = -1;
			break;
		}

		// 文件头信息读取正确
		// 检查升级包的正确性
		if(XM_CheckSystemUpdatePackageHeader(update_package_info))
		{
			XM_printf ("system_update_check failed, illegal package\n");
			ret = -1;
			break;
		}
		
		//if(Spi_Read  (URTOS_ADDR, (UINT8 *)local_system_info, MAX_SYSTEMUPDATE_HEADER_SIZE) == 0)
		if(LoadSystemPackageHeaderFromSpi ((char *)local_system_info, MAX_SYSTEMUPDATE_HEADER_SIZE) < 0)
		{
			XM_printf ("system_update_check failed, spi io error\n");
			ret = -1;
			break;
		}
		// 
				
			
		// 升级包OK
		sys_version = update_package_info->brand_version;
		// 读取本机的系统版本号
		cur_version = 0xFFFFFFFF;		// 0xFFFFFFFF表示系统版本号不存在
		if(XMSYS_GetSystemSoftwareVersion ((unsigned char *)&cur_version) < 0)
		{
			XM_printf ("system_update_check failed, get software version io error\n");
			ret = -1;
			break;
		}

		XM_printf(">cur_version:%x\r\n", cur_version);
		XM_printf(">sys_version:%x\r\n", sys_version);
		// 检查以下条件 
		// 	1) 本机的系统版本号不存在 或者 2) SD卡上升级包的系统版本号是否大于本机的系统版本号
		if( (cur_version == 0xFFFFFFFF) || (sys_version != cur_version) )
		{
			// 仅在手动安装版本确认厂商信息. 自动安装版本用于内部调试, 无需检查
			if(cur_version != 0xFFFFFFFF && sys_version != 0xFFFFFFFF)
			{
				XM_printf(">update_package_info->brand_type:%x\r\n", update_package_info->brand_type);
				XM_printf(">local_system_info->brand_type:%x\r\n", local_system_info->brand_type);
				XM_printf(">update_package_info->brand_id:%s\r\n", update_package_info->brand_id);
				XM_printf(">local_system_info->brand_id:%s\r\n", local_system_info->brand_id);
				
				// 检查厂商信息是否匹配
				if(update_package_info->brand_type != local_system_info->brand_type)
				{
					XM_printf ("system_update_check failed, brand_type mismatch\n");
					ret = -1;
					break;
				}
				
				if(memcmp (update_package_info->brand_id, local_system_info->brand_id, sizeof(update_package_info->brand_id)))
				{
					XM_printf ("system_update_check failed, brand_id mismatch\n");
					ret = -1;
					break;
				}
			}
			ret = 0;
			// 准备"辅机升级包通知"异步消息
			// 消息包数据8个字节
			// 版本(4字节)，文件长度（4字节）
			//	辅机检测到SD卡中的系统升级文件(合法的升级文件)，通知主机。(辅机开机时及卡插入时检查)
			*(unsigned int *)(temp + 0) = update_package_info->brand_version;
			*(unsigned int *)(temp + 4) = size;
			XM_printf("UPDATE_PACKAGE Notification, Version=0x%08x, Length=%d", 
												*(unsigned int *)(temp + 0), 
												*(unsigned int *)(temp + 4)
												);
			
			systemupdate_version = sys_version;
		
			if(systemupdate_version == 0xFFFFFFFF)
			{
				notify_system_update_event = 1;
			}
			XM_printf(">systemupdate_version:%x...\r\n", systemupdate_version);
			XM_printf(">notify_system_update_event:%x...\r\n", notify_system_update_event);
			XM_printf(">update file ok, send event...\r\n");
			
			XM_KeyEventProc(VK_AP_SYSTEM_EVENT, SYSTEM_EVENT_SYSTEM_UPDATE_FILE_CHECKED );

			#if 0
			XMSYS_UvcSocketTransferResponsePacket	(XMSYS_UVC_SOCKET_TYPE_ASYC,
												XMSYS_UVC_NID_SYSTEM_UPDATE_PACKAGE,
												(unsigned char *)temp,
												8
												);		
			#endif
			/*
			XMSYS_MessageSocketTransferResponsePacket (XMSYS_MESSAGE_SOCKET_TYPE_ASYC,
												 -1,
												XMSYS_NID_SYSTEM_UPDATE_PACKAGE,
												(unsigned char *)temp,
												8
												);	
			*/
		}
		else
		{
			XM_printf ("system_update_check failed, cur_version(%08x) >= sys_version(%08x)\n", cur_version, sys_version);
			ret = -1;
		}
		// 通知主机
	} while(0);	

	if(local_system_info)
		kernel_free (local_system_info);
	if(fp)
		FS_FClose (fp);
	if(update_package_info_base)
		kernel_free (update_package_info_base);
	update_package_info = NULL;

	OS_Unuse(&systemupdate_sema); /* Make sure nobody else uses */

	return ret;	
}

static int load_boot (char *file,  char *loader_buff)
{
	FS_FILE *fp;
	int ret;
		
	fp = NULL;

	ret = -1;
	do 
	{
		fp = FS_FOpen (file, "rb");
		if(fp == NULL)
			break;
		
		// 检查文件的大小
		unsigned int size = FS_GetFileSize (fp);
		if(size == (unsigned int)(-1))
		{
			// 无法获取文件字节大小
			break;
		}
		if(size > PAGE_SIZE)
		{
			// 超出系统允许的最大升级包字节大小
			XM_printf ("XMSYS_system_update failed, illegal size(%d) of %s\n", size, file);
			break;
		}

		ret = FS_Read (fp, (char *)loader_buff, size);
		if(ret != size)
		{
			XM_printf ("load_boot failed, file io error\n");
			break;
		}

		// 文件头信息读取正确
		// 检查升级包的正确性
		if(*(unsigned int *)(loader_buff + 4) != 0x4E313431)	// N141
		{
			XM_printf ("load_boot failed, file (%s) is illegal format\n", file);
			break;
		}
		
		// crc检查
		unsigned short crc = crc16_ccitt ((char *)loader_buff, PAGE_SIZE - 2);
		if( *(unsigned short *)( ((char *)loader_buff) + PAGE_SIZE - 2) != crc )
		{
			XM_printf ("load_boot failed, file (%s) is illegal format\n", file);
			break;			
		}
			
		ret = 0;

		// 通知主机
	} while(0);	

	if(fp)
		FS_FClose (fp);
	return ret;		
}

// 从SPI读取boot code
static int load_boot_from_spi (char *loader_buff, int size)
{
	int ret;
	int loop;
		
	ret = -1;
	loop = 4;
	while (loop > 0)
	{
		do 
		{
			if(Spi_Read  (0, (UINT8 *)loader_buff, size) == 0)	// io error
				break;
			// 文件头信息读取正确
			// 检查升级包的正确性
			if(*(unsigned int *)(loader_buff + 4) != 0x4E313431)	// N141
			{
				XM_printf ("spi io error\n");
				break;
			}
			
			// crc检查
			unsigned short crc = crc16_ccitt ((char *)loader_buff, PAGE_SIZE - 2);
			if( *(unsigned short *)( ((char *)loader_buff) + PAGE_SIZE - 2) != crc )
			{
				XM_printf ("load_boot failed, spi io error\n");
				break;			
			}
				
			ret = 0;
	
		} while(0);	
		
		if(ret == 0)
			break;
		loop --;
	}

	return ret;
}

static int update_load_boot (char *file)
{
	#define	PAGE_SIZE	0x1000
	FS_FILE *fp;
	int ret;
	char temp[16];
	char *update_package = NULL;
	char *verify_buff = NULL;
	memset (temp, 0, sizeof(temp));
	
	OS_Use(&systemupdate_sema); /* Make sure nobody else uses */
	
	fp = NULL;

	ret = -1;
	do 
	{
		fp = FS_FOpen (file, "rb");
		if(fp == NULL)
			break;
		
		// 检查文件的大小
		unsigned int size = FS_GetFileSize (fp);
		if(size == (unsigned int)(-1))
		{
			// 无法获取文件字节大小
			break;
		}
		if(size > PAGE_SIZE)
		{
			// 超出系统允许的最大升级包字节大小
			XM_printf ("XMSYS_system_update failed, illegal size(%d) of %s\n", size, file);
			break;
		}

		// 读取升级包头并检查完备性
		update_package = (char *)kernel_malloc (PAGE_SIZE);
		if(update_package == NULL)
		{
			XM_printf ("update_load_boot failed, memory busy\n");
			break;
		}
		verify_buff = (char *)kernel_malloc (PAGE_SIZE);
		if(verify_buff == NULL)
		{
			XM_printf ("update_load_boot failed, memory busy\n");
			break;			
		}
		memset (update_package, 0, PAGE_SIZE);
		memset (verify_buff, 0, PAGE_SIZE);

		ret = FS_Read (fp, (char *)update_package, size);
		if(ret != size)
		{
			XM_printf ("update_load_boot failed, file io error\n");
			break;
		}

		// 文件头信息读取正确
		// 检查升级包的正确性
		if(*(unsigned int *)(update_package + 4) != 0x4E313431)	// N141
		{
			XM_printf ("update_load_boot failed, file (%s) is illegal format\n", file);
			break;
		}
		
		// crc检查
		unsigned short crc = crc16_ccitt ((char *)update_package, PAGE_SIZE - 2);
		if( *(unsigned short *)( ((char *)update_package) + PAGE_SIZE - 2) != crc )
		{
			XM_printf ("update_load_boot failed, file (%s) is illegal format\n", file);
			break;			
		}
			
		// 开始升级过程	
		XM_printf ("%s update start...\n", file);
		
		int loop;
		for (loop = 0; loop < 4; loop ++)
		{
			if(Spi_Write (0, (UINT8 *)update_package,   PAGE_SIZE) == 0)
				continue;
			// 回读比较
			if(Spi_Read  (0, (UINT8 *)verify_buff, PAGE_SIZE) == 0)
				continue;
			if(memcmp (update_package, verify_buff, PAGE_SIZE))
				continue;
			break;
		}		
		if(loop == 4)
		{
			ret = -1;
		}
		else
		{
			ret = 0;
		}
		
		XM_printf ("%s update %s\n", file, (ret == 0) ? "OK" : "NG");

		// 通知主机
	} while(0);	

	if(fp)
		FS_FClose (fp);
	if(update_package)
		kernel_free (update_package);
	if(verify_buff)
		kernel_free (verify_buff);
	
	OS_Unuse(&systemupdate_sema); /* Make sure nobody else uses */

	return ret;		
	
}

//#define	LPAGE_SIZE		(PAGE_SIZE*32)
#define	LPAGE_SIZE		(PAGE_SIZE*16)//一次性写入一个block,避免有时候出现写、读比较不成功，造成升级失败
#define	REPEAT_COUNT	8

// 执行系统升级
// 返回值
//		0		系统升级成功
//		-1		系统升级失败
static int system_update (void)
{
	// 检查系统升级文件是否存在
	FS_FILE *fp;
	int ret;
	char temp[16];
	char *update_package = NULL;
	char *verify_buff = NULL;
	
	char *loader_buff = NULL;
	char *loader_verify_buff = NULL;
	
	unsigned int sys_version, cur_version;
	int loop;	// 循环次数，4次
	
	// 停止自动测试
	//xm_autotest_stop ();
	
	memset (temp, 0, sizeof(temp));
		
	OS_Use(&systemupdate_sema); /* Make sure nobody else uses */
	
	systemupdate_state = 1;
	systemupdate_step = 0;
	
	fp = NULL;

	ret = -1;
	do 
	{
		fp = FS_FOpen ("\\update.bin", "rb");
		if(fp == NULL)
			break;
		
		// 检查文件的大小
		unsigned int size = FS_GetFileSize (fp);
		if(size == (unsigned int)(-1))
		{
			// 无法获取文件字节大小
			break;
		}
		if(size >= XMSYS_SYSTEM_UPDATE_PACKAGE_SIZE || size < sizeof(XM_SYSTEMUPDATE_INFO))
		{
			// 超出系统允许的最大升级包字节大小
			XM_printf ("XMSYS_system_update failed, illegal size(%d) of \\update.bin great than the maximum size(%d)\n", size, XMSYS_SYSTEM_UPDATE_PACKAGE_SIZE);
			break;
		}
		
		if(size >= Spi_GetSize())
		{
			// 超出系统允许的最大升级包字节大小
			XM_printf ("XMSYS_system_update failed, file size(%d) of \\update.bin great than the size(%d) of SPI Flash\n", size, Spi_GetSize() );
			break;
		}

		// 读取升级包头并检查完备性
		update_package = (char *)kernel_malloc (size);
		if(update_package == NULL)
		{
			XM_printf ("XMSYS_system_update failed, memory busy\n");
			break;
		}
		verify_buff = (char *)kernel_malloc (LPAGE_SIZE);
		if(verify_buff == NULL)
		{
			XM_printf ("XMSYS_system_update failed, memory busy\n");
			break;			
		}

		if(FS_Read (fp, (char *)update_package, size) != size)
		{
			XM_printf ("XMSYS_system_update failed, file io error\n");
			break;
		}
		FS_FClose (fp);
		fp = NULL;

		// 文件头信息读取正确
		// 检查升级包的正确性
		if(XM_CheckSystemUpdatePackage (update_package, size))
		{
			XM_printf ("XMSYS_system_update failed, illegal package\n");
			break;
		}
			
		// 升级包OK
		sys_version = ((XM_SYSTEMUPDATE_INFO *)update_package)->brand_version;
		// 读取本机的系统版本号
		cur_version = 0xFFFFFFFF;		// 0xFFFFFFFF表示系统版本号不存在
		if(XMSYS_GetSystemSoftwareVersion ((unsigned char *)&cur_version) < 0)
		{
			XM_printf ("XMSYS_system_update failed, illegal software version\n");
			break;			
		}
		// 检查以下条件 
		// 	1) 本机的系统版本号不存在 或者 2) SD卡上升级包的系统版本号是否大于本机的系统版本号
		if( (cur_version == 0xFFFFFFFF) || (sys_version != cur_version) )
		{
			//ret = 0;
		}
		else
		{
			XM_printf ("XMSYS_system_update failed, cur_version(%08x) >= sys_version(%08x)\n", cur_version, sys_version);
			break;
		}
		
		// 读取LOADSYS.BIN
		loader_buff = kernel_malloc (PAGE_SIZE);
		loader_verify_buff = kernel_malloc (PAGE_SIZE);
		if(loader_buff == NULL || loader_verify_buff == NULL)
		{
			XM_printf ("XMSYS_system_update failed, memory busy\n");
			break;
		}
		memset (loader_verify_buff, 0, PAGE_SIZE);
		if(load_boot ("\\LOADSYS.BIN", loader_buff) < 0)
		{
			// 从SPI回读到RAM, 对读取的内容进行完整性检查
			if(load_boot_from_spi ((char *)loader_buff, PAGE_SIZE) < 0)
			{
				XM_printf ("XMSYS_system_update failed, spi io error\n");
				break;
			}
			/*
			for (loop = 0; loop < 4; loop ++)
			{
				if(Spi_Read  (0, (UINT8 *)loader_buff, PAGE_SIZE) == 1)
				{
					break;
				}
			}
			if(loop == 4)
			{
				XM_printf ("XMSYS_system_update failed, spi io error\n");
				break;
			}
			*/
		}
		
		// LOADSYS.BIN UPDATE.BIN 均已读取且检查	
		
		// 等待主机UI准备
		OS_Delay (50);
		
		// 开始升级过程	
		XM_printf ("system update start...\n");
		
		// 擦除 LOADER区域
		Spi_Write (0, (UINT8 *)loader_verify_buff, PAGE_SIZE);
				
		unsigned int base = URTOS_ADDR;
		unsigned int last = base + size;
		char *data = (char *)update_package;
		char *data_r = (char *)verify_buff;
		memset (data_r, 0, sizeof(verify_buff));
		last = (last + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
		while(base < last)
		{
			unsigned int to_write = last - base;
			unsigned char step;
			if(to_write > LPAGE_SIZE)
				to_write = LPAGE_SIZE;
			for (loop = 0; loop < REPEAT_COUNT; loop ++)
			{
				if(Spi_Write (base, (UINT8 *)data,   to_write) == 0)
					continue;
				// 回读比较
				if(Spi_Read  (base, (UINT8 *)data_r, to_write) == 0)
					continue;
				if(memcmp (data, data_r, to_write))
					continue;
				break;
			}
			if(loop == REPEAT_COUNT)
			{
				// error
				break;
			}
			
			data += LPAGE_SIZE;
			base += LPAGE_SIZE;
			if(base > last)
				base = last;
				
			// 计算进度百分比
			step = (unsigned char)((last - base) * 100 / size);
			step = 100 - step;
			
			XM_printf ("step = %d%%\n", step);
			if(step == 100)
				step = 99;
			
			systemupdate_step = step;				
		}
		
		if(base < last)
		{
			break;
		}

		XM_printf ("Program LOADSYS ...\n");
		for (loop = 0; loop < REPEAT_COUNT; loop ++)
		{
			if(Spi_Write (0, (UINT8 *)loader_buff,   PAGE_SIZE) == 0)
				continue;
			// 回读比较
			if(Spi_Read  (0, (UINT8 *)loader_verify_buff, PAGE_SIZE) == 0)
				continue;
			if(memcmp (loader_buff, loader_verify_buff, PAGE_SIZE))
				continue;
			
			ret = 0;
			break;
		}
		
		if(ret != 0)
			XM_printf ("Program LOADSYS NG\n");
		
		// 通知主机
	} while(0);	

	if(ret == 0)
	{			
		unsigned char step = 100;
		systemupdate_step = step;
		XM_printf ("update OK\n");
        AP_RestoreMenuData();//升级成功恢复默认参数
		XMSYS_MessageSocketTransferResponsePacket (XMSYS_MESSAGE_SOCKET_TYPE_ASYC,
															 -1,
															XMSYS_NID_SYSTEM_UPDATE_STEP,
															&step,
															1
															);
	}
	else
	{
		// 升级失败
		unsigned char step = 0xFF;
		systemupdate_step = step;
		XM_printf ("update NG\n");
		XMSYS_MessageSocketTransferResponsePacket (XMSYS_MESSAGE_SOCKET_TYPE_ASYC,
															 -1,
															XMSYS_NID_SYSTEM_UPDATE_STEP,
															&step,
															1
															);
	}		
	
	if(fp)
		FS_FClose (fp);
	if(update_package)
		kernel_free (update_package);
	if(verify_buff)
		kernel_free (verify_buff);
	
	if(loader_buff)
		kernel_free (loader_buff);
	if(loader_verify_buff)
		kernel_free (loader_verify_buff);
	
	systemupdate_state = 0;
	OS_Unuse(&systemupdate_sema); /* Make sure nobody else uses */

	return ret;	
}



#define	XMSYS_SYSTEM_UPDATE_EVENT_CHECK		0x01		// 检查升级包
#define	XMSYS_SYSTEM_UPDATE_EVENT_UPDATE		0x02		// 执行升级
#define	XMSYS_SYSTEM_UPDATE_EVENT_TIMER		0x04		// 定时器, 发送升级通知

__no_init static OS_STACKPTR int StackSystemUpdate[XMSYS_SYSTEM_UPDATE_TASK_STACK_SIZE/4];          /* Task stacks */
static OS_TASK TCB_SystemUpdate;                        /* Task-control-blocks */

static void SystemUpdateTask(void)
{
	while(1)
	{
		OS_U8 system_update_event = 0;
		
		system_update_event = OS_WaitEvent (XMSYS_SYSTEM_UPDATE_EVENT_CHECK|XMSYS_SYSTEM_UPDATE_EVENT_UPDATE|XMSYS_SYSTEM_UPDATE_EVENT_TIMER);
		
		// 检查升级包
		if(system_update_event & XMSYS_SYSTEM_UPDATE_EVENT_CHECK )
		{
			unsigned char old_priority;
			OS_TASK *pCurrentTask;
			XM_printf(">>>>>>>>>>>>SYSTEM_UPDATE_EVENT_CHECK......\r\n");
			notify_system_update_event = 0;
			pCurrentTask = OS_GetpCurrentTask();
			old_priority = OS_GetPriority (pCurrentTask);
			OS_SetPriority(pCurrentTask, 253);//改变任务优先级
			system_update_check ();
			OS_SetPriority(pCurrentTask, old_priority);//恢复任务优先级
		}
		// 系统升级
		else if(system_update_event & XMSYS_SYSTEM_UPDATE_EVENT_UPDATE )
		{
			XM_printf ("SYSTEM_UPDATE_EVENT_UPDATE\n");
			notify_system_update_event = 0;
			if(system_update () < 0)
			{
				// 清除升级信息
				systemupdate_version = 0;
			}
		}
		
		if(system_update_event & XMSYS_SYSTEM_UPDATE_EVENT_TIMER )
		{
			//XM_printf(">>>>>>>>>>>systemupdate_version:%x\r\n", systemupdate_version);
			//XM_printf(">>>>>>>>>>>notify_system_update_event:%x\r\n", notify_system_update_event);
			// 检查自动升级包是否存在. 若存在, 定时给桌面发送消息
			if(systemupdate_version == 0xFFFFFFFF && notify_system_update_event)
			{
				if(IsCardExist(0))				
				{
					XM_KeyEventProc(VK_AP_SYSTEM_EVENT, SYSTEM_EVENT_SYSTEM_UPDATE_FILE_CHECKED );
				}
				else
				{
					notify_system_update_event = 0;
				}
			}
		}
	}
}

// 检查升级包
int XMSYS_system_update_check (void)
{
	OS_SignalEvent (XMSYS_SYSTEM_UPDATE_EVENT_CHECK, &TCB_SystemUpdate); /* 通知事件 */
	return 0;
}

// 重新设置LCDC的自动关屏时间
// 0xFFFFFFFF 表示永不关屏
// 其他值 表示自动关屏的时间(毫秒), 
void hw_backlight_set_auto_off_ticket (unsigned int ticket);

// 系统升级
// 返回值
//		0		系统升级成功
//		-1		系统升级失败
int XMSYS_system_update (void)
{
	unsigned char step;
	unsigned int ticket_to_timeout;
	
	hw_backlight_set_auto_off_ticket (0xFFFFFFFF);	// 禁止关闭背光
	
	OS_Use(&systemupdate_sema); /* Make sure nobody else uses */
	systemupdate_step = 0;	// 0%完成状态
	OS_SignalEvent (XMSYS_SYSTEM_UPDATE_EVENT_UPDATE, &TCB_SystemUpdate); /* 通知事件 */
	OS_Unuse(&systemupdate_sema);
	
	OS_Delay (300);		// 等待通知事件传输完毕
	
	ticket_to_timeout = XM_GetTickCount () + 250 * 1000;
	step = systemupdate_step;
	do {
		if(step != systemupdate_step)
			XMSYS_UvcSocketTransferResponsePacket (XMSYS_UVC_SOCKET_TYPE_ASYC,
															XMSYS_UVC_NID_SYSTEM_UPDATE_STEP,
															(unsigned char *)&step,
															1
															);
		step = systemupdate_step;
		if(systemupdate_step == 100)
			break;
		if(systemupdate_step == 0xff)
			break;
		
		// 检查是否超时
		if(XM_GetTickCount() >= ticket_to_timeout)
		{
			step = 0xff;
			break;
		}
		
		OS_Delay (1);
	} while ( 1 );
	
	// 确保完成状态信息可靠的发送给主机
	int loop = 3;
	while (loop > 0)
	{
		XMSYS_UvcSocketTransferResponsePacket (XMSYS_UVC_SOCKET_TYPE_ASYC,
															XMSYS_UVC_NID_SYSTEM_UPDATE_STEP,
															(unsigned char *)&step,
															1
															);
		loop --;
		OS_Delay (10);
	}
	
	if(systemupdate_step == 100)
		return 0;
	else 	// 0xff表示失败
		return -1;
}

// 获取系统升级过程当前的阶段, 
// 0 ~ 100		表示升级完成的百分比
// 0xFF			表示升级已失败
unsigned int XMSYS_system_update_get_step (void)
{
	return systemupdate_step;
}

// 检查系统升级是否执行中
// 返回值 
// 1   正在执行中
// 0   没有执行
unsigned int XMSYS_system_update_busy_state (void)
{
	return systemupdate_state;
}

// 获取待升级系统的版本
// 0xFFFFFFFF为自动安装版本, 无需用户确认, 用于内部测试
unsigned int XMSYS_system_update_get_version (void)
{
	return systemupdate_version;
}

static void UpdateNotificationTicketCallback (void)
{
	OS_SignalEvent(XMSYS_SYSTEM_UPDATE_EVENT_TIMER, &TCB_SystemUpdate); /* 通知事件 */
	OS_RetriggerTimer(&UpdateNotificationTimer);
}

void XMSYS_SystemUpdateInit (void)
{
	OS_CREATERSEMA (&systemupdate_sema); /* Creates resource semaphore */
	
	// 创建1hz的定时器
	OS_CREATETIMER (&UpdateNotificationTimer, UpdateNotificationTicketCallback, 1000);
	
	// 系统升级线程的优先级与UI线程优先级一致, 这样UI在升级中可以刷新显示, 同时系统升级线程可以较快相应升级事件
	OS_CREATETASK(&TCB_SystemUpdate, "SystemUpdate", SystemUpdateTask, XMSYS_SYSTEM_UPDATE_TASK_PRIORITY, StackSystemUpdate);
}

void XMSYS_SystemUpdateExit (void)
{
	OS_DeleteRSema (&systemupdate_sema); 
}
