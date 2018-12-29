//****************************************************************************
//
//	Copyright (C) 2012 ZhuoYongHong
//
//	Author	ZhuoYongHong
//
//	File name: xm_key.h
//	  constant definition of key
//
//	Revision history
//
//		2010.08.31	ZhuoYongHong Initial version
//
//****************************************************************************
#ifndef _XM_KEY_H_
#define _XM_KEY_H_

#define KEY_NULL        0xffff           // �ް���

/* virtual key codes*/
#define VK_POWER          0x01		// POWER
// 20180129 zhuoyonghong
// ���������������
#define VK_REBOOT         0x02		// REBOOT

#define VK_ENTER           0x08		/* backspace*/
#define VK_TAB            0x09
#define VK_RETURN         0x0D
#define VK_SHIFT          0x10		
#define VK_CONTROL        0x11		
#define VK_MENU           0x12		
#define VK_CAPITAL        0x14		
#define VK_ESCAPE         0x1B		/* esc*/
#define VK_SPACE          0x20		/* spacebar*/
#define VK_PRIOR          0x21		/* page up*/
#define VK_NEXT           0x22		/* page dn*/
#define VK_END            0x23
#define VK_HOME           0x24
#define VK_LEFT           0x25
#define VK_UP             0x26
#define VK_RIGHT          0x27
#define VK_DOWN           0x28
#define VK_INSERT         0x2D
#define VK_DELETE         0x2E
#define VK_HELP           0x2F	//	help key 

/* 0x30 - 0x39 ASCII 0 - 9*/
#define VK_0				  0x30	// 0 key 
#define VK_1				  0x31	// 1 key 
#define VK_2				  0x32	// 2 key 
#define VK_3				  0x33	// 3 key 
#define VK_4				  0x34	// 4 key 
#define VK_5				  0x35	// 5 key 
#define VK_6				  0x36	// 6 key 
#define VK_7				  0x37	// 7 key 
#define VK_8				  0x38	// 8 key 
#define VK_9				  0x39	// 9 key 
/* 0x41 - 0x5a ASCII A - Z*/
#define VK_A				  0x41	//	 a key  
#define VK_B				  0x42	//	 b key  
#define VK_C				  0x43	//	 c key  
#define VK_D				  0x44	//	 d key  
#define VK_E				  0x45	//	 e key  
#define VK_F				  0x46	//	 f key  
#define VK_G				  0x47	//	 g key  
#define VK_H				  0x48	//	 h key  
#define VK_I				  0x49	//	 i key  
#define VK_J				  0x4A	//	 j key  
#define VK_K				  0x4B	//	 k key  
#define VK_L				  0x4C	//	 l key  
#define VK_M				  0x4D	//	 m key  
#define VK_N				  0x4E	//	 n key  
#define VK_O				  0x4F	//	 o key  
#define VK_P				  0x50	//	 p key  
#define VK_Q				  0x51	//	 q key  
#define VK_R				  0x52	//	 r key  
#define VK_S				  0x53	//	 s key  
#define VK_T				  0x54	//	 t key  
#define VK_U              0x55	//	 u key  
#define VK_V              0x56	//	 v key  
#define VK_W              0x57	//	 w key  
#define VK_X              0x58	//	 x key  
#define VK_Y              0x59	//	 y key  
#define VK_Z              0x5A	//	 z key  

/* numeric keypad keys*/
#define VK_NUMPAD0        0x60
#define VK_NUMPAD1        0x61
#define VK_NUMPAD2        0x62
#define VK_NUMPAD3        0x63
#define VK_NUMPAD4        0x64
#define VK_NUMPAD5        0x65
#define VK_NUMPAD6        0x66
#define VK_NUMPAD7        0x67
#define VK_NUMPAD8        0x68
#define VK_NUMPAD9        0x69
#define VK_MULTIPLY       0x6A		/* kp * */
#define VK_ADD            0x6B		/* kp + */
#define VK_SEPARATOR      0x6C
#define VK_SUBTRACT       0x6D		/* kp - */
#define VK_DECIMAL        0x6E		/* kp . */
#define VK_DIVIDE         0x6F		/* kp / */

#define VK_F1             0x70
#define VK_F2             0x71
#define VK_F3             0x72
#define VK_F4             0x73
#define VK_F5             0x74
#define VK_F6             0x75
#define VK_F7             0x76
#define VK_F8             0x77
#define VK_F9             0x78
#define VK_F10            0x79
#define VK_F11            0x7A
#define VK_F12            0x7B

#define VK_VIRGULE        0xBF




#define VK_AP_SYSTEM_EVENT     0xF0	// ϵͳ�¼�(ģ�ⰴ����Ϣ)	
												// ���γ�/������(д����)/������(��дģʽ)
                                    // lp = 0	���γ�
                                    // lp = 1   ������(д����)
                                    // lp = 2   ������(��дģʽ)
												// lp���� �ο� XM_SYSTEMEVENT ����Ϣ��������

#define VK_AP_VIDEOSTOP  0xF1		// ��Ƶ���Ž����¼�
												// lp = 0	������������
												//	lp = 1	��ѹ�쳣����
												// lp = 2	����ʽ����
												//	lp = 3	�����쳣	(��SD���쳣���ļ�ϵͳ�쳣��)	

#define VK_AP_VIDEOITEMERR  0xF2		// ��Ƶ���쳣�¼�(���¸�ʽ�����߸���SD��)
														

// TACHOGRAPH ��������
#define VK_AP_UP          	0x26		// UP    ���ϼ�
#define VK_AP_DOWN        	0x28      // DOWN	���¼�	
#define VK_AP_MENU        	VK_F1     // VK_F1 �˵���(����/��ͣ)
#define VK_AP_SWITCH      	VK_F2     // VK_F2 OK
#define VK_AP_MODE        	VK_F3     // VK_F3 ģʽ�л���
#define VK_AP_POWER       	VK_POWER     // POWER ��Դ��(¼��������¼�����)
#define VK_AP_OK            VK_AP_SWITCH   

#define VK_AP_VOLINC      	VK_F5		// ����������ݼ�
#define VK_AP_VOLDEC      	VK_F6
#define VK_AP_MICINC      	VK_F7		// ¼��������ݼ�
#define VK_AP_MICDEC      	VK_F8
#define VK_AP_URGENT      	VK_F9		// ����¼��
#define VK_AP_MIC			VK_F10	   //������
#define VK_AP_FONT_BACK_SWITCH  VK_F11
#define VK_AP_AV         VK_ENTER


#define REMOTE_KEY_POWER    0x30
#define REMOTE_KEY_UP       0x31
#define REMOTE_KEY_DOWN     0x32
#define REMOTE_KEY_LEFT     0x33
#define REMOTE_KEY_RIGHT    0x34
#define REMOTE_KEY_MENU     0x35

#endif		// _XM_KEY_
