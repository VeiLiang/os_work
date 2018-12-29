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
#include "pr1000_ptz_table_hdt.h"

#else //#ifdef __LINUX_SYSTEM__

#include <stdio.h>
#include "pr1000_drvcommon.h"
#include "pr1000_ptz_table.h"
#include "pr1000_ptz_table_hdt.h"
#include "pr1000_user_config.h"

#endif // __LINUX_SYSTEM__


/////////////////////////////////////////////////////////////////////////////////////////////

#ifndef DONT_SUPPORT_PTZ_FUNC
/*** HDT COMMAND ***/
/* Isn't HDT osd command */
const uint8_t pr1000_ptz_table_hdt_CMD_VALID[(4*2)] = 		{0xFF,0xFF,0xFF,0x00,/**/0x00,0xFF,0xFF,0x00,};// define rx compare byte. 1's compare bit
const uint8_t pr1000_ptz_table_hdt_OSD_TOP[(4*2)] = 		{0xFF,0xFF,0xFF,0xFF,/**/0xFF,0xFF,0xFF,0xFF,};
const uint8_t pr1000_ptz_table_hdt_OSD_RIGHT[(4*2)] =		{0xFF,0xFF,0xFF,0xFF,/**/0xFF,0xFF,0xFF,0xFF,};
const uint8_t pr1000_ptz_table_hdt_OSD_BOTTOM[(4*2)]=		{0xFF,0xFF,0xFF,0xFF,/**/0xFF,0xFF,0xFF,0xFF,};
const uint8_t pr1000_ptz_table_hdt_OSD_LEFT[(4*2)] = 		{0xFF,0xFF,0xFF,0xFF,/**/0xFF,0xFF,0xFF,0xFF,};
const uint8_t pr1000_ptz_table_hdt_IRIS_PLUS[(4*2)] =		{0xB5,0x00,0x0F,0x00,/**/0x00,0x00,0x00,0xC4,};
const uint8_t pr1000_ptz_table_hdt_IRIS_MINUS[(4*2)]=		{0xB5,0x00,0x0E,0x00,/**/0x00,0x00,0x00,0xC3,};
#ifndef DONT_SUPPORT_PTZ_ETC_CMD
const uint8_t pr1000_ptz_table_hdt_FOCUS_PLUS[(4*2)] =		{0xB5,0x00,0x10,0x00,/**/0x00,0x00,0x00,0xC5,};
const uint8_t pr1000_ptz_table_hdt_FOCUS_MINUS[(4*2)]=		{0xB5,0x00,0x11,0x00,/**/0x00,0x00,0x00,0xC6,};
const uint8_t pr1000_ptz_table_hdt_ZOOM_PLUS[(4*2)] =		{0xB5,0x00,0x13,0x04,/**/0x00,0x00,0x00,0xCC,};
const uint8_t pr1000_ptz_table_hdt_ZOOM_MINUS[(4*2)]=		{0xB5,0x00,0x12,0x04,/**/0x00,0x00,0x00,0xCB,};
#endif // DONT_SUPPORT_PTZ_ETC_CMD
//const uint8_t pr1000_ptz_table_hdt_CENTER[(4*2)]=		{0xB5,0x00,0x17,0x63,/**/0x00,0x00,0x00,0x2F,};

const uint8_t pr1000_ptz_table_hdt_CENTER[(4*2)]=			{0xB5,0x00,0x17,0x5F,/**/0x00,0x00,0x00,0x2B,}; 
const uint8_t pr1000_ptz_table_hdt_DIR_TOP[(4*2)] = 		{0xB5,0x00,0x06,0x24,/**/0x00,0x00,0x00,0xDF,};
const uint8_t pr1000_ptz_table_hdt_DIR_RIGHT[(4*2)] = 		{0xB5,0x00,0x08,0x00,/**/0x24,0x00,0x00,0xE1,}; 
const uint8_t pr1000_ptz_table_hdt_DIR_BOTTOM[(4*2)]= 		{0xB5,0x00,0x07,0x24,/**/0x00,0x00,0x00,0xE0,};
const uint8_t pr1000_ptz_table_hdt_DIR_LEFT[(4*2)] = 		{0xB5,0x00,0x09,0x00,/**/0x24,0x00,0x00,0xE2,}; 

const uint8_t pr1000_ptz_table_hdt_DIR_RIGHT_TOP[(4*2)] = 	{0xB5,0x00,0x0C,0x24,/**/0x24,0x00,0x00,0x0B,};
const uint8_t pr1000_ptz_table_hdt_DIR_RIGHT_BOTTOM[(4*2)]= 	{0xB5,0x00,0x0D,0x24,/**/0x24,0x00,0x00,0x0A,};
const uint8_t pr1000_ptz_table_hdt_DIR_LEFT_TOP[(4*2)] = 	{0xB5,0x00,0x0A,0x24,/**/0x24,0x00,0x00,0x07,};
const uint8_t pr1000_ptz_table_hdt_DIR_LEFT_BOTTOM[(4*2)] = 	{0xB5,0x00,0x0B,0x24,/**/0x24,0x00,0x00,0x08,};
const uint8_t pr1000_ptz_table_hdt_RESERVED0[(4*2)]       = 	{0xB5,0x00,0x14,0x00,/**/0x00,0x00,0x00,0xC9,}; //KEY STOP

