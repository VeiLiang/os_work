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
	unsigned int bitIsBadBlock;		//每一个bit，标记一个块的状态，共32个块，
						//称为一个索引区域; 1 for bad block; 0 for good
	unsigned int mPosInBBT;		//如果有坏块，则表示索引区域在坏块表中的偏移
}BBTINDEX;

typedef struct
{
	unsigned short mBadBlock;		//坏块号
	unsigned short mReplaceBlock;	//替换块号
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
