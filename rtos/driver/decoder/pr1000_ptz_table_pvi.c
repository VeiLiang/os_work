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
#include "pr1000_ptz_table_pvi.h"

#else //#ifdef __LINUX_SYSTEM__

#include <stdio.h>
#include "pr1000_drvcommon.h"
#include "pr1000_ptz_table.h"
#include "pr1000_ptz_table_pvi.h"
#include "pr1000_user_config.h"

#endif // __LINUX_SYSTEM__

#ifndef DONT_SUPPORT_STD_PVI
//////////////////////////////////////////////////////////////////////////////////////// 

#ifndef DONT_SUPPORT_PTZ_FUNC
/*** PVI COMMAND ***/
const uint8_t pr1000_ptz_table_pvi_CMD_VALID[(7)] = 	{0xFF,0xFF,0xFF,/**/0xFF,0xFF,0xFF,0x00,};// define rx compare byte. 1's compare bit
const uint8_t pr1000_ptz_table_pvi_OSD_TOP[(7)] = 	{0x00,0x00,0x08,/**/0xAA,0x00,0x00,0xB2,};
const uint8_t pr1000_ptz_table_pvi_OSD_RIGHT[(7)] =	{0x00,0x00,0x04,/**/0xAA,0x00,0x00,0xAE,};
const uint8_t pr1000_ptz_table_pvi_OSD_BOTTOM[(7)]=	{0x00,0x00,0x02,/**/0xAA,0x00,0x00,0xAC,};
const uint8_t pr1000_ptz_table_pvi_OSD_LEFT[(7)] = 	{0x00,0x00,0x01,/**/0xAA,0x00,0x00,0xAB,};
const uint8_t pr1000_ptz_table_pvi_OSD_CENTER[(7)]=	{0x00,0x00,0x00,/**/0xAA,0x00,0x00,0xAA,};
const uint8_t pr1000_ptz_table_pvi_IRIS_PLUS[(7)] =	{0x00,0x00,0x00,/**/0xAA,0x00,0x00,0xAA,};
const uint8_t pr1000_ptz_table_pvi_IRIS_MINUS[(7)]=	{0x00,0x00,0x00,/**/0xAA,0x00,0x00,0xAA,};
const uint8_t pr1000_ptz_table_pvi_FOCUS_PLUS[(7)] =	{0x00,0x00,0x00,/**/0xAA,0x00,0x00,0xAA,};
const uint8_t pr1000_ptz_table_pvi_FOCUS_MINUS[(7)]=	{0x00,0x00,0x00,/**/0xAA,0x00,0x00,0xAA,};
const uint8_t pr1000_ptz_table_pvi_ZOOM_PLUS[(7)] =	{0x00,0x00,0x00,/**/0xAA,0x00,0x00,0xAA,};
const uint8_t pr1000_ptz_table_pvi_ZOOM_MINUS[(7)]=	{0x00,0x00,0x00,/**/0xAA,0x00,0x00,0xAA,};

