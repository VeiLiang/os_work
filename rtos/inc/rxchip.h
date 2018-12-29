#ifndef _RXCHIP_H_
#define _RXCHIP_H_

#ifdef  __cplusplus
extern "C" {
#endif  // __cplusplus


//#define EN_RN6752_CHIP
#define EN_RN6752M_CHIP
//#define EN_RN6752M1080N_CHIP

//#define EN_UTC_FUN		//使能UTC功能


typedef struct _RXCHIPCMD
{
    unsigned char cmd;
    unsigned char dat;
}RXCHIPCMD;

#define BEST_BRIGHTNESS	0x08
#define BEST_CONTRAST 	0x80
#define BEST_SATURATION 0x80
#define BEST_HUE 		0x80

#define CMD_BRIGHTNESS	0
#define CMD_CONTRAST	1
#define CMD_SATURATION	2
#define CMD_HUE			3



void rxchip_setting(RXCHIPCMD cmd);
static void rxchip_clear_mailbox(void);
u8 rxchip_get_ch1_signal_status(void);
u8 rxchip_get_ch2_signal_status(void);
void rxchip_check_ctrl(u8 val);
void reset_check_time(void);
u8 rxchip_get_check_flag(void);
void rxchip_set_check_flag(u8 val);
void rxchip_coaxitron_control(u8_t sen, u8_t res, u8_t cmd_index);


#ifdef __cplusplus
}
#endif          // __cplusplus


#endif	
