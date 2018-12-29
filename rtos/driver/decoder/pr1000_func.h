#ifndef __PR1000_FUNC_H__
#define __PR1000_FUNC_H__

#include "pr1000.h"
#include "pr1000_drvcommon.h"
#include "pr1000_user_config.h"
#include "pr1000_table.h"

///////////////////////////////////////////////////////////////////////////
void PR1000_ChipInfo(const int fd, _drvHost *pHost);
int PR1000_TestInfeface(const int fd, _drvHost *pHost);
void InitPR1000(const int fd);
int PR1000_Init(const int fd, _drvHost *pHost);
#ifndef DONT_SUPPORT_ETC_FUNC
int PR1000_SetPwDown(const int fd, const _stPrPwDown *pstPrPwDown);
int PR1000_GetPwDown(const int fd, _stPrPwDown *pstPrPwDown);
#endif // DONT_SUPPORT_ETC_FUNC

//////////////////////////////////// IRQ /////////////////////////////////////////////
int PR1000_IRQ_Init(const int fd, _drvHost *pHost);
int PR1000_Kthread(void *arg);
int PR1000_IRQ_Isr(const int fd, _drvHost *pHost);
int PR1000_IRQ_PTZ_Isr(const int fd, const uint8_t prChip, const uint8_t prChn, const uint8_t intReg, _drvHost *pHost);
int PR1000_IRQ_NOVID_Isr(const int fd, const uint8_t prChip, const uint8_t intReg, const uint8_t intStatus, _drvHost *pHost);
int PR1000_IRQ_VFD_Isr(const int fd, const uint8_t prChip, const uint8_t intReg, const uint8_t intStatus, _drvHost *pHost);
#ifndef DONT_SUPPORT_EVENT_FUNC
int PR1000_IRQ_MD_Isr(const int fd, const uint8_t prChip, const uint8_t intReg, const uint8_t intStatus, _drvHost *pHost);
int PR1000_IRQ_BD_Isr(const int fd, const uint8_t prChip, const uint8_t intReg, const uint8_t intStatus, _drvHost *pHost);
int PR1000_IRQ_ND_Isr(const int fd, const uint8_t prChip, const uint8_t intReg, const uint8_t intStatus, _drvHost *pHost);
int PR1000_IRQ_DD_Isr(const int fd, const uint8_t prChip, const uint8_t intReg, const uint8_t intStatus, _drvHost *pHost);
int PR1000_IRQ_DFD_Isr(const int fd, const uint8_t prChip, const uint8_t intReg, const uint8_t intStatus, _drvHost *pHost);
int PR1000_IRQ_AD_DIFF_Isr(const int fd, const uint8_t prChip, const uint8_t intReg, const uint8_t intStatus, _drvHost *pHost);
int PR1000_IRQ_NOAUD_Isr(const int fd, const uint8_t prChip, const uint8_t intReg, const uint8_t intStatus, _drvHost *pHost);
int PR1000_IRQ_AD_ABS_Isr(const int fd, const uint8_t prChip, const uint8_t intReg, const uint8_t intStatus, _drvHost *pHost);
#endif // DONT_SUPPORT_EVENT_FUNC
int PR1000_IRQ_GPIO_Isr(const int fd, const uint8_t prChip, const uint8_t prChn, const uint8_t intReg, _drvHost *pHost);
int PR1000_IRQ_WAKE_Isr(const int fd, _drvHost *pHost);

