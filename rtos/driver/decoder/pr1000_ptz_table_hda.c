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
#include "pr1000_ptz_table_hda.h"

#else //#ifdef __LINUX_SYSTEM__

#include <stdio.h>
#include "pr1000_drvcommon.h"
#include "pr1000_ptz_table.h"
#include "pr1000_ptz_table_hda.h"
#include "pr1000_user_config.h"

#endif // __LINUX_SYSTEM__

/////////////////////////////////////////////////////////////////////////////////////////////
#ifndef DONT_SUPPORT_PTZ_FUNC
/*** HDA COMMAND ***/
/* hda special escape sync code */
const uint8_t pr1000_ptz_table_hda_ESCAPE_CODE0_2Byte_1080p[2] = 
	{0x00,0x00,}; //escape 0x00,0x00,0x00,0x00 or 0x00,0x00
const uint8_t pr1000_ptz_table_hda_ESCAPE_CODE1_1Byte_1080p[1] = 
	{0xAA,};
const uint8_t pr1000_ptz_table_hda_CMD_VALID_1080p[(4*6)] = 
	{0x00,0x00,0x00,0x00,/**/0x00,0x00,0x00,0x00,/**/0xFF,0xFF,0x00,0x00,/**/0x00,0x00,0x00,0x00,/**/0x00,0x00,0x00,0x00,/**/0x00,0x00,0x00,0x00,};// define rx compare byte. 1's compare bit

const uint8_t pr1000_ptz_table_hda_RESET_1080p[(4*6)] = 	
	{0x00,0x00,0x00,0x00,/**/0xAA,0x3C,0xFF,0xFF,/**/0x00,0x00,0x00,0x00,/**/0xAA,0x3C,0xFF,0xFF,/**/0xAA,0x1B,0x00,0x00,/**/0xAA,0x3B,0x00,0x00,};
const uint8_t pr1000_ptz_table_hda_SET_1080p[(4*6)] = 	
	{0x00,0x00,0x00,0x00,/**/0xAA,0x3C,0xFF,0xFF,/**/0x02,0x00,0x00,0x00,/**/0xAA,0x3C,0xFF,0xFF,/**/0xAA,0x1B,0x00,0x00,/**/0xAA,0x3B,0x00,0x00,};
const uint8_t pr1000_ptz_table_hda_UP_1080p[(4*6)] = 	
	{0x00,0x00,0x00,0x00,/**/0xAA,0x3C,0xFF,0xFF,/**/0x00,0x08,0x00,0x32,/**/0xAA,0x08,0x00,0xFF,/**/0xAA,0x3C,0x00,0xFF,/**/0xAA,0x3B,0x00,0x00,};
const uint8_t pr1000_ptz_table_hda_RIGHT_1080p[(4*6)] =	
	{0x00,0x00,0x00,0x00,/**/0xAA,0x3C,0xFF,0xFF,/**/0x00,0x02,0x32,0x00,/**/0xAA,0x02,0x32,0x00,/**/0xAA,0x3C,0x00,0xFF,/**/0xAA,0x3B,0x00,0x00,};
const uint8_t pr1000_ptz_table_hda_DOWN_1080p[(4*6)]=	
	{0x00,0x00,0x00,0x00,/**/0xAA,0x3C,0xFF,0xFF,/**/0x00,0x10,0x00,0x32,/**/0xAA,0x3C,0x00,0x32,/**/0xAA,0x3C,0x00,0xFF,/**/0xAA,0x3B,0x00,0x00,};
const uint8_t pr1000_ptz_table_hda_LEFT_1080p[(4*6)] = 	
	{0x00,0x00,0x00,0x00,/**/0xAA,0x3C,0xFF,0xFF,/**/0x00,0x04,0x32,0x00,/**/0xAA,0x3C,0x32,0xFF,/**/0xAA,0x3C,0x00,0xFF,/**/0xAA,0x3B,0x00,0x00,};

const uint8_t pr1000_ptz_table_hda_LEFT_UP_1080p[(4*6)]= 	
	{0x00,0x00,0x00,0x00,/**/0xAA,0x3C,0xFF,0xFF,/**/0x00,0x0C,0x00,0x32,/**/0xAA,0x3C,0xFF,0xFF,/**/0xAA,0x1B,0x00,0x00,/**/0xAA,0x3B,0x00,0x00,};
const uint8_t pr1000_ptz_table_hda_LEFT_DOWN_1080p[(4*6)]= 	
	{0x00,0x00,0x00,0x00,/**/0xAA,0x3C,0xFF,0xFF,/**/0x00,0x14,0x00,0x32,/**/0xAA,0x3C,0xFF,0xFF,/**/0xAA,0x1B,0x00,0x00,/**/0xAA,0x3B,0x00,0x00,};
const uint8_t pr1000_ptz_table_hda_RIGHT_UP_1080p[(4*6)]= 	
	{0x00,0x00,0x00,0x00,/**/0xAA,0x3C,0xFF,0xFF,/**/0x00,0x0A,0x00,0x32,/**/0xAA,0x3C,0xFF,0xFF,/**/0xAA,0x1B,0x00,0x00,/**/0xAA,0x3B,0x00,0x00,};
const uint8_t pr1000_ptz_table_hda_RIGHT_DOWN_1080p[(4*6)]= 	
	{0x00,0x00,0x00,0x00,/**/0xAA,0x3C,0xFF,0xFF,/**/0x00,0x12,0x00,0x32,/**/0xAA,0x3C,0xFF,0xFF,/**/0xAA,0x1B,0x00,0x00,/**/0xAA,0x3B,0x00,0x00,};
const uint8_t pr1000_ptz_table_hda_IRIS_OPEN_1080p[(4*6)] =	
	{0x00,0x00,0x00,0x00,/**/0xAA,0x3C,0xFF,0xFF,/**/0x02,0x00,0x00,0x00,/**/0xAA,0x3C,0xFF,0xFF,/**/0xAA,0x1B,0x00,0x00,/**/0xAA,0x3B,0x00,0x00,};
