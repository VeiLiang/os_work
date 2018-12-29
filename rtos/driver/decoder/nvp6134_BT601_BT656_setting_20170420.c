#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/fcntl.h>
#include <linux/mm.h>
#include <linux/proc_fs.h>

#ifndef CONFIG_HISI_SNAPSHOT_BOOT
#include <linux/miscdevice.h>
#endif

#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <asm/system.h>
#include <linux/interrupt.h>
#include <linux/ioport.h>
#include <linux/string.h>
#include <linux/list.h>
#include <asm/delay.h>
#include <linux/timer.h>
#include <linux/delay.h>
#include <linux/proc_fs.h>
#include <linux/poll.h>
#include <asm/bitops.h>
#include <asm/uaccess.h>
#include <asm/irq.h>
#include <linux/moduleparam.h>
#include <linux/ioport.h>
#include <linux/interrupt.h>
#include <linux/semaphore.h>
#include <linux/kthread.h>

#ifdef I2C_INTERNAL
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#endif

//#include "gpio_i2c.h"
#include "video.h"
#include "coax.h"
#include "motion.h"
#include "common.h"
#include "audio.h"
#include "acp.h"
#include "acp_firmup.h"
#include "eq.h"

nvp6134_equalizer s_eq;	

#ifdef CONFIG_HISI_SNAPSHOT_BOOT
#include "himedia.h"
#define DEV_NAME "nvp6134"
#endif

#ifdef I2C_INTERNAL
static struct i2c_board_info hi_info =
{
    I2C_BOARD_INFO("nvp6134", 0x60),
};

static struct i2c_client* nvp6134_client;
#endif


#ifdef CONFIG_HISI_SNAPSHOT_BOOT
static struct himedia_device s_stNvp6134Device;
#endif

#define DRIVER_VER "17.02.09.01"

#define SET_BIT(data,bit) ((data)|=(1<<(bit)))
#define CLE_BIT(data,bit) ((data)&=(~(1<<(bit))))


#ifdef HI_I2C   
unsigned char __I2CReadByte8(unsigned char devaddress, unsigned char address)
{
    return (HI_I2C_Read(devaddress, address, 1, 1) & 0xff);
}

void __I2CWriteByte8(unsigned char devaddress, unsigned char address, unsigned char data)
{
    HI_I2C_Write(devaddress, address, 1, data, 1);
	udelay(200);
}
#endif

#ifdef I2C_INTERNAL
void __I2CWriteByte8(unsigned char chip_addr, unsigned char reg_addr, unsigned char value)
{
    int ret;
    unsigned char buf[2];
    struct i2c_client* client = nvp6134_client;
    
    nvp6134_client->addr = chip_addr;

    buf[0] = reg_addr;
    buf[1] = value;

    ret = i2c_master_send(client, buf, 2);
    udelay(300);
    //return ret;
}

unsigned char __I2CReadByte8(unsigned char chip_addr, unsigned char reg_addr)
{
    int ret_data = 0xFF;
    int ret;
    struct i2c_client* client = nvp6134_client;
    unsigned char buf[2];

    nvp6134_client->addr = chip_addr;

    buf[0] = reg_addr;
    ret = i2c_master_recv(client, buf, 1);
    if (ret >= 0)
    {
        ret_data = buf[0];
    }
    return ret_data;
}
#endif

#if (defined HI_CHIP_HI3521A)||(defined HI_CHIP_HI3531A)
#define HI_CHIPID_BASE 0x12050000
#else
#define HI_CHIPID_BASE 0x20050000
#endif
#define HI_CHIPID0 IO_ADDRESS(HI_CHIPID_BASE + 0xEEC)
#define HI_CHIPID1 IO_ADDRESS(HI_CHIPID_BASE + 0xEE8)
#define HI_CHIPID2 IO_ADDRESS(HI_CHIPID_BASE + 0xEE4)
#define HI_CHIPID3 IO_ADDRESS(HI_CHIPID_BASE + 0xEE0)
#define HW_REG(reg)         *((volatile unsigned int *)(reg))

int g_soc_chiptype=0x3521;
int chip_id[4];
int rev_id[4];
unsigned int nvp6134_mode = PAL;  //0:ntsc, 1: pal
unsigned int nvp6134_cnt = 0;
unsigned int nvp6134_iic_addr[4] = {0x60, 0x62, 0x64, 0x66};
module_param(nvp6134_mode, uint, S_IRUGO);
struct semaphore nvp6134_lock;
unsigned char det_mode[16];
unsigned char ch_mode_status[16];
unsigned char ch_vfmt_status[16];
unsigned char acp_isp_wr_en[16];

void nvp6134_datareverse(unsigned char chip, unsigned char port);
static struct task_struct *nvp6134_kt = NULL;


void nvp6134_datareverse(unsigned char chip, unsigned char port)
{
/*
BANK1 0xCB[3:0],每个bit控制一个bt656的数据顺序，1为反序，0为正序。
*/
	unsigned char tmp;
	gpio_i2c_write(nvp6134_iic_addr[chip], 0xFF, 0x01);
	tmp = gpio_i2c_read(nvp6134_iic_addr[chip], 0xCB);
	SET_BIT(tmp, port);
	gpio_i2c_write(nvp6134_iic_addr[chip], 0xCB, tmp); 
	printk("nvp6134[%d] port[%d] data reversed\n", chip, port);
}

// device address define
#define NVP6134_R0_ID 		0x91
#define NVP6134B_R0_ID 		0x90   //6134B AND 6134C USES THE SAME CHIPID,DIFF IN REV_ID
#define NVP6134B_REV_ID 	0x00
#define NVP6134C_REV_ID 	0x01
#define CH_PER_CHIP			4

#define NTSC		0x00
#define PAL			0x01

/*0x00:CVBS mode;  0x01:720P mode; 0x02:1080p mode  */
unsigned int SINGAL_HD = 0x01; 

/*0x00: BT656 mode ; 0x01:bt601 mode ; 0x02: bt1120 mode  */
unsigned int BT656_BT601_SEL = 0x00; 

/*0x00: need  init ; 0x01: not need  init  */
unsigned int NVP6134_INIT_ON_OFF = 0x01; 


typedef enum _nvp6134_vi_mode
{
	NVP6134_VI_720H			= 0x00,	//720x576i(480)
	NVP6134_VI_960H,       			//960x576i(480)
	NVP6134_VI_720P_2530	= 0x10,	//1280x720@25p(30)
	NVP6134_VI_720P_5060,			//1280x720@50p(60)
	NVP6134_VI_1080P_2530	= 0x20,	//1920x1080@25p(30)
	NVP6134_VI_1080P_NOVIDEO,		//1920x1080@25p(30)	
	NVP6134_VI_BUTT
}NVP6134_VI_MODE;

/* max channel number */
#define MAX_CH_NUM				16

/********************************************************************
 *  structure
 ********************************************************************/
/* EQ structure, this structure shared with application */
typedef struct _nvp6134_equalizer_
{
	
	unsigned char ch_equalizer_type[MAX_CH_NUM];	//0->auto, 1->manually, 2->nextchip test auto
	unsigned char ch_detect_type[MAX_CH_NUM];	//0->auto, 1->AHD, 2->CHD, 3->THD, 4->CVBS
	unsigned char ch_cable_type[MAX_CH_NUM];	//CABLE_TYPE_COAX[0], CABLE_TYPE_UTP[1]
	unsigned char ch_previdmode[MAX_CH_NUM];	// pre video mode
	unsigned char ch_curvidmode[MAX_CH_NUM];	// current video mode
	unsigned char ch_previdon[MAX_CH_NUM];		// pre video on/off status
	unsigned char ch_curvidon[MAX_CH_NUM];		// current video on/off status
	unsigned char ch_previdformat[MAX_CH_NUM];	// pre video format for Auto detection value
	unsigned char ch_curvidformat[MAX_CH_NUM];	// current video format for Auto detection value


	int acc_gain[MAX_CH_NUM];					// first, get value: acc gain(value when video is ON status)
	int y_plus_slope[MAX_CH_NUM];				// first, get value: y plus slope(value when video is ON status)
	int y_minus_slope[MAX_CH_NUM];				// first, get value: y minus slope(value when video is ON status)

	int cur_y_plus_slope[MAX_CH_NUM];			// current y plus slope
	int cur_acc_gain[MAX_CH_NUM];				// current acc gain
	int cur_y_minus_slope[MAX_CH_NUM];			// current y minus slope

	int fr_ac_min_value[MAX_CH_NUM];			// first AC Min Value
	int fr_ac_max_value[MAX_CH_NUM];			// first AC Max Value
	int fr_dc_value[MAX_CH_NUM];				// first DC value

	int cur_fr_ac_min[MAX_CH_NUM];				// current y plus slope
	int cur_fr_ac_max[MAX_CH_NUM];				// current acc gain
	int cur_fr_dc[MAX_CH_NUM];					// current y minus slope

	int fr_sync_width[MAX_CH_NUM];
	int cur_sync_width[MAX_CH_NUM];

	unsigned char ch_stage[MAX_CH_NUM];			// stage of channel
	unsigned char ch_vfmt_status[MAX_CH_NUM];	// NTSC/PAL

}nvp6134_equalizer;


typedef enum _nvp6134_outmode_sel
{
	NVP6134_OUTMODE_1MUX_SD = 0,
	NVP6134_OUTMODE_1MUX_HD,
	NVP6134_OUTMODE_1MUX_HD5060,
	NVP6134_OUTMODE_1MUX_FHD,
	NVP6134_OUTMODE_2MUX_SD,
	NVP6134_OUTMODE_2MUX_HD_X,
	NVP6134_OUTMODE_2MUX_HD,
	NVP6134_OUTMODE_2MUX_FHD_X,
	NVP6134_OUTMODE_4MUX_SD,
	NVP6134_OUTMODE_4MUX_HD_X,
	NVP6134_OUTMODE_4MUX_HD,
	NVP6134_OUTMODE_2MUX_FHD,
	NVP6134_OUTMODE_1MUX_HD_X,  
	NVP6134_OUTMODE_1MUX_FHD_X,
	NVP6134_OUTMODE_4MUX_FHD_X,
	NVP6134_OUTMODE_4MUX_MIX,
	NVP6134_OUTMODE_2MUX_MIX,
	NVP6134_OUTMODE_1MUX_BT1120S,
	NVP6134_OUTMODE_1MUX_3M_RT,
	NVP6134_OUTMODE_1MUX_4M_NRT,
	NVP6134_OUTMODE_BUTT
}NVP6134_OUTMODE_SEL;



/* for baud rate */
#define ACP_PACKET_MODE			0x0B
#define ACP_AHD2_FHD_D0			0x10
#define ACP_AHD2_PEL_BAUD		0x02
#define ACP_AHD2_PEL_LINE		0x07
#define ACP_AHD2_PEL_SYNC		0x0D
#define ACP_AHD2_PEL_EVEN		0x2F
#define ACP_AHD2_FHD_BAUD		0x00
#define ACP_AHD2_FHD_LINE		0x03
#define ACP_AHD2_FHD_LINES		0x05
#define ACP_AHD2_FHD_BYTE		0x0A
#define ACP_AHD2_FHD_MODE		0x0B
#define ACP_AHD2_FHD_OUT		0x09
#define ACP_CLR_REG				0x3A


/*******************************************************************************
*	Description		: ?
*	Argurments		: ch(channel ID),
*	Return value	: void
*	Modify			:
*	warning			:
*******************************************************************************/
void acp_reg_rx_clear(unsigned char ch)
{
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xFF, 0x03+((ch%4)/2));
	gpio_i2c_write(nvp6134_iic_addr[ch/4], ACP_CLR_REG+((ch%2)*0x80), 0x01);
	msleep(10);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], ACP_CLR_REG+((ch%2)*0x80), 0x00);
	msleep(200);
}

/*******************************************************************************
*	Description		: Initialize ACP each CHIP ID
*	Argurments		: ch(channel ID)
*	Return value	: void
*	Modify			:
*	warning			: Now, The Chip ID of NVP6134 and NVP6134B is 0x90
*******************************************************************************/
void init_acp(unsigned char ch)
{
	acp_reg_rx_clear(ch);
}



/*******************************************************************************
*	Description		: Get rev ID
*	Argurments		: dec(slave address)
*	Return value	: rev ID
*	Modify			:
*	warning			:
*******************************************************************************/
int check_rev(unsigned int dec)
{
	int ret;
	gpio_i2c_write(dec, 0xFF, 0x00);
	ret = gpio_i2c_read(dec, 0xf5);
	return ret;
}

