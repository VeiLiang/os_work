#ifdef __LINUX_SYSTEM__
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/fcntl.h>
#include <linux/mm.h>
#include <linux/miscdevice.h>
#include <linux/proc_fs.h>

#include "pr1000_drvcommon.h"
#include "pr1000_ptz_table.h"
#include "pr1000_ptz_drvtable.h"

#else //#ifdef __LINUX_SYSTEM__

#include "pr1000_drvcommon.h"
#include "pr1000_ptz_table.h"
#include "pr1000_ptz_drvtable.h"

#endif // __LINUX_SYSTEM__

////////////////////////////////// HDA ////////////////////////////////////////////////////
const _stPTZTxParam pr1000_ptz_txparam_hda_stdformat_def[6] =
{
	//720p60
	{/*{{{*/
		0, //don't care. int mapChn;

			0x00, //uint8_t tx_field_type; //reg 4x20 b[1:0]
			0x02, //uint8_t tx_line_cnt; //reg 4x21 b[7:3]
			0x00, //uint8_t tx_hst_os; //reg 4x21 b[2:0]
			0x07, //uint8_t tx_hst; //reg 4x22 b[6:0]
			0x035376, //uint32_t tx_freq_first; //reg 4x23,4x24,4x25
			0x035376, //uint32_t tx_freq; //reg 4x26,4x27,4x28
			0x01A0, //uint16_t tx_hpst; //reg 4x29,4x2A b[12:0]
			0x30, //uint8_t tx_line_len; //reg 4x2B b[5:0]
			0x0C, //uint8_t tx_all_data_len; //reg 4x2C b[7:0]
	},/*}}}*/
	//720p50
	{/*{{{*/
		0, //don't care. int mapChn;

			0x00, //uint8_t tx_field_type; //reg 4x20 b[1:0]
			0x02, //uint8_t tx_line_cnt; //reg 4x21 b[7:3]
			0x00, //uint8_t tx_hst_os; //reg 4x21 b[2:0]
			0x07, //uint8_t tx_hst; //reg 4x22 b[6:0]
			0x035376, //uint32_t tx_freq_first; //reg 4x23,4x24,4x25
			0x035376, //uint32_t tx_freq; //reg 4x26,4x27,4x28
			0x01A0, //uint16_t tx_hpst; //reg 4x29,4x2A b[12:0]
			0x30, //uint8_t tx_line_len; //reg 4x2B b[5:0]
			0x0C, //uint8_t tx_all_data_len; //reg 4x2C b[7:0]
	},/*}}}*/
	//720p30
	{/*{{{*/
		0, //don't care. int mapChn;

			0x00, //uint8_t tx_field_type; //reg 4x20 b[1:0]
			0x02, //uint8_t tx_line_cnt; //reg 4x21 b[7:3]
			0x00, //uint8_t tx_hst_os; //reg 4x21 b[2:0]
			0x10, //uint8_t tx_hst; //reg 4x22 b[6:0]
			0x035376, //uint32_t tx_freq_first; //reg 4x23,4x24,4x25
			0x035376, //uint32_t tx_freq; //reg 4x26,4x27,4x28
			0x02C0, //uint16_t tx_hpst; //reg 4x29,4x2A b[12:0]
			0x30, //uint8_t tx_line_len; //reg 4x2B b[5:0]
			0x0C, //uint8_t tx_all_data_len; //reg 4x2C b[7:0]
	},/*}}}*/
	//720p25
	{/*{{{*/
		0, //don't care. int mapChn;

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
		0, //don't care. int mapChn;

			0x00, //uint8_t tx_field_type; //reg 4x20 b[1:0]
			0x04, //uint8_t tx_line_cnt; //reg 4x21 b[7:3]
			0x00, //uint8_t tx_hst_os; //reg 4x21 b[2:0]
			0x10, //uint8_t tx_hst; //reg 4x22 b[6:0]
			0x035376, //uint32_t tx_freq_first; //reg 4x23,4x24,4x25
			0x035376, //uint32_t tx_freq; //reg 4x26,4x27,4x28
			0x0124, //uint16_t tx_hpst; //reg 4x29,4x2A b[12:0]
			0x18, //uint8_t tx_line_len; //reg 4x2B b[5:0]
			0x48, //uint8_t tx_all_data_len; //reg 4x2C b[7:0]
	},/*}}}*/
	//1080p25
	{/*{{{*/
		0, //don't care. int mapChn;

			0x00, //uint8_t tx_field_type; //reg 4x20 b[1:0]
			0x04, //uint8_t tx_line_cnt; //reg 4x21 b[7:3]
			0x00, //uint8_t tx_hst_os; //reg 4x21 b[2:0]
			0x10, //uint8_t tx_hst; //reg 4x22 b[6:0]
			0x035376, //uint32_t tx_freq_first; //reg 4x23,4x24,4x25
			0x035376, //uint32_t tx_freq; //reg 4x26,4x27,4x28
			0x0124, //uint16_t tx_hpst; //reg 4x29,4x2A b[12:0]
			0x18, //uint8_t tx_line_len; //reg 4x2B b[5:0]
			0x48, //uint8_t tx_all_data_len; //reg 4x2C b[7:0]
	},/*}}}*/

};

