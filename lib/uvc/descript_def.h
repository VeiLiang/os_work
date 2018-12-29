#ifndef _DESCRIPT_DEF_H
#define _DESCRIPT_DEF_H

#define TEST_MODE	0
#define JPEG_TEST	1

#define JPEG_FRAME_RED_SIZE	0x4d020000		//…Ë÷√JPEG FRAME BUFFER∂ÓÕ‚÷µ

int AddRec(void *pSrc, int nLength);
int init_descriptor(BOOL flag);
void frameIdx_sel(BYTE bFormatIdx);

#endif
