#include "xm_proj_define.h"
#include "common.h"
#include "descript.h"
#include "descript_def.h"
#include "requests.h"
#include "sensorlib.h"
#include "sensorcommon.h"
#include "gpio_function.h"
#include "string.h"
#include <stdio.h>

extern void UVCStart (void);
extern void UVCStop (void);

BYTE idata bCurAltSet;
bit bOtherSpeedDescFlg = 0;	// 0-use other-speed desc; 1-use fs desc


//descript.c :
//string
extern BYTE code gcLangIDString[4];
extern BYTE code gcManufacturerString[24];
extern BYTE code gcProduceString[34];
extern BYTE code gcProduceString1[22];
extern BYTE code gcSerialnumString[18];
extern BYTE code HQualifierData[10];
extern BYTE code FQualifierData[10];

//descriptor
extern STD_DEV_DSCR xdata gsStdDevDesc;


BOOL usb_init_descriptor()		
{
	BOOL bIsHighSpeed;

	//printf ("reg_usb_Power=0x%x\n", reg_usb_Power);
	if(((reg_usb_Power & bmBIT4) == bmBIT4) && (gsDeviceStatus.bUsbHsOrFs == 1))
	{
		//printf ("USB 2.0 High Speed\n");
		bIsHighSpeed = 1;
	}
	else
	{
		printf ("USB 1.1 Full Speed\n");
		bIsHighSpeed = 0;
		
		XM_lock();
		hw_watchdog_init (1);
		while(1);
	}

	gsStdDevDesc.bLength = sizeof(STD_DEV_DSCR);	//0x12;
	gsStdDevDesc.bDescriptorType = 0x01;	//M_DST_DEVICE
	
	if(((reg_usb_Power & bmBIT4) == bmBIT4) && (gsDeviceStatus.bUsbHsOrFs == 1))
	{
		// gsStdDevDesc.bcdUSB = 0x0002;	// USB spec rev is 2.0
		gsStdDevDesc.bcdUSB[0] = 0x00;
		gsStdDevDesc.bcdUSB[1] = 0x02;
	}
	else
	{
		// gsStdDevDesc.bcdUSB = 0x1001;	 // USB spec rev is 1.1
		gsStdDevDesc.bcdUSB[0] = 0x10;	 // USB spec rev is 1.1
		gsStdDevDesc.bcdUSB[1] = 0x01;
	}

	gsStdDevDesc.bDeviceClass = 0xEF;
	gsStdDevDesc.bDeviceSubClass = 0x02;
	gsStdDevDesc.bDeviceProtocol = 0x01;
	gsStdDevDesc.bMaxPacketSize0 = 0x40;
	//	gsStdDevDesc.idVendor = (gwVID >> 8) | (gwVID << 8);
	gsStdDevDesc.idVendor[0] = (BYTE)(gwVID & 0xFF);
	gsStdDevDesc.idVendor[1] = (BYTE)(gwVID >> 8);
	// gsStdDevDesc.idProduct = (gwPID >> 8) | (gwPID << 8);
	gsStdDevDesc.idProduct[0] = (BYTE)(gwPID & 0xFF);
	gsStdDevDesc.idProduct[1] = (BYTE)(gwPID >> 8);
	// gsStdDevDesc.bcdDevice = 0x0001;
	gsStdDevDesc.bcdDevice[0] = 0x00;
	gsStdDevDesc.bcdDevice[1] = 0x01;
	gsStdDevDesc.iManufacturer = 0x01;
	gsStdDevDesc.iProduct = 0x02;
	//gsStdDevDesc.iSerialNumber = 0x03;		//使用serial num
	gsStdDevDesc.iSerialNumber = 0x00;			//不使用serial num
	gsStdDevDesc.bNumConfigurations = 0x01;

	return bIsHighSpeed;
}

void usb_start_video()
{
	frameIdx_sel(gsVideoProbeCtrl.bFormatIndex);	


	{
		(*pFunc_Sensor_Sel_AltSet)();
	}


}

//仅设置好数据源,  以后再传输
void usb_setup_in(BYTE bCount)
{
	if (!gbError)
	{
		gsEp0Status.wBytesLeft = bCount;
		gsEp0Status.pbData = &gbBuffer[0];
		gsEp0Status.bState = EP0_TX;
	}
}

