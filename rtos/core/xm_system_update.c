#include "hardware.h"
#include "xm_flash_space_define.h"		// Flash�ռ���䶨��
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


#define URTOS_ADDR	XM_FLASH_RTOS_BASE	// ��SPI Flash��4KB��ַƫ�ƿ�ʼ���� RTOS
#define	PAGE_SIZE	0x1000


#define	XMSYS_SYSTEM_UPDATE_PACKAGE_SIZE		XM_FLASH_RTOS_SIZE			// ���ϵͳ�������ֽڴ�С

static OS_RSEMA systemupdate_sema;			// ������Ϣ�Ļ��Ᵽ��

static volatile unsigned char systemupdate_step;
static volatile unsigned char systemupdate_state;		// 1 ���������� 0 ����δִ��

// 0x00000000��ʾû�������汾��Ҫ��װ
// 0xFFFFFFFFΪ�Զ���װ�汾, �����û�ȷ��, �����ڲ�����
static volatile unsigned int systemupdate_version;		// ������������汾��

static OS_TIMER UpdateNotificationTimer;
static int notify_system_update_event;			// ��ʱ����ϵͳ������Ϣ


// ϵͳ��������Ϊ������ȼ�����(��̨����)
// ������ʱ, ��̨����Ƿ����ϵͳ������. 
// ������, Ͷ����Ϣ��UI�߳���ʾϵͳ����������.

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
	//check_sum������У��ͼ���   // 3
	count+= checksum_int(s+4,len-4);// 4 , 5, 6....

	return count;
}

// ���ϵͳ������(����ϵͳ������Ϣ�ṹ������������)(checksum, ID, ���ȣ�CRC)
// 0  --> ����������OK������������Ϣ�ṹ��䵽systemupdate_info
// -1 --> �����������쳣
static int	XM_CheckSystemUpdatePackage (char *package_buffer, 
											  int package_length
											  )
{
	unsigned int header_checksum;
	int ret=0 ;
	XM_SYSTEMUPDATE_INFO *info = (XM_SYSTEMUPDATE_INFO *)package_buffer;
	// ���ID
	if(info->id != XMSYS_SYSTEMUPDATE_ID)
	{
		XM_printf ("illegal package ID\n");
		return -1;
	}
	// ��������ļ���ʽ�汾����ǰ��֧��1.0�汾
	if(info->version != XMSYS_SYSTEMUPDATE_VERSION_1_0)
	{
		XM_printf ("packet version 0x(%08x) don't support by current fireware\n", info->version);
		return -1;		
	}
	
	// ���ݽṹ��У��� ÿ��32bit���� ���
	header_checksum = UpdatePackagechecksum((unsigned int *)(info) , sizeof(XM_SYSTEMUPDATE_INFO)/4 );
	if(info->checksum != (unsigned int)(0xAAAAAAAA - header_checksum) )
	{	
		XM_printf("packet header checksum is Error!\n");
		return -1;
	}

	//�������ļ� CRC16У���                                              =640
//	if( info->binary_crc == crc16_ccitt( (char*)package_buffer + info->binary_offset, systemupdate_info->binary_length ))/*����������ɵ���У�����*/
	if( info->binary_crc == crc16_ccitt( (char*)package_buffer + info->binary_offset, info->binary_length ))
		XM_printf("data binary_crc is correct.\n");
	else
	{	
		XM_printf("data binary_crc is Error!\n");
		return -1;
	}
		
	return ret;
}