const _stPTZRxParam pr1000_ptz_rxparam_hda_stdformat_def[6] =
{
	//720p60
	{/*{{{*/
		0, //don't care. int mapChn;

			0x00, //uint8_t rx_field_type; //reg 4x00 b[1:0]
			0x04, //uint8_t rx_line_cnt; //reg 4x01 b[7:3]
			0x00, //uint8_t rx_hst_os; //reg 4x01 b[2:0]
			0x10, //uint8_t rx_hst; //reg 4x02 b[6:0]
			0x002600, //uint32_t rx_freq_first; //reg 4x03,4x04,4x05
			0x025076, //uint32_t rx_freq; //reg 4x06,4x07,4x08
			0x04, //uint8_t rx_lpf_len; //reg 4x09 b[5:0]
			0x10, //uint8_t rx_h_pix_offset; //reg 4x0A b[7:0]
			0x18, //uint8_t rx_line_len; //reg 4x0B b[5:0]
			0x02, //uint8_t rx_valid_cnt; //reg 4x0C b[7:0]
	},/*}}}*/
	//720p50
	{/*{{{*/
		0, //don't care. int mapChn;

			0x00, //uint8_t rx_field_type; //reg 4x00 b[1:0]
			0x04, //uint8_t rx_line_cnt; //reg 4x01 b[7:3]
			0x00, //uint8_t rx_hst_os; //reg 4x01 b[2:0]
			0x10, //uint8_t rx_hst; //reg 4x02 b[6:0]
			0x002600, //uint32_t rx_freq_first; //reg 4x03,4x04,4x05
			0x025076, //uint32_t rx_freq; //reg 4x06,4x07,4x08
			0x04, //uint8_t rx_lpf_len; //reg 4x09 b[5:0]
			0x10, //uint8_t rx_h_pix_offset; //reg 4x0A b[7:0]
			0x18, //uint8_t rx_line_len; //reg 4x0B b[5:0]
			0x02, //uint8_t rx_valid_cnt; //reg 4x0C b[7:0]
	},/*}}}*/
	//720p30
	{/*{{{*/
		0, //don't care. int mapChn;

			0x00, //uint8_t rx_field_type; //reg 4x00 b[1:0]
			0x04, //uint8_t rx_line_cnt; //reg 4x01 b[7:3]
			0x00, //uint8_t rx_hst_os; //reg 4x01 b[2:0]
			0x10, //uint8_t rx_hst; //reg 4x02 b[6:0]
			0x002600, //uint32_t rx_freq_first; //reg 4x03,4x04,4x05
			0x025076, //uint32_t rx_freq; //reg 4x06,4x07,4x08
			0x04, //uint8_t rx_lpf_len; //reg 4x09 b[5:0]
			0x10, //uint8_t rx_h_pix_offset; //reg 4x0A b[7:0]
			0x18, //uint8_t rx_line_len; //reg 4x0B b[5:0]
			0x02, //uint8_t rx_valid_cnt; //reg 4x0C b[7:0]
	},/*}}}*/
	//720p25
	{/*{{{*/
		0, //don't care. int mapChn;

			0x00, //uint8_t rx_field_type; //reg 4x00 b[1:0]
			0x04, //uint8_t rx_line_cnt; //reg 4x01 b[7:3]
			0x00, //uint8_t rx_hst_os; //reg 4x01 b[2:0]
			0x10, //uint8_t rx_hst; //reg 4x02 b[6:0]
			0x002600, //uint32_t rx_freq_first; //reg 4x03,4x04,4x05
			0x025076, //uint32_t rx_freq; //reg 4x06,4x07,4x08
			0x04, //uint8_t rx_lpf_len; //reg 4x09 b[5:0]
			0x10, //uint8_t rx_h_pix_offset; //reg 4x0A b[7:0]
			0x18, //uint8_t rx_line_len; //reg 4x0B b[5:0]
			0x02, //uint8_t rx_valid_cnt; //reg 4x0C b[7:0]
	},/*}}}*/
	//1080p30
	{/*{{{*/
		0, //don't care. int mapChn;

			0x00, //uint8_t rx_field_type; //reg 4x00 b[1:0]
			0x04, //uint8_t rx_line_cnt; //reg 4x01 b[7:3]
			0x00, //uint8_t rx_hst_os; //reg 4x01 b[2:0]
			0x10, //uint8_t rx_hst; //reg 4x02 b[6:0]
			0x002600, //uint32_t rx_freq_first; //reg 4x03,4x04,4x05
			0x02A076, //uint32_t rx_freq; //reg 4x06,4x07,4x08
			0x04, //uint8_t rx_lpf_len; //reg 4x09 b[5:0]
			0x10, //uint8_t rx_h_pix_offset; //reg 4x0A b[7:0]
			0x18, //uint8_t rx_line_len; //reg 4x0B b[5:0]
			0x02, //uint8_t rx_valid_cnt; //reg 4x0C b[7:0]
	},/*}}}*/
	//1080p25
	{/*{{{*/
		0, //don't care. int mapChn;

			0x00, //uint8_t rx_field_type; //reg 4x00 b[1:0]
			0x04, //uint8_t rx_line_cnt; //reg 4x01 b[7:3]
			0x00, //uint8_t rx_hst_os; //reg 4x01 b[2:0]
			0x10, //uint8_t rx_hst; //reg 4x02 b[6:0]
			0x002600, //uint32_t rx_freq_first; //reg 4x03,4x04,4x05
			0x02A076, //uint32_t rx_freq; //reg 4x06,4x07,4x08
			0x04, //uint8_t rx_lpf_len; //reg 4x09 b[5:0]
			0x10, //uint8_t rx_h_pix_offset; //reg 4x0A b[7:0]
			0x18, //uint8_t rx_line_len; //reg 4x0B b[5:0]
			0x02, //uint8_t rx_valid_cnt; //reg 4x0C b[7:0]
	},/*}}}*/

};

