#ifndef __PR1000_PTZ_TABLE_SD_H__
#define __PR1000_PTZ_TABLE_SD_H__

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
/*** SD720 COMMAND ***/
#define PR1000_PTZ_SD_TXCMD_BASE_BYTE_CNT		(4)
extern const uint8_t pr1000_ptz_table_sd_CMD_VALID[(4)];
extern const uint8_t pr1000_ptz_table_sd_OSD_TOP[(4)];
extern const uint8_t pr1000_ptz_table_sd_OSD_RIGHT[(4)];
extern const uint8_t pr1000_ptz_table_sd_OSD_BOTTOM[(4)];
extern const uint8_t pr1000_ptz_table_sd_OSD_LEFT[(4)];
extern const uint8_t pr1000_ptz_table_sd_OSD_CENTER[(4)];
extern const uint8_t pr1000_ptz_table_sd_FOCUS_PLUS[(4)];
extern const uint8_t pr1000_ptz_table_sd_RESERVED0[(4)];

/* SD720/SD960 */
extern const _pr1000PTZCmd pr1000_ptz_table_sdcmd[max_pr1000_ptz_table_command];
#endif // DONT_SUPPORT_PTZ_FUNC

#ifndef DONT_SUPPORT_PTZ_FUNC
extern const _stPTZTxParam pr1000_ptz_txparam_sd_def[2];
#endif // DONT_SUPPORT_PTZ_FUNC
extern const _stPTZRxParam pr1000_ptz_rxparam_sd_def[2];

/////////////////////////////////////////////////////////////////////////////////////////

/*** SD720/SD960 PATTERN ***/
#ifndef DONT_SUPPORT_PTZ_FUNC
extern const unsigned char pr1000_ptz_table_sd_tx_pat_format[((6)*2)];
extern const unsigned char pr1000_ptz_table_sd_tx_pat_data[((6)*2)];
#endif // DONT_SUPPORT_PTZ_FUNC
extern const unsigned char pr1000_ptz_table_sd_rx_pat_format[((6)*2)];
extern const unsigned char pr1000_ptz_table_sd_rx_pat_start_format[(6)];
extern const unsigned char pr1000_ptz_table_sd_rx_pat_start_data[(6)];

//////////////////////////////////////////////////////////////////////////////////////////

#endif /* __PR1000_PTZ_TABLE_SD_H__ */