const uint8_t pr1000_ptz_table_hda_IRIS_CLOSE_1080p[(4*6)]=	
	{0x00,0x00,0x00,0x00,/**/0xAA,0x3C,0xFF,0xFF,/**/0x04,0x00,0x00,0x00,/**/0xAA,0x3C,0xFF,0xFF,/**/0xAA,0x1B,0x00,0x00,/**/0xAA,0x3B,0x00,0x00,};
const uint8_t pr1000_ptz_table_hda_FOCUS_FAR_1080p[(4*6)] =	
	{0x00,0x00,0x00,0x00,/**/0xAA,0x3C,0xFF,0xFF,/**/0x00,0x80,0x00,0x00,/**/0xAA,0x3C,0xFF,0xFF,/**/0xAA,0x1B,0x00,0x00,/**/0xAA,0x3B,0x00,0x00,};
#ifndef DONT_SUPPORT_PTZ_ETC_CMD
const uint8_t pr1000_ptz_table_hda_FOCUS_NEAR_1080p[(4*6)]=	
	{0x00,0x00,0x00,0x00,/**/0xAA,0x3C,0xFF,0xFF,/**/0x01,0x00,0x00,0x00,/**/0xAA,0x3C,0xFF,0xFF,/**/0xAA,0x1B,0x00,0x00,/**/0xAA,0x3B,0x00,0x00,};
const uint8_t pr1000_ptz_table_hda_ZOOM_TELE_1080p[(4*6)] =	
	{0x00,0x00,0x00,0x00,/**/0xAA,0x3C,0xFF,0xFF,/**/0x00,0x20,0x00,0x00,/**/0xAA,0x3C,0xFF,0xFF,/**/0xAA,0x1B,0x00,0x00,/**/0xAA,0x3B,0x00,0x00,};
const uint8_t pr1000_ptz_table_hda_ZOOM_WIDE_1080p[(4*6)]=	
	{0x00,0x00,0x00,0x00,/**/0xAA,0x3C,0xFF,0xFF,/**/0x00,0x40,0x00,0x00,/**/0xAA,0x3C,0xFF,0xFF,/**/0xAA,0x1B,0x00,0x00,/**/0xAA,0x3B,0x00,0x00,};
const uint8_t pr1000_ptz_table_hda_SET_POINT_1080p[(4*6)] =	
	{0x00,0x00,0x00,0x00,/**/0xAA,0x3C,0xFF,0xFF,/**/0x00,0x03,0x00,0x00,/**/0xAA,0x3C,0xFF,0xFF,/**/0xAA,0x1B,0x00,0x00,/**/0xAA,0x3B,0x00,0x00,};
const uint8_t pr1000_ptz_table_hda_CLEAR_POINT_1080p[(4*6)]=	
	{0x00,0x00,0x00,0x00,/**/0xAA,0x3C,0xFF,0xFF,/**/0x00,0x05,0x00,0x00,/**/0xAA,0x3C,0xFF,0xFF,/**/0xAA,0x1B,0x00,0x00,/**/0xAA,0x3B,0x00,0x00,};
const uint8_t pr1000_ptz_table_hda_GOTO_POINT_1080p[(4*6)] =	
	{0x00,0x00,0x00,0x00,/**/0xAA,0x3C,0xFF,0xFF,/**/0x00,0x07,0x00,0x00,/**/0xAA,0x3C,0xFF,0xFF,/**/0xAA,0x1B,0x00,0x00,/**/0xAA,0x3B,0x00,0x00,};
const uint8_t pr1000_ptz_table_hda_AUTO_1080p[(4*6)]=	
	{0x00,0x00,0x00,0x00,/**/0xAA,0x3C,0xFF,0xFF,/**/0x00,0x07,0x00,0x63,/**/0xAA,0x3C,0xFF,0xFF,/**/0xAA,0x1B,0x00,0x00,/**/0xAA,0x3B,0x00,0x00,};
const uint8_t pr1000_ptz_table_hda_AUTO_STOP_1080p[(4*6)]=	
	{0x00,0x00,0x00,0x00,/**/0xAA,0x3C,0xFF,0xFF,/**/0x00,0x07,0x00,0x60,/**/0xAA,0x3C,0xFF,0xFF,/**/0xAA,0x1B,0x00,0x00,/**/0xAA,0x3B,0x00,0x00,};
#endif // DONT_SUPPORT_PTZ_ETC_CMD
const uint8_t pr1000_ptz_table_hda_RESERVED0_1080p[(4*6)] = 
	{0x00,0x00,0x00,0x00,/**/0xAA,0x3C,0xFF,0xFF,/**/0x00,0x00,0x00,0x00,/**/0xAA,0x3C,0xFF,0xFF,/**/0xAA,0x1B,0x00,0x00,/**/0xAA,0x3B,0x00,0x00,};

const uint8_t pr1000_ptz_table_hda_ESCAPE_CODE0_4Byte_720p[(4*1)] = 
	{0x00,0x00,0x00,0x00,}; //escape 0x00,0x00,0x00,0x00
const uint8_t pr1000_ptz_table_hda_CMD_VALID_720p[(4*1)] = 
	{0xFF,0xFF,0xFF,0xFF,};// define rx compare byte. 1's compare bit
const uint8_t pr1000_ptz_table_hda_RESET_720p[(4*1)] = 	
	{0x00,0x00,0x00,0x00,};
const uint8_t pr1000_ptz_table_hda_SET_720p[(4*1)] = 	
	{0x40,0x00,0x00,0x00,};
const uint8_t pr1000_ptz_table_hda_OSD_720p[(4*1)] = 	
	{0x00,0xC0,0xC0,0xFA,};
const uint8_t pr1000_ptz_table_hda_UP_720p[(4*1)] = 	
	{0x00,0x10,0x10,0x4C,};
const uint8_t pr1000_ptz_table_hda_DOWN_720p[(4*1)]=	
	{0x00,0x08,0x08,0x4C,};
const uint8_t pr1000_ptz_table_hda_LEFT_720p[(4*1)] = 	
	{0x00,0x20,0x20,0x00,};
