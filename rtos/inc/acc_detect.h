#ifndef _ACC_DETECT_H_
#define _ACC_DETECT_H_

#ifdef  __cplusplus
extern "C" {
#endif  // __cplusplus

enum 
{
    DEVICE_EVENT_IN,
    DEVICE_EVENT_OUT,
    
};

u8 get_acc_det_status(void);

void XMSYS_ACC_Init(void);

#ifdef __cplusplus
}
#endif          // __cplusplus


#endif	
