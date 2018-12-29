#include "common.h"
#include "descript.h"
#include "requests.h"

#define POWER			0x03
#define OUT_OF_RANGE		0x04
#define INVALID_UNIT		0x05
#define INVALID_CONTROL	0x06
#define INVALID_REQUEST	0x07

BYTE xdata gbAudioError;

void ERROR_AUDIO_OUT_RANGE()
{
	gbAudioError = OUT_OF_RANGE;
	//gbError = TRUE;	
}

void ERROR_AUDIO_INVALID_UNIT()
{
	gbAudioError = INVALID_UNIT;
	gbError = TRUE;
}

void ERROR_AUDIO_INVALID_CONTROL()
{
	gbAudioError = INVALID_CONTROL;
	gbError = TRUE;
}

//static void ERROR_AUDIO_INVALID_REQUEST()
//{
//	gbAudioError = INVALID_REQUEST;
//	gbError = TRUE;
//}

#define Audio_Control

void audio_control_set_cur()
{
	BYTE idata i;

	if (MSB(gsUsbRequest.wIndex) == AC_FEATURE_UNIT)
	{
		switch (MSB(gsUsbRequest.wValue))
		{
			case MUTE_CONTROL :
				usb_fetch_out(1);
				gsAudioStatus.bMute = gbBuffer[0];

				if(gsAudioStatus.bMute)
				{
				}
				else
				{
				}

				break;
			case VOLUME_CONTROL :	//signed number, 	1/256 dB
				usb_fetch_out(2);//0x1a1a
				//gsAudioStatus.bVolume = gbBuffer[1];

			#if MAKE_FOR_ASIC_OR_FPGA		//asic
				if (gbBuffer[1] <= MSB(gsAudioLimit.wMaxVolume))
				{
					for(i=0;i<AUDIO_PARA_NUM;i++)	//for(i=0;i<32;i++)
					{
						if(i<(AUDIO_PARA_NUM-1))	//if(i<31)
						{
							if((((((WORD)gbBuffer[1])<<8)+gbBuffer[0]) >= (mic_vol_mapTbl[i].dbVal))
								&& (((((WORD)gbBuffer[1])<<8)+gbBuffer[0]) < (mic_vol_mapTbl[i+1].dbVal)))
							{
								gsAudioStatus.bVolume = (mic_vol_mapTbl[i].regVal);
								break;						
							}
						}
						else
						{
							if(((((WORD)gbBuffer[1])<<8)+gbBuffer[0]) > (mic_vol_mapTbl[i].dbVal))
							{
								gsAudioStatus.bVolume = (mic_vol_mapTbl[i].regVal);
							}
						}
					}
					
					#if MASK_VER_2
						if(gsAudioStatus.bVolume <= min_vol)
						{
							gsAudioStatus.bVolume = min_vol;
						}
					#else
					#endif

				}
				else if((gbBuffer[1]==0x80) && (gbBuffer[0] == 0x00))
				{
				}
	            else
				{
					ERROR_AUDIO_OUT_RANGE();
				}
			#else		//fpga	
				if (gbBuffer[1] <= MSB(gsAudioLimit.wMaxVolume))
				{
					for(i=0;i<AUDIO_PARA_NUM;i++)
					{
						if(i<(AUDIO_PARA_NUM-1))
						{
							if((((((WORD)gbBuffer[1])<<8)+gbBuffer[0]) >= (mic_vol_mapTbl[i].dbVal))
								&& (((((WORD)gbBuffer[1])<<8)+gbBuffer[0]) < (mic_vol_mapTbl[i+1].dbVal)))
							{
								gsAudioStatus.bVolume = (mic_vol_mapTbl[i].regVal);
								break;						
							}
						}
						else
						{
							if(((((WORD)gbBuffer[1])<<8)+gbBuffer[0]) > (mic_vol_mapTbl[i].dbVal))
							{
								gsAudioStatus.bVolume = (mic_vol_mapTbl[i].regVal);
							}
						}
					}
					reg_audio_sound_value &= 0xfc;
					reg_audio_sound_value |= gsAudioStatus.bVolume;
					if((gsAudioStatus.bMute) || ((gbBuffer[0] == 0x00) && (gbBuffer[1] == 0x00)))
						reg_audio_sound_value |= NO_SOUND;
					else
						reg_audio_sound_value &= (~NO_SOUND);
				}
				else if((gbBuffer[1]==0x80) && (gbBuffer[0] == 0x00))
				{
					reg_audio_sound_value |= NO_SOUND;	
				}
                else
				{
					ERROR_AUDIO_OUT_RANGE();
				}
			#endif
				break;
			default :
				ERROR_AUDIO_INVALID_CONTROL();
				break;
		}
	}
	else
		ERROR_AUDIO_INVALID_UNIT();
}