const uint8_t pr1000_ptz_table_hda_RIGHT_720p[(4*1)] =	
	{0x00,0x40,0x40,0x00,};

const uint8_t pr1000_ptz_table_hda_IRIS_OPEN_720p[(4*1)] =	
	{0x40,0x00,0x00,0x00,};
const uint8_t pr1000_ptz_table_hda_IRIS_CLOSE_720p[(4*1)]=	
	{0x20,0x00,0x00,0x00,};
#ifndef DONT_SUPPORT_PTZ_ETC_CMD
const uint8_t pr1000_ptz_table_hda_FOCUS_FAR_720p[(4*1)] =	
	{0x00,0x01,0x01,0x00,};
const uint8_t pr1000_ptz_table_hda_FOCUS_NEAR_720p[(4*1)]=	
	{0x80,0x00,0x00,0x00,};
const uint8_t pr1000_ptz_table_hda_ZOOM_TELE_720p[(4*1)] =	
	{0x00,0x04,0x04,0x00,};
const uint8_t pr1000_ptz_table_hda_ZOOM_WIDE_720p[(4*1)]=	
	{0x00,0x02,0x02,0x00,};
const uint8_t pr1000_ptz_table_hda_SCAN_SR_720p[(4*1)] =	
	{0x00,0xE0,0xE0,0x46,};
const uint8_t pr1000_ptz_table_hda_SCAN_ST_720p[(4*1)]=	
	{0x00,0xE0,0xE0,0x00,};
const uint8_t pr1000_ptz_table_hda_PRESET1_720p[(4*1)] =	
	{0x00,0xE0,0xE0,0x80,};
const uint8_t pr1000_ptz_table_hda_PRESET2_720p[(4*1)]=	
	{0x00,0xE0,0xE0,0x40,};
const uint8_t pr1000_ptz_table_hda_PRESET3_720p[(4*1)]=	
	{0x00,0xE0,0xE0,0xC0,};
const uint8_t pr1000_ptz_table_hda_PTN1_SR_720p[(4*1)]=	
	{0x00,0xF8,0xF8,0x01,};
const uint8_t pr1000_ptz_table_hda_PTN1_ST_720p[(4*1)]=	
	{0x00,0x84,0x84,0x01,};
const uint8_t pr1000_ptz_table_hda_PTN2_SR_720p[(4*1)]=	
	{0x00,0xF8,0xF8,0x02,};
const uint8_t pr1000_ptz_table_hda_PTN2_ST_720p[(4*1)]=	
	{0x00,0x84,0x84,0x02,};
const uint8_t pr1000_ptz_table_hda_PTN3_SR_720p[(4*1)]=	
	{0x00,0xF8,0xF8,0x03,};
const uint8_t pr1000_ptz_table_hda_PTN3_ST_720p[(4*1)]=	
	{0x00,0x84,0x84,0x03,};
const uint8_t pr1000_ptz_table_hda_RUN_720p[(4*1)]=	
	{0x00,0xC4,0xC4,0x00,};
#endif // DONT_SUPPORT_PTZ_ETC_CMD

const uint8_t pr1000_ptz_table_hda_RESERVED0_720p[(4*1)] = 	
	{0x00,0x00,0x00,0x00,};

