#include <xm_proj_define.h>
#include <string.h>
#include <stdio.h>
#include "common.h"
#include "descript.h"
#include "descript_def.h"
#include "sensorlib.h"
#include "gpio_function.h"


BYTE xdata bJpegIndex_FS_1 = 0;	// 75% for 160x120
BYTE xdata bJpegIndex_FS_2 = 0; // 75% for 176x144
BYTE xdata bJpegIndex_FS_3 = 3; // 55% for 320x240 or 352x288
BYTE xdata bJpegIndex_FS_4 = 3; // 45% for >352x288
//BYTE xdata bJpegIndex_FS_4 = 4; // 35%
BYTE xdata bJpegIndex_HS_1 = 0; // 75% for <=352x288
//BYTE xdata bJpegIndex_HS_2 = 2; // 55%
//BYTE xdata bJpegIndex_HS_2 = 2; // 55%
//BYTE xdata bJpegIndex_HS_2 = 1; // 65%
BYTE xdata bJpegIndex_HS_2 = 0; // 75% for >352x288

WORD xdata bmVideoAttrContrls = B2L_16(0x3f06);

// BYTE xdata bmCamAttrControls1 = 0x00;
// 设置为非自动曝光，绝对值模式。私有UVC协议通过曝光时间绝对控制值传递。
BYTE xdata bmCamAttrControls1 = 0x08;		// D3: Exposure Time (Absolute)
BYTE xdata bmCamAttrControls2 = 0x00;

extern STD_CFG_DSCR code gcStd_cfg_desc;
extern IF_ASS_DSCR code gcStd_IAD_desc;
extern STD_IF_DSCR code gcStd_VCIF_desc;
extern CS_VCHEAD_DSCR code gcCS_VCIF_desc;
extern CAM_TERM_DSCR code gcCS_Camera_IT_desc;
extern PRO_UNIT_DSCR code gcCS_PU_desc;
//extern EXTENSION_UNIT_DSCR code gcCS_EU_desc;
extern OUT_TERM_DSCR code gcCS_OT_desc;
extern STD_EP_DSCR code gcSTD_ep_desc;
extern CS_VCEP_DSCR code gcCS_ep_desc;
extern STD_IF_DSCR code gcSTD_VSIF_desc;
extern CS_VSINHEAD_DSCR code gcCS_VS_header_desc;		//单格式
extern CS_VSINHEAD_DSCR1 code gcCS_VS_header_desc1;		//双格式
extern VS_YUV_FORMAT_DSCR code gcCS_VS_yuvformat_desc;
extern VS_MJPEG_FRAME_DSCR code gcCS_yuvframe_desc;
extern VS_MJPEG_FORMAT_DSCR code gcCS_VS_mjpegformat_desc;
extern VS_MJPEG_FRAME_DSCR code gcCS_mjpegframe_desc;
extern STIL_IMA_YUVFRAME_DSCR code gcCS_still_image_yuvframe_desc;
extern STIL_IMA_MJPEGFRAME_DSCR code gcCS_still_image_mjpegframe_desc;
extern COL_MATCH_DSCR code gcCS_color_match_desc;
#if 1
extern STD_IF_DSCR code gcSTD_VS_IF1_HS_desc1;
extern STD_EP_DSCR code gcSTD_VS_ep_HS_desc1;
extern STD_IF_DSCR code gcSTD_VS_IF1_HS_desc2;
extern STD_EP_DSCR code gcSTD_VS_ep_HS_desc2;
extern STD_IF_DSCR code gcSTD_VS_IF1_HS_desc3;
extern STD_EP_DSCR code gcSTD_VS_ep_HS_desc3;
extern STD_IF_DSCR code gcSTD_VS_IF1_HS_desc4;
extern STD_EP_DSCR code gcSTD_VS_ep_HS_desc4;
extern STD_IF_DSCR code gcSTD_VS_IF1_HS_desc5;
extern STD_EP_DSCR code gcSTD_VS_ep_HS_desc5;
extern STD_IF_DSCR code gcSTD_VS_IF1_HS_desc6;
extern STD_EP_DSCR code gcSTD_VS_ep_HS_desc6;
extern STD_IF_DSCR code gcSTD_VS_IF1_HS_desc7;
extern STD_EP_DSCR code gcSTD_VS_ep_HS_desc7;
extern STD_IF_DSCR code gcSTD_VS_IF1_HS_desc8;
extern STD_EP_DSCR code gcSTD_VS_ep_HS_desc8;
extern STD_IF_DSCR code gcSTD_VS_IF1_HS_desc9;
extern STD_EP_DSCR code gcSTD_VS_ep_HS_desc9;
extern STD_IF_DSCR code gcSTD_VS_IF1_HS_desc10;
extern STD_EP_DSCR code gcSTD_VS_ep_HS_desc10;
#endif
extern STD_IF_DSCR code gcSTD_VS_IF1_HS_desc11;
extern STD_EP_DSCR code gcSTD_VS_ep_HS_desc11;
#if 1
extern STD_IF_DSCR code gcSTD_VS_IF1_FS_desc1 ;
extern STD_EP_DSCR code gcSTD_VS_ep_FS_desc1 ;
extern STD_IF_DSCR code gcSTD_VS_IF1_FS_desc2 ;
extern STD_EP_DSCR code gcSTD_VS_ep_FS_desc2 ;
extern STD_IF_DSCR code gcSTD_VS_IF1_FS_desc3 ;
extern STD_EP_DSCR code gcSTD_VS_ep_FS_desc3 ;
extern STD_IF_DSCR code gcSTD_VS_IF1_FS_desc4 ;
extern STD_EP_DSCR code gcSTD_VS_ep_FS_desc4 ;
extern STD_IF_DSCR code gcSTD_VS_IF1_FS_desc5 ;
extern STD_EP_DSCR code gcSTD_VS_ep_FS_desc5 ;
extern STD_IF_DSCR code gcSTD_VS_IF1_FS_desc6 ;
extern STD_EP_DSCR code gcSTD_VS_ep_FS_desc6 ;
extern STD_IF_DSCR code gcSTD_VS_IF1_FS_desc7 ;
extern STD_EP_DSCR code gcSTD_VS_ep_FS_desc7 ;
#endif

extern STD_IF_DSCR code gcSTD_VS_IF1_HS_bulk;
extern STD_EP_DSCR code gcSTD_VS_ep_HS_bulk;

extern AUDIO_IAD code gcAudio_IAD_desc;
extern STD_IF_DSCR code gcSTD_ACIF_desc;
extern CS_ACHEAD_DSCR code gcCS_AcHead_desc;
extern AIN_TERM_DSCR code gcAudio_IT_desc;
extern FEA_UNIT_DSCR code gcFeater_desc;
extern AOUT_TERM_DSCR code gcAudio_OT_desc;
extern STD_IF_DSCR code gcSTD_as0_desc;
extern STD_IF_DSCR code gcSTD_as1_desc;
extern CS_AS_GENE code gcCS_as_general_desc;
extern AS_FORMAT_DSCR code gcCS_as_format_desc;
extern AUD_EP_DSCR code gcSTD_as_ep_desc;
extern CS_EP_DSCR code gcCS_as_ep_desc;

bit bFlg_mjpeg = 0;		// 0 - no mjpeg, 1 - mjpeg

//引入数组yuv_winFrame_order作为YUV格式帧INDEX顺序映射表，通过修改此表可调整各分辩率相对应帧索引
//每个SENSOR初始化时应初始化yuv_winFrame_order
BYTE xdata yuv_winFrame_order[15] =
{
	0x28,		//640x480
	0x0a,		//160x120
	0x0b,		//176x144
	0x14,		//320x240
	0x16,		//352x288
	0x00,	//0x68,		//640x360
	0x00,	//0x90,		//640x400
	0x00,	//0x20,		//720x576
	0x00,	//0x25,		//800x600
	0x00,	//0x30,		//1024x768
	0x00,	//0x2d,		//1280x720
	0x00,	//0x32,		//1280x800
	0x00,	//0x3c,		//1280x960
	0x00,	//0x40,		//1280x1024
	0x00,	//end flag
};


// 1024*600
// 800*480