extern DWORD XM_GetTickCount (void);

//等待数据的传输完毕,  然后将数据取出来
void usb_fetch_out(BYTE bCount)
{
	BYTE xdata *pData;
	unsigned int stop_ticket = XM_GetTickCount() + 100;

	gsEp0Status.bState = EP0_RX;
	//printf ("usb_EP0=0x%02x\n", reg_usb_EP0);
	// SETUP阶段一完成, 等待Data Stage
	reg_usb_EP0 |= CSR0L_ServicedRxPktRdy;
	
	delay (10);
	
	// 等待Data Packet
	while (!(reg_usb_EP0 & CSR0L_RxPktRdy))
	{
		// This bit will be set when a control transaction ends before the DataEnd bit has been set.
		// An interrupt will be generated and the FIFO flushed at this time. The bit is cleared by
		// the CPU writing a 1 to the ServicedSetupEnd bit.
		if(XM_GetTickCount() >= stop_ticket)
		{
			printf ("timeout to waiting for Data Packet\n");
			break;
		}
	}

	gsEp0Status.bState = EP0_IDLE;
	//printf ("bCount=%d\n", bCount);
	
	pData = &gbBuffer[0];
	while (bCount)
	{
		*pData++ = reg_usb_End0;
		bCount--;
	}
}

void UsbGetStatus()
{
	BYTE bTemp;
	bTemp = LSB(gsUsbRequest.wIndex);
	
	if (gsDeviceStatus.bUsb_DevState == DEVSTATE_CONFIG)
	{
		if((gsUsbRequest.bmRequestType == M_CMD_STDDEVIN)
				|| (gsUsbRequest.bmRequestType == M_CMD_STDIFIN))//dev&&if
		{
			gbBuffer[0] = 0x00;
			gbBuffer[1] = 0x00;
		}
		else if(gsUsbRequest.bmRequestType == M_CMD_STDEPIN)
		{
			switch (bTemp)	//Endpoint : Ep0, Ep1In, Ep2In, Ep3In
			{
				case 0x00 :
					//reg_usb_Index = 0x00;
					gbBuffer[0] = 0;
					gbBuffer[1] = 0;
					break;
			 /*
				case 0x80 :
					reg_usb_Index = 0x00;
					gbBuffer[0] = 0;
					gbBuffer[1] = 0;
					break;
			*/	
				case 0x01 :
					reg_usb_Index = 0x01;
					gbBuffer[0] = (MGC_Read8(MUSB_IDX_RXCSRL) & RXCSRL_SendStall) ? 1 : 0;
					gbBuffer[1] = 0;
					reg_usb_Index = 0x00;
					break;
				
				case 0x81 :
					reg_usb_Index = 0x01;
					gbBuffer[0] = (MGC_Read8(MUSB_IDX_TXCSRL) & TXCSRL_SendStall) ? 1 : 0;
					gbBuffer[1] = 0;
					reg_usb_Index = 0x00;
					break;
				case 0x82 :
					reg_usb_Index = 0x02;
					gbBuffer[0] = (MGC_Read8(MUSB_IDX_TXCSRL) & TXCSRL_SendStall) ? 1 : 0;
					gbBuffer[1] = 0;
					reg_usb_Index = 0x00;
					break;
				case VIDEO_EP_IN :
					reg_usb_Index = VIDEO_EP_INDEX;
					gbBuffer[0] = (MGC_Read8(MUSB_IDX_TXCSRL) & TXCSRL_SendStall) ? 1 : 0;
					//gbBuffer[0] = 0;
					gbBuffer[1] = 0;
					reg_usb_Index = 0x00;
					break;
				default :
					gbError = TRUE;
					break;
			}
			//reg_usb_Index = 0x00;
		}
		//gbBuffer[0] = reg_InSendStall;
		//gbBuffer[1] = 0;
	}
	else if((gsDeviceStatus.bUsb_DevState == DEVSTATE_ADDRESS)
				&&(((gsUsbRequest.bmRequestType == M_CMD_STDEPIN) && (gsUsbRequest.wIndex == 0x00))
					||(gsUsbRequest.bmRequestType == M_CMD_STDDEVIN)))
	//else if ((gsDeviceStatus.bUsb_DevState == DEVSTATE_ADDRESS)
	//			&& ((gsUsbRequest.bmRequestType == M_CMD_STDDEVIN)
	//		   || ((gsUsbRequest.bmRequestType == M_CMD_STDEPIN) && (bTemp == 0x00))))
	{
		gbBuffer[0] = 0;
		gbBuffer[1] = 0;
	}
	else
		gbError = TRUE;

	usb_setup_in(2);
}

