#ifndef __PR1000_H__
#define __PR1000_H__

///////////////////////////////////////////////////////////////////////////////////////////////
//Print "Print() & Error(), & Dbg()" to log memory not console. default define.
#define SUPPORT_LOGMEM_PRINT 	
//Enable print Dbg("string"). default undef.
#define SUPPORT_DbgPrint		
//Enable autodetect standard format. default define.
#define SUPPORT_AUTODETECT_STDFORMAT 
//Hide EQing display. default define.
#undef SUPPORT_HIDE_EQING_DISPLAY 
//Enable Cable EQ(Equalize). default define.
#define SUPPORT_CABLE_EQ

///////////////////////////////////////////////////////////////////////////////////////////////

#define HDPVI_FAIL  	(-1)
#define HDPVI_OK  	(0)

#undef TRUE
#define TRUE  		(1)
#undef FALSE
#define FALSE  		(0)
#undef ENABLE
#define ENABLE 		(1)
#undef DISABLE
#define DISABLE		(0)
#undef START
#define START  		(1)
#undef STOP
#define STOP  		(0)
#undef HIGH
#define HIGH  		(1)
#undef LOW
#define LOW  		(0)
#define WAIT_NEXT_NOVIDEO_TRUE		(1) //disconnect camera.
#define WAIT_NEXT_NOVIDEO_FALSE		(0) //connect camera.

#define COMMA ,

#ifdef __LINUX_SYSTEM__

#ifndef NULL
#define NULL  		(0)
#endif
#define _SET_BIT(bit, pData) (set_bit(bit, pData))
#define _CLEAR_BIT(bit, pData) (clear_bit(bit, pData))
#define _TEST_BIT(bit, pData) (test_bit(bit, pData))
#define WAKE_UP_INTERRUPTIBLE(pData) (wake_up_interruptible(pData))
#define DO_DIV(x,y) (do_div(x,y))
#define SPIN_LOCK_INIT(pData) spin_lock_init(pData)
#define SPIN_LOCK_IRQSAVE(pData, flag) spin_lock_irqsave(pData, flag)
#define SPIN_UNLOCK_IRQRESTORE(pData, flag) spin_unlock_irqrestore(pData, flag)
typedef spinlock_t SPINLOCK_T;
#define SUPPORT_PROC_SYSTEM //support proc system for linux debugging.
#define ZALLOC(pMem, flag) kzalloc(pMem, flag)
#define FREE(pMem) kfree(pMem)

#else //#ifdef __LINUX_SYSTEM__

typedef unsigned long long  uint64_t;
typedef	 unsigned char		 uint8_t;
//typedef	 unsigned int		 uint32_t;

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef NULL
#define NULL  		(0)
#endif
#define _SET_BIT(bit, pData) (*pData|=(1<<bit))
#define _CLEAR_BIT(bit, pData) (*pData&=~(1<<bit))
#define _TEST_BIT(bit, pData) ((*pData&(1<<bit))?1:0)
#define WAKE_UP_INTERRUPTIBLE(pData) (*pData=1)
#define DO_DIV(x,y) ((uint64_t)x/(uint64_t)y)
#define SPIN_LOCK_INIT(pData) (*pData=0)
#define SPIN_LOCK_IRQSAVE(pData, flag) {while(*pData==1);*pData=1;}
#define SPIN_UNLOCK_IRQRESTORE(pData, flag) (*pData=0)
typedef volatile int SPINLOCK_T;
#undef SUPPORT_PROC_SYSTEM //support proc system for linux debugging.

/* define value */
#define POLLIN          0x0001
#define POLLPRI         0x0002
#define POLLOUT         0x0004
#define POLLERR         0x0008
#define POLLHUP         0x0010
#define POLLNVAL        0x0020
/* The rest seem to be more-or-less nonstandard. Check them! */
#define POLLRDNORM      0x0040
#define POLLRDBAND      0x0080
#define POLLWRNORM      0x0100
#define POLLWRBAND      0x0200
#define POLLMSG         0x0400
#define POLLREMOVE      0x1000
#define POLLRDHUP       0x2000

#endif // __LINUX_SYSTEM__

////////////////////////////////////////////////////////////////////////
#include "pr1000_table.h"
#include "drv_cq.h"

///////////////////////////////////////////////////////////////////////////////////
#define PR1000_MASTER 		(0)
#define PR1000_SLAVE1 		(1)
#define PR1000_SLAVE2 		(2)
#define PR1000_SLAVE3 		(3)
#define MAX_PR1000_CHIP		(4)

