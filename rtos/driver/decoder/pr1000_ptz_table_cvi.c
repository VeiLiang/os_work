#ifdef __LINUX_SYSTEM__
#ifdef __KERNEL__
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/fcntl.h>
#include <linux/mm.h>
#include <linux/miscdevice.h>
#include <linux/proc_fs.h>
#include "pr1000_user_config.h"
#else
#include <stdint.h>
#endif /* __KERNEL__ */
#include "pr1000_drvcommon.h"
#include "pr1000_ptz_table.h"
#include "pr1000_ptz_table_cvi.h"

#else //#ifdef __LINUX_SYSTEM__

#include <stdio.h>
#include "pr1000_drvcommon.h"
#include "pr1000_ptz_table.h"
#include "pr1000_ptz_table_cvi.h"
#include "pr1000_user_config.h"

#endif // __LINUX_SYSTEM__

/////////////////////////////////////////////////////////////////////////////////////////////
#ifndef DONT_SUPPORT_PTZ_FUNC
/*** CVI COMMAND ***/
#ifndef DONT_SUPPORT_HELP_STRING
const char _STR_PR1000_CVI_PTZ_COMMAND_PTZ[max_pr1000_ptz_cvi_table_command_ptz][20] = {
	"PTZ_RIGHT",
	"PTZ_LEFT",
	"PTZ_DOWN",
	"PTZ_UP",
	"PTZ_DOWN_RIGHT",
	"PTZ_DOWN_LEFT",
	"PTZ_UP_RIGHT",
	"PTZ_UP_LEFT",
};
#endif // DONT_SUPPORT_HELP_STRING
#ifndef DONT_SUPPORT_PTZ_ETC_CMD
const uint8_t pr1000_ptz_table_cvi_PTZ_CMD[9] = {
	0x01, //"PTZ_RIGHT",
	0x02, //"PTZ_LEFT",
	0x04, //"PTZ_DOWN",
	0x08, //"PTZ_UP",
	0x05, //"PTZ_DOWN_RIGHT",
	0x06, //"PTZ_DOWN_LEFT",
	0x09, //"PTZ_UP_RIGHT",
	0x0A, //"PTZ_UP_LEFT",
	0x00, //"PTZ_RESERVED0",
};
#endif // DONT_SUPPORT_PTZ_ETC_CMD

#ifndef DONT_SUPPORT_HELP_STRING
const char _STR_PR1000_CVI_PTZ_COMMAND_LENS[max_pr1000_ptz_cvi_table_command_lens][12] = {
	"LENS_TELE",
	"LENS_WIDE",
	"LENS_FAR",
	"LENS_NEAR",
	"LENS_OPEN",
	"LENS_CLOSE",
	"LENS_RESV",
};
#endif // DONT_SUPPORT_HELP_STRING
#ifndef DONT_SUPPORT_PTZ_ETC_CMD
const uint8_t pr1000_ptz_table_cvi_LENS_CMD[7] = {
	0x41, //"LENS_TELE",
	0x42, //"LENS_WIDE",
	0x44, //"LENS_FAR",
	0x48, //"LENS_NEAR",
	0x50, //"LENS_OPEN",
	0x60, //"LENS_CLOSE",
	0x40, //"LENS_RESERVED0",
};
#endif // DONT_SUPPORT_PTZ_ETC_CMD

