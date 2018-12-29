//****************************************************************************
//
//	Copyright (C) 2010 ShenZhen ExceedSpace
//
//	Author	ZhuoYongHong
//
//	File name: xm_systemupdate.h
//	  系统升级包数据结构及接口
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

#define XMSYS_SYSTEMUPDATE_VERSION_1_0		0x00010000		// 版本1.0
#define XMSYS_SYSTEMUPDATE_VERSION_1_1		0x00010001		// 版本1.1
#define XMSYS_SYSTEMUPDATE_VERSION_2_1		0x00020001		// 版本2.1

#define XM_SYSTEMUPDATE_BRAND_ID_XSPACE			"XSPACE"
#define XM_SYSTEMUPDATE_BRAND_ID_ARKMICRO			"ARKMICRO"
#define	MAX_SYSTEMUPDATE_HEADER_SIZE			1024

#define XMSYS_ROMBIN_INFO_ID					0x4e494252		// "RBIN"
#define XMSYS_ROMBIN_INFO_VERSION_1_0		0x00010000		// 版本1.0

// 20180325 zhuoyonghong
// ROM.BIN信息结构头定义
typedef struct tagXM_ROMBIN_INFO {
	unsigned int		id;					//  0 ROM.BIN标识，必须为"RBIN"
	unsigned int		version;			//  4 版本号 1.0
	unsigned int		binary_offset;		//  8 rom.bin相对于ROM.BIN信息结构头开始的偏移
	unsigned int		binary_length;		// 12 rom.bin字节长度
	unsigned int		binary_checksum;	// 16 rom.bin数据32位checksum校验和
	unsigned char		reserved[12];		// 20 保留字段，填充为0
} XM_ROMBIN_INFO;
		
// 固定为1024字节大小
typedef struct tagXM_SYSTEMUPDATE_INFO_1_0 {
	unsigned int		id;						// 系统升级信息结构的标识，必须为"XMSU"
	unsigned int		version;					// 系统升级信息结构的版本号
	unsigned int		length;					// 系统升级信息结构的字节长度, 32字节大小对齐(不足补0)
														//		(XM_SYS_SYSTEMUPDATE_INFO 的长度, 不包含二进制数据字段)
	unsigned int		checksum;				// 系统升级信息结构的32bit累加和
														//		= 0xAAAAAAAA - 其他字段的32bit累加和(不包括checksum字段)

	unsigned short		binary_offset;			// 二进制数据相对于系统升级信息结构头开始的偏移
	unsigned short		binary_crc;				// 二进制数据的CRC16校验和
	unsigned int		binary_length;			// 二进制数据的字节长度
	
	// 以下前三个信息字段将用于升级版本检查
	unsigned char		brand_id[32];			// 开发商ID(厂家自定)
	unsigned int		brand_type;				// 产品型号(厂家自定)
	unsigned int		brand_version;			//	产品版本号(厂家自定，新版本号 > 老版本号)
	unsigned char		update_comment[512-64];// 升级信息简单描述，未使用的内容填充0

	unsigned char		reserved[512];			// 保留至少512字节用于后续扩展，填充0
														//		通过增加length字段修改保留区字节大小
} XM_SYSTEMUPDATE_INFO_1_0;

typedef struct tagXM_SYSTEMUPDATE_INFO {
	unsigned int		id;						// 系统升级信息结构的标识，必须为"XMSU"
	unsigned int		version;					// 系统升级信息结构的版本号
	unsigned int		length;					// 系统升级信息结构的字节长度, 32字节大小对齐(不足补0)
														//		(XM_SYS_SYSTEMUPDATE_INFO 的长度, 不包含二进制数据字段)
	unsigned int		checksum;				// 系统升级信息结构的32bit累加和
														//		= 0xAAAAAAAA - 其他字段的32bit累加和(不包括checksum字段)

	unsigned short		binary_offset;			// 二进制数据相对于系统升级信息结构头开始的偏移
	unsigned short		binary_crc;				// 二进制数据的CRC16校验和
	unsigned int		binary_length;			// 二进制数据的字节长度
	
	// 以下前三个信息字段将用于升级版本检查
	unsigned char		brand_id[32];			// 开发商ID(厂家自定)
	unsigned int		brand_type;				// 产品型号(厂家自定)
	unsigned int		brand_version;			//	产品版本号(厂家自定，新版本号 > 老版本号)
	unsigned char		update_comment[512-64];// 升级信息简单描述，未使用的内容填充0

	// 1.1新增rom.bin支持
	XM_ROMBIN_INFO		rom_bin_info;
	unsigned char		reserved[512 - 32];	// 保留至少512-32字节用于后续扩展，填充0
														//		通过增加length字段修改保留区字节大小
} XM_SYSTEMUPDATE_INFO;


// 检查升级包
int XMSYS_system_update_check (void);

// 系统升级
int XMSYS_system_update (void);

// 获取系统升级过程当前的阶段, 
// 0 ~ 100		表示升级完成的百分比
// 0xFF			表示升级已失败
unsigned int XMSYS_system_update_get_step (void);

// 检查系统升级是否执行中
// 返回值 
// 1   正在执行中
// 0   没有执行
unsigned int XMSYS_system_update_busy_state (void);


// 获取系统软件版本
// 返回值  
//		-1		失败
//		0		成功
int XMSYS_GetSystemSoftwareVersion (unsigned char sw_version[4]);

// 获取待升级系统的版本
// 0xFFFFFFFF为自动安装版本, 无需用户确认, 用于内部测试
unsigned int XMSYS_system_update_get_version (void);

void XMSYS_SystemUpdateInit (void);

void XMSYS_SystemUpdateExit (void);

#if defined (__cplusplus)
	}
#endif		/* end of __cplusplus */


#endif	// _XMSYS_SYSTEM_UPDATE_H_