void audio_control_get_cur()
{
	BYTE idata i;

	if (MSB(gsUsbRequest.wIndex) == AC_FEATURE_UNIT)
	{
		switch (MSB(gsUsbRequest.wValue))
		{
			case MUTE_CONTROL :
				gbBuffer[0] = gsAudioStatus.bMute;
				usb_setup_in(1);
				break;
			case VOLUME_CONTROL :
			#if MAKE_FOR_ASIC_OR_FPGA	//asic
				for(i=0;i<AUDIO_PARA_NUM;i++)
				{
					//if((mic_vol_mapTbl[i].regVal) == (reg_audio_sound_value & 0x1f))
				}
			#else	//fpga
				for(i=0;i<AUDIO_PARA_NUM;i++)
				{
					if((mic_vol_mapTbl[i].regVal) == (reg_audio_sound_value & 0x03))
					{
						//gbBuffer[0] = 0x00;
						//gbBuffer[1] = reg_audio_sound_value & 0xfc;
						gbBuffer[0] = (BYTE)(mic_vol_mapTbl[i].dbVal);
						gbBuffer[1] = (BYTE)((mic_vol_mapTbl[i].dbVal)>>8);
						break;
					}
				}
			#endif

				usb_setup_in(2);
				break;
			default :
			//	ERROR_AUDIO_INVALID_CONTROL();
			    gbBuffer[0] = 0x01;
				usb_setup_in(1);
				break;
		}
	}
	else
		ERROR_AUDIO_INVALID_UNIT();
}

void audio_control_get_min()
{
	if (MSB(gsUsbRequest.wIndex) == AC_FEATURE_UNIT)
	{
		switch (MSB(gsUsbRequest.wValue))
		{
			case MUTE_CONTROL :
				gbBuffer[0] = 0x00;
				usb_setup_in(1);				
				break;
			case VOLUME_CONTROL :
		    #if MAKE_FOR_ASIC_OR_FPGA   //asic
				gbBuffer[0] = 0x80;//
				gbBuffer[1] = 0x01;	
			#else
				gbBuffer[0] = 0x00;//0000
				gbBuffer[1] = 0x0c;	
			#endif 					
				usb_setup_in(2);				
				break;
			default :
				ERROR_AUDIO_INVALID_CONTROL();
				break;
		}
	}
	else
		ERROR_AUDIO_INVALID_UNIT();
}

void audio_control_get_max()
{
	if (MSB(gsUsbRequest.wIndex) == AC_FEATURE_UNIT)
	{
		switch (MSB(gsUsbRequest.wValue))
		{
			case MUTE_CONTROL :
				gbBuffer[0] = 0x01;
				usb_setup_in(1);				
				break;
			case VOLUME_CONTROL :
				gbBuffer[0] = LSB(gsAudioLimit.wMaxVolume);
				gbBuffer[1] = MSB(gsAudioLimit.wMaxVolume);				
				usb_setup_in(2);				
				break;
			default :
				ERROR_AUDIO_INVALID_CONTROL();
				break;
		}
	}
	else
		ERROR_AUDIO_INVALID_UNIT();
}

void audio_control_get_res()
{
	if (MSB(gsUsbRequest.wIndex) == AC_FEATURE_UNIT)
	{
		switch (MSB(gsUsbRequest.wValue))
		{
			case MUTE_CONTROL :
				gbBuffer[0] = 0x01;
				usb_setup_in(1);				
				break;
			case VOLUME_CONTROL :
				gbBuffer[0] = 0x01;
				gbBuffer[1] = 0x00;
				usb_setup_in(2);				
				break;
			default :
				ERROR_AUDIO_INVALID_CONTROL();
				break;
		}
	}
	else
		ERROR_AUDIO_INVALID_UNIT();
}

void audio_endpoint_control_set_cur()
{
	if (MSB(gsUsbRequest.wValue) == SAMPLING_FREQ_CONTROL)
	{
		usb_fetch_out(3);
		switch(gbBuffer[1])
		{
			case 0x1F :
				//reg_audio_adm_ctrl &= 0xf3;
			break;				
			case 0x3E:
			break;			
			case 0x5D:
			break;
			case 0xBB :
			default :
			break;
		}
	}
	else
		ERROR_AUDIO_INVALID_CONTROL();
}