#define PR1000_CHIPID		(0x1000)

#define ASSERT_VALID_CH(x)		((x>0)?1:0) 

#define	PR1000_THREAD_POLLING_PERIOD	(300) // should >= 300msec
#define MAX_CNT_CHROMALOCK		(10)
/////////////////////////////////////////////////////////////////////////////////
typedef struct
{
	uint8_t u8PTZ[DEF_PR1000_MAX_CHN];
	uint8_t u8NOVID;
	uint8_t u8MD;
	uint8_t u8ND;
	uint8_t u8DFD;
	uint8_t u8AD;
	uint8_t u8GPIO0;
	uint8_t u8GPIO1_5[5];
}_stIRQReg;

/*** Genlock detect format register value */
enum _PR1000_DET_HPERIOD {
	PR1000_DET_HPERIOD_720p60 = 0,
	PR1000_DET_HPERIOD_720p50,
	PR1000_DET_HPERIOD_1080p30,
	PR1000_DET_HPERIOD_1080p25,
	PR1000_DET_HPERIOD_720p30,
	PR1000_DET_HPERIOD_720p25,
	PR1000_DET_HPERIOD_SD,
	MAX_PR1000_DET_HPERIOD
};

static const char _STR_PR1000_DET_DET_HPERIOD[MAX_PR1000_DET_HPERIOD][40] = {
	"PR1000_DET_HPERIOD_720p60",
	"PR1000_DET_HPERIOD_720p50",
	"PR1000_DET_HPERIOD_1080p30",
	"PR1000_DET_HPERIOD_1080p25",
	"PR1000_DET_HPERIOD_720p30",
	"PR1000_DET_HPERIOD_720p25",
	"PR1000_DET_HPERIOD_SD",
};

typedef union 
{
	uint8_t reg;
	struct
	{    
		uint8_t det_ifmt_res:2;
		uint8_t det_ifmt_ref:2;
		uint8_t det_ifmt_std:2;
		uint8_t lock_slice:1;
		uint8_t det_video:1;
	}b;                  
}_stCeqDet;
typedef union 
{
	uint8_t reg;
	struct
	{    
		uint8_t det_signal:1;
		uint8_t det_chroma:1;
		uint8_t lock_chroma:1;
		uint8_t lock_hpll:1;
		uint8_t lock_hperiod:1;
		uint8_t lock_clamp:1;
		uint8_t lock_gain:1;
		uint8_t lock_std:1;
	}b;                  
}_stCeqLock;
typedef union 
{
	uint8_t reg;
	struct
	{    
		uint8_t man_ifmt_res:2;
		uint8_t man_ifmt_ref:2;
		uint8_t man_ifmt_std:2;
		uint8_t reserved:2;
	}b;
}_stManEQMan;

typedef union 
{
	uint8_t reg;
	struct
	{    
		uint8_t irqSlave3:1;
		uint8_t irqSlave2:1;
		uint8_t irqSlave1:1;
		uint8_t irqMaster:1;
		uint8_t reserved0:3;
		uint8_t irqTimer:1;
	}b;                  
}_stIrqChipState;

typedef union 
{
	uint8_t reg[14];
	struct
	{    
		uint8_t fieldType:2;
		uint8_t fieldPol:1;
		uint8_t ignoreFrmEn:1;
		uint8_t ignoreLineEn:2;
		uint8_t start:1;
		uint8_t pathEn:1;	//reg0
		uint8_t hstOs:3;
		uint8_t lineCnt:5; //reg1
		uint8_t hst:7;
		uint8_t dataPol:1; //reg2
		uint8_t freqFirst23:8; //reg3
		uint8_t freqFirst15:8; //reg4
		uint8_t freqFirst07:8; //reg5
		uint8_t freq23:8; //reg6
		uint8_t freq15:8; //reg7
		uint8_t freq07:8; //reg8
		uint8_t lpfLen:6;
		uint8_t reserved0:2; //reg9
		uint8_t pixOffset:8; //reg10
		uint8_t lineLen:6;
		uint8_t reserved1:2; //reg11
		uint8_t validCnt:8; //reg12
		uint8_t tpSel:3;
		uint8_t reserved2:3;
		uint8_t addrHoldEn:1;
		uint8_t testEn:1; //reg13
	}b;                  
}_stPTZRxAttr;