//////////////////////////////////// VID /////////////////////////////////////////////
int PR1000_VID_LoadTable(const int fd, const _stPortChSel *pstPortChSel, const uint8_t bBootTime, const uint8_t format, const uint8_t resol);
int PR1000_VID_SetChnAttr(const int fd, const _stPortChSel *pstPortChSel, const _stChnAttr *pstChnAttr);
int PR1000_VID_GetChnAttr(const int fd, const _stPortChSel *pstPortChSel, _stChnAttr *pstChnAttr);
#ifndef DONT_SUPPORT_VID_ENHANCEMENT
int PR1000_VID_SetCscAttr(const int fd, const _stPortChSel *pstPortChSel, const _stCscAttr *pstCscAttr);
int PR1000_VID_GetCscAttr(const int fd, const _stPortChSel *pstPortChSel, _stCscAttr *pstCscAttr);
int PR1000_VID_SetContrast(const int fd, const _stPortChSel *pstPortChSel, const _stContrast *pstContrast);
int PR1000_VID_GetContrast(const int fd, const _stPortChSel *pstPortChSel, _stContrast *pstContrast);
int PR1000_VID_SetBright(const int fd, const _stPortChSel *pstPortChSel, const _stBright *pstBright);
int PR1000_VID_GetBright(const int fd, const _stPortChSel *pstPortChSel, _stBright *pstBright);
int PR1000_VID_SetSaturation(const int fd, const _stPortChSel *pstPortChSel, const _stSaturation *pstSaturation);
int PR1000_VID_GetSaturation(const int fd, const _stPortChSel *pstPortChSel, _stSaturation *pstSaturation);
int PR1000_VID_SetHue(const int fd, const _stPortChSel *pstPortChSel, const _stHue *pstHue);
int PR1000_VID_GetHue(const int fd, const _stPortChSel *pstPortChSel, _stHue *pstHue);
int PR1000_VID_SetSharpness(const int fd, const _stPortChSel *pstPortChSel, const _stSharpness *pstSharpness);
int PR1000_VID_GetSharpness(const int fd, const _stPortChSel *pstPortChSel, _stSharpness *pstSharpness);
int PR1000_VID_SetBlank(const int fd, const _stPortChSel *pstPortChSel, const int bEnable, const int blankColor);
#endif // DONT_SUPPORT_VID_ENHANCEMENT
int PR1000_VID_SetOutFormatAttr(const int fd, const _stOutFormatAttr *pstOutFormatAttr);
int PR1000_VID_SetOutChn(const int fd, const uint8_t chip, const _stOutChn *pstOutChn);
int PR1000_IfmtToStdResol(const int fd, const uint8_t std, const uint8_t ref, const uint8_t res, enum _pr1000_table_format *pFormat, enum _pr1000_table_inresol *pResol);
int PR1000_VID_HideEQingDisplay(const int fd, const _stPortChSel *pstPortChSel);

#ifndef DONT_SUPPORT_AUD_ALINK
//////////////////////////////////// AUDIO /////////////////////////////////////////////
int PR1000_AUD_Enable(const int fd, const uint8_t chip, const int bEnable);
int PR1000_AUD_SetAiGain(const int fd, const uint8_t chip, const _stAUDAiGain *pstAUDAiGain);
int PR1000_AUD_GetAiGain(const int fd, const uint8_t chip, _stAUDAiGain *pstAUDAiGain);
int PR1000_AUD_SetDacGain(const int fd, const uint8_t chip, const _stAUDDacGain *pstAUDDacGain);
int PR1000_AUD_GetDacGain(const int fd, const uint8_t chip, _stAUDDacGain *pstAUDDacGain);
int PR1000_AUD_SetMixMode(const int fd, const uint8_t chip, const _stAUDMixMode *pstAUDMixMode);
int PR1000_AUD_GetMixMode(const int fd, const uint8_t chip, _stAUDMixMode *pstAUDMixMode);
int PR1000_AUD_SetRecAttr(const int fd, const uint8_t chip, const _stAUDRecAttr *pstAUDRecAttr);
int PR1000_AUD_GetRecAttr(const int fd, const uint8_t chip, _stAUDRecAttr *pstAUDRecAttr);
int PR1000_AUD_SetRecMute(const int fd, const uint8_t chip, const uint8_t ch, const int bEnable);
int PR1000_AUD_SetPbAttr(const int fd, const uint8_t chip, const _stAUDPbAttr *pstAUDPbAttr);
int PR1000_AUD_GetPbAttr(const int fd, const uint8_t chip, _stAUDPbAttr *pstAUDPbAttr);
int PR1000_AUD_SetMixMute(const int fd, const uint8_t chip, const int bEnable);
int PR1000_AUD_SetVocMute(const int fd, const uint8_t chip, const int bEnable);
int PR1000_AUD_SetDacMute(const int fd, const uint8_t chip, const int bEnable);
int PR1000_AUD_SetDacChn(const int fd, const uint8_t chip, const int ch); //0~15:live, 16:Pb, 17:Voice, 18:Mix
int PR1000_AUD_SetDetAttr(const int fd, const uint8_t chip, const _stAUDDetAttr *pstAUDDetAttr);
int PR1000_AUD_GetDetAttr(const int fd, const uint8_t chip, _stAUDDetAttr *pstAUDDetAttr);
int PR1000_AUD_SetCascadeAttr(const int fd, const uint8_t chip, const _stAUDCascadeAttr *pstAUDCascadeAttr);
int PR1000_AUD_GetCascadeAttr(const int fd, const uint8_t chip, _stAUDCascadeAttr *pstAUDCascadeAttr);
#endif // DONT_SUPPORT_AUD_ALINK

