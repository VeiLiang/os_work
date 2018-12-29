#ifndef __PR1000_TABLE_H__
#define __PR1000_TABLE_H__

#ifdef __LINUX_SYSTEM__
/* __KERNEL__ */
#ifdef __KERNEL__
#include "pr1000_drvcommon.h"
#include "pr1000_user_config.h"

//// register address ////////////////////////////////////////////////////////////////////
#define PR1000_REG_PAGE_COMMON				(0)
#define PR1000_REG_PAGE_VDEC1				(1) //page 1,2	
#define PR1000_REG_PAGE_VDEC2				(2) //page 1,2	
#define PR1000_REG_PAGE_AUD				(3)	
#define PR1000_REG_PAGE_PTZ				(4)	
#define PR1000_REG_PAGE_VEVENT				(5)	

#define PR1000_REG_ADDR_PTZ_RX_EN			(0x00)	
#define PR1000_REG_ADDR_PTZ_TX_EN			(0x20)	
#define PR1000_REG_ADDR_PTZ_FIFO_WR_INIT		(0x10)	
#define PR1000_REG_ADDR_PTZ_FIFO_WR_DATA		(0x11)	
#define PR1000_REG_ADDR_PTZ_FIFO_WR_SIZE		(0x12)	
#define PR1000_REG_ADDR_PTZ_FIFO_WR_ADDR		(0x13)	
#define PR1000_REG_ADDR_PTZ_FIFO_RD_INIT		(0x14)	
#define PR1000_REG_ADDR_PTZ_FIFO_RD_DATA		(0x16)	
#define PR1000_REG_ADDR_PTZ_FIFO_RD_SIZE		(0x15)	
#define PR1000_REG_ADDR_PTZ_FIFO_RD_ADDR		(0x17)	
#define PR1000_REG_ADDR_PTZ_TX_GROUP_CNT		(0x2E)	
#else
#include <stdint.h>
#endif /* __KERNEL__ */

#else //#ifdef __LINUX_SYSTEM__

#include "pr1000_user_config.h"
#include "pr1000_drvcommon.h"

//// register address ////////////////////////////////////////////////////////////////////
#define PR1000_REG_PAGE_COMMON				(0)
#define PR1000_REG_PAGE_VDEC1				(1) //page 1,2	
#define PR1000_REG_PAGE_VDEC2				(2) //page 1,2	
#define PR1000_REG_PAGE_AUD				(3)	
#define PR1000_REG_PAGE_PTZ				(4)	
#define PR1000_REG_PAGE_VEVENT				(5)	

#define PR1000_REG_ADDR_PTZ_RX_EN			(0x00)	
#define PR1000_REG_ADDR_PTZ_TX_EN			(0x20)	
#define PR1000_REG_ADDR_PTZ_FIFO_WR_INIT		(0x10)	
#define PR1000_REG_ADDR_PTZ_FIFO_WR_DATA		(0x11)	
#define PR1000_REG_ADDR_PTZ_FIFO_WR_SIZE		(0x12)	
#define PR1000_REG_ADDR_PTZ_FIFO_WR_ADDR		(0x13)	
#define PR1000_REG_ADDR_PTZ_FIFO_RD_INIT		(0x14)	
#define PR1000_REG_ADDR_PTZ_FIFO_RD_DATA		(0x16)	
#define PR1000_REG_ADDR_PTZ_FIFO_RD_SIZE		(0x15)	
#define PR1000_REG_ADDR_PTZ_FIFO_RD_ADDR		(0x17)	
#define PR1000_REG_ADDR_PTZ_TX_GROUP_CNT		(0x2E)	

#endif // __LINUX_SYSTEM__
/************************************************************/

////////////////////////////////////////////////////////////////////////
#define DEF_PR1000_MAX_CHN	(4)

enum _pr1000_table_format {
	pr1000_format_SD720 = 0,
	pr1000_format_SD960,
#ifndef DONT_SUPPORT_STD_PVI
	pr1000_format_PVI,
#endif // DONT_SUPPORT_STD_PVI
	pr1000_format_HDA,
	pr1000_format_CVI,
	pr1000_format_HDT,
	pr1000_format_HDT_NEW,
	max_pr1000_format
};

enum _pr1000_table_inresol {
	pr1000_inresol_ntsc = 0,
	pr1000_inresol_pal,