/* PTZ HDA Tx pattern format */
const unsigned char pr1000_ptz_table_hda_stdformat_tx_pat_format[((3*4)*6)] =
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
/* PTZ HDA Tx pattern data */
const unsigned char pr1000_ptz_table_hda_stdformat_tx_pat_data[((3*4)*6)] =
{/*{{{*/
	/* frame 1 */
	/* 1 */ 0x80, 0x00, 0x00, //"100 000 000 000 000 000 000 000"
	/* 2 */ 0x00, 0x00, 0x00, //"000 000 000 000 000 000 000 000"
	/* 3 */ 0x00, 0x00, 0x00, //"000 000 000 000 000 000 000 000"
	/* 4 */ 0x00, 0x00, 0x00, //"000 000 000 000 000 000 000 000"
	/* frame 2 */
	/* 1 */ 0x00, 0x00, 0x00, //"000 000 000 000 000 000 000 000"
	/* 2 */ 0x00, 0x00, 0x00, //"000 000 000 000 000 000 000 000"
	/* 3 */ 0x00, 0x00, 0x00, //"000 000 000 000 000 000 000 000"
	/* 4 */ 0x00, 0x00, 0x00, //"000 000 000 000 000 000 000 000"
	/* frame 3 */
	/* 1 */ 0x00, 0x00, 0x00, //"000 000 000 000 000 000 000 000"
	/* 2 */ 0x00, 0x00, 0x00, //"000 000 000 000 000 000 000 000"
	/* 3 */ 0x00, 0x00, 0x00, //"000 000 000 000 000 000 000 000"
	/* 4 */ 0x00, 0x00, 0x00, //"000 000 000 000 000 000 000 000"
	/* frame 4 */
	/* 1 */ 0x00, 0x00, 0x00, //"000 000 000 000 000 000 000 000"
	/* 2 */ 0x00, 0x00, 0x00, //"000 000 000 000 000 000 000 000"
	/* 3 */ 0x00, 0x00, 0x00, //"000 000 000 000 000 000 000 000"
	/* 4 */ 0x00, 0x00, 0x00, //"000 000 000 000 000 000 000 000"
	/* frame 5 */
	/* 1 */ 0x00, 0x00, 0x00, //"000 000 000 000 000 000 000 000"
	/* 2 */ 0x00, 0x00, 0x00, //"000 000 000 000 000 000 000 000"
	/* 3 */ 0x00, 0x00, 0x00, //"000 000 000 000 000 000 000 000"
	/* 4 */ 0x00, 0x00, 0x00, //"000 000 000 000 000 000 000 000"
	/* frame 6 */
	/* 1 */ 0x00, 0x00, 0x00, //"000 000 000 000 000 000 000 000"
	/* 2 */ 0x00, 0x00, 0x00, //"000 000 000 000 000 000 000 000"
	/* 3 */ 0x00, 0x00, 0x00, //"000 000 000 000 000 000 000 000"
	/* 4 */ 0x00, 0x00, 0x00, //"000 000 000 000 000 000 000 000"
};/*}}}*/

/* PTZ HDA Rx pattern format */
const unsigned char pr1000_ptz_table_hda_stdformat_rx_pat_format[((3*4)*6)] =
{/*{{{*/
	/* frame 1 */
	/* 1 */ 0x00, 0x00, 0x00, //"000 000 000 000 000 000 000 000"
	/* 2 */ 0x00, 0x00, 0x00, //"000 000 000 000 000 000 000 000"
	/* 3 */ 0x00, 0x00, 0x00, //"000 000 000 000 000 000 000 000"
	/* 4 */ 0x00, 0x00, 0x00, //"000 000 000 000 000 000 000 000"
	/* frame 2 */
	/* 1 */ 0x00, 0x00, 0x00, //"000 000 000 000 000 000 000 000"
	/* 2 */ 0x00, 0x00, 0x00, //"000 000 000 000 000 000 000 000"
	/* 3 */ 0x00, 0x00, 0x00, //"000 000 000 000 000 000 000 000"
	/* 4 */ 0x00, 0x00, 0x00, //"000 000 000 000 000 000 000 000"
	/* frame 3 */
	/* 1 */ 0x00, 0x00, 0x00, //"000 000 000 000 000 000 000 000"
	/* 2 */ 0x00, 0x00, 0x00, //"000 000 000 000 000 000 000 000"
	/* 3 */ 0x00, 0x00, 0x00, //"000 000 000 000 000 000 000 000"
	/* 4 */ 0x00, 0x00, 0x00, //"000 000 000 000 000 000 000 000"
	/* frame 4 */
	/* 1 */ 0x00, 0x00, 0x00, //"000 000 000 000 000 000 000 000"
	/* 2 */ 0x00, 0x00, 0x00, //"000 000 000 000 000 000 000 000"
	/* 3 */ 0x00, 0x00, 0x00, //"000 000 000 000 000 000 000 000"
	/* 4 */ 0x00, 0x00, 0x00, //"000 000 000 000 000 000 000 000"
	/* frame 5 */
	/* 1 */ 0x00, 0x00, 0x00, //"000 000 000 000 000 000 000 000"
	/* 2 */ 0x00, 0x00, 0x00, //"000 000 000 000 000 000 000 000"
	/* 3 */ 0x00, 0x00, 0x00, //"000 000 000 000 000 000 000 000"
	/* 4 */ 0x00, 0x00, 0x00, //"000 000 000 000 000 000 000 000"
	/* frame 6 */
	/* 1 */ 0x00, 0x00, 0x00, //"000 000 000 000 000 000 000 000"
	/* 2 */ 0x00, 0x00, 0x00, //"000 000 000 000 000 000 000 000"
	/* 3 */ 0x00, 0x00, 0x00, //"000 000 000 000 000 000 000 000"
	/* 4 */ 0x00, 0x00, 0x00, //"000 000 000 000 000 000 000 000"
};/*}}}*/
/* PTZ HDA Rx pattern start format */
const unsigned char pr1000_ptz_table_hda_stdformat_rx_pat_start_format[(3)] =
{/*{{{*/
	0x00, 0x00, 0x00, //"000 000 000 000 000 000 000 000"
};/*}}}*/
/* PTZ HDA Rx pattern start data */
const unsigned char pr1000_ptz_table_hda_stdformat_rx_pat_start_data[(3)] =
{/*{{{*/
	0x00, 0x00, 0x00, //"000 000 000 000 000 000 000 000"
};/*}}}*/


