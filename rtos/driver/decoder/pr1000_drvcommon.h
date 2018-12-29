#ifndef __PR1000_DRVCOMMON_H__
#define __PR1000_DRVCOMMON_H__

#ifdef __LINUX_SYSTEM__

/* __KERNEL__ */
#ifdef __KERNEL__
#else
#include <stdint.h>
#endif /* __KERNEL__ */

#define MAX_CHN		(16)
#else //#ifdef __LINUX_SYSTEM__

#include <stdint.h>
#include "pr1000_user_config.h"
#define MAX_CHN		(4)

#endif // __LINUX_SYSTEM__
/************************************************************/
/*                   App & Driver common 		    */
/************************************************************/
enum {
	NTSC = 0,
	PAL,
	MAX_VIDEO_SYSTEM_TYPE
};

/***************************************************************************************************************/
/* error module */
typedef enum {
	PR_ERR_COMMON = 0x01,
	PR_ERR_VDEC = 0x02,
	PR_ERR_PTZ = 0x03,
	PR_ERR_EVENT = 0x04,

	MAX_PR_ERR_MODULE
}PR_ERR_MODULE_E;
/* error code */
typedef enum {
	/* common */
	PR_ERR_HANDLE = 0x001,
	PR_ERR_INVALID_DRVFD = 0x002,
	PR_ERR_ILLEGAL_PARAM = 0x003,
	PR_ERR_EXIST = 0x004,
	PR_ERR_UNEXIST = 0x005,
	PR_ERR_NULL_PTR = 0x006,
	PR_ERR_NOT_PERM = 0x007,
	PR_ERR_NOT_SUPPORT = 0x008,
	PR_ERR_NOT_CONFIG = 0x009,
	PR_ERR_NO_MEM = 0x00A,
	PR_ERR_NO_BUF = 0x00B,
	PR_ERR_FULL_BUF = 0x00C,
	PR_ERR_EMPTY_BUF = 0x00D,
	PR_ERR_NOT_READY = 0x00E,
	PR_ERR_BUSY = 0x00F,
	PR_ERR_INVALID_DEVID = 0x010,
	PR_ERR_INVALID_CHNID = 0x011,
	PR_ERR_LOCK = 0x012,
	PR_ERR_UNLOCK = 0x013,
	PR_ERR_PUT_QUEUE = 0x014,
	PR_ERR_GET_QUEUE = 0x015,

	MAX_PR_ERR_CODE
}PR_ERR_CODE_E;

static const char strDevDrvName[10] = {"pr1000"};

/* error code */
#define PR_ERROR_CODE_TYPE_APP	(0x0)
#define PR_ERROR_CODE_TYPE_LIB	(0x1)
#define PR_ERROR_CODE_TYPE_DRV	(0x2)

#define PR_ERROR_CODE(type, module, code)	(0xF000F000|((type&0xF)<<24)|((module&0xFF)<<16)|((code)&0xFFF))
#define PR_ERROR_CODE_APP(module, code)		PR_ERROR_CODE(PR_ERROR_CODE_TYPE_APP, module, code)
#define PR_ERROR_CODE_LIB(module, code)		PR_ERROR_CODE(PR_ERROR_CODE_TYPE_LIB, module, code)
#define PR_ERROR_CODE_DRV(module, code)		PR_ERROR_CODE(PR_ERROR_CODE_TYPE_DRV, module, code)
/***************************************************************************************************************/

/********************************************************************/
/*				app & drv common struct								*/
/********************************************************************/
typedef struct
{
	uint8_t slvAddr;
	uint8_t page;
	uint8_t reg;
	uint8_t data;
}_stPrReg;

typedef struct
{
	int mapChn;

	uint8_t type;
	uint8_t resol; //enum _pr1000_table_outresol
}_stPrRxMode;

typedef struct
{
	uint8_t prChip;

	/* digital core clock power down */
	uint8_t bVDEC_CLK_PD0; //prchn 0, page0 0xEC b'3
	uint8_t bVDEC_CLK_PD1; //prchn 1, page0 0xEC b'7
	uint8_t bVDEC_CLK_PD2; //prchn 2, page0 0xED b'3
	uint8_t bVDEC_CLK_PD3; //prchn 3, page0 0xED b'7
	uint8_t bAUDIO_ALINK_PD; //0xEA b'7
	/* analog video ADC power down */
	uint8_t bVADC_PD0; //prchn 0, page1 0x68 b'4
	uint8_t bVADC_PD1; //prchn 1, page1 0xE8 b'4
	uint8_t bVADC_PD2; //prchn 2, page2 0x68 b'4
	uint8_t bVADC_PD3; //prchn 3, page2 0xE8 b'4
	/* analog audio ADC power down */
	uint8_t AADC_PD; //page0 0xEB b'[7:5]
	/* analog pll power down */
	uint8_t bPLL1_PD; //page0 0xE6 b'7 All digital/anglog
	uint8_t bPLL2_PD; //page0 0xE7 b'7 Only video output
}_stPrPwDown;