/* HDA */
#ifndef DONT_SUPPORT_PTZ_ETC_CMD
const _pr1000PTZCmd pr1000_ptz_table_hdacmd_1080p[max_pr1000_ptz_table_command] = 
{/*{{{*/
	{ pr1000_ptz_table_hda_UP_1080p,    sizeof(pr1000_ptz_table_hda_UP_1080p)/sizeof(pr1000_ptz_table_hda_UP_1080p[0]) },
	{ pr1000_ptz_table_hda_RIGHT_1080p,  sizeof(pr1000_ptz_table_hda_RIGHT_1080p)/sizeof(pr1000_ptz_table_hda_RIGHT_1080p[0]) },
	{ pr1000_ptz_table_hda_DOWN_1080p, sizeof(pr1000_ptz_table_hda_DOWN_1080p)/sizeof(pr1000_ptz_table_hda_DOWN_1080p[0]) },
	{ pr1000_ptz_table_hda_LEFT_1080p,   sizeof(pr1000_ptz_table_hda_LEFT_1080p)/sizeof(pr1000_ptz_table_hda_LEFT_1080p[0]) },
	{ pr1000_ptz_table_hda_SET_1080p,   sizeof(pr1000_ptz_table_hda_SET_1080p)/sizeof(pr1000_ptz_table_hda_SET_1080p[0]) },
	{ pr1000_ptz_table_hda_IRIS_OPEN_1080p,  sizeof(pr1000_ptz_table_hda_IRIS_OPEN_1080p)/sizeof(pr1000_ptz_table_hda_IRIS_OPEN_1080p[0]) },
	{ pr1000_ptz_table_hda_IRIS_CLOSE_1080p, sizeof(pr1000_ptz_table_hda_IRIS_CLOSE_1080p)/sizeof(pr1000_ptz_table_hda_IRIS_CLOSE_1080p[0]) },
	{ pr1000_ptz_table_hda_FOCUS_FAR_1080p, sizeof(pr1000_ptz_table_hda_FOCUS_FAR_1080p)/sizeof(pr1000_ptz_table_hda_FOCUS_FAR_1080p[0]) },
	{ pr1000_ptz_table_hda_FOCUS_NEAR_1080p,sizeof(pr1000_ptz_table_hda_FOCUS_NEAR_1080p)/sizeof(pr1000_ptz_table_hda_FOCUS_NEAR_1080p[0]) },
	{ pr1000_ptz_table_hda_ZOOM_TELE_1080p,  sizeof(pr1000_ptz_table_hda_ZOOM_TELE_1080p)/sizeof(pr1000_ptz_table_hda_ZOOM_TELE_1080p[0]) },
	{ pr1000_ptz_table_hda_ZOOM_WIDE_1080p, sizeof(pr1000_ptz_table_hda_ZOOM_WIDE_1080p)/sizeof(pr1000_ptz_table_hda_ZOOM_WIDE_1080p[0]) },
	{ NULL,    0 },
	{ NULL,    0 },
	{ NULL,    0 },
	{ NULL,    0 },
	{ pr1000_ptz_table_hda_RIGHT_UP_1080p,  sizeof(pr1000_ptz_table_hda_RIGHT_UP_1080p)/sizeof(pr1000_ptz_table_hda_RIGHT_UP_1080p[0]) },
	{ pr1000_ptz_table_hda_RIGHT_DOWN_1080p,  sizeof(pr1000_ptz_table_hda_RIGHT_DOWN_1080p)/sizeof(pr1000_ptz_table_hda_RIGHT_DOWN_1080p[0]) },
	{ pr1000_ptz_table_hda_LEFT_UP_1080p,  sizeof(pr1000_ptz_table_hda_LEFT_UP_1080p)/sizeof(pr1000_ptz_table_hda_LEFT_UP_1080p[0]) },
	{ pr1000_ptz_table_hda_LEFT_DOWN_1080p,  sizeof(pr1000_ptz_table_hda_LEFT_DOWN_1080p)/sizeof(pr1000_ptz_table_hda_LEFT_DOWN_1080p[0]) },
	{ NULL,    0 }, //LENS_CLOSE
	{ NULL,    0 }, //DIR_CLOSE
	{ pr1000_ptz_table_hda_RESET_1080p,  sizeof(pr1000_ptz_table_hda_RESET_1080p)/sizeof(pr1000_ptz_table_hda_RESET_1080p[0]) }, //RESERVED0
};/*}}}*/
const _pr1000PTZCmd pr1000_ptz_table_hdacmd_720p[max_pr1000_ptz_table_command] = 
{/*{{{*/
	{ pr1000_ptz_table_hda_UP_720p,    sizeof(pr1000_ptz_table_hda_UP_720p)/sizeof(pr1000_ptz_table_hda_UP_720p[0]) },
	{ pr1000_ptz_table_hda_RIGHT_720p,  sizeof(pr1000_ptz_table_hda_RIGHT_720p)/sizeof(pr1000_ptz_table_hda_RIGHT_720p[0]) },
	{ pr1000_ptz_table_hda_DOWN_720p, sizeof(pr1000_ptz_table_hda_DOWN_720p)/sizeof(pr1000_ptz_table_hda_DOWN_720p[0]) },
	{ pr1000_ptz_table_hda_LEFT_720p,   sizeof(pr1000_ptz_table_hda_LEFT_720p)/sizeof(pr1000_ptz_table_hda_LEFT_720p[0]) },
	{ pr1000_ptz_table_hda_SET_720p,   sizeof(pr1000_ptz_table_hda_SET_720p)/sizeof(pr1000_ptz_table_hda_SET_720p[0]) },
	{ pr1000_ptz_table_hda_IRIS_OPEN_720p,  sizeof(pr1000_ptz_table_hda_IRIS_OPEN_720p)/sizeof(pr1000_ptz_table_hda_IRIS_OPEN_720p[0]) },
	{ pr1000_ptz_table_hda_IRIS_CLOSE_720p, sizeof(pr1000_ptz_table_hda_IRIS_CLOSE_720p)/sizeof(pr1000_ptz_table_hda_IRIS_CLOSE_720p[0]) },
	{ pr1000_ptz_table_hda_FOCUS_FAR_720p, sizeof(pr1000_ptz_table_hda_FOCUS_FAR_720p)/sizeof(pr1000_ptz_table_hda_FOCUS_FAR_720p[0]) },
	{ pr1000_ptz_table_hda_FOCUS_NEAR_720p,sizeof(pr1000_ptz_table_hda_FOCUS_NEAR_720p)/sizeof(pr1000_ptz_table_hda_FOCUS_NEAR_720p[0]) },
	{ pr1000_ptz_table_hda_ZOOM_TELE_720p,  sizeof(pr1000_ptz_table_hda_ZOOM_TELE_720p)/sizeof(pr1000_ptz_table_hda_ZOOM_TELE_720p[0]) },
	{ pr1000_ptz_table_hda_ZOOM_WIDE_720p, sizeof(pr1000_ptz_table_hda_ZOOM_WIDE_720p)/sizeof(pr1000_ptz_table_hda_ZOOM_WIDE_720p[0]) },
	{ NULL,    0 }, //DIR_TOP
	{ NULL,    0 }, //DIR_RIGHT
	{ NULL,    0 }, //DIR_BOTTOM
	{ NULL,    0 }, //DIR_LEFT
	{ NULL,    0 }, //DIR_RIGHT_TOP
	{ NULL,    0 }, //DIR_RIGHT_BOTTOM
	{ NULL,    0 }, //DIR_LEFT_TOP
	{ NULL,    0 }, //DIR_LEFT_BOTTOM
	{ NULL,    0 }, //LENS_CLOSE
	{ NULL,    0 }, //DIR_CLOSE
	{ pr1000_ptz_table_hda_RESET_720p,  sizeof(pr1000_ptz_table_hda_RESET_720p)/sizeof(pr1000_ptz_table_hda_RESET_720p[0]) }, //RESERVED0
};/*}}}*/
#endif // DONT_SUPPORT_PTZ_ETC_CMD
#endif // DONT_SUPPORT_PTZ_FUNC

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*** HDA PTZ Parameter ***/
#ifndef DONT_SUPPORT_PTZ_FUNC
const _stPTZTxParam pr1000_ptz_txparam_hda_def[6] =
{
	//720p60
	{/*{{{*/
		0, //int mapChn;

			0x00, //uint8_t tx_field_type; //reg 4x20 b[1:0]
			0x02, //uint8_t tx_line_cnt; //reg 4x21 b[7:3]
			0x00, //uint8_t tx_hst_os; //reg 4x21 b[2:0]
			0x07, //uint8_t tx_hst; //reg 4x22 b[6:0]
			0x055376, //uint32_t tx_freq_first; //reg 4x23,4x24,4x25
			0x055376, //uint32_t tx_freq; //reg 4x26,4x27,4x28
			0x01A0, //uint16_t tx_hpst; //reg 4x29,4x2A b[12:0]
			0x30, //uint8_t tx_line_len; //reg 4x2B b[5:0]
			0x0C, //uint8_t tx_all_data_len; //reg 4x2C b[7:0]
	},/*}}}*/
	//720p50
	{/*{{{*/
		0, //int mapChn;

			0x00, //uint8_t tx_field_type; //reg 4x20 b[1:0]
			0x02, //uint8_t tx_line_cnt; //reg 4x21 b[7:3]
			0x00, //uint8_t tx_hst_os; //reg 4x21 b[2:0]
			0x07, //uint8_t tx_hst; //reg 4x22 b[6:0]
			0x055376, //uint32_t tx_freq_first; //reg 4x23,4x24,4x25
			0x055376, //uint32_t tx_freq; //reg 4x26,4x27,4x28
			0x01A0, //uint16_t tx_hpst; //reg 4x29,4x2A b[12:0]
			0x30, //uint8_t tx_line_len; //reg 4x2B b[5:0]
			0x0C, //uint8_t tx_all_data_len; //reg 4x2C b[7:0]
	},/*}}}*/
	//720p30
	{/*{{{*/
		0, //int mapChn;

			0x00, //uint8_t tx_field_type; //reg 4x20 b[1:0]
			0x02, //uint8_t tx_line_cnt; //reg 4x21 b[7:3]
			0x00, //uint8_t tx_hst_os; //reg 4x21 b[2:0]
			0x08, //uint8_t tx_hst; //reg 4x22 b[6:0]
			0x035376, //uint32_t tx_freq_first; //reg 4x23,4x24,4x25
			0x035376, //uint32_t tx_freq; //reg 4x26,4x27,4x28
			0x0564, //uint16_t tx_hpst; //reg 4x29,4x2A b[12:0]
			0x30, //uint8_t tx_line_len; //reg 4x2B b[5:0]
			0x0C, //uint8_t tx_all_data_len; //reg 4x2C b[7:0]
	},/*}}}*/
	//720p25
	{/*{{{*/
		0, //int mapChn;

			0x00, //uint8_t tx_field_type; //reg 4x20 b[1:0]
			0x02, //uint8_t tx_line_cnt; //reg 4x21 b[7:3]
			0x00, //uint8_t tx_hst_os; //reg 4x21 b[2:0]
			0x08, //uint8_t tx_hst; //reg 4x22 b[6:0]
			0x035376, //uint32_t tx_freq_first; //reg 4x23,4x24,4x25
			0x035376, //uint32_t tx_freq; //reg 4x26,4x27,4x28
			0x0564, //uint16_t tx_hpst; //reg 4x29,4x2A b[12:0]
			0x30, //uint8_t tx_line_len; //reg 4x2B b[5:0]
			0x0C, //uint8_t tx_all_data_len; //reg 4x2C b[7:0]
	},/*}}}*/
	//1080p30
	{/*{{{*/
		0, //int mapChn;

			0x00, //uint8_t tx_field_type; //reg 4x20 b[1:0]
			0x04, //uint8_t tx_line_cnt; //reg 4x21 b[7:3]
			0x00, //uint8_t tx_hst_os; //reg 4x21 b[2:0]
			0x07, //uint8_t tx_hst; //reg 4x22 b[6:0]
			0x035376, //uint32_t tx_freq_first; //reg 4x23,4x24,4x25
			0x035376, //uint32_t tx_freq; //reg 4x26,4x27,4x28
			0x0564, //uint16_t tx_hpst; //reg 4x29,4x2A b[12:0]
			0x18, //uint8_t tx_line_len; //reg 4x2B b[5:0]
			0x48, //uint8_t tx_all_data_len; //reg 4x2C b[7:0]
	},/*}}}*/
	//1080p25
	{/*{{{*/
		0, //int mapChn;

			0x00, //uint8_t tx_field_type; //reg 4x20 b[1:0]
			0x04, //uint8_t tx_line_cnt; //reg 4x21 b[7:3]
			0x00, //uint8_t tx_hst_os; //reg 4x21 b[2:0]
			0x08, //uint8_t tx_hst; //reg 4x22 b[6:0]
			0x035376, //uint32_t tx_freq_first; //reg 4x23,4x24,4x25
			0x035376, //uint32_t tx_freq; //reg 4x26,4x27,4x28
			0x0564, //uint16_t tx_hpst; //reg 4x29,4x2A b[12:0]
			0x18, //uint8_t tx_line_len; //reg 4x2B b[5:0]
			0x48, //uint8_t tx_all_data_len; //reg 4x2C b[7:0]
	},/*}}}*/

};
#endif // DONT_SUPPORT_PTZ_FUNC