void UsbSetFeature()
{
	printf ("SetFeature Type=%x index=%x\n",  gsUsbRequest.bmRequestType, gsUsbRequest.wIndex);
	switch (gsUsbRequest.bmRequestType)
	{
		case M_CMD_STDEPOUT :	//Endpoint Halt
			if((gsDeviceStatus.bUsb_DevState == DEVSTATE_CONFIG) && (gsUsbRequest.wValue == 0))
			{
				switch (gsUsbRequest.wIndex)
				{
					case 0x01 :		//ep1Out
						reg_usb_Index = 0x01;
						MGC_Write8(MUSB_IDX_RXCSRL, MGC_Read8(MUSB_IDX_RXCSRL)|RXCSRL_SendStall) ;
//						NoData = 1;
						reg_usb_Index = 0x00;
						break;
					case 0x81 : 	//ep1in
						reg_usb_Index = 0x01;

						MGC_Write8(MUSB_IDX_TXCSRL, MGC_Read8(MUSB_IDX_TXCSRL)|TXCSRL_SendStall) ;
//						NoData = 1;
						reg_usb_Index = 0x00;
						break;
					case 0x82 : 	//ep2in
						reg_usb_Index = 0x02;
						MGC_Write8(MUSB_IDX_TXCSRL, MGC_Read8(MUSB_IDX_TXCSRL)|TXCSRL_SendStall) ;
//						NoData = 1;
						reg_usb_Index = 0x00;
						break;
					case VIDEO_EP_IN : 	//ep5in
						reg_usb_Index = VIDEO_EP_INDEX;
						MGC_Write8(MUSB_IDX_TXCSRL, MGC_Read8(MUSB_IDX_TXCSRL)|TXCSRL_SendStall) ;
//						NoData = 1;
						reg_usb_Index = 0x00;
						break;
					default :
						gbError = TRUE;
						break;
				}
			}
			else
				gbError = TRUE;

			break;

		case M_CMD_STDDEVOUT :	//Test Mode
			if (gsUsbRequest.wValue == M_FTR_TESTMODE)
			{
				switch ((BYTE)(gsUsbRequest.wIndex >> 8))
				{
					case 0x01 : 	//Test_J
						gsEp0Status.bTestMode = 0x02;
						break;
					case 0x02 : 	//Test_K
						gsEp0Status.bTestMode = 0x04;
						break;
					case 0x03 : 	//Test_SE0
						gsEp0Status.bTestMode = 0x01;
						break;
					case 0x04 : 	//Test_PKT
						gsEp0Status.bTestMode = 0x08;
						break;
					case 0x05 : 	//Test_FORCE_ENABLE
						gsEp0Status.bTestMode = 0x10;
						break;
					default :
						gbError = TRUE;
						break;
				}
			}
			else
			{
				gbError = TRUE;
			}
			break;
			
		default :
			gbError = TRUE;
			break;
	}
}