enum _PR1000_DET_IFMT_RES {
	PR1000_DET_IFMT_RES_480i = 0,
	PR1000_DET_IFMT_RES_576i,
	PR1000_DET_IFMT_RES_720p,
	PR1000_DET_IFMT_RES_1080p,
	MAX_PR1000_DET_IFMT_RES
};

static const char _STR_PR1000_IFMT_RES[MAX_PR1000_DET_IFMT_RES][40] = {
	"PR1000_IFMT_RES_480i",
	"PR1000_IFMT_RES_576i",
	"PR1000_IFMT_RES_720p",
	"PR1000_IFMT_RES_1080p",
};

enum _PR1000_DET_IFMT_REF {
	PR1000_DET_IFMT_REF_25 = 0,
	PR1000_DET_IFMT_REF_30,
	PR1000_DET_IFMT_REF_50,
	PR1000_DET_IFMT_REF_60,
	MAX_PR1000_DET_IFMT_REF
};

static const char _STR_PR1000_IFMT_REF[MAX_PR1000_DET_IFMT_REF][40] = {
	"PR1000_IFMT_REF_25",
	"PR1000_IFMT_REF_30",
	"PR1000_IFMT_REF_50",
	"PR1000_IFMT_REF_60",
};

enum _PR1000_DET_IFMT_STD {
	PR1000_DET_IFMT_STD_PVI = 0,
	PR1000_DET_IFMT_STD_CVI,
	PR1000_DET_IFMT_STD_HDA,
	PR1000_DET_IFMT_STD_HDT,
	MAX_PR1000_DET_IFMT_STD
};

#ifndef DONT_SUPPORT_HELP_STRING
static const char _STR_PR1000_IFMT_STD[MAX_PR1000_DET_IFMT_STD][40] = {
	"PR1000_IFMT_STD_PVI",
	"PR1000_IFMT_STD_CVI",
	"PR1000_IFMT_STD_HDA",
	"PR1000_IFMT_STD_HDT",
};
#endif // DONT_SUPPORT_HELP_STRING

typedef struct
{
	uint8_t prChn;

	uint8_t std; 
	uint8_t ref; 
	uint8_t res; 
}_stIfmtAttr;

typedef struct
{
	int mapChn;

	uint16_t u16HActive; //b[12:0]
	uint16_t u16HDelay; //b[12:0]
	uint16_t u16VActive; //b[10:0]
	uint16_t u16VDelay; //b[10:0]
	uint16_t u16HSCLRatio; //b[15:0] 0:skip write
}_stChnAttr;

typedef struct
{
	int mapChn;

	uint8_t u8CbGain;
	uint8_t u8CrGain;
	uint8_t u8CbOffset;
	uint8_t u8CrOffset;
}_stCscAttr;

typedef struct
{
	int mapChn;

	uint8_t u8Contrast;
}_stContrast;

typedef struct
{
	int mapChn;

	uint8_t u8Bright;
}_stBright;

typedef struct
{
	int mapChn;

	uint8_t u8Saturation;
}_stSaturation;

typedef struct
{
	int mapChn;

	uint8_t u8Hue;
}_stHue;

typedef struct
{
	int mapChn;

	uint8_t u8Sharpness; //b[3:0]
}_stSharpness;


enum _pr1000_vid_outformat_mux_type {
	PR1000_VID_OUTF_MUX_1CH = 0,
	PR1000_VID_OUTF_MUX_2CH,
	PR1000_VID_OUTF_MUX_4CH
};
enum _pr1000_vid_outformat_resol_type {
        PR1000_VID_OUTF_RESOL_HD1080p = 0, //1080p30,1080p25,720p60,720p50
        PR1000_VID_OUTF_RESOL_HD720p = 1,  //720p30,720p25
        PR1000_VID_OUTF_RESOL_SD960H = 2,  //960i60, 960i50
        PR1000_VID_OUTF_RESOL_SD720H = 3  //480i60, 480i50
};

typedef struct
{
	uint8_t chipCnt;
	enum _pr1000_vid_outformat_mux_type muxChCnt;	//1ch multiplex, 2ch multiplex, 4ch multiplex
	uint8_t b16bit;	//16bit, 8bit
	uint8_t datarate;	//0:normal
	enum _pr1000_vid_outformat_resol_type resol;
	uint8_t outfmt_bt656;

	int portChClkPhase[4][4]; // get from pr1000_user_config.c
}_stOutFormatAttr;