/*******************************************************************************
*	Description		: Get Device ID
*	Argurments		: dec(slave address)
*	Return value	: Device ID
*	Modify			:
*	warning			:
*******************************************************************************/
int check_id(unsigned int dec)
{
	int ret;
	gpio_i2c_write(dec, 0xFF, 0x00);
	ret = gpio_i2c_read(dec, 0xf4);
	return ret;
}


int nvp6134_open(struct inode * inode, struct file * file)
{
	return 0;
} 

int nvp6134_close(struct inode * inode, struct file * file)
{
	return 0;
}

int get_hichipid(void)
{
	g_soc_chiptype = (HW_REG(HI_CHIPID0)&0xFF)<<8 | (HW_REG(HI_CHIPID1)&0xFF);
	printk("g_soc_chiptype ==> %x\n", g_soc_chiptype);
	return g_soc_chiptype;
}

unsigned char g_coax_ch;
unsigned int g_vloss=0xFFFF;

static struct file_operations nvp6134_fops = {
	.owner      = THIS_MODULE,
    .unlocked_ioctl	= nvp6134_ioctl,
	.open       = nvp6134_open,
	.release    = nvp6134_close
};


void nvp6134_system_init(unsigned char chip)
{
	gpio_i2c_write(nvp6134_iic_addr[chip], 0xFF, 0x00);
	gpio_i2c_write(nvp6134_iic_addr[chip], 0x80, 0x0F);

	gpio_i2c_write(nvp6134_iic_addr[chip], 0xFF, 0x01); 
	if(chip_id[chip] == NVP6134B_R0_ID)
		gpio_i2c_write(nvp6134_iic_addr[chip], 0xCA, 0x66);  	//NVP6134C/6134B ONLY HAS 2 PORTS
	else
		gpio_i2c_write(nvp6134_iic_addr[chip], 0xCA, 0xFF);		//NVP6134 HAS 4 PORTS

	printk("nvp6134(C)_system_init\n");
}


/*******************************************************************************
*	Description		: Initialize common value of AHD
*	Argurments		: dec(slave address)
*	Return value	: rev ID
*	Modify			:
*	warning			:
*******************************************************************************/
void nvp6134_common_init(unsigned char chip)
{	
	int ch;
	/* initialize chip */
	nvp6134_system_init(chip);

	if(NVP6134_INIT_ON_OFF==0x00)  //default  initial 
	{
		for(ch=0;ch<4;ch++)
		{
			gpio_i2c_write(nvp6134_iic_addr[chip], 0xFF, 0x03+(ch%4)/2);
			gpio_i2c_write(nvp6134_iic_addr[chip], 0x6B+0x80*(ch%2), 0x00);

			init_acp(ch);
		}
	}
}



void nvp6134_setchn_common_cvbs(const unsigned char ch, const unsigned char vfmt)
{
	unsigned char YCmerge, PN_set;

	//analog setting
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xFF, 0x00);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x00+ch%4,0x00); 

	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xFF, 0x05+ch%4);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x00,0xC0);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x01,0x00); 
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x58,0x00); 
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x59,0x00); 
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x5B,0x03); 

	//common image setting
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xFF, 0x00);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x08+ch%4,vfmt==PAL?0xDD:0xA0);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x0c+ch%4,0x08);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x10+ch%4,vfmt==PAL?0x88:0x88);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x14+ch%4,0x90);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x18+ch%4,0x08);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x21+(ch%4)*4,vfmt==PAL?0x02:0x82);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x3C+ch%4,0x84);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x44+ch%4,0x00);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x48+ch%4,0x00);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x4c+ch%4,0x00);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x50+ch%4,0x00);
	
	//BT656 or BT1120 SET
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xFF, 0x01);
	YCmerge = gpio_i2c_read(nvp6134_iic_addr[ch/4], 0xed);
	CLE_BIT(YCmerge, (ch%4));
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xed, YCmerge);

	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xFF, 0x05+ch%4);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x69,0x00);

	//SYNC_Detection_Setting
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xFF, 0x05+ch%4);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x47,vfmt==PAL?0x04:0x04);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x50,vfmt==PAL?0x84:0x84);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x84,0x00);	// ( no video option - sync enable)
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x86,0x00);	// ( sync width )
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xd1,vfmt==PAL?0x10:0x00);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x57, 0x00);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x90, 0x01);
	
	//common image hidden
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xFF, 0x05+ch%4);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x08,0x50);     
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x11,0x04);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x1b,vfmt==PAL?0x20:0x20);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x24,vfmt==PAL?0x10:0x2A);  
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x25,vfmt==PAL?0xCA:0xDA);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x26,0x30);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x29,0x30); 
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x2a,0x52); 
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x2b, 0xa8);					

	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x5f,0x70);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x56,vfmt==PAL?0x00:0x00);	
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x90,vfmt==PAL?0x0D:0x01);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x9b,0x80);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xb5,0x00);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xb7,0xff);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xb8,0xB8);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xbb,vfmt==PAL?0xb8:0xb8);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xd1,vfmt==PAL?0x20:0x00);  

	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x20,vfmt==PAL?0x84:0x84);	
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x27,vfmt==PAL?0x57:0x57);	
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x76,vfmt==PAL?0x01:0x01);	


	//V_DELAY setting	
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xFF, 0x05+ch%4);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x6E,vfmt==PAL?0x00:0x00);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x6F,vfmt==PAL?0x00:0x00);

	//each format FSC
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xff,0x09);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x40+(ch%4),  vfmt==PAL?0x00:0x00);
	PN_set = gpio_i2c_read(nvp6134_iic_addr[ch/4], 0x44);
	CLE_BIT(PN_set, (ch%4));
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x44, PN_set);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x50+(ch%4)*4,vfmt==PAL?0xCB:0x1E);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x51+(ch%4)*4,vfmt==PAL?0x8A:0x7C);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x52+(ch%4)*4,vfmt==PAL?0x09:0xF0);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x53+(ch%4)*4,vfmt==PAL?0x2A:0x21);	
	// H_SCAILER	
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x97+((ch%4)*0x20),0x00);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x98+((ch%4)*0x20),0x00);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x99+((ch%4)*0x20),0x00);

	// Motion setting     
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xFF, 0x02);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x02+((ch%4)*0x07),0x03);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x28+((ch%4)*0x06),0x01);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x29+((ch%4)*0x06),0x3C);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x2A+((ch%4)*0x06),vfmt==PAL?0x0C:0x0A);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x2B+((ch%4)*0x06),0x06);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x2C+((ch%4)*0x06),0x36);

	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xFF, 0x11);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x00+(ch%4)*0x20,0x00);
}


void nvp6134_setchn_common_720p(const unsigned char ch, const unsigned char vfmt)
{
	unsigned char YCmerge, PN_set;
	printk(">>>>> nvp6134_setchn_common_720p(%s), <<<<<\n", (vfmt==PAL) ? "PAL" : "NTSC" );

	//analog setting
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xFF, 0x00);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x00+ch%4,0x00); 

	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xFF, 0x05+ch%4);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x00,0xC0);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x01,0x02);  
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x58,0x02);  
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x59,0x00);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x5B,0x03);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x69,0x00);

	//common image setting
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xFF, 0x00);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x0c+ch%4,0x08);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x10+ch%4,vfmt==PAL?0x8D:0x8B);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x14+ch%4,0x90);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x18+ch%4,0x20);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x21+(ch%4)*4,0x82); //reduce color filter's bandwidth
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x3C+ch%4,0x84);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x40+ch%4,vfmt==PAL?0x01:0xF7);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x44+ch%4,vfmt==PAL?0x20:0x1A);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x48+ch%4,vfmt==PAL?0x11:0x11);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x4c+ch%4,vfmt==PAL?0xF6:0xF4);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x50+ch%4,vfmt==PAL?0xF5:0xF6);
	
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xFF, 0x05+ch%4);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x47,vfmt==PAL?0x04:0x04);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x50,vfmt==PAL?0x84:0x84);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x20,vfmt==PAL?0x87:0x86);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x27,vfmt==PAL?0x57:0x57);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x76,vfmt==PAL?0x00:0x00);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x57, 0x00);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x90, 0x01);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x2b, 0xa8);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xb8, 0x38);                 


	//V_DELAY setting	
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xFF, 0x05+ch%4);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x6E,vfmt==PAL?0x00:0x00);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x6F,vfmt==PAL?0x00:0x00);

	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xFF, 0x01);
	YCmerge = gpio_i2c_read(nvp6134_iic_addr[ch/4], 0xed);
	CLE_BIT(YCmerge, (ch%4));
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xed, YCmerge);
	
	// Motion setting     
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xFF, 0x02);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x02+((ch%4)*0x07),0x23);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x28+((ch%4)*0x06),0x11);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x29+((ch%4)*0x06),0x50);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x2A+((ch%4)*0x06),0x1E);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x2B+((ch%4)*0x06),0x06);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x2C+((ch%4)*0x06),0x4A);

	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xff,0x09);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x40+(ch%4),  vfmt==PAL?0x00:0x00);
	PN_set = gpio_i2c_read(nvp6134_iic_addr[ch/4], 0x44);
	CLE_BIT(PN_set, (ch%4));
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x44, PN_set);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x97+((ch%4)*0x20),0x00);
	// H_EXT_MODE ON/OFF
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xFF, 0x11);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x00+(ch%4)*0x20,0x00);	
}

void nvp6134_setchn_common_fhd(const unsigned char ch, const unsigned char vfmt)
{
	unsigned char YCmerge, PN_set;

	printk(">>>>> NVP6124_VI_1080P_2530(%s), CH:%d, <<<<<\n", (vfmt==PAL) ? "PAL" : "NTSC", ch );
	//analog setting
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xFF, 0x00);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x00+ch%4,0x10);

	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xFF, 0x05+ch%4);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x00,0xC0);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x01,0x03);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x58,0x03);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x59,0x00);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x5B,0x03);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x69,0x00);

	//common image setting
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xFF, 0x00);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x0c+ch%4,0x08);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x10+ch%4,0x88);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x14+ch%4,0x90);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x18+ch%4,0x00);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x21+(ch%4)*4,0x82);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x22+(ch%4)*4,0x0b);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x3C+ch%4,0x84);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x44+ch%4,0x00);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x48+ch%4,0x00);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x4c+ch%4,0x00);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x50+ch%4,0x00);

	//common image hidden
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xFF, 0x05+ch%4);    
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x1b, 0x08);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x20,vfmt==PAL?0x84:0x84);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x26, 0xF0);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x27,vfmt==PAL?0x57:0x57);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x29, 0x00);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x76,vfmt==PAL?0x00:0x00);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x57, 0x00);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x5f, 0x00);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x90, 0x01);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x2b, 0xa8);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xD1, 0x00);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xD4, 0x00);                 
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xb8, 0xb9);	

	//V_DELAY setting	
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xFF, 0x00);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x64+ch%4,vfmt==PAL?0x80:0x80);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xFF, 0x05+ch%4);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x6E,vfmt==PAL?0x10:0x00);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x6F,vfmt==PAL?0x2a:0x2b);

	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xFF, 0x01);					
	YCmerge = gpio_i2c_read(nvp6134_iic_addr[ch/4], 0xed);
	CLE_BIT(YCmerge, (ch%4));
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xed, YCmerge);
	
	// Motion setting
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xFF, 0x02);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x02+((ch%4)*0x07),0x23);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x28+((ch%4)*0x06),0x10);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x29+((ch%4)*0x06),0x78);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x2A+((ch%4)*0x06),0x2D);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x2B+((ch%4)*0x06),0x06);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x2C+((ch%4)*0x06),0x72);
	//each format FSC
	gpio_i2c_write(nvp6134_iic_addr[ch/4],0xff,0x09);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x40+(ch%4),0x00);
	PN_set = gpio_i2c_read(nvp6134_iic_addr[ch/4], 0x44);
	CLE_BIT(PN_set, (ch%4));
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x44, PN_set);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x97+((ch%4)*0x20),0x00);

	// H_EXT_MODE ON/OFF
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xFF, 0x11);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x00+(ch%4)*0x20,0x00);	//(00=OFF / 10=H_EX_MODE_ON)
}

