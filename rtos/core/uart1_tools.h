/*
**********************************************************************
Copyright (c)2007 Arkmicro Technologies Inc.  All Rights Reserved 
Filename: uart.h
Version : 1.1 
Date    : 2008.01.08
Author  : Salem
Abstract: ark1610 soc uart driver
History :
***********************************************************************
*/

#ifndef _UART_TOOLS_H_
#define _UART_TOOLS_H_

#include "types.h"

//void uart0_init(unsigned int baud);
//void uart0_send_char(char ch);
//void uart0_send_string(char * buf);


/*******************************************************************************

********************************************************************************/

/*******************************************************************************
*      
********************************************************************************/
#define	 RECEIVE_BUF_LEN	            2048
#define	 SEND_BUF_LEN		            2048
#define   UART_Write_Packagesize       1024

#define   UART1_FIFO_SIZE              8
#define   data_buf_size                512 // 与  PACKAGEDATASIZE 应该相等，

#define   packagestatcksize            4
#define   PACKAGEDATASIZE              512 // 与  data_buf_size 应该相等
/*******************************************************************************
*  数据包 结构     
********************************************************************************/
typedef struct
{
	UINT32		   packageno	;// 包序号
	UINT16		   packagetype;        // 包 类型
	INT16		   packagedatalength	;// 数据区域的数据长度
	UINT8          *packagedata ;// 包数据区域
	UINT8          *name;
	UINT8		   cmd;         // 命令字
	UINT8          receivedfinished; // 包接收完成 ，真=1
	UINT8          transmitfinished; // 包传输完成
	UINT8          reserved; // 保留作为32bit对齐，填充
}UART_PACKAGE;


/*******************************************************************************
*  数据处理
********************************************************************************/
#define STATE_LEAD_L             0x00
#define STATE_LEAD_H             0x01
#define STATE_GET_ENCRYPT        0x02
#define STATE_GET_COMMAND        0x03
#define STATE_GET_MACNO          0x04
#define STATE_GET_PACKAGELENGTH  0x05
#define STATE_GET_PACKAGENO      0x06
#define STATE_GET_PACKAGEDATA    0x07
#define STATE_VERIFY             0x08


/*******************************************************************************
*  命令字
********************************************************************************/
#define  COMM_SYS_CONNECT               0xF0
#define  COMM_SYS_DISCONNECT            0xF1
#define  COMM_SYS_UPGRADE               0xF2
#define  COMM_SYS_IOREAD                0xF5
#define  COMM_SYS_IOWRITE               0xF6

#define  COMM_SYS_I2COPEN               0xF7
#define  COMM_SYS_I2CCLOSE              0xF8
#define  COMM_SYS_I2CREAD               0xF9
#define  COMM_SYS_I2CWRITE              0xFA
#define  COMM_SYS_ISPRESET              0xFB
#define  COMM_SYS_LCDMOVE               0xFC
#define  COMM_SYS_WFILE                 0xFD
#define  COMM_SYS_ISPCOLOR              0xFE // 选择isp 的 rgb2yuv洗漱 
#define  COMM_SYS_VERSION               0xFF // 版本 

#define	COMM_SYS_ISP_AUTORUN				 0xD0				// 设置ISP auto-run的状态
#define	COMM_SYS_ISP_SET_WORK_MODE		 0xD1				// 设置ISP工作模式

#define	COMM_SYS_ISP_GET_CURRENT_EXP	 0xD2				// 获取当前的曝光参数
#define	COMM_SYS_ISP_SET_MAX_GAIN		 0xD3				// 设置最大增益
#define	COMM_SYS_ISP_GET_MAX_GAIN		 0xD4				// 获取最大增益设置
#define	COMM_SYS_ISP_SET_CURRENT_EXP	 0xD5				// 设置当前的曝光参数
#define	COMM_SYS_ISP_STEADY_AE			 0xD6				// ISP稳态AE控制开启/关闭

#define  COMM_SYS_AUTOAE                0xE0 
#define  COMM_SYS_SETAE                 0xE1 
#define  COMM_SYS_GETAE                 0xE2 
#define  COMM_SYS_IOWRITE_BAK           0xE3 
#define  COMM_SYS_SET_BIGLUMWF          0xE4