//////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////// HDT ////////////////////////////////////////////////////
/*** HDT O_N Sel PATTERN (Old or New select) ***/
const _stPTZTxParam pr1000_ptz_txparam_hdt_stdformat_def[6] =
{
	//720p60
	{/*{{{*/
		0, //int mapChn;

			0x00, //uint8_t tx_field_type; //reg 4x20 b[1:0]
			0x02, //uint8_t tx_line_cnt; //reg 4x21 b[7:3]
			0x00, //uint8_t tx_hst_os; //reg 4x21 b[2:0]
			0x02, //uint8_t tx_hst; //reg 4x22 b[6:0]
			0x03FF64, //uint32_t tx_freq_first; //reg 4x23,4x24,4x25
			0x03FF64, //uint32_t tx_freq; //reg 4x26,4x27,4x28
			0x00A0, //uint16_t tx_hpst; //reg 4x29,4x2A b[12:0]
			0x28, //uint8_t tx_line_len; //reg 4x2B b[5:0]
			0x0A, //uint8_t tx_all_data_len; //reg 4x2C b[7:0]
	},/*}}}*/
	//720p50
	{/*{{{*/
		0, //int mapChn;

			0x00, //uint8_t tx_field_type; //reg 4x20 b[1:0]
			0x02, //uint8_t tx_line_cnt; //reg 4x21 b[7:3]
			0x00, //uint8_t tx_hst_os; //reg 4x21 b[2:0]
			0x02, //uint8_t tx_hst; //reg 4x22 b[6:0]
			0x03FF64, //uint32_t tx_freq_first; //reg 4x23,4x24,4x25
			0x03FF64, //uint32_t tx_freq; //reg 4x26,4x27,4x28
			0x00A0, //uint16_t tx_hpst; //reg 4x29,4x2A b[12:0]
			0x28, //uint8_t tx_line_len; //reg 4x2B b[5:0]
			0x0A, //uint8_t tx_all_data_len; //reg 4x2C b[7:0]
	},/*}}}*/
	//720p30
	{/*{{{*/
		0, //int mapChn;

			0x00, //uint8_t tx_field_type; //reg 4x20 b[1:0]
			0x02, //uint8_t tx_line_cnt; //reg 4x21 b[7:3]
			0x00, //uint8_t tx_hst_os; //reg 4x21 b[2:0]
			0x03, //uint8_t tx_hst; //reg 4x22 b[6:0]
			0x03FF64, //uint32_t tx_freq_first; //reg 4x23,4x24,4x25
			0x03FF64, //uint32_t tx_freq; //reg 4x26,4x27,4x28
			0x0364, //uint16_t tx_hpst; //reg 4x29,4x2A b[12:0]
			0x28, //uint8_t tx_line_len; //reg 4x2B b[5:0]
			0x0A, //uint8_t tx_all_data_len; //reg 4x2C b[7:0]
	},/*}}}*/
	//720p25
	{/*{{{*/
		0, //int mapChn;

			0x00, //uint8_t tx_field_type; //reg 4x20 b[1:0]
			0x02, //uint8_t tx_line_cnt; //reg 4x21 b[7:3]
			0x00, //uint8_t tx_hst_os; //reg 4x21 b[2:0]
			0x03, //uint8_t tx_hst; //reg 4x22 b[6:0]
			0x03FF64, //uint32_t tx_freq_first; //reg 4x23,4x24,4x25
			0x03FF64, //uint32_t tx_freq; //reg 4x26,4x27,4x28
			0x0364, //uint16_t tx_hpst; //reg 4x29,4x2A b[12:0]
			0x28, //uint8_t tx_line_len; //reg 4x2B b[5:0]
			0x0A, //uint8_t tx_all_data_len; //reg 4x2C b[7:0]
	},/*}}}*/
	//1080p30
	{/*{{{*/
		0, //int mapChn;

			0x00, //uint8_t tx_field_type; //reg 4x20 b[1:0]
			0x02, //uint8_t tx_line_cnt; //reg 4x21 b[7:3]
			0x00, //uint8_t tx_hst_os; //reg 4x21 b[2:0]
			0x02, //uint8_t tx_hst; //reg 4x22 b[6:0]
			0x03FF64, //uint32_t tx_freq_first; //reg 4x23,4x24,4x25
			0x03FF64, //uint32_t tx_freq; //reg 4x26,4x27,4x28
			0x0364, //uint16_t tx_hpst; //reg 4x29,4x2A b[12:0]
			0x28, //uint8_t tx_line_len; //reg 4x2B b[5:0]
			0x0A, //uint8_t tx_all_data_len; //reg 4x2C b[7:0]
	},/*}}}*/
	//1080p25
	{/*{{{*/
		0, //int mapChn;

			0x00, //uint8_t tx_field_type; //reg 4x20 b[1:0]
			0x02, //uint8_t tx_line_cnt; //reg 4x21 b[7:3]
			0x00, //uint8_t tx_hst_os; //reg 4x21 b[2:0]
			0x03, //uint8_t tx_hst; //reg 4x22 b[6:0]
			0x03FF64, //uint32_t tx_freq_first; //reg 4x23,4x24,4x25
			0x03FF64, //uint32_t tx_freq; //reg 4x26,4x27,4x28
			0x0364, //uint16_t tx_hpst; //reg 4x29,4x2A b[12:0]
			0x28, //uint8_t tx_line_len; //reg 4x2B b[5:0]
			0x0A, //uint8_t tx_all_data_len; //reg 4x2C b[7:0]
	},/*}}}*/

};

