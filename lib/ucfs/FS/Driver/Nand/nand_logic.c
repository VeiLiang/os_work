/***********************************************************************
*Copyright (c)2012  Arkmicro Technologies Inc. 
*All Rights Reserved 
*
*Filename:    nand_logic.c
*Version :    1.0 
*Date    :    2008.10.07
*Author  :     
*Abstract:     
*History :     
* 
*Version :    2.0 
*Date    :    2012.02.27
*Author  :     
*Abstract:    ark1660  Ver1.0 MPW driver remove waring
*History :    1.0

************************************************************************/

 
#include <string.h>
#include "printk.h"
#include "types.h"
#include "nand_Driver.h"
#include "nand_logic.h"
#include "nand_base.h"
//////////////////////////////////////////////////////////////////////////////////////////

#define	USE_DMA		1 
#define	ECC_SUPPORT	1

unsigned short BadBlkNum = 0;
//����������
static BBTINDEX __BadBlockIndex[(NF_MAX_BLOCKNUM>>5)];
//�����
static BBTENTRY __BadBlockTable[NF_MAX_BADBLOCK];

//static unsigned char SpareBuff[128];   //���֧�ֵ���ô��(4K��ҳ��С��Ӧ)  removed waring

//Ϊ__BadBlockTable�����������ָʾ��һ�������������е�λ��
static unsigned int tableTopPos = 0;
//��������ǰλ�ã�ָʾ��һ�������ڱ�������λ��
unsigned int bakupTopPos = 0;

static unsigned char CRC_Check(unsigned char* progArgu)
{
	unsigned char Sum;
	unsigned char arguLen;

	Sum = 0;
	for (arguLen=31; arguLen!=0; arguLen --)
	{
		// NOTE: The operation is an unsigned long rotate right
		Sum = ((Sum & 1) ? 0x80 : 0) + (Sum >> 1) + *progArgu++;
	}
	return (Sum);
}

int NFC_CheckIsBadBlock(unsigned short PhyBlkNo)
{

	if(__BadBlockIndex[PhyBlkNo>>5].bitIsBadBlock & (1<<(PhyBlkNo&0x1f)))
	{
		//invalid block
		return NAND_FAIL;
	}
	else
	{
		//valid block
		return NAND_SUCCE;
	}
}

int NFC_MarkAsBadBlock(unsigned int BlkNo)
{
	int i;
	int bakBlock;	

	//���Block�Ƿ�Ϊ����,����ǻ���,���BBT�е�replace block��һ��
	//������ǻ���,��Ϊ����bakup block����һ��replace block
	if (NFC_CheckIsBadBlock(BlkNo))
	{
		for (i = 0; i < tableTopPos;i++)
		{
			if (BlkNo == __BadBlockTable[i].mBadBlock)
			{
				while (1)
				{
					bakBlock = NandInfo.BlkNum - 1 - bakupTopPos;
					
					if (NFC_CheckIsBadBlock(bakBlock))
					{
						//bak block is bad����Index���ʶ
						__BadBlockIndex[bakBlock >> 5].bitIsBadBlock |= (1 << (bakBlock & 0x1F));
						if (!(__BadBlockIndex[bakBlock >> 5].mPosInBBT))
						{
							__BadBlockIndex[bakBlock >> 5].mPosInBBT = (bakBlock & 0x1F);
						}
						
						bakupTopPos++;
					}
					else
					{
						//bak block is ok
						__BadBlockTable[i].mReplaceBlock = bakBlock;
						bakupTopPos++;
						printk("NFC_CheckIsBadBlock 111 \n");
						break;
					}
				}
				printk("NFC_CheckIsBadBlock 222 \n");
				break;
			}
		}
	}
	else
	{
		__BadBlockIndex[BlkNo >> 5].bitIsBadBlock |= (1 << (BlkNo & 0x1F));
		
		//���û�м�¼�����λ�þ����¼�¼.
		if (!(__BadBlockIndex[BlkNo >> 5].mPosInBBT))
		{
			__BadBlockIndex[BlkNo >> 5].mPosInBBT = (BlkNo & 0x1F);
		}
		
		__BadBlockTable[tableTopPos].mBadBlock = BlkNo;
		
		while (1)
		{
			bakBlock = NandInfo.BlkNum - 1 - bakupTopPos;
			
			if (NFC_CheckIsBadBlock(bakBlock))
			{
				//bak block is bad����Index���ʶ
				__BadBlockIndex[bakBlock >> 5].bitIsBadBlock |= (1 << (bakBlock & 0x1F));

				if (!(__BadBlockIndex[bakBlock >> 5].mPosInBBT))
				{
					__BadBlockIndex[bakBlock >> 5].mPosInBBT = (bakBlock & 0x1F);
				}

				bakupTopPos++;
			}
			else
			{
				//bak block is ok
				__BadBlockTable[tableTopPos++].mReplaceBlock = bakBlock;
				bakupTopPos++;
				printk("NFC_CheckIsBadBlock 333 \n");
				break;
			}
		}
	}

	if (bakupTopPos > BadBlkNum)
	{
		//����������
		return NAND_FAIL;
	}
			
	return NAND_SUCCE;
}


