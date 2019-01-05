/*
**********************************************************************
Copyright (c)2014 SHENZHEN Exceedspace Company.  All Rights Reserved
Filename: 
Version : 
Date    : 
Author  : tangchao
Abstract: 
History :
***********************************************************************
*/
#include <hardware.h>
#include "RTOS.h"		// OS头文件
#include "hw_osd_layer.h"
#include "uart1_tools.h"
//include "i2c.h"
#include <stdio.h>
#include <string.h>
//#include "crc16.h"
#include "xm_core.h"
#include "xm_uart.h"
#include "arkn141_isp.h"

#define	Gem_write	Gem_io_write
#define	Gem_read		Gem_io_read

extern unsigned char checksum_byte( const char *buf, unsigned int len);
extern int xm_uvc_set_window (unsigned int src_x, unsigned int src_y, unsigned int src_w, unsigned int src_h);
extern void cmos_exposure_set_steady_ae (int enable);



#define MAX_UART_SOCKET_LENGTH	512

// cache line aligned (Cortex A5's cache line size = 32)
#pragma data_alignment=32
static u8_t data_buf[MAX_UART_SOCKET_LENGTH + 2];
#pragma data_alignment=32
static u8_t resp_buf[MAX_UART_SOCKET_LENGTH];

xm_uart_dev_t uart_socket_dev = NULL;
#pragma data_alignment=32
static OS_TASK TCB_UART_socket_Task;
#pragma data_alignment=32
static OS_STACKPTR int StackUART_socket[0x1000/4];    
static   UINT32       packageno_ASYN    =0;
#pragma data_alignment=32
UART_PACKAGE sendpack ={0};
#pragma data_alignment=32
static unsigned char sendpackdata[256];
static volatile UINT32       connect_status=COMM_SYS_DISCONNECT;

//#define EN_DEBUG_UART