typedef struct
{
	uint8_t prChip;
	uint8_t prChn;	//0:normal
	int portChSel[4]; 
}_stOutChn;

enum _pr1000_vid_outrate_mode {
	PR1000_VID_OUTRATE_BYPASS = 0,
	PR1000_VID_OUTRATE_PROGRESSIVE = 1,
	PR1000_VID_OUTRATE_INT_ODD = 2,
	PR1000_VID_OUTRATE_INT_EVEN = 3
};

typedef struct
{
	uint8_t prChip;

	uint8_t bEnable;
}_stAudEnable;

typedef struct
{
	uint8_t prChip;
	uint8_t prChn;

	uint8_t u8Gain; 	//4bit gain
}_stAUDAiGain;
typedef struct
{
	uint8_t prChip;
	uint8_t u8Gain; 	//4bit gain
}_stAUDDacGain;
typedef struct
{
	uint8_t prChip;
	uint8_t bMixMode; 	//1bit gain(0:sum, 1:avr)
}_stAUDMixMode;
typedef struct
{
	uint8_t prChip;

	uint8_t bMaster; 	//0:slave, 1:master
	uint8_t bDSP;		//0:I2S, 1:DSP
	uint8_t bDSP_User;	//0:DSP_PCM, 1:DSP_User
	uint8_t bClkRise;	//0:Clock Falling, 1:Clock Rising
	uint8_t format;		//0:2's complement, 1:straight, 2:G711u, 3:G711a
	uint8_t sampleRate;	//0:8Khz, 1:16Khz, 2:32Khz, 3:48Khz

	uint8_t b8bits;		//0:16bits, 1:8bits
	uint8_t bitRate;	//0:16fs, 1:32fs, 2:64fs, 3:128fs, 4:256fs, 5:320fs, 6:384fs
}_stAUDRecAttr;
typedef struct
{
	uint8_t prChip;

	uint8_t b8bits;		//0:16bits, 1:8bits
	uint8_t bMaster; 	//0:slave, 1:master
	uint8_t bRightChn; 	//0:left channel, 1:right channel
	uint8_t format;		//0:straight, 1:2's complement, 2:G711u, 3:G711a

	uint8_t b8bitLow;	//0:When 8bit mode, 0:upper 8bit, 1:lower 8bit
	uint8_t bDSP;		//0:I2S, 1:DSP
	uint8_t bDSP_User;	//0:DSP_PCM, 1:DSP_User
	uint8_t bitRate;	//0:16fs, 1:32fs, 2:64fs, 3:128fs, 4:256fs, 5:320fs, 6:384fs
	uint8_t sampleRate;	//0:8Khz, 1:16Khz, 2:32Khz, 3:48Khz
}_stAUDPbAttr;

typedef struct
{
	uint8_t prChip;
	uint8_t prChn;
	uint8_t bEnable;
}_stAudRecMute;

typedef struct
{
	uint8_t prChip;
	uint8_t bEnable;
}_stAudMixMute;

typedef struct
{
	uint8_t prChip;
	uint8_t bEnable;
}_stAudVocMute;

typedef struct
{
	uint8_t prChip;
	uint8_t bEnable;
}_stAudDacMute;

typedef struct
{
	uint8_t prChip;
	uint8_t prChn;
}_stAudDacChn;

typedef struct
{
	uint8_t prChip;

	uint8_t absThresholdCh0;
	uint8_t absThresholdCh1;
	uint8_t absThresholdCh2;
	uint8_t absThresholdCh3;
	uint8_t diffThresholdCh0;
	uint8_t diffThresholdCh1;
	uint8_t diffThresholdCh2;
	uint8_t diffThresholdCh3;
	uint8_t noAudThresholdCh0;
	uint8_t noAudThresholdCh1;
	uint8_t noAudThresholdCh2;
	uint8_t noAudThresholdCh3;
	uint8_t noAudMax; 	//on->noAud detect time. increase->fast
	uint8_t noAudMin;	//noAud->on detect time. decrease->fast
}_stAUDDetAttr;

typedef struct
{
	uint8_t bCascadeMaster;	//cascade 1:master(or no cascade), 0:slave
}_stAUDCascadeAttr;

typedef struct
{
	int mapChn;

	uint8_t tx_field_type; //reg 4x20 b[1:0] : select the PTZ Tx field.
	uint8_t tx_line_cnt; //reg 4x21 b[7:3] : select the PTZ Tx line size per frame.
	uint8_t tx_hst_os; //reg 4x21 b[2:0] : select the PTZ Tx line starting offset for even fld.
	uint8_t tx_hst; //reg 4x22 b[6:0] : PTZ Tx valid line starting position.
	uint32_t tx_freq_first; //reg 4x23,4x24,4x25 :  PTZ Bit-width for 1st bit.
	uint32_t tx_freq; //reg 4x26,4x27,4x28 : PTZ Bit-width for all bits except 1st bit.
	uint16_t tx_hpst; //reg 4x29,4x2A b[12:0] : PTZ Tx starting location.
	uint8_t tx_line_len; //reg 4x2B b[5:0] : Bit length per line for PTZ data.
	uint8_t tx_all_data_len; //reg 4x2C b[7:0] : All bytes length per command of PTZ data.
}_stPTZTxParam;