#ifndef DONT_SUPPORT_HELP_STRING
const char _STR_PR1000_CVI_PTZ_COMMAND_OTHER[max_pr1000_ptz_cvi_table_command_other][40] = {
	"OTHER_SET_PRESET",
	"OTHER_CALL_PRESET",
	"OTHER_DELETE_PRESET",
	"OTHER_DELETE_ALL_PRESET",
	"OTHER_ADD_TOUR",
	"OTHER_SET_SCAN_LR_LIMIT",
	"OTHER_SET_SCAN_RATE",
	"OTHER_SET_TOUR_STAY",
	"OTHER_H360_CONT_RSPEED",
	"OTHER_SET_TOUR_SPEED",
	"OTHER_SCAN_TOUR_PAT_STOP",
	"OTHER_BEGIN_TO_SCAN",
	"OTHER_H360_CONT_ROT",
	"OTHER_BEGIN_TO_TOUR",
	"OTHER_SET_PAT_TO_BEGIN",
	"OTHER_SET_PAT_TO_STOP",
	"OTHER_BEGIN_TO_PAT",
	"OTHER_CAM_MENU_SETUP",
	"OTHER_CAM_MENU_SETUP_CLOSE",
	"OTHER_CAM_MENU_SETUP_OPEN",
	"OTHER_CAM_MENU_SETUP_MVBK",
	"OTHER_CAM_MENU_SETUP_ENTERNEXT",
	"OTHER_CAM_MENU_SETUP_MVUP",
	"OTHER_CAM_MENU_SETUP_MVDW",
	"OTHER_CAM_MENU_SETUP_MVLF",
	"OTHER_CAM_MENU_SETUP_MVRG",
	"OTHER_CAM_MENU_SETUP_CONFIRM",
	"OTHER_RESET_SELF_CHK",
	"OTHER_FACTORY_DEFSET",
};
#endif // DONT_SUPPORT_HELP_STRING
#ifndef DONT_SUPPORT_PTZ_ETC_CMD
const uint8_t pr1000_ptz_table_cvi_OTHER_CMD[29] = {
	0x81, //"OTHER_SET_PRESET",
	0x83, //"OTHER_CALL_PRESET",
	0x82, //"OTHER_DELETE_PRESET",
	0x82, //"OTHER_DELETE_ALL_PRESET",
	0x84, //"OTHER_ADD_TOUR",
	0x81, //"OTHER_SET_SCAN_LR_LIMIT",
	0x81, //"OTHER_SET_SCAN_RATE",
	0x81, //"OTHER_SET_TOUR_STAY",
	0x81, //"OTHER_H360_CONT_RSPEED",
	0x81, //"OTHER_SET_TOUR_SPEED",
	0x83, //"OTHER_SCAN_TOUR_PAT_STOP",
	0x83, //"OTHER_BEGIN_TO_SCAN",
	0x83, //"OTHER_H360_CONT_ROT",
	0x83, //"OTHER_BEGIN_TO_TOUR",
	0x85, //"OTHER_SET_PAT_TO_BEGIN",
	0x85, //"OTHER_SET_PAT_TO_STOP",
	0x85, //"OTHER_BEGIN_TO_PAT",
	0x89, //"OTHER_CAM_MENU_SETUP",
	0x89, //"OTHER_CAM_MENU_SETUP_CLOSE",
	0x89, //"OTHER_CAM_MENU_SETUP_OPEN",
	0x89, //"OTHER_CAM_MENU_SETUP_MVBK",
	0x89, //"OTHER_CAM_MENU_SETUP_ENTERNEXT",
	0x89, //"OTHER_CAM_MENU_SETUP_MVUP",
	0x89, //"OTHER_CAM_MENU_SETUP_MVDW",
	0x89, //"OTHER_CAM_MENU_SETUP_MVLF",
	0x89, //"OTHER_CAM_MENU_SETUP_MVRG",
	0x89, //"OTHER_CAM_MENU_SETUP_CONFIRM",
	0x8D, //"OTHER_RESET_SELF_CHK",
	0x8D, //"OTHER_FACTORY_DEFSET",
};
#endif // DONT_SUPPORT_PTZ_ETC_CMD

