#ifndef __PR1000_USER_CONFIG_H__
#define __PR1000_USER_CONFIG_H__

typedef unsigned long long  uint64_t;
typedef	 unsigned char		uint8_t;

int SetCPUExternalInterrupt(void);
int GetCPUExternalIrqChipState(uint8_t *pstIrqChipState);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

extern const uint8_t PR1000_CHIP_COUNT;
extern const uint8_t PR1000_I2C_SLVADDRS[4];
extern const uint8_t DEFAULT_INIT_FORMAT;
extern const uint8_t DEFAULT_INIT_RESOLUTION;
extern const uint32_t PR1000_INT_SYNC_PERIOD;
extern const uint8_t PR1000_VIDOUTF_MUX_CH;
extern const uint8_t PR1000_VIDOUTF_16BIT;
extern const uint8_t PR1000_VIDOUTF_BT656;
extern const uint8_t PR1000_VIDOUTF_RATE;
extern const uint8_t PR1000_VIDOUTF_RESOL;
extern const uint8_t PR1000_AUDIO_ALINK_EN;
#define MAX_PR1000_VID_OUTF_MUX_TYPE 		(3)
#define MAX_PR1000_VID_OUTF_RESOL_TYPE		(4)
extern const int PR1000_VID_INOUT_MUX1CH_CHDEF[2][4][4][4][2];
extern const int PR1000_VID_INOUT_MUX2CH_CHDEF[2][4][4][4][2];
extern const int PR1000_VID_INOUT_MUX4CH_CHDEF[2][4][4][4][2];
extern const int PR1000_VIDOUTF_CLKPHASE[4][4];

typedef struct
{
	uint8_t vdck_out_phase;
}_PR1000_REG_TABLE_VIDOUT_FORMAT;

extern const _PR1000_REG_TABLE_VIDOUT_FORMAT pr1000_reg_table_vidout_format[MAX_PR1000_VID_OUTF_MUX_TYPE][2][MAX_PR1000_VID_OUTF_RESOL_TYPE];


#endif /* __PR1000_USER_CONFIG_H__ */
//////////////////////////////////////////////////////////////////////////////////////////////////////