void NFC_BuildBBT(void)
{
	int i, j;

	//Ϊ__BadBlockTable�����������ָʾ��һ�������������е�λ��
	tableTopPos = 0;
	//��������ǰλ�ã�ָʾ��һ�������ڱ�������λ��
	bakupTopPos = 0;
	
	//Creat bad block table and replace block
	for (i = 0;i < ((NandInfo.BlkNum - BadBlkNum) >> 5);i++)
	{
		for (j = 0;j < 32;j++)
		{
			if (NFC_CheckIsBadBlock(i * 32 + j))
			{
				__BadBlockTable[tableTopPos].mBadBlock = i * 32 + j;
				
				while (1)
				{
					if (NFC_CheckIsBadBlock(NandInfo.BlkNum - 1 - bakupTopPos))
					{
						//bak block is bad
						bakupTopPos++;
					}
					else
					{
						//bak block is ok
						__BadBlockTable[tableTopPos++].mReplaceBlock = NandInfo.BlkNum - 1 - bakupTopPos;
						bakupTopPos++;
						printk("NFC_BuildBBT \n");
						break;
					}
				}
			}
		}
	}
}

int NFC_FormatFlash(void)
{
        int i, j;
	 int badBlkNum = 0;

	 memset(__BadBlockIndex, 0, sizeof(__BadBlockIndex)/sizeof(BBTINDEX));
	 memset(__BadBlockTable, 0, sizeof(__BadBlockTable)/sizeof(BBTENTRY));
		
	//Creat Bad Block Index, and get bad block numbers
	 for (i = 0;i < (NandInfo.BlkNum >> 5);i++)
	 {
	        for (j = 0;j < 32;j++)
		 {
		        if ((!i) && (j < 11))
			 {
			        continue;
			 }
			
			 if (NAND_FAIL == NandBlkErase(i * 32 + j))
			 {
				//i * 32 + j block is bad
			        __BadBlockIndex[i].bitIsBadBlock |= (1 << j);
				
				 if (!__BadBlockIndex[i].mPosInBBT)
				 {
					//ÿ32���У���¼��һ�γ��ֻ����λ��
				        __BadBlockIndex[i].mPosInBBT = j;
				 }
				
				 //��������1
				 badBlkNum++;
			 }
		 }
	}
	
	printk("badBlockNum=%d\n>", badBlkNum);
	
#ifndef NANDFLASH_ALLOW_OVER_MAX_BAD_BLOCK
	//���Nandflash�������Ƿ񳬳�������󻵿�
	if (badBlkNum > NF_MAX_BADBLOCK)
	{
		return NAND_FAIL;
	}
#endif
	
	//ensure bakup block sizes, size = bad + block/512 * 4
	BadBlkNum = badBlkNum + (NandInfo.BlkNum >> 9) * 4;
	
	//Creat BBT in the ram
	NFC_BuildBBT();
    
	return NAND_SUCCE;
} 

int NFC_IsFsIoctl(unsigned char *buffer)
{
        unsigned int ReadStatus;

#if USE_DMA
	 if(NandInfo.PageSize > 512)
        	ReadStatus = NandPageReadDMA(START_BLK, 0, buffer, ECC_SUPPORT);
	 else
	 	ReadStatus = NandPageReadDMAFor512(START_BLK, 0, buffer, ECC_SUPPORT);
#else
	if(NandInfo.PageSize > 512)
	        	ReadStatus = NandPageRead(START_BLK, 0, buffer, ECC_SUPPORT);
	 else
	 	ReadStatus = NandPageReadFor512(START_BLK, 0, buffer, ECC_SUPPORT);
#endif
	 
        if(NAND_FAIL==ReadStatus)
                return NAND_FAIL;; 

        if((*(buffer + 510) == 0x55)&&(*(buffer + 511) == 0xaa))
        {
                return NAND_SUCCE;
        }

        return NAND_FAIL;
}