/*** HDT_NEW COMMAND ***/
/* Isn't HDT osd command */
const uint8_t pr1000_ptz_table_hdt_new_CMD_VALID[(4*2)] = 	{0xFF,0xFF,0xFF,0x00,/**/0x00,0xFF,0xFF,0x00,};// define rx compare byte. 1's compare bit
const uint8_t pr1000_ptz_table_hdt_new_OSD_TOP[(4*2)] = 	{0xFF,0xFF,0xFF,0xFF,/**/0xFF,0xFF,0xFF,0xFF,};
const uint8_t pr1000_ptz_table_hdt_new_OSD_RIGHT[(4*2)] =	{0xFF,0xFF,0xFF,0xFF,/**/0xFF,0xFF,0xFF,0xFF,};
const uint8_t pr1000_ptz_table_hdt_new_OSD_BOTTOM[(4*2)]=	{0xFF,0xFF,0xFF,0xFF,/**/0xFF,0xFF,0xFF,0xFF,};
const uint8_t pr1000_ptz_table_hdt_new_OSD_LEFT[(4*2)] = 	{0xFF,0xFF,0xFF,0xFF,/**/0xFF,0xFF,0xFF,0xFF,};
const uint8_t pr1000_ptz_table_hdt_new_IRIS_PLUS[(4*2)] =	{0xB5,0x00,0x0F,0x00,/**/0x00,0x00,0x00,0xC4,};
const uint8_t pr1000_ptz_table_hdt_new_IRIS_MINUS[(4*2)]=	{0xB5,0x00,0x0E,0x00,/**/0x00,0x00,0x00,0xC3,};
#ifndef DONT_SUPPORT_PTZ_ETC_CMD
const uint8_t pr1000_ptz_table_hdt_new_FOCUS_PLUS[(4*2)] =	{0xB5,0x00,0x10,0x00,/**/0x00,0x00,0x00,0xC5,};
const uint8_t pr1000_ptz_table_hdt_new_FOCUS_MINUS[(4*2)]=	{0xB5,0x00,0x11,0x00,/**/0x00,0x00,0x00,0xC6,};
const uint8_t pr1000_ptz_table_hdt_new_ZOOM_PLUS[(4*2)] =	{0xB5,0x00,0x13,0x04,/**/0x00,0x00,0x00,0xCC,};
const uint8_t pr1000_ptz_table_hdt_new_ZOOM_MINUS[(4*2)]=	{0xB5,0x00,0x12,0x04,/**/0x00,0x00,0x00,0xCB,};
#endif // DONT_SUPPORT_PTZ_ETC_CMD
//const uint8_t pr1000_ptz_table_hdt_new_CENTER[(4*2)]=		{0xB5,0x00,0x17,0x63,/**/0x00,0x00,0x00,0x2F,};
const uint8_t pr1000_ptz_table_hdt_new_CENTER[(4*2)]=		{0xB5,0x00,0x17,0x5F,/**/0x00,0x00,0x00,0x2B,}; 
const uint8_t pr1000_ptz_table_hdt_new_DIR_TOP[(4*2)] = 	{0xB5,0x00,0x06,0x24,/**/0x00,0x00,0x00,0xDF,};
const uint8_t pr1000_ptz_table_hdt_new_DIR_RIGHT[(4*2)] = 	{0xB5,0x00,0x08,0x00,/**/0x24,0x00,0x00,0xE1,}; 
const uint8_t pr1000_ptz_table_hdt_new_DIR_BOTTOM[(4*2)]= 	{0xB5,0x00,0x07,0x24,/**/0x00,0x00,0x00,0xE0,};
const uint8_t pr1000_ptz_table_hdt_new_DIR_LEFT[(4*2)] = 	{0xB5,0x00,0x09,0x00,/**/0x24,0x00,0x00,0xE2,}; 
const uint8_t pr1000_ptz_table_hdt_new_DIR_RIGHT_TOP[(4*2)] = 	{0xB5,0x00,0x0C,0x24,/**/0x24,0x00,0x00,0x0B,};
const uint8_t pr1000_ptz_table_hdt_new_DIR_RIGHT_BOTTOM[(4*2)]= {0xB5,0x00,0x0D,0x24,/**/0x24,0x00,0x00,0x0A,};
const uint8_t pr1000_ptz_table_hdt_new_DIR_LEFT_TOP[(4*2)] = 	{0xB5,0x00,0x0A,0x24,/**/0x24,0x00,0x00,0x07,};
const uint8_t pr1000_ptz_table_hdt_new_DIR_LEFT_BOTTOM[(4*2)] = {0xB5,0x00,0x0B,0x24,/**/0x24,0x00,0x00,0x08,};
const uint8_t pr1000_ptz_table_hdt_new_RESERVED0[(4*2)]       = {0xB5,0x00,0x14,0x00,/**/0x00,0x00,0x00,0xC9,}; //KEY STOP

/* HDT */
#ifndef DONT_SUPPORT_PTZ_ETC_CMD
const _pr1000PTZCmd pr1000_ptz_table_hdtcmd[max_pr1000_ptz_table_command] = 
{/*{{{*/
	{ pr1000_ptz_table_hdt_DIR_TOP,    sizeof(pr1000_ptz_table_hdt_DIR_TOP)/sizeof(pr1000_ptz_table_hdt_DIR_TOP[0]) },//assume OSD_TOP
	{ pr1000_ptz_table_hdt_DIR_RIGHT,  sizeof(pr1000_ptz_table_hdt_DIR_RIGHT)/sizeof(pr1000_ptz_table_hdt_DIR_RIGHT[0]) },//assume OSD_RIGHT
	{ pr1000_ptz_table_hdt_DIR_BOTTOM, sizeof(pr1000_ptz_table_hdt_DIR_BOTTOM)/sizeof(pr1000_ptz_table_hdt_DIR_BOTTOM[0]) },//assume OSD_BOTTOM
	{ pr1000_ptz_table_hdt_DIR_LEFT,   sizeof(pr1000_ptz_table_hdt_DIR_LEFT)/sizeof(pr1000_ptz_table_hdt_DIR_LEFT[0]) },//assume OSD_LEFT
	{ pr1000_ptz_table_hdt_CENTER,     sizeof(pr1000_ptz_table_hdt_CENTER)/sizeof(pr1000_ptz_table_hdt_CENTER[0]) },
//	some camera iris_plus is menu on
//	{ pr1000_ptz_table_hdt_IRIS_PLUS,  sizeof(pr1000_ptz_table_hdt_IRIS_PLUS)/sizeof(pr1000_ptz_table_hdt_IRIS_PLUS[0]) },
	{ pr1000_ptz_table_hdt_IRIS_PLUS,  sizeof(pr1000_ptz_table_hdt_IRIS_PLUS)/sizeof(pr1000_ptz_table_hdt_IRIS_PLUS[0]) },
	{ pr1000_ptz_table_hdt_IRIS_MINUS, sizeof(pr1000_ptz_table_hdt_IRIS_MINUS)/sizeof(pr1000_ptz_table_hdt_IRIS_MINUS[0]) },
	{ pr1000_ptz_table_hdt_FOCUS_PLUS, sizeof(pr1000_ptz_table_hdt_FOCUS_PLUS)/sizeof(pr1000_ptz_table_hdt_FOCUS_PLUS[0]) },
	{ pr1000_ptz_table_hdt_FOCUS_MINUS,sizeof(pr1000_ptz_table_hdt_FOCUS_MINUS)/sizeof(pr1000_ptz_table_hdt_FOCUS_MINUS[0]) },
	{ pr1000_ptz_table_hdt_ZOOM_PLUS,  sizeof(pr1000_ptz_table_hdt_ZOOM_PLUS)/sizeof(pr1000_ptz_table_hdt_ZOOM_PLUS[0]) },
	{ pr1000_ptz_table_hdt_ZOOM_MINUS, sizeof(pr1000_ptz_table_hdt_ZOOM_MINUS)/sizeof(pr1000_ptz_table_hdt_ZOOM_MINUS[0]) },
	{ pr1000_ptz_table_hdt_DIR_TOP,    sizeof(pr1000_ptz_table_hdt_DIR_TOP)/sizeof(pr1000_ptz_table_hdt_DIR_TOP[0]) },
	{ pr1000_ptz_table_hdt_DIR_RIGHT,  sizeof(pr1000_ptz_table_hdt_DIR_RIGHT)/sizeof(pr1000_ptz_table_hdt_DIR_RIGHT[0]) },
	{ pr1000_ptz_table_hdt_DIR_BOTTOM, sizeof(pr1000_ptz_table_hdt_DIR_BOTTOM)/sizeof(pr1000_ptz_table_hdt_DIR_BOTTOM[0]) },
	{ pr1000_ptz_table_hdt_DIR_LEFT,   sizeof(pr1000_ptz_table_hdt_DIR_LEFT)/sizeof(pr1000_ptz_table_hdt_DIR_LEFT[0]) },
	{ pr1000_ptz_table_hdt_DIR_RIGHT_TOP,   sizeof(pr1000_ptz_table_hdt_DIR_RIGHT_TOP)/sizeof(pr1000_ptz_table_hdt_DIR_RIGHT_TOP[0]) },
	{ pr1000_ptz_table_hdt_DIR_RIGHT_BOTTOM,   sizeof(pr1000_ptz_table_hdt_DIR_RIGHT_BOTTOM)/sizeof(pr1000_ptz_table_hdt_DIR_RIGHT_BOTTOM[0]) },
	{ pr1000_ptz_table_hdt_DIR_LEFT_TOP,   sizeof(pr1000_ptz_table_hdt_DIR_LEFT_TOP)/sizeof(pr1000_ptz_table_hdt_DIR_LEFT_TOP[0]) },
	{ pr1000_ptz_table_hdt_DIR_LEFT_BOTTOM,   sizeof(pr1000_ptz_table_hdt_DIR_LEFT_BOTTOM)/sizeof(pr1000_ptz_table_hdt_DIR_LEFT_BOTTOM[0]) },
	{ NULL,    0 }, //LENS_CLOSE
	{ NULL,    0 }, //DIR_CLOSE
	{ pr1000_ptz_table_hdt_RESERVED0,   sizeof(pr1000_ptz_table_hdt_RESERVED0)/sizeof(pr1000_ptz_table_hdt_RESERVED0[0]) },
};/*}}}*/

