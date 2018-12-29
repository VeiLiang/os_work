#ifndef __PR1000_PTZ_TABLE_HDA_H__
#define __PR1000_PTZ_TABLE_HDA_H__

#ifdef __LINUX_SYSTEM__
/* __KERNEL__ */
#ifdef __KERNEL__
#include "pr1000_user_config.h"
#else
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#endif /* __KERNEL__ */
#else //#ifdef __LINUX_SYSTEM__
#include "pr1000_user_config.h"
#endif // __LINUX_SYSTEM__

///////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef DONT_SUPPORT_PTZ_FUNC
/*** HDA COMMAND ***/
#define PR1000_PTZ_HDA_TXCMD_BASE_BYTE_CNT_1080p		(24)
extern const uint8_t pr1000_ptz_table_hda_ESCAPE_CODE0_2Byte_1080p[2];
extern const uint8_t pr1000_ptz_table_hda_ESCAPE_CODE1_1Byte_1080p[1];
extern const uint8_t pr1000_ptz_table_hda_CMD_VALID_1080p[(4*6)];
extern const uint8_t pr1000_ptz_table_hda_RESET_1080p[(4*6)];
extern const uint8_t pr1000_ptz_table_hda_SET_1080p[(4*6)];
extern const uint8_t pr1000_ptz_table_hda_UP_1080p[(4*6)];
extern const uint8_t pr1000_ptz_table_hda_RIGHT_1080p[(4*6)];
extern const uint8_t pr1000_ptz_table_hda_DOWN_1080p[(4*6)];
extern const uint8_t pr1000_ptz_table_hda_LEFT_1080p[(4*6)];
extern const uint8_t pr1000_ptz_table_hda_LEFT_UP_1080p[(4*6)];
extern const uint8_t pr1000_ptz_table_hda_LEFT_DOWN_1080p[(4*6)];
extern const uint8_t pr1000_ptz_table_hda_RIGHT_UP_1080p[(4*6)];
extern const uint8_t pr1000_ptz_table_hda_RIGHT_DOWN_1080p[(4*6)];
extern const uint8_t pr1000_ptz_table_hda_IRIS_OPEN_1080p[(4*6)];
extern const uint8_t pr1000_ptz_table_hda_IRIS_CLOSE_1080p[(4*6)];
extern const uint8_t pr1000_ptz_table_hda_FOCUS_FAR_1080p[(4*6)];
extern const uint8_t pr1000_ptz_table_hda_FOCUS_NEAR_1080p[(4*6)];
extern const uint8_t pr1000_ptz_table_hda_ZOOM_TELE_1080p[(4*6)];
extern const uint8_t pr1000_ptz_table_hda_ZOOM_WIDE_1080p[(4*6)];
extern const uint8_t pr1000_ptz_table_hda_RESERVED0_1080p[(4*6)];
#define PR1000_PTZ_HDA_TXCMD_BASE_BYTE_CNT_720p		(4)
extern const uint8_t pr1000_ptz_table_hda_ESCAPE_CODE0_4Byte_720p[(4*1)];
extern const uint8_t pr1000_ptz_table_hda_CMD_VALID_720p[(4*1)];
extern const uint8_t pr1000_ptz_table_hda_RESET_720p[(4*1)];
extern const uint8_t pr1000_ptz_table_hda_SET_720p[(4*1)];
extern const uint8_t pr1000_ptz_table_hda_OSD_720p[(4*1)];
extern const uint8_t pr1000_ptz_table_hda_UP_720p[(4*1)];
extern const uint8_t pr1000_ptz_table_hda_RIGHT_720p[(4*1)];
extern const uint8_t pr1000_ptz_table_hda_DOWN_720p[(4*1)];
extern const uint8_t pr1000_ptz_table_hda_LEFT_720p[(4*1)];
extern const uint8_t pr1000_ptz_table_hda_IRIS_OPEN_720p[(4*1)];
extern const uint8_t pr1000_ptz_table_hda_IRIS_CLOSE_720p[(4*1)];
extern const uint8_t pr1000_ptz_table_hda_FOCUS_FAR_720p[(4*1)];
extern const uint8_t pr1000_ptz_table_hda_FOCUS_NEAR_720p[(4*1)];
extern const uint8_t pr1000_ptz_table_hda_ZOOM_TELE_720p[(4*1)];
extern const uint8_t pr1000_ptz_table_hda_ZOOM_WIDE_720p[(4*1)];