int NFC_IsFormated(unsigned char *buffer)
{
	unsigned char cmpBuf[32];
	unsigned char CRCValue;
       unsigned int ReadStatus;
	int i;
        		
	for(i=1;i<START_BLK;i++)
	{
#if USE_DMA
		if(NandInfo.PageSize > 512)
			ReadStatus = NandPageReadDMA(i, 0, buffer, ECC_SUPPORT);
		else
	 		ReadStatus = NandPageReadDMAFor512(START_BLK, 0, buffer, ECC_SUPPORT);
#else
		if(NandInfo.PageSize > 512)
			ReadStatus = NandPageRead(i, 0, buffer, ECC_SUPPORT);
		else
	 		ReadStatus = NandPageReadFor512(START_BLK, 0, buffer, ECC_SUPPORT);
#endif

              if(NAND_FAIL==ReadStatus)
                    continue;  
		
		memset(cmpBuf, 0x0, sizeof(cmpBuf));
		memcpy(cmpBuf, buffer + 0x20, sizeof(cmpBuf));
		CRCValue = CRC_Check(cmpBuf);	
				 
		if(*(buffer + 0x3F) == CRCValue)
		{
			printk("NFC_IsFormated(), NandFlash has been formated! Blk = %d\n>",i);		
			return NAND_SUCCE;
		}	
	 }
	 
	 printk("NandFlash is not  formated \n");
	 return NAND_FAIL;
}