typedef union 
{
	uint8_t reg[8];
	struct
	{    
		uint8_t rxHstrtOs13:6;
		uint8_t reserved0:1;
		uint8_t rxHsyncPol:1; //reg0
		uint8_t rxHstrtOs07:8; //reg1
		uint8_t rxVstrtOs10:3;
		uint8_t reserved1:4;
		uint8_t rxVsyncPol:1; //reg2
		uint8_t rxVstrtOs07:8; //reg3
		uint8_t txHstrtOs13:6;
		uint8_t reserved2:1;
		uint8_t txHsyncPol:1; //reg4
		uint8_t txHstrtOs07:8; //reg5
		uint8_t txVstrtOs10:3;
		uint8_t reserved3:4;
		uint8_t txVsyncPol:1; //reg6
		uint8_t txVstrtOs07:8; //reg7
	}b;                  
}_stPTZHVStartAttr;

typedef union 
{
	uint8_t reg[15];
	struct
	{    
		uint8_t fieldType:2;
		uint8_t fieldPol:1;
		uint8_t reserved0:3;
		uint8_t start:1;
		uint8_t pathEn:1;	//reg0
		uint8_t hstOs:3;
		uint8_t lineCnt:5; //reg1
		uint8_t hst:7;
		uint8_t dataPol:1; //reg2
		uint8_t freqFirst23:8; //reg3
		uint8_t freqFirst15:8; //reg4
		uint8_t freqFirst07:8; //reg5
		uint8_t freq23:8; //reg6
		uint8_t freq15:8; //reg7
		uint8_t freq07:8; //reg8
		uint8_t hpst12:5;
		uint8_t reserved1:3; //reg9
		uint8_t hpst07:8; //reg10
		uint8_t lineLen:6;
		uint8_t reserved2:2; //reg11
		uint8_t allDataLen:8; //reg12
		uint8_t lastRptNum:7;
		uint8_t rptEn:1; //reg13
		uint8_t cmdGrpNum:4;
		uint8_t tpSel:3;
		uint8_t grpEn:1; //reg14
	}b;                  
}_stPTZTxAttr;

typedef struct
{
	int chn;	//mapping channel(0~15).
	int vdPort;	//vd out port num(VD1:0~VD4:3).
	uint8_t prChip;	//chip num(0~3).
	uint8_t prChn;	//channel of chip(0~3).
	uint8_t i2cSlvAddr;	//mapping channel.
}_stPortChSel;

///////////////////////////////////////////////////////////////////////////
typedef union 
{
	uint8_t reg[12];
	struct
	{    
		uint8_t det_eq_dcgain_h:8;  //0x04
		uint8_t det_eq_dcgain_l:8; 
		uint8_t det_eq_acgain_h:8;  //0x06
		uint8_t det_eq_acgain_l:8; 
		uint8_t det_eq_comp1_h:8;  //0x08
		uint8_t det_eq_comp1_l:8; 
		uint8_t det_eq_comp2_h:8;  //0x0a
		uint8_t det_eq_comp2_l:8; 
		uint8_t det_eq_atten1_h:8; //0x0c
		uint8_t det_eq_atten1_l:8;
		uint8_t det_eq_atten2_h:8; //0x0e
		uint8_t det_eq_atten2_l:8;
	}b;                  
}_stCeqInfoReg;

enum _pr1000_ceq_step {
	PR1000_CEQ_STEP_NONE = 0,
	PR1000_CEQ_STEP_EQSTART,
	PR1000_CEQ_STEP_STDCHECK,
	PR1000_CEQ_STEP_EQCHECK,
	PR1000_CEQ_STEP_STDFORMAT_CHECK, //Old_New select check
	PR1000_CEQ_STEP_SPECIALSTD_CHECK,
	PR1000_CEQ_STEP_WAIT_CHROMALOCK1,
	PR1000_CEQ_STEP_WAIT_CHROMALOCK2,
	PR1000_CEQ_STEP_EQSTDDONE,
	PR1000_CEQ_STEP_EQTIMEOUT,
	PR1000_CEQ_STEP_CHROMALOCK,

	PR1000_CEQ_STEP_ESTSTART,
	PR1000_CEQ_STEP_ESTCHGING,

	MAX_PR1000_CEQ_STEP
};

static const char _STR_PR1000_CEQ_STEP[MAX_PR1000_CEQ_STEP][20] = {
	"NONE",
	"EQSTART",
	"STDCHECK",
	"EQCHECK",
	"STDFORMAT_CHECK", //Old_New select check
	"SPECIALSTD_CHECK",
	"WAIT_CHROMALOCK1",
	"WAIT_CHROMALOCK2",
	"EQSTDDONE",
	"EQTIMEOUT",
	"CHROMALOCK",

	"ESTSTART",
	"ESTCHGING",
};