void UsbClearFeature()
{
	//printf ("ClearFeature Type=%x index=%x\n",  gsUsbRequest.bmRequestType, gsUsbRequest.wIndex);
	if((gsDeviceStatus.bUsb_DevState == DEVSTATE_DEFAULT)
					||(gsUsbRequest.bmRequestType != M_CMD_STDEPOUT)//just endpoint can do this
					||(gsUsbRequest.wValue != 0x0000))//just halt can do
	{
		gbError = TRUE;
	}
	else if ((gsDeviceStatus.bUsb_DevState == DEVSTATE_CONFIG)&&(gsUsbRequest.wValue == 0)
			&&(gsUsbRequest.bmRequestType == M_CMD_STDEPOUT))
	{
		switch (LSB(gsUsbRequest.wIndex))
		{
			case 0x00 :	//ep0
				MGC_Write8(MUSB_IDX_CSR0L, MGC_Read8(MUSB_IDX_CSR0L)&(~CSR0L_SendStall)) ;
//				NoData = 1;
				break;
			case 0x01 :	//ep1out
				reg_usb_Index = 0x01;
				MGC_Write8(MUSB_IDX_RXCSRL, MGC_Read8(MUSB_IDX_RXCSRL)&(~RXCSRL_SendStall)|RXCSRL_ClrDataTog) ;
//				NoData = 1;
				break;
			case 0x81 :	//ep1in
				reg_usb_Index = 0x01;
				MGC_Write8(MUSB_IDX_TXCSRL, MGC_Read8(MUSB_IDX_TXCSRL)&(~TXCSRL_SendStall)|TXCSRL_ClrDataTog) ;
//				NoData = 1;
				break;
			case 0x82 :	//ep2in
				reg_usb_Index = 0x02;
				MGC_Write8(MUSB_IDX_TXCSRL, MGC_Read8(MUSB_IDX_TXCSRL)&(~TXCSRL_SendStall)|TXCSRL_ClrDataTog) ;
//				NoData = 1;
				break;
			case VIDEO_EP_IN :	//ep5in
				reg_usb_Index = VIDEO_EP_INDEX;
				printf ("EP5 Clear Stall, %x\n", MGC_Read8(MUSB_IDX_TXCSRL));
				//while(1)
				{
					//BYTE TXCSRL = MGC_Read8(MUSB_IDX_TXCSRL);
					//printf ("%x\n", TXCSRL);
					unsigned int loop = 0;
					while(MGC_Read8(MUSB_IDX_TXCSRL) & TXCSRL_FIFONotEmpty)
					{
						loop ++;
						if(loop >= 0x100000)
							break;
						//printf ("FlushFIFO\n");
						//MGC_Write8(MUSB_IDX_TXCSRL, TXCSRL_TxPktRdy);
						MGC_Write8(MUSB_IDX_TXCSRL, TXCSRL_FlushFIFO) ;
					}
					/*
					printf ("%x\n", TXCSRL);
					
					TXCSRL = MGC_Read8(MUSB_IDX_TXCSRL);
					
					//if(TXCSRL & TXCSRL_FIFONotEmpty)
					{
						printf ("FlushFIFO\n");
						MGC_Write8(MUSB_IDX_TXCSRL, TXCSRL_TxPktRdy);
						MGC_Write8(MUSB_IDX_TXCSRL, TXCSRL_FlushFIFO) ;
					}
					printf ("%x\n", TXCSRL);*/
					//printf ("%x\n", MGC_Read8(MUSB_IDX_TXCSRL));
					//else
					//	break;
				}
				MGC_Write8(MUSB_IDX_TXCSRL, MGC_Read8(MUSB_IDX_TXCSRL)&(~TXCSRL_SendStall)|TXCSRL_ClrDataTog) ;

				UVCStop ();
				//UVCStart ();
				// 删除FIFO中的包
//				NoData = 1;
				break;
			default :
				gbError = TRUE;
				break;
		}
		reg_usb_Index = 0x00;

	}
	else if ((gsDeviceStatus.bUsb_DevState == DEVSTATE_ADDRESS)&&(gsUsbRequest.wValue == 0)
			&&(gsUsbRequest.bmRequestType == M_CMD_STDEPOUT))
	//else if (gsDeviceStatus.bUsb_DevState == DEVSTATE_ADDRESS)
	{
		if (LSB(gsUsbRequest.wIndex) == 0x00)	//ep0
		{
			MGC_Write8(MUSB_IDX_CSR0L, MGC_Read8(MUSB_IDX_CSR0L)&(~CSR0L_SendStall)) ;
//			NoData = 1;
		}
		else
			gbError = TRUE;
	}
	else
	{
		gbError = TRUE;
	}
}

