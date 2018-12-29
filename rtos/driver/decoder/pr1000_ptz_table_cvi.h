#ifndef __PR1000_PTZ_TABLE_CVI_H__
#define __PR1000_PTZ_TABLE_CVI_H__

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

/************************************************************/
///////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef DONT_SUPPORT_PTZ_FUNC
/*** CVI COMMAND ***/
#define PR1000_PTZ_CVI_TXCMD_BASE_BYTE_CNT		(7)
enum _pr1000_ptz_cvi_table_command_ptz {
	pr1000_ptz_cvi_table_command_PTZ_RIGHT = 0,
	pr1000_ptz_cvi_table_command_PTZ_LEFT,
	pr1000_ptz_cvi_table_command_PTZ_DOWN,
	pr1000_ptz_cvi_table_command_PTZ_UP,
	pr1000_ptz_cvi_table_command_PTZ_DOWN_RIGHT,
	pr1000_ptz_cvi_table_command_PTZ_DOWN_LEFT,
	pr1000_ptz_cvi_table_command_PTZ_UP_RIGHT,
	pr1000_ptz_cvi_table_command_PTZ_UP_LEFT,
	pr1000_ptz_cvi_table_command_PTZ_RESERVED0,
	max_pr1000_ptz_cvi_table_command_ptz
};
extern const char _STR_PR1000_CVI_PTZ_COMMAND_PTZ[max_pr1000_ptz_cvi_table_command_ptz][20];
extern const uint8_t pr1000_ptz_table_cvi_PTZ_CMD[9];
enum _pr1000_ptz_cvi_table_command_lens {
	pr1000_ptz_cvi_table_command_LENS_TELE = 0,
	pr1000_ptz_cvi_table_command_LENS_WIDE,
	pr1000_ptz_cvi_table_command_LENS_FAR,
	pr1000_ptz_cvi_table_command_LENS_NEAR,
	pr1000_ptz_cvi_table_command_LENS_OPEN,
	pr1000_ptz_cvi_table_command_LENS_CLOSE,
	pr1000_ptz_cvi_table_command_LENS_RESERVED0,
	max_pr1000_ptz_cvi_table_command_lens
};
extern const char _STR_PR1000_CVI_PTZ_COMMAND_LENS[max_pr1000_ptz_cvi_table_command_lens][12];
extern const uint8_t pr1000_ptz_table_cvi_LENS_CMD[7];
enum _pr1000_ptz_cvi_table_command_other {
	pr1000_ptz_cvi_table_command_OTHER_SET_PRESET = 0,
	pr1000_ptz_cvi_table_command_OTHER_CALL_PRESET,
	pr1000_ptz_cvi_table_command_OTHER_DELETE_PRESET,
	pr1000_ptz_cvi_table_command_OTHER_DELETE_ALL_PRESET,
	pr1000_ptz_cvi_table_command_OTHER_ADD_TOUR,
	pr1000_ptz_cvi_table_command_OTHER_SET_SCAN_LR_LIMIT,
	pr1000_ptz_cvi_table_command_OTHER_SET_SCAN_RATE,
	pr1000_ptz_cvi_table_command_OTHER_SET_TOUR_STAY,
	pr1000_ptz_cvi_table_command_OTHER_H360_CONT_RSPEED,
	pr1000_ptz_cvi_table_command_OTHER_SET_TOUR_SPEED,
	pr1000_ptz_cvi_table_command_OTHER_SCAN_TOUR_PAT_STOP,
	pr1000_ptz_cvi_table_command_OTHER_BEGIN_TO_SCAN,
	pr1000_ptz_cvi_table_command_OTHER_H360_CONT_ROT,
	pr1000_ptz_cvi_table_command_OTHER_BEGIN_TO_TOUR,
	pr1000_ptz_cvi_table_command_OTHER_SET_PAT_TO_BEGIN,
	pr1000_ptz_cvi_table_command_OTHER_SET_PAT_TO_STOP,
	pr1000_ptz_cvi_table_command_OTHER_BEGIN_TO_PAT,
	pr1000_ptz_cvi_table_command_OTHER_CAM_MENU_SETUP,
	pr1000_ptz_cvi_table_command_OTHER_CAM_MENU_SETUP_CLOSE,
	pr1000_ptz_cvi_table_command_OTHER_CAM_MENU_SETUP_OPEN,
	pr1000_ptz_cvi_table_command_OTHER_CAM_MENU_SETUP_MVBK,
	pr1000_ptz_cvi_table_command_OTHER_CAM_MENU_SETUP_ENTERNEXT,
	pr1000_ptz_cvi_table_command_OTHER_CAM_MENU_SETUP_MVUP,
	pr1000_ptz_cvi_table_command_OTHER_CAM_MENU_SETUP_MVDW,
	pr1000_ptz_cvi_table_command_OTHER_CAM_MENU_SETUP_MVLF,
	pr1000_ptz_cvi_table_command_OTHER_CAM_MENU_SETUP_MVRG,
	pr1000_ptz_cvi_table_command_OTHER_CAM_MENU_SETUP_CONFIRM,
	pr1000_ptz_cvi_table_command_OTHER_RESET_SELF_CHK,
	pr1000_ptz_cvi_table_command_OTHER_FACTORY_DEFSET,
	max_pr1000_ptz_cvi_table_command_other
};