enum _pr1000_mon_step {
	PR1000_MON_STEP_NONE = 0,
	PR1000_MON_STEP_START,
	PR1000_MON_STEP_CHROMALOCK1,
	PR1000_MON_STEP_VADCGAINSEL,

	PR1000_MON_STEP_CHROMALOCK2,

	PR1000_MON_STEP_CHROMALOCK3,

	PR1000_MON_STEP_CHROMALOCK4,

	PR1000_MON_STEP_CHECKSTDHDA,
	PR1000_MON_STEP_CHROMALOCK5,
	PR1000_MON_STEP_CHROMALOCK6,

	PR1000_MON_STEP_COMPENSATE,
	MAX_PR1000_MON_STEP
};

static const char _STR_PR1000_MON_STEP[MAX_PR1000_MON_STEP][20] = {
	"NONE",
	"START",
	"CHROMALOCK1",
	"VADCGAINSEL",

	"CHROMALOCK2",

	"CHROMALOCK3",

	"CHROMALOCK4",

	"CHECKSTDHDA",
	"CHROMALOCK5",
	"CHROMALOCK6",

	"COMPENSATE",
};

typedef struct
{
	int C_LOCK_CNT;
	int AC_GAIN_ADJ;
	int EQ_CNT;
	int AC_GAIN_HOLD;
}_stEqProcFlag;
	
typedef struct
{
	uint8_t bEnable;
	uint8_t retryCnt;	// count retry.
	uint8_t stableEQCnt;	// check eq is stable?
	uint8_t estStep;	// eq step
	uint8_t flagStepComplete[MAX_PR1000_CEQ_STEP];
	uint8_t flagMonStepComplete[MAX_PR1000_MON_STEP];
	uint32_t estCheckTime;  /* EQ check mininum time >= PR1000_EQ_CHECK_PERIOD */
	uint8_t estStdComplete; // std complete flag.
	uint8_t bLock;		// lock complete flag.
	uint8_t bForceChgStd;	// forcely change standard format.
	_stManEQMan stManEQMan;
	enum _pr1000_table_format format;
	enum _pr1000_table_inresol camResol;
	_stCeqInfoReg infoRegs;
	uint16_t saved_det_eq_dcgain;
	_stEqProcFlag eqProcFlag;
	uint8_t monStep;	// eq step
	uint16_t compFact; 
	uint16_t attenFact; 
	uint8_t estResultComp;
	uint8_t estResultAtten;
	uint8_t vadcGain; 
}_stCEQData;
///////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////
/* sys */
typedef struct 
{
	uint8_t i2cRW_verify; 
	uint8_t chipID_verify; 
	uint8_t revID[MAX_PR1000_CHIP]; 
	uint8_t gPR1000RxType[MAX_CHN];
	uint8_t gPR1000RxResol[MAX_CHN];
	_stCEQData stCEQDataList[MAX_CHN]; //Max 4chip * 4channel
	_stOutFormatAttr stOutFormatAttr;
	_stPortChSel portChSel[MAX_CHN]; // get from pr1000_user_config.c 16:max channel
	int8_t cntSTDFORMATCheck[MAX_CHN];
	int8_t timeOverSTDFORMATCheck[MAX_CHN];

#ifdef __LINUX_SYSTEM__
	unsigned long bitChnLoadedRegTable;
	unsigned long bitChnWaitCheckChromaLock;
	unsigned long bitChnWakeIsrImmediately;

	unsigned long bitChnCheckChromaLock;
	unsigned long bitChnDoneChromaLock;
	unsigned long bitChnResultChromaLock;

	unsigned long bitChnCheckVadcGainSel;
	unsigned long bitChnDoneVadcGainSel;
	unsigned long bitChnResultVadcGainSel;
	unsigned long bitChnApplyVadcGainSel;
#else //#ifdef __LINUX_SYSTEM__
	unsigned char bitChnLoadedRegTable;
	unsigned char bitChnWaitCheckChromaLock;
	unsigned char bitChnWakeIsrImmediately;

	unsigned char bitChnCheckChromaLock;
	unsigned char bitChnDoneChromaLock;
	unsigned char bitChnResultChromaLock;

	unsigned char bitChnCheckVadcGainSel;
	unsigned char bitChnDoneVadcGainSel;
	unsigned char bitChnResultVadcGainSel;
	unsigned char bitChnApplyVadcGainSel;
#endif // __LINUX_SYSTEM__

	int8_t cntChromaLockTunn[MAX_CHN]; //Max 4chip * 4channel
	int8_t lastChromaLockTunn[MAX_CHN]; //Max 4chip * 4channel
	int dirTunnStep[MAX_CHN]; //+ or -
	int8_t cntVadcGainSelTunn[MAX_CHN]; //Max 4chip * 4channel

	uint32_t u32RxIntCnt; 
	uint32_t u32IntOldMsec; 
	uint32_t u32IntIntervalTimeMsec; 
}_sysHost;