void UsbSetAddress()
{
	if((gsUsbRequest.bmRequestType == M_CMD_STDDEVOUT) && (gsUsbRequest.wValue < 0x80))
	{
		gsEp0Status.bFAddr = LSB(gsUsbRequest.wValue);
//		NoData = 1;
	}
	else
	{
		gbError = TRUE;
	}
}

void UsbGetDescriptor()
{
	if (gsUsbRequest.bmRequestType != M_CMD_STDDEVIN)
	{
		gbError = TRUE;
	}
	else
	{
		switch (MSB(gsUsbRequest.wValue))
		{
			case 0x01 :	//Device		//有外挂处理
				if(gsUsbRequest.wLength <= sizeof(STD_DEV_DSCR))
					gsEp0Status.wBytesLeft = gsUsbRequest.wLength;
				else
					gsEp0Status.wBytesLeft = sizeof(STD_DEV_DSCR);

				{
				 	gsDeviceStatus.bUsb_HighSpeed = usb_init_descriptor();
					gsEp0Status.pbData = (BYTE xdata*)&gsStdDevDesc;
				}
			break;

			case 0x02 :	//Configuration		//有外挂处理
				if (LSB(gsUsbRequest.wValue) > 0x01)
					gbError = TRUE;
				else
				{

					if(gsDeviceStatus.bUsb_HighSpeed)
					{
						{
							gsEp0Status.wBytesLeft = init_descriptor(1);
						}
					}
					else
					{
						{
							gsEp0Status.wBytesLeft = init_descriptor(1);
						}
					}

					((STD_CFG_DSCR *)UpdateBuff)->bDescriptorType = 0x02;	//normal speed is 0x02 ???

					gsEp0Status.pbData = UpdateBuff;

					if(gsUsbRequest.wLength <= 0x40)
						gsEp0Status.wBytesLeft = gsUsbRequest.wLength;
					
				}
				break;

			case 0x03 :	//String 	//有外挂处理
				if (LSB(gsUsbRequest.wValue) == 0x00)	//LangID
				{
					if(gsUsbRequest.wLength <= 0x04)
						gsEp0Status.wBytesLeft = gsUsbRequest.wLength;
					else
						gsEp0Status.wBytesLeft = sizeof(gcLangIDString);

					{
						gsEp0Status.pbData = (BYTE*)&gcLangIDString[0];	
					}
				}
				else if (gsUsbRequest.wIndex == 0x0409)	//LangID == ENGLISH_US 	//有外挂处理
				{
					switch (LSB(gsUsbRequest.wValue))
					{
						case 0x01 :	//Manufacturer
							if(gsUsbRequest.wLength <= 0x12)
								gsEp0Status.wBytesLeft = gsUsbRequest.wLength;
							else
								gsEp0Status.wBytesLeft = sizeof(gcManufacturerString);
							
							{
								gsEp0Status.pbData = (BYTE*)&gcManufacturerString[0];	
							}
						break;

						case 0x02 :	//Produce
							if(gsUsbRequest.wLength <= 0x22)
								gsEp0Status.wBytesLeft = gsUsbRequest.wLength;
							else
								gsEp0Status.wBytesLeft = 0x22;	//sizeof(gcProduceString);
							
							{
								gsEp0Status.pbData = (BYTE*)&gcProduceString[0];
							}
						break;

						case 0x03 :	//Serialnum
							if(gsUsbRequest.wLength <= 0x12)
								gsEp0Status.wBytesLeft = gsUsbRequest.wLength;
							else
								gsEp0Status.wBytesLeft = sizeof(gcSerialnumString);

							{
								gsEp0Status.pbData = (BYTE*)&gcSerialnumString[0];
							}
						break;

						case 0x04 :	//Audio dev string
							if(gsUsbRequest.wLength <= 0x16)
								gsEp0Status.wBytesLeft = gsUsbRequest.wLength;
							else
								gsEp0Status.wBytesLeft = sizeof(gcProduceString1);

							{
								gsEp0Status.pbData = (BYTE*)&gcProduceString1[0];
							}
						break;

						//case 0x05:	//test

						//break;

						default :
							gbError = TRUE;
						break;
					}
				}
				else
				{
					gbError = TRUE;
				}				
				break;
			case 0x06 :	//Device Qualifier		//有外挂处理
				if(gsUsbRequest.wLength<=0x0a)
					gsEp0Status.wBytesLeft = gsUsbRequest.wLength;
				else
					gsEp0Status.wBytesLeft = sizeof(HQualifierData);

				if(reg_usb_Power & bmBIT5)
				{
					{
						gsEp0Status.pbData = (BYTE*)&HQualifierData[0];
					}
				}
				else
				{
					{
						gsEp0Status.pbData = (BYTE*)&FQualifierData[0];
					}
				}
				break;
			case 0x07 :
				if (LSB(gsUsbRequest.wValue) > 0x01)
					gbError = TRUE;
				else
				{					
					if(bOtherSpeedDescFlg)	//use fs-desc as other-speed desc
					{
						{
							gsEp0Status.wBytesLeft = init_descriptor(0);
						}
					}
					else
					{
						{
							gsEp0Status.wBytesLeft = init_descriptor(0);
						}
					}


					((STD_CFG_DSCR *)UpdateBuff)->bDescriptorType = 0x07;	//other-speed is 0x07

					gsEp0Status.pbData = UpdateBuff;

					if(gsUsbRequest.wLength <= 0x40)
						gsEp0Status.wBytesLeft = gsUsbRequest.wLength;
				}
					
				break;
			default :
				gbError = TRUE;
				break;
		}
	}

	if (!gbError)	//没出错,  限制一下传输的总长度,  并转入发送状态
	{
		if (gsEp0Status.wBytesLeft > gsUsbRequest.wLength)
			gsEp0Status.wBytesLeft = gsUsbRequest.wLength;

		gsEp0Status.bState = EP0_TX;										
	}
}