const uint8_t pr1000_ptz_table_cvi_OSD_TOP[7] = 	{0xA5,0x01,0x89,0x04,0x00,0x00,0x33};
const uint8_t pr1000_ptz_table_cvi_OSD_RIGHT[7] =	{0xA5,0x01,0x89,0x07,0x00,0x00,0x36};
const uint8_t pr1000_ptz_table_cvi_OSD_BOTTOM[7]=	{0xA5,0x01,0x89,0x05,0x00,0x00,0x34};
const uint8_t pr1000_ptz_table_cvi_OSD_LEFT[7] = 	{0xA5,0x01,0x89,0x06,0x00,0x00,0x35};
const uint8_t pr1000_ptz_table_cvi_IRIS_PLUS[21] =	{0xA5,0x01,0x50,0x00,0x00,0x00,0xF6,0xA5,0x01,0x40,0x00,0x00,0x00,0xE6,0xA5,0x01,0x40,0x00,0x00,0x00,0xE6}; 
const uint8_t pr1000_ptz_table_cvi_IRIS_MINUS[21]=	{0xA5,0x01,0x60,0x00,0x00,0x00,0x06,0xA5,0x01,0x40,0x00,0x00,0x00,0xE6,0xA5,0x01,0x40,0x00,0x00,0x00,0xE6}; 
const uint8_t pr1000_ptz_table_cvi_FOCUS_PLUS[21]=	{0xA5,0x01,0x44,0x00,0x00,0x00,0xEA,0xA5,0x01,0x40,0x00,0x00,0x00,0xE6,0xA5,0x01,0x40,0x00,0x00,0x00,0xE6}; 
const uint8_t pr1000_ptz_table_cvi_FOCUS_MINUS[21]=	{0xA5,0x01,0x48,0x00,0x00,0x00,0xEE,0xA5,0x01,0x40,0x00,0x00,0x00,0xE6,0xA5,0x01,0x40,0x00,0x00,0x00,0xE6}; 
const uint8_t pr1000_ptz_table_cvi_ZOOM_PLUS[21] =	{0xA5,0x01,0x41,0x00,0x00,0x00,0xE7,0xA5,0x01,0x40,0x00,0x00,0x00,0xE6,0xA5,0x01,0x40,0x00,0x00,0x00,0xE6}; 
const uint8_t pr1000_ptz_table_cvi_ZOOM_MINUS[21]=	{0xA5,0x01,0x42,0x00,0x00,0x00,0xE8,0xA5,0x01,0x40,0x00,0x00,0x00,0xE6,0xA5,0x01,0x40,0x00,0x00,0x00,0xE6}; 
const uint8_t pr1000_ptz_table_cvi_DIR_TOP[21] = 	{0xA5,0x01,0x08,0x00,0x9F,0x00,0x4D,0xA5,0x01,0x00,0x00,0x9F,0x00,0x45,0xA5,0x01,0x00,0x00,0x9F,0x00,0x45}; 
const uint8_t pr1000_ptz_table_cvi_DIR_RIGHT[21] = 	{0xA5,0x01,0x01,0x9F,0x00,0x00,0x46,0xA5,0x01,0x00,0x9F,0x00,0x00,0x45,0xA5,0x01,0x00,0x9F,0x00,0x00,0x45}; 
const uint8_t pr1000_ptz_table_cvi_DIR_BOTTOM[21]= 	{0xA5,0x01,0x04,0x00,0x9F,0x00,0x49,0xA5,0x01,0x00,0x00,0x9F,0x00,0x45,0xA5,0x01,0x00,0x00,0x9F,0x00,0x45}; 
const uint8_t pr1000_ptz_table_cvi_DIR_LEFT[21] = 	{0xA5,0x01,0x02,0x9F,0x00,0x00,0x47,0xA5,0x01,0x00,0x9F,0x00,0x00,0x45,0xA5,0x01,0x00,0x9F,0x00,0x00,0x45};
const uint8_t pr1000_ptz_table_cvi_LENS_CLOSE[7] =	{0xA5,0x01,0x40,0x00,0x00,0x00,0xE6}; 
const uint8_t pr1000_ptz_table_cvi_DIR_CLOSE[7] =	{0xA5,0x01,0x00,0x00,0x9F,0x00,0x45}; 