//引入数组mjpeg_winFrame_order作为YUV格式帧INDEX顺序映射表，通过修改此表可调整各分辩率相对应帧索引
//每个SENSOR初始化时应初始化正确的mjpeg_winFrame_order
BYTE xdata mjpeg_winFrame_order[15] =
{
#if HYBRID_H264_MJPG
	0x2d,		//1280x720		, 缺省
	0x30,		//1024x600		
	0x28,		//640x480
	0x14, 	//320x240
	0x25,		//800x600
#elif HONGJING_CVBS
	0x28,		//640x480
	0x14, 	//320x240
	0x25,		//800x600
	0x20,		//720x576
	0,
#else
	0x30,		//1024x600		, 缺省
	0x28,		//640x480
	0x14, 	//320x240
	0x25,		//800x600
	0x2d,		//1280x720
#endif
#if HIGH_VIDEO_QUALITY
	0x50,		//1920x1080
#else
	0x00,	//0x50,		//1920x1080
#endif
	0x00,	//0x3c,		//1280x960
	0x00,		// 0x0a,		//160x120
	0x00,		// 0x0b,		//176x144
	0x00,		// 0x16,		//352x288
	0x00,	//0x68,		//640x360
	0x00,	//0x90,		//640x400
	0x00,	//0x32,		//1280x800
	0x00,	//0x40,		//1280x1024
};


//引入数组yuv_winFrame_BufferSize
//每个SENSOR初始化时应初始化正确的yuv_winFrame_BufferSize
DWORD xdata yuv_winFrame_BufferSize[15] =
{
	B2L_32(0x00600900),		//640x480
	B2L_32(0x00960000),		//160x120
	B2L_32(0x00c60000),		//176x144
	B2L_32(0x00580200),		//320x240
	B2L_32(0x00180300),		//352x288
	0,	//0x00080700,		//640x360
	0,	//0x00d00700,		//640x400
	0,	//0x00000a00,		//720x576
	0,	//0x00a60e00,		//800x600
	0,	//0x00001800,		//1024x768
	0,	//0x00201c00,		//1280x720
	0,	//0x00401f00,		//1280x800
	0,	//0x00802500,		//1280x960
	0,	//0x00002800,		//1280x1024
	0,	//0x00002800,		//1920x1080
};


//引入数组mjpeg_winFrame_BufferSize
//每个SENSOR初始化时应初始化正确的mjpeg_winFrame_BufferSize
DWORD xdata mjpeg_winFrame_BufferSize[15] =
{
#if HYBRID_H264_MJPG
	B2L_32(0x00201c00),		//1280x720	
	B2L_32(0x00001800),		//1024X600
	// 0x00600900,		//640x480
	B2L_32(0x00600900),
	// 0x00580200, 	//320x240
	B2L_32(0x00580200),
	B2L_32(0x00a60e00),		//800x600
#elif HONGJING_CVBS
	// 0x00600900,		//640x480
	B2L_32(0x00600900),
	// 0x00580200, 	//320x240
	B2L_32(0x00580200),
	B2L_32(0x00a60e00),		//800x600
	B2L_32(0x00000a00),		//720x576
	0,
#else
	B2L_32(0x00001800),		//1024X600
	// 0x00600900,		//640x480
	B2L_32(0x00600900),
	// 0x00580200, 	//320x240
	B2L_32(0x00580200),
	B2L_32(0x00a60e00),		//800x600
	B2L_32(0x00201c00),		//1280x720
#endif
	0,//B2L_32(0x00802500),		//1280x960
#if HIGH_VIDEO_QUALITY
	B2L_32(0x00804000),		//1920x1080
#else
	0,//B2L_32(0x00804000),		//1920x1080
#endif
	// 0x00960000,		//160x120
	0,	//B2L_32(0x00960000),	
	// 0x00c60000,		//176x144
	0,	//B2L_32(0x00c60000),	
	// 0x00180300,		//352x288
	0,	//B2L_32(0x00180300),
	0,	//0x00080700,		//640x360
	0,	//0x00d00700,		//640x400
	0,	//0x00000a00,		//720x576
	0,	//0x00401f00,		//1280x800
	0,	//0x00002800,		//1280x1024
};

WORD code Frame_Width[15] =
{
	B2L_16(0xa000),			//160	 	4:3
	B2L_16(0xB000),			//176		4:3
	B2L_16(0x4001),			//320		4:3
	B2L_16(0x6001),			//352		4:3
	B2L_16(0x8002),			//640		16:9
	B2L_16(0x8002),			//640		16:10
	B2L_16(0x8002),			//640		4:3
	B2L_16(0xD002),			//720		4:3
	B2L_16(0x2003),			//800		4:3
	B2L_16(0x0004),			//1024		4:3
	B2L_16(0x0005),			//1280		16:9
	B2L_16(0x0005),			//1280		16:10
	B2L_16(0x0005),			//1280		4:3
	B2L_16(0x0005),			//1280		4:3
	
	B2L_16(0x8007),			//1920		4:3
	
};

WORD code Frame_Height[15] =
{
	B2L_16(0x7800),			//120		   4:3
	B2L_16(0x9000),			//144		   4:3
	B2L_16(0xF000),			//240		   4:3
	B2L_16(0x2001),			//288		   4:3
	B2L_16(0x6801),			//360		   16:9
	B2L_16(0x9001),			//400		   16:10
	B2L_16(0xE001),			//480		   4:3
	B2L_16(0x4002),			//576		   4:3
	B2L_16(0x5802),			//600		   4:3
	B2L_16(0x5802),			//600		   4:3		1024*600
	//B2L_16(0x0003),			//768		   4:3
	B2L_16(0xd002),			//720		   16:9
	B2L_16(0x2003),			//800		   16:10
	B2L_16(0xc003),			//960		   4:3
	B2L_16(0x0004),			//1024		   4:3
	
	B2L_16(0x3804),			//1080		   4:3
	
};


WORD xdata yuv_wFrameSize_width[15] =
{
	B2L_16(0x8002),		//640
	B2L_16(0xa000),    	//160
	B2L_16(0xb000),     //176
	B2L_16(0x4001),     //320
	B2L_16(0x6001),     //352
	0,     //640
	0,     //640
	0, 	//1280
	0,     //1280
	0,
	0,
	0,
	0,
	0,
	0
};

WORD xdata yuv_wFrameSize_height[15] =
{
	B2L_16(0xe001),	//480
	B2L_16(0x7800),   //120
	B2L_16(0x9000),   //144
	B2L_16(0xf000),   //240
	B2L_16(0x2001),   //288
	0,   //360
	0,   //400
	0,   //720 0x0002;   //512
	0, //800;   //600
	0,
	0,
	0,
	0,
	0,
	0
};

WORD xdata mjpeg_wFrameSize_width[15] =
{
#if HYBRID_H264_MJPG
	B2L_16(0x0005),   	//1280
	B2L_16(0x0004),   	//1024
	B2L_16(0x8002),		//640
	B2L_16(0x4001),	 	//320
	B2L_16(0x2003),	 	//800
#elif HONGJING_CVBS
	B2L_16(0x8002),		//640
	B2L_16(0x4001),	 	//320
	B2L_16(0x2003),	 	//800
	B2L_16(0xD002),	 	//720
	0,
#else	
	B2L_16(0x0004),   	//1024
	B2L_16(0x8002),		//640
	B2L_16(0x4001),	 	//320
	B2L_16(0x2003),	 	//800
	B2L_16(0x0005),   	//1280
#endif
#if HIGH_VIDEO_QUALITY
	B2L_16(0x8007), 	 	//1920
#else
	0,//B2L_16(0x8007), 	 	//1920
#endif
	0,//B2L_16(0x0005), 		//1280
	0,	//B2L_16(0xa000),    	//160
	0,	//B2L_16(0xb000),     //176
	0,	//B2L_16(0x6001),     //352
	0,     //640
	0,     //640
	0,
	0,
	0,
};

WORD xdata mjpeg_wFrameSize_height[15] =
{
#if HYBRID_H264_MJPG
	B2L_16(0xD002),   //720
	B2L_16(0x5802),	//600
	B2L_16(0xe001),	//480
	B2L_16(0xf000),	//240
	B2L_16(0x5802),	//600	
#elif HONGJING_CVBS
	B2L_16(0xe001),	//480
	B2L_16(0xf000),	//240
	B2L_16(0x5802),	//600	
	B2L_16(0x4002),	//576
	0,
#else
	B2L_16(0x5802),	//600
	B2L_16(0xe001),	//480
	B2L_16(0xf000),	//240
	B2L_16(0x5802),	//600
	//B2L_16(0x0003),	//768
	B2L_16(0xD002),   //720
#endif
#if HIGH_VIDEO_QUALITY
	B2L_16(0x3804),   //1080 0x0438
#else
	0,//B2L_16(0x3804),   //1080 0x0438
#endif
	0,//B2L_16(0xC003),   //960 0x03C0
	0,	//B2L_16(0x7800),   //120
	0,	//B2L_16(0x9000),   //144
	0,	//B2L_16(0x2001),   //288
	0,   //360
	0,   //400
	0, //800;   //600
	0,
	0,
};