	pr1000_inresol_1280x720p60,
	pr1000_inresol_1280x720p50,
	pr1000_inresol_1280x720p30,
	pr1000_inresol_1280x720p25,
	pr1000_inresol_1920x1080p30,
	pr1000_inresol_1920x1080p25,

	max_pr1000_inresol
};

enum _pr1000_table_outresol {
	pr1000_outresol_720x480i60 = 0,
	pr1000_outresol_720x576i50,
	pr1000_outresol_960x480i60,
	pr1000_outresol_960x576i50,

	pr1000_outresol_1280x720p60,
	pr1000_outresol_1280x720p50,
	pr1000_outresol_1280x720p30,
	pr1000_outresol_1280x720p25,
	pr1000_outresol_1920x1080p30,
	pr1000_outresol_1920x1080p25,

	pr1000_outresol_1280x720p60c, //640x720p60
	pr1000_outresol_1280x720p50c, //640x720p50
	pr1000_outresol_1280x720p30c, //640x720p30
	pr1000_outresol_1280x720p25c, //640x720p25
	pr1000_outresol_1920x1080p30c, //960x1080p30
	pr1000_outresol_1920x1080p25c, //960x1080p25
	pr1000_outresol_1920x1080p30s, //1280x720p30
	pr1000_outresol_1920x1080p25s, //1280x720p25

	max_pr1000_outresol
};

enum _pr1000_mux_chdef {
	pr1000_mux_chdef_ch0_y = 0,
	pr1000_mux_chdef_ch0_c,
	pr1000_mux_chdef_ch1_y,
	pr1000_mux_chdef_ch1_c,
	pr1000_mux_chdef_ch2_y,
	pr1000_mux_chdef_ch2_c,
	pr1000_mux_chdef_ch3_y,
	pr1000_mux_chdef_ch3_c,
	max_pr1000_mux_chdef
};

enum _pr1000_aud_format {
	pr1000_aud_2scomplete = 0,
	pr1000_aud_straight,
	pr1000_aud_g711u,
	pr1000_aud_g711a,
	max_pr1000_aud_format
};
enum _pr1000_aud_sample_rate {
	pr1000_aud_samrate_8k = 0,
	pr1000_aud_samrate_16k,
	pr1000_aud_samrate_32k,
	pr1000_aud_samrate_48k,
	max_pr1000_aud_samrate
};
enum _pr1000_aud_bit_rate {
	pr1000_aud_bitrate_16fs = 0,
	pr1000_aud_bitrate_32fs,
	pr1000_aud_bitrate_64fs,
	pr1000_aud_bitrate_128fs,
	pr1000_aud_bitrate_256fs,
	pr1000_aud_bitrate_320fs,
	pr1000_aud_bitrate_384fs,
	max_pr1000_aud_bitrate
};

//////////////////////////////////////////////////////////////////////////////////////////////////////
/* PR1000 app drv common function */
//////////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef __LINUX_SYSTEM__
/* __KERNEL__ */
#ifdef __KERNEL__
typedef struct
{
	uint8_t addr;
	uint16_t pData; //page+Data
}_PR1000_REG_TABLE_COMMON;

typedef struct
{
	uint8_t addr;
	uint16_t pData[pr1000_outresol_960x576i50+1];
}_PR1000_REG_TABLE_VDEC_SD;

typedef struct
{
	uint8_t addr;
	uint16_t pData[pr1000_outresol_1920x1080p25-pr1000_outresol_1280x720p60+1];
}_PR1000_REG_TABLE_VDEC_HD;

typedef struct
{
	uint8_t addr;
	uint16_t pData[pr1000_outresol_1920x1080p25s-pr1000_outresol_1280x720p60c+1];
}_PR1000_REG_TABLE_VDEC_HD_EXTEND;

typedef struct
{
	uint8_t addr;
	uint16_t pData[pr1000_outresol_960x576i50+1];
}_PR1000_REG_TABLE_PTZ_SD;

typedef struct
{
	uint8_t addr;
	uint16_t pData[(pr1000_outresol_1920x1080p25-pr1000_outresol_1280x720p60)+1];
}_PR1000_REG_TABLE_PTZ_HD;

