#ifndef __PR1000_PTZ_TABLE_HDT_H__
#define __PR1000_PTZ_TABLE_HDT_H__

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
/*** HDT COMMAND ***/
/* Isn't HDT osd command */
#define PR1000_PTZ_HDT_TXCMD_BASE_BYTE_CNT		(8)
extern const uint8_t pr1000_ptz_table_hdt_CMD_VALID[(4*2)];
extern const uint8_t pr1000_ptz_table_hdt_OSD_TOP[(4*2)];
extern const uint8_t pr1000_ptz_table_hdt_OSD_RIGHT[(4*2)];
extern const uint8_t pr1000_ptz_table_hdt_OSD_BOTTOM[(4*2)];
extern const uint8_t pr1000_ptz_table_hdt_OSD_LEFT[(4*2)];
extern const uint8_t pr1000_ptz_table_hdt_IRIS_PLUS[(4*2)];
extern const uint8_t pr1000_ptz_table_hdt_IRIS_MINUS[(4*2)];
extern const uint8_t pr1000_ptz_table_hdt_FOCUS_PLUS[(4*2)];
extern const uint8_t pr1000_ptz_table_hdt_FOCUS_MINUS[(4*2)];
extern const uint8_t pr1000_ptz_table_hdt_ZOOM_PLUS[(4*2)];
extern const uint8_t pr1000_ptz_table_hdt_ZOOM_MINUS[(4*2)];
extern const uint8_t pr1000_ptz_table_hdt_CENTER[(4*2)];
extern const uint8_t pr1000_ptz_table_hdt_DIR_TOP[(4*2)];
extern const uint8_t pr1000_ptz_table_hdt_DIR_RIGHT[(4*2)];
extern const uint8_t pr1000_ptz_table_hdt_DIR_BOTTOM[(4*2)];
extern const uint8_t pr1000_ptz_table_hdt_DIR_LEFT[(4*2)];
extern const uint8_t pr1000_ptz_table_hdt_DIR_RIGHT_TOP[(4*2)];
extern const uint8_t pr1000_ptz_table_hdt_DIR_RIGHT_BOTTOM[(4*2)];
extern const uint8_t pr1000_ptz_table_hdt_DIR_LEFT_TOP[(4*2)];
extern const uint8_t pr1000_ptz_table_hdt_DIR_LEFT_BOTTOM[(4*2)];
extern const uint8_t pr1000_ptz_table_hdt_RESERVED0[(4*2)];

/*** HDTNEW COMMAND ***/
/* Isn't HDTNEW osd command */
#define PR1000_PTZ_HDT_NEW_TXCMD_BASE_BYTE_CNT		(8)
extern const uint8_t pr1000_ptz_table_hdt_new_CMD_VALID[(4*2)];
extern const uint8_t pr1000_ptz_table_hdt_new_OSD_TOP[(4*2)];
extern const uint8_t pr1000_ptz_table_hdt_new_OSD_RIGHT[(4*2)];
extern const uint8_t pr1000_ptz_table_hdt_new_OSD_BOTTOM[(4*2)];
extern const uint8_t pr1000_ptz_table_hdt_new_OSD_LEFT[(4*2)];
extern const uint8_t pr1000_ptz_table_hdt_new_IRIS_PLUS[(4*2)];
extern const uint8_t pr1000_ptz_table_hdt_new_IRIS_MINUS[(4*2)];
extern const uint8_t pr1000_ptz_table_hdt_new_FOCUS_PLUS[(4*2)];
extern const uint8_t pr1000_ptz_table_hdt_new_FOCUS_MINUS[(4*2)];
extern const uint8_t pr1000_ptz_table_hdt_new_ZOOM_PLUS[(4*2)];
extern const uint8_t pr1000_ptz_table_hdt_new_ZOOM_MINUS[(4*2)];
extern const uint8_t pr1000_ptz_table_hdt_new_CENTER[(4*2)];
extern const uint8_t pr1000_ptz_table_hdt_new_DIR_TOP[(4*2)];
extern const uint8_t pr1000_ptz_table_hdt_new_DIR_RIGHT[(4*2)];
extern const uint8_t pr1000_ptz_table_hdt_new_DIR_BOTTOM[(4*2)];
extern const uint8_t pr1000_ptz_table_hdt_new_DIR_LEFT[(4*2)];
extern const uint8_t pr1000_ptz_table_hdt_new_DIR_RIGHT_TOP[(4*2)];
extern const uint8_t pr1000_ptz_table_hdt_new_DIR_RIGHT_BOTTOM[(4*2)];
extern const uint8_t pr1000_ptz_table_hdt_new_DIR_LEFT_TOP[(4*2)];
extern const uint8_t pr1000_ptz_table_hdt_new_DIR_LEFT_BOTTOM[(4*2)];
extern const uint8_t pr1000_ptz_table_hdt_new_RESERVED0[(4*2)];

/* HDT */
extern const _pr1000PTZCmd pr1000_ptz_table_hdtcmd[max_pr1000_ptz_table_command];
/* HDT_NEW */
extern const _pr1000PTZCmd pr1000_ptz_table_hdt_newcmd[max_pr1000_ptz_table_command];
#endif // DONT_SUPPORT_PTZ_FUNC

#ifndef DONT_SUPPORT_PTZ_FUNC
extern const _stPTZTxParam pr1000_ptz_txparam_hdt_def[6];
extern const _stPTZTxParam pr1000_ptz_txparam_hdt_new_def[6];
#endif // DONT_SUPPORT_PTZ_FUNC
extern const _stPTZRxParam pr1000_ptz_rxparam_hdt_def[6];
extern const _stPTZRxParam pr1000_ptz_rxparam_hdt_new_def[6];

/////////////////////////////////////////////////////////////////////////////////////////

/*** HDT PATTERN ***/
#ifndef DONT_SUPPORT_PTZ_FUNC
extern const unsigned char pr1000_ptz_table_hdt_tx_pat_format[((5*1)*2)];
extern const unsigned char pr1000_ptz_table_hdt_tx_pat_data[((5*1)*2)];
#endif // DONT_SUPPORT_PTZ_FUNC
extern const unsigned char pr1000_ptz_table_hdt_rx_pat_format[((5*1)*2)];
extern const unsigned char pr1000_ptz_table_hdt_rx_pat_start_format[(5*1)];
extern const unsigned char pr1000_ptz_table_hdt_rx_pat_start_data[(5*1)];

/*** HDT_NEW PATTERN ***/
#ifndef DONT_SUPPORT_PTZ_FUNC
extern const unsigned char pr1000_ptz_table_hdt_new_tx_pat_format[((5*1)*2)];
extern const unsigned char pr1000_ptz_table_hdt_new_tx_pat_data[((5*1)*2)];
#endif // DONT_SUPPORT_PTZ_FUNC
extern const unsigned char pr1000_ptz_table_hdt_new_rx_pat_format[((5*1)*2)];
extern const unsigned char pr1000_ptz_table_hdt_new_rx_pat_start_format[(5*1)];
extern const unsigned char pr1000_ptz_table_hdt_new_rx_pat_start_data[(5*1)];


//////////////////////////////////////////////////////////////////////////////////////////
#endif /* __PR1000_PTZ_TABLE_HDT_H__ */