/* HDT_NEW */
const _pr1000PTZCmd pr1000_ptz_table_hdt_newcmd[max_pr1000_ptz_table_command] = 
{/*{{{*/
	{ pr1000_ptz_table_hdt_new_DIR_TOP,    sizeof(pr1000_ptz_table_hdt_new_DIR_TOP)/sizeof(pr1000_ptz_table_hdt_new_DIR_TOP[0]) },//assume OSD_TOP
	{ pr1000_ptz_table_hdt_new_DIR_RIGHT,  sizeof(pr1000_ptz_table_hdt_new_DIR_RIGHT)/sizeof(pr1000_ptz_table_hdt_new_DIR_RIGHT[0]) },//assume OSD_RIGHT
	{ pr1000_ptz_table_hdt_new_DIR_BOTTOM, sizeof(pr1000_ptz_table_hdt_new_DIR_BOTTOM)/sizeof(pr1000_ptz_table_hdt_new_DIR_BOTTOM[0]) },//assume OSD_BOTTOM
	{ pr1000_ptz_table_hdt_new_DIR_LEFT,   sizeof(pr1000_ptz_table_hdt_new_DIR_LEFT)/sizeof(pr1000_ptz_table_hdt_new_DIR_LEFT[0]) },//assume OSD_LEFT
	{ pr1000_ptz_table_hdt_new_CENTER,     sizeof(pr1000_ptz_table_hdt_new_CENTER)/sizeof(pr1000_ptz_table_hdt_new_CENTER[0]) },
	{ pr1000_ptz_table_hdt_new_IRIS_PLUS,  sizeof(pr1000_ptz_table_hdt_new_IRIS_PLUS)/sizeof(pr1000_ptz_table_hdt_new_IRIS_PLUS[0]) },
	{ pr1000_ptz_table_hdt_new_IRIS_MINUS, sizeof(pr1000_ptz_table_hdt_new_IRIS_MINUS)/sizeof(pr1000_ptz_table_hdt_new_IRIS_MINUS[0]) },
	{ pr1000_ptz_table_hdt_new_FOCUS_PLUS, sizeof(pr1000_ptz_table_hdt_new_FOCUS_PLUS)/sizeof(pr1000_ptz_table_hdt_new_FOCUS_PLUS[0]) },
	{ pr1000_ptz_table_hdt_new_FOCUS_MINUS,sizeof(pr1000_ptz_table_hdt_new_FOCUS_MINUS)/sizeof(pr1000_ptz_table_hdt_new_FOCUS_MINUS[0]) },
	{ pr1000_ptz_table_hdt_new_ZOOM_PLUS,  sizeof(pr1000_ptz_table_hdt_new_ZOOM_PLUS)/sizeof(pr1000_ptz_table_hdt_new_ZOOM_PLUS[0]) },
	{ pr1000_ptz_table_hdt_new_ZOOM_MINUS, sizeof(pr1000_ptz_table_hdt_new_ZOOM_MINUS)/sizeof(pr1000_ptz_table_hdt_new_ZOOM_MINUS[0]) },
	{ pr1000_ptz_table_hdt_new_DIR_TOP,    sizeof(pr1000_ptz_table_hdt_new_DIR_TOP)/sizeof(pr1000_ptz_table_hdt_new_DIR_TOP[0]) },
	{ pr1000_ptz_table_hdt_new_DIR_RIGHT,  sizeof(pr1000_ptz_table_hdt_new_DIR_RIGHT)/sizeof(pr1000_ptz_table_hdt_new_DIR_RIGHT[0]) },
	{ pr1000_ptz_table_hdt_new_DIR_BOTTOM, sizeof(pr1000_ptz_table_hdt_new_DIR_BOTTOM)/sizeof(pr1000_ptz_table_hdt_new_DIR_BOTTOM[0]) },
	{ pr1000_ptz_table_hdt_new_DIR_LEFT,   sizeof(pr1000_ptz_table_hdt_new_DIR_LEFT)/sizeof(pr1000_ptz_table_hdt_new_DIR_LEFT[0]) },
	{ pr1000_ptz_table_hdt_new_DIR_RIGHT_TOP,   sizeof(pr1000_ptz_table_hdt_new_DIR_RIGHT_TOP)/sizeof(pr1000_ptz_table_hdt_new_DIR_RIGHT_TOP[0]) },
	{ pr1000_ptz_table_hdt_new_DIR_RIGHT_BOTTOM,   sizeof(pr1000_ptz_table_hdt_new_DIR_RIGHT_BOTTOM)/sizeof(pr1000_ptz_table_hdt_new_DIR_RIGHT_BOTTOM[0]) },
	{ pr1000_ptz_table_hdt_new_DIR_LEFT_TOP,   sizeof(pr1000_ptz_table_hdt_new_DIR_LEFT_TOP)/sizeof(pr1000_ptz_table_hdt_new_DIR_LEFT_TOP[0]) },
	{ pr1000_ptz_table_hdt_new_DIR_LEFT_BOTTOM,   sizeof(pr1000_ptz_table_hdt_new_DIR_LEFT_BOTTOM)/sizeof(pr1000_ptz_table_hdt_new_DIR_LEFT_BOTTOM[0]) },
	{ NULL,    0 }, //LENS_CLOSE
	{ NULL,    0 }, //DIR_CLOSE
	{ pr1000_ptz_table_hdt_new_RESERVED0,   sizeof(pr1000_ptz_table_hdt_new_RESERVED0)/sizeof(pr1000_ptz_table_hdt_new_RESERVED0[0]) },
};/*}}}*/
#endif // DONT_SUPPORT_PTZ_ETC_CMD
#endif // DONT_SUPPORT_PTZ_FUNC

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*** HDT PTZ Parameter ***/
#ifndef DONT_SUPPORT_PTZ_FUNC
const _stPTZTxParam pr1000_ptz_txparam_hdt_def[6] =
{
	//720p60
	{/*{{{*/
		0, //int mapChn;

			0x00, //uint8_t tx_field_type; //reg 4x20 b[1:0]
			0x02, //uint8_t tx_line_cnt; //reg 4x21 b[7:3]
			0x00, //uint8_t tx_hst_os; //reg 4x21 b[2:0]
			0x03, //uint8_t tx_hst; //reg 4x22 b[6:0]
			0x04F064, //uint32_t tx_freq_first; //reg 4x23,4x24,4x25
			0x04F064, //uint32_t tx_freq; //reg 4x26,4x27,4x28
			0x0070, //uint16_t tx_hpst; //reg 4x29,4x2A b[12:0]
			//0x28, //uint8_t tx_line_len; //reg 4x2B b[5:0]
			0x22, //uint8_t tx_line_len; //reg 4x2B b[5:0]
			0x0A, //uint8_t tx_all_data_len; //reg 4x2C b[7:0]
	},/*}}}*/
	//720p50
	{/*{{{*/
		0, //int mapChn;

			0x00, //uint8_t tx_field_type; //reg 4x20 b[1:0]
			0x02, //uint8_t tx_line_cnt; //reg 4x21 b[7:3]
			0x00, //uint8_t tx_hst_os; //reg 4x21 b[2:0]
			0x03, //uint8_t tx_hst; //reg 4x22 b[6:0]
			0x04F064, //uint32_t tx_freq_first; //reg 4x23,4x24,4x25
			0x04F064, //uint32_t tx_freq; //reg 4x26,4x27,4x28
			0x0070, //uint16_t tx_hpst; //reg 4x29,4x2A b[12:0]
			//0x28, //uint8_t tx_line_len; //reg 4x2B b[5:0]
			0x22, //uint8_t tx_line_len; //reg 4x2B b[5:0]
			0x0A, //uint8_t tx_all_data_len; //reg 4x2C b[7:0]
	},/*}}}*/
	//720p30
	{/*{{{*/
		0, //int mapChn;

			0x00, //uint8_t tx_field_type; //reg 4x20 b[1:0]
			0x02, //uint8_t tx_line_cnt; //reg 4x21 b[7:3]
			0x00, //uint8_t tx_hst_os; //reg 4x21 b[2:0]
			0x04, //uint8_t tx_hst; //reg 4x22 b[6:0]
			0x027832, //uint32_t tx_freq_first; //reg 4x23,4x24,4x25
			0x027832, //uint32_t tx_freq; //reg 4x26,4x27,4x28
			0x0140, //uint16_t tx_hpst; //reg 4x29,4x2A b[12:0]
			//0x28, //uint8_t tx_line_len; //reg 4x2B b[5:0]
			0x22, //uint8_t tx_line_len; //reg 4x2B b[5:0]
			0x0A, //uint8_t tx_all_data_len; //reg 4x2C b[7:0]
	},/*}}}*/
	//720p25
	{/*{{{*/
		0, //int mapChn;

			0x00, //uint8_t tx_field_type; //reg 4x20 b[1:0]
			0x02, //uint8_t tx_line_cnt; //reg 4x21 b[7:3]
			0x00, //uint8_t tx_hst_os; //reg 4x21 b[2:0]
			0x04, //uint8_t tx_hst; //reg 4x22 b[6:0]
			0x027832, //uint32_t tx_freq_first; //reg 4x23,4x24,4x25
			0x027832, //uint32_t tx_freq; //reg 4x26,4x27,4x28
			0x0140, //uint16_t tx_hpst; //reg 4x29,4x2A b[12:0]
			//0x28, //uint8_t tx_line_len; //reg 4x2B b[5:0]
			0x22, //uint8_t tx_line_len; //reg 4x2B b[5:0]
			0x0A, //uint8_t tx_all_data_len; //reg 4x2C b[7:0]
	},/*}}}*/
	//1080p30
	{/*{{{*/
		0, //int mapChn;

			0x00, //uint8_t tx_field_type; //reg 4x20 b[1:0]
			0x02, //uint8_t tx_line_cnt; //reg 4x21 b[7:3]
			0x00, //uint8_t tx_hst_os; //reg 4x21 b[2:0]
			0x03, //uint8_t tx_hst; //reg 4x22 b[6:0]
			0x04F064, //uint32_t tx_freq_first; //reg 4x23,4x24,4x25
			0x04F064, //uint32_t tx_freq; //reg 4x26,4x27,4x28
			0x0070, //uint16_t tx_hpst; //reg 4x29,4x2A b[12:0]
			//0x28, //uint8_t tx_line_len; //reg 4x2B b[5:0]
			0x22, //uint8_t tx_line_len; //reg 4x2B b[5:0]
			0x0A, //uint8_t tx_all_data_len; //reg 4x2C b[7:0]
	},/*}}}*/
	//1080p25
	{/*{{{*/
		0, //int mapChn;

			0x00, //uint8_t tx_field_type; //reg 4x20 b[1:0]
			0x02, //uint8_t tx_line_cnt; //reg 4x21 b[7:3]
			0x00, //uint8_t tx_hst_os; //reg 4x21 b[2:0]
			0x04, //uint8_t tx_hst; //reg 4x22 b[6:0]
			0x04F064, //uint32_t tx_freq_first; //reg 4x23,4x24,4x25
			0x04F064, //uint32_t tx_freq; //reg 4x26,4x27,4x28
			0x0070, //uint16_t tx_hpst; //reg 4x29,4x2A b[12:0]
			//0x28, //uint8_t tx_line_len; //reg 4x2B b[5:0]
			0x22, //uint8_t tx_line_len; //reg 4x2B b[5:0]
			0x0A, //uint8_t tx_all_data_len; //reg 4x2C b[7:0]
	},/*}}}*/

};
const _stPTZTxParam pr1000_ptz_txparam_hdt_new_def[6] =
{
	//720p60
	{/*{{{*/
		0, //int mapChn;

			0x00, //uint8_t tx_field_type; //reg 4x20 b[1:0]
			0x02, //uint8_t tx_line_cnt; //reg 4x21 b[7:3]
			0x00, //uint8_t tx_hst_os; //reg 4x21 b[2:0]
			0x03, //uint8_t tx_hst; //reg 4x22 b[6:0]
			0x04F064, //uint32_t tx_freq_first; //reg 4x23,4x24,4x25
			0x04F064, //uint32_t tx_freq; //reg 4x26,4x27,4x28
			0x0070, //uint16_t tx_hpst; //reg 4x29,4x2A b[12:0]
			//0x28, //uint8_t tx_line_len; //reg 4x2B b[5:0]
			0x22, //uint8_t tx_line_len; //reg 4x2B b[5:0]
			0x0A, //uint8_t tx_all_data_len; //reg 4x2C b[7:0]
	},/*}}}*/
	//720p50
	{/*{{{*/
		0, //int mapChn;

			0x00, //uint8_t tx_field_type; //reg 4x20 b[1:0]
			0x02, //uint8_t tx_line_cnt; //reg 4x21 b[7:3]
			0x00, //uint8_t tx_hst_os; //reg 4x21 b[2:0]
			0x03, //uint8_t tx_hst; //reg 4x22 b[6:0]
			0x04F064, //uint32_t tx_freq_first; //reg 4x23,4x24,4x25
			0x04F064, //uint32_t tx_freq; //reg 4x26,4x27,4x28
			0x0070, //uint16_t tx_hpst; //reg 4x29,4x2A b[12:0]
			//0x28, //uint8_t tx_line_len; //reg 4x2B b[5:0]
			0x22, //uint8_t tx_line_len; //reg 4x2B b[5:0]
			0x0A, //uint8_t tx_all_data_len; //reg 4x2C b[7:0]
	},/*}}}*/
	//720p30
	{/*{{{*/
		0, //int mapChn;

			0x00, //uint8_t tx_field_type; //reg 4x20 b[1:0]
			0x02, //uint8_t tx_line_cnt; //reg 4x21 b[7:3]
			0x00, //uint8_t tx_hst_os; //reg 4x21 b[2:0]
			0x04, //uint8_t tx_hst; //reg 4x22 b[6:0]
			0x027832, //uint32_t tx_freq_first; //reg 4x23,4x24,4x25
			0x027832, //uint32_t tx_freq; //reg 4x26,4x27,4x28
			0x0140, //uint16_t tx_hpst; //reg 4x29,4x2A b[12:0]
			//0x28, //uint8_t tx_line_len; //reg 4x2B b[5:0]
			0x22, //uint8_t tx_line_len; //reg 4x2B b[5:0]
			0x0A, //uint8_t tx_all_data_len; //reg 4x2C b[7:0]
	},/*}}}*/
	//720p25
	{/*{{{*/
		0, //int mapChn;

			0x00, //uint8_t tx_field_type; //reg 4x20 b[1:0]
			0x02, //uint8_t tx_line_cnt; //reg 4x21 b[7:3]
			0x00, //uint8_t tx_hst_os; //reg 4x21 b[2:0]
			0x04, //uint8_t tx_hst; //reg 4x22 b[6:0]
			0x027832, //uint32_t tx_freq_first; //reg 4x23,4x24,4x25
			0x027832, //uint32_t tx_freq; //reg 4x26,4x27,4x28
			0x0140, //uint16_t tx_hpst; //reg 4x29,4x2A b[12:0]
			//0x28, //uint8_t tx_line_len; //reg 4x2B b[5:0]
			0x22, //uint8_t tx_line_len; //reg 4x2B b[5:0]
			0x0A, //uint8_t tx_all_data_len; //reg 4x2C b[7:0]
	},/*}}}*/
	//1080p30
	{/*{{{*/
		0, //int mapChn;

			0x00, //uint8_t tx_field_type; //reg 4x20 b[1:0]
			0x02, //uint8_t tx_line_cnt; //reg 4x21 b[7:3]
			0x00, //uint8_t tx_hst_os; //reg 4x21 b[2:0]
			0x03, //uint8_t tx_hst; //reg 4x22 b[6:0]
			0x04F064, //uint32_t tx_freq_first; //reg 4x23,4x24,4x25
			0x04F064, //uint32_t tx_freq; //reg 4x26,4x27,4x28
			0x0070, //uint16_t tx_hpst; //reg 4x29,4x2A b[12:0]
			//0x28, //uint8_t tx_line_len; //reg 4x2B b[5:0]
			0x22, //uint8_t tx_line_len; //reg 4x2B b[5:0]
			0x0A, //uint8_t tx_all_data_len; //reg 4x2C b[7:0]
	},/*}}}*/
	//1080p25
	{/*{{{*/
		0, //int mapChn;

			0x00, //uint8_t tx_field_type; //reg 4x20 b[1:0]
			0x02, //uint8_t tx_line_cnt; //reg 4x21 b[7:3]
			0x00, //uint8_t tx_hst_os; //reg 4x21 b[2:0]
			0x04, //uint8_t tx_hst; //reg 4x22 b[6:0]
			0x04F064, //uint32_t tx_freq_first; //reg 4x23,4x24,4x25
			0x04F064, //uint32_t tx_freq; //reg 4x26,4x27,4x28
			0x0070, //uint16_t tx_hpst; //reg 4x29,4x2A b[12:0]
			//0x28, //uint8_t tx_line_len; //reg 4x2B b[5:0]
			0x22, //uint8_t tx_line_len; //reg 4x2B b[5:0]
			0x0A, //uint8_t tx_all_data_len; //reg 4x2C b[7:0]
	},/*}}}*/

};
#endif // DONT_SUPPORT_PTZ_FUNC