/* event */
typedef struct 
{
  #ifndef DONT_SUPPORT_EVENT_FUNC
	_stVEVENTDisplayAttr stVEVENTDisplayAttr[MAX_CHN];
  #endif // DONT_SUPPORT_EVENT_FUNC

	_stEventDetStd stEventDetStd[MAX_CHN];
	uint8_t stDetManualStd[MAX_CHN];
}_eventHost;

/* ptz */
#define PTZ_STDFORMAT_CHECK_CNT	(3)
typedef struct 
{
	uint8_t initialized[MAX_CHN];
	uint8_t bRxUsedSTDFORMATOnly[MAX_CHN];
	uint8_t flagSTDFORMATResult[MAX_CHN];
	uint8_t testSTDFORMATData[MAX_CHN][PTZ_STDFORMAT_CHECK_CNT];
	uint8_t ptzType[MAX_CHN];
	uint8_t ptzResol[MAX_CHN];
	uint32_t u32PtzTxCnt[MAX_CHN];
	uint32_t u32PtzRxCnt[MAX_CHN];
#ifdef __LINUX_SYSTEM__
	unsigned long bitPtzIsrStatus[MAX_CHN];
#else //#ifdef __LINUX_SYSTEM__
	unsigned char bitPtzIsrStatus[MAX_CHN];
#endif // __LINUX_SYSTEM__

	FIFO ptzRecvQ[MAX_CHN];
	SPINLOCK_T splockPtzRecvQ[MAX_CHN];
}_ptzHost;

////////////////////////////////////////////////////////////////////////
#ifdef SUPPORT_PROC_SYSTEM //support proc system for linux debugging.
#define DRV_LOG_MEM_LENGTH	((PAGE_SIZE*2)-80)
#define DRV_LOG_MEM_NUM		(2)
typedef struct {
	char *pMem[DRV_LOG_MEM_NUM];
	uint32_t memLength[DRV_LOG_MEM_NUM];
	uint32_t wrPos[DRV_LOG_MEM_NUM];
	uint8_t logMemNum;
}_drvLog;
#endif // SUPPORT_PROC_SYSTEM

typedef struct {
	SPINLOCK_T 		spLockIrq;
	int			irq;
	unsigned int		irqFlag;

#ifdef __LINUX_SYSTEM__
#ifdef SUPPORT_PROC_SYSTEM //support proc system for linux debugging.
	struct proc_dir_entry	*procFp;
	struct proc_dir_entry	*procFpLog;
	struct proc_dir_entry 	*procFpRegDump;
	char procRegDumpStr0[PAGE_SIZE-80];
	struct proc_dir_entry 	*procFpRegWrite;
	char procRegWriteStr0[PAGE_SIZE-80];
	_drvLog 		drvLog;
#endif // SUPPORT_PROC_SYSTEM

	wait_queue_head_t 	wqPoll;
	_wqPollChnStatus	wqPollChnStatus;
#else //#ifdef __LINUX_SYSTEM__
	volatile int 	wqPoll;
	_wqPollChnStatus	wqPollChnStatus;
#endif // __LINUX_SYSTEM__

	//////////////////////////////////////////////
	_sysHost		sysHost;
	_eventHost		eventHost;
	_ptzHost		ptzHost;
}_drvHost;

//////////////////////////////////////////////////////////////////
#ifdef __LINUX_SYSTEM__
extern int PR1000_SetPage(const int fd, unsigned char slave, unsigned char page);
extern int PR1000_Read(const int fd, unsigned char slave, unsigned char reg, unsigned char *pRet);
extern int PR1000_Write(const int fd, unsigned char slave, unsigned char reg, unsigned char value);
extern int PR1000_PageRead(const int fd, unsigned char slave, int page, unsigned char reg, unsigned char *pRet);
extern int PR1000_PageWrite(const int fd, unsigned char slave, int page, unsigned char reg, unsigned char value);
extern int PR1000_PageReadBurst(const int fd, unsigned char slave, int page, unsigned char reg, unsigned short length, unsigned char *pRetData);
extern int PR1000_PageWriteBurst(const int fd, unsigned char slave, int page, unsigned char reg, unsigned short length, const unsigned char *pData);
extern int PR1000_ReadMaskBit(const int fd, unsigned char slave, int page, unsigned char reg, unsigned char regMaskBit, unsigned char *pRet);
extern int PR1000_WriteMaskBit(const int fd, unsigned char slave, int page, unsigned char reg, unsigned char regMaskBit, unsigned char value);

