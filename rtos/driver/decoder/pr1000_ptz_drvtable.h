#ifndef __PR1000_PTZ_DRVTABLE_H__
#define __PR1000_PTZ_DRVTABLE_H__

///////////////////////////////// SD //////////////////////////////////////////////////
#define PR1000_PTZ_SD_TX_DATA_BITSWAP	(0)
#define PR1000_PTZ_SD_RX_DATA_BITSWAP	(0)
//////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////// CVI //////////////////////////////////////////////////
#define PR1000_PTZ_CVI_TX_DATA_BITSWAP		(1)
#define PR1000_PTZ_CVI_RX_DATA_BITSWAP		(1)
//////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////// HDA //////////////////////////////////////////////////
#define PR1000_PTZ_HDA_TX_DATA_BITSWAP_1080P		(0)
#define PR1000_PTZ_HDA_TX_DATA_BITSWAP_720P		(1)
#define PR1000_PTZ_HDA_RX_DATA_BITSWAP			(0)

extern const _stPTZTxParam pr1000_ptz_txparam_hda_stdformat_def[6];
extern const _stPTZRxParam pr1000_ptz_rxparam_hda_stdformat_def[6];

/*** HDA real HDA or noEQ CVI Sel PATTERN ***/
#define PR1000_PTZ_HDA_STDFORMAT_TX_DATA_BITSWAP		(0)
extern const unsigned char pr1000_ptz_table_hda_stdformat_tx_pat_format[((3*4)*6)];
extern const unsigned char pr1000_ptz_table_hda_stdformat_tx_pat_data[((3*4)*6)];
#define PR1000_PTZ_HDA_STDFORMAT_RX_DATA_BITSWAP		(0)
extern const unsigned char pr1000_ptz_table_hda_stdformat_rx_pat_format[((3*4)*6)];
extern const unsigned char pr1000_ptz_table_hda_stdformat_rx_pat_start_format[(3)];
extern const unsigned char pr1000_ptz_table_hda_stdformat_rx_pat_start_data[(3)];

//////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////// HDT //////////////////////////////////////////////////
#define PR1000_PTZ_HDT_TX_DATA_BITSWAP		(0)
#define PR1000_PTZ_HDT_RX_DATA_BITSWAP		(0)
#define PR1000_PTZ_HDT_NEW_TX_DATA_BITSWAP		(0)
#define PR1000_PTZ_HDT_NEW_RX_DATA_BITSWAP		(0)

/*** HDT O_N Sel PATTERN (Old or New select) ***/
extern const _stPTZTxParam pr1000_ptz_txparam_hdt_stdformat_def[6];
extern const _stPTZTxParam pr1000_ptz_txparam_hdt_new_stdformat_def[6];
extern const _stPTZRxParam pr1000_ptz_rxparam_hdt_stdformat_def[6];
extern const _stPTZRxParam pr1000_ptz_rxparam_hdt_new_stdformat_def[6];
#define PR1000_PTZ_HDT_STDFORMAT_TX_DATA_BITSWAP		(0)
extern const unsigned char pr1000_ptz_table_hdt_stdformat_tx_pat_format[((5)*2)];
extern const unsigned char pr1000_ptz_table_hdt_stdformat_tx_pat_data[((5)*2)];
#define PR1000_PTZ_HDT_STDFORMAT_RX_DATA_BITSWAP		(0)
extern const unsigned char pr1000_ptz_table_hdt_stdformat_rx_pat_format[(5)];
extern const unsigned char pr1000_ptz_table_hdt_stdformat_rx_pat_start_format[(5)];
extern const unsigned char pr1000_ptz_table_hdt_stdformat_rx_pat_start_data[(5)];

/*** HDT_CHGOLD PATTERN ***/
#define PR1000_PTZ_HDT_CHGOLD_TX_DATA_BITSWAP		(0)
extern const unsigned char pr1000_ptz_table_hdt_chgold_tx_pat_format[((5)*2)];
extern const unsigned char pr1000_ptz_table_hdt_chgold_tx_pat_data[((5)*2)];
#define PR1000_PTZ_HDT_CHGOLD_RX_DATA_BITSWAP		(0)
extern const unsigned char pr1000_ptz_table_hdt_chgold_rx_pat_format[((5)*2)];
extern const unsigned char pr1000_ptz_table_hdt_chgold_rx_pat_start_format[(5)];
extern const unsigned char pr1000_ptz_table_hdt_chgold_rx_pat_start_data[(5)];
//////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////// PVI //////////////////////////////////////////////////
#define PR1000_PTZ_PVI_TX_DATA_BITSWAP		(0)
#define PR1000_PTZ_PVI_RX_DATA_BITSWAP		(0)
//////////////////////////////////////////////////////////////////////////////////////////
#endif /* __PR1000_PTZ_DRVTABLE_H__ */