void nvp6134_setchn_720h(const unsigned char ch, const unsigned char vfmt)
{
	printk(">>>>> NVP6124_VI_720H(%s), CH:%d <<<<<\n", (vfmt==PAL) ? "PAL" : "NTSC", ch );
	nvp6134_setchn_common_cvbs(ch, vfmt);

	//each format basic clk
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xFF, 0x01);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x84+ch%4,vfmt==PAL?0x47:0x47);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x8c+ch%4,vfmt==PAL?0xA6:0xA6);

	//each format standard setting
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xFF, 0x00);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x40+ch%4,vfmt==PAL?0x00:0x00);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x81+ch%4,vfmt==PAL?0x70:0x60);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x85+ch%4,0x00);

	//each format delay
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xFF, 0x00);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x18+ch%4,0x14);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x30+ch%4,vfmt==PAL?0x10:0x10);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x58+ch%4,vfmt==PAL?0x30:0x40);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x5C+ch%4,vfmt==PAL?0x1E:0x1E);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x64+ch%4,vfmt==PAL?0x2D:0x28);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x89+ch%4,vfmt==PAL?0x10:0x10);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], (0x8e)+ch%4,vfmt==PAL?0x30:0x30);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xA0+ch%4,vfmt==PAL?0x18:0x08);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xA4+ch%4,0x00);

	//common image hidden
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xFF, 0x05+ch%4);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x64, 0x00);
	
	// Motion setting   
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xFF, 0x02);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x28+((ch%4)*0x06),0x00);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x29+((ch%4)*0x06),0x2D);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x2A+((ch%4)*0x06),vfmt==PAL?0x0C:0x0A);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x2C+((ch%4)*0x06),0x27);
}



void nvp6134_setchn_960h(const unsigned char ch, const unsigned char vfmt)
{
	printk(">>>>> NVP6124_VI_960H(%s), CH:%d <<<<<\n", (vfmt==PAL) ? "PAL" : "NTSC", ch );
	nvp6134_setchn_common_cvbs(ch, vfmt);

	//each format basic clk
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xFF, 0x01);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x84+ch%4,vfmt==PAL?0x47:0x47);	
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x8c+ch%4,vfmt==PAL?0xA6:0xA6);

	//each format standard setting
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xFF, 0x00);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x40+ch%4,vfmt==PAL?0x00:0x00);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x81+ch%4,vfmt==PAL?0x10:0x00);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x85+ch%4,0x00);

	//each format delay
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xFF, 0x00);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x30+ch%4,vfmt==PAL?0x10:0x10);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x58+ch%4,vfmt==PAL?0x80:0x80);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x5C+ch%4,vfmt==PAL?0x1E:0x1E);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x64+ch%4,vfmt==PAL?0x2D:0x28);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x89+ch%4,vfmt==PAL?0x10:0x10);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], (0x8e)+ch%4,vfmt==PAL?0x03:0x00);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xA0+ch%4,vfmt==PAL?0x08:0x08);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xA4+ch%4,0x00);
	
	//common image hidden
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xFF, 0x05+ch%4);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x64, 0x00);
	
	// Motion setting   
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xFF, 0x02);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x28+((ch%4)*0x06),0x01);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x29+((ch%4)*0x06),0x3C);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x2A+((ch%4)*0x06),vfmt==PAL?0x0C:0x0A);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x2C+((ch%4)*0x06),0x36);
}



void nvp6134_setchn_ahd_720p(const unsigned char ch, const unsigned char vfmt)
{
	printk(">>>>> nvp6134_setchn_ahd720p(%s), <<<<<\n", (vfmt==PAL) ? "PAL" : "NTSC" );
	nvp6134_setchn_common_720p(ch, vfmt);

	//each format basic clk
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xFF, 0x01);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x84+ch%4,vfmt==PAL?0x00:0x00);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x8c+ch%4,vfmt==PAL?0x0A:0x0A);

	//each format standard setting
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xFF, 0x00);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x08+ch%4,0x00);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x40+ch%4,vfmt==PAL?0x00:0x00);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x81+ch%4,vfmt==PAL?0x07:0x06);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x85+ch%4,0x00);

	//each format delay
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xFF, 0x00);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x30+ch%4,vfmt==PAL?0x08:0x08);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x58+ch%4,vfmt==PAL?0x80:0x80);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x5C+ch%4,vfmt==PAL?0x1E:0x1E);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x64+ch%4,vfmt==PAL?0x31:0x32);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x89+ch%4,vfmt==PAL?0x10:0x10);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], (0x8e)+ch%4,vfmt==PAL?0x00:0x00);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xA0+ch%4,0x00);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xA4+ch%4,0x09);

	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xff,0x09);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x97+((ch%4)*0x20),vfmt==PAL?0x00:0x00);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x98+((ch%4)*0x20),vfmt==PAL?0x00:0x00);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x99+((ch%4)*0x20),vfmt==PAL?0x00:0x00);

	//common image hidden
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xFF, 0x05+ch%4);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x1b,vfmt==PAL?0x08:0x08);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x24,vfmt==PAL?0x20:0x20);  
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x25,0xdc);  
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x26,0xF0);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x29,0x00);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x2a,0x52);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x5f,0x00);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x84,0x00);	// ( no video option - sync enable)
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x86,0x00);	// ( sync width )
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x90,0x01);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x9b,0x80);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xb5,0x00);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xb7,0xff);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xb8,0x38);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xbb,vfmt==PAL?0x04:0x04);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xd1,0x00);

	//each format FSC
	gpio_i2c_write(nvp6134_iic_addr[ch/4],0xff,0x09);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x50+(ch%4)*4,vfmt==PAL?0x45:0xED);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x51+(ch%4)*4,vfmt==PAL?0x08:0x00);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x52+(ch%4)*4,vfmt==PAL?0x10:0xE5);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x53+(ch%4)*4,vfmt==PAL?0x4F:0x4E);

	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xFF, 0x00);
}



void nvp6134_setchn_ahd_720p5060(const unsigned char ch, const unsigned char vfmt)
{
	printk(">>>>> nvp6134_setchn_ahd_720p5060(%s), <<<<<\n", (vfmt==PAL) ? "PAL" : "NTSC" );
	nvp6134_setchn_common_720p(ch, vfmt);

	//each format basic clk
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xFF, 0x01);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x84+ch%4,vfmt==PAL?0x00:0x00);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x8c+ch%4,vfmt==PAL?0x40:0x40);

	//each format standard setting
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xFF, 0x00);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x08+ch%4,0x00);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x0C+ch%4,0x00);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x10+ch%4,0x90);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x3C+ch%4,0x7C);

	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x40+ch%4,vfmt==PAL?0x03:0x03);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x44+ch%4,vfmt==PAL?0x10:0x10);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x48+ch%4,vfmt==PAL?0xF8:0xF8);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x4C+ch%4,vfmt==PAL?0xF2:0xF2);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x50+ch%4,vfmt==PAL?0xF6:0xF6);

	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x81+ch%4,vfmt==PAL?0x05:0x04);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x85+ch%4,0x00);

	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xFF, 0x05+ch%4);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x47,vfmt==PAL?0x04:0xEE);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x50,vfmt==PAL?0x86:0xC6);

	//each format delay
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xFF, 0x00);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x30+ch%4,vfmt==PAL?0x10:0x10);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x58+ch%4,vfmt==PAL?0x80:0x82);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x5C+ch%4,vfmt==PAL?0x1E:0x1E);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x64+ch%4,vfmt==PAL?0x30:0x31);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x89+ch%4,vfmt==PAL?0x00:0x00);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], (0x8e)+ch%4,vfmt==PAL?0x00:0x00);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xA0+ch%4,0x00);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xA4+ch%4,0x09);

	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xff,0x09);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x97+((ch%4)*0x20),vfmt==PAL?0x00:0x00);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x98+((ch%4)*0x20),vfmt==PAL?0x00:0x00);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x99+((ch%4)*0x20),vfmt==PAL?0x00:0x00);

	//common image hidden
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xFF, 0x05+ch%4);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x1b,vfmt==PAL?0x08:0x08);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x20,vfmt==PAL?0x82:0x82); 
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x24,vfmt==PAL?0x2A:0x20);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x25,0xdc);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x26,0x30);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x29,0x1F);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x2a,0x52);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x5f,0x00);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x84,0x01);	// ( no video option - sync enable)
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x86,0x30);	// ( sync width )
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x90,0x01);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x9b,0x80);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xb5,0x00);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xb7,vfmt==PAL?0xFF:0xFC);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xb8,0x38);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xbb,vfmt==PAL?0x04:0xFE);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xd1,0x00);

	//each format FSC
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xff,0x09);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x50+(ch%4)*4,0x2C);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x51+(ch%4)*4,0xE7);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x52+(ch%4)*4,0xCF);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x53+(ch%4)*4,0x52);

	// H_EXT_MODE ON/OFF
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xFF, 0x11);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x00+(ch%4)*0x20,0x00);	//(00=OFF / 10=H_EX_MODE_ON)
}



void nvp6134_setchn_ahd_1080p2530(const unsigned char ch, const unsigned char vfmt)
{
	printk(">>>>> nvp6134_setchn_ahd_1080p2530(%s), CH:%d, <<<<<\n", (vfmt==PAL) ? "PAL" : "NTSC", ch );
	nvp6134_setchn_common_fhd(ch, vfmt);

	//each format basic clk
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xFF, 0x01);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x84+ch%4,vfmt==PAL?0x00:0x00);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x8c+ch%4,vfmt==PAL?0x40:0x40);

	//each format standard setting
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xFF, 0x00);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x18+ch%4,0x30);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x08+ch%4,vfmt==PAL?0x00:0x00);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x40+ch%4,vfmt==PAL?0x00:0x00);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x81+ch%4,vfmt==PAL?0x03:0x02);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x85+ch%4,0x00);

	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xFF, 0x05+ch%4);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x47,vfmt==PAL?0x04:0xEE);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x50,vfmt==PAL?0x84:0xC6);
	
	//each format delay
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xFF, 0x00);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x30+ch%4,vfmt==PAL?0x10:0x10);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x58+ch%4,vfmt==PAL?0x7D:0x89);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x5C+ch%4,vfmt==PAL?0x9E:0x9E);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x64+ch%4,vfmt==PAL?0x80:0xC0);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x89+ch%4,vfmt==PAL?0x10:0x10);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], (0x8e)+ch%4,vfmt==PAL?0x00:0x00);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xA0+ch%4,vfmt==PAL?0x09:0x09);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xA4+ch%4,0x00);

	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xff,0x09);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x97+((ch%4)*0x20),vfmt==PAL?0x00:0x00);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x98+((ch%4)*0x20),vfmt==PAL?0x00:0x00);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x99+((ch%4)*0x20),vfmt==PAL?0x00:0x00);

	//common image hidden
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xFF, 0x05+ch%4);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x1b,vfmt==PAL?0x08:0x08);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x20,vfmt==PAL?0x90:0x90);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x24,vfmt==PAL?0x10:0x20);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x25,vfmt==PAL?0xDC:0xDC);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x26,0xF0);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x29,0x00);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x2a,0x52);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x58,0x03);	// Analog IP(bank[5~8], 0x58[7~4]:0(bypass), [3~0]:10M, 20M, 50M, 80M)

	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x5f,0x00);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x75,0x00);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x84,0x01);	// ( no video option - sync enable)
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x86,0x20);	// ( sync width )
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x90,vfmt==PAL?0x01:0x01);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x9b,0x80);		
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xb5,0x00);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xb7,vfmt==PAL?0xff:0xff);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xb8,0x38);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xbb,vfmt==PAL?0x04:0x04);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xd1,0x20);
	
	//each format FSC
	gpio_i2c_write(nvp6134_iic_addr[ch/4],0xff,0x09);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x50+(ch%4)*4,vfmt==PAL?0xAB:0x2C);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x51+(ch%4)*4,vfmt==PAL?0x7D:0xAF);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x52+(ch%4)*4,vfmt==PAL?0xC3:0xCA);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x53+(ch%4)*4,vfmt==PAL?0x52:0x52);
}



//视频丢失的时候，设置为此模式
void nvp6134_setchn_ahd_1080p_novideo(const unsigned char ch, const unsigned char vfmt)
{
	printk(">>>>> nvp6134_setchn_ahd_1080p_novideo(%s), CH:%d, <<<<<\n", (vfmt==PAL) ? "PAL" : "NTSC", ch );
	nvp6134_setchn_common_fhd(ch, vfmt);

	//each format basic clk
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xFF, 0x01);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x84+ch%4,vfmt==PAL?0x00:0x00);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x8c+ch%4,vfmt==PAL?0x40:0x40);

	//each format standard setting
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xFF, 0x00);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x08+ch%4,vfmt==PAL?0x00:0x00);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x23+(ch%4)*4,0x41);	//novideo detection 06.17
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x30+ch%4,0x10);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x81+ch%4,vfmt==PAL?0x03:0x02);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x85+ch%4,0x00);

	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xFF, 0x05+ch%4);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x47,vfmt==PAL?0x04:0x04);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x50,vfmt==PAL?0x86:0x86);	

	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xff,0x09);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x97+((ch%4)*0x20),vfmt==PAL?0x00:0x00);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x98+((ch%4)*0x20),vfmt==PAL?0x00:0x00);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x99+((ch%4)*0x20),vfmt==PAL?0x00:0x00);

	//common image hidden
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xFF, 0x05+ch%4);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x58,0x01);	//  (+) Analog IP(bank[5~8], 0x58[7~4]:0(bypass), [3~0]:10M, 20M, 50M, 80M)
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xb5,0x00);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xb7,vfmt==PAL?0xf0:0xff);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xb8,0xb9);  //16-06-27
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xbb,vfmt==PAL?0xbb:0x04);

	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xFF, 0x05+ch%4);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x84, 0x01);	// sync width - enable
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x86, 0x30);	// sync width - max value

	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xFF, 0x05+ch%4);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x24, 0x20);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x25, 0xDC);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x27, 0x57);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x5C, 0x00);  //clean status
	
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xFF, 0x00);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x00+ch%4,0x00);

	printk( ">>>>> DRV[%s:%d] CH:%d, NO video!!!!!\n", __func__, __LINE__, ch );
}