/* CVI */
#ifndef DONT_SUPPORT_PTZ_ETC_CMD
const _pr1000PTZCmd pr1000_ptz_table_cvicmd[max_pr1000_ptz_table_command] = 
{/*{{{*/
	{ pr1000_ptz_table_cvi_OSD_TOP,    sizeof(pr1000_ptz_table_cvi_OSD_TOP)/sizeof(pr1000_ptz_table_cvi_OSD_TOP[0]) },
	{ pr1000_ptz_table_cvi_OSD_RIGHT,  sizeof(pr1000_ptz_table_cvi_OSD_RIGHT)/sizeof(pr1000_ptz_table_cvi_OSD_RIGHT[0]) },
	{ pr1000_ptz_table_cvi_OSD_BOTTOM, sizeof(pr1000_ptz_table_cvi_OSD_BOTTOM)/sizeof(pr1000_ptz_table_cvi_OSD_BOTTOM[0]) },
	{ pr1000_ptz_table_cvi_OSD_LEFT,   sizeof(pr1000_ptz_table_cvi_OSD_LEFT)/sizeof(pr1000_ptz_table_cvi_OSD_LEFT[0]) },
	{ NULL,    0 }, //OSD_CENTER
	{ pr1000_ptz_table_cvi_IRIS_PLUS,  sizeof(pr1000_ptz_table_cvi_IRIS_PLUS)/sizeof(pr1000_ptz_table_cvi_IRIS_PLUS[0]) },
	{ pr1000_ptz_table_cvi_IRIS_MINUS, sizeof(pr1000_ptz_table_cvi_IRIS_MINUS)/sizeof(pr1000_ptz_table_cvi_IRIS_MINUS[0]) },
	{ pr1000_ptz_table_cvi_FOCUS_PLUS, sizeof(pr1000_ptz_table_cvi_FOCUS_PLUS)/sizeof(pr1000_ptz_table_cvi_FOCUS_PLUS[0]) },
	{ pr1000_ptz_table_cvi_FOCUS_MINUS,sizeof(pr1000_ptz_table_cvi_FOCUS_MINUS)/sizeof(pr1000_ptz_table_cvi_FOCUS_MINUS[0]) },
	{ pr1000_ptz_table_cvi_ZOOM_PLUS,  sizeof(pr1000_ptz_table_cvi_ZOOM_PLUS)/sizeof(pr1000_ptz_table_cvi_ZOOM_PLUS[0]) },
	{ pr1000_ptz_table_cvi_ZOOM_MINUS, sizeof(pr1000_ptz_table_cvi_ZOOM_MINUS)/sizeof(pr1000_ptz_table_cvi_ZOOM_MINUS[0]) },
	{ pr1000_ptz_table_cvi_DIR_TOP,    sizeof(pr1000_ptz_table_cvi_DIR_TOP)/sizeof(pr1000_ptz_table_cvi_DIR_TOP[0]) },
	{ pr1000_ptz_table_cvi_DIR_RIGHT,  sizeof(pr1000_ptz_table_cvi_DIR_RIGHT)/sizeof(pr1000_ptz_table_cvi_DIR_RIGHT[0]) },
	{ pr1000_ptz_table_cvi_DIR_BOTTOM, sizeof(pr1000_ptz_table_cvi_DIR_BOTTOM)/sizeof(pr1000_ptz_table_cvi_DIR_BOTTOM[0]) },
	{ pr1000_ptz_table_cvi_DIR_LEFT,   sizeof(pr1000_ptz_table_cvi_DIR_LEFT)/sizeof(pr1000_ptz_table_cvi_DIR_LEFT[0]) },
	{ NULL,    0 }, //DIR_RIGHT_TOP
	{ NULL,    0 }, //DIR_RIGHT_BOTTOM
	{ NULL,    0 }, //DIR_LEFT_TOP
	{ NULL,    0 }, //DIR_LEFT_BOTTOM
	{ pr1000_ptz_table_cvi_LENS_CLOSE, sizeof(pr1000_ptz_table_cvi_LENS_CLOSE)/sizeof(pr1000_ptz_table_cvi_LENS_CLOSE[0]) },
	{ pr1000_ptz_table_cvi_DIR_CLOSE,   sizeof(pr1000_ptz_table_cvi_DIR_CLOSE)/sizeof(pr1000_ptz_table_cvi_DIR_CLOSE[0]) },
	{ NULL,    0 }, //RESERVED0
};/*}}}*/
#endif // DONT_SUPPORT_PTZ_ETC_CMD
#endif // DONT_SUPPORT_PTZ_FUNC

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*** CVI PTZ Parameter ***/
#ifndef DONT_SUPPORT_PTZ_FUNC
const _stPTZTxParam pr1000_ptz_txparam_cvi_def[6] =
{
	//720p60
	{/*{{{*/
		0, //int mapChn;

			0x00, //uint8_t tx_field_type; //reg 4x20 b[1:0]
			0x06, //uint8_t tx_line_cnt; //reg 4x21 b[7:3]
			0x00, //uint8_t tx_hst_os; //reg 4x21 b[2:0]
			0x09, //uint8_t tx_hst; //reg 4x22 b[6:0]
			0x03CAE7, //uint32_t tx_freq_first; //reg 4x23,4x24,4x25
			0x05B05B, //uint32_t tx_freq; //reg 4x26,4x27,4x28
			0x02DD, //uint16_t tx_hpst; //reg 4x29,4x2A b[12:0]
			0x20, //uint8_t tx_line_len; //reg 4x2B b[5:0]
			0x48, //uint8_t tx_all_data_len; //reg 4x2C b[7:0]
	},/*}}}*/
	//720p50
	{/*{{{*/
		0, //int mapChn;

			0x00, //uint8_t tx_field_type; //reg 4x20 b[1:0]
			0x06, //uint8_t tx_line_cnt; //reg 4x21 b[7:3]
			0x00, //uint8_t tx_hst_os; //reg 4x21 b[2:0]
			0x09, //uint8_t tx_hst; //reg 4x22 b[6:0]
			0x03CAE7, //uint32_t tx_freq_first; //reg 4x23,4x24,4x25
			0x05B05B, //uint32_t tx_freq; //reg 4x26,4x27,4x28
			0x04DD, //uint16_t tx_hpst; //reg 4x29,4x2A b[12:0]
			0x20, //uint8_t tx_line_len; //reg 4x2B b[5:0]
			0x48, //uint8_t tx_all_data_len; //reg 4x2C b[7:0]
	},/*}}}*/
	//720p30
	{/*{{{*/
		0, //int mapChn;

			0x00, //uint8_t tx_field_type; //reg 4x20 b[1:0]
			0x06, //uint8_t tx_line_cnt; //reg 4x21 b[7:3]
			0x00, //uint8_t tx_hst_os; //reg 4x21 b[2:0]
			0x0A, //uint8_t tx_hst; //reg 4x22 b[6:0]
			0x03CAE7, //uint32_t tx_freq_first; //reg 4x23,4x24,4x25
			0x05B05B, //uint32_t tx_freq; //reg 4x26,4x27,4x28
			0x0464, //uint16_t tx_hpst; //reg 4x29,4x2A b[12:0]
			0x20, //uint8_t tx_line_len; //reg 4x2B b[5:0]
			0x48, //uint8_t tx_all_data_len; //reg 4x2C b[7:0]
	},/*}}}*/
	//720p25
	{/*{{{*/
		0, //int mapChn;

			0x00, //uint8_t tx_field_type; //reg 4x20 b[1:0]
			0x06, //uint8_t tx_line_cnt; //reg 4x21 b[7:3]
			0x00, //uint8_t tx_hst_os; //reg 4x21 b[2:0]
			0x0A, //uint8_t tx_hst; //reg 4x22 b[6:0]
			0x03CAE7, //uint32_t tx_freq_first; //reg 4x23,4x24,4x25
			0x05B05B, //uint32_t tx_freq; //reg 4x26,4x27,4x28
			0x0464, //uint16_t tx_hpst; //reg 4x29,4x2A b[12:0]
			0x20, //uint8_t tx_line_len; //reg 4x2B b[5:0]
			0x48, //uint8_t tx_all_data_len; //reg 4x2C b[7:0]
	},/*}}}*/
	//1080p30
	{/*{{{*/
		0, //int mapChn;

			0x00, //uint8_t tx_field_type; //reg 4x20 b[1:0]
			0x06, //uint8_t tx_line_cnt; //reg 4x21 b[7:3]
			0x00, //uint8_t tx_hst_os; //reg 4x21 b[2:0]
			0x09, //uint8_t tx_hst; //reg 4x22 b[6:0]
			0x03CAE7, //uint32_t tx_freq_first; //reg 4x23,4x24,4x25
			0x05B05B, //uint32_t tx_freq; //reg 4x26,4x27,4x28
			0x0564, //uint16_t tx_hpst; //reg 4x29,4x2A b[12:0]
			0x20, //uint8_t tx_line_len; //reg 4x2B b[5:0]
			0x48, //uint8_t tx_all_data_len; //reg 4x2C b[7:0]
	},/*}}}*/
	//1080p25
	{/*{{{*/
		0, //int mapChn;

			0x00, //uint8_t tx_field_type; //reg 4x20 b[1:0]
			0x06, //uint8_t tx_line_cnt; //reg 4x21 b[7:3]
			0x00, //uint8_t tx_hst_os; //reg 4x21 b[2:0]
			0x0A, //uint8_t tx_hst; //reg 4x22 b[6:0]
			0x03CAE7, //uint32_t tx_freq_first; //reg 4x23,4x24,4x25
			0x05B05B, //uint32_t tx_freq; //reg 4x26,4x27,4x28
			0x0564, //uint16_t tx_hpst; //reg 4x29,4x2A b[12:0]
			0x20, //uint8_t tx_line_len; //reg 4x2B b[5:0]
			0x48, //uint8_t tx_all_data_len; //reg 4x2C b[7:0]
	},/*}}}*/

};
#endif // DONT_SUPPORT_PTZ_FUNC