void UsbGetConfiguration()
{
	if (gsDeviceStatus.bUsb_DevState == DEVSTATE_ADDRESS)
		gbBuffer[0] = 0x00;
	else if (gsDeviceStatus.bUsb_DevState == DEVSTATE_CONFIG)
		gbBuffer[0] = 0x01;
	else
		gbError = TRUE;

	usb_setup_in(1);
}

void UsbSetConfiguration()
{
	BYTE bConfig;
	bConfig = LSB(gsUsbRequest.wValue);
	
	if ((gsDeviceStatus.bUsb_DevState == DEVSTATE_DEFAULT) || bConfig > 0x01)	//just one config
		gbError = TRUE;
	else if (bConfig == 0x00)
	{
		gsDeviceStatus.bUsb_DevState = DEVSTATE_ADDRESS;	//set cfg 0
//		NoData = 1;
	}
	else
	{
		gsDeviceStatus.bUsb_DevState = DEVSTATE_CONFIG;		//sef cfg 1
//		NoData = 1;
	}
	
    //clear halt	
	reg_usb_Index = 0x01;
	 MGC_Write8(MUSB_IDX_TXCSRL, MGC_Read8(MUSB_IDX_TXCSRL)&~TXCSRL_SendStall) ;
	//reg_OutSendStall = 0;
	reg_usb_Index = 0x02;
	 MGC_Write8(MUSB_IDX_TXCSRL, MGC_Read8(MUSB_IDX_TXCSRL)&~TXCSRL_SendStall) ;

	 reg_usb_Index = VIDEO_EP_INDEX;
	 MGC_Write8(MUSB_IDX_TXCSRL, MGC_Read8(MUSB_IDX_TXCSRL)&~TXCSRL_SendStall) ;
	
	reg_usb_Index = 0;

	// 20130516 ZhuoYongHong
	// USB控制器经常在Set Configuration信息并收到SetupEnd中断后出现USB复位现象,为什么?
	// 是中断应答Setup END超时?
}

void UsbGetInterface()
{
	if (gsDeviceStatus.bUsb_DevState == DEVSTATE_CONFIG)
	{
		switch (LSB(gsUsbRequest.wIndex))
		{
			case 0x00 :	//Interface 0 : VC
				gbBuffer[0] = 0;
				break;
			case 0x01 :	//Interface 1 : VS alt0, alt1
				if(gsVideoStatus.bStartVideo)
					//gbBuffer[0] = 1;
					gbBuffer[0] = bCurAltSet;
				else
					gbBuffer[0] = 0;
				break;
			case 0x02 :	//Interface 2 : AC
				gbBuffer[0] = 0;
				break;
			case 0x03 :	//Interface 3 : AS alt0, alt1
				if (gsAudioStatus.bStartAudio)
					gbBuffer[0] = 1;
				else
					gbBuffer[0] = 0;
				break;
			default :
				gbError = TRUE;
				break;
		}
	}
	else
		gbError = TRUE;

	usb_setup_in(1);
}