extern const char _STR_PR1000_CVI_PTZ_COMMAND_OTHER[max_pr1000_ptz_cvi_table_command_other][40];
extern const uint8_t pr1000_ptz_table_cvi_OTHER_CMD[29];
extern const uint8_t pr1000_ptz_table_cvi_OSD_TOP[7];
extern const uint8_t pr1000_ptz_table_cvi_OSD_RIGHT[7];
extern const uint8_t pr1000_ptz_table_cvi_OSD_BOTTOM[7];
extern const uint8_t pr1000_ptz_table_cvi_OSD_LEFT[7];
extern const uint8_t pr1000_ptz_table_cvi_IRIS_PLUS[21];
extern const uint8_t pr1000_ptz_table_cvi_IRIS_MINUS[21];
extern const uint8_t pr1000_ptz_table_cvi_FOCUS_PLUS[21];
extern const uint8_t pr1000_ptz_table_cvi_FOCUS_MINUS[21];
extern const uint8_t pr1000_ptz_table_cvi_ZOOM_PLUS[21];
extern const uint8_t pr1000_ptz_table_cvi_ZOOM_MINUS[21];
extern const uint8_t pr1000_ptz_table_cvi_DIR_TOP[21];
extern const uint8_t pr1000_ptz_table_cvi_DIR_RIGHT[21];
extern const uint8_t pr1000_ptz_table_cvi_DIR_BOTTOM[21];
extern const uint8_t pr1000_ptz_table_cvi_DIR_LEFT[21];
extern const uint8_t pr1000_ptz_table_cvi_LENS_CLOSE[7];
extern const uint8_t pr1000_ptz_table_cvi_DIR_CLOSE[7];

/* CVI */
extern const _pr1000PTZCmd pr1000_ptz_table_cvicmd[max_pr1000_ptz_table_command];
#endif // DONT_SUPPORT_PTZ_FUNC

#ifndef DONT_SUPPORT_PTZ_FUNC
extern const _stPTZTxParam pr1000_ptz_txparam_cvi_def[6];
#endif // DONT_SUPPORT_PTZ_FUNC
extern const _stPTZRxParam pr1000_ptz_rxparam_cvi_def[6];
/////////////////////////////////////////////////////////////////////////////////////////
/*** CVI PATTERN ***/
#ifndef DONT_SUPPORT_PTZ_FUNC
extern const unsigned char pr1000_ptz_table_cvi_tx_pat_format[((4*6)*3)];
extern const unsigned char pr1000_ptz_table_cvi_tx_pat_data[((4*6)*3)];
#endif // DONT_SUPPORT_PTZ_FUNC
extern const unsigned char pr1000_ptz_table_cvi_rx_pat_format[((4*6)*3)];
extern const unsigned char pr1000_ptz_table_cvi_rx_pat_start_format[(4*1)];
extern const unsigned char pr1000_ptz_table_cvi_rx_pat_start_data[(4*1)];

//////////////////////////////////////////////////////////////////////////////////////////

#endif /* __PR1000_PTZ_TABLE_CVI_H__ */