#if defined(SUPPORT_LOGMEM_PRINT) && defined(SUPPORT_PROC_SYSTEM)
extern _drvHost *gpDrvHost;
#define Print(string, args) \
        {/*printk("[PixelPlus]%s:",__FUNCTION__);printk(string,args);*/\
		{\
			if((gpDrvHost->drvLog.wrPos[gpDrvHost->drvLog.logMemNum]+80)>gpDrvHost->drvLog.memLength[gpDrvHost->drvLog.logMemNum])\
			{\
				gpDrvHost->drvLog.pMem[gpDrvHost->drvLog.logMemNum][gpDrvHost->drvLog.wrPos[gpDrvHost->drvLog.logMemNum]]=0;\
				gpDrvHost->drvLog.logMemNum++;\
				gpDrvHost->drvLog.logMemNum%=DRV_LOG_MEM_NUM;\
				gpDrvHost->drvLog.wrPos[gpDrvHost->drvLog.logMemNum]=0;\
			}\
			gpDrvHost->drvLog.wrPos[gpDrvHost->drvLog.logMemNum]+=sprintf((gpDrvHost->drvLog.pMem[gpDrvHost->drvLog.logMemNum]+gpDrvHost->drvLog.wrPos[gpDrvHost->drvLog.logMemNum]),string,args);\
		}\
	}
#define Error(string, args) \
        {/*printk("[PixelPlus] ### ERR ### :(%s:%d):",__FUNCTION__, __LINE__);printk(string,args);*/\
		{\
			if((gpDrvHost->drvLog.wrPos[gpDrvHost->drvLog.logMemNum]+80)>gpDrvHost->drvLog.memLength[gpDrvHost->drvLog.logMemNum])\
			{\
				gpDrvHost->drvLog.pMem[gpDrvHost->drvLog.logMemNum][gpDrvHost->drvLog.wrPos[gpDrvHost->drvLog.logMemNum]]=0;\
				gpDrvHost->drvLog.logMemNum++;gpDrvHost->drvLog.logMemNum%=DRV_LOG_MEM_NUM;\
				gpDrvHost->drvLog.wrPos[gpDrvHost->drvLog.logMemNum]=0;\
			}\
			gpDrvHost->drvLog.wrPos[gpDrvHost->drvLog.logMemNum]+=sprintf((gpDrvHost->drvLog.pMem[gpDrvHost->drvLog.logMemNum]+gpDrvHost->drvLog.wrPos[gpDrvHost->drvLog.logMemNum]),"### ERR ### :([%s:%d]):",__FUNCTION__,__LINE__);\
			gpDrvHost->drvLog.wrPos[gpDrvHost->drvLog.logMemNum]+=sprintf((gpDrvHost->drvLog.pMem[gpDrvHost->drvLog.logMemNum]+gpDrvHost->drvLog.wrPos[gpDrvHost->drvLog.logMemNum]),string,args);\
		}\
	}
#ifdef SUPPORT_DbgPrint
#define Dbg(string, args) \
        {/*printk("[PixelPlus]dbg>(%s):",__FUNCTION__);printk(string,args);*/\
		{\
			if((gpDrvHost->drvLog.wrPos[gpDrvHost->drvLog.logMemNum]+80)>gpDrvHost->drvLog.memLength[gpDrvHost->drvLog.logMemNum])\
			{\
				gpDrvHost->drvLog.pMem[gpDrvHost->drvLog.logMemNum][gpDrvHost->drvLog.wrPos[gpDrvHost->drvLog.logMemNum]]=0;\
				gpDrvHost->drvLog.logMemNum++;gpDrvHost->drvLog.logMemNum%=DRV_LOG_MEM_NUM;\
				gpDrvHost->drvLog.wrPos[gpDrvHost->drvLog.logMemNum]=0;\
			}\
			gpDrvHost->drvLog.wrPos[gpDrvHost->drvLog.logMemNum]+=sprintf((gpDrvHost->drvLog.pMem[gpDrvHost->drvLog.logMemNum]+gpDrvHost->drvLog.wrPos[gpDrvHost->drvLog.logMemNum]),"Dbg:(%s:%d):",__FUNCTION__,__LINE__);\
			gpDrvHost->drvLog.wrPos[gpDrvHost->drvLog.logMemNum]+=sprintf((gpDrvHost->drvLog.pMem[gpDrvHost->drvLog.logMemNum]+gpDrvHost->drvLog.wrPos[gpDrvHost->drvLog.logMemNum]),string,args);\
		}\
	}