const _stPTZRxParam pr1000_ptz_rxparam_hdt_def[6] =
{
	//720p60
	{/*{{{*/
		0, //int mapChn;

			0x00, //uint8_t rx_field_type; //reg 4x00 b[1:0]
			0x02, //uint8_t rx_line_cnt; //reg 4x01 b[7:3]
			0x00, //uint8_t rx_hst_os; //reg 4x01 b[2:0]
			0x02, //uint8_t rx_hst; //reg 4x02 b[6:0]
			0x04F064, //uint32_t rx_freq_first; //reg 4x03,4x04,4x05
			0x04F064, //uint32_t rx_freq; //reg 4x06,4x07,4x08
			0x04, //uint8_t rx_lpf_len; //reg 4x09 b[5:0]
			0x10, //uint8_t rx_h_pix_offset; //reg 4x0A b[7:0]
			//0x28, //uint8_t rx_line_len; //reg 4x0B b[5:0]
			0x22, //uint8_t rx_line_len; //reg 4x0B b[5:0]
			0x0A, //uint8_t rx_valid_cnt; //reg 4x0C b[7:0]
	},/*}}}*/
	//720p50
	{/*{{{*/
		0, //int mapChn;

			0x00, //uint8_t rx_field_type; //reg 4x00 b[1:0]
			0x02, //uint8_t rx_line_cnt; //reg 4x01 b[7:3]
			0x00, //uint8_t rx_hst_os; //reg 4x01 b[2:0]
			0x02, //uint8_t rx_hst; //reg 4x02 b[6:0]
			0x04F064, //uint32_t rx_freq_first; //reg 4x03,4x04,4x05
			0x04F064, //uint32_t rx_freq; //reg 4x06,4x07,4x08
			0x04, //uint8_t rx_lpf_len; //reg 4x09 b[5:0]
			0x10, //uint8_t rx_h_pix_offset; //reg 4x0A b[7:0]
			//0x28, //uint8_t rx_line_len; //reg 4x0B b[5:0]
			0x22, //uint8_t rx_line_len; //reg 4x0B b[5:0]
			0x0A, //uint8_t rx_valid_cnt; //reg 4x0C b[7:0]
	},/*}}}*/
	//720p30
	{/*{{{*/
		0, //int mapChn;

			0x00, //uint8_t rx_field_type; //reg 4x00 b[1:0]
			0x02, //uint8_t rx_line_cnt; //reg 4x01 b[7:3]
			0x00, //uint8_t rx_hst_os; //reg 4x01 b[2:0]
			0x03, //uint8_t rx_hst; //reg 4x02 b[6:0]
			0x04F064, //uint32_t rx_freq_first; //reg 4x03,4x04,4x05
			0x04F064, //uint32_t rx_freq; //reg 4x06,4x07,4x08
			0x04, //uint8_t rx_lpf_len; //reg 4x09 b[5:0]
			0x10, //uint8_t rx_h_pix_offset; //reg 4x0A b[7:0]
			//0x28, //uint8_t rx_line_len; //reg 4x0B b[5:0]
			0x22, //uint8_t rx_line_len; //reg 4x0B b[5:0]
			0x0A, //uint8_t rx_valid_cnt; //reg 4x0C b[7:0]
	},/*}}}*/
	//720p25
	{/*{{{*/
		0, //int mapChn;

			0x00, //uint8_t rx_field_type; //reg 4x00 b[1:0]
			0x02, //uint8_t rx_line_cnt; //reg 4x01 b[7:3]
			0x00, //uint8_t rx_hst_os; //reg 4x01 b[2:0]
			0x03, //uint8_t rx_hst; //reg 4x02 b[6:0]
			0x04F064, //uint32_t rx_freq_first; //reg 4x03,4x04,4x05
			0x04F064, //uint32_t rx_freq; //reg 4x06,4x07,4x08
			0x04, //uint8_t rx_lpf_len; //reg 4x09 b[5:0]
			0x10, //uint8_t rx_h_pix_offset; //reg 4x0A b[7:0]
			//0x28, //uint8_t rx_line_len; //reg 4x0B b[5:0]
			0x22, //uint8_t rx_line_len; //reg 4x0B b[5:0]
			0x0A, //uint8_t rx_valid_cnt; //reg 4x0C b[7:0]
	},/*}}}*/
	//1080p30
	{/*{{{*/
		0, //int mapChn;

			0x00, //uint8_t rx_field_type; //reg 4x00 b[1:0]
			0x02, //uint8_t rx_line_cnt; //reg 4x01 b[7:3]
			0x00, //uint8_t rx_hst_os; //reg 4x01 b[2:0]
			0x02, //uint8_t rx_hst; //reg 4x02 b[6:0]
			0x04F064, //uint32_t rx_freq_first; //reg 4x03,4x04,4x05
			0x04F064, //uint32_t rx_freq; //reg 4x06,4x07,4x08
			0x04, //uint8_t rx_lpf_len; //reg 4x09 b[5:0]
			0x10, //uint8_t rx_h_pix_offset; //reg 4x0A b[7:0]
			//0x28, //uint8_t rx_line_len; //reg 4x0B b[5:0]
			0x22, //uint8_t rx_line_len; //reg 4x0B b[5:0]
			0x0A, //uint8_t rx_valid_cnt; //reg 4x0C b[7:0]
	},/*}}}*/
	//1080p25
	{/*{{{*/
		0, //int mapChn;

			0x00, //uint8_t rx_field_type; //reg 4x00 b[1:0]
			0x02, //uint8_t rx_line_cnt; //reg 4x01 b[7:3]
			0x00, //uint8_t rx_hst_os; //reg 4x01 b[2:0]
			0x03, //uint8_t rx_hst; //reg 4x02 b[6:0]
			0x04F064, //uint32_t rx_freq_first; //reg 4x03,4x04,4x05
			0x04F064, //uint32_t rx_freq; //reg 4x06,4x07,4x08
			0x04, //uint8_t rx_lpf_len; //reg 4x09 b[5:0]
			0x10, //uint8_t rx_h_pix_offset; //reg 4x0A b[7:0]
			//0x28, //uint8_t rx_line_len; //reg 4x0B b[5:0]
			0x22, //uint8_t rx_line_len; //reg 4x0B b[5:0]
			0x0A, //uint8_t rx_valid_cnt; //reg 4x0C b[7:0]
	},/*}}}*/

};
const _stPTZRxParam pr1000_ptz_rxparam_hdt_new_def[6] =
{
	//720p60
	{/*{{{*/
		0, //int mapChn;

			0x00, //uint8_t rx_field_type; //reg 4x00 b[1:0]
			0x02, //uint8_t rx_line_cnt; //reg 4x01 b[7:3]
			0x00, //uint8_t rx_hst_os; //reg 4x01 b[2:0]
			0x02, //uint8_t rx_hst; //reg 4x02 b[6:0]
			0x04F064, //uint32_t rx_freq_first; //reg 4x03,4x04,4x05
			0x04F064, //uint32_t rx_freq; //reg 4x06,4x07,4x08
			0x04, //uint8_t rx_lpf_len; //reg 4x09 b[5:0]
			0x10, //uint8_t rx_h_pix_offset; //reg 4x0A b[7:0]
			//0x28, //uint8_t rx_line_len; //reg 4x0B b[5:0]
			0x22, //uint8_t rx_line_len; //reg 4x0B b[5:0]
			0x0A, //uint8_t rx_valid_cnt; //reg 4x0C b[7:0]
	},/*}}}*/
	//720p50
	{/*{{{*/
		0, //int mapChn;

			0x00, //uint8_t rx_field_type; //reg 4x00 b[1:0]
			0x02, //uint8_t rx_line_cnt; //reg 4x01 b[7:3]
			0x00, //uint8_t rx_hst_os; //reg 4x01 b[2:0]
			0x02, //uint8_t rx_hst; //reg 4x02 b[6:0]
			0x04F064, //uint32_t rx_freq_first; //reg 4x03,4x04,4x05
			0x04F064, //uint32_t rx_freq; //reg 4x06,4x07,4x08
			0x04, //uint8_t rx_lpf_len; //reg 4x09 b[5:0]
			0x10, //uint8_t rx_h_pix_offset; //reg 4x0A b[7:0]
			//0x28, //uint8_t rx_line_len; //reg 4x0B b[5:0]
			0x22, //uint8_t rx_line_len; //reg 4x0B b[5:0]
			0x0A, //uint8_t rx_valid_cnt; //reg 4x0C b[7:0]
	},/*}}}*/
	//720p30
	{/*{{{*/
		0, //int mapChn;

			0x00, //uint8_t rx_field_type; //reg 4x00 b[1:0]
			0x02, //uint8_t rx_line_cnt; //reg 4x01 b[7:3]
			0x00, //uint8_t rx_hst_os; //reg 4x01 b[2:0]
			0x03, //uint8_t rx_hst; //reg 4x02 b[6:0]
			0x04F064, //uint32_t rx_freq_first; //reg 4x03,4x04,4x05
			0x04F064, //uint32_t rx_freq; //reg 4x06,4x07,4x08
			0x04, //uint8_t rx_lpf_len; //reg 4x09 b[5:0]
			0x10, //uint8_t rx_h_pix_offset; //reg 4x0A b[7:0]
			//0x28, //uint8_t rx_line_len; //reg 4x0B b[5:0]
			0x22, //uint8_t rx_line_len; //reg 4x0B b[5:0]
			0x0A, //uint8_t rx_valid_cnt; //reg 4x0C b[7:0]
	},/*}}}*/
	//720p25
	{/*{{{*/
		0, //int mapChn;

			0x00, //uint8_t rx_field_type; //reg 4x00 b[1:0]
			0x02, //uint8_t rx_line_cnt; //reg 4x01 b[7:3]
			0x00, //uint8_t rx_hst_os; //reg 4x01 b[2:0]
			0x03, //uint8_t rx_hst; //reg 4x02 b[6:0]
			0x04F064, //uint32_t rx_freq_first; //reg 4x03,4x04,4x05
			0x04F064, //uint32_t rx_freq; //reg 4x06,4x07,4x08
			0x04, //uint8_t rx_lpf_len; //reg 4x09 b[5:0]
			0x10, //uint8_t rx_h_pix_offset; //reg 4x0A b[7:0]
			//0x28, //uint8_t rx_line_len; //reg 4x0B b[5:0]
			0x22, //uint8_t rx_line_len; //reg 4x0B b[5:0]
			0x0A, //uint8_t rx_valid_cnt; //reg 4x0C b[7:0]
	},/*}}}*/
	//1080p30
	{/*{{{*/
		0, //int mapChn;

			0x00, //uint8_t rx_field_type; //reg 4x00 b[1:0]
			0x02, //uint8_t rx_line_cnt; //reg 4x01 b[7:3]
			0x00, //uint8_t rx_hst_os; //reg 4x01 b[2:0]
			0x02, //uint8_t rx_hst; //reg 4x02 b[6:0]
			0x04F064, //uint32_t rx_freq_first; //reg 4x03,4x04,4x05
			0x04F064, //uint32_t rx_freq; //reg 4x06,4x07,4x08
			0x04, //uint8_t rx_lpf_len; //reg 4x09 b[5:0]
			0x10, //uint8_t rx_h_pix_offset; //reg 4x0A b[7:0]
			//0x28, //uint8_t rx_line_len; //reg 4x0B b[5:0]
			0x22, //uint8_t rx_line_len; //reg 4x0B b[5:0]
			0x0A, //uint8_t rx_valid_cnt; //reg 4x0C b[7:0]
	},/*}}}*/
	//1080p25
	{/*{{{*/
		0, //int mapChn;

			0x00, //uint8_t rx_field_type; //reg 4x00 b[1:0]
			0x02, //uint8_t rx_line_cnt; //reg 4x01 b[7:3]
			0x00, //uint8_t rx_hst_os; //reg 4x01 b[2:0]
			0x03, //uint8_t rx_hst; //reg 4x02 b[6:0]
			0x04F064, //uint32_t rx_freq_first; //reg 4x03,4x04,4x05
			0x04F064, //uint32_t rx_freq; //reg 4x06,4x07,4x08
			0x04, //uint8_t rx_lpf_len; //reg 4x09 b[5:0]
			0x10, //uint8_t rx_h_pix_offset; //reg 4x0A b[7:0]
			//0x28, //uint8_t rx_line_len; //reg 4x0B b[5:0]
			0x22, //uint8_t rx_line_len; //reg 4x0B b[5:0]
			0x0A, //uint8_t rx_valid_cnt; //reg 4x0C b[7:0]
	},/*}}}*/

};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*** HDT PATTERN ***/
#ifndef DONT_SUPPORT_PTZ_FUNC
/* PTZ HDT Tx pattern format */
const unsigned char pr1000_ptz_table_hdt_tx_pat_format[((5*1)*2)] = 
{/*{{{*/
	/* frame 1 */
	///* 1 */ "1111 1111 0000 0000 0000 0000 0000 0000 0000 0000"
	///* 2 */ "1111 1111 0000 0000 0000 0000 0000 0000 0000 0000"
	/* 1 */ 0xC0, 0x00, 0x00, 0x00, 0x00, //"1100 0000 0000 0000 0000 0000 0000 0000 0000 0000"
	/* 2 */ 0xC0, 0x00, 0x00, 0x00, 0x00, //"1100 0000 0000 0000 0000 0000 0000 0000 0000 0000"
};/*}}}*/
/* PTZ HDT Tx pattern data */
const unsigned char pr1000_ptz_table_hdt_tx_pat_data[((5*1)*2)] = 
{/*{{{*/
	/* frame 1 */
	///* 1 */ "1111 1111 xxxx xxxx xxxx xxxx xxxx xxxx xxxx xxxx"
	///* 2 */ "1111 1111 xxxx xxxx xxxx xxxx xxxx xxxx xxxx xxxx"
	/* 1 */ 0x80, 0x00, 0x00, 0x00, 0x00, //"10xx xxxx xxxx xxxx xxxx xxxx xxxx xxxx xxxx xxxx"
	/* 2 */ 0x80, 0x00, 0x00, 0x00, 0x00, //"10xx xxxx xxxx xxxx xxxx xxxx xxxx xxxx xxxx xxxx"
};/*}}}*/
#endif // DONT_SUPPORT_PTZ_FUNC