// ���ϵͳ������ͷ�ṹ��Ϣ
// 0  --> ������ͷ�ṹ��Ϣ��ȷ
// -1 --> ������ͷ�ṹ��Ϣ�쳣
int	XM_CheckSystemUpdatePackageHeader ( XM_SYSTEMUPDATE_INFO *package_buffer )
{
	unsigned int header_checksum;
	int ret=0 ;
	XM_SYSTEMUPDATE_INFO *info = (XM_SYSTEMUPDATE_INFO *)package_buffer;
	// ���ID
	if(info->id != XMSYS_SYSTEMUPDATE_ID)
	{
		XM_printf ("illegal package ID\n");
		return -1;
	}
	// ��������ļ���ʽ�汾����ǰ��֧��1.0�汾
	if(info->version != XMSYS_SYSTEMUPDATE_VERSION_1_0)
	{
		XM_printf ("packet version 0x(%08x) don't support by current fireware\n", info->version);
		return -1;		
	}
	
	// ���ݽṹ��У��� ÿ��32bit���� ���
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
		// ��SPI�ж�ȡϵͳ������ͷ��Ϣ
		if(Spi_Read (URTOS_ADDR, (UINT8 *)package, size) == 1)
		{
			// ���ϵͳ������ͷ��Ϣ���걸
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

// ��ȡϵͳ����汾
// ����ֵ  
//		-1		ʧ��
//		0		�ɹ�
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
			// ��SPI�ж�ȡϵͳ������ͷ��Ϣ
			if(Spi_Read (URTOS_ADDR, (UINT8 *)package, MAX_SYSTEMUPDATE_HEADER_SIZE) == 0)	// io error
				break;
			// ���ϵͳ������ͷ��Ϣ���걸
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


// ���ϵͳ�������Ƿ����
// 0  ��������������Ч
// -1 �����������ڻ���Ч
static int system_update_check (void)
{
	// ���ϵͳ�����ļ��Ƿ����
	FS_FILE *fp;
	int ret;
	char temp[16];
	char *update_package_info_base = NULL;
	XM_SYSTEMUPDATE_INFO *update_package_info = NULL;
	unsigned int sys_version, cur_version;
	
	XM_SYSTEMUPDATE_INFO *local_system_info = NULL;		// ���ذ汾��Ϣ
	
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
		
		// ����ļ��Ĵ�С
		unsigned int size = FS_GetFileSize (fp);
		if(size == (unsigned int)(-1))
		{
			// �޷���ȡ�ļ��ֽڴ�С
			break;
		}
		if(size >= XMSYS_SYSTEM_UPDATE_PACKAGE_SIZE || size < sizeof(XM_SYSTEMUPDATE_INFO))
		{
			// ����ϵͳ���������������ֽڴ�С
			XM_printf ("system_update_check failed, illegal size(%d) of \\update.bin great than the maximum size(%d)\n", size, XMSYS_SYSTEM_UPDATE_PACKAGE_SIZE);
			break;
		}

		if(size >= Spi_GetSize())
		{
			// ����ϵͳ���������������ֽڴ�С
			XM_printf ("XMSYS_system_update failed, file size(%d) of \\update.bin great than the size(%d) of SPI Flash\n", size, Spi_GetSize() );
			break;
		}

		// ��ȡ������ͷ������걸��
		update_package_info_base = (char *)kernel_malloc ((sizeof(XM_SYSTEMUPDATE_INFO) + 0x3F) & (~0x3F));	// ��cache line�������
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

		// �ļ�ͷ��Ϣ��ȡ��ȷ
		// �������������ȷ��
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
				
			
		// ������OK
		sys_version = update_package_info->brand_version;
		// ��ȡ������ϵͳ�汾��
		cur_version = 0xFFFFFFFF;		// 0xFFFFFFFF��ʾϵͳ�汾�Ų�����
		if(XMSYS_GetSystemSoftwareVersion ((unsigned char *)&cur_version) < 0)
		{
			XM_printf ("system_update_check failed, get software version io error\n");
			ret = -1;
			break;
		}

		XM_printf(">cur_version:%x\r\n", cur_version);
		XM_printf(">sys_version:%x\r\n", sys_version);
		// ����������� 
		// 	1) ������ϵͳ�汾�Ų����� ���� 2) SD������������ϵͳ�汾���Ƿ���ڱ�����ϵͳ�汾��
		if( (cur_version == 0xFFFFFFFF) || (sys_version != cur_version) )
		{
			// �����ֶ���װ�汾ȷ�ϳ�����Ϣ. �Զ���װ�汾�����ڲ�����, ������
			if(cur_version != 0xFFFFFFFF && sys_version != 0xFFFFFFFF)
			{
				XM_printf(">update_package_info->brand_type:%x\r\n", update_package_info->brand_type);
				XM_printf(">local_system_info->brand_type:%x\r\n", local_system_info->brand_type);
				XM_printf(">update_package_info->brand_id:%s\r\n", update_package_info->brand_id);
				XM_printf(">local_system_info->brand_id:%s\r\n", local_system_info->brand_id);
				
				// ��鳧����Ϣ�Ƿ�ƥ��
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
			// ׼��"����������֪ͨ"�첽��Ϣ
			// ��Ϣ������8���ֽ�
			// �汾(4�ֽ�)���ļ����ȣ�4�ֽڣ�
			//	������⵽SD���е�ϵͳ�����ļ�(�Ϸ��������ļ�)��֪ͨ������(��������ʱ��������ʱ���)
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
		// ֪ͨ����
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
		
		// ����ļ��Ĵ�С
		unsigned int size = FS_GetFileSize (fp);
		if(size == (unsigned int)(-1))
		{
			// �޷���ȡ�ļ��ֽڴ�С
			break;
		}
		if(size > PAGE_SIZE)
		{
			// ����ϵͳ���������������ֽڴ�С
			XM_printf ("XMSYS_system_update failed, illegal size(%d) of %s\n", size, file);
			break;
		}

		ret = FS_Read (fp, (char *)loader_buff, size);
		if(ret != size)
		{
			XM_printf ("load_boot failed, file io error\n");
			break;
		}

		// �ļ�ͷ��Ϣ��ȡ��ȷ
		// �������������ȷ��
		if(*(unsigned int *)(loader_buff + 4) != 0x4E313431)	// N141
		{
			XM_printf ("load_boot failed, file (%s) is illegal format\n", file);
			break;
		}
		
		// crc���
		unsigned short crc = crc16_ccitt ((char *)loader_buff, PAGE_SIZE - 2);
		if( *(unsigned short *)( ((char *)loader_buff) + PAGE_SIZE - 2) != crc )
		{
			XM_printf ("load_boot failed, file (%s) is illegal format\n", file);
			break;			
		}
			
		ret = 0;

		// ֪ͨ����
	} while(0);	

	if(fp)
		FS_FClose (fp);
	return ret;		
}

// ��SPI��ȡboot code
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
			// �ļ�ͷ��Ϣ��ȡ��ȷ
			// �������������ȷ��
			if(*(unsigned int *)(loader_buff + 4) != 0x4E313431)	// N141
			{
				XM_printf ("spi io error\n");
				break;
			}
			
			// crc���
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
		
		// ����ļ��Ĵ�С
		unsigned int size = FS_GetFileSize (fp);
		if(size == (unsigned int)(-1))
		{
			// �޷���ȡ�ļ��ֽڴ�С
			break;
		}
		if(size > PAGE_SIZE)
		{
			// ����ϵͳ���������������ֽڴ�С
			XM_printf ("XMSYS_system_update failed, illegal size(%d) of %s\n", size, file);
			break;
		}

		// ��ȡ������ͷ������걸��
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

		// �ļ�ͷ��Ϣ��ȡ��ȷ
		// �������������ȷ��
		if(*(unsigned int *)(update_package + 4) != 0x4E313431)	// N141
		{
			XM_printf ("update_load_boot failed, file (%s) is illegal format\n", file);
			break;
		}
		
		// crc���
		unsigned short crc = crc16_ccitt ((char *)update_package, PAGE_SIZE - 2);
		if( *(unsigned short *)( ((char *)update_package) + PAGE_SIZE - 2) != crc )
		{
			XM_printf ("update_load_boot failed, file (%s) is illegal format\n", file);
			break;			
		}
			
		// ��ʼ��������	
		XM_printf ("%s update start...\n", file);
		
		int loop;
		for (loop = 0; loop < 4; loop ++)
		{
			if(Spi_Write (0, (UINT8 *)update_package,   PAGE_SIZE) == 0)
				continue;
			// �ض��Ƚ�
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

		// ֪ͨ����
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
#define	LPAGE_SIZE		(PAGE_SIZE*16)//һ����д��һ��block,������ʱ�����д�����Ƚϲ��ɹ����������ʧ��
#define	REPEAT_COUNT	8

// ִ��ϵͳ����
// ����ֵ
//		0		ϵͳ�����ɹ�
//		-1		ϵͳ����ʧ��
static int system_update (void)
{
	// ���ϵͳ�����ļ��Ƿ����
	FS_FILE *fp;
	int ret;
	char temp[16];
	char *update_package = NULL;
	char *verify_buff = NULL;
	
	char *loader_buff = NULL;
	char *loader_verify_buff = NULL;
	
	unsigned int sys_version, cur_version;
	int loop;	// ѭ��������4��
	
	// ֹͣ�Զ�����
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
		
		// ����ļ��Ĵ�С
		unsigned int size = FS_GetFileSize (fp);
		if(size == (unsigned int)(-1))
		{
			// �޷���ȡ�ļ��ֽڴ�С
			break;
		}
		if(size >= XMSYS_SYSTEM_UPDATE_PACKAGE_SIZE || size < sizeof(XM_SYSTEMUPDATE_INFO))
		{
			// ����ϵͳ���������������ֽڴ�С
			XM_printf ("XMSYS_system_update failed, illegal size(%d) of \\update.bin great than the maximum size(%d)\n", size, XMSYS_SYSTEM_UPDATE_PACKAGE_SIZE);
			break;
		}
		
		if(size >= Spi_GetSize())
		{
			// ����ϵͳ���������������ֽڴ�С
			XM_printf ("XMSYS_system_update failed, file size(%d) of \\update.bin great than the size(%d) of SPI Flash\n", size, Spi_GetSize() );
			break;
		}

		// ��ȡ������ͷ������걸��
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

		// �ļ�ͷ��Ϣ��ȡ��ȷ
		// �������������ȷ��
		if(XM_CheckSystemUpdatePackage (update_package, size))
		{
			XM_printf ("XMSYS_system_update failed, illegal package\n");
			break;
		}
			
		// ������OK
		sys_version = ((XM_SYSTEMUPDATE_INFO *)update_package)->brand_version;
		// ��ȡ������ϵͳ�汾��
		cur_version = 0xFFFFFFFF;		// 0xFFFFFFFF��ʾϵͳ�汾�Ų�����
		if(XMSYS_GetSystemSoftwareVersion ((unsigned char *)&cur_version) < 0)
		{
			XM_printf ("XMSYS_system_update failed, illegal software version\n");
			break;			
		}
		// ����������� 
		// 	1) ������ϵͳ�汾�Ų����� ���� 2) SD������������ϵͳ�汾���Ƿ���ڱ�����ϵͳ�汾��
		if( (cur_version == 0xFFFFFFFF) || (sys_version != cur_version) )
		{
			//ret = 0;
		}
		else
		{
			XM_printf ("XMSYS_system_update failed, cur_version(%08x) >= sys_version(%08x)\n", cur_version, sys_version);
			break;
		}
		
		// ��ȡLOADSYS.BIN
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
			// ��SPI�ض���RAM, �Զ�ȡ�����ݽ��������Լ��
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
		
		// LOADSYS.BIN UPDATE.BIN ���Ѷ�ȡ�Ҽ��	
		
		// �ȴ�����UI׼��
		OS_Delay (50);
		
		// ��ʼ��������	
		XM_printf ("system update start...\n");
		
		// ���� LOADER����
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
				// �ض��Ƚ�
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
				
			// ������Ȱٷֱ�
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
			// �ض��Ƚ�
			if(Spi_Read  (0, (UINT8 *)loader_verify_buff, PAGE_SIZE) == 0)
				continue;
			if(memcmp (loader_buff, loader_verify_buff, PAGE_SIZE))
				continue;
			
			ret = 0;
			break;
		}
		
		if(ret != 0)
			XM_printf ("Program LOADSYS NG\n");
		
		// ֪ͨ����
	} while(0);	

	if(ret == 0)
	{			
		unsigned char step = 100;
		systemupdate_step = step;
		XM_printf ("update OK\n");
        AP_RestoreMenuData();//�����ɹ��ָ�Ĭ�ϲ���
		XMSYS_MessageSocketTransferResponsePacket (XMSYS_MESSAGE_SOCKET_TYPE_ASYC,
															 -1,
															XMSYS_NID_SYSTEM_UPDATE_STEP,
															&step,
															1
															);
	}
	else
	{
		// ����ʧ��
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



#define	XMSYS_SYSTEM_UPDATE_EVENT_CHECK		0x01		// ���������
#define	XMSYS_SYSTEM_UPDATE_EVENT_UPDATE		0x02		// ִ������
#define	XMSYS_SYSTEM_UPDATE_EVENT_TIMER		0x04		// ��ʱ��, ��������֪ͨ

__no_init static OS_STACKPTR int StackSystemUpdate[XMSYS_SYSTEM_UPDATE_TASK_STACK_SIZE/4];          /* Task stacks */
static OS_TASK TCB_SystemUpdate;                        /* Task-control-blocks */

static void SystemUpdateTask(void)
{
	while(1)
	{
		OS_U8 system_update_event = 0;
		
		system_update_event = OS_WaitEvent (XMSYS_SYSTEM_UPDATE_EVENT_CHECK|XMSYS_SYSTEM_UPDATE_EVENT_UPDATE|XMSYS_SYSTEM_UPDATE_EVENT_TIMER);
		
		// ���������
		if(system_update_event & XMSYS_SYSTEM_UPDATE_EVENT_CHECK )
		{
			unsigned char old_priority;
			OS_TASK *pCurrentTask;
			XM_printf(">>>>>>>>>>>>SYSTEM_UPDATE_EVENT_CHECK......\r\n");
			notify_system_update_event = 0;
			pCurrentTask = OS_GetpCurrentTask();
			old_priority = OS_GetPriority (pCurrentTask);
			OS_SetPriority(pCurrentTask, 253);//�ı��������ȼ�
			system_update_check ();
			OS_SetPriority(pCurrentTask, old_priority);//�ָ��������ȼ�
		}
		// ϵͳ����
		else if(system_update_event & XMSYS_SYSTEM_UPDATE_EVENT_UPDATE )
		{
			XM_printf ("SYSTEM_UPDATE_EVENT_UPDATE\n");
			notify_system_update_event = 0;
			if(system_update () < 0)
			{
				// ���������Ϣ
				systemupdate_version = 0;
			}
		}
		
		if(system_update_event & XMSYS_SYSTEM_UPDATE_EVENT_TIMER )
		{
			//XM_printf(">>>>>>>>>>>systemupdate_version:%x\r\n", systemupdate_version);
			//XM_printf(">>>>>>>>>>>notify_system_update_event:%x\r\n", notify_system_update_event);
			// ����Զ��������Ƿ����. ������, ��ʱ�����淢����Ϣ
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

// ���������
int XMSYS_system_update_check (void)
{
	OS_SignalEvent (XMSYS_SYSTEM_UPDATE_EVENT_CHECK, &TCB_SystemUpdate); /* ֪ͨ�¼� */
	return 0;
}

// ��������LCDC���Զ�����ʱ��
// 0xFFFFFFFF ��ʾ��������
// ����ֵ ��ʾ�Զ�������ʱ��(����), 
void hw_backlight_set_auto_off_ticket (unsigned int ticket);

// ϵͳ����
// ����ֵ
//		0		ϵͳ�����ɹ�
//		-1		ϵͳ����ʧ��
int XMSYS_system_update (void)
{
	unsigned char step;
	unsigned int ticket_to_timeout;
	
	hw_backlight_set_auto_off_ticket (0xFFFFFFFF);	// ��ֹ�رձ���
	
	OS_Use(&systemupdate_sema); /* Make sure nobody else uses */
	systemupdate_step = 0;	// 0%���״̬
	OS_SignalEvent (XMSYS_SYSTEM_UPDATE_EVENT_UPDATE, &TCB_SystemUpdate); /* ֪ͨ�¼� */
	OS_Unuse(&systemupdate_sema);
	
	OS_Delay (300);		// �ȴ�֪ͨ�¼��������
	
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
		
		// ����Ƿ�ʱ
		if(XM_GetTickCount() >= ticket_to_timeout)
		{
			step = 0xff;
			break;
		}
		
		OS_Delay (1);
	} while ( 1 );
	
	// ȷ�����״̬��Ϣ�ɿ��ķ��͸�����
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
	else 	// 0xff��ʾʧ��
		return -1;
}

// ��ȡϵͳ�������̵�ǰ�Ľ׶�, 
// 0 ~ 100		��ʾ������ɵİٷֱ�
// 0xFF			��ʾ������ʧ��
unsigned int XMSYS_system_update_get_step (void)
{
	return systemupdate_step;
}

// ���ϵͳ�����Ƿ�ִ����
// ����ֵ 
// 1   ����ִ����
// 0   û��ִ��
unsigned int XMSYS_system_update_busy_state (void)
{
	return systemupdate_state;
}

// ��ȡ������ϵͳ�İ汾
// 0xFFFFFFFFΪ�Զ���װ�汾, �����û�ȷ��, �����ڲ�����
unsigned int XMSYS_system_update_get_version (void)
{
	return systemupdate_version;
}

static void UpdateNotificationTicketCallback (void)
{
	OS_SignalEvent(XMSYS_SYSTEM_UPDATE_EVENT_TIMER, &TCB_SystemUpdate); /* ֪ͨ�¼� */
	OS_RetriggerTimer(&UpdateNotificationTimer);
}

void XMSYS_SystemUpdateInit (void)
{
	OS_CREATERSEMA (&systemupdate_sema); /* Creates resource semaphore */
	
	// ����1hz�Ķ�ʱ��
	OS_CREATETIMER (&UpdateNotificationTimer, UpdateNotificationTicketCallback, 1000);
	
	// ϵͳ�����̵߳����ȼ���UI�߳����ȼ�һ��, ����UI�������п���ˢ����ʾ, ͬʱϵͳ�����߳̿��ԽϿ���Ӧ�����¼�
	OS_CREATETASK(&TCB_SystemUpdate, "SystemUpdate", SystemUpdateTask, XMSYS_SYSTEM_UPDATE_TASK_PRIORITY, StackSystemUpdate);
}

void XMSYS_SystemUpdateExit (void)
{
	OS_DeleteRSema (&systemupdate_sema); 
}