//////////////////////////////////// PTZ /////////////////////////////////////////////
int PR1000_PTZ_LoadTable(const int fd, const _stPortChSel *pstPortChSel, const uint8_t format, const uint8_t resol, _drvHost *pHost);
int PR1000_PTZ_EnableRXPath(const int fd, const uint8_t i2cSlaveAddr, const uint8_t prChn, const int bEnable);
int PR1000_PTZ_InitRXFifo(const int fd, const uint8_t i2cSlaveAddr, const uint8_t prChn);
int PR1000_PTZ_EnableTXPath(const int fd, const uint8_t i2cSlaveAddr, const uint8_t prChn, const int bEnable);
int PR1000_PTZ_InitTXFifo(const int fd, const uint8_t i2cSlaveAddr, const uint8_t prChn);
int PR1000_PTZ_StartRX(const int fd, const _stPortChSel *pstPortChSel, const int bStart);
int PR1000_PTZ_StartTX(const int fd, const _stPortChSel *pstPortChSel, const int bStart);
int PR1000_PTZ_Init(const int fd, const _stPortChSel *pstPortChSel, _drvHost *pHost);
int PR1000_PTZ_SetPattern(const int fd, const _stPortChSel *pstPortChSel, const uint8_t format,  const uint8_t resol, _drvHost *pHost);
int PR1000_PTZ_STDFORMAT_Check(const int fd, const _stPortChSel *pstPortChSel, const enum _pr1000_table_format format, const enum _pr1000_table_inresol camResol, _drvHost *pHost);
int PR1000_PTZ_STDFORMAT_SetPattern(const int fd, const _stPortChSel *pstPortChSel, const uint8_t format,  const uint8_t camResol, _drvHost *pHost);
int PR1000_PTZ_CheckSendTxStatus(const int fd, const _stPortChSel *pstPortChSel);
#ifndef DONT_SUPPORT_PTZ_FUNC
int PR1000_PTZ_ResolveRecvData(const int fd, const uint8_t format, const uint8_t resol, const uint8_t *pData, const uint8_t length);
int PR1000_PTZ_GetRxAttr(const int fd, const _stPortChSel *pstPortChSel, _stPTZRxAttr *pstPTZRxAttr);
int PR1000_PTZ_GetTxAttr(const int fd, const _stPortChSel *pstPortChSel, _stPTZTxAttr *pstPTZTxAttr);
int PR1000_PTZ_GetHVStartAttr(const int fd, const _stPortChSel *pstPortChSel, _stPTZHVStartAttr *pstPTZHVStartAttr);
#endif // DONT_SUPPORT_PTZ_FUNC
int PR1000_PTZ_SetTxParam(const int fd, const _stPortChSel *pstPortChSel, const _stPTZTxParam *pstPTZTxParam);
int PR1000_PTZ_GetTxParam(const int fd, const _stPortChSel *pstPortChSel, _stPTZTxParam *pstPTZTxParam);
int PR1000_PTZ_SetRxParam(const int fd, const _stPortChSel *pstPortChSel, const _stPTZRxParam *pstPTZRxParam);
int PR1000_PTZ_GetRxParam(const int fd, const _stPortChSel *pstPortChSel, _stPTZRxParam *pstPTZRxParam);