typedef struct
{
	int mapChn;

	uint8_t rx_field_type; //reg 4x00 b[1:0] : select the PTZ Rx Field.
	uint8_t rx_line_cnt; //reg 4x01 b[7:3] : select the PTZ Rx line size per frame.
	uint8_t rx_hst_os; //reg 4x01 b[2:0] : select the PTZ Rx line starting offset for Even Fld.
	uint8_t rx_hst; //reg 4x02 b[6:0] : PTZ Rx valid line starting position.
	uint32_t rx_freq_first; //reg 4x03,4x04,4x05 : PTZ Bit-width for 1st bit.
	uint32_t rx_freq; //reg 4x06,4x07,4x08 : PTZ Bit-width for all bits except 1st bit.
	uint8_t rx_lpf_len; //reg 4x09 b[5:0] : select the Rx LPF taps.
	uint8_t rx_h_pix_offset; //reg 4x0A b[7:0] : Rx H starting offset for PTZ start bit.
	uint8_t rx_line_len; //reg 4x0B b[5:0] : Bit length per line for PTZ data.
	uint8_t rx_valid_cnt; //reg 4x0C b[7:0] : All bytes length per command of PTZ data.
}_stPTZRxParam;

typedef struct
{
	int mapChn;

	char *pPatFormat;
	int sizePatFormat;
	char *pPatData;
	int sizePatData;
}_stPTZTxPatInfo;

typedef struct
{
	int mapChn;

	char *pPatFormat;
	int sizePatFormat;
	char *pPatStartFormat;
	int sizePatStartFormat;
	char *pPatStartData;
	int sizePatStartData;
}_stPTZRxPatInfo;

typedef struct
{
	int mapChn;
	uint8_t bStart;
}_stPtzStart;

#define MAX_PTZ_CMD_LENGTH	(32)
typedef struct
{
	int mapChn;
	uint8_t camType;
	uint8_t camResol;
	uint8_t ptzCmd[MAX_PTZ_CMD_LENGTH];
	uint16_t ptzCmdLength;
}_stPtzSend;

typedef struct
{
	int mapChn;
	uint8_t size;
	uint8_t *pPtzRxBuf;
}_stPtzRecv;

enum _pr1000_vevent_mem_type {
	PR1000_VMD_DET = 0,
	PR1000_VMASK_MD = 96,
	PR1000_VMASK_BD = 120,
	PR1000_VMASK_PZ = 144,
	MAX_PR1000_VMASK_TYPE = 168
};

enum _pr1000_vevent_disp_format {
	PR1000_VEVENT_DISP_PZ = 0,
	PR1000_VEVENT_DISP_BD = 1,
	PR1000_VEVENT_DISP_MD = 2,
	PR1000_VEVENT_DISP_NONE
};
enum _pr1000_vevent_mask_color {
	PR1000_VEVENT_MASKCOLOR_BLUE = 0,
	PR1000_VEVENT_MASKCOLOR_CYAN,
	PR1000_VEVENT_MASKCOLOR_PURPLE,
	PR1000_VEVENT_MASKCOLOR_RED,
	PR1000_VEVENT_MASKCOLOR_MAGENTA,
	PR1000_VEVENT_MASKCOLOR_BLACK,
	PR1000_VEVENT_MASKCOLOR_GRAY,
	PR1000_VEVENT_MASKCOLOR_WHITE,
	MAX_PR1000_VEVENT_MASKCOLOR
};
#ifndef DONT_SUPPORT_EVENT_FUNC

#define PR1000_MAX_DET_CELL_X_NUM	(64)
#define PR1000_MAX_DET_CELL_Y_NUM	(48)
#define PR1000_MAX_MASK_CELL_X_NUM	(32)
#define PR1000_MAX_MASK_CELL_Y_NUM	(24)
typedef struct 
{
	int mapChn;

	enum _pr1000_vevent_disp_format cellFormat;
	uint8_t bEnMaskPln;	//display mask cell of mask pln buffer. Mask cell is don't detect cell.
	uint8_t bEnDetPln;	//display detect cell on mask pln cell. 0:display, 1:don't display.
	uint8_t maskBlendLevel;
	uint8_t detBndrLevel;	//0:100%white, 3:black
	uint8_t detBndrWidth; //0:disable detect cell boundray display.
	uint8_t maskBndrLevel;	//0:100%white, 3:black
	uint8_t maskBndrWidth; //0:disable mask cell boundray display.
	enum _pr1000_vevent_mask_color maskColor;
}_stVEVENTDisplayAttr;