/*
   处理数据包
*/
int  HandlePacket(unsigned int message_no,unsigned int message_id,unsigned char *command_packet_buffer,unsigned int command_packet_length)
{
	unsigned int ret ;
	UART_PACKAGE pkg;
	UART_PACKAGE *package = &pkg;
	unsigned int addr;
	unsigned char *ch,*sendch,*data;
	
	unsigned int val ; 

	unsigned char i=0;

	sendpack.packagedata = sendpackdata;
	ch = sendpack.packagedata;
	
	package->packageno   = message_no;
	package->cmd         = message_id;
	package->packagedata = command_packet_buffer;
	package->packagedatalength = command_packet_length;

	//printf(">>>>package->packageno:%x\r\n",package->packageno);
	//printf(">>>>package->cmd:%x\r\n",package->cmd);

	#if 0
	printf(">>>>command_packet_length:%x\r\n",package->packagedatalength);
	{
		u8_t i;

		for(i=0; i<package->packagedatalength; i++)
		{
			printf("i:%d, %x\r\n", i, *(package->packagedata+i));
		}
	}
	#endif
	
	{
		{
			ret = 1;
			//printf("Uart1 received one package.cmd=0x%02x\r\n", package->cmd );
			data = package->packagedata;
			if( package->cmd == COMM_SYS_CONNECT )
			{
            	connect_status = COMM_SYS_CONNECT;
				printf(">>>>Uart2 connect.\r\n");
				// reply one ok 
				ch[0] = 0;//ACK;
				sendpack.packageno = package->packageno;
				sendpack.packagedatalength = 1;
				sendpack.cmd = package->cmd;
				write_uart1_data(PACKAGE_TYPE_ANSWER, sendpack.cmd,sendpack.packageno,ch,sendpack.packagedatalength);
				ret = 0;				
			}
			else if( package->cmd == COMM_SYS_DISCONNECT )
			{
				printf(">>>>Uart2 Disconnect.\r\n");
				package->receivedfinished = 0; 
				return ret ;
			}
#if 0
         // 后面只处理已经连接之后 收到的数据包
			if( connect_status != COMM_SYS_CONNECT )
			{
				printf("Uart1 Disconnect.\r\n");
				package->receivedfinished = 0; 
				return ret ;
			}
#endif    
#if 0
			else if( package->cmd == COMM_SYS_VERSION  )// 0xff
			{
				// 只处理 数据长度为 4 字节的包
				// 一个四字节地址， 返回四字节数值
				if( package->packagedatalength == 0 )
				{
               		unsigned int version = 0x12345678;
					sendch = sendpack.packagedata;
					//memcpy(sendch, &addr , 4);// copy addr
					sendch[0] = version&0xff;
					sendch[1] = (version>>8)&0xff;
					sendch[2] = (version>>16)&0xff;
					sendch[3] = (version>>24)&0xff;

					
					sendpack.packageno = package->packageno;
					sendpack.packagedatalength = 4;
					sendpack.cmd = package->cmd;
					write_uart1_data(PACKAGE_TYPE_ANSWER, 
										  sendpack.cmd,
										  sendpack.packageno,
										  sendpack.packagedata,
										  sendpack.packagedatalength);
					ret = 0;
				}
			}
         else if( package->cmd == COMM_DEVICE_CONFIG )
         {
				if( package->packagedatalength == 4 )
				{
               		ch[0] = 0;//ACK;
					sendpack.packageno = package->packageno;
					sendpack.packagedatalength = 1;
					sendpack.cmd = package->cmd;
					write_uart1_data(PACKAGE_TYPE_ANSWER, 
										  sendpack.cmd,
										  sendpack.packageno,
										  sendpack.packagedata,
										  sendpack.packagedatalength);
					ret = 0;
				}
         }
#endif
			else if( package->cmd == COMM_SYS_IOREAD  )// 0xf5
			{
				printf(">>>>COMM_SYS_IOREAD....\r\n");
				// 只处理 数据长度为 4 字节的包
				// 一个四字节地址， 返回四字节数值
				if( package->packagedatalength == 4 )
				{
					printf(">>>>read reg....\r\n");
					addr = (data[0]<<0)|(data[1]<<8)|(data[2]<<16)|(data[3]<<24) ;
					val = *(unsigned int *)addr;
					printf("read :addr=0x%08x val=0x%08x\r\n",  addr, val);
					sendch = sendpack.packagedata;
					//memcpy(sendch, &addr , 4);// copy addr
					sendch[0] = addr&0xff;
					sendch[1] = (addr>>8)&0xff;
					sendch[2] = (addr>>16)&0xff;
					sendch[3] = (addr>>24)&0xff;
					//memcpy(sendch+4,&val, 4);// copy value
					sendch[4] = val&0xff;
					sendch[5] = (val>>8)&0xff;
					sendch[6] = (val>>16)&0xff;
					sendch[7] = (val>>24)&0xff;
					
					sendpack.packageno = package->packageno;
					sendpack.packagedatalength = 8;
					sendpack.cmd = package->cmd;
					write_uart1_data(PACKAGE_TYPE_ANSWER, sendpack.cmd,sendpack.packageno,sendpack.packagedata,sendpack.packagedatalength);
					ret = 0;
				}
				else if( package->packagedatalength == 4*49 )//read gamma
				{
					printf(">>>>read gamma....\r\n");
					sendch = sendpack.packagedata;
					//memcpy(sendch, &addr , 4);// copy addr
					//sendch[0] = addr&0xff;
					//sendch[1] = (addr>>8)&0xff;
					//sendch[2] = (addr>>16)&0xff;
					//sendch[3] = (addr>>24)&0xff;
					//memcpy(sendch+4,&val, 4);// copy value  
					for(i=0;i<49;i++)
					{
						val = *(unsigned int *)(0x7019013c+4*i);
						sendch[0+4*i] = val&0xff;
						sendch[1+4*i] = (val>>8)&0xff;
						sendch[2+4*i] = (val>>16)&0xff;
						sendch[3+4*i] = (val>>24)&0xff;
					}
						
					sendpack.packageno = package->packageno;
					sendpack.packagedatalength = 4*49;
					sendpack.cmd = package->cmd;
					write_uart1_data(PACKAGE_TYPE_ANSWER, 
										  sendpack.cmd,
										  sendpack.packageno,
										  sendpack.packagedata,
										  sendpack.packagedatalength);
					ret = 0;
				}
				
			}
			else if(package->cmd == COMM_SYS_IOWRITE   )//oxf6
			{
				printf(">>>>COMM_SYS_IOWRITE....\r\n");
				// 只处理 数据长度为 8 字节的包
				// 一个四字节地址，一个四字节数值
				if(package->packagedatalength == 8 )
				{
					printf(">>>>write reg....\r\n");

					addr = (data[0]<<0)|(data[1]<<8)|(data[2]<<16)|(data[3]<<24) ;
					val  = (data[4]<<0)|(data[5]<<8)|(data[6]<<16)|(data[7]<<24) ;
                                        io_write( addr, val );
					//*(volatile unsigned int *)addr = val;
					printf("write:addr=0x%08x val=0x%08x\r\n", addr, val);
					// reply one ok 
					ch[0] = 0;//ACK;
					sendpack.packageno = package->packageno;
					sendpack.packagedatalength = 1;
					sendpack.cmd = package->cmd;
					write_uart1_data(PACKAGE_TYPE_ANSWER, 
										  sendpack.cmd,
										  sendpack.packageno,
										  sendpack.packagedata,
										  sendpack.packagedatalength);
					ret = 0;
				}
				else if(package->packagedatalength == 8*48 )
				{
					printf(">>>>write gamma....\r\n");
					for(i=0;i<48;i++)
					{
						addr = (data[0+8*i]<<0)|(data[1+8*i]<<8)|(data[2+8*i]<<16)|(data[3+8*i]<<24) ;
						val  = (data[4+8*i]<<0)|(data[5+8*i]<<8)|(data[6+8*i]<<16)|(data[7+8*i]<<24) ;
						io_write( addr, val );
					}
					//*(volatile unsigned int *)addr = val;
					//printf("write:addr=0x%08x val=0x%08x\r\n", addr, val);
					// reply one ok 
					ch[0] = 0;//ACK;
					sendpack.packageno = package->packageno;
					sendpack.packagedatalength = 1;
					sendpack.cmd = package->cmd;
					write_uart1_data(PACKAGE_TYPE_ANSWER, 
										  sendpack.cmd,
										  sendpack.packageno,
										  sendpack.packagedata,
										  sendpack.packagedatalength);
					ret = 0;
				  }
			  }

		#if 0
         else if(package->cmd == COMM_SYS_I2COPEN   )//oxf6
         {
            unsigned int  slvaddr ;
            if(package->packagedatalength == 4 )
            {
               slvaddr = (data[0]<<0)|(data[1]<<8)|(data[2]<<16)|(data[3]<<24) ;
              // i2c_init( slvaddr );
               printf("I2c:0x%02x init..\n", slvaddr );
               
               // reply one ok 
               ch[0] = 0;//ACK;
               sendpack.packageno = package->packageno;
               sendpack.packagedatalength = 1;
               sendpack.cmd = package->cmd;
               write_uart1_data(PACKAGE_TYPE_ANSWER, 
                                sendpack.cmd,
                                sendpack.packageno,
                                ch,
                                sendpack.packagedatalength);
               ret = 0;
            }
            else
               printf("I2c:invalid package length..\n" );
         }
         else if(package->cmd == COMM_SYS_WFILE   )//写文件
         {
            unsigned int  slvaddr ;
            if(package->packagedatalength == 0 )
            {
               printf("uart:Rxved Write File..\n" );
               
               //set_waitifchangefile(1);
               // reply one ok 
               ch[0] = 0;//ACK;
               sendpack.packageno = package->packageno;
               sendpack.packagedatalength = 1;
               sendpack.cmd = package->cmd;
               write_uart1_data(PACKAGE_TYPE_ANSWER, 
                                sendpack.cmd,
                                sendpack.packageno,
                                ch,
                                sendpack.packagedatalength);
               isp_write_file_record( "",0);
               ret = 0;
            }
            else if(package->packagedatalength == 15+2 )
            {
               printf("uart:Rxved Write File with big lum changed ..\n" );
               //                    enable  lum
               //isp_big_lum_change_wf(data[package->packagedatalength-2],data[package->packagedatalength-1], package->packagedata , package->packagedatalength-2 );

               //set_waitifchangefile(1);
               // reply one ok 
               ch[0] = 0;//ACK;
               sendpack.packageno = package->packageno;
               sendpack.packagedatalength = 1;
               sendpack.cmd = package->cmd;
               write_uart1_data(PACKAGE_TYPE_ANSWER, 
                                sendpack.cmd,
                                sendpack.packageno,
                                ch,
                                sendpack.packagedatalength);
     //          isp_write_file_record( "",0);
               ret = 0;
            }
            else if(package->packagedatalength == 14+2 )
            {
               printf("uart:Rxved Write File..\n" );
               
               //set_waitifchangefile(1);
               // reply one ok 
               ch[0] = 0;//ACK;
               sendpack.packageno = package->packageno;
               sendpack.packagedatalength = 1;
               sendpack.cmd = package->cmd;
               write_uart1_data(PACKAGE_TYPE_ANSWER, 
                                sendpack.cmd,
                                sendpack.packageno,
                                ch,
                                sendpack.packagedatalength);
               isp_write_file_record(package->packagedata , package->packagedatalength );
               ret = 0;
            }
            else
               printf("I2c:invalid package length..\n" );
         }
			else if(package->cmd == COMM_SYS_ISP_AE_MULTI_EV_WRITE)
			{
				// 多次曝光文件写入
				void isp_write_multi_ae_file_record (unsigned int ae_comp, char *addpath  ,unsigned int len, unsigned int order );

				printf ("Multi-EV Image Record\n");
				// 多次曝光补偿设置
				unsigned int ae_comp[] = {24, 32, 40, 48, 64, 96};
				int i;
				unsigned int ae_compensation, ae_black_target, ae_bright_target; 
				arkn141_isp_ae_get (&ae_compensation, &ae_black_target, &ae_bright_target);
				for (i = 0; i < sizeof(ae_comp)/sizeof(ae_comp[0]); i ++)
				{
					ae_compensation = ae_comp[i];
					arkn141_isp_ae_set (ae_compensation, ae_black_target, ae_bright_target);
					
					OS_Delay (8000);	// 延时8秒,等待新的曝光参数有效
					
					// 清除曝光稳定事件
					isp_ae_event_reset ();
					
					// 等待新的曝光稳定事件
					printf ("waiting for AE steady\n");
					isp_ae_event_wait ();
					printf ("AE is steady\n");
					
					// 将曝光稳定的RAW及YUV保存
					isp_write_multi_ae_file_record (ae_compensation, package->packagedata , package->packagedatalength, i+1 );
				}
				
			}
         else if(package->cmd == COMM_SYS_AUTOAE )
         {
            if(package->packagedatalength == 1 )
            {
               set_isp_autoae( data[0] );
               // reply one ok 
               ch[0] = 0;//ACK;
               sendpack.packageno = package->packageno;
               sendpack.packagedatalength = 1;
               sendpack.cmd = package->cmd;
               write_uart1_data(PACKAGE_TYPE_ANSWER, 
                                sendpack.cmd,
                                sendpack.packageno,
                                ch,
                                sendpack.packagedatalength);
               ret = 0;
            }
            else
               printf("invalid package data length..\n" );
         }
         else if(package->cmd == COMM_SYS_SETAE )
         {
            if(package->packagedatalength == 1 )
            {
               set_isp_ae_lumTarget( data[0] );
               printf("set ae lunTarget:%d\n", data[0]);
               // reply one ok 
               ch[0] = 0;//ACK;
               sendpack.packageno = package->packageno;
               sendpack.packagedatalength = 1;
               sendpack.cmd = package->cmd;
               write_uart1_data(PACKAGE_TYPE_ANSWER, 
                                sendpack.cmd,
                                sendpack.packageno,
                                ch,
                                sendpack.packagedatalength);
               ret = 0;
            }
            else
               printf("invalid package data length..\n" );
         }
			else if(package->cmd == COMM_SD_STATUS)
			{
				// 0x01 --> SD卡拔出
				// 0x02 --> SD卡插入
				// 0x03 --> SD卡文件系统错误
				// 0x04 --> SD卡无法识别
				static const char* sd_state_code[] = {
					"card plugout",
					"card insert",
					"card fserror",
					"card invalid"
				};
				printf ("COMM_SD_STATUS\n");
				// 即使命令包错误，也返回状态
				unsigned int card_state = XM_GetFmlDeviceCap (DEVCAP_SDCARDSTATE);
				if(card_state == DEVCAP_SDCARDSTATE_UNPLUG)
					ret = 1;
				else if(card_state == DEVCAP_SDCARDSTATE_INSERT)
					ret = 2;
				else if(card_state == DEVCAP_SDCARDSTATE_FS_ERROR)
					ret = 3;
				else //if(card_state == DEVCAP_SDCARDSTATE_INVALID)
					ret = 4;
				 ch[0] = ret;
               sendpack.packageno = package->packageno;
               sendpack.packagedatalength = 1;
               sendpack.cmd = package->cmd;
               write_uart1_data(PACKAGE_TYPE_ANSWER, 
                                sendpack.cmd,
                                sendpack.packageno,
                                ch,
                                sendpack.packagedatalength);
               ret = 0;
				
			}
         else if(package->cmd == COMM_SYS_GETAE )
         {
            if(package->packagedatalength == 0 )
            {
               // reply one ok 
               ch[0] = get_isp_ae_lumTarget(  );//ACK;
               printf("get ae lunTarget:%d\n", ch[0]);
               sendpack.packageno = package->packageno;
               sendpack.packagedatalength = 1;
               sendpack.cmd = package->cmd;
               write_uart1_data(PACKAGE_TYPE_ANSWER, 
                                sendpack.cmd,
                                sendpack.packageno,
                                ch,
                                sendpack.packagedatalength);
               ret = 0;
            }
            else
               printf("invalid package data length..\n" );
         }

         else if(package->cmd == COMM_SYS_I2CCLOSE   )//oxf6
         {
            unsigned int  slvaddr ;
            if(package->packagedatalength == 4 )
            {
               slvaddr = (data[0]<<0)|(data[1]<<8)|(data[2]<<16)|(data[3]<<24) ;
               //i2c_close( slvaddr );
               printf("I2c:0x%02x close..\n", slvaddr );
               
               // reply one ok 
               ch[0] = 0;//ACK;
               sendpack.packageno = package->packageno;
               sendpack.packagedatalength = 1;
               sendpack.cmd = package->cmd;
               write_uart1_data(PACKAGE_TYPE_ANSWER, 
                                sendpack.cmd,
                                sendpack.packageno,
                                ch,
                                sendpack.packagedatalength);
               ret = 0;
            }
            else
               printf("I2c:invalid package length..\n" );
         }
		 #endif
		 
#if 0
         else if(package->cmd == COMM_SYS_I2CREAD   )//oxf6
         {
            unsigned int  slvaddr ;
            unsigned int  ucDataOffset ;
            unsigned char readval ;
				    if(package->packagedatalength == 8 )
            {
               slvaddr= (data[0]<<0)|(data[1]<<8)|(data[2]<<16)|(data[3]<<24) ;
               ucDataOffset= (data[4]<<0)|(data[5]<<8)|(data[6]<<16)|(data[7]<<24) ;
               readval = i2c_reg_read(slvaddr,ucDataOffset);//unsigned int slvaddr, unsigned int ucDataOffset
               printf("i2c read :slvaddr=0x%08x offset=0x%08x val:0x%02x\r\n",  slvaddr, ucDataOffset , readval );

               sendch = sendpack.packagedata;

					sendch[0] =  readval & 0xff;
               
					sendpack.packageno = package->packageno;
					sendpack.packagedatalength = 1;
					sendpack.cmd = package->cmd;
					write_uart1_data(PACKAGE_TYPE_ANSWER, 
										  sendpack.cmd,
										  sendpack.packageno,
										  sendpack.packagedata,
										  sendpack.packagedatalength);
					ret = 0;
            }
         }
         else if(package->cmd == COMM_SYS_I2CWRITE   )//oxf6
         {
            unsigned int  slvaddr ;
            unsigned int  ucDataOffset ;
            unsigned char writeval ;
            unsigned char readval;// 检查 I2C 写入状态
			      if(package->packagedatalength == 9 )
            {
               slvaddr= (data[0]<<0)|(data[1]<<8)|(data[2]<<16)|(data[3]<<24) ;
               ucDataOffset= (data[4]<<0)|(data[5]<<8)|(data[6]<<16)|(data[7]<<24) ;
               writeval = data[8];
               readval = i2c_reg_write(slvaddr, ucDataOffset, writeval);//unsigned int slvaddr, unsigned int ucDataOffset
               if( readval )
                  printf("I2c write error!!\n");
               else
               {
                  printf("i2c write :slvaddr=0x%08x offset=0x%08x val:0x%02x\r\n",  slvaddr, ucDataOffset , writeval );

                  sendch = sendpack.packagedata;
                  //memcpy(sendch+4,&val, 4);// copy value
                  sendch[0] =  readval ;
                  
                  sendpack.packageno = package->packageno;
                  sendpack.packagedatalength = 1;
                  sendpack.cmd = package->cmd;
                  write_uart1_data(PACKAGE_TYPE_ANSWER, 
                                   sendpack.cmd,
                                   sendpack.packageno,
                                   sendpack.packagedata,
                                   sendpack.packagedatalength);
                  ret = 0;
               }
            }
         }
#endif
		#if 0
         else if(package->cmd == COMM_SYS_ISP_SET_VIDEO_FORMAT  )
         {
            unsigned int format;
            // 四字节数值
            if(package->packagedatalength == 4 )
            {
					format = data[0];
               printf("COMM_SYS_ISP_SET_VIDEO_FORMAT %d\r\n", format );
					// reply one ok 
					ch[0] = 0;//ACK;
					sendpack.packageno = package->packageno;
					sendpack.packagedatalength = 1;
					sendpack.cmd = package->cmd;
					write_uart1_data(PACKAGE_TYPE_ANSWER, 
                            sendpack.cmd,
                            sendpack.packageno,
                            sendpack.packagedata,
                            sendpack.packagedatalength);
					ret = 0;
					
					isp_set_video_format (format);
					// ISP Reset
            //   isp_reset();
            //   isp_init_support_scale();
					
				}
         }
			else if(package->cmd == COMM_SYS_SENSOR_READOUT_DIRECTION)
			{
				if(package->packagedatalength == 2 )
				{
					extern void isp_set_sensor_readout_direction (unsigned int horz_reverse_direction, unsigned int vert_reverse_direction);

					printf ("set_sensor_readout_direction, horz(%s), vert(%s)\n", data[0] ? "reverse" : "normal", data[1] ? "reverse" : "normal");
					isp_set_sensor_readout_direction (data[0], data[1]);
               // reply one ok 
               ch[0] = 0;//ACK;
               sendpack.packageno = package->packageno;
               sendpack.packagedatalength = 1;
               sendpack.cmd = package->cmd;
               write_uart1_data(PACKAGE_TYPE_ANSWER, 
                                sendpack.cmd,
                                sendpack.packageno,
                                ch,
                                sendpack.packagedatalength);
               ret = 0;
				}
			}
			#endif
			
			else if(package->cmd == COMM_SYS_LCD_GAMMA_SETTING)
			{
				printf(">>>>COMM_SYS_LCD_GAMMA_SETTING....\r\n");
					sendch = sendpack.packagedata;
					//memcpy(sendch, &addr , 4);// copy addr
					//sendch[0] = addr&0xff;
					//sendch[1] = (addr>>8)&0xff;
					//sendch[2] = (addr>>16)&0xff;
					//sendch[3] = (addr>>24)&0xff;
					//memcpy(sendch+4,&val, 4);// copy value  
					for(i=0;i<49;i++)
					{
						val = *(unsigned int *)(0x7019013c+4*i);
						sendch[0+4*i] = val&0xff;
						sendch[1+4*i] = (val>>8)&0xff;
						sendch[2+4*i] = (val>>16)&0xff;
						sendch[3+4*i] = (val>>24)&0xff;
					}
						
					sendpack.packageno = package->packageno;
					sendpack.packagedatalength = 4*49;
					sendpack.cmd = package->cmd;
					write_uart1_data(PACKAGE_TYPE_ANSWER, 
										  sendpack.cmd,
										  sendpack.packageno,
										  sendpack.packagedata,
										  sendpack.packagedatalength);
					ret = 0;
				
			     #if 0
				if(package->packagedatalength == 1 )
				{
					extern void LCDSetGAMMA(float *r ,float *g ,float *b );

					float gamma;
					gamma = ((float)data[0]) / 10.0f;
					printf ("lcd gamma setting = %f\n", gamma);
					//LCDSetGAMMA (&gamma, &gamma, &gamma);
               // reply one ok 
               ch[0] = 0;//ACK;
               sendpack.packageno = package->packageno;
               sendpack.packagedatalength = 1;
               sendpack.cmd = package->cmd;
               write_uart1_data(PACKAGE_TYPE_ANSWER, 
                                sendpack.cmd,
                                sendpack.packageno,
                                ch,
                                sendpack.packagedatalength);
               ret = 0;
				}
				#endif
				
			}

			#if 0
			else if(package->cmd == COMM_SYS_ISP_AE_SET)
			{
				if(package->packagedatalength == 3 )
				{
					extern int arkn141_isp_ae_set (unsigned int ae_compensation, unsigned int ae_black_target, unsigned int ae_bright_target );

					printf ("ISP AE parameter set, compensation=%d, black_target=%d, bright_target=%d\n", data[0], data[1], data[2]);
					arkn141_isp_ae_set (data[0], data[1], data[2]);
					
               // reply one ok 
               ch[0] = 0;//ACK;
               sendpack.packageno = package->packageno;
               sendpack.packagedatalength = 1;
               sendpack.cmd = package->cmd;
               write_uart1_data(PACKAGE_TYPE_ANSWER, 
                                sendpack.cmd,
                                sendpack.packageno,
                                ch,
                                sendpack.packagedatalength);
               ret = 0;
				}
			}
			else if(package->cmd == COMM_SYS_ISP_AE_GET)
			{
				// 读取AE曝光控制参数
				if(package->packagedatalength == 0 )
				{
					extern int arkn141_isp_ae_get (unsigned int * ae_compensation, unsigned int * ae_black_target, unsigned int *ae_bright_target );

					unsigned int ae_compensation, ae_black_target, ae_bright_target;
					arkn141_isp_ae_get (&ae_compensation, &ae_black_target, &ae_bright_target);
					sendpack.packagedata[0] = (u8_t)ae_compensation;
					sendpack.packagedata[1] = (u8_t)ae_black_target;
					sendpack.packagedata[2] = (u8_t)ae_bright_target;
					sendpack.packageno = package->packageno;
					sendpack.packagedatalength = 3;
					sendpack.cmd = package->cmd;
					write_uart1_data(PACKAGE_TYPE_ANSWER, 
										  sendpack.cmd,
										  sendpack.packageno,
										  sendpack.packagedata,
										  sendpack.packagedatalength);
					ret = 0;
				}
			}
			else if(package->cmd == COMM_SYS_ISP_AE_INFO_HIDE)
			{
				if(package->packagedatalength == 1 )
				{
					isp_ae_info_output_enable (data[0] == 0);
               // reply one ok 
               ch[0] = 0;//ACK;
               sendpack.packageno = package->packageno;
               sendpack.packagedatalength = 1;
               sendpack.cmd = package->cmd;
               write_uart1_data(PACKAGE_TYPE_ANSWER, 
                                sendpack.cmd,
                                sendpack.packageno,
                                ch,
                                sendpack.packagedatalength);
               ret = 0;
				}
			}
			// ISP LUT Table写入
			else if(package->cmd == COMM_SYS_ISP_GEM_LUT_WRITE)
			{
				int i;
				unsigned int item_count;
				unsigned int lut_index;
				// LUT_INDEX (1) + ITEM_COUNT(1) + ITEM_DATA(=2*ITEM_COUNT)
            // reply one ok 
               ch[0] = 0;//ACK;
               sendpack.packageno = package->packageno;
               sendpack.packagedatalength = 1;
               sendpack.cmd = package->cmd;
               write_uart1_data(PACKAGE_TYPE_ANSWER, 
                                sendpack.cmd,
                                sendpack.packageno,
                                ch,
                                sendpack.packagedatalength);
            ret = 0;
				lut_index  = data[0];
				item_count = data[1];
				unsigned char *lut_data = data + 2;
				
				for (i = 0; i < item_count; i++)
				{
					#define GEM_LUT_BASE      (0x1a0)
					unsigned short item_data = lut_data[0] | (lut_data[1] << 8);
					unsigned int data0 = (lut_index) | (i << 8) | (item_data<<16);
					Gem_write ((GEM_LUT_BASE+0x00), data0);
					lut_data += 2;
				}
				
			}
			else if(package->cmd == COMM_SYS_ISP_AUTORUN)
			{
            ch[0] = 0;//ACK;
               sendpack.packageno = package->packageno;
               sendpack.packagedatalength = 1;
               sendpack.cmd = package->cmd;
               write_uart1_data(PACKAGE_TYPE_ANSWER, 
                                sendpack.cmd,
                                sendpack.packageno,
                                ch,
                                sendpack.packagedatalength);
            ret = 0;			
				printf ("auto run, item = %d, state = %d\n", data[0], data[1]);
				isp_set_auto_run_state (data[0], data[1]);
			}
			else if(package->cmd == COMM_SYS_ISP_STEADY_AE)
			{
            ch[0] = 0;//ACK;
               sendpack.packageno = package->packageno;
               sendpack.packagedatalength = 1;
               sendpack.cmd = package->cmd;
               write_uart1_data(PACKAGE_TYPE_ANSWER, 
                                sendpack.cmd,
                                sendpack.packageno,
                                ch,
                                sendpack.packagedatalength);
            ret = 0;			
				printf ("steady AE %s\n", data[0] ? "ENABLE" : "DISABLE");
				cmos_exposure_set_steady_ae (data[0]);
				
			}
         else if(package->cmd == COMM_SYS_LCDMOVE  )
         {
            if(package->packagedatalength == 8 )
            {
               int  x ;
               int  y ;
               unsigned int  Yaddr , Uaddr , Vaddr ;
               x = (data[0]<<0)|(data[1]<<8)|(data[2]<<16)|(data[3]<<24) ;
               y = (data[4]<<0)|(data[5]<<8)|(data[6]<<16)|(data[7]<<24) ;
               printf("lcd move to x:%d y:%d\n",x,y);
           //    get_isp_dispaddr2 (  &Yaddr , &Uaddr , &Vaddr ) ;
					
					/*
               get_isp_finish_addr ( get_isp_framebufferno(), &Yaddr , &Uaddr , &Vaddr ) ;
               set_current_position( x , y );
               LCD_Move( 2,
                  800, 480 , IMAGE_H_SZ, IMAGE_V_SZ, 0x3f,0xff,
                  x,y ,XM_OSD_LAYER_FORMAT_Y_UV420 ,
                  Yaddr , Uaddr ,Vaddr , IMAGE_H_SZ );	// lcd
               */
              /* LCD_Move( 2,
                  640, 480 , 320, 240, 0x3f,0xff,
                  x,y ,XM_OSD_LAYER_FORMAT_Y_UV420 ,//vga
                  Yaddr , Uaddr ,Vaddr , 1280 );	*/
              /* LCD_Move( 2,
                  720, 480 , 720, 480, 0x3f,0xff,
                  x,y ,XM_OSD_LAYER_FORMAT_Y_UV420 ,//cvbs-ntsc
                  Yaddr , Uaddr ,Vaddr , 1280 );	*/
               
               
               // reply one ok 
               ch[0] = 0;//ACK;
               sendpack.packageno = package->packageno;
               sendpack.packagedatalength = 1;
               sendpack.cmd = package->cmd;
               write_uart1_data(PACKAGE_TYPE_ANSWER, 
                                sendpack.cmd,
                                sendpack.packageno,
                                ch,
                                sendpack.packagedatalength);
               
               ret = 0;
            }
               
         }
         else if(package->cmd == COMM_SYS_ISPCOLOR  )
         {
            if(package->packagedatalength == 1 )
            {
               unsigned int  converttype ;
               converttype = data[0];
               printf("isp color type %d \n",converttype );
               
               isp_set_color_coeff( converttype );
               
               // reply one ok 
               ch[0] = 0;//ACK;
               sendpack.packageno = package->packageno;
               sendpack.packagedatalength = 1;
               sendpack.cmd = package->cmd;
               write_uart1_data(PACKAGE_TYPE_ANSWER, 
                                sendpack.cmd,
                                sendpack.packageno,
                                ch,
                                sendpack.packagedatalength);
               ret = 0;
            }
         }
         else if(package->cmd == COMM_SYS_IOWRITE_BAK  )
         {
			      unsigned int bak;
            unsigned int writeval;
            // 只处理 数据长度为 8 字节的包
            // 一个四字节地址，一个四字节数值
            if(package->packagedatalength == 12 )
            {
                addr = (data[0]<<0)|(data[1]<<8)|(data[2]<<16)|(data[3]<<24) ;
                val  = (data[4]<<0)|(data[5]<<8)|(data[6]<<16)|(data[7]<<24) ;
                bak  = (data[8]<<0)|(data[9]<<8)|(data[10]<<16)|(data[11]<<24) ;
					 printf ("io_mask: addr = 0x%08x, mask = 0x%08x, data = 0x%08x\n",  addr, bak, val);
                io_write_bak(addr,val,bak);
                printf("write:addr=0x%08x writeval=0x%08x\r\n", addr, io_read( addr ) );
                //writeval = (io_read( addr )&bak)|val;
                //io_write( addr,  writeval );
                //*(volatile unsigned int *)addr = val;
                //printf("write:addr=0x%08x writeval=0x%08x\r\n", addr, writeval );
                // reply one ok 
                ch[0] = 0;//ACK;
                sendpack.packageno = package->packageno;
                sendpack.packagedatalength = 1;
                sendpack.cmd = package->cmd;
                write_uart1_data(PACKAGE_TYPE_ANSWER, 
                            sendpack.cmd,
                            sendpack.packageno,
                            ch,
                            sendpack.packagedatalength);
                ret = 0;
             }
         }
         else if(package->cmd == COMM_SYS_ISPRESET   )// 0xFB
         {

         }
			// ISP Scale RESET
         else if(package->cmd == COMM_SYS_I2C_ISPSCALERESET   )// 0xEA
         {

         }
			// ISP Scalar Window Setting
         else if(package->cmd == COMM_SYS_ISP_SCALAR_WINDOW_SETTING   )
			{

			}
         else if(package->cmd == COMM_SYS_ISP_SCALAR_VIDEO_SETTING   )
			{

				
			}		
			else if(package->cmd == COMM_SYS_ISP_SET_WORK_MODE)	// ISP工作模式设置
			{
            unsigned int  mode ;
				int r;
            mode = data[0];
				printf ("isp_set_work_mode %d\n", mode);
				r = isp_set_work_mode (mode);
				if(r < 0)
					ch[0] = 1;	// NAK
				else
            	ch[0] = 0;	// ACK;
				sendpack.packageno = package->packageno;
				sendpack.packagedatalength = 1;
				sendpack.cmd = package->cmd;
				write_uart1_data(PACKAGE_TYPE_ANSWER, 
                            sendpack.cmd,
                            sendpack.packageno,
                            ch,
                            sendpack.packagedatalength);
            ret = 0;
			}
			else if(package->cmd == COMM_SYS_ISP_GET_CURRENT_EXP)		// 获取当前曝光值
			{
				extern void isp_ae_get_current_exp (unsigned int *inttime, 
									  float *again,
									  float *dgain
									);
				unsigned int inttime;
				float again, dgain;
				isp_ae_get_current_exp (&inttime, &again, &dgain);
				sendpack.packageno = package->packageno;
				sendpack.packagedatalength = 12;
				memcpy (ch + 0, &inttime, sizeof(inttime));
				memcpy (ch + 4, &again, sizeof(again));
				memcpy (ch + 8, &dgain, sizeof(dgain));
				sendpack.cmd = package->cmd;
				write_uart1_data(PACKAGE_TYPE_ANSWER, 
                            sendpack.cmd,
                            sendpack.packageno,
                            ch,
                            sendpack.packagedatalength);
            ret = 0;
			}
			else if(package->cmd == COMM_SYS_ISP_SET_CURRENT_EXP)		// 设置当前曝光值
			{
				extern void isp_ae_set_current_exp (unsigned int inttime, 
									  float again,
									  float dgain
									);
				unsigned int inttime;
				float again, dgain;
				memcpy (&inttime, data + 0, 4);
				memcpy (&again,   data + 4, 4);
				memcpy (&dgain,   data + 8, 4);
				
				isp_ae_set_current_exp (inttime, again, dgain);
				sendpack.packageno = package->packageno;
				sendpack.packagedatalength = 1;
				ch[0] = 1;
				sendpack.cmd = package->cmd;
				write_uart1_data(PACKAGE_TYPE_ANSWER, 
                            sendpack.cmd,
                            sendpack.packageno,
                            ch,
                            sendpack.packagedatalength);
            ret = 0;
			}
				
			else if(package->cmd == COMM_SYS_ISP_GET_MAX_GAIN)		// 读取最大可用增益
			{
				extern void isp_ae_get_max_gain (unsigned int *again, unsigned int *dgain);

				unsigned int max_again, max_dgain;
				isp_ae_get_max_gain (&max_again, &max_dgain);
				sendpack.packageno = package->packageno;
				sendpack.packagedatalength = 8;
				memcpy (ch + 0, &max_again, 4);
				memcpy (ch + 4, &max_dgain, 4);
				sendpack.cmd = package->cmd;
				write_uart1_data(PACKAGE_TYPE_ANSWER, 
                            sendpack.cmd,
                            sendpack.packageno,
                            ch,
                            sendpack.packagedatalength);
            ret = 0;
			}
			else if(package->cmd == COMM_SYS_ISP_SET_MAX_GAIN)
			{
				extern void isp_ae_set_max_gain (unsigned int again, unsigned int dgain);
            unsigned int  max_again, max_dgain ;
				memcpy (&max_again, data + 0, 4);
				memcpy (&max_dgain, data + 4, 4);
				printf ("ISP_SET_MAX_GAIN %d %d\n", max_again, max_dgain);
				isp_ae_set_max_gain (max_again, max_dgain);
            ch[0] = 0;	// ACK;
				sendpack.packageno = package->packageno;
				sendpack.packagedatalength = 1;
				sendpack.cmd = package->cmd;
				write_uart1_data(PACKAGE_TYPE_ANSWER, 
                            sendpack.cmd,
                            sendpack.packageno,
                            ch,
                            sendpack.packagedatalength);
            ret = 0;
				
			}
         	#endif
			
			if(ret)// for error return 
			{
				//返回一个错误信息 
				ch[0] = 1;//NAK;
				sendpack.packageno = package->packageno;
				sendpack.packagedatalength = 1;
				sendpack.cmd = package->cmd;
				write_uart1_data(PACKAGE_TYPE_ANSWER, 
									  sendpack.cmd,
									  sendpack.packageno,
									  sendpack.packagedata,
									  sendpack.packagedatalength);
			}
			
			//处理完成 ，该包缓存可以继续缓存下一个数据了，mark标记
			package->receivedfinished = 0; 
			
		}
		
	}
	return ret;
}

