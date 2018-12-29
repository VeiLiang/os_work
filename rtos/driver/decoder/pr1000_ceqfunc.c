
#ifdef __LINUX_SYSTEM__
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/fcntl.h>
#include <linux/mm.h>
#include <linux/miscdevice.h>
#include <linux/proc_fs.h>
#include <linux/kthread.h>
#include <linux/freezer.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <asm/system.h>
#include <linux/interrupt.h>
#include <linux/ioport.h>
#include <linux/string.h>
#include <linux/list.h>
#include <asm/delay.h>
#include <linux/timer.h>
#include <linux/delay.h>
#include <linux/proc_fs.h>
#include <linux/poll.h>
#include <asm/bitops.h>
#include <asm/uaccess.h>
#include <asm/irq.h>
#include <linux/moduleparam.h>
#include <linux/ioport.h>
#include <linux/interrupt.h>
#include "pr1000.h"
#include "pr1000_user_config.h"
#include "pr1000_table.h"
#include "pr1000_ptz_table.h"
#include "pr1000_ptz_table_sd.h"
#ifndef DONT_SUPPORT_STD_PVI
#include "pr1000_ptz_table_pvi.h"
#endif 
#include "pr1000_ptz_table_hda.h"
#include "pr1000_ptz_table_cvi.h"
#include "pr1000_ptz_table_hdt.h"
#include "pr1000_func.h"
#include "pr1000_ceqfunc.h"
#include "drv_cq.h"
#else 
#include "pr1000.h"
#include "pr1000_user_config.h"
#include "pr1000_table.h"
#include "pr1000_ptz_table.h"
#include "pr1000_ptz_table_sd.h"
#ifndef DONT_SUPPORT_STD_PVI
#include "pr1000_ptz_table_pvi.h"
#endif 
#include "pr1000_ptz_table_hda.h"
#include "pr1000_ptz_table_cvi.h"
#include "pr1000_ptz_table_hdt.h"
#include "pr1000_func.h"
#include "pr1000_ceqfunc.h"
#include "drv_cq.h"
#endif 

#define DEF_PR1000_SIZE_CEQ_6PAGE_TABLE_REG		(0x80)
#define PR1000_OFFSETADDR_CEQ_6PAGE_CH(ch)		(((ch%2)*\
DEF_PR1000_SIZE_CEQ_6PAGE_TABLE_REG))
#define	PR1000_EQCOMP_LIMIT 		(0x1000)
#define PR1000_CEQ_EST_COMP(x,y) 	((0x9600*x)/y)
#define MAX_CHECK_EQ_STABLE_CNT		(5) 