void RamBufferCtrlForOpenWindow()
{	
	usb_start_video();

	//5、sensor接口使能
}


void UsbSetInterface()
{
	BYTE bAltSetting;
	
	bAltSetting = LSB(gsUsbRequest.wValue);

	if ((gsDeviceStatus.bUsb_DevState == DEVSTATE_CONFIG) && (bAltSetting <= 0x0b))
	{
		switch (LSB(gsUsbRequest.wIndex))
		{
			case 0x01 :	//Interface 1 : VS alt0, alt1
				if ((bAltSetting <= 0x0b) && (bAltSetting != 0))	//启动Video
				{
					bCurAltSet = bAltSetting;

					(*pFunc_setInterfaceStart)(); 	//setInterfaceStart();

					UVCStart ();
				}
				else if(bAltSetting == 0)				//终止Video		
				{
					(*pFunc_setInterfaceStop)(); 	//setInterfaceStop();

					UVCStop ();
				}
			break;
			
			case 0x03 :	//Interface 3 : AS alt0, alt1
				if (bAltSetting == 1)	//启动Audio
				{
					//声音A模块enable
					
					gsAudioStatus.bStartAudio = TRUE;

					
					//reg_audio_ctl  |= ADM_EN;
					reg_usb_Index = 0x02;
					MGC_Write8(MUSB_IDX_TXCSRL, MGC_Read8(MUSB_IDX_TXCSRL)|TXCSRL_TxPktRdy) ;
				}
				else		//终止Audio
				{
					gsAudioStatus.bStartAudio = FALSE;
					reg_usb_Index = 0x02;
					MGC_Write8(MUSB_IDX_TXCSRL, MGC_Read8(MUSB_IDX_TXCSRL)|TXCSRL_TxPktRdy) ;
					MGC_Write8(MUSB_IDX_TXCSRL, MGC_Read8(MUSB_IDX_TXCSRL)|TXCSRL_FlushFIFO) ;
					//这里需要注意：清一次出现毛刺
					reg_usb_Index = 0x02;
					MGC_Write8(MUSB_IDX_TXCSRL, MGC_Read8(MUSB_IDX_TXCSRL)|TXCSRL_TxPktRdy) ;
					MGC_Write8(MUSB_IDX_TXCSRL, MGC_Read8(MUSB_IDX_TXCSRL)|TXCSRL_FlushFIFO) ;					
				}
			break;
			
			default :		//设置IF0  与IF2  是否合法???
				gbError = TRUE;
			break;
		}
	}
	//else
	//	gbError = TRUE;	
	
	
	//clear halt
	reg_usb_Index = 0x01;
	MGC_Write8(MUSB_IDX_TXCSRL, MGC_Read8(MUSB_IDX_TXCSRL)&~TXCSRL_SendStall) ;
	MGC_Write8(MUSB_IDX_RXCSRL, MGC_Read8(MUSB_IDX_RXCSRL)&(~RXCSRL_SendStall)) ;

	reg_usb_Index = 0x02;
	MGC_Write8(MUSB_IDX_TXCSRL, MGC_Read8(MUSB_IDX_TXCSRL)&~TXCSRL_SendStall) ;

	reg_usb_Index = VIDEO_EP_INDEX;
	MGC_Write8(MUSB_IDX_TXCSRL, MGC_Read8(MUSB_IDX_TXCSRL)&~TXCSRL_SendStall) ;

	reg_usb_Index = 0;
}