/*******************************************************************************
 *
 *
 *
 *  External Functions
 *
 *
 *
 *******************************************************************************/
/*******************************************************************************
*	Description		: EQ configuration value 
*	Argurments		:
*	Return value	: void
*	Modify			:
*	warning			:
*******************************************************************************/
void eq_init_each_format(unsigned char ch, int mode, const unsigned char vfmt )
{
    /* turn off Analog by-pass */
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xFF,0x05+ch%4);
    gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x59, 0x11 );
	
	s_eq.ch_stage[ch] = 0; //Set default stage to 0.
	
	switch( mode )
	{
		case NVP6134_VI_720H: 	  
		case NVP6134_VI_960H:  	  
			gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xFF,0x05+ch%4);
			gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x01,0x00);
			gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x58,0x00);
			gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x59,0x00);
			gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x5B,0x03);

			gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xFF,0x00);
			gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x00+ch%4,0x00);
			printk(">>>>> DRV : CH:%d, EQ init, NVP6134_VI_SD, NVP6134_VI_SD - conf\n", ch);
		break;
		case NVP6134_VI_720P_2530:   //HD AHD  @ 30P
		case NVP6134_VI_720P_5060:   //HD AHD  @ 25P
			gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xFF,0x05+ch%4);
			gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xC0,0x16);					// TX_PAT_STR
			gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xC1,0x13);					// ACC_GAIN_STS_SEL & TX_PAT_WIDTH
			gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xC8,0x04);					// SLOPE_VALUE_2S

			gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xFF,ch%4<2?0x0A:0x0B);
			gpio_i2c_write(nvp6134_iic_addr[ch/4], ch%2==0?0x74:0xF4,0x02);		// chX_EQ_SRC_SEL
			printk(">>>>> DRV : CH:%d, EQ init, NVP6134_VI_720P_2530, NVP6134_VI_720P_5060 - conf\n", ch);
		break;
		case NVP6134_VI_1080P_2530:   //FHD AHD
			gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xFF,0x05+ch%4);
			gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xC0,0x17);
			gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xC1,0x13);
			gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xC8,0x04);

			gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xFF,ch%4<2?0x0A:0x0B);
			gpio_i2c_write(nvp6134_iic_addr[ch/4], ch%2==0?0x74:0xF4,0x02);
			printk(">>>>> DRV : CH:%d, EQ init, NVP6134_VI_1080P_2530 - conf\n", ch);
		break;

	}
}




int nvp6134_set_bt601_mode(const unsigned char ch, const unsigned char vfmt, const unsigned char chnmode)
{  	
//	unsigned char chipaddr = nvp6134_iic_addr[chip];
	unsigned char tmp=0, tmp1=0, reg1=0, reg2=0;

	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xFF, 0x05+ch%4);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x40, 0xEF); 
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x42, 0x00);
	if(chnmode == NVP6134_VI_720P_2530)
	{
		gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x33, vfmt==PAL?0x02:0x02);	
		gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x34, vfmt==PAL?0x01:0x01);	
		gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x35, vfmt==PAL?0x1E:0x1E);	
		gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x36, vfmt==PAL?0x22:0x22);	
		gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x3b, vfmt==PAL?0x04:0x04);	
		gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x89, vfmt==PAL?0x00:0x00);	
		gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x8b, vfmt==PAL?0x00:0x00);	
		gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x9c, vfmt==PAL?0x01:0x01);	
		gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x9d, vfmt==PAL?0xA1:0xA1);	
		gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x9e, vfmt==PAL?0x0B:0x0B);	
		gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x9f, vfmt==PAL?0xA0:0xA0);	
	}
	else if(chnmode == NVP6134_VI_1080P_2530)
	{
		gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x33, vfmt==PAL?0x02:0x01);	
		gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x34, vfmt==PAL?0x01:0x01);	
		gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x35, vfmt==PAL?0x2D:0x2D);	
		gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x36, vfmt==PAL?0xB1:0xA5);	
		gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x3b, vfmt==PAL?0x04:0x04);	
		gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x89, vfmt==PAL?0x00:0x00);	
		gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x8b, vfmt==PAL?0x00:0x00);	
		gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x9c, vfmt==PAL?0x01:0x01);	
		gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x9d, vfmt==PAL?0x30:0x24);	
		gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x9e, vfmt==PAL?0x10:0x10);	
		gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x9f, vfmt==PAL?0x2F:0x23);	
	}

	if((chnmode == NVP6134_VI_720P_2530)||(chnmode == NVP6134_VI_1080P_2530))
	{
		gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xFF, 0x01);
		gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xA8, 0x01); //MPP_TEST_SEL1 
		gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xA9, 0x01); //MPP_TEST_SEL2 
		gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xAA, 0x02); //MPP_TEST_SEL3 
		gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xAB, 0x02); //MPP_TEST_SEL4 
		
		gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xBC, 0x00); //MPP_SEL1 
		gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xBD, 0x08); //MPP_SEL2 
		gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xBE, 0x00); //MPP_SEL3 
		gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xBF, 0x08); //MPP_SEL4
		gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xB3, 0x0F); //MPP_INV

		gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xFF, 0x01);
		gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xD8, 0x01); //MPP_C_TEST_SEL1 
		gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xD9, 0x01); //MPP_C_TEST_SEL2 
		gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xDA, 0x02); //MPP_C_TEST_SEL3 
		gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xDB, 0x02); //MPP_C_TEST_SEL4 
		
		gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xDC, 0x00); //MPP_C_SEL1 
		gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xDD, 0x08); //MPP_C_SEL2 
		gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xDE, 0x00); //MPP_C_SEL3 
		gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xDF, 0x08); //MPP_C_SEL4
		gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xD3, 0x0F); //MPP_C_INV
	}
}


/*******************************************************************************
*	Description		: set this value
*	Argurments		: ch(channel)
*	Return value	: void
*	Modify			:
*	warning			: You don't have to change these values.
*******************************************************************************/
void nvp6134_set_common_value(unsigned char ch, int mode)
{
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xFF, 0x00);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x00+(ch%4),0x10);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x22+(ch%4)*4,0x0B);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x23+(ch%4)*4,0x43);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x30+ch%4,0x10);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x34+ch%4,0x02); //08.25
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x93+ch%4,0x00);	//Hzoom off
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xA4+ch%4,0x00);

	/* Analog IP - EQ bypass 
	 * bank[5~8], 0x58[7~4]:0(bypass), [3~0]:10M, 20M, 50M, 80M */
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xFF, 0x05+ch%4);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x00, 0xC0); // Clamp speed
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x01, 0x02); // Backend Antialiasing Filter Bandwidth 50Mhz
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x08, 0x50);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x11, 0x06);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x23, 0x00); // PN init
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x2A, 0x52);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x58, 0x02);	// Analog IP(bank[5~8], 0x58[7~4]:0(bypass), [3~0]:10M, 20M, 50M, 80M)
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x59, 0x11); // Analog filter bypass( bypass on )
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xB8, 0xB9); // H-PLL No video option These will be recovery by EQ routine.

	/* for EQ(common) */
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xC8, 0x04);
	
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xFF, 0x0A+(ch%4)/2);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], ((ch%2)*0x80+0x74), 0x02);
	printk(">>>>> DRV : CHIPID:%d[sa:0x%X], CH:%d, init Analog IP-EQ bypass and EQ\n", ch/4, nvp6134_iic_addr[ch/4], ch );
	
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xFF, 0x09);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x64, 0xB8); 	//for thd A/B detection
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x6A, 0x18);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x6B, 0xFF); 
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x6C, 0xC0); 
	printk(">>>>> DRV : CHIPID:%d[sa:0x%X], CH:%d, Set VFC parameter\n", ch/4, nvp6134_iic_addr[ch/4], ch );

	/* Initialize Digital EQ - disable Digital EQ ( CH1=9x80, CH2=9xA0,  CH3=9xC0, CH4=9xE0 ) */
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xFF, 0x09);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x80+(0x20*(ch%4)), 0x00);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x81+(0x20*(ch%4)), 0x00);

	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xFF, 0x0A+(ch%4)/2);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x60+(ch%2)*0x80,0x02); //recovery bankA data
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x61+(ch%2)*0x80,0x01); 
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x62+(ch%2)*0x80,0x00); 
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x63+(ch%2)*0x80,0x01); 
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x64+(ch%2)*0x80,0x03); 
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x65+(ch%2)*0x80,0xA0); 
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x66+(ch%2)*0x80,0x04); 
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x67+(ch%2)*0x80,0x03); 
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x68+(ch%2)*0x80,0x03); 
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x69+(ch%2)*0x80,0x0B); 
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x6A+(ch%2)*0x80,0x0A); 
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x6B+(ch%2)*0x80,0x11); 
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x6C+(ch%2)*0x80,0x0D); 
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x6D+(ch%2)*0x80,0x06); 
	gpio_i2c_write(nvp6134_iic_addr[ch/4], (0x6E)+(ch%2)*0x80,0x27);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x6F+(ch%2)*0x80,0x00); 
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x70+(ch%2)*0x80,0x4E); 
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x71+(ch%2)*0x80,0x6D); 
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x72+(ch%2)*0x80,0x74); 
}