typedef struct 
{
	int mapChn;

	enum _pr1000_vevent_mem_type maskType;
	uint32_t maskBuf[PR1000_MAX_MASK_CELL_Y_NUM];
}_stVEVENTMaskAttr;

typedef struct 
{
	int mapChn;

	uint8_t resol;	//enum _pr1000_table_outresol
	uint8_t cellCntX;
	uint8_t cellCntY;
	uint8_t cellWidth;
	uint8_t cellHeight;
	uint16_t cellVStartOffset;
	uint16_t cellHStartOffset;
}_stVEVENTMaskCellSize;

typedef struct 
{
	int mapChn;

	uint8_t startLine;
	uint8_t lineCnt;
        uint64_t detBuf[PR1000_MAX_DET_CELL_Y_NUM];
}_stVEVENTDetData;

typedef struct
{
	int mapChn;

	uint8_t bEn;
	uint8_t bMaskEn; 
}_stMdAttr;
typedef struct
{
	int mapChn;

	uint16_t lvsens;
}_stMdLvSens;
typedef struct
{
	int mapChn;

	uint16_t spsens;
}_stMdSpSens;
typedef struct
{
	int mapChn;

	uint8_t tmpsens;
}_stMdTmpSens;
typedef struct
{
	int mapChn;

	uint8_t velocity;
}_stMdVelocity;
typedef struct
{
	int mapChn;

	uint8_t bEn;
	uint8_t bMaskEn; //enable mask cell setting of mask buffer.
}_stBdAttr;
typedef struct
{
	int mapChn;

	uint16_t lvsens;
}_stBdLvSens;
typedef struct
{
	int mapChn;

	uint16_t spsens;
}_stBdSpSens;
typedef struct
{
	int mapChn;

	uint8_t tmpsens;
}_stBdTmpSens;
typedef struct
{
	int mapChn;

	uint8_t bEn;
}_stNdAttr;
typedef struct
{
	int mapChn;

	uint16_t lvsens_low;
	uint16_t lvsens_high;
}_stNdLvSens;
typedef struct
{
	int mapChn;

	uint8_t tmpsens;
}_stNdTmpSens;
typedef struct
{
	int mapChn;

	uint8_t bEn;
}_stDdAttr;
typedef struct
{
	int mapChn;

	uint16_t lvsens_low;
	uint16_t lvsens_high;
}_stDdLvSens;
typedef struct
{
	int mapChn;

	uint8_t tmpsens;
}_stDdTmpSens;
typedef struct
{
	int mapChn;

	uint16_t vstart;
	uint16_t hstart;
	uint16_t vsize;
	uint16_t hsize;
}_stDfdAttr;
typedef struct
{
	int mapChn;

	uint8_t lvsens;
}_stDfdLvSens;
typedef struct
{
	int mapChn;

	uint8_t spsens;
}_stDfdSpSens;
typedef struct
{
	int mapChn;

	uint8_t tmpsens;
}_stDfdTmpSens;
#endif // DONT_SUPPORT_EVENT_FUNC

typedef struct
{
	int mapChn;

	uint8_t blankColor; //0:black, 1:blue
}_stNovidAttr;

typedef struct
{
	int mapChn;
	uint8_t prChn;

	uint8_t format;
	uint8_t resol;
}_stEventDetStd;

typedef struct 
{
#ifdef __LINUX_SYSTEM__
	unsigned long bitWqPollDetStd; //chn0 : b'0
	unsigned long bitWqPollNovid; //chn0 : b'0
	unsigned long bitWqPollVfd; //chn0 : b'0
	unsigned long bitWqPollMd; //chn0 : b'0
	unsigned long bitWqPollBd; //chn0 : b'0
	unsigned long bitWqPollNd; //chn0 : b'0
	unsigned long bitWqPollDd; //chn0 : b'0
	unsigned long bitWqPollDfd; //chn0 : b'0
	unsigned long bitWqPollAdMute; //chn0 : b'0
	unsigned long bitWqPollAdAbs; //chn0 : b'0
	unsigned long bitWqPollAdDiff; //chn0 : b'0
	unsigned long bitWqPollGpio; //chn0 : b'0
	unsigned long bitWqPollPtzRx; //chn0 : b'0
#else //#ifdef __LINUX_SYSTEM__
	unsigned char bitWqPollDetStd; //chn0 : b'0
	unsigned char bitWqPollNovid; //chn0 : b'0
	unsigned char bitWqPollVfd; //chn0 : b'0
	unsigned char bitWqPollMd; //chn0 : b'0
	unsigned char bitWqPollBd; //chn0 : b'0
	unsigned char bitWqPollNd; //chn0 : b'0
	unsigned char bitWqPollDd; //chn0 : b'0
	unsigned char bitWqPollDfd; //chn0 : b'0
	unsigned char bitWqPollAdMute; //chn0 : b'0
	unsigned char bitWqPollAdAbs; //chn0 : b'0
	unsigned char bitWqPollAdDiff; //chn0 : b'0
	unsigned char bitWqPollGpio; //chn0 : b'0
	unsigned char bitWqPollPtzRx; //chn0 : b'0
#endif // __LINUX_SYSTEM__
}_wqPollChnStatus;