int PR1000_GetCeqDetInfo(const int fd,const _stPortChSel*pstPortChSel,_stCEQData
*pstCEQData,_stCeqDet*pstCeqDet,_stCeqLock*pstCeqLock){uint8_t i2cReg=0;uint8_t 
page;uint8_t tmpBuffer[3];_stCeqDet stCeqDet;_stCeqLock stCeqLock;int mapChn;
uint8_t prChip,prChn,i2cSlaveAddr;if((pstPortChSel==NULL)||(pstCEQData==NULL)){
ErrorString("\x49\x6e\x76\x61\x6c\x69\x64\x20\x61\x72\x67\x75" "\n");return(-1);
}mapChn=pstPortChSel->chn;prChip=pstPortChSel->prChip;prChn=pstPortChSel->prChn;
i2cSlaveAddr=pstPortChSel->i2cSlvAddr;page=PR1000_REG_PAGE_COMMON;i2cReg=0x00+
PR1000_OFFSETADDR_CEQ_INFO_CH(prChn);if(PR1000_PageReadBurst(fd,i2cSlaveAddr,
page,i2cReg,2,tmpBuffer)<0){ErrorString(
"\x50\x61\x67\x65\x52\x65\x61\x64\x42\x75\x72\x73\x74\x2e" "\n");return(-1);}
stCeqDet.reg=tmpBuffer[0];stCeqLock.reg=tmpBuffer[1];if(pstCEQData->bForceChgStd
==TRUE)
{switch(pstCEQData->format){
#ifndef DONT_SUPPORT_STD_PVI
case pr1000_format_PVI:{stCeqDet.b.det_ifmt_std=PR1000_DET_IFMT_STD_PVI;}break;
#endif 
case pr1000_format_HDA:{stCeqDet.b.det_ifmt_std=PR1000_DET_IFMT_STD_HDA;}break;
case pr1000_format_CVI:{stCeqDet.b.det_ifmt_std=PR1000_DET_IFMT_STD_CVI;}break;
case pr1000_format_HDT:case pr1000_format_HDT_NEW:{stCeqDet.b.det_ifmt_std=
PR1000_DET_IFMT_STD_HDT;}break;default:{stCeqDet.b.det_ifmt_std=
PR1000_DET_IFMT_STD_CVI;}break;}}if(pstCeqDet){pstCeqDet->reg=stCeqDet.reg;}if(
pstCeqLock){pstCeqLock->reg=stCeqLock.reg;}return(2);}
int PR1000_CEQ_Start(const int fd,const int mapChn,void*pArgu){int ret=-1;
uint8_t i2cReg=0;uint8_t i2cData=0;uint8_t i2cMask=0;uint8_t page;_stCeqInfoReg 
infoRegs;_stCeqDet stCeqDet,*pstCeqDetManualStd;_stCeqLock stCeqLock;_stManEQMan
 stManEQMan;uint8_t tmpBuffer[2];uint8_t estResult[2];uint16_t estResultFact[2];
uint8_t bWrAtten=FALSE;uint8_t bWrComp=FALSE;uint8_t prChip,prChn,i2cSlaveAddr;
_drvHost*pHost=(_drvHost*)pArgu;_stPortChSel*pstPortChSel=(_stPortChSel*)&pHost
->sysHost.portChSel[mapChn];_stCEQData*pstCEQData=(_stCEQData*)&pHost->sysHost.
stCEQDataList[mapChn];if((pstPortChSel==NULL)||!ASSERT_VALID_CH(pstPortChSel->
i2cSlvAddr)){ErrorString("\x49\x6e\x76\x61\x6c\x69\x64\x20\x63\x68\x2e" "\n");
return(-1);}if(pstPortChSel->chn!=mapChn){Error(
"\x49\x6e\x76\x61\x6c\x69\x64\x20\x6d\x61\x70\x43\x68\x6e\x28\x25\x64\x29\x2d\x63\x68\x6e\x28\x25\x64\x29" "\n"
,mapChn COMMA pstPortChSel->chn);return(-1);}prChip=pstPortChSel->prChip;prChn=
pstPortChSel->prChn;i2cSlaveAddr=pstPortChSel->i2cSlvAddr;pstCeqDetManualStd=(
_stCeqDet*)&pHost->eventHost.stDetManualStd[mapChn];Dbg(
"\x6d\x61\x70\x43\x68\x6e\x3a\x25\x64" "\n",mapChn);if((PR1000_CEQ_STEP_NONE<
pstCEQData->estStep)&&(pstCEQData->estStep<PR1000_CEQ_STEP_EQSTDDONE)){
pstCEQData->estCheckTime+=PR1000_INT_SYNC_PERIOD;if(pstCEQData->estCheckTime<
PR1000_EQ_CHECK_PERIOD){return(0);}pstCEQData->estCheckTime=0;}Dbg(
"\x6d\x61\x70\x43\x68\x6e\x3a\x25\x64\x20\x65\x73\x74\x53\x74\x65\x70\x3a\x25\x64\x28\x25\x73\x29" "\n"
,mapChn COMMA pstCEQData->estStep COMMA _STR_PR1000_CEQ_STEP[pstCEQData->estStep
]);switch(pstCEQData->estStep){case PR1000_CEQ_STEP_NONE:{if(pstCEQData->bEnable
==0){enum _pr1000_table_format format;enum _pr1000_table_inresol camResol;enum 
_pr1000_table_outresol outResol;_PR1000_REG_TABLE_VDEC_HD*pTableHD=NULL;
_PR1000_REG_TABLE_VDEC_SD*pTableSD=NULL;
#ifndef DONT_SUPPORT_VDRESOL_EXTEND
_PR1000_REG_TABLE_VDEC_HD_EXTEND*pTableHD_EXTEND=NULL;
#endif 
if(PR1000_GetCeqDetInfo(fd,pstPortChSel,pstCEQData,&stCeqDet,&stCeqLock)<0){
ErrorString("\x47\x65\x74\x43\x65\x71\x44\x65\x74\x49\x6e\x66\x6f\x2e" "\n");
break;}Dbg(
"\x53\x74\x65\x70\x3a\x25\x64\x20\x72\x65\x67\x5b\x25\x30\x32\x78\x5d\x20\x6c\x6f\x63\x6b\x5b\x68\x70\x6c\x6c\x3a\x25\x64\x2c\x68\x70\x65\x72\x3a\x25\x64\x2c\x73\x74\x64\x3a\x25\x64\x5d" "\n"
,pstCEQData->estStep COMMA stCeqLock.reg COMMA stCeqLock.b.lock_hpll COMMA 
stCeqLock.b.lock_hperiod COMMA stCeqLock.b.lock_std);if((stCeqLock.b.lock_hpll==
1)&&(stCeqLock.b.lock_hperiod==1)&&(stCeqLock.b.lock_std==1)){Dbg(
"\x72\x65\x67\x20\x30\x78\x25\x30\x32\x78\x20\x20\x73\x74\x64\x3a\x25\x64\x2c\x72\x65\x66\x3a\x25\x64\x2c\x72\x65\x73\x3a\x25\x64" "\n"
,stCeqDet.reg COMMA stCeqDet.b.det_ifmt_std COMMA stCeqDet.b.det_ifmt_ref COMMA 
stCeqDet.b.det_ifmt_res);PR1000_IfmtToStdResol(fd,stCeqDet.b.det_ifmt_std,
stCeqDet.b.det_ifmt_ref,stCeqDet.b.det_ifmt_res,&format,&camResol);if(pstCEQData
->flagStepComplete[PR1000_CEQ_STEP_NONE]==FALSE){pstCEQData->flagStepComplete[
PR1000_CEQ_STEP_NONE]=TRUE;break;}}else{pstCEQData->retryCnt++;if(pstCEQData->
retryCnt>=5){format=pr1000_format_CVI;camResol=pr1000_inresol_1920x1080p25;}else
{break;
}}Print(
"\x45\x51\x20\x73\x74\x61\x72\x74\x20\x73\x74\x64\x20\x66\x6f\x72\x6d\x61\x74\x5b\x25\x73\x5d\x2c\x20\x72\x65\x73\x6f\x6c\x5b\x25\x73\x5d\x20\x6d\x61\x70\x43\x68\x6e\x3a\x25\x64" "\n"
,_STR_PR1000_FORMAT[format]COMMA _STR_PR1000_INRESOL[camResol]COMMA mapChn);{
page=PR1000_VDEC_PAGE(prChn);i2cReg=0x46+PR1000_OFFSETADDR_VDEC_1_2PAGE_CH(prChn
);i2cMask=0x20;i2cData=0x00;Dbg(
"\x57\x72\x69\x74\x65\x20\x5b\x70\x25\x64\x2c\x20\x30\x78\x25\x30\x32\x78\x2d\x30\x78\x25\x30\x32\x78\x28\x6d\x61\x73\x6b\x3a\x25\x30\x32\x78\x29\x5d" "\n"
,page COMMA i2cReg COMMA i2cData COMMA i2cMask);if((ret=PR1000_WriteMaskBit(fd,
i2cSlaveAddr,page,i2cReg,i2cMask,i2cData))<0){ErrorString(
"\x57\x72\x69\x74\x65\x20\x72\x65\x67\x2e" "\n");}page=PR1000_VDEC_PAGE(prChn);
i2cReg=0x50+PR1000_OFFSETADDR_VDEC_1_2PAGE_CH(prChn);i2cMask=0x07;i2cData=0x01;
Dbg(
"\x57\x72\x69\x74\x65\x20\x5b\x70\x25\x64\x2c\x20\x30\x78\x25\x30\x32\x78\x2d\x30\x78\x25\x30\x32\x78\x28\x6d\x61\x73\x6b\x3a\x25\x30\x32\x78\x29\x5d" "\n"
,page COMMA i2cReg COMMA i2cData COMMA i2cMask);if((ret=PR1000_WriteMaskBit(fd,
i2cSlaveAddr,page,i2cReg,i2cMask,i2cData))<0){ErrorString(
"\x57\x72\x69\x74\x65\x20\x72\x65\x67\x2e" "\n");}page=PR1000_VDEC_PAGE(prChn);
i2cReg=0x62+PR1000_OFFSETADDR_VDEC_1_2PAGE_CH(prChn);i2cData=0x8d;Dbg(
"\x57\x72\x69\x74\x65\x20\x5b\x70\x25\x64\x2c\x20\x30\x78\x25\x30\x32\x78\x2d\x30\x78\x25\x30\x32\x78\x5d" "\n"
,page COMMA i2cReg COMMA i2cData);if((ret=PR1000_PageWrite(fd,i2cSlaveAddr,page,
i2cReg,(uint8_t)i2cData))<0){ErrorString(
"\x57\x72\x69\x74\x65\x20\x72\x65\x67\x2e" "\n");}tmpBuffer[0]=0x00;
tmpBuffer[1]=0x00;
switch(format){case pr1000_format_SD720:case pr1000_format_SD960:{pTableSD=(
_PR1000_REG_TABLE_VDEC_SD*)pr1000_reg_table_vdec_SD;switch(camResol){case 
pr1000_inresol_ntsc:{outResol=pr1000_outresol_720x480i60;}break;case 
pr1000_inresol_pal:{outResol=pr1000_outresol_720x576i50;}break;default:{outResol
=pr1000_outresol_720x480i60;}break;}tmpBuffer[0]=(uint8_t)(pTableSD[0].pData[
outResol]);
tmpBuffer[1]=(uint8_t)(pTableSD[1].pData[outResol]);
}break;
#ifndef DONT_SUPPORT_STD_PVI
case pr1000_format_PVI:{pTableHD=(_PR1000_REG_TABLE_VDEC_HD*)
pr1000_reg_table_vdec_PVI;
#ifndef DONT_SUPPORT_VDRESOL_EXTEND
pTableHD_EXTEND=(_PR1000_REG_TABLE_VDEC_HD_EXTEND*)
pr1000_reg_table_vdec_PVI_extend;
#endif 
switch(camResol){case pr1000_inresol_1280x720p60:{outResol=
pr1000_outresol_1280x720p60;}break;case pr1000_inresol_1280x720p50:{outResol=
pr1000_outresol_1280x720p50;}break;case pr1000_inresol_1280x720p30:{outResol=
pr1000_outresol_1280x720p30;}break;case pr1000_inresol_1280x720p25:{outResol=
pr1000_outresol_1280x720p25;}break;case pr1000_inresol_1920x1080p30:{outResol=
pr1000_outresol_1920x1080p30;}break;case pr1000_inresol_1920x1080p25:{outResol=
pr1000_outresol_1920x1080p25;}break;default:{outResol=
pr1000_outresol_1920x1080p30;}break;}if(outResol<=pr1000_outresol_1920x1080p25){
tmpBuffer[0]=(uint8_t)(pTableHD[0].pData[outResol-pr1000_outresol_1280x720p60]);
tmpBuffer[1]=(uint8_t)(pTableHD[1].pData[outResol-pr1000_outresol_1280x720p60]);
}
#ifndef DONT_SUPPORT_VDRESOL_EXTEND
else{tmpBuffer[0]=(uint8_t)(pTableHD_EXTEND[0].pData[outResol-
pr1000_outresol_1280x720p60c]);
tmpBuffer[1]=(uint8_t)(pTableHD_EXTEND[1].pData[outResol-
pr1000_outresol_1280x720p60c]);
}
#endif 
}break;
#endif 
case pr1000_format_HDA:{pTableHD=(_PR1000_REG_TABLE_VDEC_HD*)
pr1000_reg_table_vdec_HDA;
#ifndef DONT_SUPPORT_VDRESOL_EXTEND
pTableHD_EXTEND=(_PR1000_REG_TABLE_VDEC_HD_EXTEND*)
pr1000_reg_table_vdec_HDA_extend;
#endif 
switch(camResol){case pr1000_inresol_1280x720p60:{outResol=
pr1000_outresol_1280x720p60;}break;case pr1000_inresol_1280x720p50:{outResol=
pr1000_outresol_1280x720p50;}break;case pr1000_inresol_1280x720p30:{outResol=
pr1000_outresol_1280x720p30;}break;case pr1000_inresol_1280x720p25:{outResol=
pr1000_outresol_1280x720p25;}break;case pr1000_inresol_1920x1080p30:{outResol=
pr1000_outresol_1920x1080p30;}break;case pr1000_inresol_1920x1080p25:{outResol=
pr1000_outresol_1920x1080p25;}break;default:{outResol=
pr1000_outresol_1920x1080p30;}break;}if(outResol<=pr1000_outresol_1920x1080p25){
tmpBuffer[0]=(uint8_t)(pTableHD[0].pData[outResol-pr1000_outresol_1280x720p60]);
tmpBuffer[1]=(uint8_t)(pTableHD[1].pData[outResol-pr1000_outresol_1280x720p60]);
}
#ifndef DONT_SUPPORT_VDRESOL_EXTEND
else{tmpBuffer[0]=(uint8_t)(pTableHD_EXTEND[0].pData[outResol-
pr1000_outresol_1280x720p60c]);
tmpBuffer[1]=(uint8_t)(pTableHD_EXTEND[1].pData[outResol-
pr1000_outresol_1280x720p60c]);
}
#endif 
}break;case pr1000_format_CVI:{pTableHD=(_PR1000_REG_TABLE_VDEC_HD*)
pr1000_reg_table_vdec_CVI;
#ifndef DONT_SUPPORT_VDRESOL_EXTEND
pTableHD_EXTEND=(_PR1000_REG_TABLE_VDEC_HD_EXTEND*)
pr1000_reg_table_vdec_CVI_extend;
#endif 
switch(camResol){case pr1000_inresol_1280x720p60:{outResol=
pr1000_outresol_1280x720p60;}break;case pr1000_inresol_1280x720p50:{outResol=
pr1000_outresol_1280x720p50;}break;case pr1000_inresol_1280x720p30:{outResol=
pr1000_outresol_1280x720p30;}break;case pr1000_inresol_1280x720p25:{outResol=
pr1000_outresol_1280x720p25;}break;case pr1000_inresol_1920x1080p30:{outResol=
pr1000_outresol_1920x1080p30;}break;case pr1000_inresol_1920x1080p25:{outResol=
pr1000_outresol_1920x1080p25;}break;default:{outResol=
pr1000_outresol_1920x1080p30;}break;}if(outResol<=pr1000_outresol_1920x1080p25){
tmpBuffer[0]=(uint8_t)(pTableHD[0].pData[outResol-pr1000_outresol_1280x720p60]);
tmpBuffer[1]=(uint8_t)(pTableHD[1].pData[outResol-pr1000_outresol_1280x720p60]);
}
#ifndef DONT_SUPPORT_VDRESOL_EXTEND
else{tmpBuffer[0]=(uint8_t)(pTableHD_EXTEND[0].pData[outResol-
pr1000_outresol_1280x720p60c]);
tmpBuffer[1]=(uint8_t)(pTableHD_EXTEND[1].pData[outResol-
pr1000_outresol_1280x720p60c]);
}
#endif 
}break;case pr1000_format_HDT:case pr1000_format_HDT_NEW:{pTableHD=(
_PR1000_REG_TABLE_VDEC_HD*)pr1000_reg_table_vdec_HDT;
#ifndef DONT_SUPPORT_VDRESOL_EXTEND
pTableHD_EXTEND=(_PR1000_REG_TABLE_VDEC_HD_EXTEND*)
pr1000_reg_table_vdec_HDT_extend;
#endif 
switch(camResol){case pr1000_inresol_1280x720p60:{outResol=
pr1000_outresol_1280x720p60;}break;case pr1000_inresol_1280x720p50:{outResol=
pr1000_outresol_1280x720p50;}break;case pr1000_inresol_1280x720p30:{outResol=
pr1000_outresol_1280x720p30;}break;case pr1000_inresol_1280x720p25:{outResol=
pr1000_outresol_1280x720p25;}break;case pr1000_inresol_1920x1080p30:{outResol=
pr1000_outresol_1920x1080p30;}break;case pr1000_inresol_1920x1080p25:{outResol=
pr1000_outresol_1920x1080p25;}break;default:{outResol=
pr1000_outresol_1920x1080p30;}break;}if(outResol<=pr1000_outresol_1920x1080p25){
tmpBuffer[0]=(uint8_t)(pTableHD[0].pData[outResol-pr1000_outresol_1280x720p60]);
tmpBuffer[1]=(uint8_t)(pTableHD[1].pData[outResol-pr1000_outresol_1280x720p60]);
}
#ifndef DONT_SUPPORT_VDRESOL_EXTEND
else{tmpBuffer[0]=(uint8_t)(pTableHD_EXTEND[0].pData[outResol-
pr1000_outresol_1280x720p60c]);
tmpBuffer[1]=(uint8_t)(pTableHD_EXTEND[1].pData[outResol-
pr1000_outresol_1280x720p60c]);
}
#endif 
}break;default:{pTableHD=(_PR1000_REG_TABLE_VDEC_HD*)pr1000_reg_table_vdec_CVI;
#ifndef DONT_SUPPORT_VDRESOL_EXTEND
pTableHD_EXTEND=(_PR1000_REG_TABLE_VDEC_HD_EXTEND*)
pr1000_reg_table_vdec_CVI_extend;
#endif 
switch(camResol){case pr1000_inresol_1280x720p60:{outResol=
pr1000_outresol_1280x720p60;}break;case pr1000_inresol_1280x720p50:{outResol=
pr1000_outresol_1280x720p50;}break;case pr1000_inresol_1280x720p30:{outResol=
pr1000_outresol_1280x720p30;}break;case pr1000_inresol_1280x720p25:{outResol=
pr1000_outresol_1280x720p25;}break;case pr1000_inresol_1920x1080p30:{outResol=
pr1000_outresol_1920x1080p30;}break;case pr1000_inresol_1920x1080p25:{outResol=
pr1000_outresol_1920x1080p25;}break;default:{outResol=
pr1000_outresol_1920x1080p30;}break;}if(outResol<=pr1000_outresol_1920x1080p25){
tmpBuffer[0]=(uint8_t)(pTableHD[0].pData[outResol-pr1000_outresol_1280x720p60]);
tmpBuffer[1]=(uint8_t)(pTableHD[1].pData[outResol-pr1000_outresol_1280x720p60]);
}
#ifndef DONT_SUPPORT_VDRESOL_EXTEND
else{tmpBuffer[0]=(uint8_t)(pTableHD_EXTEND[0].pData[outResol-
pr1000_outresol_1280x720p60c]);
tmpBuffer[1]=(uint8_t)(pTableHD_EXTEND[1].pData[outResol-
pr1000_outresol_1280x720p60c]);
}
#endif 
}break;}page=PR1000_REG_PAGE_COMMON;
i2cReg=0x10+PR1000_OFFSETADDR_VDEC_0PAGE_CH(prChn);if(PR1000_PageWrite(fd,
i2cSlaveAddr,page,i2cReg,tmpBuffer[0])<0){ErrorString(
"\x57\x72\x69\x74\x65\x20\x72\x65\x67\x2e" "\n");break;}i2cReg=0x11+
PR1000_OFFSETADDR_VDEC_0PAGE_CH(prChn);i2cMask=0x0f;if((ret=PR1000_WriteMaskBit(
fd,i2cSlaveAddr,page,i2cReg,i2cMask,tmpBuffer[1]))<0){ErrorString(
"\x57\x72\x69\x74\x65\x20\x72\x65\x67\x2e" "\n");break;}}memset(pstCEQData,0,
sizeof(_stCEQData));pHost->sysHost.cntChromaLockTunn[mapChn]=0;pstCEQData->
bEnable=1;
#ifdef SUPPORT_CABLE_EQ
pstCEQData->estStep=PR1000_CEQ_STEP_EQSTART;
#else
page=PR1000_REG_PAGE_COMMON;
i2cReg=0x12+PR1000_OFFSETADDR_VDEC_0PAGE_CH(prChn);i2cMask=0x20;i2cData=0x00;if(
(ret=PR1000_WriteMaskBit(fd,i2cSlaveAddr,page,i2cReg,i2cMask,i2cData))<0){
ErrorString("\x57\x72\x69\x74\x65\x20\x72\x65\x67\x2e" "\n");break;}pstCEQData->
estStep=PR1000_CEQ_STEP_STDCHECK;
#endif 
pHost->sysHost.cntSTDFORMATCheck[mapChn]=0;pHost->sysHost.timeOverSTDFORMATCheck
[mapChn]=0;pstCEQData->stableEQCnt=0;pstCEQData->format=format;
pstCEQData->camResol=camResol;
Print(
"\x53\x74\x61\x72\x74\x20\x45\x51\x20\x5b\x6d\x61\x70\x43\x68\x6e\x3a\x25\x64\x2c\x20\x65\x73\x74\x53\x74\x65\x70\x3a\x25\x64\x2c\x20\x69\x6e\x69\x74\x20\x53\x54\x44\x46\x4f\x52\x4d\x41\x54\x5d" "\n"
,mapChn COMMA pstCEQData->estStep);}else{ErrorString(
"\x49\x6e\x76\x61\x6c\x69\x64\x20\x53\x74\x65\x70" "\n");break;}}break;case 
PR1000_CEQ_STEP_EQSTART:{
#ifdef SUPPORT_CABLE_EQ
bWrAtten=TRUE;bWrComp=FALSE;if(PR1000_CEQ_SetEQGain(fd,bWrAtten,bWrComp,
pstPortChSel,pstCEQData)<0){Error(
"\x53\x65\x74\x20\x45\x51\x20\x47\x61\x69\x6e\x2e\x20\x6d\x61\x70\x43\x68\x6e\x3a\x25\x64" "\n"
,mapChn);break;}if(PR1000_CEQ_SetVADCGain(fd,pstCEQData->format,pstCEQData->
camResol,pstPortChSel,pstCEQData)<0){Error(
"\x53\x65\x74\x20\x56\x41\x44\x43\x20\x47\x61\x69\x6e\x2e\x20\x6d\x61\x70\x43\x68\x6e\x3a\x25\x64" "\n"
,mapChn);break;}
#endif 
pstCEQData->flagStepComplete[PR1000_CEQ_STEP_EQSTART]=TRUE;pstCEQData->estStep=
PR1000_CEQ_STEP_STDCHECK;}break;case PR1000_CEQ_STEP_STDCHECK:{if(
PR1000_GetCeqDetInfo(fd,pstPortChSel,pstCEQData,&stCeqDet,&stCeqLock)<0){
ErrorString("\x47\x65\x74\x43\x65\x71\x44\x65\x74\x49\x6e\x66\x6f\x2e" "\n");
break;}Dbg(
"\x53\x74\x65\x70\x3a\x25\x64\x20\x72\x65\x67\x5b\x25\x30\x32\x78\x5d\x20\x6c\x6f\x63\x6b\x5b\x68\x70\x6c\x6c\x3a\x25\x64\x2c\x68\x70\x65\x72\x3a\x25\x64\x2c\x73\x74\x64\x3a\x25\x64\x5d" "\n"
,pstCEQData->estStep COMMA stCeqLock.reg COMMA stCeqLock.b.lock_hpll COMMA 
stCeqLock.b.lock_hperiod COMMA stCeqLock.b.lock_std);if((stCeqLock.b.lock_hpll==
1)&&(stCeqLock.b.lock_hperiod==1)&&(stCeqLock.b.lock_std==1)){Dbg(
"\x72\x65\x67\x20\x30\x78\x25\x30\x32\x78\x20\x20\x73\x74\x64\x3a\x25\x64\x2c\x72\x65\x66\x3a\x25\x64\x2c\x72\x65\x73\x3a\x25\x64" "\n"
,stCeqDet.reg COMMA stCeqDet.b.det_ifmt_std COMMA stCeqDet.b.det_ifmt_ref COMMA 
stCeqDet.b.det_ifmt_res);page=PR1000_REG_PAGE_COMMON;i2cReg=0x10+
PR1000_OFFSETADDR_CEQ_INFO_CH(prChn);if((ret=PR1000_PageRead(fd,i2cSlaveAddr,
page,i2cReg,&stManEQMan.reg))<0){ErrorString(
"\x52\x65\x61\x64\x20\x72\x65\x67\x2e" "\n");return(-1);}Dbg(
"\x72\x65\x67\x20\x30\x78\x25\x30\x32\x78\x20\x20\x73\x74\x64\x3a\x25\x64\x2c\x72\x65\x66\x3a\x25\x64\x2c\x72\x65\x73\x3a\x25\x64" "\n"
,stManEQMan.reg COMMA stManEQMan.b.man_ifmt_std COMMA stManEQMan.b.man_ifmt_ref 
COMMA stManEQMan.b.man_ifmt_res);if((stCeqDet.b.det_ifmt_std!=stManEQMan.b.
man_ifmt_std)||(stCeqDet.b.det_ifmt_ref!=stManEQMan.b.man_ifmt_ref)||(stCeqDet.b
.det_ifmt_res!=stManEQMan.b.man_ifmt_res)){Print(
"\x4f\x6c\x64\x20\x73\x74\x64\x3a\x25\x64\x28\x25\x73\x29\x2c\x72\x65\x66\x3a\x25\x64\x28\x25\x73\x29\x2c\x72\x65\x73\x3a\x25\x64\x28\x25\x73\x29" "\n"
,stManEQMan.b.man_ifmt_std COMMA _STR_PR1000_IFMT_STD[stManEQMan.b.man_ifmt_std]
COMMA stManEQMan.b.man_ifmt_ref COMMA _STR_PR1000_IFMT_REF[stManEQMan.b.
man_ifmt_ref]COMMA stManEQMan.b.man_ifmt_res COMMA _STR_PR1000_IFMT_RES[
stManEQMan.b.man_ifmt_res]);Print(
"\x43\x75\x72\x20\x73\x74\x64\x3a\x25\x64\x28\x25\x73\x29\x2c\x72\x65\x66\x3a\x25\x64\x28\x25\x73\x29\x2c\x72\x65\x73\x3a\x25\x64\x28\x25\x73\x29" "\n"
,stCeqDet.b.det_ifmt_std COMMA _STR_PR1000_IFMT_STD[stCeqDet.b.det_ifmt_std]
COMMA stCeqDet.b.det_ifmt_ref COMMA _STR_PR1000_IFMT_REF[stCeqDet.b.det_ifmt_ref
]COMMA stCeqDet.b.det_ifmt_res COMMA _STR_PR1000_IFMT_RES[stCeqDet.b.
det_ifmt_res]);stManEQMan.b.man_ifmt_std=stCeqDet.b.det_ifmt_std;stManEQMan.b.
man_ifmt_ref=stCeqDet.b.det_ifmt_ref;stManEQMan.b.man_ifmt_res=stCeqDet.b.
det_ifmt_res;page=PR1000_REG_PAGE_COMMON;i2cReg=0x10+
PR1000_OFFSETADDR_CEQ_INFO_CH(prChn);if((ret=PR1000_PageWrite(fd,i2cSlaveAddr,
page,i2cReg,stManEQMan.reg))<0){ErrorString(
"\x57\x72\x69\x74\x65\x20\x72\x65\x67\x2e" "\n");}Print(
"\x53\x54\x44\x43\x68\x65\x63\x6b\x20\x43\x68\x61\x6e\x67\x65\x20\x73\x74\x64\x3a\x25\x64\x28\x25\x73\x29\x2c\x72\x65\x66\x3a\x25\x64\x28\x25\x73\x29\x2c\x72\x65\x73\x3a\x25\x64\x28\x25\x73\x29" "\n"
,stManEQMan.b.man_ifmt_std COMMA _STR_PR1000_IFMT_STD[stManEQMan.b.man_ifmt_std]
COMMA stManEQMan.b.man_ifmt_ref COMMA _STR_PR1000_IFMT_REF[stManEQMan.b.
man_ifmt_ref]COMMA stManEQMan.b.man_ifmt_res COMMA _STR_PR1000_IFMT_RES[
stManEQMan.b.man_ifmt_res]);PR1000_IfmtToStdResol(fd,stManEQMan.b.man_ifmt_std,
stManEQMan.b.man_ifmt_ref,stManEQMan.b.man_ifmt_res,&pstCEQData->format,&
pstCEQData->camResol);
#ifdef SUPPORT_CABLE_EQ
bWrAtten=TRUE;bWrComp=FALSE;if(PR1000_CEQ_SetEQGain(fd,bWrAtten,bWrComp,
pstPortChSel,pstCEQData)<0){Error(
"\x53\x65\x74\x20\x45\x51\x20\x47\x61\x69\x6e\x2e\x20\x6d\x61\x70\x43\x68\x6e\x3a\x25\x64" "\n"
,mapChn);break;}if(PR1000_CEQ_SetVADCGain(fd,pstCEQData->format,pstCEQData->
camResol,pstPortChSel,pstCEQData)<0){Error(
"\x53\x65\x74\x20\x56\x41\x44\x43\x20\x47\x61\x69\x6e\x2e\x20\x6d\x61\x70\x43\x68\x6e\x3a\x25\x64" "\n"
,mapChn);break;}
#endif 
pstCEQData->retryCnt++;if(pstCEQData->retryCnt>=5){Dbg(
"\x4c\x6f\x63\x6b\x20\x44\x65\x74\x65\x63\x74\x20\x73\x74\x64\x3a\x25\x64\x28\x25\x73\x29\x2c\x72\x65\x66\x3a\x25\x64\x28\x25\x73\x29\x2c\x72\x65\x73\x3a\x25\x64\x28\x25\x73\x29" "\n"
,stManEQMan.b.man_ifmt_std COMMA _STR_PR1000_IFMT_STD[stManEQMan.b.man_ifmt_std]
COMMA stManEQMan.b.man_ifmt_ref COMMA _STR_PR1000_IFMT_REF[stManEQMan.b.
man_ifmt_ref]COMMA stManEQMan.b.man_ifmt_res COMMA _STR_PR1000_IFMT_RES[
stManEQMan.b.man_ifmt_res]);pstCEQData->bLock=1;pstCEQData->retryCnt=0;
pstCEQData->flagStepComplete[PR1000_CEQ_STEP_STDCHECK]=FALSE;memcpy(&pstCEQData
->stManEQMan,&stManEQMan,sizeof(stManEQMan));PR1000_IfmtToStdResol(fd,stManEQMan
.b.man_ifmt_std,stManEQMan.b.man_ifmt_ref,stManEQMan.b.man_ifmt_res,&pstCEQData
->format,&pstCEQData->camResol);if(((pstCEQData->format==pr1000_format_HDT)&&((
pstCEQData->camResol==pr1000_inresol_1280x720p30)||(pstCEQData->camResol==
pr1000_inresol_1280x720p25)))||((pstCEQData->format==pr1000_format_HDA))){
PR1000_CEQ_SetVIDStd(fd,pstPortChSel,pstCEQData->format,pstCEQData->camResol);
PR1000_PTZ_STDFORMAT_Check(fd,pstPortChSel,pstCEQData->format,pstCEQData->
camResol,pHost);pstCEQData->estStep=PR1000_CEQ_STEP_STDFORMAT_CHECK;}else{
pstCEQData->estStep=PR1000_CEQ_STEP_EQCHECK;}break;}}else{Print(
"\x4c\x6f\x63\x6b\x20\x44\x65\x74\x65\x63\x74\x20\x73\x74\x64\x3a\x25\x64\x28\x25\x73\x29\x2c\x72\x65\x66\x3a\x25\x64\x28\x25\x73\x29\x2c\x72\x65\x73\x3a\x25\x64\x28\x25\x73\x29" "\n"
,stManEQMan.b.man_ifmt_std COMMA _STR_PR1000_IFMT_STD[stManEQMan.b.man_ifmt_std]
COMMA stManEQMan.b.man_ifmt_ref COMMA _STR_PR1000_IFMT_REF[stManEQMan.b.
man_ifmt_ref]COMMA stManEQMan.b.man_ifmt_res COMMA _STR_PR1000_IFMT_RES[
stManEQMan.b.man_ifmt_res]);pstCEQData->bLock=1;pstCEQData->estStdComplete=1;
pstCEQData->flagStepComplete[PR1000_CEQ_STEP_STDCHECK]=TRUE;pstCEQData->retryCnt
=0;memcpy(&pstCEQData->stManEQMan,&stManEQMan,sizeof(stManEQMan));
PR1000_IfmtToStdResol(fd,stManEQMan.b.man_ifmt_std,stManEQMan.b.man_ifmt_ref,
stManEQMan.b.man_ifmt_res,&pstCEQData->format,&pstCEQData->camResol);if(((
pstCEQData->format==pr1000_format_HDT)&&((pstCEQData->camResol==
pr1000_inresol_1280x720p30)||(pstCEQData->camResol==pr1000_inresol_1280x720p25))
)||((pstCEQData->format==pr1000_format_HDA))){PR1000_CEQ_SetVIDStd(fd,
pstPortChSel,pstCEQData->format,pstCEQData->camResol);PR1000_PTZ_STDFORMAT_Check
(fd,pstPortChSel,pstCEQData->format,pstCEQData->camResol,pHost);pstCEQData->
estStep=PR1000_CEQ_STEP_STDFORMAT_CHECK;}else{pstCEQData->estStep=
PR1000_CEQ_STEP_EQCHECK;}}}else{pstCEQData->retryCnt++;if(pstCEQData->retryCnt>=
5){Error(
"\x53\x54\x44\x20\x6c\x6f\x63\x6b\x20\x66\x61\x69\x6c\x2e\x20\x4f\x76\x65\x72\x20\x63\x68\x65\x63\x6b\x20\x74\x69\x6d\x65\x2e\x20\x46\x6f\x72\x63\x65\x6c\x79\x20\x73\x65\x74\x20\x64\x65\x66\x61\x75\x6c\x74\x2e\x20\x5b\x6d\x61\x70\x43\x68\x6e\x3a\x25\x64\x2c\x20\x65\x73\x74\x53\x74\x65\x70\x3a\x25\x64\x5d" "\n"
,mapChn COMMA pstCEQData->estStep);Dbg(
"\x72\x65\x67\x20\x30\x78\x25\x30\x32\x78\x20\x20\x73\x74\x64\x3a\x25\x64\x2c\x72\x65\x66\x3a\x25\x64\x2c\x72\x65\x73\x3a\x25\x64" "\n"
,stCeqDet.reg COMMA stCeqDet.b.det_ifmt_std COMMA stCeqDet.b.det_ifmt_ref COMMA 
stCeqDet.b.det_ifmt_res);stCeqDet.b.det_ifmt_std=PR1000_DET_IFMT_STD_CVI;
stManEQMan.b.man_ifmt_std=stCeqDet.b.det_ifmt_std;stManEQMan.b.man_ifmt_ref=
stCeqDet.b.det_ifmt_ref;stManEQMan.b.man_ifmt_res=stCeqDet.b.det_ifmt_res;page=
PR1000_REG_PAGE_COMMON;i2cReg=0x10+PR1000_OFFSETADDR_CEQ_INFO_CH(prChn);if((ret=
PR1000_PageWrite(fd,i2cSlaveAddr,page,i2cReg,stManEQMan.reg))<0){ErrorString(
"\x57\x72\x69\x74\x65\x20\x72\x65\x67\x2e" "\n");}pstCEQData->bLock=1;pstCEQData
->retryCnt=0;pstCEQData->flagStepComplete[PR1000_CEQ_STEP_STDCHECK]=FALSE;memcpy
(&pstCEQData->stManEQMan,&stManEQMan,sizeof(stManEQMan));PR1000_IfmtToStdResol(
fd,stManEQMan.b.man_ifmt_std,stManEQMan.b.man_ifmt_ref,stManEQMan.b.man_ifmt_res
,&pstCEQData->format,&pstCEQData->camResol);if(((pstCEQData->format==
pr1000_format_HDT)&&((pstCEQData->camResol==pr1000_inresol_1280x720p30)||(
pstCEQData->camResol==pr1000_inresol_1280x720p25)))||((pstCEQData->format==
pr1000_format_HDA))){PR1000_CEQ_SetVIDStd(fd,pstPortChSel,pstCEQData->format,
pstCEQData->camResol);PR1000_PTZ_STDFORMAT_Check(fd,pstPortChSel,pstCEQData->
format,pstCEQData->camResol,pHost);pstCEQData->estStep=
PR1000_CEQ_STEP_STDFORMAT_CHECK;}else{pstCEQData->estStep=
PR1000_CEQ_STEP_EQCHECK;}}}}break;case PR1000_CEQ_STEP_EQCHECK:{
#ifdef SUPPORT_CABLE_EQ
if((PR1000_CEQ_GetInfoRegs(fd,pstPortChSel,&infoRegs))<0){pstCEQData->retryCnt=0
;pstCEQData->flagStepComplete[PR1000_CEQ_STEP_EQCHECK]=FALSE;PR1000_CEQ_Done(fd,
mapChn,(void*)pHost);break;}if((ret=PR1000_CEQ_GetSTDEst(fd,pstCEQData->format,
pstCEQData->camResol,&infoRegs,estResult,estResultFact))<0){ErrorString(
"\x67\x65\x74\x20\x65\x73\x74" "\n");pstCEQData->retryCnt=0;pstCEQData->
flagStepComplete[PR1000_CEQ_STEP_EQCHECK]=FALSE;PR1000_CEQ_Done(fd,mapChn,(void*
)pHost);break;}else if(ret==0){estResult[0]=pstCEQData->estResultAtten;estResult
[1]=pstCEQData->estResultComp;estResultFact[0]=pstCEQData->attenFact;
estResultFact[1]=pstCEQData->compFact;}if((pstCEQData->estResultAtten==estResult
[0])&&(pstCEQData->estResultComp==estResult[1])){Print(
"\x3d\x3e\x20\x53\x54\x44\x20\x45\x51\x20\x76\x61\x6c\x75\x65\x20\x73\x61\x6d\x65\x20\x77\x69\x74\x68\x20\x70\x72\x65\x76\x69\x6f\x75\x73\x28\x6d\x61\x70\x43\x68\x6e\x3a\x25\x64\x29\x2e\x20\x28\x41\x74\x74\x65\x6e\x3a\x25\x64\x6d\x2c\x30\x78\x25\x78\x29\x2c\x20\x28\x43\x6f\x6d\x70\x3a\x25\x64\x6d\x2c\x30\x78\x25\x78\x29" "\n"
,mapChn COMMA estResultFact[0]COMMA estResult[0]COMMA estResultFact[1]COMMA 
estResult[1]);pstCEQData->retryCnt=0;pstCEQData->flagStepComplete[
PR1000_CEQ_STEP_EQCHECK]=TRUE;PR1000_CEQ_Done(fd,mapChn,(void*)pHost);}else{
pstCEQData->retryCnt++;bWrAtten=TRUE;bWrComp=FALSE;if(bWrAtten){page=
PR1000_REG_PAGE_COMMON;i2cReg=0x12+PR1000_OFFSETADDR_CEQ_INFO_CH(prChn);
estResult[0]|=0x20;
PR1000_PageWrite(fd,i2cSlaveAddr,page,i2cReg,(uint8_t)estResult[0]);Print(
"\x53\x65\x74\x20\x53\x54\x44\x20\x45\x51\x20\x76\x61\x6c\x75\x65\x20\x6d\x61\x70\x43\x68\x6e\x3a\x25\x64\x2e\x20\x28\x41\x74\x74\x65\x6e\x3a\x25\x64\x6d\x2c\x30\x78\x25\x78\x29" "\n"
,mapChn COMMA estResultFact[0]COMMA estResult[0]);}if(bWrComp){page=
PR1000_REG_PAGE_COMMON;i2cReg=0x13+PR1000_OFFSETADDR_CEQ_INFO_CH(prChn);
PR1000_PageWrite(fd,i2cSlaveAddr,page,i2cReg,(uint8_t)estResult[1]);Print(
"\x53\x65\x74\x20\x53\x54\x44\x20\x45\x51\x20\x76\x61\x6c\x75\x65\x20\x6d\x61\x70\x43\x68\x6e\x3a\x25\x64\x2e\x20\x28\x43\x6f\x6d\x70\x3a\x25\x64\x6d\x2c\x30\x78\x25\x78\x29" "\n"
,mapChn COMMA estResultFact[1]COMMA estResult[1]);}Print(
"\x3d\x3e\x20\x53\x54\x44\x20\x45\x51\x20\x76\x61\x6c\x75\x65\x20\x69\x73\x6e\x27\x74\x20\x73\x61\x6d\x65\x20\x77\x69\x74\x68\x20\x70\x72\x65\x76\x69\x6f\x75\x73\x28\x6d\x61\x70\x43\x68\x6e\x3a\x25\x64\x29\x2e\x20\x28\x41\x74\x74\x65\x6e\x3a\x25\x64\x6d\x2c\x30\x78\x25\x78\x29\x2c\x20\x28\x43\x6f\x6d\x70\x3a\x25\x64\x6d\x2c\x30\x78\x25\x78\x29\x20\x72\x65\x74\x72\x79\x3a\x25\x64\x2f\x35" "\n"
,mapChn COMMA estResultFact[0]COMMA estResult[0]COMMA estResultFact[1]COMMA 
estResult[1]COMMA pstCEQData->retryCnt);pstCEQData->estResultAtten=estResult[0]&
0x1f;
pstCEQData->estResultComp=estResult[1]&0x3f;
pstCEQData->attenFact=estResultFact[0];pstCEQData->compFact=estResultFact[1];if(
PR1000_CEQ_SetVADCGain(fd,pstCEQData->format,pstCEQData->camResol,pstPortChSel,
pstCEQData)<0){Error(
"\x53\x65\x74\x20\x56\x41\x44\x43\x20\x47\x61\x69\x6e\x2e\x20\x6d\x61\x70\x43\x68\x6e\x3a\x25\x64" "\n"
,mapChn);break;}if(pstCEQData->retryCnt>=MAX_CHECK_EQ_STABLE_CNT){Error(
"\x4f\x76\x65\x72\x20\x63\x68\x65\x63\x6b\x20\x74\x69\x6d\x65\x2e\x20\x46\x6f\x72\x63\x65\x6c\x79\x20\x62\x79\x70\x61\x73\x73\x20\x45\x51\x2e\x20\x5b\x6d\x61\x70\x43\x68\x6e\x3a\x25\x64\x2c\x20\x65\x73\x74\x53\x74\x65\x70\x3a\x25\x64\x5d" "\n"
,mapChn COMMA pstCEQData->estStep);pstCEQData->retryCnt=0;pstCEQData->
flagStepComplete[PR1000_CEQ_STEP_EQCHECK]=FALSE;PR1000_CEQ_Done(fd,mapChn,(void*
)pHost);break;}}
#else
pstCEQData->retryCnt=0;pstCEQData->flagStepComplete[PR1000_CEQ_STEP_EQCHECK]=
TRUE;PR1000_CEQ_Done(fd,mapChn,(void*)pHost);
#endif 
}break;case PR1000_CEQ_STEP_STDFORMAT_CHECK:{enum _pr1000_table_format format;
enum _pr1000_table_inresol camResol;format=pstCEQData->format;camResol=
pstCEQData->camResol;
Print(
"\x6d\x61\x70\x43\x68\x3a\x25\x64\x20\x74\x69\x6d\x65\x4f\x76\x65\x72\x3a\x25\x64\x2c\x20\x63\x6e\x74\x43\x68\x65\x63\x6b\x3a\x25\x64\x20\x66\x6f\x72\x6d\x61\x74\x3a\x25\x73\x2c\x20\x63\x61\x6d\x52\x65\x73\x6f\x6c\x3a\x25\x73" "\n"
,mapChn COMMA pHost->sysHost.timeOverSTDFORMATCheck[mapChn]COMMA pHost->sysHost.
cntSTDFORMATCheck[mapChn]COMMA _STR_PR1000_FORMAT[pstCEQData->format]COMMA 
_STR_PR1000_INRESOL[pstCEQData->camResol]);if((--pHost->sysHost.
timeOverSTDFORMATCheck[mapChn]<=0)&&(pHost->sysHost.cntSTDFORMATCheck[mapChn]>0)
){Print(
"\x54\x69\x6d\x65\x4f\x76\x65\x72\x20\x53\x54\x44\x46\x4f\x52\x4d\x41\x54\x43\x48\x45\x43\x4b\x28\x6e\x6f\x20\x70\x74\x7a\x64\x61\x74\x61\x29\x2e\x20\x6d\x61\x70\x43\x68\x3a\x25\x64" "\n"
,mapChn);pstCEQData->retryCnt=0;pHost->sysHost.cntSTDFORMATCheck[mapChn]=0;pHost
->sysHost.timeOverSTDFORMATCheck[mapChn]=0;pstCEQData->flagStepComplete[
PR1000_CEQ_STEP_STDFORMAT_CHECK]=FALSE;pstCEQData->estStep=
PR1000_CEQ_STEP_SPECIALSTD_CHECK;pstCEQData->estStdComplete=0;break;}if(pHost->
sysHost.cntSTDFORMATCheck[mapChn]<=0){if(format==pr1000_format_HDT){if((camResol
==pr1000_inresol_1280x720p25)||(camResol==pr1000_inresol_1280x720p30)){if(pHost
->ptzHost.flagSTDFORMATResult[mapChn]==TRUE){pstCEQData->format=
pr1000_format_HDT_NEW;}}Print(
"\x6d\x61\x70\x43\x68\x3a\x25\x64\x20\x74\x69\x6d\x65\x4f\x76\x65\x72\x3a\x25\x64\x2c\x20\x63\x6e\x74\x43\x68\x65\x63\x6b\x3a\x25\x64\x20\x66\x6f\x72\x6d\x61\x74\x3a\x25\x73\x2c\x20\x63\x61\x6d\x52\x65\x73\x6f\x6c\x3a\x25\x73" "\n"
,mapChn COMMA pHost->sysHost.timeOverSTDFORMATCheck[mapChn]COMMA pHost->sysHost.
cntSTDFORMATCheck[mapChn]COMMA _STR_PR1000_FORMAT[pstCEQData->format]COMMA 
_STR_PR1000_INRESOL[pstCEQData->camResol]);pstCEQData->retryCnt=0;pHost->sysHost
.cntSTDFORMATCheck[mapChn]=0;pHost->sysHost.timeOverSTDFORMATCheck[mapChn]=0;
pstCEQData->flagStepComplete[PR1000_CEQ_STEP_STDFORMAT_CHECK]=TRUE;pstCEQData->
estStep=PR1000_CEQ_STEP_EQCHECK;
#ifdef SUPPORT_CABLE_EQ
bWrAtten=TRUE;bWrComp=FALSE;if(PR1000_CEQ_SetEQGain(fd,bWrAtten,bWrComp,
pstPortChSel,pstCEQData)<0){Error(
"\x53\x65\x74\x20\x45\x51\x20\x47\x61\x69\x6e\x2e\x20\x6d\x61\x70\x43\x68\x6e\x3a\x25\x64" "\n"
,mapChn);break;}if(PR1000_CEQ_SetVADCGain(fd,pstCEQData->format,pstCEQData->
camResol,pstPortChSel,pstCEQData)<0){Error(
"\x53\x65\x74\x20\x56\x41\x44\x43\x20\x47\x61\x69\x6e\x2e\x20\x6d\x61\x70\x43\x68\x6e\x3a\x25\x64" "\n"
,mapChn);break;}
#endif 
break;}else if(format==pr1000_format_HDA){if(pHost->ptzHost.flagSTDFORMATResult[
mapChn]==TRUE){pstCEQData->format=pr1000_format_HDA;Print(
"\x53\x65\x74\x20\x6d\x61\x70\x43\x68\x3a\x25\x64\x20\x74\x69\x6d\x65\x4f\x76\x65\x72\x3a\x25\x64\x2c\x20\x63\x6e\x74\x43\x68\x65\x63\x6b\x3a\x25\x64\x20\x66\x6f\x72\x6d\x61\x74\x3a\x25\x73\x2c\x20\x63\x61\x6d\x52\x65\x73\x6f\x6c\x3a\x25\x73" "\n"
,mapChn COMMA pHost->sysHost.timeOverSTDFORMATCheck[mapChn]COMMA pHost->sysHost.
cntSTDFORMATCheck[mapChn]COMMA _STR_PR1000_FORMAT[pstCEQData->format]COMMA 
_STR_PR1000_INRESOL[pstCEQData->camResol]);pstCEQData->retryCnt=0;pHost->sysHost
.cntSTDFORMATCheck[mapChn]=0;pHost->sysHost.timeOverSTDFORMATCheck[mapChn]=0;
pstCEQData->flagStepComplete[PR1000_CEQ_STEP_STDFORMAT_CHECK]=TRUE;pstCEQData->
estStep=PR1000_CEQ_STEP_EQCHECK;break;}}}}break;case 
PR1000_CEQ_STEP_SPECIALSTD_CHECK:{if(PR1000_GetCeqDetInfo(fd,pstPortChSel,
pstCEQData,&stCeqDet,&stCeqLock)<0){ErrorString(
"\x47\x65\x74\x43\x65\x71\x44\x65\x74\x49\x6e\x66\x6f\x2e" "\n");break;}Print(
"\x53\x74\x65\x70\x3a\x25\x64\x20\x72\x65\x67\x5b\x25\x30\x32\x78\x5d\x20\x6c\x6f\x63\x6b\x5b\x68\x70\x6c\x6c\x3a\x25\x64\x2c\x68\x70\x65\x72\x3a\x25\x64\x2c\x73\x74\x64\x3a\x25\x64\x5d\x20\x64\x65\x74\x5b\x63\x68\x72\x6f\x6d\x61\x3a\x25\x64\x5d" "\n"
,pstCEQData->estStep COMMA stCeqLock.reg COMMA stCeqLock.b.lock_hpll COMMA 
stCeqLock.b.lock_hperiod COMMA stCeqLock.b.lock_std COMMA stCeqLock.b.det_chroma
);Print(
"\x6d\x61\x70\x43\x68\x3a\x25\x64\x20\x66\x6f\x72\x6d\x61\x74\x3a\x25\x73\x2c\x20\x63\x61\x6d\x52\x65\x73\x6f\x6c\x3a\x25\x73" "\n"
,mapChn COMMA _STR_PR1000_FORMAT[pstCEQData->format]COMMA _STR_PR1000_INRESOL[
pstCEQData->camResol]);if(stCeqLock.b.det_chroma){Print(
"\x53\x74\x61\x72\x74\x20\x73\x65\x61\x72\x63\x68\x20\x63\x68\x72\x6f\x6d\x61\x20\x6c\x6f\x63\x6b\x2e\x20\x6d\x61\x70\x43\x68\x3a\x25\x64" "\n"
,mapChn);pstCEQData->retryCnt=0;pHost->sysHost.timeOverSTDFORMATCheck[mapChn]=(2
*MAX_CNT_CHROMALOCK)+1;pstCEQData->flagStepComplete[
PR1000_CEQ_STEP_SPECIALSTD_CHECK]=TRUE;pstCEQData->estStep=
PR1000_CEQ_STEP_WAIT_CHROMALOCK1;pstCEQData->flagStepComplete[
PR1000_CEQ_STEP_CHROMALOCK]=FALSE;_CLEAR_BIT(mapChn,&pHost->sysHost.
bitChnDoneChromaLock);_CLEAR_BIT(mapChn,&pHost->sysHost.bitChnResultChromaLock);
_SET_BIT(mapChn,&pHost->sysHost.bitChnCheckChromaLock);}else{pstCEQData->
retryCnt++;if(pstCEQData->retryCnt>=MAX_CHECK_EQ_STABLE_CNT){Print(
"\x66\x6f\x72\x63\x65\x6c\x79\x20\x73\x65\x74\x20\x61\x62\x6e\x6f\x72\x6d\x61\x6c\x20\x74\x6f\x20\x43\x56\x49\x20\x66\x6f\x72\x6d\x61\x74\x20\x6c\x61\x73\x74\x2e\x20\x6d\x61\x70\x43\x68\x3a\x25\x64" "\n"
,mapChn);pstCEQData->bForceChgStd=TRUE;
pstCEQData->format=pr1000_format_CVI;PR1000_CEQ_SetVIDStd(fd,pstPortChSel,
pstCEQData->format,pstCEQData->camResol);pstCEQData->retryCnt=0;pHost->sysHost.
cntSTDFORMATCheck[mapChn]=0;pHost->sysHost.timeOverSTDFORMATCheck[mapChn]=0;
pstCEQData->flagStepComplete[PR1000_CEQ_STEP_SPECIALSTD_CHECK]=FALSE;pstCEQData
->estStep=PR1000_CEQ_STEP_EQCHECK;
#ifdef SUPPORT_CABLE_EQ
bWrAtten=TRUE;bWrComp=FALSE;if(PR1000_CEQ_SetEQGain(fd,bWrAtten,bWrComp,
pstPortChSel,pstCEQData)<0){Error(
"\x53\x65\x74\x20\x45\x51\x20\x47\x61\x69\x6e\x2e\x20\x6d\x61\x70\x43\x68\x6e\x3a\x25\x64" "\n"
,mapChn);break;}if(PR1000_CEQ_SetVADCGain(fd,pstCEQData->format,pstCEQData->
camResol,pstPortChSel,pstCEQData)<0){Error(
"\x53\x65\x74\x20\x56\x41\x44\x43\x20\x47\x61\x69\x6e\x2e\x20\x6d\x61\x70\x43\x68\x6e\x3a\x25\x64" "\n"
,mapChn);break;}
#endif 
break;}}}break;case PR1000_CEQ_STEP_WAIT_CHROMALOCK1:{Print(
"\x6d\x61\x70\x43\x68\x3a\x25\x64\x20\x74\x69\x6d\x65\x4f\x76\x65\x72\x3a\x25\x64\x2c\x20\x6c\x6f\x63\x6b\x3a\x25\x64\x2c\x20\x66\x6f\x72\x6d\x61\x74\x3a\x25\x73\x2c\x20\x63\x61\x6d\x52\x65\x73\x6f\x6c\x3a\x25\x73" "\n"
,mapChn COMMA pHost->sysHost.timeOverSTDFORMATCheck[mapChn]COMMA pstCEQData->
flagStepComplete[PR1000_CEQ_STEP_CHROMALOCK]COMMA _STR_PR1000_FORMAT[pstCEQData
->format]COMMA _STR_PR1000_INRESOL[pstCEQData->camResol]);
if(pHost->sysHost.timeOverSTDFORMATCheck[mapChn]-- >0){if((_TEST_BIT(mapChn,&
pHost->sysHost.bitChnDoneChromaLock))&&(_TEST_BIT(mapChn,&pHost->sysHost.
bitChnResultChromaLock))){pstCEQData->flagStepComplete[
PR1000_CEQ_STEP_CHROMALOCK]=TRUE;pstCEQData->retryCnt=0;pHost->sysHost.
cntSTDFORMATCheck[mapChn]=0;pHost->sysHost.timeOverSTDFORMATCheck[mapChn]=0;
pstCEQData->flagStepComplete[PR1000_CEQ_STEP_WAIT_CHROMALOCK1]=TRUE;pstCEQData->
estStep=PR1000_CEQ_STEP_EQCHECK;pstCEQData->eqProcFlag.C_LOCK_CNT=1;
#ifdef SUPPORT_CABLE_EQ
bWrAtten=TRUE;bWrComp=FALSE;if(PR1000_CEQ_SetEQGain(fd,bWrAtten,bWrComp,
pstPortChSel,pstCEQData)<0){Error(
"\x53\x65\x74\x20\x45\x51\x20\x47\x61\x69\x6e\x2e\x20\x6d\x61\x70\x43\x68\x6e\x3a\x25\x64" "\n"
,mapChn);break;}if(PR1000_CEQ_SetVADCGain(fd,pstCEQData->format,pstCEQData->
camResol,pstPortChSel,pstCEQData)<0){Error(
"\x53\x65\x74\x20\x56\x41\x44\x43\x20\x47\x61\x69\x6e\x2e\x20\x6d\x61\x70\x43\x68\x6e\x3a\x25\x64" "\n"
,mapChn);break;}
#endif 
}}else{Print(
"\x66\x6f\x72\x63\x65\x6c\x79\x20\x73\x65\x74\x20\x61\x62\x6e\x6f\x72\x6d\x61\x6c\x20\x74\x6f\x20\x43\x56\x49\x20\x66\x6f\x72\x6d\x61\x74\x20\x6c\x61\x73\x74\x2e\x20\x6d\x61\x70\x43\x68\x3a\x25\x64" "\n"
,mapChn);pstCEQData->bForceChgStd=TRUE;
pstCEQData->format=pr1000_format_CVI;PR1000_CEQ_SetVIDStd(fd,pstPortChSel,
pstCEQData->format,pstCEQData->camResol);Print(
"\x53\x74\x61\x72\x74\x20\x73\x65\x61\x72\x63\x68\x20\x63\x68\x72\x6f\x6d\x61\x20\x6c\x6f\x63\x6b\x2e\x20\x6d\x61\x70\x43\x68\x3a\x25\x64" "\n"
,mapChn);pstCEQData->retryCnt=0;pHost->sysHost.timeOverSTDFORMATCheck[mapChn]=(2
*MAX_CNT_CHROMALOCK)+1;pstCEQData->flagStepComplete[
PR1000_CEQ_STEP_WAIT_CHROMALOCK1]=FALSE;pstCEQData->estStep=
PR1000_CEQ_STEP_WAIT_CHROMALOCK2;pstCEQData->flagStepComplete[
PR1000_CEQ_STEP_CHROMALOCK]=FALSE;_CLEAR_BIT(mapChn,&pHost->sysHost.
bitChnDoneChromaLock);_CLEAR_BIT(mapChn,&pHost->sysHost.bitChnResultChromaLock);
_SET_BIT(mapChn,&pHost->sysHost.bitChnCheckChromaLock);}}break;case 
PR1000_CEQ_STEP_WAIT_CHROMALOCK2:{Print(
"\x6d\x61\x70\x43\x68\x3a\x25\x64\x20\x74\x69\x6d\x65\x4f\x76\x65\x72\x3a\x25\x64\x2c\x20\x6c\x6f\x63\x6b\x3a\x25\x64\x2c\x20\x66\x6f\x72\x6d\x61\x74\x3a\x25\x73\x2c\x20\x63\x61\x6d\x52\x65\x73\x6f\x6c\x3a\x25\x73" "\n"
,mapChn COMMA pHost->sysHost.timeOverSTDFORMATCheck[mapChn]COMMA pstCEQData->
flagStepComplete[PR1000_CEQ_STEP_CHROMALOCK]COMMA _STR_PR1000_FORMAT[pstCEQData
->format]COMMA _STR_PR1000_INRESOL[pstCEQData->camResol]);
if(pHost->sysHost.timeOverSTDFORMATCheck[mapChn]-- >0){if((_TEST_BIT(mapChn,&
pHost->sysHost.bitChnDoneChromaLock))&&(_TEST_BIT(mapChn,&pHost->sysHost.
bitChnResultChromaLock))){pstCEQData->retryCnt=0;pHost->sysHost.
cntSTDFORMATCheck[mapChn]=0;pHost->sysHost.timeOverSTDFORMATCheck[mapChn]=0;
pstCEQData->flagStepComplete[PR1000_CEQ_STEP_WAIT_CHROMALOCK2]=TRUE;pstCEQData->
estStep=PR1000_CEQ_STEP_EQCHECK;pstCEQData->eqProcFlag.C_LOCK_CNT=1;
#ifdef SUPPORT_CABLE_EQ
bWrAtten=TRUE;bWrComp=FALSE;if(PR1000_CEQ_SetEQGain(fd,bWrAtten,bWrComp,
pstPortChSel,pstCEQData)<0){Error(
"\x53\x65\x74\x20\x45\x51\x20\x47\x61\x69\x6e\x2e\x20\x6d\x61\x70\x43\x68\x6e\x3a\x25\x64" "\n"
,mapChn);break;}if(PR1000_CEQ_SetVADCGain(fd,pstCEQData->format,pstCEQData->
camResol,pstPortChSel,pstCEQData)<0){Error(
"\x53\x65\x74\x20\x56\x41\x44\x43\x20\x47\x61\x69\x6e\x2e\x20\x6d\x61\x70\x43\x68\x6e\x3a\x25\x64" "\n"
,mapChn);break;}
#endif 
}}else{Print(
"\x66\x6f\x72\x63\x65\x6c\x79\x20\x73\x65\x74\x20\x61\x62\x6e\x6f\x72\x6d\x61\x6c\x20\x74\x6f\x20\x43\x56\x49\x20\x66\x6f\x72\x6d\x61\x74\x20\x6c\x61\x73\x74\x2e\x20\x6d\x61\x70\x43\x68\x3a\x25\x64" "\n"
,mapChn);pstCEQData->bForceChgStd=TRUE;
pstCEQData->format=pr1000_format_CVI;PR1000_CEQ_SetVIDStd(fd,pstPortChSel,
pstCEQData->format,pstCEQData->camResol);pstCEQData->retryCnt=0;pHost->sysHost.
cntSTDFORMATCheck[mapChn]=0;pHost->sysHost.timeOverSTDFORMATCheck[mapChn]=0;
pstCEQData->flagStepComplete[PR1000_CEQ_STEP_WAIT_CHROMALOCK2]=FALSE;pstCEQData
->estStep=PR1000_CEQ_STEP_EQCHECK;
#ifdef SUPPORT_CABLE_EQ
bWrAtten=TRUE;bWrComp=FALSE;if(PR1000_CEQ_SetEQGain(fd,bWrAtten,bWrComp,
pstPortChSel,pstCEQData)<0){Error(
"\x53\x65\x74\x20\x45\x51\x20\x47\x61\x69\x6e\x2e\x20\x6d\x61\x70\x43\x68\x6e\x3a\x25\x64" "\n"
,mapChn);break;}if(PR1000_CEQ_SetVADCGain(fd,pstCEQData->format,pstCEQData->
camResol,pstPortChSel,pstCEQData)<0){Error(
"\x53\x65\x74\x20\x56\x41\x44\x43\x20\x47\x61\x69\x6e\x2e\x20\x6d\x61\x70\x43\x68\x6e\x3a\x25\x64" "\n"
,mapChn);break;}
#endif 
}}break;case PR1000_CEQ_STEP_EQSTDDONE:{PR1000_CEQ_Done(fd,mapChn,(void*)pHost);
pstCEQData->retryCnt=0;pstCEQData->estStep=PR1000_CEQ_STEP_ESTSTART;pstCEQData->
flagStepComplete[PR1000_CEQ_STEP_EQSTDDONE]=TRUE;}break;case 
PR1000_CEQ_STEP_EQTIMEOUT:{pstCEQData->retryCnt=0;pstCEQData->estStep=
PR1000_CEQ_STEP_ESTSTART;
pstCEQData->flagStepComplete[PR1000_CEQ_STEP_EQTIMEOUT]=TRUE;}break;default:
break;}ret=pstCEQData->estStep;return(ret);}int PR1000_CEQ_Stop(const int fd,
const int mapChn,void*pArgu){int ret=-1;uint8_t i2cReg=0;uint8_t i2cData=0;
uint8_t i2cMask=0;uint8_t page;_stIRQReg stIRQEn;uint8_t regEQ[2]={0,};uint8_t 
prChip,prChn,i2cSlaveAddr;_drvHost*pHost=(_drvHost*)pArgu;_stPortChSel*
pstPortChSel=(_stPortChSel*)&pHost->sysHost.portChSel[mapChn];_stCEQData*
pstCEQData=(_stCEQData*)&pHost->sysHost.stCEQDataList[mapChn];if((pstPortChSel==
NULL)||!ASSERT_VALID_CH(pstPortChSel->i2cSlvAddr)){ErrorString(
"\x49\x6e\x76\x61\x6c\x69\x64\x20\x63\x68\x2e" "\n");return(-1);}prChip=
pstPortChSel->prChip;prChn=pstPortChSel->prChn;i2cSlaveAddr=pstPortChSel->
i2cSlvAddr;pHost->sysHost.cntSTDFORMATCheck[mapChn]=0;pHost->sysHost.
timeOverSTDFORMATCheck[mapChn]=0;if(pstCEQData->bEnable){Dbg(
"\x53\x74\x6f\x70\x20\x43\x45\x51\x20\x5b\x6d\x61\x70\x43\x68\x6e\x3a\x25\x64\x5d" "\n"
,mapChn);if(pstCEQData->bLock){if((pstCEQData->stManEQMan.b.man_ifmt_res!=
PR1000_DET_IFMT_RES_480i)&&(pstCEQData->stManEQMan.b.man_ifmt_res!=
PR1000_DET_IFMT_RES_576i)){if(pstCEQData->stManEQMan.b.man_ifmt_std==
PR1000_DET_IFMT_STD_HDT){page=PR1000_VDEC_PAGE(prChn);i2cReg=0x0c+
PR1000_OFFSETADDR_VDEC_1_2PAGE_CH(prChn);i2cMask=0x40;i2cData=0x00;DbgString(
"\x48\x44\x54\x20\x63\x61\x73\x65\x2c\x20\x73\x70\x65\x63\x69\x66\x69\x63\x20\x72\x65\x67\x69\x73\x74\x65\x72" "\n"
);Dbg(
"\x57\x72\x69\x74\x65\x20\x5b\x70\x25\x64\x2c\x20\x30\x78\x25\x30\x32\x78\x2d\x30\x78\x25\x30\x32\x78\x28\x6d\x61\x73\x6b\x3a\x25\x30\x32\x78\x29\x5d" "\n"
,page COMMA i2cReg COMMA i2cData COMMA i2cMask);if((ret=PR1000_WriteMaskBit(fd,
i2cSlaveAddr,page,i2cReg,i2cMask,i2cData))<0){ErrorString(
"\x57\x72\x69\x74\x65\x20\x72\x65\x67\x2e" "\n");return(-1);}}}}
#ifdef SUPPORT_HIDE_EQING_DISPLAY 
PrintString(
"\x48\x69\x64\x65\x20\x45\x51\x69\x6e\x67\x20\x64\x69\x73\x70\x6c\x61\x79" "\n")
;PR1000_VID_HideEQingDisplay(fd,pstPortChSel);
#endif 
DbgString("\x43\x45\x51\x5f\x53\x74\x6f\x70\x2e\x20\x69\x6e\x69\x74" "\n");
memset(pstCEQData,0,sizeof(_stCEQData));pHost->sysHost.cntChromaLockTunn[mapChn]
=0;page=PR1000_REG_PAGE_COMMON;i2cReg=0x90;i2cMask=0x01<<prChn;i2cData=(
WAIT_NEXT_NOVIDEO_FALSE)<<prChn;
Dbg(
"\x4e\x6f\x76\x69\x64\x65\x6f\x20\x43\x75\x72\x72\x65\x6e\x74\x3a\x25\x64\x2c\x20\x4c\x65\x76\x65\x6c\x20\x70\x72\x43\x68\x6e\x3a\x25\x64\x20\x64\x61\x74\x61\x3a\x30\x78\x25\x30\x32\x78" "\n"
,i2cData COMMA prChn COMMA i2cData);Dbg(
"\x57\x72\x20\x5b\x70\x25\x64\x2c\x20\x30\x78\x25\x30\x32\x78\x2d\x30\x78\x25\x30\x32\x78\x5d" "\n"
,page COMMA i2cReg COMMA i2cData);if((ret=PR1000_WriteMaskBit(fd,i2cSlaveAddr,
page,i2cReg,i2cMask,i2cData))<0){ErrorString(
"\x57\x72\x69\x74\x65\x20\x72\x65\x67\x2e" "\n");}
#ifdef SUPPORT_CABLE_EQ

page=PR1000_REG_PAGE_COMMON;i2cReg=0x12+PR1000_OFFSETADDR_CEQ_INFO_CH(prChn);
regEQ[0]=0x20;regEQ[1]=0x00;if(PR1000_PageWriteBurst(fd,i2cSlaveAddr,page,i2cReg
,sizeof(regEQ),(uint8_t*)&regEQ)<0){ErrorString(
"\x50\x61\x67\x65\x57\x72\x69\x74\x65\x42\x75\x72\x73\x74\x2e" "\n");return(-1);
}
i2cReg=0x11+PR1000_OFFSETADDR_CEQ_INFO_CH(prChn);i2cMask=0x70;i2cData=0;if((
PR1000_WriteMaskBit(fd,i2cSlaveAddr,PR1000_REG_PAGE_COMMON,i2cReg,i2cMask,
i2cData))<0){ErrorString("\x57\x72\x69\x74\x65\x20\x72\x65\x67\x2e" "\n");return
(-1);}
#else

page=PR1000_REG_PAGE_COMMON;i2cReg=0x12+PR1000_OFFSETADDR_CEQ_INFO_CH(prChn);
regEQ[0]=0x00;regEQ[1]=0x00;if(PR1000_PageWriteBurst(fd,i2cSlaveAddr,page,i2cReg
,sizeof(regEQ),(uint8_t*)&regEQ)<0){ErrorString(
"\x50\x61\x67\x65\x57\x72\x69\x74\x65\x42\x75\x72\x73\x74\x2e" "\n");return(-1);
}
#endif 
Dbg(
"\x64\x69\x73\x61\x62\x6c\x65\x20\x69\x6e\x74\x65\x72\x72\x75\x70\x74\x20\x61\x6c\x6c\x2e\x20\x65\x78\x63\x65\x70\x74\x20\x6e\x6f\x76\x69\x64\x65\x6f\x20\x26\x20\x47\x50\x49\x4f\x30\x2e\x20\x70\x72\x43\x68\x6e\x28\x25\x64\x29" "\n"
,prChn);{page=PR1000_REG_PAGE_COMMON;i2cReg=0xa0;if(PR1000_PageReadBurst(fd,
i2cSlaveAddr,page,i2cReg,sizeof(stIRQEn),(uint8_t*)&stIRQEn)<0){ErrorString(
"\x50\x61\x67\x65\x52\x65\x61\x64\x42\x75\x72\x73\x74\x2e" "\n");}stIRQEn.u8PTZ[
prChn]=0x0;stIRQEn.u8NOVID&=~(1<<(prChn+4));stIRQEn.u8NOVID|=(1<<prChn);
stIRQEn.u8MD&=~((1<<(prChn+4))|(1<<prChn));stIRQEn.u8ND&=~((1<<(prChn+4))|(1<<
prChn));stIRQEn.u8DFD&=~((1<<(prChn+4))|(1<<prChn));stIRQEn.u8AD&=~((1<<(prChn+4
))|(1<<prChn));
stIRQEn.u8GPIO1_5[0]=0x0;stIRQEn.u8GPIO1_5[1]=0x0;stIRQEn.u8GPIO1_5[2]=0x0;
stIRQEn.u8GPIO1_5[3]=0x0;stIRQEn.u8GPIO1_5[4]=0x0;if(prChip==PR1000_MASTER)
stIRQEn.u8GPIO1_5[4]|=0x80;
if(PR1000_PageWriteBurst(fd,i2cSlaveAddr,page,i2cReg,sizeof(stIRQEn),(uint8_t*)&
stIRQEn)<0){ErrorString(
"\x50\x61\x67\x65\x57\x72\x69\x74\x65\x42\x75\x72\x73\x74\x2e" "\n");}}}return(
ret);}int PR1000_CEQ_Done(const int fd,const int mapChn,void*pArgu){_drvHost*
pHost=(_drvHost*)pArgu;int ret=-1;uint8_t i2cReg=0;uint8_t i2cData=0;uint8_t 
i2cMask=0;uint8_t page;uint8_t prChip,prChn,i2cSlaveAddr;uint8_t curNovidStatus;
_stIRQReg stIRQEn;_stPortChSel*pstPortChSel=NULL;_stCEQData*pstCEQData=NULL;if((
pHost==NULL)){ErrorString(
"\x49\x6e\x76\x61\x6c\x69\x64\x20\x61\x72\x67\x75" "\n");return(-1);}if((
pstPortChSel=(_stPortChSel*)&pHost->sysHost.portChSel[mapChn])==NULL){
ErrorString("\x49\x6e\x76\x61\x6c\x69\x64\x20\x61\x72\x67\x75" "\n");return(-1);
}prChip=pstPortChSel->prChip;prChn=pstPortChSel->prChn;i2cSlaveAddr=pstPortChSel
->i2cSlvAddr;if((pstCEQData=(_stCEQData*)&pHost->sysHost.stCEQDataList[mapChn])
==NULL){ErrorString("\x49\x6e\x76\x61\x6c\x69\x64\x20\x61\x72\x67\x75" "\n");
return(-1);}page=PR1000_REG_PAGE_COMMON;i2cReg=0x90;i2cMask=0x01<<prChn;
PR1000_VEVENT_GetNovidStatus(fd,pstPortChSel,&curNovidStatus);if(curNovidStatus)
{i2cData=(WAIT_NEXT_NOVIDEO_FALSE)<<prChn;
}else{i2cData=(WAIT_NEXT_NOVIDEO_TRUE)<<prChn;
}Dbg(
"\x4e\x6f\x76\x69\x64\x65\x6f\x20\x4c\x65\x76\x65\x6c\x20\x70\x72\x43\x68\x6e\x3a\x25\x64\x20\x63\x68\x61\x6e\x67\x65\x28\x30\x78\x25\x78\x29" "\n"
,prChn COMMA i2cMask&i2cData);Dbg(
"\x57\x72\x20\x5b\x70\x25\x64\x2c\x20\x30\x78\x25\x30\x32\x78\x2d\x30\x78\x25\x30\x32\x78\x5d" "\n"
,page COMMA i2cReg COMMA i2cData);if((ret=PR1000_WriteMaskBit(fd,i2cSlaveAddr,
page,i2cReg,i2cMask,i2cData))<0){ErrorString(
"\x57\x72\x69\x74\x65\x20\x72\x65\x67\x2e" "\n");}Dbg(
"\x52\x65\x73\x74\x6f\x72\x65\x20\x69\x6e\x74\x65\x72\x72\x75\x70\x74\x20\x70\x72\x43\x68\x6e\x28\x25\x64\x29\x2e" "\n"
,prChn);{page=PR1000_REG_PAGE_COMMON;i2cReg=0xa0;if(PR1000_PageReadBurst(fd,
i2cSlaveAddr,page,i2cReg,sizeof(stIRQEn),(uint8_t*)&stIRQEn)<0){ErrorString(
"\x50\x61\x67\x65\x52\x65\x61\x64\x42\x75\x72\x73\x74\x2e" "\n");}stIRQEn.u8PTZ[
prChn]=0xbf;stIRQEn.u8NOVID|=(1<<(prChn+4));if(((pstCEQData->format==
pr1000_format_HDT)||(pstCEQData->format==pr1000_format_HDT_NEW))&&((pstCEQData->
camResol==pr1000_inresol_1280x720p25)||(pstCEQData->camResol==
pr1000_inresol_1280x720p30))){Print(
"\x6d\x61\x70\x43\x68\x6e\x3a\x25\x64\x2c\x20\x64\x6f\x6e\x27\x74\x20\x75\x73\x65\x20\x76\x66\x64\x20\x77\x68\x65\x6e\x20\x68\x64\x74\x20\x37\x32\x30\x70\x32\x35\x2c\x37\x32\x30\x70\x33\x30\x2e" "\n"
,mapChn);stIRQEn.u8NOVID&=~(1<<(prChn+4));
}if(((pstCEQData->bForceChgStd==TRUE)&&(pstCEQData->format==pr1000_format_CVI)))
{Print(
"\x6d\x61\x70\x43\x68\x6e\x3a\x25\x64\x2c\x20\x64\x6f\x6e\x27\x74\x20\x75\x73\x65\x20\x76\x66\x64\x20\x77\x68\x65\x6e\x20\x66\x6f\x72\x63\x65\x6c\x79\x20\x73\x65\x74\x20\x63\x76\x69\x2e" "\n"
,mapChn);stIRQEn.u8NOVID&=~(1<<(prChn+4));
}stIRQEn.u8NOVID|=(1<<prChn);
stIRQEn.u8MD|=((1<<(prChn+4))|(1<<prChn));stIRQEn.u8ND|=((1<<(prChn+4))|(1<<
prChn));stIRQEn.u8DFD|=((1<<(prChn+4))|(1<<prChn));stIRQEn.u8AD|=((1<<(prChn+4))
|(1<<prChn));stIRQEn.u8GPIO0&=~(1<<(prChn));
Print(
"\x53\x74\x6f\x70\x20\x47\x50\x49\x4f\x30\x20\x66\x6f\x72\x20\x45\x51\x20\x69\x6e\x74\x65\x72\x72\x75\x70\x74\x20\x70\x72\x43\x68\x6e\x3a\x25\x64" "\n"
,prChn);stIRQEn.u8GPIO1_5[0]=0x0;stIRQEn.u8GPIO1_5[1]=0x0;stIRQEn.u8GPIO1_5[2]=
0x0;stIRQEn.u8GPIO1_5[3]=0x0;stIRQEn.u8GPIO1_5[4]=0x0;if(prChip==PR1000_MASTER)
stIRQEn.u8GPIO1_5[4]|=0x80;
if(PR1000_PageWriteBurst(fd,i2cSlaveAddr,page,i2cReg,sizeof(stIRQEn),(uint8_t*)&
stIRQEn)<0){ErrorString(
"\x50\x61\x67\x65\x57\x72\x69\x74\x65\x42\x75\x72\x73\x74\x2e" "\n");}}
pstCEQData->retryCnt=0;pHost->sysHost.cntSTDFORMATCheck[mapChn]=0;pHost->sysHost
.timeOverSTDFORMATCheck[mapChn]=0;pstCEQData->flagStepComplete[
PR1000_CEQ_STEP_EQSTDDONE]=TRUE;pstCEQData->estStep=PR1000_CEQ_STEP_ESTSTART;
pHost->eventHost.stEventDetStd[mapChn].mapChn=mapChn;pHost->eventHost.
stEventDetStd[mapChn].prChn=prChn;pHost->eventHost.stEventDetStd[mapChn].format=
pstCEQData->format;pHost->eventHost.stEventDetStd[mapChn].resol=pstCEQData->
camResol;_SET_BIT(mapChn,&pHost->wqPollChnStatus.bitWqPollDetStd);
WAKE_UP_INTERRUPTIBLE(&pHost->wqPoll);Dbg(
"\x6d\x61\x70\x43\x68\x6e\x3a\x25\x64\x20\x70\x6f\x6c\x6c\x44\x65\x74\x53\x74\x64\x3a\x25\x6c\x78" "\n"
,mapChn COMMA pHost->wqPollChnStatus.bitWqPollDetStd);ret=0;return(ret);}int 
PR1000_CEQ_GetInfoRegs(const int fd,const _stPortChSel*pstPortChSel,
_stCeqInfoReg*pstCeqInfoReg){int ret=-1;uint8_t i2cReg=0;int page;int mapChn;
uint8_t prChip,prChn,i2cSlaveAddr;if((pstPortChSel==NULL)||!ASSERT_VALID_CH(
pstPortChSel->i2cSlvAddr)){ErrorString(
"\x49\x6e\x76\x61\x6c\x69\x64\x20\x63\x68\x2e" "\n");return(-1);}mapChn=
pstPortChSel->chn;prChip=pstPortChSel->prChip;prChn=pstPortChSel->prChn;
i2cSlaveAddr=pstPortChSel->i2cSlvAddr;if(pstCeqInfoReg==NULL){ErrorString(
"\x49\x6e\x76\x61\x6c\x69\x64\x20\x61\x72\x67\x75" "\n");return(-1);}page=
PR1000_REG_PAGE_COMMON;i2cReg=0x04+PR1000_OFFSETADDR_CEQ_INFO_CH(prChn);Dbg(
"\x43\x45\x51\x20\x47\x65\x74\x49\x6e\x66\x6f\x52\x65\x67\x73\x20\x5b\x6d\x61\x70\x43\x68\x6e\x3a\x25\x64\x2c\x20\x25\x64\x70\x20\x72\x65\x67\x3a\x30\x78\x25\x30\x32\x78\x5d" "\n"
,mapChn COMMA page COMMA i2cReg);if((ret=PR1000_PageReadBurst(fd,i2cSlaveAddr,
page,i2cReg,sizeof(_stCeqInfoReg),(uint8_t*)pstCeqInfoReg))<0){ErrorString(
"\x52\x65\x61\x64\x20\x72\x65\x67\x2e" "\n");return(-1);}return(ret);}int 
PR1000_CEQ_GetEQEstFactor(const int fd,const enum _pr1000_table_format format,
const enum _pr1000_table_inresol camResol,const _stCeqInfoReg*pstCeqInfoReg,
uint8_t*pu8ResultInfo,uint16_t*pu16ResultFact){int ret=-1;uint16_t u16Comp1,
u16Comp2;uint16_t u16NewComp=0;uint16_t u16NewAtten=0;_PR1000_CEQ_TABLE_EST_SD*
pTableSD=NULL;_PR1000_CEQ_TABLE_EST_HD*pTableHD=NULL;uint16_t distance=0;
uint16_t resultDistance[2]={0,};uint16_t rangeMin,rangeMax,factor;uint8_t bFind[
2]={0,};if((pu8ResultInfo==NULL)||(pu16ResultFact==NULL)){ErrorString(
"\x49\x6e\x76\x61\x6c\x69\x64\x20\x61\x72\x67\x75\x2e" "\n");return(-1);}
u16Comp1=(pstCeqInfoReg->b.det_eq_comp1_h<<8)|(pstCeqInfoReg->b.det_eq_comp1_l);
u16Comp2=(pstCeqInfoReg->b.det_eq_comp2_h<<8)|(pstCeqInfoReg->b.det_eq_comp2_l);
if(u16Comp2!=0){u16NewComp=PR1000_CEQ_EST_COMP(u16Comp1,u16Comp2);}else{
u16NewComp=0;return(0);}u16NewAtten=(pstCeqInfoReg->b.det_eq_atten2_h<<8)|(
pstCeqInfoReg->b.det_eq_atten2_l);Dbg(
"\x4e\x65\x77\x43\x6f\x6d\x70\x3a\x30\x78\x25\x30\x34\x78\x2c\x20\x4e\x65\x77\x41\x74\x74\x65\x6e\x3a\x30\x78\x25\x30\x34\x78" "\n"
,u16NewComp COMMA u16NewAtten);if((format==pr1000_format_SD720)||(format==
pr1000_format_SD960))
{if(camResol>pr1000_inresol_pal){Error(
"\x49\x6e\x76\x61\x6c\x69\x64\x20\x73\x64\x20\x63\x61\x6d\x52\x65\x73\x6f\x6c\x28\x25\x64\x29\x2e" "\n"
,camResol);return(-1);}pTableSD=(_PR1000_CEQ_TABLE_EST_SD*)
pr1000_ceq_table_estComplex_SD;bFind[1]=0;while(pTableSD->distance!=0xffff){
distance=pTableSD->distance;rangeMin=pTableSD->pData[camResol][0];rangeMax=
pTableSD->pData[camResol][1];factor=pTableSD->pData[camResol][2];if((rangeMin<=
u16NewComp)&&(u16NewComp<=rangeMax)){pu8ResultInfo[1]=factor&0xff;
resultDistance[1]=distance;bFind[1]=1;if(distance>1200){pu8ResultInfo[0]=(factor
>>8)&0xff;
resultDistance[0]=distance;bFind[0]=1;}break;}pTableSD++;}if(bFind[1]==0)
ErrorString("\x43\x61\x6e\x27\x74\x20\x66\x69\x6e\x64\x20\x65\x73\x74\x31" "\n")
;if(bFind[1]&&(distance<=1200)){pTableSD=(_PR1000_CEQ_TABLE_EST_SD*)
pr1000_ceq_table_estAtten2_SD;bFind[0]=0;while(pTableSD->distance!=0xffff){
distance=pTableSD->distance;rangeMin=pTableSD->pData[camResol][0];rangeMax=
pTableSD->pData[camResol][1];factor=pTableSD->pData[camResol][2];if((rangeMin<=
u16NewAtten)&&(u16NewAtten<=rangeMax)){pu8ResultInfo[0]=factor&0xff;
resultDistance[0]=distance;bFind[0]=1;break;}pTableSD++;}if(bFind[0]==0)
ErrorString("\x43\x61\x6e\x27\x74\x20\x66\x69\x6e\x64\x20\x65\x73\x74\x32" "\n")
;}}else if(
#ifndef DONT_SUPPORT_STD_PVI
(format==pr1000_format_PVI)||
#endif 
(format==pr1000_format_HDA)||(format==pr1000_format_CVI)||(format==
pr1000_format_HDT)||(format==pr1000_format_HDT_NEW))
{if((camResol<pr1000_inresol_1280x720p60)){Error(
"\x49\x6e\x76\x61\x6c\x69\x64\x20\x68\x64\x20\x72\x65\x73\x6f\x6c\x28\x25\x64\x29\x2e" "\n"
,camResol);return(-1);}switch(format){
#ifndef DONT_SUPPORT_STD_PVI
case pr1000_format_PVI:{pTableHD=(_PR1000_CEQ_TABLE_EST_HD*)
pr1000_ceq_table_estComplex_PVI;}break;
#endif 
case pr1000_format_HDA:{pTableHD=(_PR1000_CEQ_TABLE_EST_HD*)
pr1000_ceq_table_estComplex_HDA;}break;case pr1000_format_CVI:{pTableHD=(
_PR1000_CEQ_TABLE_EST_HD*)pr1000_ceq_table_estComplex_CVI;}break;case 
pr1000_format_HDT:{pTableHD=(_PR1000_CEQ_TABLE_EST_HD*)
pr1000_ceq_table_estComplex_HDT;}break;case pr1000_format_HDT_NEW:{pTableHD=(
_PR1000_CEQ_TABLE_EST_HD*)pr1000_ceq_table_estComplex_HDT;}break;default:{
pTableHD=(_PR1000_CEQ_TABLE_EST_HD*)pr1000_ceq_table_estComplex_CVI;}break;}
bFind[1]=0;while(pTableHD->distance!=0xffff){distance=pTableHD->distance;
rangeMin=pTableHD->pData[camResol-pr1000_inresol_1280x720p60][0];rangeMax=
pTableHD->pData[camResol-pr1000_inresol_1280x720p60][1];factor=pTableHD->pData[
camResol-pr1000_inresol_1280x720p60][2];if((rangeMin<=u16NewComp)&&(u16NewComp<=
rangeMax)){pu8ResultInfo[1]=factor&0xff;
resultDistance[1]=distance;bFind[1]=1;if(distance>1200){pu8ResultInfo[0]=(factor
>>8)&0xff;
resultDistance[0]=distance;bFind[0]=1;}break;}pTableHD++;}if(bFind[1]==0)
ErrorString("\x43\x61\x6e\x27\x74\x20\x66\x69\x6e\x64\x20\x65\x73\x74\x31" "\n")
;if(bFind[1]&&(distance<=1200)){switch(format){
#ifndef DONT_SUPPORT_STD_PVI
case pr1000_format_PVI:{pTableHD=(_PR1000_CEQ_TABLE_EST_HD*)
pr1000_ceq_table_estAtten2_PVI;}break;
#endif 
case pr1000_format_HDA:{pTableHD=(_PR1000_CEQ_TABLE_EST_HD*)
pr1000_ceq_table_estAtten2_HDA;}break;case pr1000_format_CVI:{pTableHD=(
_PR1000_CEQ_TABLE_EST_HD*)pr1000_ceq_table_estAtten2_CVI;}break;case 
pr1000_format_HDT:{pTableHD=(_PR1000_CEQ_TABLE_EST_HD*)
pr1000_ceq_table_estAtten2_HDT;}break;case pr1000_format_HDT_NEW:{pTableHD=(
_PR1000_CEQ_TABLE_EST_HD*)pr1000_ceq_table_estAtten2_HDT;}break;default:{
pTableHD=(_PR1000_CEQ_TABLE_EST_HD*)pr1000_ceq_table_estAtten2_CVI;}break;}bFind
[0]=0;while(pTableHD->distance!=0xffff){distance=pTableHD->distance;rangeMin=
pTableHD->pData[camResol-pr1000_inresol_1280x720p60][0];rangeMax=pTableHD->pData
[camResol-pr1000_inresol_1280x720p60][1];factor=pTableHD->pData[camResol-
pr1000_inresol_1280x720p60][2];if((rangeMin<=u16NewAtten)&&(u16NewAtten<=
rangeMax)){pu8ResultInfo[0]=factor&0xff;
resultDistance[0]=distance;bFind[0]=1;break;}pTableHD++;}if(bFind[0]==0)
ErrorString("\x43\x61\x6e\x27\x74\x20\x66\x69\x6e\x64\x20\x65\x73\x74\x32" "\n")
;}}else{Error(
"\x49\x6e\x76\x61\x6c\x69\x64\x20\x66\x6f\x72\x6d\x61\x74\x28\x25\x64\x29\x2e" "\n"
,format);return(-1);}pu16ResultFact[0]=resultDistance[0];
pu16ResultFact[1]=resultDistance[1];
if((bFind[0]==1)&&(bFind[1]==1)){Dbg(
"\x45\x51\x20\x45\x73\x74\x5b\x25\x73\x2c\x20\x25\x73\x5d" "\n",
_STR_PR1000_FORMAT[format]COMMA _STR_PR1000_INRESOL[camResol]);Print(
"\x45\x51\x20\x45\x73\x74\x20\x43\x6f\x6d\x70\x20\x76\x61\x6c\x75\x65\x5b\x25\x64\x6d\x3a\x30\x78\x25\x30\x34\x78\x28\x63\x6f\x6d\x70\x31\x3a\x25\x30\x34\x78\x2c\x63\x6f\x6d\x70\x32\x3a\x25\x30\x34\x78\x29\x5d\x3d\x3e\x28\x30\x78\x25\x30\x32\x78\x29" "\n"
,resultDistance[1]COMMA u16NewComp COMMA u16Comp1 COMMA u16Comp2 COMMA 
pu8ResultInfo[1]);Print(
"\x45\x51\x20\x45\x73\x74\x20\x41\x74\x74\x65\x6e\x20\x76\x61\x6c\x75\x65\x5b\x25\x64\x6d\x3a\x30\x78\x25\x30\x34\x78\x5d\x3d\x3e\x28\x30\x78\x25\x30\x32\x78\x29" "\n"
,resultDistance[0]COMMA u16NewAtten COMMA pu8ResultInfo[0]);ret=1;}else{
pu8ResultInfo[0]=0x00;pu8ResultInfo[1]=0x00;Print(
"\x49\x6e\x76\x61\x6c\x69\x64\x20\x45\x51\x20\x45\x73\x74\x20\x43\x6f\x6d\x70\x20\x76\x61\x6c\x75\x65\x5b\x25\x64\x6d\x3a\x30\x78\x25\x30\x34\x78\x28\x63\x6f\x6d\x70\x31\x3a\x25\x30\x34\x78\x2c\x63\x6f\x6d\x70\x32\x3a\x25\x30\x34\x78\x29\x5d\x3d\x3e\x28\x30\x78\x25\x30\x32\x78\x29" "\n"
,resultDistance[1]COMMA u16NewComp COMMA u16Comp1 COMMA u16Comp2 COMMA 
pu8ResultInfo[1]);Print(
"\x49\x6e\x76\x61\x6c\x69\x64\x20\x45\x51\x20\x45\x73\x74\x20\x41\x74\x74\x65\x6e\x20\x76\x61\x6c\x75\x65\x5b\x25\x64\x6d\x3a\x30\x78\x25\x30\x34\x78\x5d\x3d\x3e\x28\x30\x78\x25\x30\x32\x78\x29" "\n"
,resultDistance[0]COMMA u16NewAtten COMMA pu8ResultInfo[0]);ret=-1;}return(ret);
}int PR1000_CEQ_GetEQDistFactor(const int fd,const enum _pr1000_table_format 
format,const enum _pr1000_table_inresol camResol,const uint16_t*pu16DistInfo,
uint8_t*pu8ResultFact){int ret=-1;_PR1000_CEQ_TABLE_EST_SD*pTableSD=NULL;
_PR1000_CEQ_TABLE_EST_HD*pTableHD=NULL;uint16_t distance=0;uint16_t factor;
uint8_t bFind[2]={0,};if((pu8ResultFact==NULL)||(pu16DistInfo==NULL)){
ErrorString("\x49\x6e\x76\x61\x6c\x69\x64\x20\x61\x72\x67\x75\x2e" "\n");return(
-1);}if((format==pr1000_format_SD720)||(format==pr1000_format_SD960))
{if(camResol>pr1000_inresol_pal){Error(
"\x49\x6e\x76\x61\x6c\x69\x64\x20\x73\x64\x20\x63\x61\x6d\x52\x65\x73\x6f\x6c\x28\x25\x64\x29\x2e" "\n"
,camResol);return(-1);}pTableSD=(_PR1000_CEQ_TABLE_EST_SD*)
pr1000_ceq_table_estComplex_SD;bFind[1]=0;while(pTableSD->distance!=0xffff){
distance=pTableSD->distance;factor=pTableSD->pData[camResol][2];if((distance==
pu16DistInfo[1])){pu8ResultFact[1]=factor&0xff;
bFind[1]=1;if(distance>1200){pu8ResultFact[0]=(factor>>8)&0xff;
bFind[0]=1;}break;}pTableSD++;}if(bFind[1]==0)ErrorString(
"\x43\x61\x6e\x27\x74\x20\x66\x69\x6e\x64\x20\x65\x73\x74\x31" "\n");if(bFind[1]
&&(distance<=1200)){pTableSD=(_PR1000_CEQ_TABLE_EST_SD*)
pr1000_ceq_table_estAtten2_SD;bFind[0]=0;while(pTableSD->distance!=0xffff){
distance=pTableSD->distance;factor=pTableSD->pData[camResol][2];if((distance==
pu16DistInfo[0])){pu8ResultFact[0]=factor&0xff;
bFind[0]=1;break;}pTableSD++;}if(bFind[0]==0)ErrorString(
"\x43\x61\x6e\x27\x74\x20\x66\x69\x6e\x64\x20\x65\x73\x74\x32" "\n");}}else if(
#ifndef DONT_SUPPORT_STD_PVI
(format==pr1000_format_PVI)||
#endif 
(format==pr1000_format_HDA)||(format==pr1000_format_CVI)||(format==
pr1000_format_HDT)||(format==pr1000_format_HDT_NEW))
{if((camResol<pr1000_inresol_1280x720p60)){Error(
"\x49\x6e\x76\x61\x6c\x69\x64\x20\x68\x64\x20\x72\x65\x73\x6f\x6c\x28\x25\x64\x29\x2e" "\n"
,camResol);return(-1);}switch(format){
#ifndef DONT_SUPPORT_STD_PVI
case pr1000_format_PVI:{pTableHD=(_PR1000_CEQ_TABLE_EST_HD*)
pr1000_ceq_table_estComplex_PVI;}break;
#endif 
case pr1000_format_HDA:{pTableHD=(_PR1000_CEQ_TABLE_EST_HD*)
pr1000_ceq_table_estComplex_HDA;}break;case pr1000_format_CVI:{pTableHD=(
_PR1000_CEQ_TABLE_EST_HD*)pr1000_ceq_table_estComplex_CVI;}break;case 
pr1000_format_HDT:{pTableHD=(_PR1000_CEQ_TABLE_EST_HD*)
pr1000_ceq_table_estComplex_HDT;}break;case pr1000_format_HDT_NEW:{pTableHD=(
_PR1000_CEQ_TABLE_EST_HD*)pr1000_ceq_table_estComplex_HDT;}break;default:{
pTableHD=(_PR1000_CEQ_TABLE_EST_HD*)pr1000_ceq_table_estComplex_CVI;}break;}
bFind[1]=0;while(pTableHD->distance!=0xffff){distance=pTableHD->distance;factor=
pTableHD->pData[camResol-pr1000_inresol_1280x720p60][2];if((distance==
pu16DistInfo[1])){pu8ResultFact[1]=factor&0xff;
bFind[1]=1;if(distance>1200){pu8ResultFact[0]=(factor>>8)&0xff;
bFind[0]=1;}break;}pTableHD++;}if(bFind[1]==0)ErrorString(
"\x43\x61\x6e\x27\x74\x20\x66\x69\x6e\x64\x20\x65\x73\x74\x31" "\n");if(bFind[1]
&&(distance<=1200)){switch(format){
#ifndef DONT_SUPPORT_STD_PVI
case pr1000_format_PVI:{pTableHD=(_PR1000_CEQ_TABLE_EST_HD*)
pr1000_ceq_table_estAtten2_PVI;}break;
#endif 
case pr1000_format_HDA:{pTableHD=(_PR1000_CEQ_TABLE_EST_HD*)
pr1000_ceq_table_estAtten2_HDA;}break;case pr1000_format_CVI:{pTableHD=(
_PR1000_CEQ_TABLE_EST_HD*)pr1000_ceq_table_estAtten2_CVI;}break;case 
pr1000_format_HDT:{pTableHD=(_PR1000_CEQ_TABLE_EST_HD*)
pr1000_ceq_table_estAtten2_HDT;}break;case pr1000_format_HDT_NEW:{pTableHD=(
_PR1000_CEQ_TABLE_EST_HD*)pr1000_ceq_table_estAtten2_HDT;}break;default:{
pTableHD=(_PR1000_CEQ_TABLE_EST_HD*)pr1000_ceq_table_estAtten2_CVI;}break;}bFind
[0]=0;while(pTableHD->distance!=0xffff){distance=pTableHD->distance;factor=
pTableHD->pData[camResol-pr1000_inresol_1280x720p60][2];if((distance==
pu16DistInfo[0])){pu8ResultFact[0]=factor&0xff;
bFind[0]=1;break;}pTableHD++;}if(bFind[0]==0)ErrorString(
"\x43\x61\x6e\x27\x74\x20\x66\x69\x6e\x64\x20\x65\x73\x74\x32" "\n");}}else{
Error(
"\x49\x6e\x76\x61\x6c\x69\x64\x20\x66\x6f\x72\x6d\x61\x74\x28\x25\x64\x29\x2e" "\n"
,format);return(-1);}if((bFind[0]==1)&&(bFind[1]==1)){Dbg(
"\x45\x51\x20\x44\x69\x73\x74\x5b\x25\x73\x2c\x20\x25\x73\x5d" "\n",
_STR_PR1000_FORMAT[format]COMMA _STR_PR1000_INRESOL[camResol]);Print(
"\x45\x51\x20\x44\x69\x73\x74\x20\x43\x6f\x6d\x70\x20\x76\x61\x6c\x75\x65\x5b\x25\x64\x6d\x3a\x30\x78\x25\x30\x32\x78\x5d" "\n"
,pu16DistInfo[1]COMMA pu8ResultFact[1]);Print(
"\x45\x51\x20\x44\x69\x73\x74\x20\x41\x74\x74\x65\x6e\x20\x76\x61\x6c\x75\x65\x5b\x25\x64\x6d\x3a\x30\x78\x25\x30\x32\x78\x5d" "\n"
,pu16DistInfo[0]COMMA pu8ResultFact[0]);ret=1;}else{ret=-1;}return(ret);}int 
PR1000_CEQ_GetSTDEst(const int fd,const enum _pr1000_table_format format,const 
enum _pr1000_table_inresol camResol,const _stCeqInfoReg*pstCeqInfoReg,uint8_t*
pu8ResultInfo,uint16_t*pu16ResultFact){int ret=-1;uint8_t u8Info[2]={0,};
uint16_t u16Fact[2]={0,};if((pu8ResultInfo==NULL)||(pu16ResultFact==NULL)){
ErrorString("\x49\x6e\x76\x61\x6c\x69\x64\x20\x61\x72\x67\x75\x2e" "\n");return(
-1);}DbgString("\x47\x65\x74\x20\x53\x54\x44\x45\x73\x74" "\n");if((ret=
PR1000_CEQ_GetEQEstFactor(fd,format,camResol,pstCeqInfoReg,u8Info,u16Fact))<0){
ErrorString(
"\x43\x61\x6e\x27\x74\x20\x67\x65\x74\x20\x65\x71\x20\x65\x73\x74\x20\x66\x61\x63\x74\x6f\x72" "\n"
);return(-1);}pu8ResultInfo[0]=u8Info[0];
pu8ResultInfo[1]=u8Info[1];
pu16ResultFact[0]=u16Fact[0];pu16ResultFact[1]=u16Fact[1];return(ret);}int 
PR1000_CEQ_GetEQTunnEstFactor(const int fd,const enum _pr1000_table_format 
format,const enum _pr1000_table_inresol camResol,const _stCeqInfoReg*
pstCeqInfoReg,int stepDir,uint16_t*pu16CurFact,uint8_t*pu8ResultInfo,uint16_t*
pu16ResultFact){int ret=-1;_PR1000_CEQ_TABLE_EST_SD*pTableSD=NULL;
_PR1000_CEQ_TABLE_EST_HD*pTableHD=NULL;uint16_t distance;uint16_t resultDistance
[2]={0,};uint16_t factor;uint8_t bFind[2]={0,};int inx=0;int bLast[2]={0,};if((
pu8ResultInfo==NULL)||(pu16ResultFact==NULL)){ErrorString(
"\x49\x6e\x76\x61\x6c\x69\x64\x20\x61\x72\x67\x75\x2e" "\n");return(-1);}if((
format==pr1000_format_SD720)||(format==pr1000_format_SD960))
{if(camResol>pr1000_inresol_pal){Error(
"\x49\x6e\x76\x61\x6c\x69\x64\x20\x73\x64\x20\x63\x61\x6d\x52\x65\x73\x6f\x6c\x28\x25\x64\x29\x2e" "\n"
,camResol);return(-1);}pTableSD=(_PR1000_CEQ_TABLE_EST_SD*)
pr1000_ceq_table_estComplex_SD;bFind[1]=0;for(inx=0;inx<MAX_PR1000_CEQ_TBL_NUM;
inx++){distance=pTableSD[inx].distance;if(stepDir>0){if(distance>pu16CurFact[1])
{bFind[1]=1;break;}}else if(stepDir<0){if(distance>=pu16CurFact[1]){if(inx>0)inx
--;else{inx=0;bLast[1]=TRUE;}bFind[1]=1;break;}}else
{if(distance==pu16CurFact[1]){bFind[1]=1;break;}}}if(inx>=MAX_PR1000_CEQ_TBL_NUM
)bLast[1]=TRUE;else if(inx==0)bLast[1]=TRUE;if(bFind[1]){factor=pTableSD[inx].
pData[camResol][2];distance=pTableSD[inx].distance;pu8ResultInfo[1]=factor&0xff;
resultDistance[1]=distance;if(distance>1200){pu8ResultInfo[0]=(factor>>8)&0xff;
resultDistance[0]=distance;bLast[0]=TRUE;}}if(bFind[1]==0)ErrorString(
"\x43\x61\x6e\x27\x74\x20\x66\x69\x6e\x64\x20\x65\x73\x74\x31" "\n");if(bFind[1]
&&(distance<=1200)){pTableSD=(_PR1000_CEQ_TABLE_EST_SD*)
pr1000_ceq_table_estAtten2_SD;bFind[0]=0;for(inx=0;inx<MAX_PR1000_CEQ_TBL_NUM;
inx++){distance=pTableSD[inx].distance;if(stepDir>0){if(distance>pu16CurFact[0])
{bFind[0]=1;break;}}else if(stepDir<0){if(distance>=pu16CurFact[0]){if(inx>0)inx
--;else{inx=0;bLast[0]=TRUE;}bFind[0]=1;break;}}else
{if(distance==pu16CurFact[0]){bFind[0]=1;break;}}}if(inx>=MAX_PR1000_CEQ_TBL_NUM
)bLast[0]=TRUE;else if(inx==0)bLast[0]=TRUE;if(bFind[0]){factor=pTableSD[inx].
pData[camResol][2];distance=pTableSD[inx].distance;pu8ResultInfo[0]=factor&0xff;
resultDistance[0]=distance;}if(bFind[0]==0)ErrorString(
"\x43\x61\x6e\x27\x74\x20\x66\x69\x6e\x64\x20\x65\x73\x74\x32" "\n");}}else if(
#ifndef DONT_SUPPORT_STD_PVI
(format==pr1000_format_PVI)||
#endif 
(format==pr1000_format_HDA)||(format==pr1000_format_CVI)||(format==
pr1000_format_HDT)||(format==pr1000_format_HDT_NEW))
{if((camResol<pr1000_inresol_1280x720p60)){Error(
"\x49\x6e\x76\x61\x6c\x69\x64\x20\x68\x64\x20\x72\x65\x73\x6f\x6c\x28\x25\x64\x29\x2e" "\n"
,camResol);return(-1);}switch(format){
#ifndef DONT_SUPPORT_STD_PVI
case pr1000_format_PVI:{pTableHD=(_PR1000_CEQ_TABLE_EST_HD*)
pr1000_ceq_table_estComplex_PVI;}break;
#endif 
case pr1000_format_HDA:{pTableHD=(_PR1000_CEQ_TABLE_EST_HD*)
pr1000_ceq_table_estComplex_HDA;}break;case pr1000_format_CVI:{pTableHD=(
_PR1000_CEQ_TABLE_EST_HD*)pr1000_ceq_table_estComplex_CVI;}break;case 
pr1000_format_HDT:{pTableHD=(_PR1000_CEQ_TABLE_EST_HD*)
pr1000_ceq_table_estComplex_HDT;}break;case pr1000_format_HDT_NEW:{pTableHD=(
_PR1000_CEQ_TABLE_EST_HD*)pr1000_ceq_table_estComplex_HDT;}break;default:{
pTableHD=(_PR1000_CEQ_TABLE_EST_HD*)pr1000_ceq_table_estComplex_CVI;}break;}
bFind[1]=0;for(inx=0;inx<MAX_PR1000_CEQ_TBL_NUM;inx++){distance=pTableHD[inx].
distance;if(stepDir>0){if(distance>pu16CurFact[1]){bFind[1]=1;break;}}else if(
stepDir<0){if(distance>=pu16CurFact[1]){if(inx>0)inx--;else{inx=0;bLast[1]=TRUE;
}bFind[1]=1;break;}}else
{if(distance==pu16CurFact[1]){bFind[1]=1;break;}}}if(inx>=MAX_PR1000_CEQ_TBL_NUM
)bLast[1]=TRUE;else if(inx==0)bLast[1]=TRUE;if(bFind[1]){factor=pTableHD[inx].
pData[camResol-pr1000_inresol_1280x720p60][2];distance=pTableHD[inx].distance;
pu8ResultInfo[1]=factor&0xff;
resultDistance[1]=distance;if(distance>1200){pu8ResultInfo[0]=(factor>>8)&0xff;
resultDistance[0]=distance;bLast[0]=TRUE;}}if(bFind[1]==0)ErrorString(
"\x43\x61\x6e\x27\x74\x20\x66\x69\x6e\x64\x20\x65\x73\x74\x31" "\n");if(bFind[1]
&&(distance<=1200)){switch(format){
#ifndef DONT_SUPPORT_STD_PVI
case pr1000_format_PVI:{pTableHD=(_PR1000_CEQ_TABLE_EST_HD*)
pr1000_ceq_table_estAtten2_PVI;}break;
#endif 
case pr1000_format_HDA:{pTableHD=(_PR1000_CEQ_TABLE_EST_HD*)
pr1000_ceq_table_estAtten2_HDA;}break;case pr1000_format_CVI:{pTableHD=(
_PR1000_CEQ_TABLE_EST_HD*)pr1000_ceq_table_estAtten2_CVI;}break;case 
pr1000_format_HDT:{pTableHD=(_PR1000_CEQ_TABLE_EST_HD*)
pr1000_ceq_table_estAtten2_HDT;}break;case pr1000_format_HDT_NEW:{pTableHD=(
_PR1000_CEQ_TABLE_EST_HD*)pr1000_ceq_table_estAtten2_HDT;}break;default:{
pTableHD=(_PR1000_CEQ_TABLE_EST_HD*)pr1000_ceq_table_estAtten2_CVI;}break;}bFind
[0]=0;for(inx=0;inx<MAX_PR1000_CEQ_TBL_NUM;inx++){distance=pTableHD[inx].
distance;if(stepDir>0){if(distance>pu16CurFact[0]){bFind[0]=1;break;}}else if(
stepDir<0){if(distance>=pu16CurFact[0]){if(inx>0)inx--;else{inx=0;bLast[0]=TRUE;
}bFind[0]=1;break;}}else
{if(distance==pu16CurFact[0]){bFind[0]=1;break;}}}if(inx>=MAX_PR1000_CEQ_TBL_NUM
)bLast[0]=TRUE;else if(inx==0)bLast[0]=TRUE;if(bFind[0]){factor=pTableHD[inx].
pData[camResol-pr1000_inresol_1280x720p60][2];distance=pTableHD[inx].distance;
pu8ResultInfo[0]=factor&0xff;
resultDistance[0]=distance;}else{pu8ResultInfo[0]=0x00;resultDistance[0]=0;}
bFind[0]=1;}}else{Error(
"\x49\x6e\x76\x61\x6c\x69\x64\x20\x66\x6f\x72\x6d\x61\x74\x28\x25\x64\x29\x2e" "\n"
,format);return(-1);}pu16ResultFact[0]=resultDistance[0];
pu16ResultFact[1]=resultDistance[1];
if((bFind[0]==1)&&(bFind[1]==1)){Dbg(
"\x45\x51\x20\x54\x55\x4e\x4e\x20\x45\x73\x74\x5b\x25\x73\x2c\x20\x25\x73\x5d" "\n"
,_STR_PR1000_FORMAT[format]COMMA _STR_PR1000_INRESOL[camResol]);Print(
"\x45\x51\x20\x54\x55\x4e\x4e\x20\x45\x73\x74\x20\x43\x6f\x6d\x70\x20\x76\x61\x6c\x75\x65\x5b\x25\x64\x6d\x2c\x30\x78\x25\x30\x32\x78\x5d\x2c\x20\x62\x4c\x61\x73\x74\x3a\x25\x64" "\n"
,resultDistance[1]COMMA pu8ResultInfo[1]COMMA bLast[1]);Print(
"\x45\x51\x20\x54\x55\x4e\x4e\x20\x45\x73\x74\x20\x41\x74\x74\x65\x6e\x20\x76\x61\x6c\x75\x65\x5b\x25\x64\x6d\x2c\x30\x78\x25\x30\x32\x78\x5d\x2c\x20\x62\x4c\x61\x73\x74\x3a\x25\x64" "\n"
,resultDistance[0]COMMA pu8ResultInfo[0]COMMA bLast[0]);if((bLast[0]==TRUE)&&(
bLast[1]==TRUE))ret=0;else ret=1;}else{ret=-1;}return(ret);}int 
PR1000_CEQ_Compensate(const int fd,const int mapChn,const enum 
_pr1000_table_format format,const enum _pr1000_table_inresol camResol,const 
uint8_t bWrAtten,const uint8_t bWrComp,void*pArgu){int ret=-1;uint8_t i2cReg=0;
int page;_stCeqInfoReg infoRegs;uint16_t u16Comp1,u16Comp2;uint16_t u16CompValue
=0;uint16_t u16NewComp1,u16NewComp2;uint16_t u16NewCompValue=0;uint16_t 
u16CompAbsDiff=0;uint8_t estNewResult[2];uint16_t estResultFact[2];uint8_t 
prChip,prChn,i2cSlaveAddr;_drvHost*pHost=(_drvHost*)pArgu;_stPortChSel*
pstPortChSel=(_stPortChSel*)&pHost->sysHost.portChSel[mapChn];_stCEQData*
pstCEQData=(_stCEQData*)&pHost->sysHost.stCEQDataList[mapChn];if((pstPortChSel==
NULL)||!ASSERT_VALID_CH(pstPortChSel->i2cSlvAddr)){ErrorString(
"\x49\x6e\x76\x61\x6c\x69\x64\x20\x63\x68\x2e" "\n");return(-1);}prChip=
pstPortChSel->prChip;prChn=pstPortChSel->prChn;i2cSlaveAddr=pstPortChSel->
i2cSlvAddr;if((!pstCEQData->bEnable)||(!pstCEQData->bLock)){Error(
"\x44\x69\x73\x61\x62\x6c\x65\x64\x20\x5b\x6d\x61\x70\x43\x68\x6e\x3a\x25\x64\x5d" "\n"
,mapChn);return(-1);}if((pstCEQData->estStep!=PR1000_CEQ_STEP_ESTSTART)&&(
pstCEQData->estStep!=PR1000_CEQ_STEP_ESTCHGING)){Dbg(
"\x43\x6f\x6d\x70\x65\x6e\x73\x61\x74\x65\x20\x6d\x61\x70\x43\x68\x6e\x3a\x25\x64\x20\x25\x73" "\n"
,mapChn COMMA _STR_PR1000_CEQ_STEP[pstCEQData->estStep]);return(-1);}
#ifdef SUPPORT_CABLE_EQ
if((PR1000_CEQ_GetInfoRegs(fd,pstPortChSel,&infoRegs))<0){Error(
"\x47\x65\x74\x20\x49\x6e\x66\x6f\x52\x65\x67\x73\x20\x5b\x6d\x61\x70\x43\x68\x6e\x3a\x25\x64\x5d" "\n"
,mapChn);return(-1);}Dbg(
"\x43\x6f\x6d\x70\x65\x6e\x73\x61\x74\x65\x20\x6d\x61\x70\x43\x68\x6e\x3a\x25\x64\x20\x25\x73" "\n"
,mapChn COMMA _STR_PR1000_CEQ_STEP[pstCEQData->estStep]);if(pstCEQData->estStep
==PR1000_CEQ_STEP_ESTSTART){u16Comp1=(pstCEQData->infoRegs.b.det_eq_comp1_h<<8)|
(pstCEQData->infoRegs.b.det_eq_comp1_l);u16Comp2=(pstCEQData->infoRegs.b.
det_eq_comp2_h<<8)|(pstCEQData->infoRegs.b.det_eq_comp2_l);if(u16Comp2!=0)
u16CompValue=PR1000_CEQ_EST_COMP(u16Comp1,u16Comp2);else u16CompValue=0;Dbg(
"\x45\x51\x20\x4f\x6c\x64\x43\x6f\x6d\x70\x20\x76\x61\x6c\x75\x65\x5b\x30\x78\x25\x30\x34\x78\x28\x63\x6f\x6d\x70\x31\x3a\x25\x30\x34\x78\x2c\x63\x6f\x6d\x70\x32\x3a\x25\x30\x34\x78\x29\x5d" "\n"
,u16CompValue COMMA u16Comp1 COMMA u16Comp2);u16NewComp1=(infoRegs.b.
det_eq_comp1_h<<8)|(infoRegs.b.det_eq_comp1_l);u16NewComp2=(infoRegs.b.
det_eq_comp2_h<<8)|(infoRegs.b.det_eq_comp2_l);if(u16NewComp2!=0)u16NewCompValue
=PR1000_CEQ_EST_COMP(u16NewComp1,u16NewComp2);else u16NewCompValue=0;Dbg(
"\x45\x51\x20\x4e\x65\x77\x43\x6f\x6d\x70\x20\x76\x61\x6c\x75\x65\x5b\x30\x78\x25\x30\x34\x78\x28\x63\x6f\x6d\x70\x31\x3a\x25\x30\x34\x78\x2c\x63\x6f\x6d\x70\x32\x3a\x25\x30\x34\x78\x29\x5d" "\n"
,u16NewCompValue COMMA u16NewComp1 COMMA u16NewComp2);pstCEQData->
flagStepComplete[PR1000_CEQ_STEP_ESTSTART]=TRUE;u16CompAbsDiff=(u16NewCompValue
>=u16CompValue)?(u16NewCompValue-u16CompValue):(u16CompValue-u16NewCompValue);if
(u16CompAbsDiff<PR1000_EQCOMP_LIMIT){Dbg(
"\x3d\x3e\x20\x53\x54\x44\x20\x45\x51\x28\x6d\x61\x70\x43\x68\x6e\x3a\x25\x64\x29\x20\x63\x6f\x6d\x70\x20\x64\x69\x66\x66\x20\x75\x6e\x64\x65\x72\x20\x6c\x69\x6d\x69\x74\x2e\x20\x5b\x30\x78\x25\x30\x34\x78\x2f\x30\x78\x25\x30\x34\x78\x5d" "\n"
,mapChn COMMA u16CompAbsDiff COMMA PR1000_EQCOMP_LIMIT);ret=0;return(ret);}else{
Dbg(
"\x45\x51\x20\x4f\x6c\x64\x43\x6f\x6d\x70\x20\x76\x61\x6c\x75\x65\x5b\x30\x78\x25\x30\x34\x78\x28\x63\x6f\x6d\x70\x31\x3a\x25\x30\x34\x78\x2c\x63\x6f\x6d\x70\x32\x3a\x25\x30\x34\x78\x29\x5d" "\n"
,u16CompValue COMMA u16Comp1 COMMA u16Comp2);Dbg(
"\x45\x51\x20\x4e\x65\x77\x43\x6f\x6d\x70\x20\x76\x61\x6c\x75\x65\x5b\x30\x78\x25\x30\x34\x78\x28\x63\x6f\x6d\x70\x31\x3a\x25\x30\x34\x78\x2c\x63\x6f\x6d\x70\x32\x3a\x25\x30\x34\x78\x29\x5d" "\n"
,u16NewCompValue COMMA u16NewComp1 COMMA u16NewComp2);Print(
"\x3d\x3e\x20\x53\x54\x44\x20\x45\x51\x28\x6d\x61\x70\x43\x68\x6e\x3a\x25\x64\x29\x20\x63\x6f\x6d\x70\x20\x64\x69\x66\x66\x20\x6f\x76\x65\x72\x20\x6c\x69\x6d\x69\x74\x2e\x20\x43\x68\x61\x6e\x67\x65\x20\x66\x61\x63\x74\x6f\x72\x2e\x20\x5b\x30\x78\x25\x30\x34\x78\x2f\x30\x78\x25\x30\x34\x78\x5d" "\n"
,mapChn COMMA u16CompAbsDiff COMMA PR1000_EQCOMP_LIMIT);pstCEQData->estStep=
PR1000_CEQ_STEP_ESTCHGING;memcpy(&pstCEQData->infoRegs,&infoRegs,sizeof(
_stCeqInfoReg));ret=0;}}if(pstCEQData->estStep==PR1000_CEQ_STEP_ESTCHGING){if((
ret=PR1000_CEQ_GetSTDEst(fd,format,camResol,&infoRegs,estNewResult,estResultFact
))<0){ErrorString("\x67\x65\x74\x20\x65\x73\x74" "\n");return(-1);}else if(ret==
0){estNewResult[0]=pstCEQData->estResultAtten;estNewResult[1]=pstCEQData->
estResultComp;estResultFact[0]=pstCEQData->attenFact;estResultFact[1]=pstCEQData
->compFact;}pstCEQData->flagStepComplete[PR1000_CEQ_STEP_ESTCHGING]=TRUE;if((
pstCEQData->estResultAtten==estNewResult[0])&&(pstCEQData->estResultComp==
estNewResult[1])){pstCEQData->estStep=PR1000_CEQ_STEP_ESTSTART;Dbg(
"\x3d\x3e\x20\x53\x54\x44\x20\x45\x51\x28\x6d\x61\x70\x43\x68\x6e\x3a\x25\x64\x29\x20\x66\x61\x63\x74\x6f\x72\x20\x73\x61\x6d\x65\x20\x77\x69\x74\x68\x20\x70\x72\x65\x76\x69\x6f\x75\x73\x2e" "\n"
,mapChn);ret=0;}else{Print(
"\x65\x73\x74\x44\x69\x66\x66\x20\x5b\x28\x41\x74\x74\x65\x6e\x3a\x25\x64\x6d\x2c\x30\x78\x25\x78\x29\x2c\x20\x28\x43\x6f\x6d\x70\x3a\x25\x64\x6d\x2c\x30\x78\x25\x78\x29\x5d\x2d\x3e\x5b\x28\x41\x74\x74\x65\x6e\x3a\x25\x64\x6d\x2c\x30\x78\x25\x78\x29\x2c\x20\x28\x43\x6f\x6d\x70\x3a\x25\x64\x6d\x2c\x30\x78\x25\x78\x29\x5d" "\n"
,pstCEQData->attenFact COMMA pstCEQData->estResultAtten COMMA pstCEQData->
compFact COMMA pstCEQData->estResultComp COMMA estResultFact[0]COMMA 
estNewResult[0]COMMA estResultFact[1]COMMA estNewResult[1]);Dbg(
"\x65\x73\x74\x44\x69\x66\x66\x20\x5b\x30\x78\x25\x30\x32\x78\x20\x30\x78\x25\x30\x32\x78\x20\x2d\x3e\x20\x30\x78\x25\x30\x32\x78\x20\x30\x78\x25\x30\x32\x78\x5d" "\n"
,pstCEQData->estResultAtten COMMA pstCEQData->estResultComp COMMA estNewResult[0
]COMMA estNewResult[1]);if(bWrAtten){pstCEQData->attenFact=estResultFact[0];if(
pstCEQData->estResultAtten<estNewResult[0]){if(pstCEQData->estResultAtten<0x1f)
pstCEQData->estResultAtten++;}else if(pstCEQData->estResultAtten>estNewResult[0]
){if(pstCEQData->estResultAtten>0x00)pstCEQData->estResultAtten--;}page=
PR1000_REG_PAGE_COMMON;i2cReg=0x12+PR1000_OFFSETADDR_CEQ_INFO_CH(prChn);
estNewResult[0]=pstCEQData->estResultAtten|0x20;
PR1000_PageWrite(fd,i2cSlaveAddr,page,i2cReg,(uint8_t)estNewResult[0]);Print(
"\x53\x65\x74\x20\x45\x51\x20\x76\x61\x6c\x75\x65\x20\x6d\x61\x70\x43\x68\x6e\x3a\x25\x64\x2e\x20\x28\x41\x74\x74\x65\x6e\x3a\x25\x64\x6d\x2c\x30\x78\x25\x78\x29" "\n"
,mapChn COMMA estResultFact[0]COMMA estNewResult[0]);}if(bWrComp){if(
estResultFact[1]>=estResultFact[0]){pstCEQData->compFact=estResultFact[0];
estResultFact[0]=pstCEQData->compFact;estResultFact[1]=pstCEQData->attenFact;
Print(
"\x6d\x61\x70\x43\x68\x6e\x3a\x25\x64\x20\x67\x65\x74\x20\x66\x72\x6f\x6d\x20\x61\x74\x74\x65\x6e\x3a\x25\x64\x6d" "\n"
,mapChn COMMA pstCEQData->attenFact);}else{pstCEQData->compFact=estResultFact[1]
;estResultFact[1]=pstCEQData->compFact;
estResultFact[0]=pstCEQData->attenFact;Print(
"\x6d\x61\x70\x43\x68\x6e\x3a\x25\x64\x20\x67\x65\x74\x20\x66\x72\x6f\x6d\x20\x63\x6f\x6d\x70\x3a\x25\x64\x6d" "\n"
,mapChn COMMA pstCEQData->compFact);}if((PR1000_CEQ_GetEQDistFactor(fd,
pstCEQData->format,pstCEQData->camResol,estResultFact,estNewResult))<0){Error(
"\x49\x6e\x76\x61\x6c\x69\x64\x20\x67\x65\x74\x73\x74\x64\x64\x69\x73\x74\x2e\x20\x46\x6f\x72\x63\x65\x6c\x79\x20\x62\x79\x70\x61\x73\x73\x20\x45\x51\x2e\x20\x5b\x6d\x61\x70\x43\x68\x6e\x3a\x25\x64\x5d" "\n"
,mapChn);return(-1);}if(pstCEQData->estResultComp<estNewResult[1]){if(pstCEQData
->estResultComp<0x3f)pstCEQData->estResultComp++;}else if(pstCEQData->
estResultComp>estNewResult[1]){if(pstCEQData->estResultComp>0x00)pstCEQData->
estResultComp--;}Print(
"\x6d\x61\x70\x43\x68\x6e\x3a\x25\x64\x20\x64\x65\x74\x5f\x65\x71\x5f\x64\x63\x67\x61\x69\x6e\x3a\x30\x78\x25\x30\x34\x78" "\n"
,mapChn COMMA pstCEQData->saved_det_eq_dcgain);if(pstCEQData->
saved_det_eq_dcgain>=0x2000){page=PR1000_REG_PAGE_COMMON;i2cReg=0x13+
PR1000_OFFSETADDR_CEQ_INFO_CH(prChn);estNewResult[1]=pstCEQData->estResultComp;
PR1000_PageWrite(fd,i2cSlaveAddr,page,i2cReg,(uint8_t)estNewResult[1]);Print(
"\x53\x65\x74\x20\x45\x51\x20\x76\x61\x6c\x75\x65\x20\x6d\x61\x70\x43\x68\x6e\x3a\x25\x64\x2e\x20\x28\x43\x6f\x6d\x70\x3a\x25\x64\x6d\x2c\x30\x78\x25\x78\x29" "\n"
,mapChn COMMA estResultFact[1]COMMA estNewResult[1]);}}Dbg(
"\x45\x51\x20\x4e\x65\x77\x20\x77\x72\x69\x74\x65\x20\x5b\x30\x78\x25\x78\x2c\x30\x78\x25\x78\x5d" "\n"
,pstCEQData->estResultAtten COMMA pstCEQData->estResultComp);ret=0;}memcpy(&
pstCEQData->infoRegs,&infoRegs,sizeof(_stCeqInfoReg));}
#else
ret=0;
#endif 
return(ret);}int PR1000_CEQ_GetEQInfo(const int fd,const _stPortChSel*
pstPortChSel,_stCEQInfo*pstCEQInfo){int ret=0;_stCeqInfoReg infoRegs;if((
pstPortChSel==NULL)||!ASSERT_VALID_CH(pstPortChSel->i2cSlvAddr)){ErrorString(
"\x49\x6e\x76\x61\x6c\x69\x64\x20\x63\x68\x2e" "\n");return(-1);}if(pstCEQInfo==
NULL){ErrorString("\x4e\x75\x6c\x6c\x20\x70\x74\x72\x2e" "\n");return(-1);}
PR1000_CEQ_GetInfoRegs(fd,pstPortChSel,&infoRegs);pstCEQInfo->dcGain=(infoRegs.b
.det_eq_dcgain_h<<8)|infoRegs.b.det_eq_dcgain_l;pstCEQInfo->acGain=(infoRegs.b.
det_eq_acgain_h<<8)|infoRegs.b.det_eq_acgain_l;pstCEQInfo->comp1=(infoRegs.b.
det_eq_comp1_h<<8)|infoRegs.b.det_eq_comp1_l;pstCEQInfo->comp2=(infoRegs.b.
det_eq_comp2_h<<8)|infoRegs.b.det_eq_comp2_l;pstCEQInfo->atten1=(infoRegs.b.
det_eq_atten1_h<<8)|infoRegs.b.det_eq_atten1_l;pstCEQInfo->atten2=(infoRegs.b.
det_eq_atten2_h<<8)|infoRegs.b.det_eq_atten2_l;return(ret);}int 
PR1000_CEQ_SetEQGain(const int fd,const uint8_t bWrAtten,const uint8_t bWrComp,
const _stPortChSel*pstPortChSel,_stCEQData*pstCEQData){int ret=-1;uint8_t i2cReg
=0;int page;int mapChn;uint8_t prChip,prChn,i2cSlaveAddr;uint8_t estResult[2];
uint16_t estResultFact[2];if((pstPortChSel==NULL)||!ASSERT_VALID_CH(pstPortChSel
->i2cSlvAddr)){ErrorString("\x49\x6e\x76\x61\x6c\x69\x64\x20\x63\x68\x2e" "\n");
return(-1);}if(pstCEQData==NULL){ErrorString(
"\x4e\x75\x6c\x6c\x20\x70\x74\x72\x2e" "\n");return(-1);}mapChn=pstPortChSel->
chn;prChip=pstPortChSel->prChip;prChn=pstPortChSel->prChn;i2cSlaveAddr=
pstPortChSel->i2cSlvAddr;Print(
"\x45\x51\x20\x73\x65\x74\x20\x73\x74\x64\x20\x66\x6f\x72\x6d\x61\x74\x5b\x25\x73\x5d\x2c\x20\x72\x65\x73\x6f\x6c\x5b\x25\x73\x5d\x20\x6d\x61\x70\x43\x68\x6e\x3a\x25\x64" "\n"
,_STR_PR1000_FORMAT[pstCEQData->format]COMMA _STR_PR1000_INRESOL[pstCEQData->
camResol]COMMA mapChn);if((PR1000_CEQ_GetInfoRegs(fd,pstPortChSel,&pstCEQData->
infoRegs))<0){estResult[0]=0x20;
estResult[1]=0x00;Error(
"\x49\x6e\x76\x61\x6c\x69\x64\x20\x69\x6e\x66\x6f\x72\x65\x67\x73\x2e\x20\x46\x6f\x72\x63\x65\x6c\x79\x20\x62\x79\x70\x61\x73\x73\x20\x45\x51\x2e\x20\x5b\x6d\x61\x70\x43\x68\x6e\x3a\x25\x64\x5d" "\n"
,mapChn);if(bWrAtten){page=PR1000_REG_PAGE_COMMON;i2cReg=0x12+
PR1000_OFFSETADDR_CEQ_INFO_CH(prChn);PR1000_PageWrite(fd,i2cSlaveAddr,page,
i2cReg,(uint8_t)estResult[0]);Print(
"\x53\x65\x74\x20\x53\x54\x44\x20\x45\x51\x20\x76\x61\x6c\x75\x65\x20\x6d\x61\x70\x43\x68\x6e\x3a\x25\x64\x2e\x20\x28\x41\x74\x74\x65\x6e\x3a\x25\x64\x6d\x2c\x30\x78\x25\x78\x29" "\n"
,mapChn COMMA estResultFact[0]COMMA estResult[0]);}if(bWrComp){page=
PR1000_REG_PAGE_COMMON;i2cReg=0x13+PR1000_OFFSETADDR_CEQ_INFO_CH(prChn);
PR1000_PageWrite(fd,i2cSlaveAddr,page,i2cReg,(uint8_t)estResult[1]);Print(
"\x53\x65\x74\x20\x53\x54\x44\x20\x45\x51\x20\x76\x61\x6c\x75\x65\x20\x6d\x61\x70\x43\x68\x6e\x3a\x25\x64\x2e\x20\x28\x43\x6f\x6d\x70\x3a\x25\x64\x6d\x2c\x30\x78\x25\x78\x29" "\n"
,mapChn COMMA estResultFact[1]COMMA estResult[1]);}return(-1);}if(pstCEQData->
saved_det_eq_dcgain==0){pstCEQData->saved_det_eq_dcgain=pstCEQData->infoRegs.b.
det_eq_dcgain_h<<8|pstCEQData->infoRegs.b.det_eq_dcgain_l;Dbg(
"\x73\x61\x76\x65\x20\x65\x71\x20\x64\x63\x67\x61\x69\x6e\x3a\x30\x78\x25\x30\x34\x78" "\n"
,pstCEQData->saved_det_eq_dcgain);}if((PR1000_CEQ_GetSTDEst(fd,pstCEQData->
format,pstCEQData->camResol,&pstCEQData->infoRegs,estResult,estResultFact))<0){
estResult[0]=0x20;
estResult[1]=0x00;Error(
"\x49\x6e\x76\x61\x6c\x69\x64\x20\x67\x65\x74\x73\x74\x64\x65\x73\x74\x2e\x20\x46\x6f\x72\x63\x65\x6c\x79\x20\x62\x79\x70\x61\x73\x73\x20\x45\x51\x2e\x20\x5b\x6d\x61\x70\x43\x68\x6e\x3a\x25\x64\x5d" "\n"
,mapChn);if(bWrAtten){page=PR1000_REG_PAGE_COMMON;i2cReg=0x12+
PR1000_OFFSETADDR_CEQ_INFO_CH(prChn);PR1000_PageWrite(fd,i2cSlaveAddr,page,
i2cReg,(uint8_t)estResult[0]);Print(
"\x53\x65\x74\x20\x53\x54\x44\x20\x45\x51\x20\x76\x61\x6c\x75\x65\x20\x6d\x61\x70\x43\x68\x6e\x3a\x25\x64\x2e\x20\x28\x41\x74\x74\x65\x6e\x3a\x25\x64\x6d\x2c\x30\x78\x25\x78\x29" "\n"
,mapChn COMMA estResultFact[0]COMMA estResult[0]);}if(bWrComp){page=
PR1000_REG_PAGE_COMMON;i2cReg=0x13+PR1000_OFFSETADDR_CEQ_INFO_CH(prChn);
PR1000_PageWrite(fd,i2cSlaveAddr,page,i2cReg,(uint8_t)estResult[1]);Print(
"\x53\x65\x74\x20\x53\x54\x44\x20\x45\x51\x20\x76\x61\x6c\x75\x65\x20\x6d\x61\x70\x43\x68\x6e\x3a\x25\x64\x2e\x20\x28\x43\x6f\x6d\x70\x3a\x25\x64\x6d\x2c\x30\x78\x25\x78\x29" "\n"
,mapChn COMMA estResultFact[1]COMMA estResult[1]);}return(-1);}else if(ret==0){
estResult[0]=pstCEQData->estResultAtten;estResult[1]=pstCEQData->estResultComp;
estResultFact[0]=pstCEQData->attenFact;estResultFact[1]=pstCEQData->compFact;}if
(bWrAtten){pstCEQData->estResultAtten=estResult[0]&0x1f;
pstCEQData->attenFact=estResultFact[0];page=PR1000_REG_PAGE_COMMON;i2cReg=0x12+
PR1000_OFFSETADDR_CEQ_INFO_CH(prChn);estResult[0]|=0x20;
ret=PR1000_PageWrite(fd,i2cSlaveAddr,page,i2cReg,(uint8_t)estResult[0]);Print(
"\x45\x51\x20\x67\x61\x69\x6e\x20\x6d\x61\x70\x43\x68\x6e\x3a\x25\x64\x2c\x20\x41\x74\x74\x65\x6e\x3a\x30\x78\x25\x30\x32\x78" "\n"
,mapChn COMMA estResult[0]);}if(bWrComp){pstCEQData->estResultComp=estResult[1]&
0x3f;
pstCEQData->compFact=estResultFact[1];if(pstCEQData->compFact>1200){page=
PR1000_REG_PAGE_COMMON;i2cReg=0x13+PR1000_OFFSETADDR_CEQ_INFO_CH(prChn);ret=
PR1000_PageWrite(fd,i2cSlaveAddr,page,i2cReg,(uint8_t)estResult[1]);Print(
"\x45\x51\x20\x67\x61\x69\x6e\x20\x6d\x61\x70\x43\x68\x6e\x3a\x25\x64\x2c\x20\x43\x6f\x6d\x70\x3a\x30\x78\x25\x30\x32\x78" "\n"
,mapChn COMMA estResult[1]);}else{uint16_t distance[2];uint8_t distResult[2];
Print(
"\x6d\x61\x70\x43\x68\x6e\x3a\x25\x64\x20\x63\x6f\x6d\x70\x3a\x25\x64\x6d\x2c\x20\x61\x74\x74\x65\x6e\x3a\x25\x64\x6d" "\n"
,mapChn COMMA pstCEQData->compFact COMMA pstCEQData->attenFact);if(pstCEQData->
compFact>=pstCEQData->attenFact){distance[0]=pstCEQData->compFact;distance[1]=
pstCEQData->attenFact;
Print(
"\x6d\x61\x70\x43\x68\x6e\x3a\x25\x64\x20\x67\x65\x74\x20\x66\x72\x6f\x6d\x20\x61\x74\x74\x65\x6e\x3a\x25\x64\x6d" "\n"
,mapChn COMMA pstCEQData->attenFact);}else{distance[1]=pstCEQData->compFact;
distance[0]=pstCEQData->attenFact;Print(
"\x6d\x61\x70\x43\x68\x6e\x3a\x25\x64\x20\x67\x65\x74\x20\x66\x72\x6f\x6d\x20\x63\x6f\x6d\x70\x3a\x25\x64\x6d" "\n"
,mapChn COMMA pstCEQData->compFact);}if((PR1000_CEQ_GetEQDistFactor(fd,
pstCEQData->format,pstCEQData->camResol,distance,distResult))<0){Error(
"\x49\x6e\x76\x61\x6c\x69\x64\x20\x67\x65\x74\x73\x74\x64\x64\x69\x73\x74\x2e\x20\x46\x6f\x72\x63\x65\x6c\x79\x20\x62\x79\x70\x61\x73\x73\x20\x45\x51\x2e\x20\x5b\x6d\x61\x70\x43\x68\x6e\x3a\x25\x64\x5d" "\n"
,mapChn);return(-1);}page=PR1000_REG_PAGE_COMMON;i2cReg=0x13+
PR1000_OFFSETADDR_CEQ_INFO_CH(prChn);ret=PR1000_PageWrite(fd,i2cSlaveAddr,page,
i2cReg,distResult[1]);Print(
"\x45\x51\x20\x67\x61\x69\x6e\x20\x6d\x61\x70\x43\x68\x6e\x3a\x25\x64\x2c\x20\x43\x6f\x6d\x70\x3a\x30\x78\x25\x30\x32\x78" "\n"
,mapChn COMMA distResult[1]);pstCEQData->estResultComp=distResult[1];
}}return(ret);}int PR1000_CEQ_SetVIDStd(const int fd,const _stPortChSel*
pstPortChSel,const enum _pr1000_table_format format,const enum 
_pr1000_table_inresol camResol){int ret=-1;uint8_t i2cReg=0;uint8_t i2cData=0;
int page;int mapChn;uint8_t prChip,prChn,i2cSlaveAddr;uint16_t tblData=0;enum 
_pr1000_table_outresol outResol;_PR1000_REG_TABLE_VDEC_HD*pTableHD=NULL;
_PR1000_REG_TABLE_VDEC_SD*pTableSD=NULL;
#ifndef DONT_SUPPORT_VDRESOL_EXTEND
_PR1000_REG_TABLE_VDEC_HD_EXTEND*pTableHD_EXTEND=NULL;
#endif 
if((pstPortChSel==NULL)||!ASSERT_VALID_CH(pstPortChSel->i2cSlvAddr)){ErrorString
("\x49\x6e\x76\x61\x6c\x69\x64\x20\x63\x68\x2e" "\n");return(-1);}mapChn=
pstPortChSel->chn;prChip=pstPortChSel->prChip;prChn=pstPortChSel->prChn;
i2cSlaveAddr=pstPortChSel->i2cSlvAddr;Print(
"\x45\x51\x20\x73\x65\x74\x20\x56\x49\x44\x20\x73\x74\x64\x20\x66\x6f\x72\x6d\x61\x74\x5b\x25\x73\x5d\x2c\x20\x72\x65\x73\x6f\x6c\x5b\x25\x73\x5d\x20\x6d\x61\x70\x43\x68\x6e\x3a\x25\x64" "\n"
,_STR_PR1000_FORMAT[format]COMMA _STR_PR1000_INRESOL[camResol]COMMA mapChn);
switch(format){case pr1000_format_SD720:case pr1000_format_SD960:{pTableSD=(
_PR1000_REG_TABLE_VDEC_SD*)pr1000_reg_table_vdec_SD;}break;
#ifndef DONT_SUPPORT_STD_PVI
case pr1000_format_PVI:{pTableHD=(_PR1000_REG_TABLE_VDEC_HD*)
pr1000_reg_table_vdec_PVI;
#ifndef DONT_SUPPORT_VDRESOL_EXTEND
pTableHD_EXTEND=(_PR1000_REG_TABLE_VDEC_HD_EXTEND*)
pr1000_reg_table_vdec_PVI_extend;
#endif 
}break;
#endif 
case pr1000_format_HDA:{pTableHD=(_PR1000_REG_TABLE_VDEC_HD*)
pr1000_reg_table_vdec_HDA;
#ifndef DONT_SUPPORT_VDRESOL_EXTEND
pTableHD_EXTEND=(_PR1000_REG_TABLE_VDEC_HD_EXTEND*)
pr1000_reg_table_vdec_HDA_extend;
#endif 
}break;case pr1000_format_CVI:{pTableHD=(_PR1000_REG_TABLE_VDEC_HD*)
pr1000_reg_table_vdec_CVI;
#ifndef DONT_SUPPORT_VDRESOL_EXTEND
pTableHD_EXTEND=(_PR1000_REG_TABLE_VDEC_HD_EXTEND*)
pr1000_reg_table_vdec_CVI_extend;
#endif 
}break;case pr1000_format_HDT:case pr1000_format_HDT_NEW:{pTableHD=(
_PR1000_REG_TABLE_VDEC_HD*)pr1000_reg_table_vdec_HDT;
#ifndef DONT_SUPPORT_VDRESOL_EXTEND
pTableHD_EXTEND=(_PR1000_REG_TABLE_VDEC_HD_EXTEND*)
pr1000_reg_table_vdec_HDT_extend;
#endif 
}break;default:{pTableHD=(_PR1000_REG_TABLE_VDEC_HD*)pr1000_reg_table_vdec_CVI;
#ifndef DONT_SUPPORT_VDRESOL_EXTEND
pTableHD_EXTEND=(_PR1000_REG_TABLE_VDEC_HD_EXTEND*)
pr1000_reg_table_vdec_CVI_extend;
#endif 
}break;}switch(camResol){case pr1000_inresol_ntsc:{outResol=
pr1000_outresol_720x480i60;}break;case pr1000_inresol_pal:{outResol=
pr1000_outresol_720x576i50;}break;case pr1000_inresol_1280x720p60:{outResol=
pr1000_outresol_1280x720p60;}break;case pr1000_inresol_1280x720p50:{outResol=
pr1000_outresol_1280x720p50;}break;case pr1000_inresol_1280x720p30:{outResol=
pr1000_outresol_1280x720p30;}break;case pr1000_inresol_1280x720p25:{outResol=
pr1000_outresol_1280x720p25;}break;case pr1000_inresol_1920x1080p30:{outResol=
pr1000_outresol_1920x1080p30;}break;case pr1000_inresol_1920x1080p25:{outResol=
pr1000_outresol_1920x1080p25;}break;default:{outResol=
pr1000_outresol_1920x1080p30;}break;}if(format<=pr1000_format_SD960)
{while(pTableSD->addr!=0xff){i2cReg=pTableSD->addr;tblData=pTableSD->pData[
outResol];pTableSD++;if(tblData==0xff00)continue;
else if(tblData==0xffff)break;
else{page=tblData>>8;if(page==PR1000_REG_PAGE_VDEC1){if(((i2cReg>=0x11)&&(i2cReg
<=0x1f))||((i2cReg==0x37)||(i2cReg==0x3a)||(i2cReg==0x3e))||((i2cReg>=0x40)&&(
i2cReg<=0x43))||((i2cReg>=0x46)&&(i2cReg<=0x49))){if(i2cReg==0x46)tblData|=0x20;
page=PR1000_VDEC_PAGE(prChn);i2cReg+=PR1000_OFFSETADDR_VDEC_1_2PAGE_CH(prChn);
Dbg(
"\x53\x44\x20\x57\x72\x69\x74\x65\x20\x5b\x70\x25\x64\x2c\x20\x30\x78\x25\x30\x32\x78\x2d\x30\x78\x25\x30\x34\x78\x5d" "\n"
,page COMMA i2cReg COMMA tblData);if((ret=PR1000_PageWrite(fd,i2cSlaveAddr,page,
i2cReg,(uint8_t)tblData))<0){ErrorString(
"\x57\x72\x69\x74\x65\x20\x72\x65\x67\x2e" "\n");return(-1);}}}}}}else
{if(outResol<=pr1000_outresol_1920x1080p25){while(pTableHD->addr!=0xff){i2cReg=
pTableHD->addr;tblData=pTableHD->pData[outResol-pr1000_outresol_1280x720p60];
pTableHD++;if(tblData==0xff00)continue;
else if(tblData==0xffff)break;
else{page=tblData>>8;if(page==PR1000_REG_PAGE_VDEC1){if(((i2cReg>=0x11)&&(i2cReg
<=0x1f))||((i2cReg==0x37)||(i2cReg==0x3a)||(i2cReg==0x3e))||((i2cReg>=0x40)&&(
i2cReg<=0x43))||((i2cReg>=0x46)&&(i2cReg<=0x49))){if(i2cReg==0x46)tblData|=0x20;
page=PR1000_VDEC_PAGE(prChn);i2cReg+=PR1000_OFFSETADDR_VDEC_1_2PAGE_CH(prChn);
Dbg(
"\x48\x44\x20\x57\x72\x69\x74\x65\x20\x5b\x70\x25\x64\x2c\x20\x30\x78\x25\x30\x32\x78\x2d\x30\x78\x25\x30\x34\x78\x5d" "\n"
,page COMMA i2cReg COMMA tblData);if((ret=PR1000_PageWrite(fd,i2cSlaveAddr,page,
i2cReg,(uint8_t)tblData))<0){ErrorString(
"\x57\x72\x69\x74\x65\x20\x72\x65\x67\x2e" "\n");return(-1);}}}}}}
#ifndef DONT_SUPPORT_VDRESOL_EXTEND
else{while(pTableHD_EXTEND->addr!=0xff){i2cReg=pTableHD_EXTEND->addr;tblData=
pTableHD_EXTEND->pData[outResol-pr1000_outresol_1280x720p60c];pTableHD_EXTEND++;
if(tblData==0xff00)continue;
else if(tblData==0xffff)break;
else{page=tblData>>8;if(page==PR1000_REG_PAGE_VDEC1){if(((i2cReg>=0x11)&&(i2cReg
<=0x1f))||((i2cReg==0x37)||(i2cReg==0x3a)||(i2cReg==0x3e))||((i2cReg>=0x40)&&(
i2cReg<=0x43))||((i2cReg>=0x46)&&(i2cReg<=0x49))){if(i2cReg==0x46)tblData|=0x20;
page=PR1000_VDEC_PAGE(prChn);i2cReg+=PR1000_OFFSETADDR_VDEC_1_2PAGE_CH(prChn);
Dbg(
"\x48\x44\x20\x57\x72\x69\x74\x65\x20\x5b\x70\x25\x64\x2c\x20\x30\x78\x25\x30\x32\x78\x2d\x30\x78\x25\x30\x34\x78\x5d" "\n"
,page COMMA i2cReg COMMA tblData);if((ret=PR1000_PageWrite(fd,i2cSlaveAddr,page,
i2cReg,(uint8_t)tblData))<0){ErrorString(
"\x57\x72\x69\x74\x65\x20\x72\x65\x67\x2e" "\n");return(-1);}}}}}}
#endif 
}{page=PR1000_VDEC_PAGE(prChn);i2cReg=0x61+PR1000_OFFSETADDR_VDEC_1_2PAGE_CH(
prChn);i2cData=0x0e;Dbg(
"\x57\x72\x69\x74\x65\x20\x5b\x70\x25\x64\x2c\x20\x30\x78\x25\x30\x32\x78\x2d\x30\x78\x25\x30\x34\x78\x5d" "\n"
,page COMMA i2cReg COMMA i2cData);if((ret=PR1000_PageWrite(fd,i2cSlaveAddr,page,
i2cReg,i2cData))<0){ErrorString("\x57\x72\x69\x74\x65\x20\x72\x65\x67\x2e" "\n")
;return(-1);}page=PR1000_VDEC_PAGE(prChn);i2cReg=0x61+
PR1000_OFFSETADDR_VDEC_1_2PAGE_CH(prChn);i2cData=0x0f;Dbg(
"\x57\x72\x69\x74\x65\x20\x5b\x70\x25\x64\x2c\x20\x30\x78\x25\x30\x32\x78\x2d\x30\x78\x25\x30\x34\x78\x5d" "\n"
,page COMMA i2cReg COMMA i2cData);if((ret=PR1000_PageWrite(fd,i2cSlaveAddr,page,
i2cReg,i2cData))<0){ErrorString("\x57\x72\x69\x74\x65\x20\x72\x65\x67\x2e" "\n")
;return(-1);}}return(ret);}int PR1000_CEQ_SetVIDHDA_CUNLOCK(const int fd,const 
_stPortChSel*pstPortChSel,const enum _pr1000_table_inresol camResol){int ret=-1;
uint8_t i2cReg=0;uint8_t i2cData=0;int page;int mapChn;uint8_t prChip,prChn,
i2cSlaveAddr;uint16_t tblData=0;enum _pr1000_table_outresol outResol;
_PR1000_REG_TABLE_VDEC_HD*pTableHD=NULL;
#ifndef DONT_SUPPORT_VDRESOL_EXTEND
_PR1000_REG_TABLE_VDEC_HD_EXTEND*pTableHD_EXTEND=NULL;
#endif 
if((pstPortChSel==NULL)||!ASSERT_VALID_CH(pstPortChSel->i2cSlvAddr)){ErrorString
("\x49\x6e\x76\x61\x6c\x69\x64\x20\x63\x68\x2e" "\n");return(-1);}mapChn=
pstPortChSel->chn;prChip=pstPortChSel->prChip;prChn=pstPortChSel->prChn;
i2cSlaveAddr=pstPortChSel->i2cSlvAddr;Print(
"\x45\x51\x20\x73\x65\x74\x20\x56\x49\x44\x20\x73\x74\x64\x20\x48\x44\x41\x2c\x20\x72\x65\x73\x6f\x6c\x5b\x25\x73\x5d\x20\x6d\x61\x70\x43\x68\x6e\x3a\x25\x64" "\n"
,_STR_PR1000_INRESOL[camResol]COMMA mapChn);pTableHD=(_PR1000_REG_TABLE_VDEC_HD*
)pr1000_reg_table_vdec_HDA;
#ifndef DONT_SUPPORT_VDRESOL_EXTEND
pTableHD_EXTEND=(_PR1000_REG_TABLE_VDEC_HD_EXTEND*)
pr1000_reg_table_vdec_HDA_extend;
#endif 
switch(camResol){case pr1000_inresol_1280x720p60:{outResol=
pr1000_outresol_1280x720p60;}break;case pr1000_inresol_1280x720p50:{outResol=
pr1000_outresol_1280x720p50;}break;case pr1000_inresol_1280x720p30:{outResol=
pr1000_outresol_1280x720p30;}break;case pr1000_inresol_1280x720p25:{outResol=
pr1000_outresol_1280x720p25;}break;case pr1000_inresol_1920x1080p30:{outResol=
pr1000_outresol_1920x1080p30;}break;case pr1000_inresol_1920x1080p25:{outResol=
pr1000_outresol_1920x1080p25;}break;default:return(-1);}if(outResol<=
pr1000_outresol_1920x1080p25){while(pTableHD->addr!=0xff){i2cReg=pTableHD->addr;
tblData=pTableHD->pData[outResol-pr1000_outresol_1280x720p60];pTableHD++;if(
tblData==0xff00)continue;
else if(tblData==0xffff)break;
else{page=tblData>>8;if(page==PR1000_REG_PAGE_VDEC1){if((i2cReg>=0x17)&&(i2cReg
<=0x1f)){page=PR1000_VDEC_PAGE(prChn);i2cReg+=PR1000_OFFSETADDR_VDEC_1_2PAGE_CH(
prChn);Dbg(
"\x48\x44\x20\x57\x72\x69\x74\x65\x20\x5b\x70\x25\x64\x2c\x20\x30\x78\x25\x30\x32\x78\x2d\x30\x78\x25\x30\x34\x78\x5d" "\n"
,page COMMA i2cReg COMMA tblData);if((ret=PR1000_PageWrite(fd,i2cSlaveAddr,page,
i2cReg,(uint8_t)tblData))<0){ErrorString(
"\x57\x72\x69\x74\x65\x20\x72\x65\x67\x2e" "\n");return(-1);}}}}}}
#ifndef DONT_SUPPORT_VDRESOL_EXTEND
else{while(pTableHD_EXTEND->addr!=0xff){i2cReg=pTableHD_EXTEND->addr;tblData=
pTableHD_EXTEND->pData[outResol-pr1000_outresol_1280x720p60c];pTableHD_EXTEND++;
if(tblData==0xff00)continue;
else if(tblData==0xffff)break;
else{page=tblData>>8;if(page==PR1000_REG_PAGE_VDEC1){if((i2cReg>=0x17)&&(i2cReg
<=0x1f)){page=PR1000_VDEC_PAGE(prChn);i2cReg+=PR1000_OFFSETADDR_VDEC_1_2PAGE_CH(
prChn);Dbg(
"\x48\x44\x20\x57\x72\x69\x74\x65\x20\x5b\x70\x25\x64\x2c\x20\x30\x78\x25\x30\x32\x78\x2d\x30\x78\x25\x30\x34\x78\x5d" "\n"
,page COMMA i2cReg COMMA tblData);if((ret=PR1000_PageWrite(fd,i2cSlaveAddr,page,
i2cReg,(uint8_t)tblData))<0){ErrorString(
"\x57\x72\x69\x74\x65\x20\x72\x65\x67\x2e" "\n");return(-1);}}}}}}
#endif 
switch(camResol){case pr1000_inresol_1280x720p60:{i2cReg=0x46;i2cData=0x64;page=
PR1000_VDEC_PAGE(prChn);i2cReg+=PR1000_OFFSETADDR_VDEC_1_2PAGE_CH(prChn);if((ret
=PR1000_PageWrite(fd,i2cSlaveAddr,page,i2cReg,i2cData))<0){ErrorString(
"\x57\x72\x69\x74\x65\x20\x72\x65\x67\x2e" "\n");return(-1);}i2cReg=0x47;i2cData
=0xef;page=PR1000_VDEC_PAGE(prChn);i2cReg+=PR1000_OFFSETADDR_VDEC_1_2PAGE_CH(
prChn);if((ret=PR1000_PageWrite(fd,i2cSlaveAddr,page,i2cReg,i2cData))<0){
ErrorString("\x57\x72\x69\x74\x65\x20\x72\x65\x67\x2e" "\n");return(-1);}i2cReg=
0x48;i2cData=0x8e;page=PR1000_VDEC_PAGE(prChn);i2cReg+=
PR1000_OFFSETADDR_VDEC_1_2PAGE_CH(prChn);if((ret=PR1000_PageWrite(fd,
i2cSlaveAddr,page,i2cReg,i2cData))<0){ErrorString(
"\x57\x72\x69\x74\x65\x20\x72\x65\x67\x2e" "\n");return(-1);}}break;case 
pr1000_inresol_1280x720p50:{i2cReg=0x46;i2cData=0x64;page=PR1000_VDEC_PAGE(prChn
);i2cReg+=PR1000_OFFSETADDR_VDEC_1_2PAGE_CH(prChn);if((ret=PR1000_PageWrite(fd,
i2cSlaveAddr,page,i2cReg,i2cData))<0){ErrorString(
"\x57\x72\x69\x74\x65\x20\x72\x65\x67\x2e" "\n");return(-1);}i2cReg=0x47;i2cData
=0xef;page=PR1000_VDEC_PAGE(prChn);i2cReg+=PR1000_OFFSETADDR_VDEC_1_2PAGE_CH(
prChn);if((ret=PR1000_PageWrite(fd,i2cSlaveAddr,page,i2cReg,i2cData))<0){
ErrorString("\x57\x72\x69\x74\x65\x20\x72\x65\x67\x2e" "\n");return(-1);}i2cReg=
0x48;i2cData=0xf8;page=PR1000_VDEC_PAGE(prChn);i2cReg+=
PR1000_OFFSETADDR_VDEC_1_2PAGE_CH(prChn);if((ret=PR1000_PageWrite(fd,
i2cSlaveAddr,page,i2cReg,i2cData))<0){ErrorString(
"\x57\x72\x69\x74\x65\x20\x72\x65\x67\x2e" "\n");return(-1);}}break;case 
pr1000_inresol_1280x720p30:{i2cReg=0x46;i2cData=0x64;page=PR1000_VDEC_PAGE(prChn
);i2cReg+=PR1000_OFFSETADDR_VDEC_1_2PAGE_CH(prChn);if((ret=PR1000_PageWrite(fd,
i2cSlaveAddr,page,i2cReg,i2cData))<0){ErrorString(
"\x57\x72\x69\x74\x65\x20\x72\x65\x67\x2e" "\n");return(-1);}i2cReg=0x47;i2cData
=0xee;page=PR1000_VDEC_PAGE(prChn);i2cReg+=PR1000_OFFSETADDR_VDEC_1_2PAGE_CH(
prChn);if((ret=PR1000_PageWrite(fd,i2cSlaveAddr,page,i2cReg,i2cData))<0){
ErrorString("\x57\x72\x69\x74\x65\x20\x72\x65\x67\x2e" "\n");return(-1);}i2cReg=
0x48;i2cData=0x50;page=PR1000_VDEC_PAGE(prChn);i2cReg+=
PR1000_OFFSETADDR_VDEC_1_2PAGE_CH(prChn);if((ret=PR1000_PageWrite(fd,
i2cSlaveAddr,page,i2cReg,i2cData))<0){ErrorString(
"\x57\x72\x69\x74\x65\x20\x72\x65\x67\x2e" "\n");return(-1);}}break;case 
pr1000_inresol_1280x720p25:{i2cReg=0x46;i2cData=0x64;page=PR1000_VDEC_PAGE(prChn
);i2cReg+=PR1000_OFFSETADDR_VDEC_1_2PAGE_CH(prChn);if((ret=PR1000_PageWrite(fd,
i2cSlaveAddr,page,i2cReg,i2cData))<0){ErrorString(
"\x57\x72\x69\x74\x65\x20\x72\x65\x67\x2e" "\n");return(-1);}i2cReg=0x47;i2cData
=0xf1;page=PR1000_VDEC_PAGE(prChn);i2cReg+=PR1000_OFFSETADDR_VDEC_1_2PAGE_CH(
prChn);if((ret=PR1000_PageWrite(fd,i2cSlaveAddr,page,i2cReg,i2cData))<0){
ErrorString("\x57\x72\x69\x74\x65\x20\x72\x65\x67\x2e" "\n");return(-1);}i2cReg=
0x48;i2cData=0x01;page=PR1000_VDEC_PAGE(prChn);i2cReg+=
PR1000_OFFSETADDR_VDEC_1_2PAGE_CH(prChn);if((ret=PR1000_PageWrite(fd,
i2cSlaveAddr,page,i2cReg,i2cData))<0){ErrorString(
"\x57\x72\x69\x74\x65\x20\x72\x65\x67\x2e" "\n");return(-1);}}break;case 
pr1000_inresol_1920x1080p30:{i2cReg=0x46;i2cData=0x6a;page=PR1000_VDEC_PAGE(
prChn);i2cReg+=PR1000_OFFSETADDR_VDEC_1_2PAGE_CH(prChn);if((ret=PR1000_PageWrite
(fd,i2cSlaveAddr,page,i2cReg,i2cData))<0){ErrorString(
"\x57\x72\x69\x74\x65\x20\x72\x65\x67\x2e" "\n");return(-1);}i2cReg=0x47;i2cData
=0x59;page=PR1000_VDEC_PAGE(prChn);i2cReg+=PR1000_OFFSETADDR_VDEC_1_2PAGE_CH(
prChn);if((ret=PR1000_PageWrite(fd,i2cSlaveAddr,page,i2cReg,i2cData))<0){
ErrorString("\x57\x72\x69\x74\x65\x20\x72\x65\x67\x2e" "\n");return(-1);}i2cReg=
0x48;i2cData=0x5e;page=PR1000_VDEC_PAGE(prChn);i2cReg+=
PR1000_OFFSETADDR_VDEC_1_2PAGE_CH(prChn);if((ret=PR1000_PageWrite(fd,
i2cSlaveAddr,page,i2cReg,i2cData))<0){ErrorString(
"\x57\x72\x69\x74\x65\x20\x72\x65\x67\x2e" "\n");return(-1);}}break;case 
pr1000_inresol_1920x1080p25:{i2cReg=0x46;i2cData=0x6a;page=PR1000_VDEC_PAGE(
prChn);i2cReg+=PR1000_OFFSETADDR_VDEC_1_2PAGE_CH(prChn);if((ret=PR1000_PageWrite
(fd,i2cSlaveAddr,page,i2cReg,i2cData))<0){ErrorString(
"\x57\x72\x69\x74\x65\x20\x72\x65\x67\x2e" "\n");return(-1);}i2cReg=0x47;i2cData
=0x58;page=PR1000_VDEC_PAGE(prChn);i2cReg+=PR1000_OFFSETADDR_VDEC_1_2PAGE_CH(
prChn);if((ret=PR1000_PageWrite(fd,i2cSlaveAddr,page,i2cReg,i2cData))<0){
ErrorString("\x57\x72\x69\x74\x65\x20\x72\x65\x67\x2e" "\n");return(-1);}i2cReg=
0x48;i2cData=0x70;page=PR1000_VDEC_PAGE(prChn);i2cReg+=
PR1000_OFFSETADDR_VDEC_1_2PAGE_CH(prChn);if((ret=PR1000_PageWrite(fd,
i2cSlaveAddr,page,i2cReg,i2cData))<0){ErrorString(
"\x57\x72\x69\x74\x65\x20\x72\x65\x67\x2e" "\n");return(-1);}}break;default:
return(-1);}{page=PR1000_VDEC_PAGE(prChn);i2cReg=0x61+
PR1000_OFFSETADDR_VDEC_1_2PAGE_CH(prChn);i2cData=0x0e;Dbg(
"\x57\x72\x69\x74\x65\x20\x5b\x70\x25\x64\x2c\x20\x30\x78\x25\x30\x32\x78\x2d\x30\x78\x25\x30\x34\x78\x5d" "\n"
,page COMMA i2cReg COMMA i2cData);if((ret=PR1000_PageWrite(fd,i2cSlaveAddr,page,
i2cReg,i2cData))<0){ErrorString("\x57\x72\x69\x74\x65\x20\x72\x65\x67\x2e" "\n")
;return(-1);}page=PR1000_VDEC_PAGE(prChn);i2cReg=0x61+
PR1000_OFFSETADDR_VDEC_1_2PAGE_CH(prChn);i2cData=0x0f;Dbg(
"\x57\x72\x69\x74\x65\x20\x5b\x70\x25\x64\x2c\x20\x30\x78\x25\x30\x32\x78\x2d\x30\x78\x25\x30\x34\x78\x5d" "\n"
,page COMMA i2cReg COMMA i2cData);if((ret=PR1000_PageWrite(fd,i2cSlaveAddr,page,
i2cReg,i2cData))<0){ErrorString("\x57\x72\x69\x74\x65\x20\x72\x65\x67\x2e" "\n")
;return(-1);}}return(ret);}int PR1000_CEQ_SetVIDCVI_CUNLOCK(const int fd,const 
_stPortChSel*pstPortChSel,const enum _pr1000_table_inresol camResol){int ret=-1;
uint8_t i2cReg=0;uint8_t i2cData=0;int page;int mapChn;uint8_t prChip,prChn,
i2cSlaveAddr;uint16_t tblData=0;enum _pr1000_table_outresol outResol;
_PR1000_REG_TABLE_VDEC_HD*pTableHD=NULL;
#ifndef DONT_SUPPORT_VDRESOL_EXTEND
_PR1000_REG_TABLE_VDEC_HD_EXTEND*pTableHD_EXTEND=NULL;
#endif 
if((pstPortChSel==NULL)||!ASSERT_VALID_CH(pstPortChSel->i2cSlvAddr)){ErrorString
("\x49\x6e\x76\x61\x6c\x69\x64\x20\x63\x68\x2e" "\n");return(-1);}mapChn=
pstPortChSel->chn;prChip=pstPortChSel->prChip;prChn=pstPortChSel->prChn;
i2cSlaveAddr=pstPortChSel->i2cSlvAddr;Print(
"\x45\x51\x20\x73\x65\x74\x20\x56\x49\x44\x20\x73\x74\x64\x20\x43\x56\x49\x2c\x20\x72\x65\x73\x6f\x6c\x5b\x25\x73\x5d\x20\x6d\x61\x70\x43\x68\x6e\x3a\x25\x64" "\n"
,_STR_PR1000_INRESOL[camResol]COMMA mapChn);pTableHD=(_PR1000_REG_TABLE_VDEC_HD*
)pr1000_reg_table_vdec_CVI;
#ifndef DONT_SUPPORT_VDRESOL_EXTEND
pTableHD_EXTEND=(_PR1000_REG_TABLE_VDEC_HD_EXTEND*)
pr1000_reg_table_vdec_CVI_extend;
#endif 
switch(camResol){case pr1000_inresol_1280x720p60:{outResol=
pr1000_outresol_1280x720p60;}break;case pr1000_inresol_1280x720p50:{outResol=
pr1000_outresol_1280x720p50;}break;case pr1000_inresol_1280x720p30:{outResol=
pr1000_outresol_1280x720p30;}break;case pr1000_inresol_1280x720p25:{outResol=
pr1000_outresol_1280x720p25;}break;case pr1000_inresol_1920x1080p30:{outResol=
pr1000_outresol_1920x1080p30;}break;case pr1000_inresol_1920x1080p25:{outResol=
pr1000_outresol_1920x1080p25;}break;default:return(-1);}if(outResol<=
pr1000_outresol_1920x1080p25){while(pTableHD->addr!=0xff){i2cReg=pTableHD->addr;
tblData=pTableHD->pData[outResol-pr1000_outresol_1280x720p60];pTableHD++;if(
tblData==0xff00)continue;
else if(tblData==0xffff)break;
else{page=tblData>>8;if(page==PR1000_REG_PAGE_VDEC1){if(((i2cReg>=0x17)&&(i2cReg
<=0x1f))||((i2cReg>=0x46)&&(i2cReg<=0x48))){if(i2cReg==0x46)tblData|=0x20;page=
PR1000_VDEC_PAGE(prChn);i2cReg+=PR1000_OFFSETADDR_VDEC_1_2PAGE_CH(prChn);Dbg(
"\x48\x44\x20\x57\x72\x69\x74\x65\x20\x5b\x70\x25\x64\x2c\x20\x30\x78\x25\x30\x32\x78\x2d\x30\x78\x25\x30\x34\x78\x5d" "\n"
,page COMMA i2cReg COMMA tblData);if((ret=PR1000_PageWrite(fd,i2cSlaveAddr,page,
i2cReg,(uint8_t)tblData))<0){ErrorString(
"\x57\x72\x69\x74\x65\x20\x72\x65\x67\x2e" "\n");return(-1);}}}}}}
#ifndef DONT_SUPPORT_VDRESOL_EXTEND
else{while(pTableHD_EXTEND->addr!=0xff){i2cReg=pTableHD_EXTEND->addr;tblData=
pTableHD_EXTEND->pData[outResol-pr1000_outresol_1280x720p60c];pTableHD_EXTEND++;
if(tblData==0xff00)continue;
else if(tblData==0xffff)break;
else{page=tblData>>8;if(page==PR1000_REG_PAGE_VDEC1){if(((i2cReg>=0x17)&&(i2cReg
<=0x1f))||((i2cReg>=0x46)&&(i2cReg<=0x48))){if(i2cReg==0x46)tblData|=0x20;page=
PR1000_VDEC_PAGE(prChn);i2cReg+=PR1000_OFFSETADDR_VDEC_1_2PAGE_CH(prChn);Dbg(
"\x48\x44\x20\x57\x72\x69\x74\x65\x20\x5b\x70\x25\x64\x2c\x20\x30\x78\x25\x30\x32\x78\x2d\x30\x78\x25\x30\x34\x78\x5d" "\n"
,page COMMA i2cReg COMMA tblData);if((ret=PR1000_PageWrite(fd,i2cSlaveAddr,page,
i2cReg,(uint8_t)tblData))<0){ErrorString(
"\x57\x72\x69\x74\x65\x20\x72\x65\x67\x2e" "\n");return(-1);}}}}}}
#endif 
{page=PR1000_VDEC_PAGE(prChn);i2cReg=0x61+PR1000_OFFSETADDR_VDEC_1_2PAGE_CH(
prChn);i2cData=0x0e;Dbg(
"\x57\x72\x69\x74\x65\x20\x5b\x70\x25\x64\x2c\x20\x30\x78\x25\x30\x32\x78\x2d\x30\x78\x25\x30\x34\x78\x5d" "\n"
,page COMMA i2cReg COMMA i2cData);if((ret=PR1000_PageWrite(fd,i2cSlaveAddr,page,
i2cReg,i2cData))<0){ErrorString("\x57\x72\x69\x74\x65\x20\x72\x65\x67\x2e" "\n")
;return(-1);}page=PR1000_VDEC_PAGE(prChn);i2cReg=0x61+
PR1000_OFFSETADDR_VDEC_1_2PAGE_CH(prChn);i2cData=0x0f;Dbg(
"\x57\x72\x69\x74\x65\x20\x5b\x70\x25\x64\x2c\x20\x30\x78\x25\x30\x32\x78\x2d\x30\x78\x25\x30\x34\x78\x5d" "\n"
,page COMMA i2cReg COMMA i2cData);if((ret=PR1000_PageWrite(fd,i2cSlaveAddr,page,
i2cReg,i2cData))<0){ErrorString("\x57\x72\x69\x74\x65\x20\x72\x65\x67\x2e" "\n")
;return(-1);}}return(ret);}int PR1000_CEQ_SetVADCGain(const int fd,const enum 
_pr1000_table_format format,const enum _pr1000_table_inresol camResol,
_stPortChSel*pstPortChSel,_stCEQData*pstCEQData){int ret=-1;uint8_t i2cReg=0;
uint8_t i2cData=0;uint8_t i2cMask=0;int page;int mapChn;uint8_t prChip,prChn,
i2cSlaveAddr;uint8_t vadcGain=0;uint16_t dcGain=0;uint8_t gainfit=0;uint8_t 
y_out_gain=0;if((pstPortChSel==NULL)||!ASSERT_VALID_CH(pstPortChSel->i2cSlvAddr)
||(pstCEQData==NULL)){ErrorString(
"\x49\x6e\x76\x61\x6c\x69\x64\x20\x63\x68\x2e" "\n");return(-1);}if(pstCEQData->
saved_det_eq_dcgain==0){ErrorString(
"\x49\x6e\x76\x61\x6c\x69\x64\x20\x64\x65\x74\x20\x65\x71\x20\x64\x63\x67\x61\x69\x6e\x2e" "\n"
);return(-1);}mapChn=pstPortChSel->chn;prChip=pstPortChSel->prChip;prChn=
pstPortChSel->prChn;i2cSlaveAddr=pstPortChSel->i2cSlvAddr;vadcGain=0;dcGain=
pstCEQData->saved_det_eq_dcgain;Print(
"\x45\x51\x20\x73\x65\x74\x20\x56\x41\x44\x43\x20\x67\x61\x69\x6e\x20\x66\x6f\x72\x6d\x61\x74\x5b\x25\x73\x5d\x2c\x20\x72\x65\x73\x6f\x6c\x5b\x25\x73\x5d\x20\x6d\x61\x70\x43\x68\x6e\x3a\x25\x64" "\n"
,_STR_PR1000_FORMAT[format]COMMA _STR_PR1000_INRESOL[camResol]COMMA mapChn);if((
pstCEQData->compFact<1400)){if(pstCEQData->attenFact<100){vadcGain=0x00;gainfit=
0x61;
y_out_gain=0xc5;
}else if(pstCEQData->attenFact<200){if(dcGain<=0x2e00)vadcGain=0x01;else 
vadcGain=0x02;gainfit=0x61;
y_out_gain=0xc5;
}else if(pstCEQData->attenFact<300){if(dcGain<=0x2e00)vadcGain=0x02;else 
vadcGain=0x03;gainfit=0x61;
y_out_gain=0xc5;
}else if(pstCEQData->attenFact<400){if(dcGain<=0x2e00)vadcGain=0x03;else 
vadcGain=0x05;gainfit=0x61;
y_out_gain=0xc5;
}else if(pstCEQData->attenFact<500){if(dcGain<=0x2e00)vadcGain=0x04;else 
vadcGain=0x06;gainfit=0x61;
y_out_gain=0xc5;
}else if(pstCEQData->attenFact<600){if(dcGain<=0x2e00)vadcGain=0x04;else 
vadcGain=0x07;gainfit=0x61;
y_out_gain=0xc5;
}else if(pstCEQData->attenFact<700){if(dcGain<=0x2e00)vadcGain=0x05;else 
vadcGain=0x07;gainfit=0x61;
y_out_gain=0xc5;
}else if(pstCEQData->attenFact<800){if(dcGain<=0x2e00)vadcGain=0x05;else 
vadcGain=0x07;gainfit=0x78;
y_out_gain=0xa0;
}else if(pstCEQData->attenFact<900){if(dcGain<=0x2e00)vadcGain=0x05;else 
vadcGain=0x07;gainfit=0x90;
y_out_gain=0x85;
}else if(pstCEQData->attenFact<1000){if(dcGain<=0x2e00)vadcGain=0x05;else 
vadcGain=0x07;gainfit=0xa0;
y_out_gain=0x85;
}else{if(dcGain<=0x2e00)vadcGain=0x05;else vadcGain=0x07;gainfit=0xa0;
y_out_gain=0x85;
}}else if((1400<=pstCEQData->compFact)&&(pstCEQData->compFact<1600)){vadcGain=
0x03;gainfit=0xa0;
y_out_gain=0x85;
}else if((1600<=pstCEQData->compFact)&&(pstCEQData->compFact<1800)){vadcGain=
0x03;gainfit=0xa0;
y_out_gain=0x85;
}else if((1800<=pstCEQData->compFact)&&(pstCEQData->compFact<2000)){vadcGain=
0x03;gainfit=0xa0;
y_out_gain=0x85;
}else if((2000<=pstCEQData->compFact)&&(pstCEQData->compFact<2200)){vadcGain=
0x04;gainfit=0xa0;
y_out_gain=0x85;
}else if((2200<=pstCEQData->compFact)&&(pstCEQData->compFact<2400)){vadcGain=
0x04;gainfit=0xa0;
y_out_gain=0x85;
}else if((2400<=pstCEQData->compFact)&&(pstCEQData->compFact<2600)){vadcGain=
0x05;gainfit=0xa0;
y_out_gain=0x85;
}else if((2600<=pstCEQData->compFact)){vadcGain=0x05;gainfit=0xa0;
y_out_gain=0x85;
}Print(
"\x53\x65\x74\x20\x56\x41\x44\x43\x20\x6d\x61\x70\x43\x68\x6e\x3a\x25\x64\x20\x5b\x41\x74\x74\x65\x6e\x3a\x25\x64\x6d\x2c\x30\x78\x25\x78\x2c\x20\x43\x6f\x6d\x70\x3a\x25\x64\x6d\x2c\x30\x78\x25\x78\x2c\x20\x56\x61\x64\x63\x3a\x30\x78\x25\x78\x2c\x20\x64\x63\x67\x61\x69\x6e\x3a\x30\x78\x25\x30\x34\x78\x5d" "\n"
,mapChn COMMA pstCEQData->attenFact COMMA pstCEQData->estResultAtten COMMA 
pstCEQData->compFact COMMA pstCEQData->estResultComp COMMA vadcGain COMMA dcGain
);
i2cReg=0x11+PR1000_OFFSETADDR_CEQ_INFO_CH(prChn);i2cMask=0x70;i2cData=vadcGain<<
4;if((ret=PR1000_WriteMaskBit(fd,i2cSlaveAddr,PR1000_REG_PAGE_COMMON,i2cReg,
i2cMask,i2cData))<0){ErrorString("\x57\x72\x69\x74\x65\x20\x72\x65\x67\x2e" "\n"
);return(-1);}pstCEQData->vadcGain=vadcGain;if(format==pr1000_format_CVI){Print(
"\x53\x70\x65\x63\x69\x61\x6c\x20\x43\x56\x49\x20\x6d\x6f\x64\x65\x20\x6d\x61\x70\x43\x68\x6e\x3a\x25\x64\x2c\x20\x67\x61\x69\x6e\x66\x69\x74\x3a\x30\x78\x25\x30\x32\x78\x2c\x20\x79\x6f\x75\x74\x67\x61\x69\x6e\x3a\x30\x78\x25\x30\x32\x78" "\n"
,mapChn COMMA gainfit COMMA y_out_gain);page=PR1000_VDEC_PAGE(prChn);i2cReg=0x01
+PR1000_OFFSETADDR_VDEC_1_2PAGE_CH(prChn);i2cData=gainfit;Dbg(
"\x57\x72\x69\x74\x65\x20\x5b\x70\x25\x64\x2c\x20\x30\x78\x25\x30\x32\x78\x2d\x30\x78\x25\x30\x32\x78\x5d" "\n"
,page COMMA i2cReg COMMA i2cData);if((ret=PR1000_PageWrite(fd,i2cSlaveAddr,page,
i2cReg,(uint8_t)i2cData))<0){ErrorString(
"\x57\x72\x69\x74\x65\x20\x72\x65\x67\x2e" "\n");}page=PR1000_VDEC_PAGE(prChn);
i2cReg=0x28+PR1000_OFFSETADDR_VDEC_1_2PAGE_CH(prChn);i2cData=y_out_gain;Dbg(
"\x57\x72\x69\x74\x65\x20\x5b\x70\x25\x64\x2c\x20\x30\x78\x25\x30\x32\x78\x2d\x30\x78\x25\x30\x32\x78\x5d" "\n"
,page COMMA i2cReg COMMA i2cData);if((ret=PR1000_PageWrite(fd,i2cSlaveAddr,page,
i2cReg,(uint8_t)i2cData))<0){ErrorString(
"\x57\x72\x69\x74\x65\x20\x72\x65\x67\x2e" "\n");}}return(ret);}int 
PR1000_MON_CheckChgVF(const int fd,_stPortChSel*pstPortChSel,_stCEQData*
pstCEQData,void*pArgu){int ret=-1;uint8_t i2cReg=0;uint8_t i2cData=0;uint8_t 
i2cMask;int mapChn;uint8_t prChn,i2cSlaveAddr;uint8_t bChangeFormat=FALSE;
_drvHost*pHost=(_drvHost*)pArgu;_stCeqDet stCeqDet,*pstCeqDetManualStd;
_stCeqLock stCeqLock;uint8_t regs[4];if((pstPortChSel==NULL)||(pstCEQData==NULL)
){ErrorString("\x49\x6e\x76\x61\x6c\x69\x64\x20\x61\x72\x67\x75" "\n");return(-1
);
}mapChn=pstPortChSel->chn;i2cSlaveAddr=pstPortChSel->i2cSlvAddr;prChn=
pstPortChSel->prChn;pstCeqDetManualStd=(_stCeqDet*)&pHost->eventHost.
stDetManualStd[mapChn];Dbg(
"\x6d\x61\x70\x43\x68\x6e\x3a\x25\x64\x20\x63\x65\x71\x45\x6e\x61\x62\x6c\x65\x3a\x25\x64\x2c\x20\x63\x65\x71\x4c\x6f\x63\x6b\x3a\x25\x64" "\n"
,mapChn COMMA pstCEQData->bEnable COMMA pstCEQData->bLock);if((pstCEQData->
bEnable)&&(pstCEQData->bLock)){i2cReg=0x00+PR1000_OFFSETADDR_CEQ_INFO_CH(prChn);
if(PR1000_PageReadBurst(fd,i2cSlaveAddr,PR1000_REG_PAGE_COMMON,i2cReg,2,regs)<0)
{ErrorString("\x52\x65\x61\x64\x20\x72\x65\x67\x2e" "\n");return(-1);
}stCeqDet.reg=regs[0];stCeqLock.reg=regs[1];Dbg(
"\x43\x75\x72\x20\x66\x6f\x72\x6d\x61\x74\x5b\x25\x73\x5d\x2c\x20\x72\x65\x73\x6f\x6c\x5b\x25\x73\x5d\x20\x6d\x61\x70\x43\x68\x6e\x3a\x25\x64" "\n"
,_STR_PR1000_FORMAT[pHost->eventHost.stEventDetStd[mapChn].format]COMMA 
_STR_PR1000_INRESOL[pHost->eventHost.stEventDetStd[mapChn].resol]COMMA mapChn);
Dbg(
"\x64\x65\x74\x20\x72\x65\x67\x20\x30\x78\x25\x30\x32\x78\x20\x20\x73\x74\x64\x3a\x25\x64\x2c\x72\x65\x66\x3a\x25\x64\x2c\x72\x65\x73\x3a\x25\x64" "\n"
,stCeqDet.reg COMMA stCeqDet.b.det_ifmt_std COMMA stCeqDet.b.det_ifmt_ref COMMA 
stCeqDet.b.det_ifmt_res);if(((pHost->eventHost.stEventDetStd[mapChn].format==
pr1000_format_HDT)||(pHost->eventHost.stEventDetStd[mapChn].format==
pr1000_format_HDT_NEW))&&((pHost->eventHost.stEventDetStd[mapChn].resol==
pr1000_inresol_1280x720p25)||(pHost->eventHost.stEventDetStd[mapChn].resol==
pr1000_inresol_1280x720p30))){bChangeFormat=FALSE;if((stCeqDet.b.det_ifmt_std!=
PR1000_DET_IFMT_STD_HDT)){bChangeFormat=TRUE;}else if(pHost->eventHost.
stEventDetStd[mapChn].resol==pr1000_inresol_1280x720p25){if((stCeqDet.b.
det_ifmt_ref!=PR1000_DET_IFMT_REF_25)||(stCeqDet.b.det_ifmt_res!=
PR1000_DET_IFMT_RES_720p)){bChangeFormat=TRUE;}}else if(pHost->eventHost.
stEventDetStd[mapChn].resol==pr1000_inresol_1280x720p30){if((stCeqDet.b.
det_ifmt_ref!=PR1000_DET_IFMT_REF_30)||(stCeqDet.b.det_ifmt_res!=
PR1000_DET_IFMT_RES_720p)){bChangeFormat=TRUE;}}if(bChangeFormat){Print(
"\x43\x68\x61\x6e\x67\x65\x20\x56\x69\x64\x65\x6f\x20\x66\x6f\x72\x6d\x61\x74\x20\x6d\x61\x70\x43\x68\x6e\x28\x25\x64\x29\x2e\x20\x49\x6e\x20\x66\x6f\x72\x6d\x61\x74\x28\x25\x64\x29" "\n"
,mapChn COMMA pHost->eventHost.stEventDetStd[mapChn].format);Print(
"\x43\x45\x51\x20\x53\x74\x6f\x70\x20\x62\x79\x20\x56\x69\x64\x65\x6f\x20\x66\x6f\x72\x6d\x61\x74\x2e\x20\x49\x6e\x74\x20\x70\x72\x43\x68\x6e\x3a\x25\x64\x2e" "\n"
,prChn);PR1000_CEQ_Stop(fd,mapChn,(void*)pHost);memset(pHost->ptzHost.
testSTDFORMATData[mapChn],0,sizeof(uint8_t)*PTZ_STDFORMAT_CHECK_CNT);pHost->
ptzHost.flagSTDFORMATResult[mapChn]=0;pHost->sysHost.cntChromaLockTunn[mapChn]=0
;_CLEAR_BIT(mapChn,&pHost->sysHost.bitChnLoadedRegTable);_CLEAR_BIT(mapChn,&
pHost->sysHost.bitChnCheckChromaLock);_CLEAR_BIT(mapChn,&pHost->sysHost.
bitChnDoneChromaLock);_CLEAR_BIT(mapChn,&pHost->sysHost.bitChnResultChromaLock);
pHost->sysHost.cntVadcGainSelTunn[mapChn]=0;_CLEAR_BIT(mapChn,&pHost->sysHost.
bitChnCheckVadcGainSel);_CLEAR_BIT(mapChn,&pHost->sysHost.bitChnDoneVadcGainSel)
;_CLEAR_BIT(mapChn,&pHost->sysHost.bitChnResultVadcGainSel);_CLEAR_BIT(mapChn,&
pHost->sysHost.bitChnApplyVadcGainSel);Dbg(
"\x6d\x61\x70\x43\x68\x6e\x3a\x25\x64\x20\x70\x6f\x6c\x6c\x56\x66\x64\x3a\x25\x6c\x78" "\n"
,mapChn COMMA pHost->wqPollChnStatus.bitWqPollVfd);return(1);
}}if(((pstCEQData->bForceChgStd==TRUE)&&(pHost->eventHost.stEventDetStd[mapChn].
format==pr1000_format_CVI))){bChangeFormat=FALSE;if((stCeqDet.b.det_ifmt_std!=
PR1000_DET_IFMT_STD_HDA)){bChangeFormat=TRUE;}else if(pHost->eventHost.
stEventDetStd[mapChn].resol==pr1000_inresol_1920x1080p25){if((stCeqDet.b.
det_ifmt_ref!=PR1000_DET_IFMT_REF_25)||(stCeqDet.b.det_ifmt_res!=
PR1000_DET_IFMT_RES_1080p)){bChangeFormat=TRUE;}}else if(pHost->eventHost.
stEventDetStd[mapChn].resol==pr1000_inresol_1920x1080p30){if((stCeqDet.b.
det_ifmt_ref!=PR1000_DET_IFMT_REF_30)||(stCeqDet.b.det_ifmt_res!=
PR1000_DET_IFMT_RES_1080p)){bChangeFormat=TRUE;}}else if(pHost->eventHost.
stEventDetStd[mapChn].resol==pr1000_inresol_1280x720p25){if((stCeqDet.b.
det_ifmt_ref!=PR1000_DET_IFMT_REF_25)||(stCeqDet.b.det_ifmt_res!=
PR1000_DET_IFMT_RES_720p)){bChangeFormat=TRUE;}}else if(pHost->eventHost.
stEventDetStd[mapChn].resol==pr1000_inresol_1280x720p30){if((stCeqDet.b.
det_ifmt_ref!=PR1000_DET_IFMT_REF_30)||(stCeqDet.b.det_ifmt_res!=
PR1000_DET_IFMT_RES_720p)){bChangeFormat=TRUE;}}if(bChangeFormat){Print(
"\x43\x68\x61\x6e\x67\x65\x20\x56\x69\x64\x65\x6f\x20\x66\x6f\x72\x6d\x61\x74\x20\x6d\x61\x70\x43\x68\x6e\x28\x25\x64\x29\x2e\x20\x49\x6e\x20\x66\x6f\x72\x6d\x61\x74\x28\x25\x64\x29" "\n"
,mapChn COMMA pHost->eventHost.stEventDetStd[mapChn].format);Print(
"\x43\x45\x51\x20\x53\x74\x6f\x70\x20\x62\x79\x20\x56\x69\x64\x65\x6f\x20\x66\x6f\x72\x6d\x61\x74\x2e\x20\x49\x6e\x74\x20\x70\x72\x43\x68\x6e\x3a\x25\x64\x2e" "\n"
,prChn);PR1000_CEQ_Stop(fd,mapChn,(void*)pHost);memset(pHost->ptzHost.
testSTDFORMATData[mapChn],0,sizeof(uint8_t)*PTZ_STDFORMAT_CHECK_CNT);pHost->
ptzHost.flagSTDFORMATResult[mapChn]=0;pHost->sysHost.cntChromaLockTunn[mapChn]=0
;_CLEAR_BIT(mapChn,&pHost->sysHost.bitChnLoadedRegTable);_CLEAR_BIT(mapChn,&
pHost->sysHost.bitChnCheckChromaLock);_CLEAR_BIT(mapChn,&pHost->sysHost.
bitChnDoneChromaLock);_CLEAR_BIT(mapChn,&pHost->sysHost.bitChnResultChromaLock);
pHost->sysHost.cntVadcGainSelTunn[mapChn]=0;_CLEAR_BIT(mapChn,&pHost->sysHost.
bitChnCheckVadcGainSel);_CLEAR_BIT(mapChn,&pHost->sysHost.bitChnDoneVadcGainSel)
;_CLEAR_BIT(mapChn,&pHost->sysHost.bitChnResultVadcGainSel);_CLEAR_BIT(mapChn,&
pHost->sysHost.bitChnApplyVadcGainSel);Dbg(
"\x6d\x61\x70\x43\x68\x6e\x3a\x25\x64\x20\x70\x6f\x6c\x6c\x56\x66\x64\x3a\x25\x6c\x78" "\n"
,mapChn COMMA pHost->wqPollChnStatus.bitWqPollVfd);return(1);
}}if(((pHost->eventHost.stEventDetStd[mapChn].format==pr1000_format_HDT)||(pHost
->eventHost.stEventDetStd[mapChn].format==pr1000_format_HDT_NEW))&&((pHost->
eventHost.stEventDetStd[mapChn].resol==pr1000_inresol_1280x720p25)||(pHost->
eventHost.stEventDetStd[mapChn].resol==pr1000_inresol_1280x720p30))){if(pHost->
ptzHost.bRxUsedSTDFORMATOnly[mapChn]==TRUE)
{if(pHost->sysHost.cntSTDFORMATCheck[mapChn]<=0){Dbg(
"\x53\x74\x61\x72\x74\x20\x6d\x61\x70\x43\x68\x6e\x3a\x25\x64\x20\x53\x54\x44\x46\x4f\x52\x4d\x41\x54" "\n"
,mapChn);pHost->sysHost.cntSTDFORMATCheck[mapChn]=2;
pHost->sysHost.timeOverSTDFORMATCheck[mapChn]=2;
if(pHost->eventHost.stEventDetStd[mapChn].format==pr1000_format_HDT){if(pHost->
ptzHost.flagSTDFORMATResult[mapChn]==TRUE){Print(
"\x43\x68\x61\x6e\x67\x65\x20\x48\x44\x54\x2d\x3e\x48\x44\x54\x5f\x4e\x45\x57\x20\x6d\x61\x70\x43\x68\x6e\x3a\x25\x64" "\n"
,mapChn);PR1000_CEQ_Stop(fd,mapChn,(void*)pHost);memset(pHost->ptzHost.
testSTDFORMATData[mapChn],0,sizeof(uint8_t)*PTZ_STDFORMAT_CHECK_CNT);pHost->
ptzHost.flagSTDFORMATResult[mapChn]=0;pHost->sysHost.cntChromaLockTunn[mapChn]=0
;_CLEAR_BIT(mapChn,&pHost->sysHost.bitChnLoadedRegTable);_CLEAR_BIT(mapChn,&
pHost->sysHost.bitChnCheckChromaLock);_CLEAR_BIT(mapChn,&pHost->sysHost.
bitChnDoneChromaLock);_CLEAR_BIT(mapChn,&pHost->sysHost.bitChnResultChromaLock);
pHost->sysHost.cntVadcGainSelTunn[mapChn]=0;_CLEAR_BIT(mapChn,&pHost->sysHost.
bitChnCheckVadcGainSel);_CLEAR_BIT(mapChn,&pHost->sysHost.bitChnDoneVadcGainSel)
;_CLEAR_BIT(mapChn,&pHost->sysHost.bitChnResultVadcGainSel);_CLEAR_BIT(mapChn,&
pHost->sysHost.bitChnApplyVadcGainSel);}}else if(pHost->eventHost.stEventDetStd[
mapChn].format==pr1000_format_HDT_NEW){if(pHost->ptzHost.flagSTDFORMATResult[
mapChn]==FALSE){Print(
"\x43\x68\x61\x6e\x67\x65\x20\x48\x44\x54\x5f\x4e\x45\x57\x2d\x3e\x48\x44\x54\x20\x6d\x61\x70\x43\x68\x6e\x3a\x25\x64" "\n"
,mapChn);PR1000_CEQ_Stop(fd,mapChn,(void*)pHost);memset(pHost->ptzHost.
testSTDFORMATData[mapChn],0,sizeof(uint8_t)*PTZ_STDFORMAT_CHECK_CNT);pHost->
ptzHost.flagSTDFORMATResult[mapChn]=0;pHost->sysHost.cntChromaLockTunn[mapChn]=0
;_CLEAR_BIT(mapChn,&pHost->sysHost.bitChnLoadedRegTable);_CLEAR_BIT(mapChn,&
pHost->sysHost.bitChnCheckChromaLock);_CLEAR_BIT(mapChn,&pHost->sysHost.
bitChnDoneChromaLock);_CLEAR_BIT(mapChn,&pHost->sysHost.bitChnResultChromaLock);
pHost->sysHost.cntVadcGainSelTunn[mapChn]=0;_CLEAR_BIT(mapChn,&pHost->sysHost.
bitChnCheckVadcGainSel);_CLEAR_BIT(mapChn,&pHost->sysHost.bitChnDoneVadcGainSel)
;_CLEAR_BIT(mapChn,&pHost->sysHost.bitChnResultVadcGainSel);_CLEAR_BIT(mapChn,&
pHost->sysHost.bitChnApplyVadcGainSel);}}}i2cMask=0x08;i2cData=0x08;i2cReg=0x00+
PR1000_OFFSETADDR_PTZ_CH(prChn);PR1000_WriteMaskBit(fd,i2cSlaveAddr,
PR1000_REG_PAGE_PTZ,i2cReg,i2cMask,i2cData);i2cMask=0x80;i2cData=0x00;i2cReg=
0x1a+PR1000_OFFSETADDR_PTZ_CH(prChn);PR1000_PageRead(fd,i2cSlaveAddr,
PR1000_REG_PAGE_PTZ,i2cReg,&i2cData);if((i2cData&0x80))i2cData&=~0x80;else 
i2cData|=0x80;PR1000_PageWrite(fd,i2cSlaveAddr,PR1000_REG_PAGE_PTZ,i2cReg,
i2cData);Dbg(
"\x61\x64\x64\x72\x3a\x30\x78\x25\x30\x32\x78\x2c\x30\x78\x25\x30\x32\x78" "\n",
i2cReg COMMA i2cData);Dbg(
"\x53\x74\x61\x72\x74\x20\x50\x54\x5a\x5f\x52\x58\x20\x6d\x61\x70\x43\x68\x6e\x28\x25\x64\x29" "\n"
,mapChn);i2cMask=0x40;i2cData=0x40;i2cReg=PR1000_REG_ADDR_PTZ_RX_EN+
PR1000_OFFSETADDR_PTZ_CH(prChn);PR1000_WriteMaskBit(fd,i2cSlaveAddr,
PR1000_REG_PAGE_PTZ,i2cReg,i2cMask,i2cData);}}}ret=0;return(ret);
}