typedef struct
{
	uint8_t addr;
	uint16_t pData[max_pr1000_outresol];
}_PR1000_REG_TABLE_VEVENT;
typedef struct
{
	uint8_t cellCntX;
	uint8_t cellCntY;
	uint8_t cellWidth;
	uint8_t cellHeight;
	uint16_t cellVStartOffset;
	uint16_t cellHStartOffset;
}_stMaskCellAttr;

typedef struct
{
	uint16_t distance;
	uint16_t pData[pr1000_inresol_1280x720p60][3]; //rangeMin, rangeMax, factor;
}_PR1000_CEQ_TABLE_EST_SD;
typedef struct
{
	uint16_t distance;
	uint16_t pData[max_pr1000_inresol-pr1000_inresol_1280x720p60][3]; //rangeMin, rangeMax, factor;
}_PR1000_CEQ_TABLE_EST_HD;

//////////////////////////////////////////////////////////////////////////////////////////////////////
#define DEF_PR1000_SIZE_CEQ_INFO_TABLE_REG		(0x20)
#define PR1000_OFFSETADDR_CEQ_INFO_CH(ch)		((ch%DEF_PR1000_MAX_CHN)*DEF_PR1000_SIZE_CEQ_INFO_TABLE_REG)

extern const _PR1000_REG_TABLE_COMMON pr1000_reg_table_common[1+(4*4*7)+1+1];
#define DEF_PR1000_SIZE_VDEC_0PAGE_TABLE_REG		(0x20)
#define PR1000_OFFSETADDR_VDEC_0PAGE_CH(ch)		(((ch%DEF_PR1000_MAX_CHN)*DEF_PR1000_SIZE_VDEC_0PAGE_TABLE_REG))
#define PR1000_VDEC_PAGE(ch)				(((ch%DEF_PR1000_MAX_CHN)<2)?PR1000_REG_PAGE_VDEC1:PR1000_REG_PAGE_VDEC2)
#define DEF_PR1000_SIZE_VDEC_1PAGE_TABLE_REG		(0x80)
#define PR1000_OFFSETADDR_VDEC_1_2PAGE_CH(ch)		(((ch%2)*DEF_PR1000_SIZE_VDEC_1PAGE_TABLE_REG))
extern const _PR1000_REG_TABLE_COMMON pr1000_reg_table_vdec_common[16+4+5+11+6+16+1];
extern const _PR1000_REG_TABLE_VDEC_SD pr1000_reg_table_vdec_SD[4+16+12+12+16+5+1+1];
#ifndef DONT_SUPPORT_STD_PVI
extern const _PR1000_REG_TABLE_VDEC_HD pr1000_reg_table_vdec_PVI[4+16+12+11+16+5+1+1];
#ifndef DONT_SUPPORT_VDRESOL_EXTEND
extern const _PR1000_REG_TABLE_VDEC_HD_EXTEND pr1000_reg_table_vdec_PVI_extend[4+16+12+11+16+5+1+1];
#endif // DONT_SUPPORT_VDRESOL_EXTEND
#endif // DONT_SUPPORT_STD_PVI
extern const _PR1000_REG_TABLE_VDEC_HD pr1000_reg_table_vdec_HDA[4+16+12+11+16+5+1+1];
#ifndef DONT_SUPPORT_VDRESOL_EXTEND
extern const _PR1000_REG_TABLE_VDEC_HD_EXTEND pr1000_reg_table_vdec_HDA_extend[4+16+12+11+16+5+1+1];
#endif // DONT_SUPPORT_VDRESOL_EXTEND
extern const _PR1000_REG_TABLE_VDEC_HD pr1000_reg_table_vdec_CVI[4+16+12+11+16+5+1+1];
#ifndef DONT_SUPPORT_VDRESOL_EXTEND
extern const _PR1000_REG_TABLE_VDEC_HD_EXTEND pr1000_reg_table_vdec_CVI_extend[4+16+12+11+16+5+1+1];
#endif // DONT_SUPPORT_VDRESOL_EXTEND
extern const _PR1000_REG_TABLE_VDEC_HD pr1000_reg_table_vdec_HDT[4+16+12+11+16+5+1+1];
#ifndef DONT_SUPPORT_VDRESOL_EXTEND
extern const _PR1000_REG_TABLE_VDEC_HD_EXTEND pr1000_reg_table_vdec_HDT_extend[4+16+12+11+16+5+1+1];
#endif // DONT_SUPPORT_VDRESOL_EXTEND
extern const _PR1000_REG_TABLE_VDEC_HD pr1000_reg_table_vdec_HDT_NEW[4+16+12+11+16+5+1+1];
#ifndef DONT_SUPPORT_VDRESOL_EXTEND
extern const _PR1000_REG_TABLE_VDEC_HD_EXTEND pr1000_reg_table_vdec_HDT_NEW_extend[4+16+12+11+16+5+1+1];
#endif // DONT_SUPPORT_VDRESOL_EXTEND