extern const uint8_t pr1000_ptz_table_hda_SCAN_SR_720p[(4*1)];
extern const uint8_t pr1000_ptz_table_hda_SCAN_ST_720p[(4*1)];
extern const uint8_t pr1000_ptz_table_hda_PRESET1_720p[(4*1)];
extern const uint8_t pr1000_ptz_table_hda_PRESET2_720p[(4*1)];
extern const uint8_t pr1000_ptz_table_hda_PRESET3_720p[(4*1)];
extern const uint8_t pr1000_ptz_table_hda_PTN1_SR_720p[(4*1)];
extern const uint8_t pr1000_ptz_table_hda_PTN1_ST_720p[(4*1)];
extern const uint8_t pr1000_ptz_table_hda_PTN2_SR_720p[(4*1)];
extern const uint8_t pr1000_ptz_table_hda_PTN2_ST_720p[(4*1)];
extern const uint8_t pr1000_ptz_table_hda_PTN3_SR_720p[(4*1)];
extern const uint8_t pr1000_ptz_table_hda_PTN3_ST_720p[(4*1)];
extern const uint8_t pr1000_ptz_table_hda_RUN_720p[(4*1)];

extern const uint8_t pr1000_ptz_table_hda_RESERVED0_720p[(4*1)];

/* HDA */
extern const _pr1000PTZCmd pr1000_ptz_table_hdacmd_1080p[max_pr1000_ptz_table_command];
extern const _pr1000PTZCmd pr1000_ptz_table_hdacmd_720p[max_pr1000_ptz_table_command];
#endif // DONT_SUPPORT_PTZ_FUNC

#ifndef DONT_SUPPORT_PTZ_FUNC
extern const _stPTZTxParam pr1000_ptz_txparam_hda_def[6];
#endif // DONT_SUPPORT_PTZ_FUNC
extern const _stPTZRxParam pr1000_ptz_rxparam_hda_def[6];

/////////////////////////////////////////////////////////////////////////////////////////

/*** HDA PATTERN ***/
#ifndef DONT_SUPPORT_PTZ_FUNC
extern const unsigned char pr1000_ptz_table_hda_tx_pat_format_1080p[((3*4)*6)];
extern const unsigned char pr1000_ptz_table_hda_tx_pat_format_720p[((3*2)*2)];
extern const unsigned char pr1000_ptz_table_hda_tx_pat_data_1080p[((3*4)*6)];
extern const unsigned char pr1000_ptz_table_hda_tx_pat_data_720p[((3*2)*2)];
#endif // DONT_SUPPORT_PTZ_FUNC
extern const unsigned char pr1000_ptz_table_hda_rx_pat_format_1080p[((3*4)*6)];
extern const unsigned char pr1000_ptz_table_hda_rx_pat_format_720p[((3*2)*2)];
extern const unsigned char pr1000_ptz_table_hda_rx_pat_start_format_1080p[(3*1)];
extern const unsigned char pr1000_ptz_table_hda_rx_pat_start_format_720p[(3*2)];
extern const unsigned char pr1000_ptz_table_hda_rx_pat_start_data_1080p[(3*1)];
extern const unsigned char pr1000_ptz_table_hda_rx_pat_start_data_720p[(3*2)];

//////////////////////////////////////////////////////////////////////////////////////////

#endif /* __PR1000_PTZ_TABLE_HDA_H__ */