const _stPTZTxParam pr1000_ptz_txparam_hdt_new_stdformat_def[6] =
{
	//720p60
	{/*{{{*/
		0, //int mapChn;

			0x00, //uint8_t tx_field_type; //reg 4x20 b[1:0]
			0x02, //uint8_t tx_line_cnt; //reg 4x21 b[7:3]
			0x00, //uint8_t tx_hst_os; //reg 4x21 b[2:0]
			0x02, //uint8_t tx_hst; //reg 4x22 b[6:0]
			0x03FF64, //uint32_t tx_freq_first; //reg 4x23,4x24,4x25
			0x03FF64, //uint32_t tx_freq; //reg 4x26,4x27,4x28
			0x00A0, //uint16_t tx_hpst; //reg 4x29,4x2A b[12:0]
			0x28, //uint8_t tx_line_len; //reg 4x2B b[5:0]
			0x0A, //uint8_t tx_all_data_len; //reg 4x2C b[7:0]
	},/*}}}*/
	//720p50
	{/*{{{*/
		0, //int mapChn;

			0x00, //uint8_t tx_field_type; //reg 4x20 b[1:0]
			0x02, //uint8_t tx_line_cnt; //reg 4x21 b[7:3]
			0x00, //uint8_t tx_hst_os; //reg 4x21 b[2:0]
			0x02, //uint8_t tx_hst; //reg 4x22 b[6:0]
			0x03FF64, //uint32_t tx_freq_first; //reg 4x23,4x24,4x25
			0x03FF64, //uint32_t tx_freq; //reg 4x26,4x27,4x28
			0x00A0, //uint16_t tx_hpst; //reg 4x29,4x2A b[12:0]
			0x28, //uint8_t tx_line_len; //reg 4x2B b[5:0]
			0x0A, //uint8_t tx_all_data_len; //reg 4x2C b[7:0]
	},/*}}}*/
	//720p30
	{/*{{{*/
		0, //int mapChn;

			0x00, //uint8_t tx_field_type; //reg 4x20 b[1:0]
			0x02, //uint8_t tx_line_cnt; //reg 4x21 b[7:3]
			0x00, //uint8_t tx_hst_os; //reg 4x21 b[2:0]
			0x03, //uint8_t tx_hst; //reg 4x22 b[6:0]
			0x03FF64, //uint32_t tx_freq_first; //reg 4x23,4x24,4x25
			0x03FF64, //uint32_t tx_freq; //reg 4x26,4x27,4x28
			0x0364, //uint16_t tx_hpst; //reg 4x29,4x2A b[12:0]
			0x28, //uint8_t tx_line_len; //reg 4x2B b[5:0]
			0x0A, //uint8_t tx_all_data_len; //reg 4x2C b[7:0]
	},/*}}}*/
	//720p25
	{/*{{{*/
		0, //int mapChn;

			0x00, //uint8_t tx_field_type; //reg 4x20 b[1:0]
			0x02, //uint8_t tx_line_cnt; //reg 4x21 b[7:3]
			0x00, //uint8_t tx_hst_os; //reg 4x21 b[2:0]
			0x03, //uint8_t tx_hst; //reg 4x22 b[6:0]
			0x03FF64, //uint32_t tx_freq_first; //reg 4x23,4x24,4x25
			0x03FF64, //uint32_t tx_freq; //reg 4x26,4x27,4x28
			0x0364, //uint16_t tx_hpst; //reg 4x29,4x2A b[12:0]
			0x28, //uint8_t tx_line_len; //reg 4x2B b[5:0]
			0x0A, //uint8_t tx_all_data_len; //reg 4x2C b[7:0]
	},/*}}}*/
	//1080p30
	{/*{{{*/
		0, //int mapChn;

			0x00, //uint8_t tx_field_type; //reg 4x20 b[1:0]
			0x02, //uint8_t tx_line_cnt; //reg 4x21 b[7:3]
			0x00, //uint8_t tx_hst_os; //reg 4x21 b[2:0]
			0x02, //uint8_t tx_hst; //reg 4x22 b[6:0]
			0x03FF64, //uint32_t tx_freq_first; //reg 4x23,4x24,4x25
			0x03FF64, //uint32_t tx_freq; //reg 4x26,4x27,4x28
			0x0364, //uint16_t tx_hpst; //reg 4x29,4x2A b[12:0]
			0x28, //uint8_t tx_line_len; //reg 4x2B b[5:0]
			0x0A, //uint8_t tx_all_data_len; //reg 4x2C b[7:0]
	},/*}}}*/
	//1080p25
	{/*{{{*/
		0, //int mapChn;

			0x00, //uint8_t tx_field_type; //reg 4x20 b[1:0]
			0x02, //uint8_t tx_line_cnt; //reg 4x21 b[7:3]
			0x00, //uint8_t tx_hst_os; //reg 4x21 b[2:0]
			0x03, //uint8_t tx_hst; //reg 4x22 b[6:0]
			0x03FF64, //uint32_t tx_freq_first; //reg 4x23,4x24,4x25
			0x03FF64, //uint32_t tx_freq; //reg 4x26,4x27,4x28
			0x0364, //uint16_t tx_hpst; //reg 4x29,4x2A b[12:0]
			0x28, //uint8_t tx_line_len; //reg 4x2B b[5:0]
			0x0A, //uint8_t tx_all_data_len; //reg 4x2C b[7:0]
	},/*}}}*/

};


