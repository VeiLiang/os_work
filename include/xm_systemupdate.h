//****************************************************************************
//
//	Copyright (C) 2010 ShenZhen ExceedSpace
//
//	Author	ZhuoYongHong
//
//	File name: xm_systemupdate.h
//	  ϵͳ���������ݽṹ���ӿ�
//
//	Revision history
//
//		2013.09.01	ZhuoYongHong Initial version
//
//****************************************************************************

#ifndef _XMSYS_SYSTEM_UPDATE_H_
#define _XMSYS_SYSTEM_UPDATE_H_

#if defined (__cplusplus)
	extern "C"{
#endif

#define XMSYS_SYSTEMUPDATE_ID					0x55534D58		// "XMSU"

#define XMSYS_SYSTEMUPDATE_VERSION_1_0		0x00010000		// �汾1.0
#define XMSYS_SYSTEMUPDATE_VERSION_1_1		0x00010001		// �汾1.1
#define XMSYS_SYSTEMUPDATE_VERSION_2_1		0x00020001		// �汾2.1

#define XM_SYSTEMUPDATE_BRAND_ID_XSPACE			"XSPACE"
#define XM_SYSTEMUPDATE_BRAND_ID_ARKMICRO			"ARKMICRO"
#define	MAX_SYSTEMUPDATE_HEADER_SIZE			1024

#define XMSYS_ROMBIN_INFO_ID					0x4e494252		// "RBIN"
#define XMSYS_ROMBIN_INFO_VERSION_1_0		0x00010000		// �汾1.0

// 20180325 zhuoyonghong
// ROM.BIN��Ϣ�ṹͷ����
typedef struct tagXM_ROMBIN_INFO {
	unsigned int		id;					//  0 ROM.BIN��ʶ������Ϊ"RBIN"
	unsigned int		version;			//  4 �汾�� 1.0
	unsigned int		binary_offset;		//  8 rom.bin�����ROM.BIN��Ϣ�ṹͷ��ʼ��ƫ��
	unsigned int		binary_length;		// 12 rom.bin�ֽڳ���
	unsigned int		binary_checksum;	// 16 rom.bin����32λchecksumУ���
	unsigned char		reserved[12];		// 20 �����ֶΣ����Ϊ0
} XM_ROMBIN_INFO;
		
// �̶�Ϊ1024�ֽڴ�С
typedef struct tagXM_SYSTEMUPDATE_INFO_1_0 {
	unsigned int		id;						// ϵͳ������Ϣ�ṹ�ı�ʶ������Ϊ"XMSU"
	unsigned int		version;					// ϵͳ������Ϣ�ṹ�İ汾��
	unsigned int		length;					// ϵͳ������Ϣ�ṹ���ֽڳ���, 32�ֽڴ�С����(���㲹0)
														//		(XM_SYS_SYSTEMUPDATE_INFO �ĳ���, �����������������ֶ�)
	unsigned int		checksum;				// ϵͳ������Ϣ�ṹ��32bit�ۼӺ�
														//		= 0xAAAAAAAA - �����ֶε�32bit�ۼӺ�(������checksum�ֶ�)

	unsigned short		binary_offset;			// ���������������ϵͳ������Ϣ�ṹͷ��ʼ��ƫ��
	unsigned short		binary_crc;				// ���������ݵ�CRC16У���
	unsigned int		binary_length;			// ���������ݵ��ֽڳ���
	
	// ����ǰ������Ϣ�ֶν����������汾���
	unsigned char		brand_id[32];			// ������ID(�����Զ�)
	unsigned int		brand_type;				// ��Ʒ�ͺ�(�����Զ�)
	unsigned int		brand_version;			//	��Ʒ�汾��(�����Զ����°汾�� > �ϰ汾��)
	unsigned char		update_comment[512-64];// ������Ϣ��������δʹ�õ��������0

	unsigned char		reserved[512];			// ��������512�ֽ����ں�����չ�����0
														//		ͨ������length�ֶ��޸ı������ֽڴ�С
} XM_SYSTEMUPDATE_INFO_1_0;

typedef struct tagXM_SYSTEMUPDATE_INFO {
	unsigned int		id;						// ϵͳ������Ϣ�ṹ�ı�ʶ������Ϊ"XMSU"
	unsigned int		version;					// ϵͳ������Ϣ�ṹ�İ汾��
	unsigned int		length;					// ϵͳ������Ϣ�ṹ���ֽڳ���, 32�ֽڴ�С����(���㲹0)
														//		(XM_SYS_SYSTEMUPDATE_INFO �ĳ���, �����������������ֶ�)
	unsigned int		checksum;				// ϵͳ������Ϣ�ṹ��32bit�ۼӺ�
														//		= 0xAAAAAAAA - �����ֶε�32bit�ۼӺ�(������checksum�ֶ�)

	unsigned short		binary_offset;			// ���������������ϵͳ������Ϣ�ṹͷ��ʼ��ƫ��
	unsigned short		binary_crc;				// ���������ݵ�CRC16У���
	unsigned int		binary_length;			// ���������ݵ��ֽڳ���
	
	// ����ǰ������Ϣ�ֶν����������汾���
	unsigned char		brand_id[32];			// ������ID(�����Զ�)
	unsigned int		brand_type;				// ��Ʒ�ͺ�(�����Զ�)
	unsigned int		brand_version;			//	��Ʒ�汾��(�����Զ����°汾�� > �ϰ汾��)
	unsigned char		update_comment[512-64];// ������Ϣ��������δʹ�õ��������0

	// 1.1����rom.bin֧��
	XM_ROMBIN_INFO		rom_bin_info;
	unsigned char		reserved[512 - 32];	// ��������512-32�ֽ����ں�����չ�����0
														//		ͨ������length�ֶ��޸ı������ֽڴ�С
} XM_SYSTEMUPDATE_INFO;


// ���������
int XMSYS_system_update_check (void);

// ϵͳ����
int XMSYS_system_update (void);

// ��ȡϵͳ�������̵�ǰ�Ľ׶�, 
// 0 ~ 100		��ʾ������ɵİٷֱ�
// 0xFF			��ʾ������ʧ��
unsigned int XMSYS_system_update_get_step (void);

// ���ϵͳ�����Ƿ�ִ����
// ����ֵ 
// 1   ����ִ����
// 0   û��ִ��
unsigned int XMSYS_system_update_busy_state (void);


// ��ȡϵͳ����汾
// ����ֵ  
//		-1		ʧ��
//		0		�ɹ�
int XMSYS_GetSystemSoftwareVersion (unsigned char sw_version[4]);

// ��ȡ������ϵͳ�İ汾
// 0xFFFFFFFFΪ�Զ���װ�汾, �����û�ȷ��, �����ڲ�����
unsigned int XMSYS_system_update_get_version (void);

void XMSYS_SystemUpdateInit (void);

void XMSYS_SystemUpdateExit (void);

#if defined (__cplusplus)
	}
#endif		/* end of __cplusplus */


#endif	// _XMSYS_SYSTEM_UPDATE_H_