void usb_request()
{
	// printf ("bRequest=%x\n", gsUsbRequest.bRequest);
	switch (gsUsbRequest.bRequest)
	{
    	case GET_STATUS:
			UsbGetStatus();
        break;

		case SET_FEATURE:
        	UsbSetFeature();
        break;		

		case CLEAR_FEATURE:
        	UsbClearFeature();
        break;

    	//case 0x02:		//reserved for future use
		//break;

		//case 0x04:		//reserved for future use
		//break;

		case SET_ADDRESS:
        	UsbSetAddress();
        break;

    	case GET_DESCRIPTOR:
        	UsbGetDescriptor();
        break;

		//case SET_DESCRIPTOR:
		//break;

    	case GET_CONFIGURATION:
        	UsbGetConfiguration();
        break;

    	case SET_CONFIGURATION:
        	UsbSetConfiguration();
        break;

    	case GET_INTERFACE:
        	UsbGetInterface();
        break;

    	case SET_INTERFACE:
        	(*pFunc_UsbSetInterface)();	//UsbSetInterface();
        break;

		//case SYNCH_FRAME:
		//break;

    	default:
        	gbError = TRUE;
        break;
	}
}


void usb_proc_cmd()
{
	BYTE bReqType;

	gbError = FALSE;
	//signed long long ticket_s = XM_GetHighResolutionTickCount ();
	if(gsUsbRequest.wLength == 0) // No Data State
	{
		// Clear RxPktRdy & Set DataEnd	
		reg_usb_EP0 |= CSR0L_ServicedRxPktRdy | CSR0L_DataEnd;
	}
	else if((gsUsbRequest.bmRequestType & 0x80) == bmBIT7)	// Device To Host
	{
		//set SevOutPktRdy to clear OutPktRdy
		reg_usb_EP0 |= CSR0L_ServicedRxPktRdy;
	}
	
	bReqType = (gsUsbRequest.bmRequestType) & 0x60;

	switch (bReqType)
	{
		case 0x00 :				//standard request
			usb_request();
			break;
		case 0x20 : 			//class request，have debug interface
			uvc_request();
			break;
		case 0x60:				//reserved
			break;
		default :
			gbError = TRUE;
			break;
	}

	reg_usb_Index = 0x00;	//防止上面的处理将其切走了
	
	if (gbError)
	{
		//printf ("gbError\n");
		MGC_Write8(MUSB_IDX_CSR0L, MGC_Read8(MUSB_IDX_CSR0L)|CSR0L_SendStall) ;
		gbError = FALSE;
	}

	if((gsUsbRequest.bmRequestType & 0x80) == bmBIT7)
	{
		//printf ("nop\n");
	}
	else
	{
		// Host To Device
		reg_usb_EP0 |= CSR0L_ServicedRxPktRdy | CSR0L_DataEnd;
		//signed long long ticket_e = XM_GetHighResolutionTickCount ();
		//printf ("serice tick=%d\n", (unsigned int)(ticket_e - ticket_s));
	}

	if (gsEp0Status.bState == EP0_CMD)		//不必Tx  或Rx,  故跳至Idle
		gsEp0Status.bState = EP0_IDLE;

}

//ep0发包处理函数
void usb_ep0_transfer()
{
	BYTE bCount;
	BYTE bTmp;
	BYTE * pSrc;
	

	if (gsEp0Status.bTxLock)
		return;

	if (gsEp0Status.wBytesLeft <= EP0_MAXP)
	{
		bCount = gsEp0Status.wBytesLeft;
		gsEp0Status.wBytesLeft = 0;
	}
	else
	{
		bCount = EP0_MAXP;
		gsEp0Status.wBytesLeft -= EP0_MAXP;
	}

	pSrc = gsEp0Status.pbData;
	bTmp = bCount;
	while (bTmp)
	{
		reg_usb_End0 = *pSrc++;
		bTmp--;
	}

	gsEp0Status.pbData += bCount;

	if (bCount < EP0_MAXP)		//最后一个数据包
	{
		// The CPU sets this bit:
		//		1. When setting TxPktRdy for the last data packet.
		reg_usb_EP0 |= CSR0L_DataEnd | CSR0L_TxPktRdy;
		gsEp0Status.bState = EP0_IDLE;
	}
	else
	{
		gsEp0Status.bTxLock = TRUE;
		reg_usb_EP0 |= CSR0L_TxPktRdy;
	}

}