int NFC_SaveSysParaAndBBT(unsigned char *buffer)
{
        unsigned int i;                
        unsigned int res;
        unsigned char CrcDataBuff[32];
        unsigned char CharLen;
        unsigned char CrcValue;
        unsigned short PageNum;
        unsigned int OffsetSize;
        unsigned short PageIndex;

	 res = 1;
	 for(i=1;i<START_BLK;i++)
	 {
                if(NandBlkErase(i) == 1)
		        res++;
		  else
		        break;  
	}

	if(res > (START_BLK-1))
	{
	        printk("bakup area is over, re-format\n>");
	        return -1;
	}
			
	memset(buffer, 0xFF, NandInfo.PageSize);
	memset(CrcDataBuff, 0x0, sizeof(CrcDataBuff));
		
	//format flag
	CharLen = sizeof("NAND FLASH FORMAT");	
	memcpy(CrcDataBuff, (unsigned char *)"NAND FLASH FORMAT", CharLen);	
	memcpy(buffer + 0x20, CrcDataBuff,sizeof(CrcDataBuff));
	
	//CRC flag
	CrcValue = CRC_Check(CrcDataBuff);
	*(buffer + 0x3F) = CrcValue;
	
	//flash info
	*(buffer + 0x40) = 1;
	*(buffer + 0x41) = 0;
	*(buffer + 0x42) = 0;
	*(buffer + 0x43) = 0;
	
	*(buffer + 0x44) = NandInfo.BlkNum;
	*(buffer + 0x45) = (NandInfo.BlkNum >> 8);
	*(buffer + 0x46) = (NandInfo.BlkNum >> 16);
	*(buffer + 0x47) = (NandInfo.BlkNum >> 24);
	
  	*(buffer + 0x48) = NF_MAX_BADBLOCK;// removed waring NG 
	*(buffer + 0x49) = (NF_MAX_BADBLOCK >> 8);
	*(buffer + 0x4a) = (NF_MAX_BADBLOCK >> 16);
	*(buffer + 0x4b) = (NF_MAX_BADBLOCK >> 24);
		
	*(buffer + 0x4c) = (NandInfo.BlkNum - NF_MAX_BADBLOCK);
	*(buffer + 0x4d) = ((NandInfo.BlkNum - NF_MAX_BADBLOCK) >> 8);
	*(buffer + 0x4e) = ((NandInfo.BlkNum - NF_MAX_BADBLOCK) >> 16);
	*(buffer + 0x4f) = ((NandInfo.BlkNum - NF_MAX_BADBLOCK) >> 24);
	
	*(buffer + 0x50) = BadBlkNum;
	*(buffer + 0x51) = (BadBlkNum >> 8);
	*(buffer + 0x52) = (BadBlkNum >> 16);
	*(buffer + 0x53) = (BadBlkNum >> 24);
	
	*(buffer + 0x54) = tableTopPos;
	*(buffer + 0x55) = (tableTopPos >> 8);
	*(buffer + 0x56) = (tableTopPos >> 16);
	*(buffer + 0x57) = (tableTopPos >> 24);
	
	*(buffer + 0x58) = bakupTopPos;
	*(buffer + 0x59) = (bakupTopPos >> 8);
	*(buffer + 0x5a) = (bakupTopPos >> 16);
	*(buffer + 0x5b) = (bakupTopPos >> 24);

#if USE_DMA
	//write info to block 0 page 0
	if(NandInfo.PageSize > 512)
	{
		if (NAND_SUCCE!= NandPageWriteDMA(res, 0, buffer, ECC_SUPPORT))
		{
			printk("Write Page 0 is error!\n>");
			return NAND_FAIL;
		}
	}
	else
	{
		if (NAND_SUCCE!= NandPageWriteDMAFor512(res, 0, buffer, ECC_SUPPORT))
		{
			printk("Write Page 0 is error!\n>");
			return NAND_FAIL;
		}
	}
#else
	//write info to block 0 page 0
	if(NandInfo.PageSize > 512)
	{
		if (NAND_SUCCE!= NandPageWrite(res, 0, buffer, ECC_SUPPORT))
		{
			printk("Write Page 0 is error!\n>");
			return NAND_FAIL;
		}
	}
	else
	{
		if (NAND_SUCCE!= NandPageWriteFor512(res, 0, buffer, ECC_SUPPORT))
		{
			printk("Write Page 0 is error!\n>");
			return NAND_FAIL;
		}
	}
#endif

	 //����Index��Ҫд��ҳ
	 PageNum = sizeof(__BadBlockIndex) /NandInfo.PageSize;
	 
	 //����Index��ҳ�����µ��ֽ���
	 OffsetSize = sizeof(__BadBlockIndex) % NandInfo.PageSize;
	
	 if(0!=OffsetSize)
	 {
	        PageNum++;
	 }
        printk("Write __BadBlockIndex need %d Page\n",PageNum);	
        PageIndex = PageNum;
	 for (i = 0;i < PageNum;i++)
	 {
	        if (OffsetSize && (i == (PageNum - 1)))
		 {
		        memset(buffer, 0, sizeof(buffer));
			 memcpy(buffer, (__BadBlockIndex + (i * NandInfo.PageSize)/sizeof(BBTINDEX)), OffsetSize);
		 }
		 else
		 {
		        memcpy(buffer, (__BadBlockIndex + (i * NandInfo.PageSize)/sizeof(BBTINDEX)), NandInfo.PageSize);
		 }

#if USE_DMA
		 if(NandInfo.PageSize > 512)
		 {
			 if (NAND_SUCCE != NandPageWriteDMA(res, 1 + i, buffer, ECC_SUPPORT))
			 {
	                      printk("Write Page %d is error!\n>",1 + i);
			        return NAND_FAIL;
			 }
		 }
		 else
		 {
		 	 if (NAND_SUCCE != NandPageWriteDMAFor512(res, 1 + i, buffer, ECC_SUPPORT))
			 {
	                      printk("Write Page %d is error!\n>",1 + i);
			        return NAND_FAIL;
			 }
		 }
#else
		 if(NandInfo.PageSize > 512)
		 {
			 if (NAND_SUCCE != NandPageWrite(res, 1 + i, buffer, ECC_SUPPORT))
			 {
	                      printk("Write Page %d is error!\n>",1 + i);
			        return NAND_FAIL;
			 }
		 }
		 else
		 {
		 	 if (NAND_SUCCE != NandPageWriteFor512(res, 1 + i, buffer, ECC_SUPPORT))
			 {
	                      printk("Write Page %d is error!\n>",1 + i);
			        return NAND_FAIL;
			 }
		 }
#endif
	}

	//����BBT��Ҫд��ҳ
	PageNum = sizeof(__BadBlockTable) / NandInfo.PageSize;
	
	//����BBT��ҳ�����µ��ֽ���
	OffsetSize = sizeof(__BadBlockTable) % NandInfo.PageSize;
	
	if (OffsetSize)
	{
		PageNum++;
	}
       printk("Write __BadBlockTable need %d Page\n",PageNum);		
	for (i = 0;i < PageNum;i++)
	{
		if (OffsetSize && (i == (PageNum - 1)))
		{
			memset(buffer, 0, sizeof(buffer));
			memcpy(buffer, (__BadBlockTable + (i * NandInfo.PageSize)/sizeof(BBTENTRY)), OffsetSize);
		}
		else
		{
			memcpy(buffer, (__BadBlockTable + (i * NandInfo.PageSize)/sizeof(BBTENTRY)), NandInfo.PageSize);
		}

#if USE_DMA
		if(NandInfo.PageSize > 512)
		{
			if (NAND_SUCCE!= NandPageWriteDMA(res, 1 + i + PageIndex, buffer, ECC_SUPPORT))
			{
				printk("Write Page %d is error!\n>",1 + i + PageIndex);
				return NAND_FAIL;
			}		
		}
		else
		{
			if (NAND_SUCCE!= NandPageWriteDMAFor512(res, 1 + i + PageIndex, buffer, ECC_SUPPORT))
			{
				printk("Write Page %d is error!\n>",1 + i + PageIndex);
				return NAND_FAIL;
			}
		}
#else
		if(NandInfo.PageSize > 512)
		{
			if (NAND_SUCCE!= NandPageWrite(res, 1 + i + PageIndex, buffer, ECC_SUPPORT))
			{
				printk("Write Page %d is error!\n>",1 + i + PageIndex);
				return NAND_FAIL;
			}		
		}
		else
		{
			if (NAND_SUCCE!= NandPageWriteFor512(res, 1 + i + PageIndex, buffer, ECC_SUPPORT))
			{
				printk("Write Page %d is error!\n>",1 + i + PageIndex);
				return NAND_FAIL;
			}
		}
#endif
	}
    
	return NAND_SUCCE;
} 