/* PTZ HDT Rx pattern format */
const unsigned char pr1000_ptz_table_hdt_rx_pat_format[((5*1)*2)] = 
{/*{{{*/
	/* frame 1 */
	///* 1 */ "1111 1111 0000 0000 0000 0000 0000 0000 0000 0000"
	///* 2 */ "1111 1111 0000 0000 0000 0000 0000 0000 0000 0000"
	/* 1 */ 0xC0, 0x00, 0x00, 0x00, 0x00, //"1100 0000 0000 0000 0000 0000 0000 0000 0000 0000"
	/* 2 */ 0xC0, 0x00, 0x00, 0x00, 0x00, //"1100 0000 0000 0000 0000 0000 0000 0000 0000 0000"
};/*}}}*/
/* PTZ HDT Rx pattern start format */
const unsigned char pr1000_ptz_table_hdt_rx_pat_start_format[(5*1)] = 
{/*{{{*/
	//"1111 1111 0000 0000 0000 0000 0000 0000 0000 0000"
	0xC0, 0x00, 0x00, 0x00, 0x00, //"1100 0000 0000 0000 0000 0000 0000 0000 0000 0000"
};/*}}}*/
/* PTZ HDT Rx pattern start data */
const unsigned char pr1000_ptz_table_hdt_rx_pat_start_data[(5*1)] = 
{/*{{{*/
	//"1111 1111 xxxx xxxx xxxx xxxx xxxx xxxx xxxx xxxx"
	0x80, 0x00, 0x00, 0x00, 0x00, //"10xx xxxx xxxx xxxx xxxx xxxx xxxx xxxx xxxx xxxx"
};/*}}}*/