const _stPTZRxParam pr1000_ptz_rxparam_cvi_def[6] =
{
	//720p60
	{/*{{{*/
		0, //int mapChn;

			0x00, //uint8_t rx_field_type; //reg 4x00 b[1:0]
			0x06, //uint8_t rx_line_cnt; //reg 4x01 b[7:3]
			0x00, //uint8_t rx_hst_os; //reg 4x01 b[2:0]
			0x09, //uint8_t rx_hst; //reg 4x02 b[6:0]
			0x03CAE7, //uint32_t rx_freq_first; //reg 4x03,4x04,4x05
			0x05B05B, //uint32_t rx_freq; //reg 4x06,4x07,4x08
			0x04, //uint8_t rx_lpf_len; //reg 4x09 b[5:0]
			0x10, //uint8_t rx_h_pix_offset; //reg 4x0A b[7:0]
			0x20, //uint8_t rx_line_len; //reg 4x0B b[5:0]
			0x1C, //uint8_t rx_valid_cnt; //reg 4x0C b[7:0]
	},/*}}}*/
	//720p50
	{/*{{{*/
		0, //int mapChn;

			0x00, //uint8_t rx_field_type; //reg 4x00 b[1:0]
			0x06, //uint8_t rx_line_cnt; //reg 4x01 b[7:3]
			0x00, //uint8_t rx_hst_os; //reg 4x01 b[2:0]
			0x09, //uint8_t rx_hst; //reg 4x02 b[6:0]
			0x03CAE7, //uint32_t rx_freq_first; //reg 4x03,4x04,4x05
			0x05B05B, //uint32_t rx_freq; //reg 4x06,4x07,4x08
			0x04, //uint8_t rx_lpf_len; //reg 4x09 b[5:0]
			0x10, //uint8_t rx_h_pix_offset; //reg 4x0A b[7:0]
			0x20, //uint8_t rx_line_len; //reg 4x0B b[5:0]
			0x1C, //uint8_t rx_valid_cnt; //reg 4x0C b[7:0]
	},/*}}}*/
	//720p30
	{/*{{{*/
		0, //int mapChn;

			0x00, //uint8_t rx_field_type; //reg 4x00 b[1:0]
			0x06, //uint8_t rx_line_cnt; //reg 4x01 b[7:3]
			0x00, //uint8_t rx_hst_os; //reg 4x01 b[2:0]
			0x0A, //uint8_t rx_hst; //reg 4x02 b[6:0]
			0x03CAE7, //uint32_t rx_freq_first; //reg 4x03,4x04,4x05
			0x05B05B, //uint32_t rx_freq; //reg 4x06,4x07,4x08
			0x04, //uint8_t rx_lpf_len; //reg 4x09 b[5:0]
			0x10, //uint8_t rx_h_pix_offset; //reg 4x0A b[7:0]
			0x20, //uint8_t rx_line_len; //reg 4x0B b[5:0]
			0x1C, //uint8_t rx_valid_cnt; //reg 4x0C b[7:0]
	},/*}}}*/
	//720p25
	{/*{{{*/
		0, //int mapChn;

			0x00, //uint8_t rx_field_type; //reg 4x00 b[1:0]
			0x06, //uint8_t rx_line_cnt; //reg 4x01 b[7:3]
			0x00, //uint8_t rx_hst_os; //reg 4x01 b[2:0]
			0x0A, //uint8_t rx_hst; //reg 4x02 b[6:0]
			0x03CAE7, //uint32_t rx_freq_first; //reg 4x03,4x04,4x05
			0x05B05B, //uint32_t rx_freq; //reg 4x06,4x07,4x08
			0x04, //uint8_t rx_lpf_len; //reg 4x09 b[5:0]
			0x10, //uint8_t rx_h_pix_offset; //reg 4x0A b[7:0]
			0x20, //uint8_t rx_line_len; //reg 4x0B b[5:0]
			0x1C, //uint8_t rx_valid_cnt; //reg 4x0C b[7:0]
	},/*}}}*/
	//1080p30
	{/*{{{*/
		0, //int mapChn;

			0x00, //uint8_t rx_field_type; //reg 4x00 b[1:0]
			0x06, //uint8_t rx_line_cnt; //reg 4x01 b[7:3]
			0x00, //uint8_t rx_hst_os; //reg 4x01 b[2:0]
			0x09, //uint8_t rx_hst; //reg 4x02 b[6:0]
			0x03CAE7, //uint32_t rx_freq_first; //reg 4x03,4x04,4x05
			0x05B05B, //uint32_t rx_freq; //reg 4x06,4x07,4x08
			0x04, //uint8_t rx_lpf_len; //reg 4x09 b[5:0]
			0x10, //uint8_t rx_h_pix_offset; //reg 4x0A b[7:0]
			0x20, //uint8_t rx_line_len; //reg 4x0B b[5:0]
			0x1C, //uint8_t rx_valid_cnt; //reg 4x0C b[7:0]
	},/*}}}*/
	//1080p25
	{/*{{{*/
		0, //int mapChn;

			0x00, //uint8_t rx_field_type; //reg 4x00 b[1:0]
			0x06, //uint8_t rx_line_cnt; //reg 4x01 b[7:3]
			0x00, //uint8_t rx_hst_os; //reg 4x01 b[2:0]
			0x0A, //uint8_t rx_hst; //reg 4x02 b[6:0]
			0x03CAE7, //uint32_t rx_freq_first; //reg 4x03,4x04,4x05
			0x05B05B, //uint32_t rx_freq; //reg 4x06,4x07,4x08
			0x04, //uint8_t rx_lpf_len; //reg 4x09 b[5:0]
			0x10, //uint8_t rx_h_pix_offset; //reg 4x0A b[7:0]
			0x20, //uint8_t rx_line_len; //reg 4x0B b[5:0]
			0x1C, //uint8_t rx_valid_cnt; //reg 4x0C b[7:0]
	},/*}}}*/

};
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*** CVI PATTERN ***/
#ifndef DONT_SUPPORT_PTZ_FUNC
/* PTZ CVI Tx pattern format */
const unsigned char pr1000_ptz_table_cvi_tx_pat_format[((4*6)*3)] = 
{/*{{{*/
	/* frame 1 */
	/* 1 */ 0xC0, 0x00, 0x3F, 0xFF, //"1100 0000 0000 0000 0011 1111 1111 1111"
	/* 2 */ 0xC0, 0x00, 0x3F, 0xFF, //"1100 0000 0000 0000 0011 1111 1111 1111"
	/* 3 */ 0xC0, 0x00, 0x3F, 0xFF, //"1100 0000 0000 0000 0011 1111 1111 1111"
	/* 4 */ 0xC0, 0x00, 0x3F, 0xFF, //"1100 0000 0000 0000 0011 1111 1111 1111"
	/* 5 */ 0xC0, 0x00, 0x3F, 0xFF, //"1100 0000 0000 0000 0011 1111 1111 1111"
	/* 6 */ 0xC0, 0x00, 0x3F, 0xFF, //"1100 0000 0000 0000 0011 1111 1111 1111"
	/* frame 2 */
	/* 1 */ 0xC0, 0x00, 0x3F, 0xFF, //"1100 0000 0000 0000 0011 1111 1111 1111"
	/* 2 */ 0xFF, 0xFF, 0xFF, 0xFF, //"1111 1111 1111 1111 1111 1111 1111 1111"
	/* 3 */ 0xFF, 0xFF, 0xFF, 0xFF, //"1111 1111 1111 1111 1111 1111 1111 1111"
	/* 4 */ 0xFF, 0xFF, 0xFF, 0xFF, //"1111 1111 1111 1111 1111 1111 1111 1111"
	/* 5 */ 0xFF, 0xFF, 0xFF, 0xFF, //"1111 1111 1111 1111 1111 1111 1111 1111"
	/* 6 */ 0xFF, 0xFF, 0xFF, 0xFF, //"1111 1111 1111 1111 1111 1111 1111 1111"
	/* frame 3 */
	/* 1 */ 0xFF, 0xFF, 0xFF, 0xFF, //"1111 1111 1111 1111 1111 1111 1111 1111"
	/* 2 */ 0xFF, 0xFF, 0xFF, 0xFF, //"1111 1111 1111 1111 1111 1111 1111 1111"
	/* 3 */ 0xFF, 0xFF, 0xFF, 0xFF, //"1111 1111 1111 1111 1111 1111 1111 1111"
	/* 4 */ 0xFF, 0xFF, 0xFF, 0xFF, //"1111 1111 1111 1111 1111 1111 1111 1111"
	/* 5 */ 0xFF, 0xFF, 0xFF, 0xFF, //"1111 1111 1111 1111 1111 1111 1111 1111"
	/* 6 */ 0xFF, 0xFF, 0xFF, 0xFF, //"1111 1111 1111 1111 1111 1111 1111 1111"
};/*}}}*/
/* PTZ CVI Tx pattern data */
const unsigned char pr1000_ptz_table_cvi_tx_pat_data[((4*6)*3)] = 
{/*{{{*/
	/* frame 1 */
	/* 1 */ 0x80, 0x00, 0x3F, 0xFE, //"10xx xxxx xxxx xxxx xx11 1111 1111 1110"
	/* 2 */ 0x80, 0x00, 0x3F, 0xFE, //"10xx xxxx xxxx xxxx xx11 1111 1111 1110"
	/* 3 */ 0x80, 0x00, 0x3F, 0xFE, //"10xx xxxx xxxx xxxx xx11 1111 1111 1110"
	/* 4 */ 0x80, 0x00, 0x3F, 0xFE, //"10xx xxxx xxxx xxxx xx11 1111 1111 1110"
	/* 5 */ 0x80, 0x00, 0x3F, 0xFE, //"10xx xxxx xxxx xxxx xx11 1111 1111 1110"
	/* 6 */ 0x80, 0x00, 0x3F, 0xFE, //"10xx xxxx xxxx xxxx xx11 1111 1111 1110"
	/* frame 2 */
	/* 1 */ 0x80, 0x00, 0x3F, 0xFE, //"10xx xxxx xxxx xxxx xx11 1111 1111 1110"
	/* 2 */ 0xFF, 0xFF, 0xFF, 0xFE, //"1111 1111 1111 1111 1111 1111 1111 1110"
	/* 3 */ 0xFF, 0xFF, 0xFF, 0xFE, //"1111 1111 1111 1111 1111 1111 1111 1110"
	/* 4 */ 0xFF, 0xFF, 0xFF, 0xFE, //"1111 1111 1111 1111 1111 1111 1111 1110"
	/* 5 */ 0xFF, 0xFF, 0xFF, 0xFE, //"1111 1111 1111 1111 1111 1111 1111 1110"
	/* 6 */ 0xFF, 0xFF, 0xFF, 0xFE, //"1111 1111 1111 1111 1111 1111 1111 1110"
	/* frame 3 */
	/* 1 */ 0xFF, 0xFF, 0xFF, 0xFE, //"1111 1111 1111 1111 1111 1111 1111 1110"
	/* 2 */ 0xFF, 0xFF, 0xFF, 0xFE, //"1111 1111 1111 1111 1111 1111 1111 1110"
	/* 3 */ 0xFF, 0xFF, 0xFF, 0xFE, //"1111 1111 1111 1111 1111 1111 1111 1110"
	/* 4 */ 0xFF, 0xFF, 0xFF, 0xFE, //"1111 1111 1111 1111 1111 1111 1111 1110"
	/* 5 */ 0xFF, 0xFF, 0xFF, 0xFE, //"1111 1111 1111 1111 1111 1111 1111 1110"
	/* 6 */ 0xFF, 0xFF, 0xFF, 0xFE, //"1111 1111 1111 1111 1111 1111 1111 1110"
};/*}}}*/
#endif // DONT_SUPPORT_PTZ_FUNC