int  NFC_ReadSysParaAndBBT(unsigned char *buffer)
{
        unsigned int i;
        unsigned int res;
        unsigned char CrcBuff[32];
        unsigned char CrcValue;
        unsigned short PageNum;
        unsigned int OffsetSize;
        unsigned short PageIndex;

        for(i=1;i<START_BLK;i++)
        {
#if USE_DMA
		  if(NandInfo.PageSize > 512)
		  {
		  	if(NAND_FAIL==NandPageReadDMA(i, 0, buffer, ECC_SUPPORT))
                        continue;
		  }
		  else
		  {
		  	if(NAND_FAIL==NandPageReadDMAFor512(i, 0, buffer, ECC_SUPPORT))
                        continue;
		  }
#else
		  if(NandInfo.PageSize > 512)
		  {
		  	if(NAND_FAIL==NandPageRead(i, 0, buffer, ECC_SUPPORT))
                        continue;
		  }
		  else
		  {
		  	if(NAND_FAIL==NandPageReadFor512(i, 0, buffer, ECC_SUPPORT))
                        continue;
		  }
#endif		
                memset(CrcBuff, 0x0, sizeof(CrcBuff));
    	         memcpy(CrcBuff, buffer + 0x20, sizeof(CrcBuff));
    	         CrcValue = CRC_Check(CrcBuff);	

                if(*(buffer + 0x3F) == CrcValue)
                {
		        break;                   
                }
        }
        
        if(i==START_BLK)
        {
                return NAND_FAIL;
        }
        
	 res = i;

#if USE_DMA
	 if(NandInfo.PageSize > 512)
	 	NandPageReadDMA(i, 0, buffer, ECC_SUPPORT);
	 else
	 	NandPageReadDMAFor512(i, 0, buffer, ECC_SUPPORT);
#else
	 if(NandInfo.PageSize > 512)
	 	NandPageRead(i, 0, buffer, ECC_SUPPORT);
	 else
	 	NandPageReadFor512(i, 0, buffer, ECC_SUPPORT);
#endif

	 BadBlkNum = *(buffer + 0x50) + (*(buffer + 0x51) << 8) + (*(buffer + 0x52) << 16) + (*(buffer + 0x53) << 24);
	 tableTopPos = *(buffer + 0x54) + (*(buffer + 0x55) << 8) + (*(buffer + 0x56) << 16) + (*(buffer + 0x57) << 24);
	 bakupTopPos = *(buffer + 0x58) + (*(buffer + 0x59) << 8) + (*(buffer + 0x5a) << 16) + (*(buffer + 0x5b) << 24);

	 //������Ҫ����ҳ
	 PageNum = sizeof(__BadBlockIndex) / NandInfo.PageSize;
	
	 //������ҳ�����µ��ֽ���
	 OffsetSize = sizeof(__BadBlockIndex) % NandInfo.PageSize;
	
	 if(OffsetSize)
	 {
	        PageNum++;
	 }

	 for (i = 0;i < PageNum;i++)
	 {
#if USE_DMA
	 	 if(NandInfo.PageSize > 512)
	        	NandPageReadDMA(res, 1 + i, buffer, ECC_SUPPORT);
		 else
		 	NandPageReadDMAFor512(res, 1 + i, buffer, ECC_SUPPORT);
#else
	 	 if(NandInfo.PageSize > 512)
	        	NandPageRead(res, 1 + i, buffer, ECC_SUPPORT);
		 else
		 	NandPageReadFor512(res, 1 + i, buffer, ECC_SUPPORT);
#endif

		 if (OffsetSize && (i == (PageNum - 1)))
		 {
			 //����ҳ�����µ��ֽ���
		        memcpy((__BadBlockIndex + (i * NandInfo.PageSize)/sizeof(BBTINDEX)), buffer, OffsetSize);
		 }
		 else
		 {
		        memcpy((__BadBlockIndex + (i * NandInfo.PageSize)/sizeof(BBTINDEX)), buffer, NandInfo.PageSize);
		 }
	 }
        PageIndex = PageNum;
    
	 //����BBT��Ҫ����ҳ
	 PageNum = sizeof(__BadBlockTable) / NandInfo.PageSize;
	
	 //����BBT��ҳ�����µ��ֽ���
	 OffsetSize = sizeof(__BadBlockTable) % NandInfo.PageSize;
	
	 if(OffsetSize)
	 {
	        PageNum++;
	 }

	 for (i = 0;i < PageNum;i++)
	 {
#if USE_DMA
	 	 if(NandInfo.PageSize > 512)
	        	NandPageReadDMA(res, 1 + i + PageIndex, buffer, ECC_SUPPORT);
		 else
		 	NandPageReadDMAFor512(res, 1 + i + PageIndex, buffer, ECC_SUPPORT);
#else
	 	 if(NandInfo.PageSize > 512)
	        	NandPageRead(res, 1 + i + PageIndex, buffer, ECC_SUPPORT);
		 else
		 	NandPageReadFor512(res, 1 + i + PageIndex, buffer, ECC_SUPPORT);
#endif
		 if(OffsetSize && (i == (PageNum - 1)))
		 {
			//����ҳ�����µ��ֽ���
		        memcpy((__BadBlockTable + (i * NandInfo.PageSize)/sizeof(BBTENTRY)), buffer, OffsetSize);
		 }
		 else
		 {
		        memcpy((__BadBlockTable + (i * NandInfo.PageSize)/sizeof(BBTENTRY)), buffer, NandInfo.PageSize);
		 }
	}
	
	return NAND_SUCCE;
}