/*** HDT_NEW PATTERN ***/
#ifndef DONT_SUPPORT_PTZ_FUNC
/* PTZ HDT_NEW Tx pattern format */
const unsigned char pr1000_ptz_table_hdt_new_tx_pat_format[((5*1)*2)] = 
{/*{{{*/
	/* frame 1 */
	///* 1 */ "1111 1111 0000 0000 0000 0000 0000 0000 0000 0000"
	///* 2 */ "1111 1111 0000 0000 0000 0000 0000 0000 0000 0000"
	/* 1 */ 0xC0, 0x00, 0x00, 0x00, 0x00, //"1100 0000 0000 0000 0000 0000 0000 0000 0000 0000"
	/* 2 */ 0xC0, 0x00, 0x00, 0x00, 0x00, //"1100 0000 0000 0000 0000 0000 0000 0000 0000 0000"
};/*}}}*/
/* PTZ HDT_NEW Tx pattern data */
const unsigned char pr1000_ptz_table_hdt_new_tx_pat_data[((5*1)*2)] = 
{/*{{{*/
	/* frame 1 */
	///* 1 */ "1111 1111 xxxx xxxx xxxx xxxx xxxx xxxx xxxx xxxx"
	///* 2 */ "1111 1111 xxxx xxxx xxxx xxxx xxxx xxxx xxxx xxxx"
	/* 1 */ 0x80, 0x00, 0x00, 0x00, 0x00, //"10xx xxxx xxxx xxxx xxxx xxxx xxxx xxxx xxxx xxxx"
	/* 2 */ 0x80, 0x00, 0x00, 0x00, 0x00, //"10xx xxxx xxxx xxxx xxxx xxxx xxxx xxxx xxxx xxxx"
};/*}}}*/
#endif // DONT_SUPPORT_PTZ_FUNC