#else
#define Dbg(string, args)
#endif //#ifdef SUPPORT_DbgPrint
#define PrintString(string) \
        {/*printk("[PixelPlus]%s:",__FUNCTION__);printk(string);*/\
		{\
			if((gpDrvHost->drvLog.wrPos[gpDrvHost->drvLog.logMemNum]+80)>gpDrvHost->drvLog.memLength[gpDrvHost->drvLog.logMemNum])\
			{\
				gpDrvHost->drvLog.pMem[gpDrvHost->drvLog.logMemNum][gpDrvHost->drvLog.wrPos[gpDrvHost->drvLog.logMemNum]]=0;\
				gpDrvHost->drvLog.logMemNum++;\
				gpDrvHost->drvLog.logMemNum%=DRV_LOG_MEM_NUM;\
				gpDrvHost->drvLog.wrPos[gpDrvHost->drvLog.logMemNum]=0;\
			}\
			gpDrvHost->drvLog.wrPos[gpDrvHost->drvLog.logMemNum]+=sprintf((gpDrvHost->drvLog.pMem[gpDrvHost->drvLog.logMemNum]+gpDrvHost->drvLog.wrPos[gpDrvHost->drvLog.logMemNum]),string);\
		}\
	}
#define ErrorString(string) \
        {/*printk("[PixelPlus] ### ERR ### :(%s:%d):",__FUNCTION__, __LINE__);printk(string);*/\
		{\
			if((gpDrvHost->drvLog.wrPos[gpDrvHost->drvLog.logMemNum]+80)>gpDrvHost->drvLog.memLength[gpDrvHost->drvLog.logMemNum])\
			{\
				gpDrvHost->drvLog.pMem[gpDrvHost->drvLog.logMemNum][gpDrvHost->drvLog.wrPos[gpDrvHost->drvLog.logMemNum]]=0;\
				gpDrvHost->drvLog.logMemNum++;gpDrvHost->drvLog.logMemNum%=DRV_LOG_MEM_NUM;\
				gpDrvHost->drvLog.wrPos[gpDrvHost->drvLog.logMemNum]=0;\
			}\
			gpDrvHost->drvLog.wrPos[gpDrvHost->drvLog.logMemNum]+=sprintf((gpDrvHost->drvLog.pMem[gpDrvHost->drvLog.logMemNum]+gpDrvHost->drvLog.wrPos[gpDrvHost->drvLog.logMemNum]),"### ERR ### :([%s:%d]):",__FUNCTION__,__LINE__);\
			gpDrvHost->drvLog.wrPos[gpDrvHost->drvLog.logMemNum]+=sprintf((gpDrvHost->drvLog.pMem[gpDrvHost->drvLog.logMemNum]+gpDrvHost->drvLog.wrPos[gpDrvHost->drvLog.logMemNum]),string);\
		}\
	}
#ifdef SUPPORT_DbgPrint
#define DbgString(string) \
        {/*printk("[PixelPlus]dbg>(%s):",__FUNCTION__);printk(string);*/\
		{\
			if((gpDrvHost->drvLog.wrPos[gpDrvHost->drvLog.logMemNum]+80)>gpDrvHost->drvLog.memLength[gpDrvHost->drvLog.logMemNum])\
			{\
				gpDrvHost->drvLog.pMem[gpDrvHost->drvLog.logMemNum][gpDrvHost->drvLog.wrPos[gpDrvHost->drvLog.logMemNum]]=0;\
				gpDrvHost->drvLog.logMemNum++;gpDrvHost->drvLog.logMemNum%=DRV_LOG_MEM_NUM;\
				gpDrvHost->drvLog.wrPos[gpDrvHost->drvLog.logMemNum]=0;\
			}\
			gpDrvHost->drvLog.wrPos[gpDrvHost->drvLog.logMemNum]+=sprintf((gpDrvHost->drvLog.pMem[gpDrvHost->drvLog.logMemNum]+gpDrvHost->drvLog.wrPos[gpDrvHost->drvLog.logMemNum]),"Dbg:(%s:%d):",__FUNCTION__,__LINE__);\
			gpDrvHost->drvLog.wrPos[gpDrvHost->drvLog.logMemNum]+=sprintf((gpDrvHost->drvLog.pMem[gpDrvHost->drvLog.logMemNum]+gpDrvHost->drvLog.wrPos[gpDrvHost->drvLog.logMemNum]),string);\
		}\
	}