typedef struct
{
	int mapChn;

	uint16_t dcGain;
	uint16_t acGain;
	uint16_t comp1;
	uint16_t comp2;
	uint16_t atten1;
	uint16_t atten2;
}_stCEQInfo;

typedef struct
{
	int mapChn;

	int offset;
}_stCEQGainOffset;

typedef struct
{
	int mapChn;

	uint16_t eqGain;
}_stCEQGain;

#ifndef __LINUX_SYSTEM__
#define _IO(x,y)	(x<<8|y)
#endif // __LINUX_SYSTEM__
/********************************************************************/
/*		ioctl command define		*/
/********************************************************************/
/* Use 'P' as magic number */
#define PR_IOC_MAGIC  'P'

#define PR_IOC_RESET    			_IO(PR_IOC_MAGIC, (0x00))
#define PR_IOS_REGWRITE  			_IO(PR_IOC_MAGIC, (0x01))
#define PR_IOG_REGREAD  			_IO(PR_IOC_MAGIC, (0x02))
#define PR_IOS_RXMODE  				_IO(PR_IOC_MAGIC, (0x04))
#define PR_IOG_RXMODE  				_IO(PR_IOC_MAGIC, (0x05))
#define PR_IOS_PWDOWN 				_IO(PR_IOC_MAGIC, (0x06))
#define PR_IOG_PWDOWN 				_IO(PR_IOC_MAGIC, (0x07))

#define PR_IOS_VID_CHNATTR  			_IO(PR_IOC_MAGIC, (0x10))
#define PR_IOG_VID_CHNATTR  			_IO(PR_IOC_MAGIC, (0x11))
#define PR_IOS_VID_CSCATTR  			_IO(PR_IOC_MAGIC, (0x12))
#define PR_IOG_VID_CSCATTR  			_IO(PR_IOC_MAGIC, (0x13))
#define PR_IOS_VID_CONTRAST  			_IO(PR_IOC_MAGIC, (0x14))
#define PR_IOG_VID_CONTRAST  			_IO(PR_IOC_MAGIC, (0x15))
#define PR_IOS_VID_BRIGHT  			_IO(PR_IOC_MAGIC, (0x16))
#define PR_IOG_VID_BRIGHT  			_IO(PR_IOC_MAGIC, (0x17))
#define PR_IOS_VID_SATURATION  			_IO(PR_IOC_MAGIC, (0x18))
#define PR_IOG_VID_SATURATION  			_IO(PR_IOC_MAGIC, (0x19))
#define PR_IOS_VID_HUE  			_IO(PR_IOC_MAGIC, (0x1A))
#define PR_IOG_VID_HUE  			_IO(PR_IOC_MAGIC, (0x1B))
#define PR_IOS_VID_SHARPNESS  			_IO(PR_IOC_MAGIC, (0x1C))
#define PR_IOG_VID_SHARPNESS  			_IO(PR_IOC_MAGIC, (0x1D))
#define PR_IOG_VID_OUTFORMATATTR                _IO(PR_IOC_MAGIC, (0x1E))
#define PR_IOS_VID_OUTCHN 	 		_IO(PR_IOC_MAGIC, (0x1F))