/*******************************************************************************
*	Description		: set each channel's baud rate of coax
*	Argurments		: ch(channel ID)
*	Return value	: void
*	Modify			:
*	warning			:
*******************************************************************************/
void acp_set_baudrate(unsigned char ch)
{
	/* set baud rate */
	
	if((ch_mode_status[ch] < NVP6134_VI_720P_2530))
	{
		gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xFF, 0x05+ch%4);
		gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x7C, 0x11);
		gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xFF, 0x03+((ch%4)/2));
		gpio_i2c_write(nvp6134_iic_addr[ch/4], ACP_AHD2_PEL_BAUD+((ch%2)*0x80), ch_vfmt_status[ch]==PAL? 0x1B:0x1B);
		gpio_i2c_write(nvp6134_iic_addr[ch/4], ACP_AHD2_PEL_LINE+((ch%2)*0x80), ch_vfmt_status[ch]==PAL? 0x0E:0x0E);
		gpio_i2c_write(nvp6134_iic_addr[ch/4], ACP_PACKET_MODE+((ch%2)*0x80), 0x06);
		gpio_i2c_write(nvp6134_iic_addr[ch/4], ACP_AHD2_PEL_SYNC+((ch%2)*0x80), ch_vfmt_status[ch]==PAL? 0x20:0xd4);
		gpio_i2c_write(nvp6134_iic_addr[ch/4], ACP_AHD2_PEL_SYNC+1+((ch%2)*0x80), ch_vfmt_status[ch]==PAL?0x06:0x05);
		gpio_i2c_write(nvp6134_iic_addr[ch/4], ACP_AHD2_PEL_EVEN+((ch%2)*0x80), 0x01);

		printk(">>>>> DRV[%s:%d] NVP6134_VI_CVBS COAXIAL PROTOCOL IS SETTING....\n", __func__, __LINE__ );
	}
	#ifdef AHD_PELCO_16BIT
	else if(ch_mode_status[ch] == NVP6134_VI_720P_2530 ) //||ch_mode_status[ch] == NVP6134_VI_HDEX)
	{
		gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xFF, 0x05+ch%4);
		gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x7C, 0x01);
		
		#if 0
		if(ch_mode_status[ch] == NVP6134_VI_HDEX)  //check AHDEX adc clock at the same time
		{
			gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xFF, 0x03+((ch%4)/2));
			gpio_i2c_write(nvp6134_iic_addr[ch/4], ACP_AHD2_PEL_BAUD+((ch%2)*0x80), ch_vfmt_status[ch]==PAL?0x26:0x26);
			gpio_i2c_write(nvp6134_iic_addr[ch/4], ACP_AHD2_PEL_LINE+((ch%2)*0x80), ch_vfmt_status[ch]==PAL?0x0E:0x0E);
			gpio_i2c_write(nvp6134_iic_addr[ch/4], ACP_AHD2_PEL_SYNC+((ch%2)*0x80), ch_vfmt_status[ch]==PAL? 0x00:0x00);
			gpio_i2c_write(nvp6134_iic_addr[ch/4], ACP_AHD2_PEL_SYNC+1+((ch%2)*0x80), ch_vfmt_status[ch]==PAL?0x01:0x01);
		}
		else
		#endif 
		{
			gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xFF, 0x03+((ch%4)/2));
			gpio_i2c_write(nvp6134_iic_addr[ch/4], ACP_AHD2_PEL_BAUD+((ch%2)*0x80), ch_vfmt_status[ch]==PAL? 0x12:0x12);
			gpio_i2c_write(nvp6134_iic_addr[ch/4], ACP_AHD2_PEL_LINE+((ch%2)*0x80), ch_vfmt_status[ch]==PAL? 0x0D:0x0E);
			gpio_i2c_write(nvp6134_iic_addr[ch/4], ACP_AHD2_PEL_SYNC+((ch%2)*0x80), ch_vfmt_status[ch]==PAL? 0x50:0x50);
			gpio_i2c_write(nvp6134_iic_addr[ch/4], ACP_AHD2_PEL_SYNC+1+((ch%2)*0x80), ch_vfmt_status[ch]==PAL?0x00:0x00);
		}	
		gpio_i2c_write(nvp6134_iic_addr[ch/4], ACP_PACKET_MODE+((ch%2)*0x80), 0x06);
		gpio_i2c_write(nvp6134_iic_addr[ch/4], ACP_AHD2_PEL_EVEN+((ch%2)*0x80), 0x00);

		printk(">>>>> DRV[%s:%d] NVP6134_VI_720P_2530 COAXIAL PROTOCOL IS SETTING....\n", __func__, __LINE__ );
	}
	#else
	else if(ch_mode_status[ch] == NVP6134_VI_720P_2530 ) //|| ch_mode_status[ch] == NVP6134_VI_HDEX )
	{
		gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xFF, 0x05+ch%4);
		gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x7C, 0x11);
		gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xFF, 0x03+((ch%4)/2));

		#if 0
		if(ch_mode_status[ch] == NVP6134_VI_HDEX)
		{
			gpio_i2c_write(nvp6134_iic_addr[ch/4], ACP_AHD2_FHD_BAUD+((ch%2)*0x80),0x2C);
			gpio_i2c_write(nvp6134_iic_addr[ch/4], ACP_AHD2_FHD_LINE+((ch%2)*0x80),ch_vfmt_status[ch]==PAL? 0x0E:0x0E);
		}
		else
		#endif 
		{
			gpio_i2c_write(nvp6134_iic_addr[ch/4], ACP_AHD2_FHD_BAUD+((ch%2)*0x80),0x15);
			gpio_i2c_write(nvp6134_iic_addr[ch/4], ACP_AHD2_FHD_LINE+((ch%2)*0x80),ch_vfmt_status[ch]==PAL? 0x0D:0x0E);
		}
		gpio_i2c_write(nvp6134_iic_addr[ch/4], (0x0D)+((ch%2)*0x80), ch_vfmt_status[ch]==PAL? 0x35:0x30);
		gpio_i2c_write(nvp6134_iic_addr[ch/4], (0x0E)+((ch%2)*0x80), 0x00);
		gpio_i2c_write(nvp6134_iic_addr[ch/4], ACP_AHD2_FHD_LINES+((ch%2)*0x80), 0x03);
		gpio_i2c_write(nvp6134_iic_addr[ch/4], ACP_AHD2_FHD_BYTE+((ch%2)*0x80), 0x03);
		gpio_i2c_write(nvp6134_iic_addr[ch/4], ACP_AHD2_FHD_MODE+((ch%2)*0x80), 0x10);
		gpio_i2c_write(nvp6134_iic_addr[ch/4], ACP_AHD2_PEL_EVEN+((ch%2)*0x80), 0x00);
		printk(">>>>> DRV[%s:%d] NVP6134_VI_720P_2530 COAXIAL PROTOCOL IS SETTING....\n", __func__, __LINE__ );
	}
	#endif
	else if( ch_mode_status[ch] == NVP6134_VI_720P_5060)
	{
		gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xFF, 0x05+ch%4);
		gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x7C, 0x01);
		gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xFF, 0x03+((ch%4)/2));
		gpio_i2c_write(nvp6134_iic_addr[ch/4], ACP_AHD2_FHD_BAUD+((ch%2)*0x80),0x1A);
		gpio_i2c_write(nvp6134_iic_addr[ch/4], ACP_AHD2_FHD_LINE+((ch%2)*0x80),ch_vfmt_status[ch]==PAL? 0x0D:0x0E);
		gpio_i2c_write(nvp6134_iic_addr[ch/4], (0x0D)+((ch%2)*0x80), ch_vfmt_status[ch]==PAL? 0x16:0x20);
		gpio_i2c_write(nvp6134_iic_addr[ch/4], (0x0E)+((ch%2)*0x80), 0x00);
		gpio_i2c_write(nvp6134_iic_addr[ch/4], ACP_AHD2_FHD_LINES+((ch%2)*0x80), 0x03);
		gpio_i2c_write(nvp6134_iic_addr[ch/4], ACP_AHD2_FHD_BYTE+((ch%2)*0x80), 0x03);
		gpio_i2c_write(nvp6134_iic_addr[ch/4], ACP_AHD2_FHD_MODE+((ch%2)*0x80), 0x10);
		gpio_i2c_write(nvp6134_iic_addr[ch/4], ACP_AHD2_PEL_EVEN+((ch%2)*0x80), 0x00);
		printk(">>>>> DRV[%s:%d] NVP6134_VI_720P_5060 COAXIAL PROTOCOL IS SETTING....\n", __func__, __LINE__ );
	}
	else if(ch_mode_status[ch] == NVP6134_VI_1080P_2530) //|| ch_mode_status[ch] == NVP6134_VI_1080P_NRT )
	{
		gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xFF, 0x05+ch%4);
		gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x7C, 0x11);
		gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xFF, 0x03+((ch%4)/2));
		gpio_i2c_write(nvp6134_iic_addr[ch/4], ACP_AHD2_FHD_BAUD+((ch%2)*0x80),0x27);		
		gpio_i2c_write(nvp6134_iic_addr[ch/4], ACP_AHD2_FHD_LINE+((ch%2)*0x80),ch_vfmt_status[ch]==PAL? 0x0E:0x0E);
		gpio_i2c_write(nvp6134_iic_addr[ch/4], (0x0D)+((ch%2)*0x80), ch_vfmt_status[ch]==PAL? 0xB4:0xBB);
		gpio_i2c_write(nvp6134_iic_addr[ch/4], (0x0E)+((ch%2)*0x80), ch_vfmt_status[ch]==PAL? 0x00:0x00);
		gpio_i2c_write(nvp6134_iic_addr[ch/4], ACP_AHD2_FHD_LINES+((ch%2)*0x80), 0x03);
		gpio_i2c_write(nvp6134_iic_addr[ch/4], ACP_AHD2_FHD_BYTE+((ch%2)*0x80), 0x03);
		gpio_i2c_write(nvp6134_iic_addr[ch/4], ACP_AHD2_FHD_MODE+((ch%2)*0x80), 0x10);
		gpio_i2c_write(nvp6134_iic_addr[ch/4], ACP_AHD2_PEL_EVEN+((ch%2)*0x80), 0x00);

		printk(">>>>> DRV[%s:%d] NVP6124_VI_1080P_2530, NVP6134_VI_1080P_NRT COAXIAL PROTOCOL IS SETTING....\n", __func__, __LINE__ );
	}	
	else
	{
		printk(">>>>> DRV[%s:%d] COAXIAL MODE NOT RIGHT...\n", __func__, __LINE__ );
	}
}


/*******************************************************************************
 *
 *
 *
 *  External Functions
 *
 *
 *
 *******************************************************************************/
/*******************************************************************************
*	Description		: set each acp
*	Argurments		: ch(channel ID),
*	Return value	: void
*	Modify			:
*	warning			:
*******************************************************************************/
void acp_each_setting(unsigned char ch)
{
	int vidmode = 0;
	//unsigned char val, val1;
	
	/* mpp setting */
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xFF, 0x01);		/*   - set band(1) */
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xA8+ch%4, (ch%4)<2?0x01:0x02);
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xBC+ch%4, (ch%2)==0?0x07:0x0F);

	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xFF, 0x05+ch%4);//   - set bank(5)
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x2F, 0x00);		//  (+) - internal MPP, HVF(Horizontal Vertical Field sync inversion option)
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x30, 0x00);		// H sync start position
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x31, 0x43);		// H sync start position
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x32, 0xa2);		// H sync end position
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x7C, 0x11);		// RX coax Input selection
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x7D, 0x80);		// RX threshold

	/* set baud rate each format - TX */
	acp_set_baudrate(ch);

	/* a-cp setting - RX */
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xFF, 0x03+((ch%4)/2));		//   - set bank
	//gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x05+((ch%2)*0x80), 0x07);	// TX active Max line(8 line)
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x60+((ch%2)*0x80), 0x55);	// RX [coax header value]

	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x0b+((ch%2)*0x80), 0x10 );	// change coaxial mode

	vidmode = ch_mode_status[ch];
	if( vidmode == NVP6134_VI_1080P_2530 || vidmode == NVP6134_VI_720P_5060 || \
		vidmode == NVP6134_VI_720P_2530 )
	{
		gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x66+((ch%2)*0x80), 0x80);	// RX Auto duty 
		if( vidmode == NVP6134_VI_1080P_2530 )// 1080P
		{
			gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x62+((ch%2)*0x80), 0x06);	// RX receive start line
			gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x68+((ch%2)*0x80), 0x70);	// RX(Receive max line - 8 line, high-4bits)
		}
		else // 720P
		{
			gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x62+((ch%2)*0x80), ch_vfmt_status[ch]==PAL?0x05:0x05);	// RX receive start line
			gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x68+((ch%2)*0x80), 0x70);	// RX(Receive max line - 8 line, high-4bits)
		}
	}

	/*********************************************************************
	 * recognize our code
	 *********************************************************************/

	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x63+((ch%2)*0x80), 0x01);	// RX device(module) ON(1), OFF(0)
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x64+((ch%2)*0x80), 0x00);	// Delay count
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x67+((ch%2)*0x80), 0x01);	// RX(1:interrupt enable), (0:interrupt disable)

	acp_reg_rx_clear(ch);												// reset
}



/*
设置通道模式
变量
ch: 通道号，取值范围0~(nvp6134_cnt*4-1)
vfmt: 0:NTSC, 1:PAL
chnmode:通道模式，参考NVP6134_VI_MODE.
*/
int nvp6134_set_chnmode(const unsigned char ch, const unsigned char vfmt, const unsigned char chnmode)
{  
	//unsigned char tmp;
	
	if(ch >= (nvp6134_cnt*4))
	{
		printk("func[nvp6134_set_chnmode] Channel %d is out of range!!!\n", ch);
		return -1;
	}
	if(vfmt > PAL)
	{
		printk("func[nvp6134_set_chnmode] vfmt %d is out of range!!!\n", vfmt);
		return -1;
	}

	/* set video format each format */
	if(chnmode < NVP6134_VI_BUTT) 
	{
		/*  (+) - set these value */
		nvp6134_set_common_value( ch, chnmode );
		
		switch(chnmode)
		{
			case NVP6134_VI_720H:		nvp6134_setchn_720h(ch, vfmt);			break;
			case NVP6134_VI_960H:		nvp6134_setchn_960h(ch, vfmt);			break;
			case NVP6134_VI_720P_2530:	nvp6134_setchn_ahd_720p(ch, vfmt);		break;
			case NVP6134_VI_720P_5060:	nvp6134_setchn_ahd_720p5060(ch, vfmt);	break;	
			case NVP6134_VI_1080P_2530:	nvp6134_setchn_ahd_1080p2530(ch, vfmt);	break;
			case NVP6134_VI_1080P_NOVIDEO:
			default:	
				nvp6134_setchn_ahd_1080p_novideo(ch, vfmt);
				printk("Default Set to 1080P novideo mode[ch%d]\n", ch);			
			break;
		}

		/* save Video mode and video format(NTSC/PAL) */
		ch_mode_status[ch] = chnmode;
		ch_vfmt_status[ch] = vfmt;

		if(NVP6134_VI_1080P_NOVIDEO != ch_mode_status[ch])
		{
			if(NVP6134_INIT_ON_OFF==0x00)  //default initial 
			{
				/* Initalize ACP protocol each mode */
			    acp_each_setting(ch);
			}

			/*  (+) - set EQ configuration */
			eq_init_each_format( ch, chnmode, vfmt );
		}
		
		gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xff,0x09);
		gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x40+ch%4,0x61); 	
        msleep(35);
		if(NVP6134_VI_720P_2530 > ch_mode_status[ch])
			gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x40+ch%4,0x60); //for comet setting
		else
			gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x40+ch%4,0x00); 
	}

	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0xFF, 0x00); 
	gpio_i2c_write(nvp6134_iic_addr[ch/4], 0x00+ch%4,0x00); 

	return 0;
}