#define PR1000_OFFSETADDR_PTZ_CH(ch)		((ch%DEF_PR1000_MAX_CHN)*0x40) //0x40 register channel offset
extern const _PR1000_REG_TABLE_COMMON pr1000_reg_table_ptz_SD_common[8+6+12];
extern const _PR1000_REG_TABLE_PTZ_SD pr1000_reg_table_ptz_SD[6+2+5];
#ifndef DONT_SUPPORT_STD_PVI
extern const _PR1000_REG_TABLE_COMMON pr1000_reg_table_ptz_PVI_common[13+5+12+13];
extern const _PR1000_REG_TABLE_PTZ_HD pr1000_reg_table_ptz_PVI[1+3+3+1];
#endif // DONT_SUPPORT_STD_PVI
extern const _PR1000_REG_TABLE_COMMON pr1000_reg_table_ptz_HDA_common[13+5+12+13];
extern const _PR1000_REG_TABLE_PTZ_HD pr1000_reg_table_ptz_HDA[1+3+3+1];
extern const _PR1000_REG_TABLE_COMMON pr1000_reg_table_ptz_CVI_common[13+5+11+13];
extern const _PR1000_REG_TABLE_PTZ_HD pr1000_reg_table_ptz_CVI[1+3+4+1];
extern const _PR1000_REG_TABLE_COMMON pr1000_reg_table_ptz_HDT_common[13+5+12+1];
extern const _PR1000_REG_TABLE_PTZ_HD pr1000_reg_table_ptz_HDT[1+3+3+1];
extern const _PR1000_REG_TABLE_COMMON pr1000_reg_table_ptz_HDT_NEW_common[13+5+13+1];
extern const _PR1000_REG_TABLE_PTZ_HD pr1000_reg_table_ptz_HDT_NEW[1+3+3+1];

#define PR1000_OFFSETADDR_VEVENT_CH(ch)		((ch%DEF_PR1000_MAX_CHN)*0x40) //0x40 register channel offset
extern const _PR1000_REG_TABLE_VEVENT pr1000_reg_table_vevent[14+13+10];
extern const _stMaskCellAttr pr1000_mask_attr_table_vevent[max_pr1000_outresol];

#endif /* __KERNEL__ */

#else //#ifdef __LINUX_SYSTEM__

typedef struct
{
	uint8_t addr;
	uint16_t pData; //page+Data
}_PR1000_REG_TABLE_COMMON;

typedef struct
{
	uint8_t addr;
	uint16_t pData[pr1000_outresol_960x576i50+1];
}_PR1000_REG_TABLE_VDEC_SD;

typedef struct
{
	uint8_t addr;
	uint16_t pData[max_pr1000_outresol-pr1000_outresol_1280x720p60];
}_PR1000_REG_TABLE_VDEC_HD;

typedef struct
{
	uint8_t addr;
	uint16_t pData[pr1000_outresol_1920x1080p25s-pr1000_outresol_1280x720p60c+1];
}_PR1000_REG_TABLE_VDEC_HD_EXTEND;

typedef struct
{
	uint8_t addr;
	uint16_t pData[pr1000_outresol_960x576i50+1];
}_PR1000_REG_TABLE_PTZ_SD;

typedef struct
{
	uint8_t addr;
	uint16_t pData[(pr1000_outresol_1920x1080p25-pr1000_outresol_1280x720p60)+1];
}_PR1000_REG_TABLE_PTZ_HD;

typedef struct
{
	uint8_t addr;
	uint16_t pData[max_pr1000_outresol];
}_PR1000_REG_TABLE_VEVENT;
typedef struct
{
	uint8_t cellCntX;
	uint8_t cellCntY;
	uint8_t cellWidth;
	uint8_t cellHeight;
	uint16_t cellVStartOffset;
	uint16_t cellHStartOffset;
}_stMaskCellAttr;