int NFC_MoveDataToValidBlock(unsigned short scrBlockNo, unsigned short desBlockNo, unsigned int endPage, unsigned char *buffer)
{
	unsigned int i;
       unsigned int Status;

	//����ĺ���������(Buff����û���)
	
	//�ѳ��ִ���ҳ������д���µĿ��Ӧҳ��
	for (i = endPage;i < NandInfo.PagePerBlk;i++)
	{
#if USE_DMA
		if(NandInfo.PageSize > 512)
              	Status = NandPageWriteDMA(desBlockNo, i, buffer, ECC_SUPPORT);
		else
			Status = NandPageWriteDMAFor512(desBlockNo, i, buffer, ECC_SUPPORT);
#else
		if(NandInfo.PageSize > 512)
              	Status = NandPageWrite(desBlockNo, i, buffer, ECC_SUPPORT);
		else
			Status = NandPageWriteFor512(desBlockNo, i, buffer, ECC_SUPPORT);
#endif		
		if(NAND_SUCCE != Status)
		{
			return Status;
		}
	}
	
	//�ѳ��ִ���ҳ֮ǰ������д���µĿ��Ӧҳ��
	for (i = 0;i < endPage;i++)
	{
#if USE_DMA
		if(NandInfo.PageSize > 512)
              	Status = NandPageReadDMA(scrBlockNo, i, buffer, ECC_SUPPORT);
		else
			Status = NandPageReadDMAFor512(scrBlockNo, i, buffer, ECC_SUPPORT);
#else
		if(NandInfo.PageSize > 512)
              	Status = NandPageRead(scrBlockNo, i, buffer, ECC_SUPPORT);
		else
			Status = NandPageReadFor512(scrBlockNo, i, buffer, ECC_SUPPORT);
#endif
		if(NAND_SUCCE == Status)
		{
#if USE_DMA
			if(NandInfo.PageSize > 512)
                     	Status = NandPageWriteDMA(desBlockNo, i, buffer, ECC_SUPPORT);
			else
				Status = NandPageWriteDMAFor512(desBlockNo, i, buffer, ECC_SUPPORT);
#else
			if(NandInfo.PageSize > 512)
                     	Status = NandPageWrite(desBlockNo, i, buffer, ECC_SUPPORT);
			else
				Status = NandPageWriteFor512(desBlockNo, i, buffer, ECC_SUPPORT);
#endif			
			if(NAND_SUCCE != Status)
			{
				return Status;
			}
		}
		else
		{
			//ECC error or Read data error
			return Status;
		}
	}
	
	return NAND_SUCCE;
}