const _stPTZRxParam pr1000_ptz_rxparam_hdt_stdformat_def[6] =
{
	//720p60
	{/*{{{*/
		0, //int mapChn;

			0x00, //uint8_t rx_field_type; //reg 4x00 b[1:0]
			0x02, //uint8_t rx_line_cnt; //reg 4x01 b[7:3]
			0x00, //uint8_t rx_hst_os; //reg 4x01 b[2:0]
			0x02, //uint8_t rx_hst; //reg 4x02 b[6:0]
			0x03FF64, //uint32_t rx_freq_first; //reg 4x03,4x04,4x05
			0x03FF64, //uint32_t rx_freq; //reg 4x06,4x07,4x08
			0x04, //uint8_t rx_lpf_len; //reg 4x09 b[5:0]
			0x10, //uint8_t rx_h_pix_offset; //reg 4x0A b[7:0]
			0x28, //uint8_t rx_line_len; //reg 4x0B b[5:0]
			0x0A, //uint8_t rx_valid_cnt; //reg 4x0C b[7:0]
	},/*}}}*/
	//720p50
	{/*{{{*/
		0, //int mapChn;

			0x00, //uint8_t rx_field_type; //reg 4x00 b[1:0]
			0x02, //uint8_t rx_line_cnt; //reg 4x01 b[7:3]
			0x00, //uint8_t rx_hst_os; //reg 4x01 b[2:0]
			0x02, //uint8_t rx_hst; //reg 4x02 b[6:0]
			0x03FF64, //uint32_t rx_freq_first; //reg 4x03,4x04,4x05
			0x03FF64, //uint32_t rx_freq; //reg 4x06,4x07,4x08
			0x04, //uint8_t rx_lpf_len; //reg 4x09 b[5:0]
			0x10, //uint8_t rx_h_pix_offset; //reg 4x0A b[7:0]
			0x28, //uint8_t rx_line_len; //reg 4x0B b[5:0]
			0x0A, //uint8_t rx_valid_cnt; //reg 4x0C b[7:0]
	},/*}}}*/
	//720p30
	{/*{{{*/
		0, //int mapChn;

			0x00, //uint8_t rx_field_type; //reg 4x00 b[1:0]
			0x01, //uint8_t rx_line_cnt; //reg 4x01 b[7:3]
			0x00, //uint8_t rx_hst_os; //reg 4x01 b[2:0]
			0x03, //uint8_t rx_hst; //reg 4x02 b[6:0]
			0x03FF64, //uint32_t rx_freq_first; //reg 4x03,4x04,4x05
			0x03FF64, //uint32_t rx_freq; //reg 4x06,4x07,4x08
			0x04, //uint8_t rx_lpf_len; //reg 4x09 b[5:0]
			0x10, //uint8_t rx_h_pix_offset; //reg 4x0A b[7:0]
			0x28, //uint8_t rx_line_len; //reg 4x0B b[5:0]
			0x05, //uint8_t rx_valid_cnt; //reg 4x0C b[7:0]
	},/*}}}*/
	//720p25
	{/*{{{*/
		0, //int mapChn;

			0x00, //uint8_t rx_field_type; //reg 4x00 b[1:0]
			0x01, //uint8_t rx_line_cnt; //reg 4x01 b[7:3]
			0x00, //uint8_t rx_hst_os; //reg 4x01 b[2:0]
			0x03, //uint8_t rx_hst; //reg 4x02 b[6:0]
			0x03FF64, //uint32_t rx_freq_first; //reg 4x03,4x04,4x05
			0x03FF64, //uint32_t rx_freq; //reg 4x06,4x07,4x08
			0x04, //uint8_t rx_lpf_len; //reg 4x09 b[5:0]
			0x10, //uint8_t rx_h_pix_offset; //reg 4x0A b[7:0]
			0x28, //uint8_t rx_line_len; //reg 4x0B b[5:0]
			0x05, //uint8_t rx_valid_cnt; //reg 4x0C b[7:0]
	},/*}}}*/
	//1080p30
	{/*{{{*/
		0, //int mapChn;

			0x00, //uint8_t rx_field_type; //reg 4x00 b[1:0]
			0x02, //uint8_t rx_line_cnt; //reg 4x01 b[7:3]
			0x00, //uint8_t rx_hst_os; //reg 4x01 b[2:0]
			0x02, //uint8_t rx_hst; //reg 4x02 b[6:0]
			0x03FF64, //uint32_t rx_freq_first; //reg 4x03,4x04,4x05
			0x03FF64, //uint32_t rx_freq; //reg 4x06,4x07,4x08
			0x04, //uint8_t rx_lpf_len; //reg 4x09 b[5:0]
			0x10, //uint8_t rx_h_pix_offset; //reg 4x0A b[7:0]
			0x28, //uint8_t rx_line_len; //reg 4x0B b[5:0]
			0x0A, //uint8_t rx_valid_cnt; //reg 4x0C b[7:0]
	},/*}}}*/
	//1080p25
	{/*{{{*/
		0, //int mapChn;

			0x00, //uint8_t rx_field_type; //reg 4x00 b[1:0]
			0x02, //uint8_t rx_line_cnt; //reg 4x01 b[7:3]
			0x00, //uint8_t rx_hst_os; //reg 4x01 b[2:0]
			0x03, //uint8_t rx_hst; //reg 4x02 b[6:0]
			0x03FF64, //uint32_t rx_freq_first; //reg 4x03,4x04,4x05
			0x03FF64, //uint32_t rx_freq; //reg 4x06,4x07,4x08
			0x04, //uint8_t rx_lpf_len; //reg 4x09 b[5:0]
			0x10, //uint8_t rx_h_pix_offset; //reg 4x0A b[7:0]
			0x28, //uint8_t rx_line_len; //reg 4x0B b[5:0]
			0x0A, //uint8_t rx_valid_cnt; //reg 4x0C b[7:0]
	},/*}}}*/

};