BYTE xdata UpdateBuff[1760];
BYTE xdata *pDesc;
WORD xdata nDescLength;

//配置描述符内需要调整变化的参数
//1、配置描述符总长度
//2、根据是否需要声音决定是否加上AUDIO部分描述符及接口
//2、VS header内接口描述符总长度
//3、VS header内视频格式总数
//4、VS FORMAT内的INDEX及总帧数
//5、VS frame内的INDEX及分辩率
//6、默认帧INDEX总是SENSOR所支持最大分辩率所对应的帧

int AddRec(void *pSrc, int nLength)
{
	memcpy(pDesc, pSrc, nLength);
	pDesc += nLength;
	nDescLength += nLength;
	return nDescLength;
}

//准备上电时需要的配置描述符参数
int init_descriptor(BOOL flag)
{
	BYTE idata i;
	BYTE idata j;
	BYTE xdata bYuvTotalFrameNum = 0;
	BYTE xdata bMjpegTotalFrameNum = 0;
	WORD wTotalLength_temp = 0;

	gsEp0Status.pbData = UpdateBuff;

	pDesc = UpdateBuff;
	nDescLength = 0;

	AddRec(&gcStd_cfg_desc, sizeof(STD_CFG_DSCR));
	
	if(flag)
		((STD_CFG_DSCR *)(pDesc-sizeof(STD_CFG_DSCR)))->bDescriptorType = 0x02;
	else		//other speed
		((STD_CFG_DSCR *)(pDesc-sizeof(STD_CFG_DSCR)))->bDescriptorType = 0x07;
	//--------------------------------------------------------------------------

	AddRec(&gcStd_IAD_desc,sizeof(IF_ASS_DSCR));

	//
	// **** Video Control Interface Descriptor ******
	//
	// The VideoControl interface describes the device structure (video function topology) 
	// and is used to manipulate the video controls.
	AddRec(&gcStd_VCIF_desc,sizeof(STD_IF_DSCR));
	AddRec(&gcCS_VCIF_desc,sizeof(CS_VCHEAD_DSCR));

	if(gsDeviceStatus.bUsb_HighSpeed && flag)		//hs & normal speed
	{
		wTotalLength_temp = sizeof(CS_VCHEAD_DSCR) +
							sizeof(CAM_TERM_DSCR) +
							sizeof(PRO_UNIT_DSCR) +
							//sizeof(EXTENSION_UNIT_DSCR) +
							sizeof(OUT_TERM_DSCR) ;
	}
	else		//fs or other speed
	{
		wTotalLength_temp = sizeof(CS_VCHEAD_DSCR) +
							sizeof(CAM_TERM_DSCR) +
							sizeof(PRO_UNIT_DSCR) +
							sizeof(OUT_TERM_DSCR) ;
	}

	// ((CS_VCHEAD_DSCR *)(pDesc-sizeof(CS_VCHEAD_DSCR)))->wTotalLength = (wTotalLength_temp>>8) + (wTotalLength_temp<<8);
	((CS_VCHEAD_DSCR *)(pDesc-sizeof(CS_VCHEAD_DSCR)))->wTotalLength[0] = (BYTE)(wTotalLength_temp & 0xFF);
	((CS_VCHEAD_DSCR *)(pDesc-sizeof(CS_VCHEAD_DSCR)))->wTotalLength[1] = (BYTE)(wTotalLength_temp >> 8);

	AddRec(&gcCS_Camera_IT_desc,sizeof(CAM_TERM_DSCR));
	((CAM_TERM_DSCR *)(pDesc-sizeof(CAM_TERM_DSCR)))->bmControls1 = bmCamAttrControls1;
	((CAM_TERM_DSCR *)(pDesc-sizeof(CAM_TERM_DSCR)))->bmControls2 = bmCamAttrControls2;
	AddRec(&gcCS_PU_desc,sizeof(PRO_UNIT_DSCR));
	// ((PRO_UNIT_DSCR *)(pDesc-sizeof(PRO_UNIT_DSCR)))->bmControls = bmVideoAttrContrls;
	((PRO_UNIT_DSCR *)(pDesc-sizeof(PRO_UNIT_DSCR)))->bmControls[0] = (BYTE)(bmVideoAttrContrls & 0xFF);
	((PRO_UNIT_DSCR *)(pDesc-sizeof(PRO_UNIT_DSCR)))->bmControls[1] = (BYTE)(bmVideoAttrContrls >> 8);
	AddRec(&gcCS_OT_desc,sizeof(OUT_TERM_DSCR));
	AddRec(&gcSTD_ep_desc,sizeof(STD_EP_DSCR));

	// 参考USB_Video_Class_1.1.pdf 
	// 2.4.4 Control Transfer and Request Processing (P37、38、39页)
	// 4.1.2 Get Request (P75、P76)
	// 关闭"Status Interrupt Endpoint".端口，不需要"asynchronous control" "Autoupdate control"
	// 即所有控制均为同步操作，设备没有自动控制功能
	// 通过 GEI_INFO 协议，D2 1=Disabled due to automatic mode (under device control)
	// AddRec(&gcCS_ep_desc,sizeof(CS_VCEP_DSCR));

	//
	// ********* Video Streaming Interface Descriptor **********
	// 
#if _PAYLOAD_PIPE_ == _PAYLOAD_PIPE_ISO_
	AddRec(&gcSTD_VSIF_desc,sizeof(STD_IF_DSCR));
#elif _PAYLOAD_PIPE_ == _PAYLOAD_PIPE_BULK_
	AddRec(&gcSTD_VS_IF1_HS_bulk,sizeof(STD_IF_DSCR));
#endif

	//copy vs header, re-calc vs length
	if((gsDeviceStatus.bUsb_HighSpeed) && (gsDeviceStatus.bDualFormatSel == 0) && flag)
		AddRec(&gcCS_VS_header_desc1,sizeof(CS_VSINHEAD_DSCR1));		//双格式
	else
		AddRec(&gcCS_VS_header_desc,sizeof(CS_VSINHEAD_DSCR));			//单格式

	//计算YUV总窗口数
	j=0;
	while(1)
	{
		if(yuv_winFrame_order[j] == 0x00)
			break;
		else
			j++;
	}
	bYuvTotalFrameNum = j;

	//计算MJPEG总窗口数
	j=0;
	while(1)
	{
		if(mjpeg_winFrame_order[j] == 0x00)
			break;
		else
			j++;
	}
	bMjpegTotalFrameNum = j;

	if(gsDeviceStatus.bUsb_HighSpeed && flag)	//hs，视情况决定所支持的视频格式
	{
		if((gsDeviceStatus.bDualFormatSel) && (gsDeviceStatus.bFormatSel))	//HS,仅支持YUV
		{
			wTotalLength_temp = sizeof(CS_VSINHEAD_DSCR) +
								sizeof(VS_YUV_FORMAT_DSCR) +
								sizeof(VS_MJPEG_FRAME_DSCR)*bYuvTotalFrameNum +
								(7 + (bYuvTotalFrameNum << 2)) +
								sizeof(COL_MATCH_DSCR) ;

			((CS_VSINHEAD_DSCR *)(pDesc-sizeof(CS_VSINHEAD_DSCR)))->bNumFormats = 1;
			// ((CS_VSINHEAD_DSCR *)(pDesc-sizeof(CS_VSINHEAD_DSCR)))->wTotalLength = (wTotalLength_temp>>8) + (wTotalLength_temp<<8);
			((CS_VSINHEAD_DSCR *)(pDesc-sizeof(CS_VSINHEAD_DSCR)))->wTotalLength[0] = (BYTE)(wTotalLength_temp & 0xFF);
			((CS_VSINHEAD_DSCR *)(pDesc-sizeof(CS_VSINHEAD_DSCR)))->wTotalLength[1] = (BYTE)(wTotalLength_temp >> 8);
		}

		if((gsDeviceStatus.bDualFormatSel) && (gsDeviceStatus.bFormatSel == 0))	//HS,仅支持MJPEG
		{
			wTotalLength_temp = sizeof(CS_VSINHEAD_DSCR) +
								sizeof(VS_MJPEG_FORMAT_DSCR) +
								sizeof(VS_MJPEG_FRAME_DSCR)*bMjpegTotalFrameNum +
								(10 + (bMjpegTotalFrameNum << 2)) +
								sizeof(COL_MATCH_DSCR) ;

			((CS_VSINHEAD_DSCR *)(pDesc-sizeof(CS_VSINHEAD_DSCR)))->bNumFormats = 1;
			//((CS_VSINHEAD_DSCR *)(pDesc-sizeof(CS_VSINHEAD_DSCR)))->wTotalLength = (wTotalLength_temp>>8) + (wTotalLength_temp<<8);
			((CS_VSINHEAD_DSCR *)(pDesc-sizeof(CS_VSINHEAD_DSCR)))->wTotalLength[0] = (BYTE)(wTotalLength_temp);
			((CS_VSINHEAD_DSCR *)(pDesc-sizeof(CS_VSINHEAD_DSCR)))->wTotalLength[1] = (BYTE) (wTotalLength_temp >> 8);
		}

		if(gsDeviceStatus.bDualFormatSel == 0)	//HS,支持YUV+MJPEG
		{
			wTotalLength_temp = sizeof(CS_VSINHEAD_DSCR1) +
								sizeof(VS_YUV_FORMAT_DSCR) +
								sizeof(VS_MJPEG_FRAME_DSCR)*bYuvTotalFrameNum +
								(7 + (bYuvTotalFrameNum << 2)) + 	//sizeof(STIL_IMA_YUVFRAME_DSCR) +
								sizeof(COL_MATCH_DSCR) +
								sizeof(VS_MJPEG_FORMAT_DSCR) +
								sizeof(VS_MJPEG_FRAME_DSCR)*bMjpegTotalFrameNum +
								(10 + (bMjpegTotalFrameNum << 2)) + 	//sizeof(STIL_IMA_MJPEGFRAME_DSCR) +
								sizeof(COL_MATCH_DSCR) ;

			((CS_VSINHEAD_DSCR1 *)(pDesc-sizeof(CS_VSINHEAD_DSCR1)))->bNumFormats = 2;
			// ((CS_VSINHEAD_DSCR1 *)(pDesc-sizeof(CS_VSINHEAD_DSCR1)))->wTotalLength = (wTotalLength_temp>>8) + (wTotalLength_temp<<8);
			((CS_VSINHEAD_DSCR1 *)(pDesc-sizeof(CS_VSINHEAD_DSCR1)))->wTotalLength[0] = (BYTE)(wTotalLength_temp & 0xFF);
			((CS_VSINHEAD_DSCR1 *)(pDesc-sizeof(CS_VSINHEAD_DSCR1)))->wTotalLength[1] = (BYTE)(wTotalLength_temp >> 8);
		}
	}
	else	//FS，仅支持MJPEG , fs or other speed
	{
		//USB1.1下只支持JPEG，且最大窗口为VGA
		//重新计算USB1.1下的窗口总数
		j = 0;
		for(i=0;i<bMjpegTotalFrameNum;i++)
		{
			if((mjpeg_winFrame_order[i] == 0x20)
				|| (mjpeg_winFrame_order[i] == 0x25)
				|| (mjpeg_winFrame_order[i] == 0x30)
				|| (mjpeg_winFrame_order[i] == 0x2d)
				|| (mjpeg_winFrame_order[i] == 0x32)
				|| (mjpeg_winFrame_order[i] == 0x3c)
				|| (mjpeg_winFrame_order[i] == 0x40))
			{
				mjpeg_winFrame_order[i] = 0x00;
				j+=1;
			}
		}
		bMjpegTotalFrameNum -= j;

		wTotalLength_temp = sizeof(CS_VSINHEAD_DSCR) +
							sizeof(VS_MJPEG_FORMAT_DSCR) +
							sizeof(VS_MJPEG_FRAME_DSCR)*bMjpegTotalFrameNum +
							(10 + (bMjpegTotalFrameNum << 2)) +
							sizeof(COL_MATCH_DSCR) ;

		((CS_VSINHEAD_DSCR *)(pDesc-sizeof(CS_VSINHEAD_DSCR)))->bNumFormats = 1;
		// ((CS_VSINHEAD_DSCR *)(pDesc-sizeof(CS_VSINHEAD_DSCR)))->wTotalLength = (wTotalLength_temp>>8) + (wTotalLength_temp<<8);
		((CS_VSINHEAD_DSCR *)(pDesc-sizeof(CS_VSINHEAD_DSCR)))->wTotalLength[0] = (BYTE)(wTotalLength_temp & 0xFF);
		((CS_VSINHEAD_DSCR *)(pDesc-sizeof(CS_VSINHEAD_DSCR)))->wTotalLength[1] = (BYTE)(wTotalLength_temp >> 8);
	}

	if(gsDeviceStatus.bUsb_HighSpeed && flag)	//hs，视情况决定所支持的视频格式
	{
		if((gsDeviceStatus.bDualFormatSel == 0)|| ((gsDeviceStatus.bDualFormatSel)&&(gsDeviceStatus.bFormatSel)))
		{
			//add yuv format here
			AddRec(&gcCS_VS_yuvformat_desc,sizeof(VS_YUV_FORMAT_DSCR));
			
			if(bDualFormatOrder)
				((VS_YUV_FORMAT_DSCR *)(pDesc-sizeof(VS_YUV_FORMAT_DSCR)))->bFormatIndex = 1;
			else
				((VS_YUV_FORMAT_DSCR *)(pDesc-sizeof(VS_YUV_FORMAT_DSCR)))->bFormatIndex = 2;
			
			((VS_YUV_FORMAT_DSCR *)(pDesc-sizeof(VS_YUV_FORMAT_DSCR)))->bNumFrameDescriptors = bYuvTotalFrameNum;
			
			for(i = 0; i<bYuvTotalFrameNum; i++)
			{
				AddRec(&gcCS_yuvframe_desc,sizeof(VS_MJPEG_FRAME_DSCR));			
				((VS_MJPEG_FRAME_DSCR *)(pDesc-sizeof(VS_MJPEG_FRAME_DSCR)))->bFrameIndex = i+1;

				// ((VS_MJPEG_FRAME_DSCR *)(pDesc-sizeof(VS_MJPEG_FRAME_DSCR)))->wWidth = yuv_wFrameSize_width[i];
				((VS_MJPEG_FRAME_DSCR *)(pDesc-sizeof(VS_MJPEG_FRAME_DSCR)))->wWidth[0] = (BYTE)(yuv_wFrameSize_width[i] & 0xFF);
				((VS_MJPEG_FRAME_DSCR *)(pDesc-sizeof(VS_MJPEG_FRAME_DSCR)))->wWidth[1] = (BYTE)(yuv_wFrameSize_width[i] >> 8);
				// ((VS_MJPEG_FRAME_DSCR *)(pDesc-sizeof(VS_MJPEG_FRAME_DSCR)))->wHeight = yuv_wFrameSize_height[i];
				((VS_MJPEG_FRAME_DSCR *)(pDesc-sizeof(VS_MJPEG_FRAME_DSCR)))->wHeight[0] = (BYTE)(yuv_wFrameSize_height[i] & 0xFF);
				((VS_MJPEG_FRAME_DSCR *)(pDesc-sizeof(VS_MJPEG_FRAME_DSCR)))->wHeight[1] = (BYTE)(yuv_wFrameSize_height[i] >> 8);
				// ((VS_MJPEG_FRAME_DSCR *)(pDesc-sizeof(VS_MJPEG_FRAME_DSCR)))->dwMaxVideoFrameBufferSize = yuv_winFrame_BufferSize[i];
				((VS_MJPEG_FRAME_DSCR *)(pDesc-sizeof(VS_MJPEG_FRAME_DSCR)))->dwMaxVideoFrameBufferSize[0] = (BYTE)(yuv_winFrame_BufferSize[i] & 0xFF);
				((VS_MJPEG_FRAME_DSCR *)(pDesc-sizeof(VS_MJPEG_FRAME_DSCR)))->dwMaxVideoFrameBufferSize[1] = (BYTE)(yuv_winFrame_BufferSize[i] >> 8);
				((VS_MJPEG_FRAME_DSCR *)(pDesc-sizeof(VS_MJPEG_FRAME_DSCR)))->dwMaxVideoFrameBufferSize[2] = (BYTE)(yuv_winFrame_BufferSize[i] >> 16);
				((VS_MJPEG_FRAME_DSCR *)(pDesc-sizeof(VS_MJPEG_FRAME_DSCR)))->dwMaxVideoFrameBufferSize[3] = (BYTE)(yuv_winFrame_BufferSize[i] >> 24);
			}

			AddRec(&gcCS_still_image_yuvframe_desc,5);
			((STIL_IMA_YUVFRAME_DSCR *)(pDesc-5))->bLength = 7 + (bYuvTotalFrameNum << 2);
			((STIL_IMA_YUVFRAME_DSCR *)(pDesc-5))->bNumImageSizePatterns = bYuvTotalFrameNum;
			for(i=0; i<bYuvTotalFrameNum; i++)
			{
				wTotalLength_temp = yuv_wFrameSize_width[i];
				AddRec(&wTotalLength_temp, 2);
				wTotalLength_temp = yuv_wFrameSize_height[i];
				AddRec(&wTotalLength_temp, 2);
			}
			wTotalLength_temp = B2L_16(0x0101);
			AddRec(&wTotalLength_temp, 2);
			wTotalLength_temp = 0;				

			AddRec(&gcCS_color_match_desc,sizeof(COL_MATCH_DSCR));
		}
	
		if((gsDeviceStatus.bDualFormatSel)&&(gsDeviceStatus.bFormatSel==0))
		{
			//add mjpeg format here
			AddRec(&gcCS_VS_mjpegformat_desc,sizeof(VS_MJPEG_FORMAT_DSCR));
			((VS_MJPEG_FORMAT_DSCR *)(pDesc-sizeof(VS_MJPEG_FORMAT_DSCR)))->bFormatIndex = 1;
			((VS_MJPEG_FORMAT_DSCR *)(pDesc-sizeof(VS_MJPEG_FORMAT_DSCR)))->bNumFrameDescriptors = bMjpegTotalFrameNum;

			for(i = 0; i<bMjpegTotalFrameNum; i++)
			{
				AddRec(&gcCS_mjpegframe_desc,sizeof(VS_MJPEG_FRAME_DSCR));			
				((VS_MJPEG_FRAME_DSCR *)(pDesc-sizeof(VS_MJPEG_FRAME_DSCR)))->bFrameIndex = i+1;

				// ((VS_MJPEG_FRAME_DSCR *)(pDesc-sizeof(VS_MJPEG_FRAME_DSCR)))->wWidth = mjpeg_wFrameSize_width[i];
				((VS_MJPEG_FRAME_DSCR *)(pDesc-sizeof(VS_MJPEG_FRAME_DSCR)))->wWidth[0] = (BYTE)(mjpeg_wFrameSize_width[i] & 0xFF);
				((VS_MJPEG_FRAME_DSCR *)(pDesc-sizeof(VS_MJPEG_FRAME_DSCR)))->wWidth[1] = (BYTE)(mjpeg_wFrameSize_width[i] >> 8);
				// ((VS_MJPEG_FRAME_DSCR *)(pDesc-sizeof(VS_MJPEG_FRAME_DSCR)))->wHeight = mjpeg_wFrameSize_height[i];
				((VS_MJPEG_FRAME_DSCR *)(pDesc-sizeof(VS_MJPEG_FRAME_DSCR)))->wHeight[0] = (BYTE)(mjpeg_wFrameSize_height[i] & 0xFF);
				((VS_MJPEG_FRAME_DSCR *)(pDesc-sizeof(VS_MJPEG_FRAME_DSCR)))->wHeight[1] = (BYTE)(mjpeg_wFrameSize_height[i] >> 8);
				// ((VS_MJPEG_FRAME_DSCR *)(pDesc-sizeof(VS_MJPEG_FRAME_DSCR)))->dwMaxVideoFrameBufferSize = mjpeg_winFrame_BufferSize[i];
				((VS_MJPEG_FRAME_DSCR *)(pDesc-sizeof(VS_MJPEG_FRAME_DSCR)))->dwMaxVideoFrameBufferSize[0] = (BYTE)(mjpeg_winFrame_BufferSize[i] & 0xFF);
				((VS_MJPEG_FRAME_DSCR *)(pDesc-sizeof(VS_MJPEG_FRAME_DSCR)))->dwMaxVideoFrameBufferSize[1] = (BYTE)(mjpeg_winFrame_BufferSize[i] >> 8);
				((VS_MJPEG_FRAME_DSCR *)(pDesc-sizeof(VS_MJPEG_FRAME_DSCR)))->dwMaxVideoFrameBufferSize[2] = (BYTE)(mjpeg_winFrame_BufferSize[i] >> 16);
				((VS_MJPEG_FRAME_DSCR *)(pDesc-sizeof(VS_MJPEG_FRAME_DSCR)))->dwMaxVideoFrameBufferSize[3] = (BYTE)(mjpeg_winFrame_BufferSize[i] >> 24);
			}

			AddRec(&gcCS_still_image_mjpegframe_desc,5);
			((STIL_IMA_MJPEGFRAME_DSCR *)(pDesc-5))->bLength = 10 + (bMjpegTotalFrameNum << 2);
			((STIL_IMA_MJPEGFRAME_DSCR *)(pDesc-5))->bNumImageSizePatterns = bMjpegTotalFrameNum;
			for(i=0; i<bMjpegTotalFrameNum; i++)
			{
				wTotalLength_temp = mjpeg_wFrameSize_width[i];
				AddRec(&wTotalLength_temp, 2);
				wTotalLength_temp = mjpeg_wFrameSize_height[i];
				AddRec(&wTotalLength_temp, 2);
			}
			wTotalLength_temp = B2L_16(0x0401);				
			AddRec(&wTotalLength_temp, 2);
			wTotalLength_temp = B2L_16(0x0205);				
			AddRec(&wTotalLength_temp, 2);
			wTotalLength_temp = B2L_16(0x0e00);				
			AddRec(&wTotalLength_temp, 1);
			wTotalLength_temp = 0;				

			AddRec(&gcCS_color_match_desc,sizeof(COL_MATCH_DSCR));
		}
	
		if(gsDeviceStatus.bDualFormatSel==0)
		{
			//add mjpeg format here
			AddRec(&gcCS_VS_mjpegformat_desc,sizeof(VS_MJPEG_FORMAT_DSCR));

			if(bDualFormatOrder)
				((VS_MJPEG_FORMAT_DSCR *)(pDesc-sizeof(VS_MJPEG_FORMAT_DSCR)))->bFormatIndex = 2;
			else
				((VS_MJPEG_FORMAT_DSCR *)(pDesc-sizeof(VS_MJPEG_FORMAT_DSCR)))->bFormatIndex = 1;

			((VS_MJPEG_FORMAT_DSCR *)(pDesc-sizeof(VS_MJPEG_FORMAT_DSCR)))->bNumFrameDescriptors = bMjpegTotalFrameNum;
			
			for(i = 0; i<bMjpegTotalFrameNum; i++)
			{
				AddRec(&gcCS_mjpegframe_desc,sizeof(VS_MJPEG_FRAME_DSCR));			
				((VS_MJPEG_FRAME_DSCR *)(pDesc-sizeof(VS_MJPEG_FRAME_DSCR)))->bFrameIndex = i+1;

				// ((VS_MJPEG_FRAME_DSCR *)(pDesc-sizeof(VS_MJPEG_FRAME_DSCR)))->wWidth = mjpeg_wFrameSize_width[i];
				((VS_MJPEG_FRAME_DSCR *)(pDesc-sizeof(VS_MJPEG_FRAME_DSCR)))->wWidth[0] = (BYTE)(mjpeg_wFrameSize_width[i] & 0xFF);
				((VS_MJPEG_FRAME_DSCR *)(pDesc-sizeof(VS_MJPEG_FRAME_DSCR)))->wWidth[1] = (BYTE)(mjpeg_wFrameSize_width[i] >> 8);
				// ((VS_MJPEG_FRAME_DSCR *)(pDesc-sizeof(VS_MJPEG_FRAME_DSCR)))->wHeight = mjpeg_wFrameSize_height[i];
				((VS_MJPEG_FRAME_DSCR *)(pDesc-sizeof(VS_MJPEG_FRAME_DSCR)))->wHeight[0] = (BYTE)(mjpeg_wFrameSize_height[i] & 0xFF);
				((VS_MJPEG_FRAME_DSCR *)(pDesc-sizeof(VS_MJPEG_FRAME_DSCR)))->wHeight[1] = (BYTE)(mjpeg_wFrameSize_height[i] >> 8);
				// ((VS_MJPEG_FRAME_DSCR *)(pDesc-sizeof(VS_MJPEG_FRAME_DSCR)))->dwMaxVideoFrameBufferSize = mjpeg_winFrame_BufferSize[i];
				((VS_MJPEG_FRAME_DSCR *)(pDesc-sizeof(VS_MJPEG_FRAME_DSCR)))->dwMaxVideoFrameBufferSize[0] = (BYTE)(mjpeg_winFrame_BufferSize[i] & 0xFF);
				((VS_MJPEG_FRAME_DSCR *)(pDesc-sizeof(VS_MJPEG_FRAME_DSCR)))->dwMaxVideoFrameBufferSize[1] = (BYTE)(mjpeg_winFrame_BufferSize[i] >> 8);
				((VS_MJPEG_FRAME_DSCR *)(pDesc-sizeof(VS_MJPEG_FRAME_DSCR)))->dwMaxVideoFrameBufferSize[2] = (BYTE)(mjpeg_winFrame_BufferSize[i] >> 16);
				((VS_MJPEG_FRAME_DSCR *)(pDesc-sizeof(VS_MJPEG_FRAME_DSCR)))->dwMaxVideoFrameBufferSize[3] = (BYTE)(mjpeg_winFrame_BufferSize[i] >> 24);
			}
			
			AddRec(&gcCS_still_image_mjpegframe_desc,5);
			((STIL_IMA_MJPEGFRAME_DSCR *)(pDesc-5))->bLength = 10 + (bMjpegTotalFrameNum << 2);
			((STIL_IMA_MJPEGFRAME_DSCR *)(pDesc-5))->bNumImageSizePatterns = bMjpegTotalFrameNum;
			for(i=0; i<bMjpegTotalFrameNum; i++)
			{
				wTotalLength_temp = mjpeg_wFrameSize_width[i];
				AddRec(&wTotalLength_temp, 2);
				wTotalLength_temp = mjpeg_wFrameSize_height[i];
				AddRec(&wTotalLength_temp, 2);
			}
			wTotalLength_temp = B2L_16(0x0401);				
			AddRec(&wTotalLength_temp, 2);
			wTotalLength_temp = B2L_16(0x0205);				
			AddRec(&wTotalLength_temp, 2);
			wTotalLength_temp = B2L_16(0x0e00);				
			AddRec(&wTotalLength_temp, 1);
			wTotalLength_temp = 0;				

			AddRec(&gcCS_color_match_desc,sizeof(COL_MATCH_DSCR));
		}
	}
	else  //FS，仅支持MJPEG , fs or other speed
	{
		//add mjpeg format here
		AddRec(&gcCS_VS_mjpegformat_desc,sizeof(VS_MJPEG_FORMAT_DSCR));
		((VS_MJPEG_FORMAT_DSCR *)(pDesc-sizeof(VS_MJPEG_FORMAT_DSCR)))->bFormatIndex = 1;
		((VS_MJPEG_FORMAT_DSCR *)(pDesc-sizeof(VS_MJPEG_FORMAT_DSCR)))->bNumFrameDescriptors = bMjpegTotalFrameNum;

		for(i = 0; i<bMjpegTotalFrameNum; i++)
		{
			AddRec(&gcCS_mjpegframe_desc,sizeof(VS_MJPEG_FRAME_DSCR));			
			((VS_MJPEG_FRAME_DSCR *)(pDesc-sizeof(VS_MJPEG_FRAME_DSCR)))->bFrameIndex = i+1;

			// ((VS_MJPEG_FRAME_DSCR *)(pDesc-sizeof(VS_MJPEG_FRAME_DSCR)))->wWidth = mjpeg_wFrameSize_width[i];
			((VS_MJPEG_FRAME_DSCR *)(pDesc-sizeof(VS_MJPEG_FRAME_DSCR)))->wWidth[0] = (BYTE)(mjpeg_wFrameSize_width[i]);
			((VS_MJPEG_FRAME_DSCR *)(pDesc-sizeof(VS_MJPEG_FRAME_DSCR)))->wWidth[1] = (BYTE)(mjpeg_wFrameSize_width[i] >> 8);
			// ((VS_MJPEG_FRAME_DSCR *)(pDesc-sizeof(VS_MJPEG_FRAME_DSCR)))->wHeight = mjpeg_wFrameSize_height[i];
			((VS_MJPEG_FRAME_DSCR *)(pDesc-sizeof(VS_MJPEG_FRAME_DSCR)))->wHeight[0] = (BYTE)(mjpeg_wFrameSize_height[i]);
			((VS_MJPEG_FRAME_DSCR *)(pDesc-sizeof(VS_MJPEG_FRAME_DSCR)))->wHeight[1] = (BYTE)(mjpeg_wFrameSize_height[i] >> 8);
			// ((VS_MJPEG_FRAME_DSCR *)(pDesc-sizeof(VS_MJPEG_FRAME_DSCR)))->dwMinBitRate = 0x00600900;	//test
			((VS_MJPEG_FRAME_DSCR *)(pDesc-sizeof(VS_MJPEG_FRAME_DSCR)))->dwMinBitRate[0] = 0x00;
			((VS_MJPEG_FRAME_DSCR *)(pDesc-sizeof(VS_MJPEG_FRAME_DSCR)))->dwMinBitRate[1] = 0x60;
			((VS_MJPEG_FRAME_DSCR *)(pDesc-sizeof(VS_MJPEG_FRAME_DSCR)))->dwMinBitRate[2] = 0x09;
			((VS_MJPEG_FRAME_DSCR *)(pDesc-sizeof(VS_MJPEG_FRAME_DSCR)))->dwMinBitRate[3] = 0x00;
			// ((VS_MJPEG_FRAME_DSCR *)(pDesc-sizeof(VS_MJPEG_FRAME_DSCR)))->dwMaxBitRate = 0x00401901;	//test
			((VS_MJPEG_FRAME_DSCR *)(pDesc-sizeof(VS_MJPEG_FRAME_DSCR)))->dwMaxBitRate[0] = 0x00;
			((VS_MJPEG_FRAME_DSCR *)(pDesc-sizeof(VS_MJPEG_FRAME_DSCR)))->dwMaxBitRate[1] = 0x40;
			((VS_MJPEG_FRAME_DSCR *)(pDesc-sizeof(VS_MJPEG_FRAME_DSCR)))->dwMaxBitRate[2] = 0x19;
			((VS_MJPEG_FRAME_DSCR *)(pDesc-sizeof(VS_MJPEG_FRAME_DSCR)))->dwMaxBitRate[3] = 0x01;
			// ((VS_MJPEG_FRAME_DSCR *)(pDesc-sizeof(VS_MJPEG_FRAME_DSCR)))->dwMaxVideoFrameBufferSize = mjpeg_winFrame_BufferSize[i];
			((VS_MJPEG_FRAME_DSCR *)(pDesc-sizeof(VS_MJPEG_FRAME_DSCR)))->dwMaxVideoFrameBufferSize[0] = (BYTE)(mjpeg_winFrame_BufferSize[i]);
			((VS_MJPEG_FRAME_DSCR *)(pDesc-sizeof(VS_MJPEG_FRAME_DSCR)))->dwMaxVideoFrameBufferSize[1] = (BYTE)(mjpeg_winFrame_BufferSize[i] >> 8);
			((VS_MJPEG_FRAME_DSCR *)(pDesc-sizeof(VS_MJPEG_FRAME_DSCR)))->dwMaxVideoFrameBufferSize[2] = (BYTE)(mjpeg_winFrame_BufferSize[i] >> 16);
			((VS_MJPEG_FRAME_DSCR *)(pDesc-sizeof(VS_MJPEG_FRAME_DSCR)))->dwMaxVideoFrameBufferSize[3] = (BYTE)(mjpeg_winFrame_BufferSize[i] >> 24);
			// ((VS_MJPEG_FRAME_DSCR *)(pDesc-sizeof(VS_MJPEG_FRAME_DSCR)))->dwDefaultFrameInterval = 0x15160500;	//test
			((VS_MJPEG_FRAME_DSCR *)(pDesc-sizeof(VS_MJPEG_FRAME_DSCR)))->dwDefaultFrameInterval[0] = 0x15;
			((VS_MJPEG_FRAME_DSCR *)(pDesc-sizeof(VS_MJPEG_FRAME_DSCR)))->dwDefaultFrameInterval[1] = 0x16;
			((VS_MJPEG_FRAME_DSCR *)(pDesc-sizeof(VS_MJPEG_FRAME_DSCR)))->dwDefaultFrameInterval[2] = 0x05;
			((VS_MJPEG_FRAME_DSCR *)(pDesc-sizeof(VS_MJPEG_FRAME_DSCR)))->dwDefaultFrameInterval[3] = 0x00;	//test
			// ((VS_MJPEG_FRAME_DSCR *)(pDesc-sizeof(VS_MJPEG_FRAME_DSCR)))->dwFrameInterval0 = 0x15160500;		//30fps
			((VS_MJPEG_FRAME_DSCR *)(pDesc-sizeof(VS_MJPEG_FRAME_DSCR)))->dwFrameInterval0[0] = 0x15;
			((VS_MJPEG_FRAME_DSCR *)(pDesc-sizeof(VS_MJPEG_FRAME_DSCR)))->dwFrameInterval0[1] = 0x16;
			((VS_MJPEG_FRAME_DSCR *)(pDesc-sizeof(VS_MJPEG_FRAME_DSCR)))->dwFrameInterval0[2] = 0x05;
			((VS_MJPEG_FRAME_DSCR *)(pDesc-sizeof(VS_MJPEG_FRAME_DSCR)))->dwFrameInterval0[3] = 0x00;	
			
			// ((VS_MJPEG_FRAME_DSCR *)(pDesc-sizeof(VS_MJPEG_FRAME_DSCR)))->dwFrameInterval1 = 0x2A2C0A00;		//15fps
			((VS_MJPEG_FRAME_DSCR *)(pDesc-sizeof(VS_MJPEG_FRAME_DSCR)))->dwFrameInterval1[0] = 0x2A;
			((VS_MJPEG_FRAME_DSCR *)(pDesc-sizeof(VS_MJPEG_FRAME_DSCR)))->dwFrameInterval1[1] = 0x2C;
			((VS_MJPEG_FRAME_DSCR *)(pDesc-sizeof(VS_MJPEG_FRAME_DSCR)))->dwFrameInterval1[2] = 0x0A;
			((VS_MJPEG_FRAME_DSCR *)(pDesc-sizeof(VS_MJPEG_FRAME_DSCR)))->dwFrameInterval1[3] = 0x00;		//15fps
			// ((VS_MJPEG_FRAME_DSCR *)(pDesc-sizeof(VS_MJPEG_FRAME_DSCR)))->dwFrameInterval2 = 0x40420F00;		//10fps
			((VS_MJPEG_FRAME_DSCR *)(pDesc-sizeof(VS_MJPEG_FRAME_DSCR)))->dwFrameInterval2[0] = 0x40;
			((VS_MJPEG_FRAME_DSCR *)(pDesc-sizeof(VS_MJPEG_FRAME_DSCR)))->dwFrameInterval2[1] = 0x42;
			((VS_MJPEG_FRAME_DSCR *)(pDesc-sizeof(VS_MJPEG_FRAME_DSCR)))->dwFrameInterval2[2] = 0x0F;
			((VS_MJPEG_FRAME_DSCR *)(pDesc-sizeof(VS_MJPEG_FRAME_DSCR)))->dwFrameInterval2[3] = 0x00;
			// ((VS_MJPEG_FRAME_DSCR *)(pDesc-sizeof(VS_MJPEG_FRAME_DSCR)))->dwFrameInterval3 = 0x80841E00;		//5fps
			((VS_MJPEG_FRAME_DSCR *)(pDesc-sizeof(VS_MJPEG_FRAME_DSCR)))->dwFrameInterval3[0] = 0x80;
			((VS_MJPEG_FRAME_DSCR *)(pDesc-sizeof(VS_MJPEG_FRAME_DSCR)))->dwFrameInterval3[1] = 0x84;
			((VS_MJPEG_FRAME_DSCR *)(pDesc-sizeof(VS_MJPEG_FRAME_DSCR)))->dwFrameInterval3[2] = 0x1E;
			((VS_MJPEG_FRAME_DSCR *)(pDesc-sizeof(VS_MJPEG_FRAME_DSCR)))->dwFrameInterval3[3] = 0x00;
		}

		AddRec(&gcCS_still_image_mjpegframe_desc,5);
		((STIL_IMA_MJPEGFRAME_DSCR *)(pDesc-5))->bLength = 10 + (bMjpegTotalFrameNum << 2);
		((STIL_IMA_MJPEGFRAME_DSCR *)(pDesc-5))->bNumImageSizePatterns = bMjpegTotalFrameNum;
		for(i=0; i<bMjpegTotalFrameNum; i++)
		{
			wTotalLength_temp = mjpeg_wFrameSize_width[i];
			AddRec(&wTotalLength_temp, 2);
			wTotalLength_temp = mjpeg_wFrameSize_height[i];
			AddRec(&wTotalLength_temp, 2);
		}
		wTotalLength_temp = B2L_16(0x0401);				
		AddRec(&wTotalLength_temp, 2);
		wTotalLength_temp = B2L_16(0x0205);				
		AddRec(&wTotalLength_temp, 2);
		wTotalLength_temp = B2L_16(0x0e00);				
		AddRec(&wTotalLength_temp, 1);
		wTotalLength_temp = 0;				

		AddRec(&gcCS_color_match_desc,sizeof(COL_MATCH_DSCR));
	}

#if _PAYLOAD_PIPE_ == _PAYLOAD_PIPE_BULK_
	AddRec(&gcSTD_VS_ep_HS_bulk, sizeof(STD_EP_DSCR));
#endif
	

#if _PAYLOAD_PIPE_ == _PAYLOAD_PIPE_ISO_
	if(gsDeviceStatus.bUsb_HighSpeed && flag)	//hs
	{
		
		#if 1
		AddRec(&gcSTD_VS_IF1_HS_desc1,sizeof(STD_IF_DSCR));
		AddRec(&gcSTD_VS_ep_HS_desc1,sizeof(STD_EP_DSCR));	
		AddRec(&gcSTD_VS_IF1_HS_desc2,sizeof(STD_IF_DSCR));
		AddRec(&gcSTD_VS_ep_HS_desc2,sizeof(STD_EP_DSCR));	
		AddRec(&gcSTD_VS_IF1_HS_desc3,sizeof(STD_IF_DSCR));
		AddRec(&gcSTD_VS_ep_HS_desc3,sizeof(STD_EP_DSCR));	
		AddRec(&gcSTD_VS_IF1_HS_desc4,sizeof(STD_IF_DSCR));
		AddRec(&gcSTD_VS_ep_HS_desc4,sizeof(STD_EP_DSCR));	
		AddRec(&gcSTD_VS_IF1_HS_desc5,sizeof(STD_IF_DSCR));
		AddRec(&gcSTD_VS_ep_HS_desc5,sizeof(STD_EP_DSCR));	
		AddRec(&gcSTD_VS_IF1_HS_desc6,sizeof(STD_IF_DSCR));
		AddRec(&gcSTD_VS_ep_HS_desc6,sizeof(STD_EP_DSCR));	
		AddRec(&gcSTD_VS_IF1_HS_desc7,sizeof(STD_IF_DSCR));
		AddRec(&gcSTD_VS_ep_HS_desc7,sizeof(STD_EP_DSCR));	
		AddRec(&gcSTD_VS_IF1_HS_desc8,sizeof(STD_IF_DSCR));
		AddRec(&gcSTD_VS_ep_HS_desc8,sizeof(STD_EP_DSCR));	
		AddRec(&gcSTD_VS_IF1_HS_desc9,sizeof(STD_IF_DSCR));
		AddRec(&gcSTD_VS_ep_HS_desc9,sizeof(STD_EP_DSCR));	
		AddRec(&gcSTD_VS_IF1_HS_desc10,sizeof(STD_IF_DSCR));
		AddRec(&gcSTD_VS_ep_HS_desc10,sizeof(STD_EP_DSCR));
		#endif
		AddRec(&gcSTD_VS_IF1_HS_desc11,sizeof(STD_IF_DSCR));
		AddRec(&gcSTD_VS_ep_HS_desc11,sizeof(STD_EP_DSCR));
		((STD_EP_DSCR *)(pDesc-sizeof(STD_EP_DSCR)))->bmAttributes = 0x05;
		// ((STD_EP_DSCR *)(pDesc-sizeof(STD_EP_DSCR)))->wMaxPacketSize = USB20_MAX_VIDEO_ISO_PACKET_SIZE;
		((STD_EP_DSCR *)(pDesc-sizeof(STD_EP_DSCR)))->wMaxPacketSize[0] = (BYTE)(USB20_MAX_VIDEO_ISO_PACKET_SIZE & 0xFF);
		((STD_EP_DSCR *)(pDesc-sizeof(STD_EP_DSCR)))->wMaxPacketSize[1] = (BYTE)(USB20_MAX_VIDEO_ISO_PACKET_SIZE >> 8);
	}
	else		//fs or other speed
	{
		AddRec(&gcSTD_VS_IF1_FS_desc1,sizeof(STD_IF_DSCR));	//test
		((STD_IF_DSCR *)(pDesc-sizeof(STD_IF_DSCR)))->bAlternateSetting = 0x01; //test
		AddRec(&gcSTD_VS_ep_FS_desc1,sizeof(STD_EP_DSCR)); //test
		((STD_EP_DSCR *)(pDesc-sizeof(STD_EP_DSCR)))->bmAttributes = 0x01;
		AddRec(&gcSTD_VS_IF1_FS_desc2,sizeof(STD_IF_DSCR));	//test
		((STD_IF_DSCR *)(pDesc-sizeof(STD_IF_DSCR)))->bAlternateSetting = 0x02; //test
		AddRec(&gcSTD_VS_ep_FS_desc2,sizeof(STD_EP_DSCR)); //test
		((STD_EP_DSCR *)(pDesc-sizeof(STD_EP_DSCR)))->bmAttributes = 0x01;
		AddRec(&gcSTD_VS_IF1_FS_desc3,sizeof(STD_IF_DSCR));	//test
		((STD_IF_DSCR *)(pDesc-sizeof(STD_IF_DSCR)))->bAlternateSetting = 0x03; //test
		AddRec(&gcSTD_VS_ep_FS_desc3,sizeof(STD_EP_DSCR)); //test
		((STD_EP_DSCR *)(pDesc-sizeof(STD_EP_DSCR)))->bmAttributes = 0x01;
		AddRec(&gcSTD_VS_IF1_FS_desc4,sizeof(STD_IF_DSCR));	//test
		((STD_IF_DSCR *)(pDesc-sizeof(STD_IF_DSCR)))->bAlternateSetting = 0x04; //test
		AddRec(&gcSTD_VS_ep_FS_desc4,sizeof(STD_EP_DSCR)); //test
		((STD_EP_DSCR *)(pDesc-sizeof(STD_EP_DSCR)))->bmAttributes = 0x01;
		AddRec(&gcSTD_VS_IF1_FS_desc5,sizeof(STD_IF_DSCR));	//test
		((STD_IF_DSCR *)(pDesc-sizeof(STD_IF_DSCR)))->bAlternateSetting = 0x05; //test
		AddRec(&gcSTD_VS_ep_FS_desc5,sizeof(STD_EP_DSCR)); //test
		((STD_EP_DSCR *)(pDesc-sizeof(STD_EP_DSCR)))->bmAttributes = 0x01;
		AddRec(&gcSTD_VS_IF1_FS_desc6,sizeof(STD_IF_DSCR));	//test
		((STD_IF_DSCR *)(pDesc-sizeof(STD_IF_DSCR)))->bAlternateSetting = 0x06; //test
		AddRec(&gcSTD_VS_ep_FS_desc6,sizeof(STD_EP_DSCR)); //test
		((STD_EP_DSCR *)(pDesc-sizeof(STD_EP_DSCR)))->bmAttributes = 0x01;
		AddRec(&gcSTD_VS_IF1_FS_desc7,sizeof(STD_IF_DSCR));	//test
		((STD_IF_DSCR *)(pDesc-sizeof(STD_IF_DSCR)))->bAlternateSetting = 0x07; //test
		AddRec(&gcSTD_VS_ep_FS_desc7,sizeof(STD_EP_DSCR)); //test
		((STD_EP_DSCR *)(pDesc-sizeof(STD_EP_DSCR)))->bmAttributes = 0x01;

		// ((STD_EP_DSCR *)(pDesc-sizeof(STD_EP_DSCR)))->wMaxPacketSize = USB11_MAX_VIDEO_ISO_PACKET_SIZE;
		((STD_EP_DSCR *)(pDesc-sizeof(STD_EP_DSCR)))->wMaxPacketSize[0] = (USB11_MAX_VIDEO_ISO_PACKET_SIZE & 0xFF);
		((STD_EP_DSCR *)(pDesc-sizeof(STD_EP_DSCR)))->wMaxPacketSize[1] = (USB11_MAX_VIDEO_ISO_PACKET_SIZE >> 8);
		
	}
#endif

	if(gsDeviceStatus.bMicSel)	//have audio
	{
		//如果需要声音，执行下述代码
		AddRec(&gcAudio_IAD_desc,sizeof(AUDIO_IAD));
		AddRec(&gcSTD_ACIF_desc,sizeof(STD_IF_DSCR));
		AddRec(&gcCS_AcHead_desc,sizeof(CS_ACHEAD_DSCR));
		AddRec(&gcAudio_IT_desc,sizeof(AIN_TERM_DSCR));
		AddRec(&gcAudio_OT_desc,sizeof(AOUT_TERM_DSCR));
		AddRec(&gcFeater_desc,sizeof(FEA_UNIT_DSCR));
		AddRec(&gcSTD_as0_desc,sizeof(STD_IF_DSCR));
		AddRec(&gcSTD_as1_desc,sizeof(STD_IF_DSCR));
		AddRec(&gcCS_as_general_desc,sizeof(CS_AS_GENE));
		AddRec(&gcCS_as_format_desc,sizeof(AS_FORMAT_DSCR));
		AddRec(&gcSTD_as_ep_desc,sizeof(AUD_EP_DSCR));
		// ((AUD_EP_DSCR *)(pDesc-sizeof(AUD_EP_DSCR)))->wMaxPacketSize = MAX_AUDIO_PAYLOAD_SIZE ;
		((AUD_EP_DSCR *)(pDesc-sizeof(AUD_EP_DSCR)))->wMaxPacketSize[0] = (BYTE)(MAX_AUDIO_PAYLOAD_SIZE & 0xFF) ;
		((AUD_EP_DSCR *)(pDesc-sizeof(AUD_EP_DSCR)))->wMaxPacketSize[1] = (BYTE)(MAX_AUDIO_PAYLOAD_SIZE >> 8) ;
		
		if(gsDeviceStatus.bUsb_HighSpeed && flag)		//hs & normal speed
		{
			// ((AUD_EP_DSCR *)(pDesc-sizeof(AUD_EP_DSCR)))->wInterval = AUDIO_INTERVAL_HS ;
			((AUD_EP_DSCR *)(pDesc-sizeof(AUD_EP_DSCR)))->wInterval[0] = (BYTE)(AUDIO_INTERVAL_HS) ;
			((AUD_EP_DSCR *)(pDesc-sizeof(AUD_EP_DSCR)))->wInterval[1] = (BYTE)(AUDIO_INTERVAL_HS >> 8) ;
		}
		else
		{
			// ((AUD_EP_DSCR *)(pDesc-sizeof(AUD_EP_DSCR)))->wInterval = AUDIO_INTERVAL_FS ;
			((AUD_EP_DSCR *)(pDesc-sizeof(AUD_EP_DSCR)))->wInterval[0] = (BYTE)(AUDIO_INTERVAL_FS) ;
			((AUD_EP_DSCR *)(pDesc-sizeof(AUD_EP_DSCR)))->wInterval[1] = (BYTE)(AUDIO_INTERVAL_FS >> 8) ;
		}

		AddRec(&gcCS_as_ep_desc,sizeof(CS_EP_DSCR));
	}
	else  //如无声音，则将接口数减为2
	{
		((STD_CFG_DSCR *)(pDesc-nDescLength))->bNumInterfaces = 2;
	}

	//按实际配置描述符计算配置描述符总长度
	// ((STD_CFG_DSCR *)(pDesc-nDescLength))->wTotalLength = (WORD)(nDescLength << 8) + (WORD)(nDescLength >> 8);
	((STD_CFG_DSCR *)(pDesc-nDescLength))->wTotalLength[0] = (BYTE)(nDescLength & 0xFF);
	((STD_CFG_DSCR *)(pDesc-nDescLength))->wTotalLength[1] = (BYTE)(nDescLength >> 8);

	return nDescLength;
}


void frameIdx_sel(BYTE bFormatIdx)
{
	if(gsDeviceStatus.bUsb_HighSpeed)	//hs
	{
		if(gsDeviceStatus.bDualFormatSel)	//单格式, yuv or mjpeg
		{
			if(gsDeviceStatus.bFormatSel)	//yuv
				bFlg_mjpeg = 0;
			else		//mjpeg
				bFlg_mjpeg = 1;
		}
		else	//双格式, yuv+mjpeg	
		{
			if(bDualFormatOrder)
			{
				if(bFormatIdx == 1)	//yuv
					bFlg_mjpeg = 0;
				if(bFormatIdx == 2)	//mjpeg
					bFlg_mjpeg = 1;
			}
			else
			{
				if(bFormatIdx == 2)	//yuv
					bFlg_mjpeg = 0;
				if(bFormatIdx == 1)	//mjpeg
					bFlg_mjpeg = 1;
			}
		}
	}
	else		//fs
	{
		bFlg_mjpeg = 1;
	}


	{
		(*pFunc_SensorSetDspWin)();
	}
}