/* PVI */
#ifndef DONT_SUPPORT_PTZ_ETC_CMD
const _pr1000PTZCmd pr1000_ptz_table_pvicmd[max_pr1000_ptz_table_command] = 
{/*{{{*/
	{ pr1000_ptz_table_pvi_OSD_TOP,    sizeof(pr1000_ptz_table_pvi_OSD_TOP)/sizeof(pr1000_ptz_table_pvi_OSD_TOP[0]) },
	{ pr1000_ptz_table_pvi_OSD_RIGHT,  sizeof(pr1000_ptz_table_pvi_OSD_RIGHT)/sizeof(pr1000_ptz_table_pvi_OSD_RIGHT[0]) },
	{ pr1000_ptz_table_pvi_OSD_BOTTOM, sizeof(pr1000_ptz_table_pvi_OSD_BOTTOM)/sizeof(pr1000_ptz_table_pvi_OSD_BOTTOM[0]) },
	{ pr1000_ptz_table_pvi_OSD_LEFT,   sizeof(pr1000_ptz_table_pvi_OSD_LEFT)/sizeof(pr1000_ptz_table_pvi_OSD_LEFT[0]) },
	{ pr1000_ptz_table_pvi_OSD_CENTER, sizeof(pr1000_ptz_table_pvi_OSD_CENTER)/sizeof(pr1000_ptz_table_pvi_OSD_CENTER[0]) },
	{ pr1000_ptz_table_pvi_IRIS_PLUS,  sizeof(pr1000_ptz_table_pvi_IRIS_PLUS)/sizeof(pr1000_ptz_table_pvi_IRIS_PLUS[0]) },
	{ pr1000_ptz_table_pvi_IRIS_MINUS, sizeof(pr1000_ptz_table_pvi_IRIS_MINUS)/sizeof(pr1000_ptz_table_pvi_IRIS_MINUS[0]) },
	{ pr1000_ptz_table_pvi_FOCUS_PLUS, sizeof(pr1000_ptz_table_pvi_FOCUS_PLUS)/sizeof(pr1000_ptz_table_pvi_FOCUS_PLUS[0]) },
	{ pr1000_ptz_table_pvi_FOCUS_MINUS,sizeof(pr1000_ptz_table_pvi_FOCUS_MINUS)/sizeof(pr1000_ptz_table_pvi_FOCUS_MINUS[0]) },
	{ pr1000_ptz_table_pvi_ZOOM_PLUS,  sizeof(pr1000_ptz_table_pvi_ZOOM_PLUS)/sizeof(pr1000_ptz_table_pvi_ZOOM_PLUS[0]) },
	{ pr1000_ptz_table_pvi_ZOOM_MINUS, sizeof(pr1000_ptz_table_pvi_ZOOM_MINUS)/sizeof(pr1000_ptz_table_pvi_ZOOM_MINUS[0]) },
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
	{ NULL,    0 }, //RESERVED0
};/*}}}*/
#endif // DONT_SUPPORT_PTZ_ETC_CMD
#endif // DONT_SUPPORT_PTZ_FUNC

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*** PVI PTZ Parameter ***/
#ifndef DONT_SUPPORT_PTZ_FUNC
const _stPTZTxParam pr1000_ptz_txparam_pvi_def[6] =
{
	//720p60
	{/*{{{*/
		0, //int mapChn;

			0x00, //uint8_t tx_field_type; //reg 4x20 b[1:0]
			0x04, //uint8_t tx_line_cnt; //reg 4x21 b[7:3]
			0x00, //uint8_t tx_hst_os; //reg 4x21 b[2:0]
			0x07, //uint8_t tx_hst; //reg 4x22 b[6:0]
			0x035376, //uint32_t tx_freq_first; //reg 4x23,4x24,4x25
			0x035376, //uint32_t tx_freq; //reg 4x26,4x27,4x28
			0x01A0, //uint16_t tx_hpst; //reg 4x29,4x2A b[12:0]
			0x18, //uint8_t tx_line_len; //reg 4x2B b[5:0]
			0x18, //uint8_t tx_all_data_len; //reg 4x2C b[7:0]
	},/*}}}*/
	//720p50
	{/*{{{*/
		0, //int mapChn;

			0x00, //uint8_t tx_field_type; //reg 4x20 b[1:0]
			0x04, //uint8_t tx_line_cnt; //reg 4x21 b[7:3]
			0x00, //uint8_t tx_hst_os; //reg 4x21 b[2:0]
			0x07, //uint8_t tx_hst; //reg 4x22 b[6:0]
			0x035376, //uint32_t tx_freq_first; //reg 4x23,4x24,4x25
			0x035376, //uint32_t tx_freq; //reg 4x26,4x27,4x28
			0x01A0, //uint16_t tx_hpst; //reg 4x29,4x2A b[12:0]
			0x18, //uint8_t tx_line_len; //reg 4x2B b[5:0]
			0x18, //uint8_t tx_all_data_len; //reg 4x2C b[7:0]
	},/*}}}*/
	//720p30
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
			0x18, //uint8_t tx_all_data_len; //reg 4x2C b[7:0]
	},/*}}}*/
	//720p25
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
			0x18, //uint8_t tx_all_data_len; //reg 4x2C b[7:0]
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
			0x18, //uint8_t tx_all_data_len; //reg 4x2C b[7:0]
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
			0x18, //uint8_t tx_all_data_len; //reg 4x2C b[7:0]
	},/*}}}*/

};
#endif // DONT_SUPPORT_PTZ_FUNC