/*
nvp6134和nvp6134b共用同一函数
portsel上有差异，nvp6134b只能使用1和2，nvp6134有4个port，可以使用0~3。
chip:chip select[0,1,2,3];
portsel: port select->6134b[1,2],6134[0,1,2,3];
portmode: port mode select[1mux,2mux,4mux]
chid:  channel id, 1mux[0,1,2,3], 2mux[0,1], 4mux[0]
*/
/*******************************************************************************
*	Description		: select port
*	Argurments		: chip(chip select[0,1,2,3]),
*					  portsel(port select->6134b[1,2],6134[0,1,2,3];)
*					  portmode(port mode select[1mux,2mux,4mux]),
*					  chid(channel id, 1mux[0,1,2,3], 2mux[0,1], 4mux[0])
*	Return value	: 0
*	Modify			:
*	warning			:
*******************************************************************************/
int nvp6134_set_portmode(const unsigned char chip, const unsigned char portsel, const unsigned char portmode, const unsigned char chid)
{
	unsigned char chipaddr = nvp6134_iic_addr[chip];
	unsigned char tmp=0, tmp1=0, reg1=0, reg2=0;
	
	if((portsel!=1) && (portsel!=2) && (chip_id[chip]==NVP6134B_R0_ID))
	{
		printk("nvp6134b_set_portmode portsel[%d] error!!!\n", portsel);
		//return -1;
	}
	
	switch(portmode)
	{
		case NVP6134_OUTMODE_1MUX_SD:
			/*输出720H或者960H单通道,数据37.125MHz,时钟37.125MHz,单沿采样.*/
			gpio_i2c_write(chipaddr, 0xFF, 0x00);
			gpio_i2c_write(chipaddr, 0x56, 0x10);
			gpio_i2c_write(chipaddr, 0xFF, 0x01);
			gpio_i2c_write(chipaddr, 0xC0+portsel*2, (chid<<4)|chid);
			gpio_i2c_write(chipaddr, 0xC1+portsel*2, (chid<<4)|chid);
			tmp = gpio_i2c_read(chipaddr, 0xC8+(portsel/2)) & (portsel%2?0x0F:0xF0);
			gpio_i2c_write(chipaddr, 0xC8+(portsel/2), tmp);
			gpio_i2c_write(chipaddr, 0xCC+portsel, 0x86);			
		break;
		case NVP6134_OUTMODE_1MUX_HD:
			/*输出720P或者1280H或者1440H单通道,数据74.25MHz,时钟74.25MHz,单沿采样.*/
			gpio_i2c_write(chipaddr, 0xFF, 0x00);
			gpio_i2c_write(chipaddr, 0x56, 0x10);
			gpio_i2c_write(chipaddr, 0xFF, 0x01);
			gpio_i2c_write(chipaddr, 0xC0+portsel*2, (chid<<4)|chid); 
			gpio_i2c_write(chipaddr, 0xC1+portsel*2, (chid<<4)|chid);
			tmp = gpio_i2c_read(chipaddr, 0xC8+(portsel/2)) & (portsel%2?0x0F:0xF0);
			gpio_i2c_write(chipaddr, 0xC8+(portsel/2), tmp);
			gpio_i2c_write(chipaddr, 0xCC+portsel, 0x16);   
		break;
		case NVP6134_OUTMODE_1MUX_HD5060:
		case NVP6134_OUTMODE_1MUX_FHD:
		case NVP6134_OUTMODE_1MUX_4M_NRT:	
			/*输出720P@5060或者1080P单通道,数据148.5MHz,时钟148.5MHz,单沿采样.*/
			gpio_i2c_write(chipaddr, 0xFF, 0x00);
			gpio_i2c_write(chipaddr, 0x56, 0x10);
			gpio_i2c_write(chipaddr, 0xFF, 0x01);
			gpio_i2c_write(chipaddr, 0xC0+portsel*2, (chid<<4)|chid);  
			gpio_i2c_write(chipaddr, 0xC1+portsel*2, (chid<<4)|chid);
			tmp = gpio_i2c_read(chipaddr, 0xC8+(portsel/2)) & (portsel%2?0x0F:0xF0);
			gpio_i2c_write(chipaddr, 0xC8+(portsel/2), tmp);
			gpio_i2c_write(chipaddr, 0xCC+portsel, 0x46);
			break;
		case NVP6134_OUTMODE_2MUX_SD:
			/*输出720H或者960H 2通道,数据74.25MHz,时钟74.25MHz,单沿采样.*/
			gpio_i2c_write(chipaddr, 0xFF, 0x00);
			gpio_i2c_write(chipaddr, 0x56, 0x10);
			gpio_i2c_write(chipaddr, 0xFF, 0x01);
			gpio_i2c_write(chipaddr, 0xC0+portsel*2, chid==0?0x10:0x32);
			gpio_i2c_write(chipaddr, 0xC1+portsel*2, chid==0?0x10:0x32);
			tmp = gpio_i2c_read(chipaddr, 0xC8+(portsel/2)) & (portsel%2?0x0F:0xF0);
			tmp |= (portsel%2?0x20:0x02);
			gpio_i2c_write(chipaddr, 0xC8+(portsel/2), tmp);
			gpio_i2c_write(chipaddr, 0xCC+portsel, 0x16);
			break;
		case NVP6134_OUTMODE_2MUX_HD_X:
			/*输出HD-X 2通道,数据74.25MHz,时钟74.25MHz,单沿采样.*/
			gpio_i2c_write(chipaddr, 0xFF, 0x00);
			gpio_i2c_write(chipaddr, 0x56, 0x10);
			gpio_i2c_write(chipaddr, 0xFF, 0x01);
			gpio_i2c_write(chipaddr, 0xC0+portsel*2, chid==0?0x98:0xBA);
			gpio_i2c_write(chipaddr, 0xC1+portsel*2, chid==0?0x98:0xBA);
			tmp = gpio_i2c_read(chipaddr, 0xC8+(portsel/2)) & (portsel%2?0x0F:0xF0);
			tmp |= (portsel%2?0x20:0x02);
			gpio_i2c_write(chipaddr, 0xC8+(portsel/2), tmp);
			gpio_i2c_write(chipaddr, 0xCC+portsel, 0x16);
			break;			
		case NVP6134_OUTMODE_2MUX_HD:
			/*输出HD 2通道,数据148.5MHz,时钟148.5MHz,单沿采样.*/
			gpio_i2c_write(chipaddr, 0xFF, 0x00);
			gpio_i2c_write(chipaddr, 0x56, 0x10);
			gpio_i2c_write(chipaddr, 0xFF, 0x01);
			gpio_i2c_write(chipaddr, 0xC0+portsel*2, chid==0?0x10:0x32);
			gpio_i2c_write(chipaddr, 0xC1+portsel*2, chid==0?0x10:0x32);
			tmp = gpio_i2c_read(chipaddr, 0xC8+(portsel/2)) & (portsel%2?0x0F:0xF0);
			tmp |= (portsel%2?0x20:0x02);
			gpio_i2c_write(chipaddr, 0xC8+(portsel/2), tmp);
			gpio_i2c_write(chipaddr, 0xCC+portsel, 0x46);			
			break;
		case NVP6134_OUTMODE_4MUX_SD:
			/*输出720H或者960H 4通道,数据148.5MHz,时钟148.5MHz,单沿采样.*/
			gpio_i2c_write(chipaddr, 0xFF, 0x00);
			gpio_i2c_write(chipaddr, 0x56, 0x32);
			gpio_i2c_write(chipaddr, 0xFF, 0x01);
			gpio_i2c_write(chipaddr, 0xC0+portsel*2, 0x10);    
			gpio_i2c_write(chipaddr, 0xC1+portsel*2, 0x32);
			tmp = gpio_i2c_read(chipaddr, 0xC8+(portsel/2)) & (portsel%2?0x0F:0xF0);
			tmp |= (portsel%2?0x80:0x08);
			gpio_i2c_write(chipaddr, 0xC8+(portsel/2), tmp);
			gpio_i2c_write(chipaddr, 0xCC+portsel, 0x46);  
			break;
		case NVP6134_OUTMODE_4MUX_HD:	
			/*输出720P 4通道,数据297MHz,时钟297MHz,单沿采样.*/
			gpio_i2c_write(chipaddr, 0xFF, 0x00);
			gpio_i2c_write(chipaddr, 0x56, 0x32);
			gpio_i2c_write(chipaddr, 0xFF, 0x01);
			gpio_i2c_write(chipaddr, 0xC0+portsel*2, 0x10);    
			gpio_i2c_write(chipaddr, 0xC1+portsel*2, 0x32);
			tmp = gpio_i2c_read(chipaddr, 0xC8+(portsel/2)) & (portsel%2?0x0F:0xF0);
			tmp |= (portsel%2?0x80:0x08);
			gpio_i2c_write(chipaddr, 0xC8+(portsel/2), tmp);
			gpio_i2c_write(chipaddr, 0xCC+portsel, 0x46);
			//gpio_i2c_write(chipaddr, 0xCC+portsel, 0x66);  //single up
			break;
		case NVP6134_OUTMODE_4MUX_HD_X:
			/*输出HD-X 4通道,数据148.5MHz,时钟148.5MHz,单沿采样.*/
			gpio_i2c_write(chipaddr, 0xFF, 0x00);
			gpio_i2c_write(chipaddr, 0x56, 0x32);
			gpio_i2c_write(chipaddr, 0xFF, 0x01);
			gpio_i2c_write(chipaddr, 0xC0+portsel*2, 0x98);    
			gpio_i2c_write(chipaddr, 0xC1+portsel*2, 0xBA);
			tmp = gpio_i2c_read(chipaddr, 0xC8+(portsel/2)) & (portsel%2?0x0F:0xF0);
			tmp |= (portsel%2?0x80:0x08);
			gpio_i2c_write(chipaddr, 0xC8+(portsel/2), tmp);
			gpio_i2c_write(chipaddr, 0xCC+portsel, 0x46);
			break;
		case NVP6134_OUTMODE_2MUX_FHD:	
			/*5M_20P,5M_12P,4M_RT,4M_15P,3M_RT/NRT,FHD,3840H,HDEX 2mux任意混合,数据297MHz,时钟148.5MHz,双沿采样.
			SOC VI端通过丢点，实现3840H->960H, HDEX->720P  */
			gpio_i2c_write(chipaddr, 0xFF, 0x00);
			gpio_i2c_write(chipaddr, 0x56, 0x10);
			#if 0
			//CHANNEL 1 JUDGE
			tmp  = gpio_i2c_read(chipaddr, 0x81)&0x0F;
			tmp1 = gpio_i2c_read(chipaddr, 0x85)&0x0F;
			if(((tmp == 0x02) || (tmp == 0x03)) && (tmp1 == 0x04))
				reg1 |= 0x08;							//3M_RT, THEN OUTPUT 3M_CIF DATA
			else if(((tmp == 0x0E) || (tmp == 0x0F)) && (tmp1 == 0x00))
				reg1 |= 0x08;							//4M, THEN OUTPUT 4M_CIF DATA
			else if(ch_mode_status[(chip*4)] == NVP6134_VI_5M_20P) //5M_20P, THEN OUTPUT 5M_CIF DATA
				reg1 |= 0x08;
			else				
				reg1 &= 0xF0;
			//CHANNEL 2 JUDGE
			tmp  = gpio_i2c_read(chipaddr, 0x82)&0x0F;
			tmp1 = gpio_i2c_read(chipaddr, 0x86)&0x0F;
			if(((tmp == 0x02) || (tmp == 0x03)) && (tmp1 == 0x04))
				reg1 |= 0x80;
			else if(((tmp == 0x0E) || (tmp == 0x0F)) && (tmp1 == 0x00))
				reg1 |= 0x80;
			else if(ch_mode_status[(chip*4+1)] == NVP6134_VI_5M_20P) //5M_20P, THEN OUTPUT 5M_CIF DATA
				reg1 |= 0x80;
			else
				reg1 &= 0x0F;
			//CHANNEL 3 JUDGE
			tmp  = gpio_i2c_read(chipaddr, 0x83)&0x0F;
			tmp1 = gpio_i2c_read(chipaddr, 0x87)&0x0F;
			if(((tmp == 0x02) || (tmp == 0x03)) && (tmp1 == 0x04))
				reg2 |= 0x08;
			else if(((tmp == 0x0E) || (tmp == 0x0F)) && (tmp1 == 0x00))
				reg2 |= 0x08;
			else if(ch_mode_status[(chip*4+2)] == NVP6134_VI_5M_20P) //5M_20P, THEN OUTPUT 5M_CIF DATA
				reg2 |= 0x08;
			else
				reg2 &= 0xF0;
			//CHANNEL 4 JUDGE
			tmp  = gpio_i2c_read(chipaddr, 0x84)&0x0F;
			tmp1 = gpio_i2c_read(chipaddr, 0x88)&0x0F;
			if(((tmp == 0x02) || (tmp == 0x03)) && (tmp1 == 0x04))
				reg2 |= 0x80;
			else if(((tmp == 0x0E) || (tmp == 0x0F)) && (tmp1 == 0x00))
				reg2 |= 0x80;
			else if(ch_mode_status[(chip*4+3)] == NVP6134_VI_5M_20P) //5M_20P, THEN OUTPUT 5M_CIF DATA
				reg2 |= 0x80;
			else
				reg2 &= 0x0F;
			gpio_i2c_write(chipaddr, 0xFF, 0x01);
			gpio_i2c_write(chipaddr, 0xC0+portsel*2, chid==0?(0x10|reg1):(0x32|reg2));
			gpio_i2c_write(chipaddr, 0xC1+portsel*2, chid==0?(0x10|reg1):(0x32|reg2));
			#else
			gpio_i2c_write(chipaddr, 0xFF, 0x01);
			gpio_i2c_write(chipaddr, 0xC0+portsel*2, chid==0?0x10:0x32);
			gpio_i2c_write(chipaddr, 0xC1+portsel*2, chid==0?0x10:0x32);
			#endif
			tmp = gpio_i2c_read(chipaddr, 0xC8+(portsel/2)) & (portsel%2?0x0F:0xF0);
			tmp |= (portsel%2?0x20:0x02);
			gpio_i2c_write(chipaddr, 0xC8+(portsel/2), tmp);
			gpio_i2c_write(chipaddr, 0xCC+portsel, 0x46);
			//gpio_i2c_write(chipaddr, 0xCC+portsel, 0x66);  //single up
			break;	
		case NVP6134_OUTMODE_4MUX_FHD_X:
			/*输出FHD-X 4通道,数据297MHz,时钟148.5MHz,双沿采样.*/
			gpio_i2c_write(chipaddr, 0xFF, 0x00);
			gpio_i2c_write(chipaddr, 0x56, 0x32);
			gpio_i2c_write(chipaddr, 0xFF, 0x01);
			gpio_i2c_write(chipaddr, 0xC0+portsel*2, 0x98);    
			gpio_i2c_write(chipaddr, 0xC1+portsel*2, 0xBA);
			tmp = gpio_i2c_read(chipaddr, 0xC8+(portsel/2)) & (portsel%2?0x0F:0xF0);
			tmp |= (portsel%2?0x80:0x08);
			gpio_i2c_write(chipaddr, 0xC8+(portsel/2), tmp);
			gpio_i2c_write(chipaddr, 0xCC+portsel, 0x46);  
			//gpio_i2c_write(chipaddr, 0xCC+portsel, 0x66);  //single up
			break;
		case NVP6134_OUTMODE_4MUX_MIX: 
			/*HD,1920H,FHD-X 4mux任意混合,数据297MHz,时钟148.5MHz,双沿采样
			SOC VI端通过丢点，实现1920H->960H  */
			gpio_i2c_write(chipaddr, 0xFF, 0x00);
			gpio_i2c_write(chipaddr, 0x56, 0x32);
			tmp = gpio_i2c_read(chipaddr, 0x81)&0x0F;
			if(((tmp) == 0x02) || ((tmp) == 0x03))
				reg1 |= 0x08;
			else
				reg1 &= 0xF0;
			tmp = gpio_i2c_read(chipaddr, 0x82)&0x0F;
			if((tmp == 0x02) || (tmp == 0x03))
				reg1 |= 0x80;
			else
				reg1 &= 0x0F;
			tmp = gpio_i2c_read(chipaddr, 0x83)&0x0F;
			if(((tmp) == 0x02) || ((tmp) == 0x03))
				reg2 |= 0x08;
			else
				reg2 &= 0xF0;
			tmp = gpio_i2c_read(chipaddr, 0x84)&0x0F;
			if((tmp == 0x02) || (tmp == 0x03))
				reg2 |= 0x80;
			else
				reg2 &= 0x0F;
			gpio_i2c_write(chipaddr, 0xFF, 0x01);
			gpio_i2c_write(chipaddr, 0xC0+portsel*2, 0x10|reg1);    
			gpio_i2c_write(chipaddr, 0xC1+portsel*2, 0x32|reg2);
			tmp = gpio_i2c_read(chipaddr, 0xC8+(portsel/2)) & (portsel%2?0x0F:0xF0);
			tmp |= (portsel%2?0x80:0x08);
			gpio_i2c_write(chipaddr, 0xC8+(portsel/2), tmp);
			gpio_i2c_write(chipaddr, 0xCC+portsel, 0x46);  
			//gpio_i2c_write(chipaddr, 0xCC+portsel, 0x66);  //single up
			break;	
		case NVP6134_OUTMODE_2MUX_MIX: 
		case NVP6134_OUTMODE_2MUX_FHD_X:	
			/*HD,1920H,FHD-X 2MUX任意混合,数据148.5MHz,时钟148.5MHz,单沿采样	
			SOC VI端通过丢点，实现1920H->960H  */
			gpio_i2c_write(chipaddr, 0xFF, 0x00);
			gpio_i2c_write(chipaddr, 0x56, 0x10);
			tmp = gpio_i2c_read(chipaddr, 0x81)&0x0F;
			if(((tmp) == 0x02) || ((tmp) == 0x03))
				reg1 |= 0x08;
			else
				reg1 &= 0xF0;
			tmp = gpio_i2c_read(chipaddr, 0x82)&0x0F;
			if((tmp == 0x02) || (tmp == 0x03))
				reg1 |= 0x80;
			else
				reg1 &= 0x0F;
			tmp = gpio_i2c_read(chipaddr, 0x83)&0x0F;
			if(((tmp) == 0x02) || ((tmp) == 0x03))
				reg2 |= 0x08;
			else
				reg2 &= 0xF0;
			tmp = gpio_i2c_read(chipaddr, 0x84)&0x0F;
			if((tmp == 0x02) || (tmp == 0x03))
				reg2 |= 0x80;
			else
				reg2 &= 0x0F;
			gpio_i2c_write(chipaddr, 0xFF, 0x01);
			gpio_i2c_write(chipaddr, 0xC0+portsel*2, chid==0?(0x10|reg1):(0x32|reg2));
			gpio_i2c_write(chipaddr, 0xC1+portsel*2, chid==0?(0x10|reg1):(0x32|reg2));
			tmp = gpio_i2c_read(chipaddr, 0xC8+(portsel/2)) & (portsel%2?0x0F:0xF0);
			tmp |= (portsel%2?0x20:0x02);
			gpio_i2c_write(chipaddr, 0xC8+(portsel/2), tmp);
			gpio_i2c_write(chipaddr, 0xCC+portsel, 0x46); 				
			break;	
		case NVP6134_OUTMODE_1MUX_BT1120S:	
			gpio_i2c_write(chipaddr, 0xFF, 0x00);
			gpio_i2c_write(chipaddr, 0x56, 0x10);
			gpio_i2c_write(chipaddr, 0xFF, 0x01);
			//gpio_i2c_write(chipaddr, 0xED, 0x0F);
			if(chip_id[chip] == NVP6134B_R0_ID)
			{
				//6134C makes 2 bt656 ports to 1 bt1120 port.  portsel=[1,2] to choose clock.
				gpio_i2c_write(chipaddr, 0xC2, (0+0x44)); //vdo1 ouput     //ch1 Y_data; 0x55:ch2 y data; 0x66:ch3 y data ;0x77:ch4 y data 
				gpio_i2c_write(chipaddr, 0xC3, (0+0x44));
				gpio_i2c_write(chipaddr, 0xC4, (0+0xcc)); //vdo2 output  //ch1 C_data; 0xdd:ch2 c data; 0xee:ch3 c data;0xff: ch4 c data 
				gpio_i2c_write(chipaddr, 0xC5, (0+0xcc));
				gpio_i2c_write(chipaddr, 0xC8, 0x00);
				gpio_i2c_write(chipaddr, 0xC9, 0x00);
				
				if(SINGAL_HD == 0x02) 
					gpio_i2c_write(chipaddr, 0xCC+portsel, 0x06);		//74.25MHz clock for BT1120 1080P mode 
				else
					gpio_i2c_write(chipaddr, 0xCC+portsel, 0x86);		//37.125MHz clock for BT1120 720P mode 
			}
			else
			{
				//6134 makes 4 bt656 ports to 2 bt1120 port.   portsel=[0,1] to choose clock.
				gpio_i2c_write(chipaddr, 0xC0, (0+0xcc));  //vdo1 output   //ch1 C_data; 0x0d:ch2 c data; 0x0e:ch3 c data;0x0f: ch4 c data 
				gpio_i2c_write(chipaddr, 0xC1, (0+0xcc));
				gpio_i2c_write(chipaddr, 0xC2, (0+0x44));  //vdo2 ouput     //ch1 Y_data; 0x55:ch2 y data; 0x66:ch3 y data ;0x77:ch4 y data 
				gpio_i2c_write(chipaddr, 0xC3, (0+0x44));

				gpio_i2c_write(chipaddr, 0xC4, (0+0xcc));  //vdo3 output   //ch1 C_data; 0xdd:ch2 c data; 0xee:ch3 c data;0xff: ch4 c data 
				gpio_i2c_write(chipaddr, 0xC5, (0+0xcc));
				gpio_i2c_write(chipaddr, 0xC6, (0+0x44));  //vdo4 ouput     //ch1 Y_data; 0x55:ch2 y data; 0x66:ch3 y data ;0x77:ch4 y data 
				gpio_i2c_write(chipaddr, 0xC7, (0+0x44));
				
				gpio_i2c_write(chipaddr, 0xC8, 0x00);
				gpio_i2c_write(chipaddr, 0xC9, 0x00);
				
				if(SINGAL_HD == 0x02)
					gpio_i2c_write(chipaddr, 0xCC+portsel, 0x06);		//74.25MHz clock for BT1120 1080P mode 
				else
					gpio_i2c_write(chipaddr, 0xCC+portsel, 0x86);		//37.125MHz clock for BT1120 720P mode 
			}
			break;
		case NVP6134_OUTMODE_1MUX_3M_RT:
			/*1MUX数据输出，时钟是297MHZ输出*/
			gpio_i2c_write(chipaddr, 0xFF, 0x00);
			gpio_i2c_write(chipaddr, 0x56, 0x10);
			gpio_i2c_write(chipaddr, 0xFF, 0x01);
			gpio_i2c_write(chipaddr, 0xC0+portsel*2, (chid<<4)|chid);   /* Port selection */
			gpio_i2c_write(chipaddr, 0xC1+portsel*2, (chid<<4)|chid);   /* Port selection */
			tmp = gpio_i2c_read(chipaddr, 0xC8+(portsel/2)) & (portsel%2?0x0F:0xF0);
			gpio_i2c_write(chipaddr, 0xC8+(portsel/2), tmp);
			//gpio_i2c_write(chipaddr, 0xCC+portsel, 0x46);
			gpio_i2c_write(chipaddr, 0xCC+portsel, 0x66);
			break;
		default:
			printk("portmode %d not supported yet\n", portmode);
			break;		
  	}
	
	if(	portmode==NVP6134_OUTMODE_2MUX_MIX  ||
		portmode==NVP6134_OUTMODE_2MUX_SD	||
		portmode==NVP6134_OUTMODE_2MUX_HD_X ||
		portmode==NVP6134_OUTMODE_2MUX_FHD_X)
	{
		gpio_i2c_write(chipaddr, 0xFF, 0x01);
		gpio_i2c_write(chipaddr, 0xE4, 0x10);  //enable 2mix cif mode
		gpio_i2c_write(chipaddr, 0xE5, 0x10);
	}
	else 
	{
		gpio_i2c_write(chipaddr, 0xFF, 0x01);
		gpio_i2c_write(chipaddr, 0xE4, 0x00);  //disable 2mix cif mode
		gpio_i2c_write(chipaddr, 0xE5, 0x00);
	}

	if(	portmode==NVP6134_OUTMODE_2MUX_SD || 
		portmode==NVP6134_OUTMODE_4MUX_SD || 
		portmode==NVP6134_OUTMODE_4MUX_HD_X)
	{
		gpio_i2c_write(chipaddr, 0xFF, 0x01);
		gpio_i2c_write(chipaddr, 0xA0+portsel, 0x20);  //TM clock mode sel manual
	}
	else 
	{
		gpio_i2c_write(chipaddr, 0xFF, 0x01);
		gpio_i2c_write(chipaddr, 0xA0+portsel, 0x00);  //TM clock mode sel auto
	}

	if(BT656_BT601_SEL==0x01) //BT601 mode 
	{
	    /* MPP hsync and vsync channel select  */
	    if(chip_id[chip]==NVP6134B_R0_ID)
	    {
		    gpio_i2c_write(chipaddr, 0xFF, 0x05+portsel-1);
			if(chid == 0)
				gpio_i2c_write(chipaddr, 0x3c, 0x10);
			else if(chid == 1)
				gpio_i2c_write(chipaddr, 0x3c, 0x54);
			else if(chid == 2)
				gpio_i2c_write(chipaddr, 0x3c, 0x98);
			else
				gpio_i2c_write(chipaddr, 0x3c, 0xDC);
	    }
		else
	    {
		    gpio_i2c_write(chipaddr, 0xFF, 0x05+portsel);
			if(chid == 0)
				gpio_i2c_write(chipaddr, 0x3c, 0x10);
			else if(chid == 1)
				gpio_i2c_write(chipaddr, 0x3c, 0x54);
			else if(chid == 2)
				gpio_i2c_write(chipaddr, 0x3c, 0x98);
			else
				gpio_i2c_write(chipaddr, 0x3c, 0xDC);
	    }
	}
	
	printk("nvp6134(b)_set_portmode portsel %d portmode %d setting\n", portsel, portmode);
	return 0;
}