typedef struct
{
	uint16_t distance;
	uint16_t pData[pr1000_inresol_1280x720p60][3]; //rangeMin, rangeMax, factor;
}_PR1000_CEQ_TABLE_EST_SD;
typedef struct
{
	uint16_t distance;
	uint16_t pData[max_pr1000_inresol-pr1000_inresol_1280x720p60][3]; //rangeMin, rangeMax, factor;
}_PR1000_CEQ_TABLE_EST_HD;

//////////////////////////////////////////////////////////////////////////////////////////////////////
#define DEF_PR1000_SIZE_CEQ_INFO_TABLE_REG		(0x20)
#define PR1000_OFFSETADDR_CEQ_INFO_CH(ch)		((ch%DEF_PR1000_MAX_CHN)*DEF_PR1000_SIZE_CEQ_INFO_TABLE_REG)

extern const _PR1000_REG_TABLE_COMMON pr1000_reg_table_common[1+(4*4*7)+1+1];
#define DEF_PR1000_SIZE_VDEC_0PAGE_TABLE_REG		(0x20)
#define PR1000_OFFSETADDR_VDEC_0PAGE_CH(ch)		(((ch%DEF_PR1000_MAX_CHN)*DEF_PR1000_SIZE_VDEC_0PAGE_TABLE_REG))
#define PR1000_VDEC_PAGE(ch)				(((ch%DEF_PR1000_MAX_CHN)<2)?PR1000_REG_PAGE_VDEC1:PR1000_REG_PAGE_VDEC2)
#define DEF_PR1000_SIZE_VDEC_1PAGE_TABLE_REG		(0x80)
#define PR1000_OFFSETADDR_VDEC_1_2PAGE_CH(ch)		(((ch%2)*DEF_PR1000_SIZE_VDEC_1PAGE_TABLE_REG))
extern const _PR1000_REG_TABLE_COMMON pr1000_reg_table_vdec_common[16+4+5+11+6+16+1];
extern const _PR1000_REG_TABLE_VDEC_SD pr1000_reg_table_vdec_SD[4+16+12+12+16+5+1+1];
#ifndef DONT_SUPPORT_STD_PVI
extern const _PR1000_REG_TABLE_VDEC_HD pr1000_reg_table_vdec_PVI[4+16+12+11+16+5+1+1];
#ifndef DONT_SUPPORT_VDRESOL_EXTEND
extern const _PR1000_REG_TABLE_VDEC_HD_EXTEND pr1000_reg_table_vdec_PVI_extend[4+16+12+11+16+5+1+1];
#endif // DONT_SUPPORT_VDRESOL_EXTEND
#endif // DONT_SUPPORT_STD_PVI
extern const _PR1000_REG_TABLE_VDEC_HD pr1000_reg_table_vdec_HDA[4+16+12+11+16+5+1+1];
#ifndef DONT_SUPPORT_VDRESOL_EXTEND
extern const _PR1000_REG_TABLE_VDEC_HD_EXTEND pr1000_reg_table_vdec_HDA_extend[4+16+12+11+16+5+1+1];
#endif // DONT_SUPPORT_VDRESOL_EXTEND
extern const _PR1000_REG_TABLE_VDEC_HD pr1000_reg_table_vdec_CVI[4+16+12+11+16+5+1+1];
#ifndef DONT_SUPPORT_VDRESOL_EXTEND
extern const _PR1000_REG_TABLE_VDEC_HD_EXTEND pr1000_reg_table_vdec_CVI_extend[4+16+12+11+16+5+1+1];
#endif // DONT_SUPPORT_VDRESOL_EXTEND
extern const _PR1000_REG_TABLE_VDEC_HD pr1000_reg_table_vdec_HDT[4+16+12+11+16+5+1+1];
#ifndef DONT_SUPPORT_VDRESOL_EXTEND
extern const _PR1000_REG_TABLE_VDEC_HD_EXTEND pr1000_reg_table_vdec_HDT_extend[4+16+12+11+16+5+1+1];
#endif // DONT_SUPPORT_VDRESOL_EXTEND
extern const _PR1000_REG_TABLE_VDEC_HD pr1000_reg_table_vdec_HDT_NEW[4+16+12+11+16+5+1+1];
#ifndef DONT_SUPPORT_VDRESOL_EXTEND
extern const _PR1000_REG_TABLE_VDEC_HD_EXTEND pr1000_reg_table_vdec_HDT_NEW_extend[4+16+12+11+16+5+1+1];
#endif // DONT_SUPPORT_VDRESOL_EXTEND