const _stPTZRxParam pr1000_ptz_rxparam_hdt_new_stdformat_def[6] =
{
	//720p60
	{/*{{{*/
		0, //int mapChn;

			0x00, //uint8_t rx_field_type; //reg 4x00 b[1:0]
			0x02, //uint8_t rx_line_cnt; //reg 4x01 b[7:3]
			0x00, //uint8_t rx_hst_os; //reg 4x01 b[2:0]
			0x02, //uint8_t rx_hst; //reg 4x02 b[6:0]
			0x03FF64, //uint32_t rx_freq_first; //reg 4x03,4x04,4x05
			0x03FF64, //uint32_t rx_freq; //reg 4x06,4x07,4x08
			0x04, //uint8_t rx_lpf_len; //reg 4x09 b[5:0]
			0x10, //uint8_t rx_h_pix_offset; //reg 4x0A b[7:0]
			0x28, //uint8_t rx_line_len; //reg 4x0B b[5:0]
			0x0A, //uint8_t rx_valid_cnt; //reg 4x0C b[7:0]
	},/*}}}*/
	//720p50
	{/*{{{*/
		0, //int mapChn;

			0x00, //uint8_t rx_field_type; //reg 4x00 b[1:0]
			0x02, //uint8_t rx_line_cnt; //reg 4x01 b[7:3]
			0x00, //uint8_t rx_hst_os; //reg 4x01 b[2:0]
			0x02, //uint8_t rx_hst; //reg 4x02 b[6:0]
			0x03FF64, //uint32_t rx_freq_first; //reg 4x03,4x04,4x05
			0x03FF64, //uint32_t rx_freq; //reg 4x06,4x07,4x08
			0x04, //uint8_t rx_lpf_len; //reg 4x09 b[5:0]
			0x10, //uint8_t rx_h_pix_offset; //reg 4x0A b[7:0]
			0x28, //uint8_t rx_line_len; //reg 4x0B b[5:0]
			0x0A, //uint8_t rx_valid_cnt; //reg 4x0C b[7:0]
	},/*}}}*/
	//720p30
	{/*{{{*/
		0, //int mapChn;

			0x00, //uint8_t rx_field_type; //reg 4x00 b[1:0]
			0x01, //uint8_t rx_line_cnt; //reg 4x01 b[7:3]
			0x00, //uint8_t rx_hst_os; //reg 4x01 b[2:0]
			0x03, //uint8_t rx_hst; //reg 4x02 b[6:0]
			0x03FF64, //uint32_t rx_freq_first; //reg 4x03,4x04,4x05
			0x03FF64, //uint32_t rx_freq; //reg 4x06,4x07,4x08
			0x04, //uint8_t rx_lpf_len; //reg 4x09 b[5:0]
			0x10, //uint8_t rx_h_pix_offset; //reg 4x0A b[7:0]
			0x28, //uint8_t rx_line_len; //reg 4x0B b[5:0]
			0x05, //uint8_t rx_valid_cnt; //reg 4x0C b[7:0]
	},/*}}}*/
	//720p25
	{/*{{{*/
		0, //int mapChn;

			0x00, //uint8_t rx_field_type; //reg 4x00 b[1:0]
			0x01, //uint8_t rx_line_cnt; //reg 4x01 b[7:3]
			0x00, //uint8_t rx_hst_os; //reg 4x01 b[2:0]
			0x03, //uint8_t rx_hst; //reg 4x02 b[6:0]
			0x03FF64, //uint32_t rx_freq_first; //reg 4x03,4x04,4x05
			0x03FF64, //uint32_t rx_freq; //reg 4x06,4x07,4x08
			0x04, //uint8_t rx_lpf_len; //reg 4x09 b[5:0]
			0x10, //uint8_t rx_h_pix_offset; //reg 4x0A b[7:0]
			0x28, //uint8_t rx_line_len; //reg 4x0B b[5:0]
			0x05, //uint8_t rx_valid_cnt; //reg 4x0C b[7:0]
	},/*}}}*/
	//1080p30
	{/*{{{*/
		0, //int mapChn;

			0x00, //uint8_t rx_field_type; //reg 4x00 b[1:0]
			0x02, //uint8_t rx_line_cnt; //reg 4x01 b[7:3]
			0x00, //uint8_t rx_hst_os; //reg 4x01 b[2:0]
			0x02, //uint8_t rx_hst; //reg 4x02 b[6:0]
			0x03FF64, //uint32_t rx_freq_first; //reg 4x03,4x04,4x05
			0x03FF64, //uint32_t rx_freq; //reg 4x06,4x07,4x08
			0x04, //uint8_t rx_lpf_len; //reg 4x09 b[5:0]
			0x10, //uint8_t rx_h_pix_offset; //reg 4x0A b[7:0]
			0x28, //uint8_t rx_line_len; //reg 4x0B b[5:0]
			0x0A, //uint8_t rx_valid_cnt; //reg 4x0C b[7:0]
	},/*}}}*/
	//1080p25
	{/*{{{*/
		0, //int mapChn;

			0x00, //uint8_t rx_field_type; //reg 4x00 b[1:0]
			0x02, //uint8_t rx_line_cnt; //reg 4x01 b[7:3]
			0x00, //uint8_t rx_hst_os; //reg 4x01 b[2:0]
			0x03, //uint8_t rx_hst; //reg 4x02 b[6:0]
			0x03FF64, //uint32_t rx_freq_first; //reg 4x03,4x04,4x05
			0x03FF64, //uint32_t rx_freq; //reg 4x06,4x07,4x08
			0x04, //uint8_t rx_lpf_len; //reg 4x09 b[5:0]
			0x10, //uint8_t rx_h_pix_offset; //reg 4x0A b[7:0]
			0x28, //uint8_t rx_line_len; //reg 4x0B b[5:0]
			0x0A, //uint8_t rx_valid_cnt; //reg 4x0C b[7:0]
	},/*}}}*/

};


