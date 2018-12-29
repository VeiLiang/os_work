/*****************************************************************************
 *                                                                           *
 *      Copyright Mentor Graphics Corporation 2003-2006                      *
 *                                                                           *
 *                All Rights Reserved.                                       *
 *                                                                           *
 *    THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION            *
 *  WHICH IS THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS              *
 *  LICENSORS AND IS SUBJECT TO LICENSE TERMS.                               *
 *                                                                           *
 ****************************************************************************/

/*
 * IPOD client functions (driver interfacing, command transfer, and
 * command preparation)
 * $Revision: 1.3 $
 */

#include "mu_mem.h"
#include "mu_cdi.h"
#include "mu_ictrl.h"

#include "mu_impl.h"
/****************************** CONSTANTS ********************************/

/******************************** TYPES **********************************/

/******************************* FORWARDS ********************************/

/******************************* GLOBALS *********************************/

/****************************** FUNCTIONS ********************************/
static uint32_t MGC_IpodCmdSend(void* pContext, uint8_t *pCmdBuf)
{
	MGC_IpodDevice *pIpodDevice = (MGC_IpodDevice *)pContext;
	MUSB_DeviceRequest		*pSetup;
	MUSB_ControlIrp 			*pControlIrp;
	uint32_t					SetupLen = sizeof(MUSB_DeviceRequest);
	uint32_t 					iapLen = 0;
	uint32_t					dwStatus = 0xFFFFFFFF;

	IPODIAP_PACKET *pUserPacket=(IPODIAP_PACKET *)pCmdBuf;
	IPODHID_SMALL_TELEGRAM *pSmallTLM=&((pUserPacket->telegram).smallTlm);
	IPODHID_LONG_TELEGRAM *pLongTLM=&((pUserPacket->telegram).longTlm);

	pSetup = &(pIpodDevice->SetupPacket);
	pControlIrp = &(pIpodDevice->ControlIrp);
	memset(pIpodDevice->ctrl_buffer, 0, 128);
	
	switch(pUserPacket->reportID)
	{
		case 0x5:
			iapLen = 0x9;
		break;

		case 0x6:
			iapLen = 0xB;
		break;

		case 0x7:
			iapLen = 0xF;
		break;

		case 0x8:
			iapLen = 0x15;
		break;

		case 0x9:
			iapLen = 0x40;
		break;

		default:
			return dwStatus;
	}

	if(pSmallTLM->tlmlength > 0)
	{
		int mult = 0;

		(pIpodDevice->ctrl_buffer)[0 + SetupLen] = pUserPacket->reportID;
		(pIpodDevice->ctrl_buffer)[1 + SetupLen] = pUserPacket->multipacket;
		(pIpodDevice->ctrl_buffer)[2 + SetupLen] = pSmallTLM->tlmstart;
		(pIpodDevice->ctrl_buffer)[3 + SetupLen] = pSmallTLM->tlmlength;
		(pIpodDevice->ctrl_buffer)[4 + SetupLen] = pSmallTLM->lignoID;

		if((iapLen == 64) && (pSmallTLM->tlmlength > iapLen))
		{
			mult = 2;
		}

		if(mult == 0)
		{
			unsigned char *pcurData = pSmallTLM->ptlmData;

			MUSB_MemCopy(&(pIpodDevice->ctrl_buffer[5 + SetupLen]), pcurData, pSmallTLM->tlmlength);

			/** Prepare the Setup Packet for sending Set Config Request */
			MGC_IPOD_PREPARE_SETUP_PACKET(pSetup,
										 0x21,
										 0x9,
										 (0x02<<8) | (pIpodDevice->ctrl_buffer)[0 + SetupLen],
										 0x02,
										 iapLen);

			MUSB_MemCopy(pIpodDevice->ctrl_buffer, pSetup, SetupLen);
	
			/** Fill Control Irp */
			MGC_IPOD_FILL_CONTROL_IRP(pIpodDevice,
									  pControlIrp,
									  pIpodDevice->ctrl_buffer,
									  SetupLen + iapLen,
									  NULL,
									  0,
									  NULL);


			//printk("pPort is 0x%x\n", pIpodDevice->pUsbDevice->pPort);

			dwStatus = MUSB_StartControlTransfer (pIpodDevice->pUsbDevice->pPort, pControlIrp);
		}
		else
		{
			unsigned int leftLen = 0;
			unsigned int curLongLen = 0;
			unsigned int curIndex = 0;
			unsigned int i = 0;
			unsigned char *pcurData = NULL;
			
			(pIpodDevice->ctrl_buffer)[1 + SetupLen] = mult;
			curLongLen = pSmallTLM->tlmlength;
			pcurData = pSmallTLM->ptlmData;
			leftLen = curLongLen;

			do
			{
				if(leftLen == curLongLen)
				{
					for(i=0; i<(iapLen-5); i++)
					{
						(pIpodDevice->ctrl_buffer)[5 + i + SetupLen]= pcurData[curIndex++];
					}
				}
				else if(leftLen <= (iapLen - 2))
				{
					memset(pIpodDevice->ctrl_buffer, 0, 128);
					
					iapLen = leftLen + 2;
					(pIpodDevice->ctrl_buffer)[1 + SetupLen] = 1;

					for(i=0; i<(iapLen-2); i++)
					{
						(pIpodDevice->ctrl_buffer)[2 + i + SetupLen]= pcurData[curIndex++];
					}
					
					if(iapLen <= 0x9)
					{
						(pIpodDevice->ctrl_buffer)[0 + SetupLen] = 0x5;
						iapLen = 0x9;
					}
					else if(iapLen <= 0xB)
					{
						(pIpodDevice->ctrl_buffer)[0 + SetupLen] = 0x6;
						iapLen = 0xB;
					}
					else if(iapLen <= 0xF)
					{
						(pIpodDevice->ctrl_buffer)[0 + SetupLen] = 0x7;
						iapLen = 0xF;
					}
					else if(iapLen <= 0x15)
					{
						(pIpodDevice->ctrl_buffer)[0 + SetupLen] = 0x8;
						iapLen = 0x15;
					}
					else
					{
						(pIpodDevice->ctrl_buffer)[0 + SetupLen] = 0x9;
						iapLen = 64;
					}
				}
				else
				{
					(pIpodDevice->ctrl_buffer)[1 + SetupLen] = 3;

					for(i=0; i<(iapLen-2); i++)
					{
						(pIpodDevice->ctrl_buffer)[2 + i + SetupLen]= pcurData[curIndex++];
					}
				}

				/** Prepare the Setup Packet for sending Set Config Request */
				MGC_IPOD_PREPARE_SETUP_PACKET(pSetup,
											 0x21,
											 0x9,
											 (0x02<<8) | (pIpodDevice->ctrl_buffer)[0 + SetupLen],
											 0x02,
											 iapLen);

				MUSB_MemCopy(pIpodDevice->ctrl_buffer, pSetup, SetupLen);
				
				/** Fill Control Irp */
				MGC_IPOD_FILL_CONTROL_IRP(pIpodDevice,
										  pControlIrp,
										  pIpodDevice->ctrl_buffer,
										  SetupLen + iapLen,
										  NULL,
										  0,
										  NULL);
				
				dwStatus = MUSB_StartControlTransfer (pIpodDevice->pUsbDevice->pPort, pControlIrp);
				if(dwStatus)
				{
					MUSB_PRINTK("MUSB_StartControlTransfer dwStatus is 0x%x===\n", dwStatus); 
					break;
				}
				else
				{
					OSTimeDly(20);
					leftLen = curLongLen - curIndex;
				}
			}while(leftLen>0);
		}
	}
	else
	{
		unsigned int curLongLen = 0;
		unsigned int leftLen = 0;
		unsigned int curIndex = 0;
		unsigned int i = 0;
		unsigned char *pcurData = NULL;

		curLongLen = (pLongTLM->lengthH<<8) | (pLongTLM->lengthL);
		pcurData = pLongTLM->ptlmData;
		leftLen = curLongLen;
		do
		{
			(pIpodDevice->ctrl_buffer)[0 + SetupLen]=pUserPacket->reportID;

			if(leftLen == curLongLen)
			{
				(pIpodDevice->ctrl_buffer)[1 + SetupLen] = 2;
				(pIpodDevice->ctrl_buffer)[2 + SetupLen] = pLongTLM->tlmstart;
				(pIpodDevice->ctrl_buffer)[3 + SetupLen] = pLongTLM->markerlen;
				(pIpodDevice->ctrl_buffer)[4 + SetupLen] = pLongTLM->lengthH;
				(pIpodDevice->ctrl_buffer)[5 + SetupLen] = pLongTLM->lengthL;
				(pIpodDevice->ctrl_buffer)[6 + SetupLen] = pLongTLM->lignoID;

				for(i=0; i<(iapLen-7); i++)
				{
					(pIpodDevice->ctrl_buffer)[7 + i + SetupLen]= pcurData[curIndex++];
				}
			}
			else if(leftLen <= (iapLen-2))
			{
				iapLen = leftLen+2;
				(pIpodDevice->ctrl_buffer)[1 + SetupLen] = 1;

				for(i=0; i<(iapLen-2); i++)
				{
					(pIpodDevice->ctrl_buffer)[2 + i + SetupLen]= pcurData[curIndex++];
				}
			}
			else
			{
				(pIpodDevice->ctrl_buffer)[1 + SetupLen] = 3;

				for(i=0; i<(iapLen-2); i++)
				{
					(pIpodDevice->ctrl_buffer)[2 + i + SetupLen]= pcurData[curIndex++];
				}
			}

			/** Prepare the Setup Packet for sending Set Config Request */
			MGC_IPOD_PREPARE_SETUP_PACKET(pSetup,
										 0x21,
										 0x9,
										 (0x02<<8) | (pIpodDevice->ctrl_buffer)[0 + SetupLen],
										 0x02,
										 iapLen);

			MUSB_MemCopy(pIpodDevice->ctrl_buffer, pSetup, SetupLen);
	
			/** Fill Control Irp */
			MGC_IPOD_FILL_CONTROL_IRP(pIpodDevice,
									  pControlIrp,
									  pIpodDevice->ctrl_buffer,
									  SetupLen + iapLen,
									  NULL,
									  0,
									  NULL);
			
			dwStatus = MUSB_StartControlTransfer (pIpodDevice->pUsbDevice->pPort, pControlIrp);
			if(dwStatus)
			{
				MUSB_PRINTK("MUSB_StartControlTransfer dwStatus is 0x%x===\n", dwStatus); 
				break;
			}
			else
			{
				leftLen = curLongLen - curIndex;
			}
		}while(leftLen > 0);
	}

	//MUSB_PRINTK("===MGC_IpodCmdSend dwStatus is 0x%x===\n", dwStatus); 
	return dwStatus;
}