/*
return value:	 
	0 :	success. 返回成功
	-1 :	failed.  执行失败，函数可用空间不够
*/
int write_uart1_data(  UINT16  packagetype, // package type 
                          UINT8   cmd,         // cid
                          UINT32  packageno_for_cmd_reply, // the package no of [current cmd] 
                          void   *data,        // data
                          UINT16  len   )      // data lenght
{
	// definition
	int ret;
	UINT32 packageno=0 ;

	// 调用者使用互斥信号量保护。OS_EnterRegion会阻止高优先级进程的调度
	// function body 
	
	//有可用空间 ，继续填充数据并开启uart的自动发送
	//	*(UINT16*)(&Wdata[0])=packagetype;  // 0  1 
	resp_buf[0] =  packagetype&0xff;
	resp_buf[1] = (packagetype&0xff00)>>8;
	//	*(UINT16*)(&Wdata[2]) = package->packagedatalength;// 2 3   (total =9 bytes now )
	resp_buf[2] = ( len+6)&0xff;
	resp_buf[3] = ((len+6)&0xff00)>>8;
	resp_buf[4] = cmd;  //4
	
	if( packagetype == PACKAGE_TYPE_ASYN )
	{
		packageno = packageno_ASYN++ ;
	}
	else
	{	
		//命令包的序号由主机给定
		packageno = packageno_for_cmd_reply ;
	}
	//	*(UINT32*)(&Wdata[5]) = packageno;// 5 6 7 8
	resp_buf[5] =  (packageno)&0xff;
	resp_buf[6] = ((packageno)&0xff00)>>8;
	resp_buf[7] = ((packageno)&0xff0000)>>16;
	resp_buf[8] = ((packageno)&0xff000000)>>24;
   
	if( len  > 0 )
	{
		memcpy( &resp_buf[9] , data , len ) ;// 拷贝数据区
	}
	resp_buf[ 9+len ] = checksum_byte( (char*)&resp_buf[2] , 7+len  ) ;// 计算校验码
	ret = xm_uart_write (uart_socket_dev, resp_buf, 10+len, -1);
	if(ret != (10+len))
	{
		printf ("write_uart1_data falied, ret (%d) != %d\n",  ret, (10+len));
	}
	// putUART1SendStr( resp_buf , 10+len ); // fill data buffer for ( start uart send )
	return ret;// success return .
}