//////////////////////////////////// PTZ /////////////////////////////////////////////
#ifndef DONT_SUPPORT_PTZ_FUNC
int PR1000_PTZ_WriteTxPattern(const int fd, const uint8_t i2cSlaveAddr, const uint8_t prChn, const unsigned char *pTxPatFormat, const int sizeTxPatFormat, const unsigned char *pTxPatData, const int sizeTxPatData);
#endif // DONT_SUPPORT_PTZ_FUNC
int PR1000_PTZ_WriteRxPattern(const int fd, const uint8_t i2cSlaveAddr, const uint8_t prChn, const unsigned char *pRxPatFormat, const int sizeRxPatFormat, const unsigned char *pRxPatStartFormat, const int sizeRxPatStartFormat, const unsigned char *pRxPatStartData, const int sizeRxPatStartData);
#ifndef DONT_SUPPORT_PTZ_FUNC
int PR1000_PTZ_SD_SendTxData(const int fd, const _stPortChSel *pstPortChSel, const uint8_t resol, const uint8_t *pPtzCmd, uint16_t ptzCmdLength);
#ifndef DONT_SUPPORT_STD_PVI
int PR1000_PTZ_PVI_SendTxData(const int fd, const _stPortChSel *pstPortChSel, const uint8_t resol, const uint8_t *pPtzCmd, uint16_t ptzCmdLength);
#endif // DONT_SUPPORT_STD_PVI
int PR1000_PTZ_HDA_SendTxData(const int fd, const _stPortChSel *pstPortChSel, const uint8_t resol, const uint8_t *pPtzCmd, uint16_t ptzCmdLength);
int PR1000_PTZ_HDA_STDFORMAT_SendTxData(const int fd, const _stPortChSel *pstPortChSel, const uint8_t resol, const uint8_t *pPtzCmd, uint16_t ptzCmdLength);
int PR1000_PTZ_CVI_SendTxData(const int fd, const _stPortChSel *pstPortChSel, const uint8_t resol, const uint8_t *pPtzCmd, uint16_t ptzCmdLength);
int PR1000_PTZ_HDT_SendTxData(const int fd, const _stPortChSel *pstPortChSel, const uint8_t resol, const uint8_t *pPtzCmd, uint16_t ptzCmdLength);
int PR1000_PTZ_HDT_NEW_SendTxData(const int fd, const _stPortChSel *pstPortChSel, const uint8_t resol, const uint8_t *pPtzCmd, uint16_t ptzCmdLength);
int PR1000_PTZ_HDT_STDFORMAT_SendTxData(const int fd, const _stPortChSel *pstPortChSel, const uint8_t resol, const uint8_t *pPtzCmd, uint16_t ptzCmdLength);
int PR1000_PTZ_HDT_CHGOLD_SendTxCmd(const int fd, const _stPortChSel *pstPortChSel, const uint8_t resol);
#endif // DONT_SUPPORT_PTZ_FUNC


//////////////////////////////////// VEVENT /////////////////////////////////////////////
int PR1000_VEVENT_LoadTable(const int fd, const _stPortChSel *pstPortChSel, const uint8_t format, const uint8_t resol);
#ifndef DONT_SUPPORT_EVENT_FUNC
int PR1000_VEVENT_SetMaskAttr(const int fd, const _stPortChSel *pstPortChSel, const uint8_t resol, const _stMaskCellAttr *pstMaskCellAttr);
int PR1000_VEVENT_GetMaskAttr(const int fd, const _stPortChSel *pstPortChSel, const uint8_t resol, _stMaskCellAttr *pstMaskCellAttr);
int PR1000_VEVENT_SetMaskData(const int fd, const _stPortChSel *pstPortChSel, const enum _pr1000_vevent_mem_type maskType, const uint8_t startLine, const uint8_t lineCnt, const uint32_t *pu32MaskLineData);
int PR1000_VEVENT_GetMaskData(const int fd, const _stPortChSel *pstPortChSel, const enum _pr1000_vevent_mem_type maskType, const uint8_t startLine, const uint8_t lineCnt, uint32_t *pu32RetLineData);
int PR1000_VEVENT_ClearMask(const int fd, const _stPortChSel *pstPortChSel);
int PR1000_VEVENT_GetDetData(const int fd, const _stPortChSel *pstPortChSel, const uint8_t startLine, const uint8_t lineCnt, uint64_t *pu64RetLineData);
int PR1000_VEVENT_SetDisplayCellFormat(const int fd, const _stPortChSel *pstPortChSel, const _stVEVENTDisplayAttr *pstVeventDisplayAttr);