/* PTZ HDT_NEW Rx pattern format */
const unsigned char pr1000_ptz_table_hdt_new_rx_pat_format[((5*1)*2)] = 
{/*{{{*/
	/* frame 1 */
	///* 1 */ "1111 1111 0000 0000 0000 0000 0000 0000 0000 0000"
	///* 2 */ "1111 1111 0000 0000 0000 0000 0000 0000 0000 0000"
	/* 1 */ 0xC0, 0x00, 0x00, 0x00, 0x00, //"1100 0000 0000 0000 0000 0000 0000 0000 0000 0000"
	/* 2 */ 0xC0, 0x00, 0x00, 0x00, 0x00, //"1100 0000 0000 0000 0000 0000 0000 0000 0000 0000"
};/*}}}*/
/* PTZ HDT_NEW Rx pattern start format */
const unsigned char pr1000_ptz_table_hdt_new_rx_pat_start_format[(5*1)] = 
{/*{{{*/
	//"1111 1111 0000 0000 0000 0000 0000 0000 0000 0000"
	0xC0, 0x00, 0x00, 0x00, 0x00, //"1100 0000 0000 0000 0000 0000 0000 0000 0000 0000"
};/*}}}*/
/* PTZ HDT_NEW Rx pattern start data */
const unsigned char pr1000_ptz_table_hdt_new_rx_pat_start_data[(5*1)] = 
{/*{{{*/
	//"1111 1111 xxxx xxxx xxxx xxxx xxxx xxxx xxxx xxxx"
	0x80, 0x00, 0x00, 0x00, 0x00, //"10xx xxxx xxxx xxxx xxxx xxxx xxxx xxxx xxxx xxxx"
};/*}}}*/

//////////////////////////////////////////////////////////////////////////////////////////

