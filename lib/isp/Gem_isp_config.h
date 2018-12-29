
#ifndef _ISP_CONFIG_H_
#define _ISP_CONFIG_H_

void isp_SelPad (void);
void isp_UnSelPad (void);
void isp_reset (void);

void isp_clock_init (void);
void isp_clock_stop (void);

void isp_int_init (void);
void isp_i2c_init(void);
void isp_i2c_exit(void);
void isp_int_exit (void);

void isp_ae_event_init (void);

void isp_ae_event_exit (void);


#endif	// _ISP_CONFIG_H_