int PR1000_VEVENT_WriteMaskFormat(const int fd, const _stPortChSel *pstPortChSel, const enum _pr1000_vevent_mem_type maskType, const uint32_t *pFormat);
int PR1000_VEVENT_ReadMaskFormat(const int fd, const _stPortChSel *pstPortChSel, const enum _pr1000_vevent_mem_type maskType, uint32_t *pFormat);
int PR1000_VEVENT_PrintDetData(const int fd, const uint8_t resol, const uint64_t *pData, const int lineCnt);
#endif // DONT_SUPPORT_EVENT_FUNC

int PR1000_VEVENT_SetNovidAttr(const int fd, const _stPortChSel *pstPortChSel, const _stNovidAttr *pstNovidAttr);
int PR1000_VEVENT_GetNovidAttr(const int fd, const _stPortChSel *pstPortChSel, _stNovidAttr *pstNovidAttr);
#ifndef DONT_SUPPORT_EVENT_FUNC
int PR1000_VEVENT_SetMdAttr(const int fd, const _stPortChSel *pstPortChSel, const _stMdAttr *pstMdAttr);
int PR1000_VEVENT_GetMdAttr(const int fd, const _stPortChSel *pstPortChSel, _stMdAttr *pstMdAttr);
int PR1000_VEVENT_SetMdLvSens(const int fd, const _stPortChSel *pstPortChSel, const _stMdLvSens *pstMdLvSens);
int PR1000_VEVENT_GetMdLvSens(const int fd, const _stPortChSel *pstPortChSel, _stMdLvSens *pstMdLvSens);
int PR1000_VEVENT_SetMdSpSens(const int fd, const _stPortChSel *pstPortChSel, const _stMdSpSens *pstMdSpSens);
int PR1000_VEVENT_GetMdSpSens(const int fd, const _stPortChSel *pstPortChSel, _stMdSpSens *pstMdSpSens);
int PR1000_VEVENT_SetMdTmpSens(const int fd, const _stPortChSel *pstPortChSel, const _stMdTmpSens *pstMdTmpSens);
int PR1000_VEVENT_GetMdTmpSens(const int fd, const _stPortChSel *pstPortChSel, _stMdTmpSens *pstMdTmpSens);
int PR1000_VEVENT_SetMdVelocity(const int fd, const _stPortChSel *pstPortChSel, const _stMdVelocity *pstMdVelocity);
int PR1000_VEVENT_GetMdVelocity(const int fd, const _stPortChSel *pstPortChSel, _stMdVelocity *pstMdVelocity);
int PR1000_VEVENT_SetBdAttr(const int fd, const _stPortChSel *pstPortChSel, const _stBdAttr *pstBdAttr);
int PR1000_VEVENT_GetBdAttr(const int fd, const _stPortChSel *pstPortChSel, _stBdAttr *pstBdAttr);
int PR1000_VEVENT_SetBdSpSens(const int fd, const _stPortChSel *pstPortChSel, const _stBdSpSens *pstBdSpSens);
int PR1000_VEVENT_GetBdSpSens(const int fd, const _stPortChSel *pstPortChSel, _stBdSpSens *pstBdSpSens);
int PR1000_VEVENT_SetBdLvSens(const int fd, const _stPortChSel *pstPortChSel, const _stBdLvSens *pstBdLvSens);
int PR1000_VEVENT_GetBdLvSens(const int fd, const _stPortChSel *pstPortChSel, _stBdLvSens *pstBdLvSens);
int PR1000_VEVENT_SetBdTmpSens(const int fd, const _stPortChSel *pstPortChSel, const _stBdTmpSens *pstBdTmpSens);
int PR1000_VEVENT_GetBdTmpSens(const int fd, const _stPortChSel *pstPortChSel, _stBdTmpSens *pstBdTmpSens);
int PR1000_VEVENT_SetNdAttr(const int fd, const _stPortChSel *pstPortChSel, const _stNdAttr *pstNdAttr);
int PR1000_VEVENT_GetNdAttr(const int fd, const _stPortChSel *pstPortChSel, _stNdAttr *pstNdAttr);
int PR1000_VEVENT_SetNdLvSens(const int fd, const _stPortChSel *pstPortChSel, const _stNdLvSens *pstNdLvSens);
int PR1000_VEVENT_GetNdLvSens(const int fd, const _stPortChSel *pstPortChSel, _stNdLvSens *pstNdLvSens);
int PR1000_VEVENT_SetNdTmpSens(const int fd, const _stPortChSel *pstPortChSel, const _stNdTmpSens *pstNdTmpSens);
int PR1000_VEVENT_GetNdTmpSens(const int fd, const _stPortChSel *pstPortChSel, _stNdTmpSens *pstNdTmpSens);
int PR1000_VEVENT_SetDdAttr(const int fd, const _stPortChSel *pstPortChSel, const _stDdAttr *pstDdAttr);
int PR1000_VEVENT_GetDdAttr(const int fd, const _stPortChSel *pstPortChSel, _stDdAttr *pstDdAttr);
int PR1000_VEVENT_SetDdLvSens(const int fd, const _stPortChSel *pstPortChSel, const _stDdLvSens *pstDdLvSens);
int PR1000_VEVENT_GetDdLvSens(const int fd, const _stPortChSel *pstPortChSel, _stDdLvSens *pstDdLvSens);
int PR1000_VEVENT_SetDdTmpSens(const int fd, const _stPortChSel *pstPortChSel, const _stDdTmpSens *pstDdTmpSens);
int PR1000_VEVENT_GetDdTmpSens(const int fd, const _stPortChSel *pstPortChSel, _stDdTmpSens *pstDdTmpSens);
int PR1000_VEVENT_SetDfdAttr(const int fd, const _stPortChSel *pstPortChSel, const _stDfdAttr *pstDfdAttr);
int PR1000_VEVENT_GetDfdAttr(const int fd, const _stPortChSel *pstPortChSel, _stDfdAttr *pstDfdAttr);
int PR1000_VEVENT_SetDfdLvSens(const int fd, const _stPortChSel *pstPortChSel, const _stDfdLvSens *pstDfdLvSens);
int PR1000_VEVENT_GetDfdLvSens(const int fd, const _stPortChSel *pstPortChSel, _stDfdLvSens *pstDfdLvSens);
int PR1000_VEVENT_SetDfdSpSens(const int fd, const _stPortChSel *pstPortChSel, const _stDfdSpSens *pstDfdSpSens);
int PR1000_VEVENT_GetDfdSpSens(const int fd, const _stPortChSel *pstPortChSel, _stDfdSpSens *pstDfdSpSens);
int PR1000_VEVENT_SetDfdTmpSens(const int fd, const _stPortChSel *pstPortChSel, const _stDfdTmpSens *pstDfdTmpSens);
int PR1000_VEVENT_GetDfdTmpSens(const int fd, const _stPortChSel *pstPortChSel, _stDfdTmpSens *pstDfdTmpSens);
#endif // DONT_SUPPORT_EVENT_FUNC

