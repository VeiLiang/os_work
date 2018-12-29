//****************************************************************************
//
//	Copyright (C) 2010 ShenZhen ExceedSpace
//
//	Author	ZhuoYongHong
//
//	File name: gui_res.h
//	  constant，macro, data structure，function protocol definition of resource interface 
//
//	Revision history
//
//		2010.09.09	ZhuoYongHong Initial version
//
//****************************************************************************
#ifndef _GDI_RES_H_
#define _GDI_RES_H_

#include <xmtype.h>

#define	MAX_RC_NAME			8
#define	MAX_RES_NAME		16

#define	RES_STRING			0
#define	RES_BITMAP			1
#define	RES_FONT				2
#define	RES_MENU				3
#define	RES_BINARY			4		// unknown binary type resource
#define	RES_DDB				5		// 设备相关位图格式



// 资源(RC)数据结构定义
// 一个扇区顺序放置32个资源项
// 一个代码页只能定义一个资源RC
typedef struct tagRC_HEAD {			// 结构大小为16字节
	char		rc_name[MAX_RC_NAME];	// 0	rc名字
	u16_t		code_page;					//	8	language code
	u16_t		res_count;					// 10	此RC中定义的资源个数(即资源节点个数)
	u32_t		node_offset;				// 12	资源节点表(RES_NODE)偏移
} RC_HEAD;


//resource 
typedef struct tagRES_HEAD {

	char 		res_name[MAX_RES_NAME];	// 资源名字
	u16_t		res_id;						// 资源序号
	u16_t		res_type;					// 资源类型
	u32_t		res_offset;					// 资源数据文件偏移
	u32_t		res_size;					// 资源数据长度
	u32_t		reserved;					// 保留字段, 为0		
} RES_HEAD;

#endif	// _GDI_RES_H_