/* PTZ HDT Tx pattern format */
const unsigned char pr1000_ptz_table_hdt_stdformat_tx_pat_format[((5)*2)] = 
{/*{{{*/
	/* frame 1 */
	/* 1 */ 0xFF, 0x00, 0x00, 0x00, 0x00, //"1111 1111 0000 0000 0000 0000 0000 0000 0000 0000"
	/* 2 */ 0xFF, 0x00, 0x00, 0x00, 0x00, //"1111 1111 0000 0000 0000 0000 0000 0000 0000 0000"
};/*}}}*/
/* PTZ HDT Tx pattern data */
const unsigned char pr1000_ptz_table_hdt_stdformat_tx_pat_data[((5)*2)] = 
{/*{{{*/
	/* frame 1 */
	/* 1 */ 0xFF, 0x00, 0x00, 0x00, 0x00, //"1111 1111 xxxx xxxx xxxx xxxx xxxx xxxx xxxx xxxx"
	/* 2 */ 0xFF, 0x00, 0x00, 0x00, 0x00, //"1111 1111 xxxx xxxx xxxx xxxx xxxx xxxx xxxx xxxx"
};/*}}}*/

/* PTZ HDT Rx pattern format */
const unsigned char pr1000_ptz_table_hdt_stdformat_rx_pat_format[(5)] = 
{/*{{{*/
	/* frame 1 */
	/* 1 */ 0x00, 0x00, 0x00, 0x00, 0x00, //"0000 0000 0000 0000 0000 0000 0000 0000 0000 0000"
};/*}}}*/
/* PTZ HDT Rx pattern start format */
const unsigned char pr1000_ptz_table_hdt_stdformat_rx_pat_start_format[(5)] = 
{/*{{{*/
	0x00, 0x00, 0x00, 0x00, 0x00, //"0000 0000 0000 0000 0000 0000 0000 0000 0000 0000"
};/*}}}*/
/* PTZ HDT Rx pattern start data */
const unsigned char pr1000_ptz_table_hdt_stdformat_rx_pat_start_data[(5)] = 
{/*{{{*/
	0x00, 0x00, 0x00, 0x00, 0x00, //"0xxx xxxx xxxx xxxx xxxx xxxx xxxx xxxx xxxx xxxx"
};/*}}}*/

/*** HDT_CHGOLD PATTERN (Change old format) ***/
/* PTZ HDT_CHGOLD Tx pattern format */
const unsigned char pr1000_ptz_table_hdt_chgold_tx_pat_format[((5)*2)] = 
{/*{{{*/
	/* frame 1 */
	/* 1 */ 0xFF, 0x00, 0x00, 0x00, 0x00, //"1111 1111 0000 0000 0000 0000 0000 0000 0000 0000"
	/* 2 */ 0xFF, 0x00, 0x00, 0x00, 0x00, //"1111 1111 0000 0000 0000 0000 0000 0000 0000 0000"
};/*}}}*/
/* PTZ HDT_CHGOLD Tx pattern data */
const unsigned char pr1000_ptz_table_hdt_chgold_tx_pat_data[((5)*2)] = 
{/*{{{*/
	/* frame 1 */
	/* 1 */ 0xED, 0x00, 0x00, 0x00, 0x00, //"1110 1101 xxxx xxxx xxxx xxxx xxxx xxxx xxxx xxxx"
	/* 2 */ 0xC0, 0x00, 0x00, 0x00, 0x00, //"1100 0000 xxxx xxxx xxxx xxxx xxxx xxxx xxxx xxxx"
};/*}}}*/

/* PTZ HDT_CHGOLD Rx pattern format */
const unsigned char pr1000_ptz_table_hdt_chgold_rx_pat_format[((5)*2)] = 
{/*{{{*/
	/* frame 1 */
	/* 1 */ 0xFF, 0x00, 0x00, 0x00, 0x00, //"1111 1111 0000 0000 0000 0000 0000 0000 0000 0000"
	/* 2 */ 0xFF, 0x00, 0x00, 0x00, 0x00, //"1111 1111 0000 0000 0000 0000 0000 0000 0000 0000"
};/*}}}*/
/* PTZ HDT_CHGOLD Rx pattern start format */
const unsigned char pr1000_ptz_table_hdt_chgold_rx_pat_start_format[(5)] = 
{/*{{{*/
	0xFF, 0x00, 0x00, 0x00, 0x00, //"1111 1111 0000 0000 0000 0000 0000 0000 0000 0000"
};/*}}}*/
/* PTZ HDT_CHGOLD Rx pattern start data */
const unsigned char pr1000_ptz_table_hdt_chgold_rx_pat_start_data[(5)] = 
{/*{{{*/
	0xFF, 0x00, 0x00, 0x00, 0x00, //"1111 1111 xxxx xxxx xxxx xxxx xxxx xxxx xxxx xxxx"
};/*}}}*/

//////////////////////////////////////////////////////////////////////////////////////////