#else
#define DbgString(string)
#endif //#ifdef SUPPORT_DbgPrint


#else //#if defined(SUPPORT_LOGMEM_PRINT) && defined(SUPPORT_PROC_SYSTEM)
#define Print(string, args) {printk("[PixelPlus]%s:",__FUNCTION__);printk(string,args); }
#define Error(string, args) {printk("[PixelPlus] ### ERR ### :(%s:%d):",__FUNCTION__, __LINE__);printk(string,args);}
#ifdef SUPPORT_DbgPrint
#define Dbg(string, args) {printk("[PixelPlus]dbg>(%s):",__FUNCTION__);printk(string,args);}
#else
#define Dbg(string, args)
#endif
#define PrintString(string) {printk("[PixelPlus]%s:",__FUNCTION__);printk(string); }
#define ErrorString(string) {printk("[PixelPlus] ### ERR ### :(%s:%d):",__FUNCTION__, __LINE__);printk(string);}
#ifdef SUPPORT_DbgPrint
#define DbgString(string) {printk("[PixelPlus]dbg>(%s):",__FUNCTION__);printk(string);}
#else
#define DbgString(string)
#endif
#endif // SUPPORT_LOGMEM_PRINT

#else //__LINUX_SYSTEM__

int pr1000_module_init(void);
void pr1000_module_exit(void);
int IrqHandler(void);
unsigned int pr1000_poll(unsigned int waitMsec);

extern int PR1000_SetPage(const int fd, unsigned char slave, unsigned char page);
extern int PR1000_Read(const int fd, unsigned char slave, unsigned char reg, unsigned char *pRet);
extern int PR1000_Write(const int fd, unsigned char slave, unsigned char reg, unsigned char value);
extern int PR1000_PageRead(const int fd, unsigned char slave, int page, unsigned char reg, unsigned char *pRet);
extern int PR1000_PageWrite(const int fd, unsigned char slave, int page, unsigned char reg, unsigned char value);
extern int PR1000_PageReadBurst(const int fd, unsigned char slave, int page, unsigned char reg, unsigned short length, unsigned char *pRetData);
extern int PR1000_PageWriteBurst(const int fd, unsigned char slave, int page, unsigned char reg, unsigned short length, const unsigned char *pData);
extern int PR1000_ReadMaskBit(const int fd, unsigned char slave, int page, unsigned char reg, unsigned char regMaskBit, unsigned char *pRet);
extern int PR1000_WriteMaskBit(const int fd, unsigned char slave, int page, unsigned char reg, unsigned char regMaskBit, unsigned char value);

extern _drvHost *gpDrvHost;
extern void MDelay(int);

#if defined(SUPPORT_LOGMEM_PRINT)
#define Print(string, args)  //{printf("[PixelPlus]:");printf(string,args); printf("\r");}
#define Error(string, args)
#define Dbg(string, args)
#define PrintString(string)
#define ErrorString(string)
#define DbgString(string)
#else //if defined(SUPPORT_LOGMEM_PRINT)
#define Print(string, args) {printf("[PixelPlus]:");printf(string,args); printf("\r");}
#define Error(string, args) {printf("[PixelPlus] ### ERR ### :");printf(string,args);printf("\r");}
#ifdef SUPPORT_DbgPrint
#define Dbg(string, args) {printf("[PixelPlus]dbg>:");printf(string,args);printf("\r");}
#else
#define Dbg(string, args)
#endif
#define PrintString(string) {printf("[PixelPlus]:");printf(string); printf("\r");}
#define ErrorString(string) {printf("[PixelPlus] ### ERR ### :");printf(string);printf("\r");}
#ifdef SUPPORT_DbgPrint
#define DbgString(string) {printf("[PixelPlus]dbg>:");printf(string);printf("\r");}
#else
#define DbgString(string)
#endif

#endif //if defined(SUPPORT_LOGMEM_PRINT)

#endif //__LINUX_SYSTEM__

#endif /* __PR1000_H__ */