int PR1000_VEVENT_GetNovidStatus(const int fd, const _stPortChSel *pstPortChSel, uint8_t *pStatus);
int PR1000_VEVENT_GetVfdStatus(const int fd, const _stPortChSel *pstPortChSel, uint8_t *pStatus);
#ifndef DONT_SUPPORT_EVENT_FUNC
int PR1000_VEVENT_GetMdStatus(const int fd, const _stPortChSel *pstPortChSel, uint8_t *pStatus);
int PR1000_VEVENT_GetBdStatus(const int fd, const _stPortChSel *pstPortChSel, uint8_t *pStatus);
int PR1000_VEVENT_GetNdStatus(const int fd, const _stPortChSel *pstPortChSel, uint8_t *pStatus);
int PR1000_VEVENT_GetDdStatus(const int fd, const _stPortChSel *pstPortChSel, uint8_t *pStatus);
int PR1000_VEVENT_GetDfdStatus(const int fd, const _stPortChSel *pstPortChSel, uint8_t *pStatus);
int PR1000_VEVENT_GetAdMuteStatus(const int fd, const uint8_t chip, const uint8_t prChn, uint8_t *pStatus);
int PR1000_VEVENT_GetAdAbsStatus(const int fd, const uint8_t chip, const uint8_t prChn, uint8_t *pStatus);
int PR1000_VEVENT_GetAdDiffStatus(const int fd, const uint8_t chip, const uint8_t prChn, uint8_t *pStatus);
#endif // DONT_SUPPORT_EVENT_FUNC


#endif /* __PR1000_FUNC_H__ */