int NFC_LogInit(unsigned char *Buff)
{
        int Status;

        Status = NFC_IsFormated(Buff);
//        printk("NFC_IsFormated Status=%d\n",Status);
  
        if(NAND_FAIL==Status)       //û�и�ʽ��
        {
                printk("No Format!!!\n");
                return NAND_FAIL;
        }
        else
        {
                Status = NFC_IsFsIoctl(Buff);
                printk("NFC_IsFsIoctl Status=%d\n",Status);

                if(NAND_FAIL==Status)       //û��ͨ���ļ�ϵͳ��ʽ��
                {
                        printk("No Fs Format!!!\n");
                        return 2;
                }
                else
                {
                        NFC_ReadSysParaAndBBT(Buff);
                        //ǰ���Ѿ��ж�SysInfor������������Բ����ж�
                }
        }

        return NAND_SUCCE;
}

int NFC_LogicBlockErase(unsigned short BlkNo)
{
	int i;
	unsigned short TempBlkNo;

	TempBlkNo = BlkNo;
	
	if (TempBlkNo < NandInfo.BlkNum - BadBlkNum)
	{
		//check bad block table
		if (NAND_FAIL==NFC_CheckIsBadBlock(TempBlkNo))
		{
			for (i = 0; i < tableTopPos;i++)
			{
				if (TempBlkNo == __BadBlockTable[i].mBadBlock)
				{
					TempBlkNo = __BadBlockTable[i].mReplaceBlock;					
					break;
				}
			}
		}
	}
	
	if (NAND_FAIL == NandBlkErase(TempBlkNo))
	{		
		while(1)
		{
			if (NAND_FAIL == NFC_MarkAsBadBlock(BlkNo))
			{
				return NAND_FAIL;
			}
			
			if (NAND_SUCCE==(NandBlkErase(__BadBlockTable[tableTopPos - 1].mReplaceBlock)))
			{
				break;
			}
		}
	}

	return NAND_SUCCE;
}

int NFC_LogicBlockRead(unsigned short curSelBlock)
{
 	int i;//,j; removed waring
       unsigned int Status;
	unsigned int blockNo = curSelBlock;

	//block0 is ok
	if (blockNo && (blockNo < NandInfo.BlkNum - BadBlkNum))
	{
		//check bad block table
		if (NAND_FAIL==NFC_CheckIsBadBlock(blockNo))
		{
			//find out which block is bad,and replace the bad block
			for (i = 0; i < tableTopPos;i++)
			{
				if (blockNo == __BadBlockTable[i].mBadBlock)
				{
					blockNo = __BadBlockTable[i].mReplaceBlock;					
					break;
				}
			}
		}
	}
//printk("\nCome From Log Block=%d Read",blockNo);
 
	#if NAND_ECC
               NandSpareRead(blockNo, 0, SpareBuff);    //��ҳ����ǰ�ȶ���������           
               Status = CheckSpare(SpareBuff);  //ͨ���������жϵ�ǰҳ�Ǳ�д�����ǿ�ҳ
               if(Status==NAND_EMPTYPAGE)
               {
                        for(i = 0;i < NandInfo.PagePerBlk;i++)
                            for(j=0;j<NandInfo.PageSize;j++)
                                g_BlockBuffer[i][j] = 0xFF;
                        printk("NAND_EMPTYPAGE\n");
                        return 0;
                }
                else
                {
            	          for(i = 0;i < NandInfo.PagePerBlk;i++)
                	   {
                                delay(100);
#if USE_DMA
				    if(NandInfo.PageSize > 512)
                                	Status = NandPageReadDMA(blockNo, i, g_BlockBuffer[i], ECC_SUPPORT);
				    else
				    	Status = NandPageReadDMAFor512(blockNo, i, g_BlockBuffer[i], ECC_SUPPORT);
#else
				    if(NandInfo.PageSize > 512)
                                	Status = NandPageRead(blockNo, i, g_BlockBuffer[i], ECC_SUPPORT);
				    else
				    	Status = NandPageReadFor512(blockNo, i, g_BlockBuffer[i], ECC_SUPPORT);
#endif					
                		    if (NAND_SUCCE != Status)
                        	    {
                        			printk("NandPage[%d]Read Fail\n",i);
                                          //return Status;
                        	    }                         
                	    }
                }
        #else
                for(i = 0;i < NandInfo.PagePerBlk;i++)
                {
#if USE_DMA
			    if(NandInfo.PageSize > 512)
                            	Status = NandPageReadDMA(blockNo, i, g_BlockBuffer[i], ECC_SUPPORT);
			    else
			    	Status = NandPageReadDMAFor512(blockNo, i, g_BlockBuffer[i], ECC_SUPPORT);
#else
			    if(NandInfo.PageSize > 512)
                            	Status = NandPageRead(blockNo, i, g_BlockBuffer[i], ECC_SUPPORT);
			    else
			    	Status = NandPageReadFor512(blockNo, i, g_BlockBuffer[i], ECC_SUPPORT);
#endif	
    		          if (NAND_SUCCE != Status)
    		          {
    			            printk("NandPage[%d]Read Fail\n",i);
                                 //return Status;
    		          }
                }
        #endif
	
	 return NAND_SUCCE;
}