static uint8_t cmd_count = 0;
uint32_t MUSB_IPOD_Control(uint16_t Command, void *Arg)
{
	uint32_t dwStatus = 0xFFFFFFFF;
	uint8_t myerr;
	MGC_IpodDevice *pIpodDevice =  MGC_GetIpodDeviceContext();

	if(!pIpodDevice)
	{
		MUSB_PRINTK("IpodDevice is not Present \n");
		return dwStatus;
	}

	if(!Arg)
	{
		MUSB_PRINTK("Arg is NULL \n");
		return dwStatus;
	}
	
	if(cmd_count)
		OSSemPend(IpodDeviceSem, 0, &myerr);
	cmd_count = 1;
	
	switch(Command)
	{
		case REGISTER_USER_FUNC:
		{
			pIpodDevice->user_event = (uint32_t (*)(void *))Arg;
			dwStatus = 0;
			break;
		}
		case REQUEST_CONTROL_IPOD:
		{
			dwStatus = MGC_IpodCmdSend(pIpodDevice, (uint8_t *)Arg);		
			break;
		}

		default:
			MUSB_PRINTK("This Command is not Support \n");
			break;
	}

	//MUSB_PRINTK("MUSB_IPOD_Control Command is 0x%x Status is 0x%x \n", Command, dwStatus);
	OSSemPost(IpodDeviceSem);
	cmd_count = 0;
	return dwStatus;
}

