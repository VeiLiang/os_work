//////////////////////////////////////////////////////////////////////////////////////////
#ifndef _NAND_LOGIC_H_
#define _NAND_LOGIC_H_

#ifdef __cplusplus
extern "C" {
#endif

#define START_BLK       11// removed waring and change 0-->11

#define	NF_MAX_BLOCKNUM 	0x8000	//max support 32768 block 
#define	NF_MAX_BADBLOCK		0x400	//max support 1024 bad block


typedef struct
{
	unsigned int bitIsBadBlock;		//ÿһ��bit�����һ�����״̬����32���飬
						//��Ϊһ����������; 1 for bad block; 0 for good
	unsigned int mPosInBBT;		//����л��飬���ʾ���������ڻ�����е�ƫ��
}BBTINDEX;

typedef struct
{
	unsigned short mBadBlock;		//�����
	unsigned short mReplaceBlock;	//�滻���
}BBTENTRY;

extern unsigned short BadBlkNum;
extern unsigned int bakupTopPos;


int NFC_LogInit(unsigned char *Buff);
int NFC_SaveSysParaAndBBT(unsigned char *buffer);
int NFC_FormatFlash(void);

int NFC_LogicBlockErase(unsigned short BlkNo);
int NFC_LogicBlockRead(unsigned short curSelBlock);
int NFC_LogicBlockWrite(unsigned short curSelBlock);
int NFC_LogicPageRead(unsigned short BlkNo, unsigned short PageNo, unsigned char *buffer);

#ifdef __cplusplus
}
#endif

#endif