int NFC_LogicBlockWrite(unsigned short curSelBlock)
{

	int i;
	unsigned int tempBlockNo;
	int res;

	//block0 һ��Ҫд�������м��
	if (curSelBlock < START_BLK)
	{
		return -1;
	}
	
	tempBlockNo = curSelBlock;

	if (tempBlockNo < NandInfo.BlkNum - BadBlkNum)
	{
		//check bad block table
		if (NAND_FAIL==NFC_CheckIsBadBlock(tempBlockNo))
		{
			for (i = 0; i < tableTopPos;i++)
			{
				if (tempBlockNo == __BadBlockTable[i].mBadBlock)
				{
					tempBlockNo = __BadBlockTable[i].mReplaceBlock;
					
					break;
				}
			}
		}
	}

REPLACE_START:
       for(i=0;i<NandInfo.PagePerBlk;i++)
       {
       
#if USE_DMA
       	  if(NandInfo.PageSize > 512)
                	res = NandPageWriteDMA(tempBlockNo, i, g_BlockBuffer[i], ECC_SUPPORT);
		  else
		  	res = NandPageWriteDMAFor512(tempBlockNo, i, g_BlockBuffer[i], ECC_SUPPORT);
#else
       	  if(NandInfo.PageSize > 512)
                	res = NandPageWrite(tempBlockNo, i, g_BlockBuffer[i], ECC_SUPPORT);
		  else
		  	res = NandPageWriteFor512(tempBlockNo, i, g_BlockBuffer[i], ECC_SUPPORT);
#endif
                if(NAND_FAIL==res)
                {
                        if (NAND_FAIL == NFC_MarkAsBadBlock(curSelBlock))
			   {
				    return NAND_FAIL;
			    }
                         tempBlockNo = __BadBlockTable[tableTopPos - 1].mReplaceBlock;
                         goto REPLACE_START;
                }
       }
	return NAND_SUCCE;
}

int NFC_LogicPageRead(unsigned short BlkNo, unsigned short PageNo, unsigned char *buffer)
{
	int i;

	//block0 is ok
	if (BlkNo && (BlkNo < NandInfo.BlkNum - BadBlkNum))
	{
		//check bad block table
		if (NAND_FAIL==NFC_CheckIsBadBlock(BlkNo))
		{
			//find out which block is bad,and replace the bad block
			for (i = 0; i < tableTopPos;i++)
			{
				if (BlkNo == __BadBlockTable[i].mBadBlock)
				{
					BlkNo = __BadBlockTable[i].mReplaceBlock;					
					break;
				}
			}
		}
	}

#if USE_DMA
	if(NandInfo.PageSize > 512)
		return NandPageReadDMA(BlkNo, PageNo, buffer, ECC_SUPPORT);
	else
		return NandPageReadDMAFor512(BlkNo, PageNo, buffer, ECC_SUPPORT);
#else
	if(NandInfo.PageSize > 512)
		return NandPageRead(BlkNo, PageNo, buffer, ECC_SUPPORT);
	else
		return NandPageReadFor512(BlkNo, PageNo, buffer, ECC_SUPPORT);
#endif
}

int NFC_LogicPageWrite(unsigned short BlkNo, unsigned short PageNo, unsigned char *buffer)
{
	int i;

	if (BlkNo < START_BLK)
	{
		return -1;
	}
	
	//block0 is ok
	if (BlkNo && (BlkNo < NandInfo.BlkNum - BadBlkNum))
	{
		//check bad block table
		if (NAND_FAIL==NFC_CheckIsBadBlock(BlkNo))
		{
			//find out which block is bad,and replace the bad block
			for (i = 0; i < tableTopPos;i++)
			{
				if (BlkNo == __BadBlockTable[i].mBadBlock)
				{
					BlkNo = __BadBlockTable[i].mReplaceBlock;					
					break;
				}
			}
		}
	}
#if USE_DMA
	if(NandInfo.PageSize > 512)
		return NandPageWriteDMA(BlkNo, PageNo, buffer, ECC_SUPPORT);
	else
		return NandPageWriteDMAFor512(BlkNo, PageNo, buffer, ECC_SUPPORT);
#else
	if(NandInfo.PageSize > 512)
		return NandPageWrite(BlkNo, PageNo, buffer, ECC_SUPPORT);
	else
		return NandPageWriteFor512(BlkNo, PageNo, buffer, ECC_SUPPORT);
#endif
}
