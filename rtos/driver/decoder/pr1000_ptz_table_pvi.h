#ifndef __PR1000_PTZ_TABLE_PVI_H__
#define __PR1000_PTZ_TABLE_PVI_H__

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
/*** PVI COMMAND ***/
#define PR1000_PTZ_PVI_TXCMD_BASE_BYTE_CNT		(7)
extern const uint8_t pr1000_ptz_table_pvi_CMD_VALID[(7)];
extern const uint8_t pr1000_ptz_table_pvi_OSD_TOP[(7)];
extern const uint8_t pr1000_ptz_table_pvi_OSD_RIGHT[(7)];
extern const uint8_t pr1000_ptz_table_pvi_OSD_BOTTOM[(7)];
extern const uint8_t pr1000_ptz_table_pvi_OSD_LEFT[(7)];
extern const uint8_t pr1000_ptz_table_pvi_OSD_CENTER[(7)];
extern const uint8_t pr1000_ptz_table_pvi_IRIS_PLUS[(7)];
extern const uint8_t pr1000_ptz_table_pvi_IRIS_MINUS[(7)];
extern const uint8_t pr1000_ptz_table_pvi_FOCUS_PLUS[(7)];
extern const uint8_t pr1000_ptz_table_pvi_FOCUS_MINUS[(7)];	
extern const uint8_t pr1000_ptz_table_pvi_ZOOM_PLUS[(7)];
extern const uint8_t pr1000_ptz_table_pvi_ZOOM_MINUS[(7)];

/* PVI */
extern const _pr1000PTZCmd pr1000_ptz_table_pvicmd[max_pr1000_ptz_table_command];
#endif // DONT_SUPPORT_PTZ_FUNC

#ifndef DONT_SUPPORT_PTZ_FUNC
extern const _stPTZTxParam pr1000_ptz_txparam_pvi_def[6];
#endif // DONT_SUPPORT_PTZ_FUNC
extern const _stPTZRxParam pr1000_ptz_rxparam_pvi_def[6];

/////////////////////////////////////////////////////////////////////////////////////////
/*** PVI PATTERN ***/
#ifndef DONT_SUPPORT_PTZ_FUNC
extern const unsigned char pr1000_ptz_table_pvi_tx_pat_format[((3*4)*2)];
extern const unsigned char pr1000_ptz_table_pvi_tx_pat_data[((3*4)*2)];
#endif // DONT_SUPPORT_PTZ_FUNC
extern const unsigned char pr1000_ptz_table_pvi_rx_pat_format[((3*4)*2)];
extern const unsigned char pr1000_ptz_table_pvi_rx_pat_start_format[(3*1)];
extern const unsigned char pr1000_ptz_table_pvi_rx_pat_start_data[(3*1)];

//////////////////////////////////////////////////////////////////////////////////////////

#endif /* __PR1000_PTZ_TABLE_PVI_H__ */