/*******************************************************************************
*	Description		: It is called when "insmod nvp61XX_ex.ko" command run
*	Argurments		: void
*	Return value	: -1(could not register nvp61XX device), 0(success)
*	Modify			:
*	warning			:
*******************************************************************************/
static int __init nvp6134_module_init(void)
{
	int ret = 0;
	int ch = 0;
	int chip = 0;

	#ifdef CONFIG_HISI_SNAPSHOT_BOOT
    snprintf(s_stNvp6134Device.devfs_name, sizeof(s_stNvp6134Device.devfs_name), DEV_NAME);
    s_stNvp6134Device.minor  = HIMEDIA_DYNAMIC_MINOR;
    s_stNvp6134Device.fops   = &nvp6134_fops;
    s_stNvp6134Device.drvops = &stNvp6134DrvOps;
    s_stNvp6134Device.owner  = THIS_MODULE;

    ret = himedia_register(&s_stNvp6134Device);
    if (ret)
    {
        printk(0, "could not register nvp6134_dev device");
        return -1;
    }
	#else
	ret = misc_register(&nvp6134_dev);
   	if (ret)
	{
		printk("ERROR: could not register nvp6134_dev devices:%#x \n",ret);		
        return -1;
	}
	#endif
	
    /* check Device ID of maxium 4chip on the slave address,
     * manage slave address. chip count. */
	for(chip=0;chip<4;chip++)
	{
		chip_id[chip] = check_id(nvp6134_iic_addr[chip]);
		rev_id[chip]  = check_rev(nvp6134_iic_addr[chip]);
		if( (chip_id[chip] != NVP6134_R0_ID )  	&& 
			(chip_id[chip] != NVP6134B_R0_ID) )
		{
			printk("Device ID Error... %x\n", chip_id[chip]);
		}
		else
		{
			printk("Device (0x%x) ID OK... %x\n", nvp6134_iic_addr[chip], chip_id[chip]);
			printk("Device (0x%x) REV ... %x\n", nvp6134_iic_addr[chip], rev_id[chip]);
			nvp6134_iic_addr[nvp6134_cnt] = nvp6134_iic_addr[chip];	
			if(nvp6134_cnt<chip)
				nvp6134_iic_addr[chip] = 0xFF;
			chip_id[nvp6134_cnt] = chip_id[chip];
			rev_id[nvp6134_cnt]  = rev_id[chip];
			nvp6134_cnt++;
		}
	}
	
	printk("Chip Count = %d, [0x%x][0x%x][0x%x][0x%x]\n", \
		nvp6134_cnt, nvp6134_iic_addr[0],nvp6134_iic_addr[1],\
		nvp6134_iic_addr[2],nvp6134_iic_addr[3]);
	
    /* initialize common value of AHD */
	for(chip=0;chip<nvp6134_cnt;chip++)
		nvp6134_common_init(chip);

	/* set channel mode(AHD 1080P) each chip - default */
	for(ch=0;ch<nvp6134_cnt*4;ch++)
	{
		//没有视频接入的时候，必须默认设置为AHD 1080P novideo模式.
		nvp6134_set_chnmode(ch, PAL, NVP6134_VI_1080P_NOVIDEO);  
		
		if(SINGAL_HD == 0x00)
			nvp6134_set_chnmode(ch, PAL, NVP6134_VI_720H); 
		else if(SINGAL_HD == 0x01)
			nvp6134_set_chnmode(ch, PAL, NVP6134_VI_720P_2530); 
		else
			nvp6134_set_chnmode(ch, PAL, NVP6134_VI_1080P_2530); 
	}
	
	/* set port(1MUX AHD 1080P) each chip - default */
	for(chip=0;chip<nvp6134_cnt;chip++)
	{
		if(chip_id[chip] == NVP6134_R0_ID)
		{
			//set nvp6134 4 vdo port ouput mode , port 0,1,2,3  is useful 
			if(SINGAL_HD == 0x00)
			{
				nvp6134_set_portmode(chip, 0, NVP6134_OUTMODE_1MUX_SD, 0);  
				nvp6134_set_portmode(chip, 1, NVP6134_OUTMODE_1MUX_SD, 1);
				nvp6134_set_portmode(chip, 2, NVP6134_OUTMODE_1MUX_SD, 2);
				nvp6134_set_portmode(chip, 3, NVP6134_OUTMODE_1MUX_SD, 3);
			}
			else if(SINGAL_HD == 0x01)
			{
				if(BT656_BT601_SEL==0x01||BT656_BT601_SEL==0x00)  //bt656 and bt601 mode 
				{
					nvp6134_set_portmode(chip, 0, NVP6134_OUTMODE_1MUX_HD, 0);  
					nvp6134_set_portmode(chip, 1, NVP6134_OUTMODE_1MUX_HD, 1);
					nvp6134_set_portmode(chip, 2, NVP6134_OUTMODE_1MUX_HD, 2);
					nvp6134_set_portmode(chip, 3, NVP6134_OUTMODE_1MUX_HD, 3);
				}
				else //BT1120 mode
				{
					nvp6134_set_portmode(chip, 0, NVP6134_OUTMODE_1MUX_BT1120S, 0);  
					nvp6134_set_portmode(chip, 1, NVP6134_OUTMODE_1MUX_BT1120S, 1);
					nvp6134_set_portmode(chip, 2, NVP6134_OUTMODE_1MUX_BT1120S, 2);
					nvp6134_set_portmode(chip, 3, NVP6134_OUTMODE_1MUX_BT1120S, 3);
				}

			}
			else if(SINGAL_HD == 0x02)
			{
				if(BT656_BT601_SEL==0x01||BT656_BT601_SEL==0x00)  //bt656 and bt601 mode 
				{
					nvp6134_set_portmode(chip, 0, NVP6134_OUTMODE_1MUX_FHD, 0);  
					nvp6134_set_portmode(chip, 1, NVP6134_OUTMODE_1MUX_FHD, 1);
					nvp6134_set_portmode(chip, 2, NVP6134_OUTMODE_1MUX_FHD, 2);
					nvp6134_set_portmode(chip, 3, NVP6134_OUTMODE_1MUX_FHD, 3);
				}
				else //BT1120 mode
				{
					nvp6134_set_portmode(chip, 0, NVP6134_OUTMODE_1MUX_BT1120S, 0);  
					nvp6134_set_portmode(chip, 1, NVP6134_OUTMODE_1MUX_BT1120S, 1);
					nvp6134_set_portmode(chip, 2, NVP6134_OUTMODE_1MUX_BT1120S, 2);
					nvp6134_set_portmode(chip, 3, NVP6134_OUTMODE_1MUX_BT1120S, 3);
				} 
			}
		}
		else if(chip_id[chip] == NVP6134B_R0_ID)
		{
			//set nvp6134c  2 vdo port ouput mode , port 1,2  is useful 
			if(SINGAL_HD == 0x00 )
			{				
				nvp6134_set_portmode(chip, 1, NVP6134_OUTMODE_1MUX_SD, 0);  
				nvp6134_set_portmode(chip, 2, NVP6134_OUTMODE_1MUX_SD, 1);
			}
			else if(SINGAL_HD == 0x01 )
			{
				if(BT656_BT601_SEL==0x01||BT656_BT601_SEL==0x00)  //bt656 and bt601 mode 
				{
					nvp6134_set_portmode(chip, 1, NVP6134_OUTMODE_1MUX_HD, 0);  
					nvp6134_set_portmode(chip, 2, NVP6134_OUTMODE_1MUX_HD, 1);
				}
				else //BT1120 mode
				{
					nvp6134_set_portmode(chip, 1, NVP6134_OUTMODE_1MUX_BT1120S, 0); 
					nvp6134_set_portmode(chip, 2, NVP6134_OUTMODE_1MUX_BT1120S, 0);
				}
			}
			else if(SINGAL_HD == 0x02 )
			{
				if(BT656_BT601_SEL==0x01||BT656_BT601_SEL==0x00)  //bt656 and bt601 mode 
				{
					nvp6134_set_portmode(chip, 1, NVP6134_OUTMODE_1MUX_FHD, 0);  
					nvp6134_set_portmode(chip, 2, NVP6134_OUTMODE_1MUX_FHD, 1);
				}
				else //BT1120 mode
				{
					nvp6134_set_portmode(chip, 1, NVP6134_OUTMODE_1MUX_BT1120S, 0);
					nvp6134_set_portmode(chip, 2, NVP6134_OUTMODE_1MUX_BT1120S, 0);
				} 
			}
			
		}
	}


	if(BT656_BT601_SEL==0x01)
	{
		/* set BT601 mode(AHD 720P) each chip - default */
		for(ch=0;ch<nvp6134_cnt*4;ch++)
		{
			if(SINGAL_HD == 0x00)
				nvp6134_set_bt601_mode(ch, PAL, NVP6134_VI_720H); 
			else if(SINGAL_HD == 0x01)
				nvp6134_set_bt601_mode(ch, PAL, NVP6134_VI_720P_2530); 
			else
				nvp6134_set_bt601_mode(ch, PAL, NVP6134_VI_1080P_2530);   
		}
	}
	
	gpio_i2c_write(nvp6134_iic_addr[0], 0xFF, 0x00);	
	gpio_i2c_write(nvp6134_iic_addr[0], 0x78, 0x66);	//0x66: red color ; 0x22:yellow color 	
	gpio_i2c_write(nvp6134_iic_addr[0], 0x79, 0x66);  	
	
	gpio_i2c_write(nvp6134_iic_addr[0], 0xFF, 0x01);
	gpio_i2c_write(nvp6134_iic_addr[0], 0x98, 0x0C);  //POWER down CH3/4 	
	
	return 0;
}

/*******************************************************************************
*	Description		: It is called when "rmmod nvp61XX_ex.ko" command run
*	Argurments		: void
*	Return value	: void
*	Modify			:
*	warning			:
*******************************************************************************/
static void __exit nvp6134_module_exit(void)
{
	if(nvp6134_kt)
        kthread_stop(nvp6134_kt);
#ifdef CONFIG_HISI_SNAPSHOT_BOOT
    himedia_unregister(&s_stNvp6134Device);
#else
    misc_deregister(&nvp6134_dev);
#endif	
#ifdef I2C_INTERNAL
    i2c_client_exit();
#endif	
}

module_init(nvp6134_module_init);
module_exit(nvp6134_module_exit);

MODULE_LICENSE("GPL");

/*******************************************************************************
*	End of file
*******************************************************************************/