#define PR1000_OFFSETADDR_PTZ_CH(ch)		((ch%DEF_PR1000_MAX_CHN)*0x40) //0x40 register channel offset
extern const _PR1000_REG_TABLE_COMMON pr1000_reg_table_ptz_SD_common[8+6+12];
extern const _PR1000_REG_TABLE_PTZ_SD pr1000_reg_table_ptz_SD[6+2+5];
#ifndef DONT_SUPPORT_STD_PVI
extern const _PR1000_REG_TABLE_COMMON pr1000_reg_table_ptz_PVI_common[13+5+12+13];
extern const _PR1000_REG_TABLE_PTZ_HD pr1000_reg_table_ptz_PVI[1+3+3+1];
#endif // DONT_SUPPORT_STD_PVI
extern const _PR1000_REG_TABLE_COMMON pr1000_reg_table_ptz_HDA_common[13+5+12+13];
extern const _PR1000_REG_TABLE_PTZ_HD pr1000_reg_table_ptz_HDA[1+3+3+1];
extern const _PR1000_REG_TABLE_COMMON pr1000_reg_table_ptz_CVI_common[13+5+11+13];
extern const _PR1000_REG_TABLE_PTZ_HD pr1000_reg_table_ptz_CVI[1+3+4+1];
extern const _PR1000_REG_TABLE_COMMON pr1000_reg_table_ptz_HDT_common[13+5+12+1];
extern const _PR1000_REG_TABLE_PTZ_HD pr1000_reg_table_ptz_HDT[1+3+3+1];
extern const _PR1000_REG_TABLE_COMMON pr1000_reg_table_ptz_HDT_NEW_common[13+5+13+1];
extern const _PR1000_REG_TABLE_PTZ_HD pr1000_reg_table_ptz_HDT_NEW[1+3+3+1];

#define PR1000_OFFSETADDR_VEVENT_CH(ch)		((ch%DEF_PR1000_MAX_CHN)*0x40) //0x40 register channel offset
extern const _PR1000_REG_TABLE_VEVENT pr1000_reg_table_vevent[14+13+10];
extern const _stMaskCellAttr pr1000_mask_attr_table_vevent[max_pr1000_outresol];

#endif // __LINUX_SYSTEM__

/************************************************************/
#ifndef DONT_SUPPORT_HELP_STRING 
extern const char _STR_PR1000_FORMAT[max_pr1000_format][40];
extern const char _STR_PR1000_INRESOL[max_pr1000_inresol][40];
extern const char _STR_PR1000_OUTRESOL[max_pr1000_outresol][50];
extern const char _STR_PR1000_MUX_CHDEF[max_pr1000_mux_chdef][8];
extern const char _STR_PR1000_MASKCOLOR[MAX_PR1000_VEVENT_MASKCOLOR][8];
extern const char _STR_PR1000_AUD_REC_FORMAT[max_pr1000_aud_format][10];
extern const char _STR_PR1000_AUD_PB_FORMAT[max_pr1000_aud_format][10];
extern const char _STR_PR1000_AUD_SAMRATE[max_pr1000_aud_samrate][8];
extern const char _STR_PR1000_AUD_BITRATE[max_pr1000_aud_bitrate][8];
#endif // DONT_SUPPORT_HELP_STRING 

extern _stAUDRecAttr stDefAUDRecAttr;
extern _stAUDPbAttr stDefAUDPbAttr;
  
#ifndef DONT_SUPPORT_EVENT_FUNC

extern const uint32_t pr1000_def_venet_md_mask_format[PR1000_MAX_MASK_CELL_Y_NUM];
extern const uint32_t pr1000_def_venet_bd_mask_format[PR1000_MAX_MASK_CELL_Y_NUM];
extern const uint32_t pr1000_def_venet_pz_mask_format[PR1000_MAX_MASK_CELL_Y_NUM];
extern const _stVEVENTDisplayAttr stDefVEVENTDisplayAttr;
#endif // DONT_SUPPORT_EVENT_FUNC

#endif /* __PR1000_TABLE_H__ */