const _stPTZRxParam pr1000_ptz_rxparam_hda_def[6] =
{
	//720p60
	{/*{{{*/
		0, //int mapChn;

			0x00, //uint8_t rx_field_type; //reg 4x00 b[1:0]
			0x02, //uint8_t rx_line_cnt; //reg 4x01 b[7:3]
			0x00, //uint8_t rx_hst_os; //reg 4x01 b[2:0]
			0x07, //uint8_t rx_hst; //reg 4x02 b[6:0]
			0x055376, //uint32_t rx_freq_first; //reg 4x03,4x04,4x05
			0x055376, //uint32_t rx_freq; //reg 4x06,4x07,4x08
			0x04, //uint8_t rx_lpf_len; //reg 4x09 b[5:0]
			0x10, //uint8_t rx_h_pix_offset; //reg 4x0A b[7:0]
			0x30, //uint8_t rx_line_len; //reg 4x0B b[5:0]
			0x0C, //uint8_t rx_valid_cnt; //reg 4x0C b[7:0]
	},/*}}}*/
	//720p50
	{/*{{{*/
		0, //int mapChn;

			0x00, //uint8_t rx_field_type; //reg 4x00 b[1:0]
			0x02, //uint8_t rx_line_cnt; //reg 4x01 b[7:3]
			0x00, //uint8_t rx_hst_os; //reg 4x01 b[2:0]
			0x07, //uint8_t rx_hst; //reg 4x02 b[6:0]
			0x055376, //uint32_t rx_freq_first; //reg 4x03,4x04,4x05
			0x055376, //uint32_t rx_freq; //reg 4x06,4x07,4x08
			0x04, //uint8_t rx_lpf_len; //reg 4x09 b[5:0]
			0x10, //uint8_t rx_h_pix_offset; //reg 4x0A b[7:0]
			0x30, //uint8_t rx_line_len; //reg 4x0B b[5:0]
			0x0C, //uint8_t rx_valid_cnt; //reg 4x0C b[7:0]
	},/*}}}*/
	//720p30
	{/*{{{*/
		0, //int mapChn;

			0x00, //uint8_t rx_field_type; //reg 4x00 b[1:0]
			0x02, //uint8_t rx_line_cnt; //reg 4x01 b[7:3]
			0x00, //uint8_t rx_hst_os; //reg 4x01 b[2:0]
			0x08, //uint8_t rx_hst; //reg 4x02 b[6:0]
			0x035376, //uint32_t rx_freq_first; //reg 4x03,4x04,4x05
			0x035376, //uint32_t rx_freq; //reg 4x06,4x07,4x08
			0x04, //uint8_t rx_lpf_len; //reg 4x09 b[5:0]
			0x10, //uint8_t rx_h_pix_offset; //reg 4x0A b[7:0]
			0x30, //uint8_t rx_line_len; //reg 4x0B b[5:0]
			0x0C, //uint8_t rx_valid_cnt; //reg 4x0C b[7:0]
	},/*}}}*/
	//720p25
	{/*{{{*/
		0, //int mapChn;

			0x00, //uint8_t rx_field_type; //reg 4x00 b[1:0]
			0x02, //uint8_t rx_line_cnt; //reg 4x01 b[7:3]
			0x00, //uint8_t rx_hst_os; //reg 4x01 b[2:0]
			0x08, //uint8_t rx_hst; //reg 4x02 b[6:0]
			0x035376, //uint32_t rx_freq_first; //reg 4x03,4x04,4x05
			0x035376, //uint32_t rx_freq; //reg 4x06,4x07,4x08
			0x04, //uint8_t rx_lpf_len; //reg 4x09 b[5:0]
			0x10, //uint8_t rx_h_pix_offset; //reg 4x0A b[7:0]
			0x30, //uint8_t rx_line_len; //reg 4x0B b[5:0]
			0x0C, //uint8_t rx_valid_cnt; //reg 4x0C b[7:0]
	},/*}}}*/
	//1080p30
	{/*{{{*/
		0, //int mapChn;

			0x00, //uint8_t rx_field_type; //reg 4x00 b[1:0]
			0x04, //uint8_t rx_line_cnt; //reg 4x01 b[7:3]
			0x00, //uint8_t rx_hst_os; //reg 4x01 b[2:0]
			0x07, //uint8_t rx_hst; //reg 4x02 b[6:0]
			0x035376, //uint32_t rx_freq_first; //reg 4x03,4x04,4x05
			0x035376, //uint32_t rx_freq; //reg 4x06,4x07,4x08
			0x04, //uint8_t rx_lpf_len; //reg 4x09 b[5:0]
			0x10, //uint8_t rx_h_pix_offset; //reg 4x0A b[7:0]
			0x18, //uint8_t rx_line_len; //reg 4x0B b[5:0]
			0x0C, //uint8_t rx_valid_cnt; //reg 4x0C b[7:0]
	},/*}}}*/
	//1080p25
	{/*{{{*/
		0, //int mapChn;

			0x00, //uint8_t rx_field_type; //reg 4x00 b[1:0]
			0x04, //uint8_t rx_line_cnt; //reg 4x01 b[7:3]
			0x00, //uint8_t rx_hst_os; //reg 4x01 b[2:0]
			0x08, //uint8_t rx_hst; //reg 4x02 b[6:0]
			0x035376, //uint32_t rx_freq_first; //reg 4x03,4x04,4x05
			0x035376, //uint32_t rx_freq; //reg 4x06,4x07,4x08
			0x04, //uint8_t rx_lpf_len; //reg 4x09 b[5:0]
			0x10, //uint8_t rx_h_pix_offset; //reg 4x0A b[7:0]
			0x18, //uint8_t rx_line_len; //reg 4x0B b[5:0]
			0x0C, //uint8_t rx_valid_cnt; //reg 4x0C b[7:0]
	},/*}}}*/

};
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*** HDA PATTERN ***/
#ifndef DONT_SUPPORT_PTZ_FUNC
/* PTZ HDA Tx pattern format */
const unsigned char pr1000_ptz_table_hda_tx_pat_format_1080p[((3*4)*6)] = 
{/*{{{*/
	/* frame 1 */
	/* 1 */ 0xB6, 0xDB, 0x6D, //"101 101 101 101 101 101 101 101"
	/* 2 */ 0xB6, 0xDB, 0x6D, //"101 101 101 101 101 101 101 101"
	/* 3 */ 0xB6, 0xDB, 0x6D, //"101 101 101 101 101 101 101 101"
	/* 4 */ 0xB6, 0xDB, 0x6D, //"101 101 101 101 101 101 101 101"
	/* frame 2 */
	/* 1 */ 0xB6, 0xDB, 0x6D, //"101 101 101 101 101 101 101 101"
	/* 2 */ 0xB6, 0xDB, 0x6D, //"101 101 101 101 101 101 101 101"
	/* 3 */ 0xB6, 0xDB, 0x6D, //"101 101 101 101 101 101 101 101"
	/* 4 */ 0xB6, 0xDB, 0x6D, //"101 101 101 101 101 101 101 101"
	/* frame 3 */
	/* 1 */ 0xB6, 0xDB, 0x6D, //"101 101 101 101 101 101 101 101"
	/* 2 */ 0xB6, 0xDB, 0x6D, //"101 101 101 101 101 101 101 101"
	/* 3 */ 0xB6, 0xDB, 0x6D, //"101 101 101 101 101 101 101 101"
	/* 4 */ 0xB6, 0xDB, 0x6D, //"101 101 101 101 101 101 101 101"
	/* frame 4 */
	/* 1 */ 0xB6, 0xDB, 0x6D, //"101 101 101 101 101 101 101 101"
	/* 2 */ 0xB6, 0xDB, 0x6D, //"101 101 101 101 101 101 101 101"
	/* 3 */ 0xB6, 0xDB, 0x6D, //"101 101 101 101 101 101 101 101"
	/* 4 */ 0xB6, 0xDB, 0x6D, //"101 101 101 101 101 101 101 101"
	/* frame 5 */
	/* 1 */ 0xB6, 0xDB, 0x6D, //"101 101 101 101 101 101 101 101"
	/* 2 */ 0xB6, 0xDB, 0x6D, //"101 101 101 101 101 101 101 101"
	/* 3 */ 0xB6, 0xDB, 0x6D, //"101 101 101 101 101 101 101 101"
	/* 4 */ 0xB6, 0xDB, 0x6D, //"101 101 101 101 101 101 101 101"
	/* frame 6 */
	/* 1 */ 0xB6, 0xDB, 0x6D, //"101 101 101 101 101 101 101 101"
	/* 2 */ 0xB6, 0xDB, 0x6D, //"101 101 101 101 101 101 101 101"
	/* 3 */ 0xB6, 0xDB, 0x6D, //"101 101 101 101 101 101 101 101"
	/* 4 */ 0xB6, 0xDB, 0x6D, //"101 101 101 101 101 101 101 101"
};/*}}}*/
const unsigned char pr1000_ptz_table_hda_tx_pat_format_720p[((3*2)*2)] = 
{/*{{{*/
	/* frame 1 */
	/* 1 */ 0xB6, 0xDB, 0x6D, 0xB6, 0xDB, 0x6D, //"101 101 101 101 101 101 101 101 101 101 101 101 101 101 101 101"
	/* 2 */ 0xB6, 0xDB, 0x6D, 0xB6, 0xDB, 0x6D, //"101 101 101 101 101 101 101 101 101 101 101 101 101 101 101 101"
};/*}}}*/
/* PTZ HDA Tx pattern data */
const unsigned char pr1000_ptz_table_hda_tx_pat_data_1080p[((3*4)*6)] = 
{/*{{{*/
	/* frame 1 */
	/* 1 */ 0x92, 0x49, 0x24, //"1x0 1x0 1x0 1x0 1x0 1x0 1x0 1x0"
	/* 2 */ 0x92, 0x49, 0x24, //"1x0 1x0 1x0 1x0 1x0 1x0 1x0 1x0"
	/* 3 */ 0x92, 0x49, 0x24, //"1x0 1x0 1x0 1x0 1x0 1x0 1x0 1x0"
	/* 4 */ 0x92, 0x49, 0x24, //"1x0 1x0 1x0 1x0 1x0 1x0 1x0 1x0"
	/* frame 2 */
	/* 1 */ 0x92, 0x49, 0x24, //"1x0 1x0 1x0 1x0 1x0 1x0 1x0 1x0"
	/* 2 */ 0x92, 0x49, 0x24, //"1x0 1x0 1x0 1x0 1x0 1x0 1x0 1x0"
	/* 3 */ 0x92, 0x49, 0x24, //"1x0 1x0 1x0 1x0 1x0 1x0 1x0 1x0"
	/* 4 */ 0x92, 0x49, 0x24, //"1x0 1x0 1x0 1x0 1x0 1x0 1x0 1x0"
	/* frame 3 */
	/* 1 */ 0x92, 0x49, 0x24, //"1x0 1x0 1x0 1x0 1x0 1x0 1x0 1x0"
	/* 2 */ 0x92, 0x49, 0x24, //"1x0 1x0 1x0 1x0 1x0 1x0 1x0 1x0"
	/* 3 */ 0x92, 0x49, 0x24, //"1x0 1x0 1x0 1x0 1x0 1x0 1x0 1x0"
	/* 4 */ 0x92, 0x49, 0x24, //"1x0 1x0 1x0 1x0 1x0 1x0 1x0 1x0"
	/* frame 4 */
	/* 1 */ 0x92, 0x49, 0x24, //"1x0 1x0 1x0 1x0 1x0 1x0 1x0 1x0"
	/* 2 */ 0x92, 0x49, 0x24, //"1x0 1x0 1x0 1x0 1x0 1x0 1x0 1x0"
	/* 3 */ 0x92, 0x49, 0x24, //"1x0 1x0 1x0 1x0 1x0 1x0 1x0 1x0"
	/* 4 */ 0x92, 0x49, 0x24, //"1x0 1x0 1x0 1x0 1x0 1x0 1x0 1x0"
	/* frame 5 */
	/* 1 */ 0x92, 0x49, 0x24, //"1x0 1x0 1x0 1x0 1x0 1x0 1x0 1x0"
	/* 2 */ 0x92, 0x49, 0x24, //"1x0 1x0 1x0 1x0 1x0 1x0 1x0 1x0"
	/* 3 */ 0x92, 0x49, 0x24, //"1x0 1x0 1x0 1x0 1x0 1x0 1x0 1x0"
	/* 4 */ 0x92, 0x49, 0x24, //"1x0 1x0 1x0 1x0 1x0 1x0 1x0 1x0"
	/* frame 6 */
	/* 1 */ 0x92, 0x49, 0x24, //"1x0 1x0 1x0 1x0 1x0 1x0 1x0 1x0"
	/* 2 */ 0x92, 0x49, 0x24, //"1x0 1x0 1x0 1x0 1x0 1x0 1x0 1x0"
	/* 3 */ 0x92, 0x49, 0x24, //"1x0 1x0 1x0 1x0 1x0 1x0 1x0 1x0"
	/* 4 */ 0x92, 0x49, 0x24, //"1x0 1x0 1x0 1x0 1x0 1x0 1x0 1x0"
};/*}}}*/
const unsigned char pr1000_ptz_table_hda_tx_pat_data_720p[((3*2)*2)] = 
{/*{{{*/
	/* frame 1 */
	/* 1 */ 0x92, 0x49, 0x24, 0x92, 0x49, 0x24, //"1x0 1x0 1x0 1x0 1x0 1x0 1x0 1x0 1x0 1x0 1x0 1x0 1x0 1x0 1x0 1x0"
	/* 2 */ 0x92, 0x49, 0x24, 0x92, 0x49, 0x24, //"1x0 1x0 1x0 1x0 1x0 1x0 1x0 1x0 1x0 1x0 1x0 1x0 1x0 1x0 1x0 1x0"
};/*}}}*/
#endif // DONT_SUPPORT_PTZ_FUNC

