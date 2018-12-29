//****************************************************************************
//
//	Copyright (C) 2010 ShenZhen ExceedSpace
//
//	Author	ZhuoYongHong
//
//	File name: xm_osd_framebuffer.h
//	  constant，macro & basic typedef definition of OSD layer's framebuffer
//
//	Revision history
//
//		2010.09.01	ZhuoYongHong Initial version
//		2014.02.21  ZhuoYongHong add ARK1960 OSD framebuffer接口
//
//****************************************************************************


#ifndef _XM_OSD_FRAMEBUFFER_H_
#define _XM_OSD_FRAMEBUFFER_H_

#include "xm_osd_layer.h"

#if defined (__cplusplus)
	extern "C"{
#endif

#define	OSD_YUV_FRAMEBUFFER_COUNT		3		// YUV层framebuffer对象个数
//#define	OSD_YUV_FRAMEBUFFER_COUNT		1		// YUV层framebuffer对象个数

#define	OSD_RGB_FRAMEBUFFER_COUNT		3		// RGB层framebuffer对象个数(最小需要3个缓冲区)


// framebuffer type类型 (YUV/RGB)
enum {
	XM_OSD_FRAMEBUFFER_TYPE_YUV = 0,
	XM_OSD_FRAMEBUFFER_TYPE_RGB,
	XM_OSD_FRAMEBUFFER_TYPE_COUNT
};	

typedef struct tagXM_LCDC_CHANNEL_CONFIG {
	unsigned int		lcdc_channel;				// 通道号
	unsigned int		lcdc_type;					// 通道接口类型 (RGB, VGA, CVBS, CPU, YPbPr, BT601)
	unsigned int		lcdc_width;					// 通道宽度
	unsigned int		lcdc_height;				// 通道高度
	XM_COLORREF			lcdc_background_moon_color;	// 通道背景色(晚上)
	XM_COLORREF			lcdc_background_sun_color;		// 通道背景色(白天)

	// OSD层的像素分辨率设置
	XMSIZE				lcdc_osd_size[XM_OSD_LAYER_COUNT];
} XM_LCDC_CHANNEL_CONFIG;

#define	MAX_XM_MOVE_STEP		32

// 移动控制
typedef struct tagXM_MOVE_CONTROL {
	int				osd_step_count;			// 
	short int		osd_offset_x[MAX_XM_MOVE_STEP];		// X轴方向OSD层原点位置(相对LCDC视频输出原点, LCDC的左上角)
	short int		osd_offset_y[MAX_XM_MOVE_STEP];		// Y轴方向OSD层原点位置(相对LCDC视频输出原点, LCDC的左上角)
	unsigned char	osd_brightness[MAX_XM_MOVE_STEP];	// 亮度因子 (0 ~ 64)
	unsigned char	osd_alpha[MAX_XM_MOVE_STEP];			// 视窗alpha因子	
} XM_MOVE_CONTROL;

typedef struct tagXM_OSD_FRAMEBUFFER {
	void *				prev;
	void *				next;
	
	unsigned int		lcd_channel;	// 通道号(通道0、通道1)
	unsigned int		osd_layer;		// OSD层号

	HANDLE				view_handle;	// framebuffer对应的视窗句柄


	unsigned int		configed;		// 标记是否已配置到OSD物理层。
												//	1 已配置。不允许修改framebuffer的规格参数(高度、宽度、格式等信息)
												// 0 未配置

	unsigned int		width;			// x轴像素分辨率
	unsigned int		height;			// y轴像素分辨率
	int					offset_x;		// 相对于LCD显示输出原点的X轴偏移
	int					offset_y;		// 相对于LCD显示输出原点的Y轴偏移
	unsigned int		format;			// 帧缓冲区源数据格式, OSD Layer格式(YUV420, ARGB888, RGB565, ARGB454, )
	unsigned int		stride;			// 帧缓冲区源数据行字节长度
	unsigned char *	address;			// 帧缓冲区源数据地址
												//		若OSD Layer的格式(format)为YUV420, YUV连续存放
	
	XM_MOVE_CONTROL	move_control;	// 移动控制

	char					user_data[2048];	// 私有格式数据
	unsigned int		frame_ticket;		// 关联的视频帧创建时间(毫秒)


}XM_OSD_FRAMEBUFFER, * xm_osd_framebuffer_t;



// 创建一个新的framebuffer实例 
// 当无framebuffer实例资源可用时，返回NULL
// 若指定OSD层的格式为YUV420, YUV平面数据连续存放
xm_osd_framebuffer_t XM_osd_framebuffer_create (
												unsigned int lcd_channel,	// 视频输出通道序号
												unsigned int osd_layer,		// OSD层序号
												unsigned int layer_format,	// OSD层格式
												unsigned int layer_width,	// OSD层宽度(像素点个数)
												unsigned int layer_height,	// OSD层高度(像素点个数)
												HANDLE view_handle,			// 与framebuffer关联的视窗句柄，
																					//		可为0, 即与任何视窗没有关联
												unsigned int copy_last_view_framebuffer_context,
																					// 是否复制最近一次的相同视窗句柄的framebuffer内容
																					//		0 不复制  1 复制
												unsigned int clear_framebuffer_context
											);


// 打开指定OSD层最近使用(即最后一次创建)的framebuffer实例。
// 若该OSD层的framebuffer实例还没有创建，则返回NULL。
//		返回NULL表示打开失败(如参数错误、framebuffer实例创建失败等原因导致)
// 打开操作返回上次已创建的framebuffer实例，然后直接使用这个framebuffer实例进行GDI读写操作(直接写屏操作)
xm_osd_framebuffer_t XM_osd_framebuffer_open (unsigned int lcd_channel,		// 视频输出通道序号
															 unsigned int osd_layer,		// OSD层序号
															 unsigned int layer_format,	// OSD层格式
															 unsigned int layer_width,		// OSD层宽度(像素点个数)
														    unsigned int layer_height		// OSD层高度(像素点个数)
															 );	

// 获取framebuffer对象的数据层指针
// 对于YUV420, 得到 Y / U / V 总计3层的数据缓冲区地址
// 对于Y_UV420, 得到 Y / UV 总计2层的数据缓冲区地址
// 对于ARGB888, 得到 ARGB 总计1层的数据缓冲区地址
int XM_osd_framebuffer_get_buffer (xm_osd_framebuffer_t framebuffer, unsigned char *data[4]);

// 删除一个framebuffer对象/实例并将其回收
int XM_osd_framebuffer_delete (xm_osd_framebuffer_t framebuffer);


// 关闭framebuffer实例的使用, 并将framebuffer显示在相应的OSD层
// 若该framebuffer实例不可视，则将对应的OSD层关闭。(若OSD层已使能)
// 若该framebuffer实例可视，  则将对应的OSD层开启。(若OSD层已关闭)
// force_to_config_osd  1  强制使用新的参数初始化OSD
int XM_osd_framebuffer_close (xm_osd_framebuffer_t framebuffer, int force_to_config_osd);


// 释放指定OSD层的所有framebuffer实例，同时将该OSD层禁止(即该OSD层显示关闭)。
void XM_osd_framebuffer_release (unsigned int lcd_channel, unsigned int osd_layer);

// 复制缓冲区的内容，源framebuffer及目标framebuffer需具有相同的尺寸、显示格式, 否则复制会失败
// 成功返回0，失败返回-1
int XM_osd_framebuffer_copy (xm_osd_framebuffer_t dst, xm_osd_framebuffer_t src);

// 使用指定的颜色(ARGB)清除帧缓冲区
int XM_osd_framebuffer_clear (xm_osd_framebuffer_t framebuffer, 
										unsigned char alpha, 
										unsigned char r, 
										unsigned char g, 
										unsigned char b);


// 外部视频输出接口配置
void XM_osd_framebuffer_init (void);
void XM_osd_framebuffer_exit (void);


// 系统定义了YUV及RGB类型的framebuffer对象及设置函数，用于双缓冲区(pingpong buffer)操作。
// framebuffer对象的缓冲区基址没有设置，使用前需要在XM_osd_framebuffer_config过程中进行设置。
// 应用可以获取每种类型(YUV或RGB)的framebuffer对象个数，并在使用前设置framebuffer对象的缓冲区基址

// framebuffer对象的用户配置初始化过程, 由系统调用
void XM_osd_framebuffer_config_init (void);

// framebuffer对象的用户配置结束过程, 由系统调用
void XM_osd_framebuffer_config_exit (void);


// 获取系统定义的OSD的framebuffer对象个数
// 失败 返回0
// 成功 返回定义的framebuffer个数
int XM_osd_framebuffer_get_framebuffer_count (	unsigned int lcd_channel,			// 视频输出通道序号
																unsigned int framebuffer_type		// YUV/RGB类型
																);

// 设置系统定义的framebuffer对象的缓冲区基址
// 失败 返回0
// 成功 返回1
int XM_osd_framebuffer_set_framebuffer_address ( unsigned int lcd_channel,			// 视频输出通道序号
																 unsigned int framebuffer_type,	// YUV/RGB类型
																 unsigned int framebuffer_index,	// framebuffer对象序号(0开始)
																 unsigned char *framebuffer_base	// 缓冲区基址
																 );

int XM_osd_framebuffer_set_video_ticket (xm_osd_framebuffer_t framebuffer, unsigned int ticket);

#if defined (__cplusplus)
	}
#endif		/* end of __cplusplus */

#endif // _XM_OSD_FRAMEBUFFER_H_