#define	UART_CMD_TIMEOUT		100		// 100ms
static void UART_socket_Task (void)
{
	int ret;
	u8_t id[2];
	unsigned int databuf_len;

	XM_printf(">>>>>>>>>>UART_socket_Task..............\r\n");
	//static u8_t uart_cmd_packet[512];
	//static int uart_cmd_length;
	
	//uart_cmd_length = 0;
	
	// 命令包
	// 0xFF, 0x00, 长度（2字节）， CID(1字节)，命令包序号(4字节)，命令包数据，CHECKSUM校验(1字节)
	// 1) 两个字节的长度标示 长度= CID+命令包序号+命令包数据+CHECKSUM
	// 2)	CHECKSUM = 0－（LEN +CID+ 命令包序号+命令包数据） 即 各字节相加取反
	
	while (1)
	{
		// 查找 FF 00
		// 读取1字节的
step_scan_ff:
		// 如果下面的xm_uart_read不做超时处理，当当前命令帧的其中1个或多个字节的数据丢失或异常时， 
		//	会导致随后的一帧或多帧数据异常
		ret = xm_uart_read (uart_socket_dev, id, 1, UART_CMD_TIMEOUT);
		if(ret == 1)
		{
			#ifdef EN_DEBUG_UART
			XM_printf(">>>>>id[0]:%d\r\n", id[0]);
			#endif
			if(id[0] != 0xFF)
			{
				/*if(uart_cmd_length < 512)
				{
					uart_cmd_packet[uart_cmd_length] = id[0];
					uart_cmd_length ++;
				}*/
				continue;
			}
			
			/*if(uart_cmd_length)
			{
				int i = 0;
				printf ("\nerror uart packet, len = %d, ", uart_cmd_length);
				for (i = 0; i < uart_cmd_length; i ++)
				{
					printf ("%02x ", uart_cmd_packet[i]);
				}
				printf ("\n");
			}*/
			
step_scan_00:
			// 读取 00
			ret = xm_uart_read (uart_socket_dev, id + 1, 1, UART_CMD_TIMEOUT);

			#ifdef EN_DEBUG_UART
			XM_printf(">>>>>ret:%d\r\n", ret);
			XM_printf(">>>>>id[1]:%d\r\n", id[1]);
			#endif
			
			if(ret != 1)
				continue;
			if(id[1] == 0xFF)
			{
				id[0] = 0xFF;
				goto step_scan_00;
			}
			
			//printf ("uart socket read 0xFF 0x00\n");
			//uart_cmd_length = 0;
			
			// 读取长度信息 (2字节)
			ret = xm_uart_read (uart_socket_dev, data_buf, 2, UART_CMD_TIMEOUT);
			if(ret != 2)
			{
				printf ("read length failed, ret = %d\n", ret);
				continue;
			}
			databuf_len = data_buf[0] | (data_buf[1] << 8);

			#ifdef EN_DEBUG_UART
			printf("uart packet length = %d\n", databuf_len);
			printf("data_buf[0] = %d\n", data_buf[0]);
			printf("data_buf[1] = %d\n", data_buf[1]);
			#endif
			
			if(databuf_len < 6 || databuf_len > MAX_UART_SOCKET_LENGTH)
			{
				// 无效的长度
				printf ("illegal packet length %d\n", databuf_len);
				continue;
			}
			ret = xm_uart_read (uart_socket_dev, data_buf + 2, databuf_len, UART_CMD_TIMEOUT);

			#ifdef EN_DEBUG_UART
			{
				u8_t i;

				for(i=0; i<databuf_len; i++)
				{
					printf("data_buf[%d]:%x\r\n", 2+i,data_buf[2+i]);
				}
			}
			#endif
			
			if(ret != databuf_len)
			{
				printf ("get packet data failed, ret = %d\n", ret);
				continue;
			}
			
			unsigned char  encrypt = checksum_byte(data_buf, databuf_len + 2 - 1);

			#ifdef EN_DEBUG_UART
			printf(">>>>rev checksum:%x\r\n", data_buf[2 + databuf_len - 1]);
			printf(">>>>encrypt:%x\r\n", encrypt);
			#endif
			
			if(encrypt == data_buf[2 + databuf_len - 1])		// 命令包完整
			{
				unsigned int packet_no = data_buf[3] | (data_buf[4] << 8) | (data_buf[5] << 16) | (data_buf[6] << 24);
				ret = HandlePacket(packet_no,data_buf[2], data_buf + 7,databuf_len - 6);
			}
			else
			{
				printf ("uart packet crc error\n");
			}
		}
		else
		{
			OS_Delay (1);
		}
	}
}


/******************************************************************************
	uart_socket_init
	初始化 串口接收数据。
********************************************************************************/
void XMSYS_MessageSocketInit (void)
{
	int err = XM_UART_ERRCODE_ILLEGAL_PARA;
	unsigned int dev;
	
	dev = XM_UART_DEV_2;		// ASIC使用UART 2调试 LCD
	
	uart_socket_dev = xm_uart_open (dev, 
									XM_UART_STOPBIT_1,
									XM_UART_PARITY_DISABLE,
									XM_UART_MODE_TR,
									57600,
									512,
									&err);
	if(uart_socket_dev == NULL)
	{
		printf ("open uart socket failed, err_code=%d\n", err);
		return;
	}
											  
	// 创建一个接收串口包的任务。
	OS_CREATETASK (&TCB_UART_socket_Task, "UART_socket", UART_socket_Task, XMSYS_UART_SOCKET_TASK_PRIORITY, StackUART_socket );	
	
	// 正常运行 isp
	//arkn141_isp();
}