uint32_t MGC_Fill_IPODIAP_SmallPacket(IPODIAP_PACKET *pIAPPacket,unsigned char reportID,unsigned char mult,unsigned char lingoID,unsigned char smalllen,unsigned char *pdata)
{
	int ret=-1;
	IPODHID_SMALL_TELEGRAM *pSmallTLM=NULL;
	unsigned char checksum=0;
	unsigned int i;

	if((pIAPPacket != NULL) && (pdata != NULL))
	{
		pSmallTLM = &((pIAPPacket->telegram).smallTlm);
		pIAPPacket->reportID = reportID;
		pIAPPacket->multipacket = mult;
		pSmallTLM->tlmstart = START_TELEGRAM;
		pSmallTLM->lignoID  =  lingoID;
		pSmallTLM->tlmlength = smalllen;
		pSmallTLM->ptlmData = pdata;
		checksum +=smalllen;
		checksum +=lingoID;
		for(i=0;i<(smalllen-1);i++)
		{
			checksum += pdata[i];
		}
		checksum =0-checksum;
		pdata[i]=checksum;
		ret = 0;
	}
	return ret;
}

uint32_t MGC_Fill_IPODIAP_LongPacket(IPODIAP_PACKET *pIAPPacket,unsigned char reportID,unsigned char mult,unsigned char lingoID,unsigned int longlen,unsigned char *pdata)
{
	int ret=-1;
	IPODHID_LONG_TELEGRAM *pLongTLM=NULL;
	unsigned char checksum=0;
	unsigned int i=0;
	if((pIAPPacket != NULL) && (pdata != NULL))
	{
		pLongTLM = &((pIAPPacket->telegram).longTlm);
		pIAPPacket->reportID = reportID;
		pIAPPacket->multipacket = mult;
		pLongTLM->tlmstart = START_TELEGRAM;
		pLongTLM->markerlen = 0;
		pLongTLM->lengthH = (longlen>>8);
		pLongTLM->lengthL = (longlen&0x0FF);
		pLongTLM->lignoID = lingoID;
		pLongTLM->ptlmData = pdata;

		checksum += pLongTLM->lengthH;
		checksum += pLongTLM->lengthL;
		checksum += lingoID;
		for(i=0;i<longlen-1;i++)
		{
			checksum += pdata[i];
		}
		checksum =0-checksum;
		pdata[i]=checksum;
	}
	return ret;
}

