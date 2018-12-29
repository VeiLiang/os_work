//****************************************************************************
//
//	Copyright (C) 2015 ZhuoYongHong
//
//	Author	ZhuoYongHong
//
//	File name: arkn141_isp_sensor_cfg.h
//	  constant£¨macro & basic typedef definition of ISP sensor configuration
//
//	Revision history
//
//		2015.08.30	ZhuoYongHong Initial version
//
//****************************************************************************
#ifndef _ARKN141_ISP_SENSOR_CFG_H_
#define _ARKN141_ISP_SENSOR_CFG_H_

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif


#define	ARKN141_CMOS_SENSOR_PS1210K		0
#define	ARKN141_CMOS_SENSOR_AR0330		1
#define	ARKN141_CMOS_SENSOR_IMX322		2
#define	ARKN141_CMOS_SENSOR_IMX323		3	
#define	ARKN141_CMOS_SENSOR_AR0238		4
#define	ARKN141_CMOS_SENSOR_AR0230		5
#define	ARKN141_CMOS_SENSOR_BG0806		6
#define	ARKN141_CMOS_SENSOR_GC0308		7
#define	ARKN141_CMOS_SENSOR_IMX291		8
#define	ARKN141_CMOS_SENSOR_JX_H65		9

//#define	ARKN141_CMOS_SENSOR	ARKN141_CMOS_SENSOR_PS1210K
//#define	ARKN141_CMOS_SENSOR	ARKN141_CMOS_SENSOR_AR0330
//#define	ARKN141_CMOS_SENSOR	ARKN141_CMOS_SENSOR_IMX322
#ifndef ARKN141_CMOS_SENSOR		// »± ° π”√IMX323
#define	ARKN141_CMOS_SENSOR	ARKN141_CMOS_SENSOR_BG0806//ARKN141_CMOS_SENSOR_IMX323	
#endif
	
#define IMX323_DCK_SYNC_MODE_ENABLE	1

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif


#endif	// _ARKN141_ISP_SENSOR_CFG_H_