/* PTZ CVI Rx pattern format */
const unsigned char pr1000_ptz_table_cvi_rx_pat_format[((4*6)*3)] = 
{/*{{{*/
	/* frame 1 */
	/* 1 */ 0xC0, 0x00, 0x3F, 0xFF, //"1100 0000 0011 1111 1111 1111 1111 1111"
	/* 2 */ 0xC0, 0x00, 0x3F, 0xFF, //"1100 0000 0011 1111 1111 1111 1111 1111"
	/* 3 */ 0xC0, 0x00, 0x3F, 0xFF, //"1100 0000 0011 1111 1111 1111 1111 1111"
	/* 4 */ 0xC0, 0x00, 0x3F, 0xFF, //"1100 0000 0011 1111 1111 1111 1111 1111"
	/* 5 */ 0xC0, 0x00, 0x3F, 0xFF, //"1100 0000 0011 1111 1111 1111 1111 1111"
	/* 6 */ 0xC0, 0x00, 0x3F, 0xFF, //"1100 0000 0011 1111 1111 1111 1111 1111"
	/* frame 2 */
	/* 1 */ 0xC0, 0x00, 0x3F, 0xFF, //"1100 0000 0011 1111 1111 1111 1111 1111"
	/* 2 */ 0xFF, 0xFF, 0xFF, 0xFF, //"1111 1111 1111 1111 1111 1111 1111 1111"
	/* 3 */ 0xFF, 0xFF, 0xFF, 0xFF, //"1111 1111 1111 1111 1111 1111 1111 1111"
	/* 4 */ 0xFF, 0xFF, 0xFF, 0xFF, //"1111 1111 1111 1111 1111 1111 1111 1111"
	/* 5 */ 0xFF, 0xFF, 0xFF, 0xFF, //"1111 1111 1111 1111 1111 1111 1111 1111"
	/* 6 */ 0xFF, 0xFF, 0xFF, 0xFF, //"1111 1111 1111 1111 1111 1111 1111 1111"
	/* frame 3 */
	/* 1 */ 0xFF, 0xFF, 0xFF, 0xFF, //"1111 1111 1111 1111 1111 1111 1111 1111"
	/* 2 */ 0xFF, 0xFF, 0xFF, 0xFF, //"1111 1111 1111 1111 1111 1111 1111 1111"
	/* 3 */ 0xFF, 0xFF, 0xFF, 0xFF, //"1111 1111 1111 1111 1111 1111 1111 1111"
	/* 4 */ 0xFF, 0xFF, 0xFF, 0xFF, //"1111 1111 1111 1111 1111 1111 1111 1111"
	/* 5 */ 0xFF, 0xFF, 0xFF, 0xFF, //"1111 1111 1111 1111 1111 1111 1111 1111"
	/* 6 */ 0xFF, 0xFF, 0xFF, 0xFF, //"1111 1111 1111 1111 1111 1111 1111 1111"
};/*}}}*/
/* PTZ CVI Rx pattern start format */
const unsigned char pr1000_ptz_table_cvi_rx_pat_start_format[(4*1)] = 
{/*{{{*/
	0xFF, 0xFF, 0xFF, 0xFF, //"11 11111111 11 1111 1111 1111 1111 1111"
};/*}}}*/
/* PTZ CVI Rx pattern start data */
const unsigned char pr1000_ptz_table_cvi_rx_pat_start_data[(4*1)] = 
{/*{{{*/
	0xA9, 0x5F, 0xFF, 0xFE, //"10 10100101 01 1111 1111 1111 1111 1110"
};/*}}}*/
//////////////////////////////////////////////////////////////////////////////////////////