#define PR_IOS_AUD_AIGAIN  			_IO(PR_IOC_MAGIC, (0x30))
#define PR_IOG_AUD_AIGAIN  			_IO(PR_IOC_MAGIC, (0x31))
#define PR_IOS_AUD_DACGAIN  			_IO(PR_IOC_MAGIC, (0x32))
#define PR_IOG_AUD_DACGAIN  			_IO(PR_IOC_MAGIC, (0x33))
#define PR_IOS_AUD_MIXMODE			_IO(PR_IOC_MAGIC, (0x34))
#define PR_IOG_AUD_MIXMODE  			_IO(PR_IOC_MAGIC, (0x35))
#define PR_IOS_AUD_RECATTR  			_IO(PR_IOC_MAGIC, (0x36))
#define PR_IOG_AUD_RECATTR  			_IO(PR_IOC_MAGIC, (0x37))
#define PR_IOS_AUD_PBATTR  			_IO(PR_IOC_MAGIC, (0x38))
#define PR_IOG_AUD_PBATTR  			_IO(PR_IOC_MAGIC, (0x39))
#define PR_IOS_AUD_RECMUTE  			_IO(PR_IOC_MAGIC, (0x3A))
#define PR_IOS_AUD_MIXMUTE  			_IO(PR_IOC_MAGIC, (0x3B))
#define PR_IOS_AUD_VOCMUTE  			_IO(PR_IOC_MAGIC, (0x3C))
#define PR_IOS_AUD_DACMUTE  			_IO(PR_IOC_MAGIC, (0x3D))
#define PR_IOS_AUD_DACCHN  			_IO(PR_IOC_MAGIC, (0x3E))
#define PR_IOS_AUD_DETATTR			_IO(PR_IOC_MAGIC, (0x3F))
#define PR_IOG_AUD_DETATTR			_IO(PR_IOC_MAGIC, (0x40))

#define PR_IOS_EVENT_MDATTR  			_IO(PR_IOC_MAGIC, (0x50))
#define PR_IOG_EVENT_MDATTR  			_IO(PR_IOC_MAGIC, (0x51))
#define PR_IOS_EVENT_MD_LVSENS  		_IO(PR_IOC_MAGIC, (0x52))
#define PR_IOG_EVENT_MD_LVSENS  		_IO(PR_IOC_MAGIC, (0x53))
#define PR_IOS_EVENT_MD_SPSENS  		_IO(PR_IOC_MAGIC, (0x54))
#define PR_IOG_EVENT_MD_SPSENS  		_IO(PR_IOC_MAGIC, (0x55))
#define PR_IOS_EVENT_MD_TMPSENS  		_IO(PR_IOC_MAGIC, (0x56))
#define PR_IOG_EVENT_MD_TMPSENS  		_IO(PR_IOC_MAGIC, (0x57))
#define PR_IOS_EVENT_MD_VELOCITY  		_IO(PR_IOC_MAGIC, (0x58))
#define PR_IOG_EVENT_MD_VELOCITY  		_IO(PR_IOC_MAGIC, (0x59))
#define PR_IOS_EVENT_BDATTR  			_IO(PR_IOC_MAGIC, (0x5A))
#define PR_IOG_EVENT_BDATTR  			_IO(PR_IOC_MAGIC, (0x5B))
#define PR_IOS_EVENT_BD_LVSENS  		_IO(PR_IOC_MAGIC, (0x5C))
#define PR_IOG_EVENT_BD_LVSENS  		_IO(PR_IOC_MAGIC, (0x5D))
#define PR_IOS_EVENT_BD_SPSENS  		_IO(PR_IOC_MAGIC, (0x5E))
#define PR_IOG_EVENT_BD_SPSENS  		_IO(PR_IOC_MAGIC, (0x5F))
#define PR_IOS_EVENT_BD_TMPSENS  		_IO(PR_IOC_MAGIC, (0x60))
#define PR_IOG_EVENT_BD_TMPSENS  		_IO(PR_IOC_MAGIC, (0x61))
#define PR_IOS_EVENT_NDATTR  			_IO(PR_IOC_MAGIC, (0x62))
#define PR_IOG_EVENT_NDATTR  			_IO(PR_IOC_MAGIC, (0x63))
#define PR_IOS_EVENT_ND_LVSENS  		_IO(PR_IOC_MAGIC, (0x64))
#define PR_IOG_EVENT_ND_LVSENS  		_IO(PR_IOC_MAGIC, (0x65))
#define PR_IOS_EVENT_ND_TMPSENS  		_IO(PR_IOC_MAGIC, (0x66))
#define PR_IOG_EVENT_ND_TMPSENS  		_IO(PR_IOC_MAGIC, (0x67))
#define PR_IOS_EVENT_DDATTR  			_IO(PR_IOC_MAGIC, (0x68))
#define PR_IOG_EVENT_DDATTR  			_IO(PR_IOC_MAGIC, (0x69))
#define PR_IOS_EVENT_DD_LVSENS  		_IO(PR_IOC_MAGIC, (0x6A))
#define PR_IOG_EVENT_DD_LVSENS  		_IO(PR_IOC_MAGIC, (0x6B))
#define PR_IOS_EVENT_DD_TMPSENS  		_IO(PR_IOC_MAGIC, (0x6C))
#define PR_IOG_EVENT_DD_TMPSENS  		_IO(PR_IOC_MAGIC, (0x6D))
#define PR_IOS_EVENT_DFDATTR  			_IO(PR_IOC_MAGIC, (0x6E))
#define PR_IOG_EVENT_DFDATTR  			_IO(PR_IOC_MAGIC, (0x6F))
#define PR_IOS_EVENT_DFD_LVSENS  		_IO(PR_IOC_MAGIC, (0x70))
#define PR_IOG_EVENT_DFD_LVSENS  		_IO(PR_IOC_MAGIC, (0x71))
#define PR_IOS_EVENT_DFD_SPSENS  		_IO(PR_IOC_MAGIC, (0x72))
#define PR_IOG_EVENT_DFD_SPSENS  		_IO(PR_IOC_MAGIC, (0x73))
#define PR_IOS_EVENT_DFD_TMPSENS  		_IO(PR_IOC_MAGIC, (0x74))
#define PR_IOG_EVENT_DFD_TMPSENS  		_IO(PR_IOC_MAGIC, (0x75))