#define  COMM_SYS_SENSOR_READOUT_DIRECTION	0xE5		// sensor读出方向设置
#define	COMM_SYS_LCD_GAMMA_SETTING		 0xE6				// LCD gamma设置 (0.5 ~ 2)
#define	COMM_SYS_ISP_AE_SET				 0xE7				// 设置当前自动曝光控制参数
#define	COMM_SYS_ISP_AE_GET				 0xE8				// 获得当前自动曝光控制参数

#define	COMM_SYS_ISP_AE_MULTI_EV_WRITE		0xE9				// 多次曝光写入
#define  COMM_SYS_I2C_ISPSCALERESET     0xEA				// ISP Scale 复位
#define	COMM_SYS_ISP_SCALAR_WINDOW_SETTING    0xEB				// 设置源及目标窗口
#define	COMM_SYS_ISP_SCALAR_VIDEO_SETTING		0xEC
#define	COMM_SYS_ISP_SET_VIDEO_FORMAT	 0xED				// ISP设置YUV视频输出格式	(0：y_uv420 1:y_uv422 2:yuv420 3:yuv422 )	
#define	COMM_SYS_ISP_AE_INFO_HIDE		 0xEE				// ISP自动曝光信息输出关闭

#define	COMM_SYS_ISP_GEM_LUT_WRITE		 0xEF				// ISP LUT写入

#define  COMM_SET_TIME                  0x00
#define  COMM_SET_VIDEODURATION         0x01
#define  COMM_SET_RECORD                0x02
#define  COMM_AUDIO_RECORD              0x02
#define  COMM_READ_RECORDSTATUS         0x03


#define  COMM_VIDEO_START               0x10
#define  COMM_VIDEO_STOP                0x11
#define  COMM_VIDEO_SWITCH              0x12
#define  COMM_VIDEO_ONEKEY_PROTECT      0x13
#define  COMM_VIDEO_ONEKEY_PHOTO        0x14

#define  COMM_FILE_NUM                  0x20
#define  COMM_FILE_LIST                 0x21

#define  COMM_REPLAY_START              0x22
#define  COMM_REPLAY_PAUSE              0x23
#define  COMM_REPLAY_STOP               0x24
#define  COMM_REPLAY_FORWARD            0x25
#define  COMM_REPLAY_BACKWARD           0x26

#define  COMM_SD_STATUS                 0x30
#define  COMM_SD_VOLUME                 0x31
#define  COMM_SD_FORMAT                 0x32
#define  COMM_SD_FILECHECK              0x33
#define  COMM_SD_FILEDELETE             0x34
#define  COMM_SD_FILELOCK               0x35

#define  COMM_DEVICE_CONFIG             0x81

/*******************************************************************************
*  异步命令字
********************************************************************************/
#define  COMM_ASY_VIDEO_OUTPUT          0x0
#define  COMM_ASY_REPLAY_FINISH         0x1
#define  COMM_ASY_SD_VOL                0x2
#define  COMM_ASY_SD_FORMAT_FINISH      0x3
#define  COMM_ASY_SD_FILEDELETE_FINISH  0x4
#define  COMM_ASY_SD_REMOVED            0x5
#define  COMM_ASY_SD_INSERT             0x6
#define  COMM_ASY_SD_FULL               0x7
#define  COMM_ASY_SD_BROKEN             0x8
#define  COMM_ASY_SD_FSERROR            0x9

#define  COMM_ASY_SD_FILELISTUPDATA     0xA
#define  COMM_ASY_SLAVE_UPDATANOTIFY    0xB
#define  COMM_ASY_SLAVE_UPDATASTATUS    0xC

/*******************************************************************************
* 包类型
********************************************************************************/
#define  PACKAGE_TYPE_Unknown               0x0
#define  PACKAGE_TYPE_CMD                   0x00ff
#define  PACKAGE_TYPE_ANSWER                0xff00
#define  PACKAGE_TYPE_ASYN                  0xaa55

/*******************************************************************************
* 应答类型
********************************************************************************/
#define  REPLY_ACK                       0x0
#define  REPLY_NAK                       0x1

#define  SLAVE_PACKAGE_ANSWERED          0x1
#define  SLAVE_PACKAGE_NOT_ANSWER        0x2
/*******************************************************************************/


void Uart_package_param_init();
void handle_uart_package( UART_PACKAGE  *Uart_Package  );



int write_uart1_data(  UINT16  packagetype, // package type 
                          UINT8   cmd,         // cid
                          UINT32  packageno_for_cmd_reply, // the package no of [current cmd] 
                          void   *data,        // data
                          UINT16  len   );      // data lenght



void updata_packagestatck(void);


#endif