const _stPTZRxParam pr1000_ptz_rxparam_pvi_def[6] =
{
	//720p60
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
			0x18, //uint8_t rx_valid_cnt; //reg 4x0C b[7:0]
	},/*}}}*/
	//720p50
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
			0x18, //uint8_t rx_valid_cnt; //reg 4x0C b[7:0]
	},/*}}}*/
	//720p30
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
			0x18, //uint8_t rx_valid_cnt; //reg 4x0C b[7:0]
	},/*}}}*/
	//720p25
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
			0x18, //uint8_t rx_valid_cnt; //reg 4x0C b[7:0]
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
			0x18, //uint8_t rx_valid_cnt; //reg 4x0C b[7:0]
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
			0x18, //uint8_t rx_valid_cnt; //reg 4x0C b[7:0]
	},/*}}}*/

};
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*** PVI PATTERN ***/
#ifndef DONT_SUPPORT_PTZ_FUNC
/* PTZ PVI Tx pattern format */
const unsigned char pr1000_ptz_table_pvi_tx_pat_format[((3*4)*2)] = 
{/*{{{*/
	/* frame 1 */
	/* 1 */ 0xFF, 0xFF, 0xFF, //"111 111 111 111 111 111 111 111"
	/* 2 */ 0xB6, 0xDB, 0x6D, //"101 101 101 101 101 101 101 101"
	/* 3 */ 0xB6, 0xDB, 0x6D, //"101 101 101 101 101 101 101 101"
	/* 4 */ 0xB6, 0xDB, 0x6D, //"101 101 101 101 101 101 101 101"
	/* frame 2 */
	/* 1 */ 0xB6, 0xDB, 0x6D, //"101 101 101 101 101 101 101 101"
	/* 2 */ 0xB6, 0xDB, 0x6D, //"101 101 101 101 101 101 101 101"
	/* 3 */ 0xB6, 0xDB, 0x6D, //"101 101 101 101 101 101 101 101"
	/* 4 */ 0xB6, 0xDB, 0x6D, //"101 101 101 101 101 101 101 101"
};/*}}}*/
/* PTZ PVI Tx pattern data */
const unsigned char pr1000_ptz_table_pvi_tx_pat_data[((3*4)*2)] = 
{/*{{{*/
	/* frame 1 */
	/* 1 */ 0xD3, 0x49, 0xA6, //"110 100 110 100 100 110 100 110"
	/* 2 */ 0x92, 0x49, 0x24, //"1x0 1x0 1x0 1x0 1x0 1x0 1x0 1x0"
	/* 3 */ 0x92, 0x49, 0x24, //"1x0 1x0 1x0 1x0 1x0 1x0 1x0 1x0"
	/* 4 */ 0x92, 0x49, 0x24, //"1x0 1x0 1x0 1x0 1x0 1x0 1x0 1x0"
	/* frame 2 */
	/* 1 */ 0x92, 0x49, 0x24, //"1x0 1x0 1x0 1x0 1x0 1x0 1x0 1x0"
	/* 2 */ 0x92, 0x49, 0x24, //"1x0 1x0 1x0 1x0 1x0 1x0 1x0 1x0"
	/* 3 */ 0x92, 0x49, 0x24, //"1x0 1x0 1x0 1x0 1x0 1x0 1x0 1x0"
	/* 4 */ 0x92, 0x49, 0x24, //"1x0 1x0 1x0 1x0 1x0 1x0 1x0 1x0"
};/*}}}*/
#endif // DONT_SUPPORT_PTZ_FUNC

/* PTZ PVI Rx pattern format */
const unsigned char pr1000_ptz_table_pvi_rx_pat_format[((3*4)*2)] = 
{/*{{{*/
	/* frame 1 */
	/* 1 */ 0xFF, 0xFF, 0xFF, //"111 111 111 111 111 111 111 111"
	/* 2 */ 0xB6, 0xDB, 0x6D, //"101 101 101 101 101 101 101 101"
	/* 3 */ 0xB6, 0xDB, 0x6D, //"101 101 101 101 101 101 101 101"
	/* 4 */ 0xB6, 0xDB, 0x6D, //"101 101 101 101 101 101 101 101"
	/* frame 2 */
	/* 1 */ 0xB6, 0xDB, 0x6D, //"101 101 101 101 101 101 101 101"
	/* 2 */ 0xB6, 0xDB, 0x6D, //"101 101 101 101 101 101 101 101"
	/* 3 */ 0xB6, 0xDB, 0x6D, //"101 101 101 101 101 101 101 101"
	/* 4 */ 0xB6, 0xDB, 0x6D, //"101 101 101 101 101 101 101 101"
};/*}}}*/
/* PTZ PVI Rx pattern start format */
const unsigned char pr1000_ptz_table_pvi_rx_pat_start_format[(3*1)] = 
{/*{{{*/
	0xFF, 0xFF, 0xFF, //"111 111 111 111 111 111 111 111"
};/*}}}*/
/* PTZ PVI Rx pattern start data */
const unsigned char pr1000_ptz_table_pvi_rx_pat_start_data[(3*1)] = 
{/*{{{*/
	0xD3, 0x49, 0xA6, //"110 100 110 100 100 110 100 110"
};/*}}}*/

//////////////////////////////////////////////////////////////////////////////////////////

#endif // DONT_SUPPORT_STD_PVI