#define PR_IOS_EVENT_MASKCELL_SIZE  		_IO(PR_IOC_MAGIC, (0x80))
#define PR_IOG_EVENT_MASKCELL_SIZE  		_IO(PR_IOC_MAGIC, (0x81))
#define PR_IOS_EVENT_MASK  			_IO(PR_IOC_MAGIC, (0x82))
#define PR_IOG_EVENT_MASK  			_IO(PR_IOC_MAGIC, (0x83))
#define PR_IOS_EVENT_DISPATTR  			_IO(PR_IOC_MAGIC, (0x84))
#define PR_IOG_EVENT_DISPATTR  			_IO(PR_IOC_MAGIC, (0x85))
#define PR_IOS_EVENT_CLEAR_MASK  		_IO(PR_IOC_MAGIC, (0x86))
#define PR_IOG_EVENT_DETDATA			_IO(PR_IOC_MAGIC, (0x87))

#define PR_IOG_QUERY_WQPOLL  			_IO(PR_IOC_MAGIC, (0x90))
#define PR_IOG_EVENT_DET_STD  			_IO(PR_IOC_MAGIC, (0x91))
#define PR_IOG_EVENT_NOVID_STATUS  		_IO(PR_IOC_MAGIC, (0x92))
#define PR_IOG_EVENT_VFD_STATUS			_IO(PR_IOC_MAGIC, (0x93))
#define PR_IOG_EVENT_MD_STATUS			_IO(PR_IOC_MAGIC, (0x94))
#define PR_IOG_EVENT_BD_STATUS			_IO(PR_IOC_MAGIC, (0x95))
#define PR_IOG_EVENT_ND_STATUS			_IO(PR_IOC_MAGIC, (0x96))
#define PR_IOG_EVENT_DD_STATUS			_IO(PR_IOC_MAGIC, (0x97))
#define PR_IOG_EVENT_DFD_STATUS			_IO(PR_IOC_MAGIC, (0x98))
#define PR_IOG_EVENT_ADMUTE_STATUS		_IO(PR_IOC_MAGIC, (0x99))
#define PR_IOG_EVENT_ADABS_STATUS		_IO(PR_IOC_MAGIC, (0x9A))
#define PR_IOG_EVENT_ADDIFF_STATUS		_IO(PR_IOC_MAGIC, (0x9B))

#define PR_IOS_PTZ_INIT				_IO(PR_IOC_MAGIC, (0xA0))
#define PR_IOS_PTZ_TXPARAM			_IO(PR_IOC_MAGIC, (0xA1))
#define PR_IOG_PTZ_TXPARAM			_IO(PR_IOC_MAGIC, (0xA2))
#define PR_IOS_PTZ_RXPARAM			_IO(PR_IOC_MAGIC, (0xA3))
#define PR_IOG_PTZ_RXPARAM			_IO(PR_IOC_MAGIC, (0xA4))
#define PR_IOS_PTZ_TX_PAT_INFO			_IO(PR_IOC_MAGIC, (0xA5))
#define PR_IOS_PTZ_RX_PAT_INFO			_IO(PR_IOC_MAGIC, (0xA6))
#define PR_IOS_PTZ_START_RX			_IO(PR_IOC_MAGIC, (0xA7))
#define PR_IOS_PTZ_START_TX			_IO(PR_IOC_MAGIC, (0xA8))
#define PR_IOS_PTZ_SEND_TXDATA			_IO(PR_IOC_MAGIC, (0xA9))
#define PR_IOG_PTZ_RECV_SIZE                    _IO(PR_IOC_MAGIC, (0xAA))
#define PR_IOG_PTZ_RECV_RXDATA                  _IO(PR_IOC_MAGIC, (0xAB))

#define PR_IOS_PTZ_HDT_CHGOLD_CMD		_IO(PR_IOC_MAGIC, (0xB0))

#define PR_IOG_DET_CEQINFO			_IO(PR_IOC_MAGIC, (0xC0))

#endif /* __PR1000_DRVCOMMON_H__ */