/* PTZ HDA Rx pattern format */
const unsigned char pr1000_ptz_table_hda_rx_pat_format_1080p[((3*4)*6)] = 
{/*{{{*/
	/* frame 1 */
	/* 1 */ 0xB6, 0xDB, 0x6D, //"101 101 101 101 101 101 101 101"
	/* 2 */ 0xB6, 0xDB, 0x6D, //"101 101 101 101 101 101 101 101"
	/* 3 */ 0xB6, 0xDB, 0x6D, //"101 101 101 101 101 101 101 101"
	/* 4 */ 0xB6, 0xDB, 0x6D, //"101 101 101 101 101 101 101 101"
	/* frame 2 */
	/* 1 */ 0xB6, 0xDB, 0x6D, //"101 101 101 101 101 101 101 101"
	/* 2 */ 0xB6, 0xDB, 0x6D, //"101 101 101 101 101 101 101 101"
	/* 3 */ 0xB6, 0xDB, 0x6D, //"101 101 101 101 101 101 101 101"
	/* 4 */ 0xB6, 0xDB, 0x6D, //"101 101 101 101 101 101 101 101"
	/* frame 3 */
	/* 1 */ 0xB6, 0xDB, 0x6D, //"101 101 101 101 101 101 101 101"
	/* 2 */ 0xB6, 0xDB, 0x6D, //"101 101 101 101 101 101 101 101"
	/* 3 */ 0xB6, 0xDB, 0x6D, //"101 101 101 101 101 101 101 101"
	/* 4 */ 0xB6, 0xDB, 0x6D, //"101 101 101 101 101 101 101 101"
	/* frame 4 */
	/* 1 */ 0xB6, 0xDB, 0x6D, //"101 101 101 101 101 101 101 101"
	/* 2 */ 0xB6, 0xDB, 0x6D, //"101 101 101 101 101 101 101 101"
	/* 3 */ 0xB6, 0xDB, 0x6D, //"101 101 101 101 101 101 101 101"
	/* 4 */ 0xB6, 0xDB, 0x6D, //"101 101 101 101 101 101 101 101"
	/* frame 5 */
	/* 1 */ 0xB6, 0xDB, 0x6D, //"101 101 101 101 101 101 101 101"
	/* 2 */ 0xB6, 0xDB, 0x6D, //"101 101 101 101 101 101 101 101"
	/* 3 */ 0xB6, 0xDB, 0x6D, //"101 101 101 101 101 101 101 101"
	/* 4 */ 0xB6, 0xDB, 0x6D, //"101 101 101 101 101 101 101 101"
	/* frame 6 */
	/* 1 */ 0xB6, 0xDB, 0x6D, //"101 101 101 101 101 101 101 101"
	/* 2 */ 0xB6, 0xDB, 0x6D, //"101 101 101 101 101 101 101 101"
	/* 3 */ 0xB6, 0xDB, 0x6D, //"101 101 101 101 101 101 101 101"
	/* 4 */ 0xB6, 0xDB, 0x6D, //"101 101 101 101 101 101 101 101"
};/*}}}*/
const unsigned char pr1000_ptz_table_hda_rx_pat_format_720p[((3*2)*2)] = 
{/*{{{*/
	/* frame 1 */
	/* 1 */ 0xB6, 0xDB, 0x6D, 0xB6, 0xDB, 0x6D, //"101 101 101 101 101 101 101 101 101 101 101 101 101 101 101 101"
	/* 2 */ 0xB6, 0xDB, 0x6D, 0xB6, 0xDB, 0x6D, //"101 101 101 101 101 101 101 101 101 101 101 101 101 101 101 101"
};/*}}}*/
/* PTZ HDA Rx pattern start format */
const unsigned char pr1000_ptz_table_hda_rx_pat_start_format_1080p[(3*1)] = 
{/*{{{*/
	0xB6, 0xDB, 0x6D, //"101 101 101 101 101 101 101 101"
};/*}}}*/
const unsigned char pr1000_ptz_table_hda_rx_pat_start_format_720p[(3*2)] = 
{/*{{{*/
	0xB6, 0xDB, 0x6D, 0xB6, 0xDB, 0x6D, //"101 101 101 101 101 101 101 101 101 101 101 101 101 101 101 101"
};/*}}}*/
/* PTZ HDA Rx pattern start data */
const unsigned char pr1000_ptz_table_hda_rx_pat_start_data_1080p[(3*1)] = 
{/*{{{*/
	0x92, 0x49, 0x24, //"1x0 1x0 1x0 1x0 1x0 1x0 1x0 1x0"
};/*}}}*/
const unsigned char pr1000_ptz_table_hda_rx_pat_start_data_720p[(3*2)] = 
{/*{{{*/
	0x92, 0x49, 0x24, 0x92, 0x49, 0x24, //"1x0 1x0 1x0 1x0 1x0 1x0 1x0 1x0 1x0 1x0 1x0 1x0 1x0 1x0 1x0 1x0"
};/*}}}*/

//////////////////////////////////////////////////////////////////////////////////////////

