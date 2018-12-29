
#ifdef _TP_ICN85XX_

#include "tp_icn85xx.h"
#include "tpd_custom_icn85xx.h"

#if TPD_ICN85XX_COMPILE_FW_WITH_DRIVER
//#include "icn85xx_fw_old.h"

#if OLD_TP
#include "icn85xx_fw.h"
#else
#include "icn85xx_fw_new.h"		// 新的TP
#endif

#endif

#include "gpio.h"
#include "xm_core.h"
#include "gpioi2c.h"
#include "xm_core.h"
#include "xm_uart.h"
#include "xm_key.h"
#include "xm_dev.h"
#include "xm_i2c.h"

#define	printk	printf
#define	msleep	OS_Delay

#if TULV_BB_TEST
// SDA	CD5 (GPIO85)
//	SCL	CD6 (GPIO86), 
//	INT	CD1 (GPIO81), Input
//	RST	CD2 (GPIO82), Output

#define	INT_GPIO		GPIO81
#define	RST_GPIO		GPIO82
#define	GPIO_CTP_EINT_PIN		INT_GPIO

#else
// 途旅产品板
// SDA	SD1_CMD(GPIO105)
//	SCL	SD1_CLK(GPIO107)
// INT	SD1_D0(GPIO101)
// RST	SD1_D1(GPIO102)

#define	INT_GPIO		GPIO101
#define	RST_GPIO		GPIO102
#define	GPIO_CTP_EINT_PIN		INT_GPIO

#endif

#define	XMSYS_icn85xx_TASK_PRIORITY		254
#define	XMSYS_icn85xx_TASK_STACK_SIZE		0x800

#define	XMSYS_TP_EVENT					0x01
#define	XMSYS_TP_EVENT_TICKET		0x02


#define	TP_TIMEOUT		64		// 64毫秒超时释放检测

static char firmware[128] =  {"icn85xx_firmware"};//{"/fw.bin"};


static OS_TASK TCB_icn85xxTask;
struct icn85xx_ts_data *icn85xx_ts = NULL;
static OS_TIMER TpTimer;

//static int tpd_load_status;
//static int tpd_flag = 0;
static volatile int tp_down = 0;
static unsigned int timeout_to_check_tp_release;
__no_init static OS_STACKPTR int Stackicn85xxTask[XMSYS_icn85xx_TASK_STACK_SIZE/4];          /* Task stacks */

#define	TP_HW_I2C

#ifdef TP_HW_I2C		// 使用硬件I2C, 与sensor复用
#undef icn85xx_i2c_read_bytes
#undef icn85xx_i2c_write_bytes

static xm_i2c_client_t tp_device = NULL;
static unsigned int tp_address = 0;

static int icn85xx_i2c_read_bytes (unsigned int slvaddr, unsigned char* addr, int size, char *rxdata, int length)
{
	int io_ret = 0;
	int ret = -1;
	
	do
	{
		// 检查tp的i2c地址是否一致.
		// 若不一致(从正常模式 <--> 编程模式 之间切换, i2c地址变化), 关闭当前的i2c设备, 然后重新打开
		if(tp_device && tp_address != slvaddr)
		{
			xm_i2c_close (tp_device);
			tp_device = NULL;
			tp_address = 0;
		}
		
		if(tp_device == NULL)
		{
			tp_device = xm_i2c_open (XM_I2C_DEV_0,
								 slvaddr,		
								 "ICN85X",
								 XM_I2C_ADDRESSING_7BIT,
								 &io_ret);
			if(tp_device == NULL)
				break;
			tp_address = slvaddr;
		}
		 
		if(xm_i2c_write (tp_device, (u8_t *)addr, size, 100) < 0)
			break;
		
		if(xm_i2c_read  (tp_device, (u8_t *)rxdata, length, 100) != length)
			break;

		ret = length;
		
	} while (0);
	
	//if(tp_device)
	//	xm_i2c_close (tp_device);
	return ret;
}

static int icn85xx_i2c_write_bytes (unsigned int slvaddr, unsigned char* addr, int size, char *txdata, int length)
{
	int ret = -1;
	int io_ret = 0;
	do
	{
		// 检查tp的i2c地址是否一致.
		// 若不一致(从正常模式 <--> 编程模式 之间切换, i2c地址变化), 关闭当前的i2c设备, 然后重新打开
		if(tp_device && tp_address != slvaddr)
		{
			xm_i2c_close (tp_device);
			tp_device = NULL;
			tp_address = 0;
		}

		if(tp_device == NULL)
		{
			tp_device = xm_i2c_open (XM_I2C_DEV_0,
								 slvaddr,		
								 "ICN85X",
								 XM_I2C_ADDRESSING_7BIT,
								 &io_ret);
			if(tp_device == NULL)
				break;
			tp_address = slvaddr;
		}
		 
		if(xm_i2c_write (tp_device, (u8_t *)addr, size, 100) < 0)
			break;
		
		if(xm_i2c_write  (tp_device, (u8_t *)txdata, length, 100) < 0)
		{
			break;
		}

		ret = 0;
		
	} while (0);
	
	//if(tp_device)
	//	xm_i2c_close (tp_device);
	return ret;
	
}
#endif

static void TpTicketCallback (void)
{
	OS_SignalEvent(XMSYS_TP_EVENT_TICKET, &TCB_icn85xxTask); /* 通知事件 */
	OS_RetriggerTimer (&TpTimer);
}

void icn85xx_ts_reset(void)
{
	XM_lock();
	SetGPIOPadData (RST_GPIO, euDataLow);
	XM_unlock();
	OS_Delay (80);
	XM_lock();
	SetGPIOPadData (RST_GPIO, euDataHigh);
	XM_unlock();
	OS_Delay (50);
}

void icn85xx_set_prog_addr(void)
{  
#ifdef LINUX
      // set INT mode
	//mt_set_gpio_mode(GPIO_CTP_EINT_PIN, GPIO_CTP_EINT_PIN_M_EINT);
	mt_set_gpio_mode(GPIO_CTP_EINT_PIN, GPIO_MODE_00);
	mt_set_gpio_dir(GPIO_CTP_EINT_PIN, GPIO_DIR_OUT);
	mt_set_gpio_pull_enable(GPIO_CTP_EINT_PIN, GPIO_PULL_ENABLE);
	mt_set_gpio_out(GPIO_CTP_EINT_PIN, GPIO_OUT_ZERO);   //????
	msleep(15);
	 icn85xx_ts_reset();
	 mt_set_gpio_out(GPIO_CTP_EINT_PIN, GPIO_OUT_ONE);
	 mt_set_gpio_mode(GPIO_CTP_EINT_PIN, GPIO_CTP_EINT_PIN_M_EINT);
	 mt_set_gpio_dir(GPIO_CTP_EINT_PIN, GPIO_DIR_IN);
	 mt_set_gpio_pull_enable(GPIO_CTP_EINT_PIN, GPIO_PULL_ENABLE);
	 mt_set_gpio_pull_select(GPIO_CTP_EINT_PIN, GPIO_PULL_UP);
	 msleep(15);
	#if 0 
	mt_set_gpio_dir(GPIO_CTP_EINT_PIN, GPIO_DIR_IN);
	mt_set_gpio_pull_select(GPIO_CTP_EINT_PIN, GPIO_PULL_UP);   
    
	mt_eint_registration(CUST_EINT_TOUCH_PANEL_NUM, EINTF_TRIGGER_FALLING, tpd_eint_interrupt_handler, 1);

	mt_eint_unmask(CUST_EINT_TOUCH_PANEL_NUM);
   #endif
#else
	XM_lock();
	SetGPIOPadDirection (GPIO_CTP_EINT_PIN, euOutputPad);
	SetGPIOPadData (GPIO_CTP_EINT_PIN, euDataLow);
	XM_unlock();
	msleep(15);
	icn85xx_ts_reset();
	XM_lock();
	SetGPIOPadData (GPIO_CTP_EINT_PIN, euDataHigh);
	SetGPIOPadDirection (GPIO_CTP_EINT_PIN, euInputPad);
	XM_unlock();
	msleep(15);
#endif
	
}

int icn85xx_i2c_rxdata(unsigned short addr, char *rxdata, int length)
{
	int ret = -1;
	int retries = 0;
	unsigned char tmp_buf[2];
	tmp_buf[0] = (unsigned char)(addr >> 8);
	tmp_buf[1] = (unsigned char)(addr );
	
	while(retries < I2C_RETRY_NUM)
	{
		ret = icn85xx_i2c_read_bytes (ICN85XX_NORMAL_IIC_ADDR, tmp_buf, 2, rxdata, length);
		if(ret == length)
			break;

		retries++;
	}
	if(retries >= I2C_RETRY_NUM)
	{
		icn85xx_error("%s i2c read error: %d, rxdata_len = %d\n", __func__, ret, length);
		ret = -1;
	}	
	return ret;
}

int icn85xx_i2c_txdata(unsigned short addr, char *txdata, int length)
{
	int ret = -1;
	int retries = 0;
	unsigned char tmp_buf[2];
	tmp_buf[0] = (unsigned char)(addr >> 8);
	tmp_buf[1] = (unsigned char)(addr );
	
	while(retries < I2C_RETRY_NUM)
	{
		ret = icn85xx_i2c_write_bytes (ICN85XX_NORMAL_IIC_ADDR, tmp_buf, 2, txdata, length);
		if(ret == 0)
			break;

		retries++;
	}
	if(retries >= I2C_RETRY_NUM)
	{
		icn85xx_error("%s i2c write error: %d, txdata_len = %d\n", __func__, ret, length);
		ret = -1;
	}	
	return ret;

}

int icn85xx_prog_i2c_rxdata(unsigned int addr, char *rxdata, int length)
{
	int ret = -1;
	int retries = 0;
	unsigned char tmp_buf[3];
	tmp_buf[0] = (unsigned char)(addr >> 16);
	tmp_buf[1] = (unsigned char)(addr >> 8);
	tmp_buf[2] = (unsigned char)(addr >> 0);
	
	while(retries < I2C_RETRY_NUM)
	{
		ret = icn85xx_i2c_read_bytes (TPD_ICN85XX_PROG_I2C_ADDR, tmp_buf, 3, rxdata, length);
		if(ret == length)
			break;

		retries++;
	}
	if(retries >= I2C_RETRY_NUM)
	{
		icn85xx_error("%s i2c read error: %d, rxdata_len = %d\n", __func__, ret, length);
		ret = -1;
	}	
	return ret;	
}


int icn85xx_read_reg(unsigned short addr, char *pdata)
{
	int ret = -1;

	ret = icn85xx_i2c_rxdata(addr, pdata, 1);
	if(ret < 0)
	{
		icn85xx_error("addr: 0x%x: 0x%x\n", addr, *pdata); 
	}
	return ret;
}


int icn85xx_prog_i2c_txdata(unsigned int addr, char *txdata, int length)
{
	int ret = -1;
	unsigned char tmp_buf[3];
	int retries = 0;

	tmp_buf[0] = (unsigned char)(addr>>16);
	tmp_buf[1] = (unsigned char)(addr>>8);
	tmp_buf[2] = (unsigned char)(addr);

	while(retries < I2C_RETRY_NUM)
	{
		ret = icn85xx_i2c_write_bytes (TPD_ICN85XX_PROG_I2C_ADDR, tmp_buf, 3, txdata, length);
		if(ret == 0)
			break;
		retries++;
	}

	if(retries >= I2C_RETRY_NUM)
	{
		icn85xx_error("%s i2c prog tx error: %d\n", __func__, ret);
		ret = -1;
	}

	return ret;
}

int icn87xx_prog_i2c_txdata(unsigned int addr, char *txdata, int length)
{
	int ret = -1;
	unsigned char tmp_buf[2];
	int retries = 0;

	tmp_buf[0] = (unsigned char)(addr>>8);
	tmp_buf[1] = (unsigned char)(addr);

	while(retries < I2C_RETRY_NUM)
	{
		ret = icn85xx_i2c_write_bytes (ICN87XX_PROG_IIC_ADDR, tmp_buf, 2, txdata, length);
		if(ret == 0)
			break;
		retries++;
	}

	if(retries >= I2C_RETRY_NUM)
	{
		icn85xx_error("%s i2c prog tx error: %d\n", __func__, ret);
		ret = -1;
	}

	return ret;
}

int icn87xx_prog_i2c_rxdata(unsigned int addr, char *rxdata, int length)
{
	int ret = -1;
	unsigned char tmp_buf[2];
	int retries = 0;
	
	if(length <= 0)
		return -1;

	tmp_buf[0] = (unsigned char)(addr>>8);
	tmp_buf[1] = (unsigned char)(addr);

	while(retries < I2C_RETRY_NUM)
	{
		ret = icn85xx_i2c_read_bytes (ICN87XX_PROG_IIC_ADDR, tmp_buf, 2, rxdata, length);
		if(ret == length)
			break;
		retries++;
	}

	if(retries >= I2C_RETRY_NUM)
	{
		icn85xx_error("%s i2c prog rx error: %d\n", __func__, ret);
		ret = -1;
	}

	return ret;
}

static int icn85xx_i2c_check_id (void)
{
	int  ret = -1;
	char value = 0;
	ret = icn85xx_read_reg(0xa, &value);
	if(ret > 0)
	{
		if(value == 0x85)
		{
			return ret;
		}
		else if((value == 0x86)||(value == 0x88) )
		{
			return ret;  
		}
		else if(value == 0x87)
		{
			return ret;  
		}
	}
	return -1;
}


static int icn85xx_i2c_test(void)
{
	//struct icn85xx_ts_data *icn85xx_ts = i2c_get_clientdata(i2c_client);
	int  ret = -1;
	char value = 0;
	char buf[3];
	int  retry = 0;
	int  flashid;
	icn85xx_ts->ictype = 0;
	icn85xx_trace("====%s begin=====.  \n", __func__);
        icn85xx_ts->ictype = ICTYPE_UNKNOWN;

	while(retry++ < 3)
	{        
		ret = icn85xx_read_reg(0xa, &value);
		if(ret > 0)
		{
			if(value == 0x85)
			{
				icn85xx_ts->ictype = ICN85XX_WITH_FLASH_85;
                       setbootmode(ICN85XX_WITH_FLASH_85);
				return ret;
			}
			else if((value == 0x86)||(value == 0x88) )
			{
				icn85xx_ts->ictype = ICN85XX_WITH_FLASH_86;
                setbootmode(ICN85XX_WITH_FLASH_86);
				return ret;  
			}
            else if(value == 0x87)
            {
                icn85xx_ts->ictype = ICN85XX_WITH_FLASH_87;
                setbootmode(ICN85XX_WITH_FLASH_87);
            return ret;  
            }
        }
		icn85xx_error("iic test error! retry = %d,value=0x%x.\n", retry,value);
		msleep(3);
	}

	icn85xx_goto_progmode();
	msleep(10);
	retry = 0;
	while(retry++ < 3)
	{       
       buf[0] = buf[1] = buf[2] = 0x0;
     ret = icn85xx_prog_i2c_txdata(0x040000,buf,3);
    if (ret < 0) {
                  icn85xx_error("write reg failed! ret: %d\n", ret);
                return ret;
              }
        ret = icn85xx_prog_i2c_rxdata(0x040000, buf, 3);
        icn85xx_trace("icn85xx_prog_i2c_rxdata: ret = %d, buf[0]=0x%2x,buf[0]=0x%2x, buf[0]=0x%2x\n", ret, buf[0], buf[1], buf[2]);
        if(ret > 0)
        {
            //if(value == 0x85)
            if((buf[2] == 0x85) && (buf[1] == 0x05))
            {
                flashid = icn85xx_read_flashid();
                if((MD25D40_ID1 == flashid) || (MD25D40_ID2 == flashid)
                    ||(MD25D20_ID1 == flashid) || (MD25D20_ID2 == flashid)
                    ||(GD25Q10_ID == flashid) || (MX25L512E_ID == flashid)
                    || (MD25D05_ID == flashid)|| (MD25D10_ID == flashid))
                {
                    icn85xx_ts->ictype = ICN85XX_WITH_FLASH_85;
                    setbootmode(ICN85XX_WITH_FLASH_85);

                }
                else
                {
                    icn85xx_ts->ictype = ICN85XX_WITHOUT_FLASH_85;
                    setbootmode(ICN85XX_WITHOUT_FLASH_85);

                }
                return ret;
            }
            else if((buf[2] == 0x85) && (buf[1] == 0x0e))
            {
                flashid = icn85xx_read_flashid();
                if((MD25D40_ID1 == flashid) || (MD25D40_ID2 == flashid)
                    ||(MD25D20_ID1 == flashid) || (MD25D20_ID2 == flashid)
                    ||(GD25Q10_ID == flashid) || (MX25L512E_ID == flashid)
                    || (MD25D05_ID == flashid)|| (MD25D10_ID == flashid))
                {
                    icn85xx_ts->ictype = ICN85XX_WITH_FLASH_86;
                    setbootmode(ICN85XX_WITH_FLASH_86);

                }
                else
                {
                    icn85xx_ts->ictype = ICN85XX_WITHOUT_FLASH_86;
                    setbootmode(ICN85XX_WITHOUT_FLASH_86);

                }
                return ret;
            }
            else  //for ICNT87
            {                
                ret = icn87xx_prog_i2c_rxdata(0xf001, buf, 2);
                if(ret > 0)                    
                {
                    if(buf[1] == 0x87)
                    {                        
                        flashid = icn87xx_read_flashid();                        
                        printk("icnt87 flashid: 0x%x\n",flashid);
                        if(0x114051 == flashid)
                        {
                            icn85xx_ts->ictype = ICN85XX_WITH_FLASH_87;  
                            setbootmode(ICN85XX_WITH_FLASH_87);
                        }                        
                        else
                        {
                            icn85xx_ts->ictype = ICN85XX_WITHOUT_FLASH_87;  
                            setbootmode(ICN85XX_WITHOUT_FLASH_87);
                        }
                        return ret;
                    }
								}
						}
				}
				icn85xx_error("i2c test error! %d\n",retry);
				msleep(3);
			}
			return ret;
	}

static void icn85xx_hw_init(void)
{
	unsigned int val;

	icn85xx_trace("icn85xx_hw_init!\n");
	
#if TULV_BB_TEST
	// 途旅TP开发板
	//	INT	CD1 (GPIO81), Input
	//	RST	CD2 (GPIO82), Output
	val = rSYS_PAD_CTRL08;
	// val &= ~( (3 << 2) | (3 << 4) | (3 << 10) | (3 << 12) );
	val &= ~( (3 << 2) | (3 << 4)  );
	rSYS_PAD_CTRL08 = val;
#else
	// 途旅产品板
	// INT	SD1_D0(GPIO101)
	// RST	SD1_D1(GPIO102)
	// pad_ctl9
	// [7]	sd1_d0	sd1_d0_pad	GPIO101	sd1_data0
	// [8]	sd1_d1	sd1_d1_pad	GPIO102	sd1_data1
	// [11]	sd1_cmd	sd1_cmd_pad	GPIO105	sd1_cmd
	// [13]	sd1_clk	sd1_clk_pad	GPIO107	sdmmc1_cclk_out
	val = rSYS_PAD_CTRL09;
	val &= ~( (1 << 7) | (1 << 8) );
	rSYS_PAD_CTRL09 = val;
#endif
	
	SetGPIOPadDirection (INT_GPIO, euInputPad);
	SetGPIOPadDirection (RST_GPIO, euOutputPad);
	SetGPIOPadData (RST_GPIO, euDataHigh);
	icn85xx_ts_reset ();
}

static void tp_icn85xx_intr (void)
{
	icn85xx_trace("TPD interrupt has been triggered\n");
	//tpd_flag = 1;
	OS_SignalEvent(XMSYS_TP_EVENT, &TCB_icn85xxTask); /* 通知事件 */
}

static int  tpd_probe(void)
{
	int retval = TPD_OK;
	int err = 0;
	short fwVersion = 0;
	short curVersion = 0;
	int retry;
//**************************************************************//
#ifdef TPD_PROXIMITY
	int errpro =0;
	struct hwmsen_object obj_ps;
#endif
//**************************************************************//	
#ifdef  TPD_SLIDE_WAKEUP
	struct class *cls;
#endif
//**************************************************************//	

	//i2c_client = client;

#if TPD_ICN85XX_I2C_DMA_SUPPORT

	i2c_client->addr = i2c_client->addr & I2C_MASK_FLAG;
	i2c_client->timing = 400;

	icn85xx_i2c_dma_va = (u8 *)dma_alloc_coherent(NULL, 4096, &icn85xx_i2c_dma_pa, GFP_KERNEL);
	if(!icn85xx_i2c_dma_va)
	{
		icn85xx_trace(KERN_ERR "%s, TPD dma_alloc_coherent error!\n", __FUNCTION__);
	}
	else
	{
		icn85xx_trace("%s, TPD dma_alloc_coherent success!\n", __FUNCTION__);
	}

	mutex_init(&i2c_data_mutex);

#endif

	icn85xx_ts = kernel_malloc(sizeof(*icn85xx_ts));
	if(!icn85xx_ts)
	{
		icn85xx_trace("Alloc icn85xx_ts memory failed.\n");
		return -1;
	}
	memset(icn85xx_ts, 0, sizeof(*icn85xx_ts));
	//i2c_set_clientdata(client, icn85xx_ts);

	icn85xx_ts->work_mode = 0;

	icn85xx_hw_init();

	err = icn85xx_i2c_test();
	if (err <= 0)
	{
		kernel_free (icn85xx_ts);
		icn85xx_ts = NULL;
		icn85xx_trace("icn85xx_iic_test  failed.\n");
		return -1;

	}
	else
	{
		icn85xx_trace("iic communication ok: 0x%x\n", icn85xx_ts->ictype); 
	}

	//tpd_load_status = 1;
	if((icn85xx_ts->ictype == ICN85XX_WITHOUT_FLASH_85) || (icn85xx_ts->ictype == ICN85XX_WITHOUT_FLASH_86))
	{
#if TPD_ICN85XX_COMPILE_FW_WITH_DRIVER
		icn85xx_set_fw(sizeof(icn85xx_fw), (unsigned char *)&icn85xx_fw[0]);
#endif    

		if(R_OK == icn85xx_fw_update(firmware))
		{
			icn85xx_ts->code_loaded_flag = 1;
			icn85xx_trace("ICN85XX_WITHOUT_FLASH, update ok\n"); 

		}
		else
		{
			kernel_free (icn85xx_ts);
			icn85xx_ts = NULL;
			icn85xx_ts->code_loaded_flag = 0;
			icn85xx_trace("ICN85XX_WITHOUT_FLASH, update error\n"); 
			return -1;
		}

	}
	else if((icn85xx_ts->ictype == ICN85XX_WITH_FLASH_85) || (icn85xx_ts->ictype == ICN85XX_WITH_FLASH_86))
	{
#if TPD_ICN85XX_SUPPORT_FW_UPDATE
		icn85xx_set_fw(sizeof(icn85xx_fw), &icn85xx_fw[0]);
		fwVersion = icn85xx_read_fw_Ver(firmware);
		curVersion = icn85xx_readVersion();
		icn85xx_trace("fwVersion : 0x%x\n", fwVersion);
		icn85xx_trace("current version: 0x%x\n", curVersion);

#if TPD_ICN85XX_FORCE_UPDATE_FW
		retry = 5;
		while(retry > 0)
		{
			if(icn85xx_goto_progmode() != 0)
			{
				kernel_free (icn85xx_ts);
				icn85xx_ts = NULL;
				
				printk("icn85xx_goto_progmode() != 0 error\n");
				return -1; 
			} 
			icn85xx_read_flashid();
			printk("begin to update\n");
			if(R_OK == icn85xx_fw_update(firmware))
			{
				break;
			}
			retry--;
			icn85xx_error("icn85xx_fw_update failed.\n");
		}
#else
		if(fwVersion > curVersion)
		{
			retry = 5;
			while(retry > 0)
			{
				if(R_OK == icn85xx_fw_update(firmware))
				{
					break;
				}
				retry--;
				icn85xx_error("icn85xx_fw_update failed.\n");
			}
		}
#endif
#endif
	}
    else if(icn85xx_ts->ictype == ICN85XX_WITHOUT_FLASH_87)
    {
        icn85xx_trace("icn85xx_update  87 without flash\n");
        
        
        #if TPD_ICN85XX_COMPILE_FW_WITH_DRIVER
            icn85xx_set_fw(sizeof(icn85xx_fw), (unsigned char *)&icn85xx_fw[0]);
        #endif
        
        fwVersion = icn85xx_read_fw_Ver(firmware);
        curVersion = icn85xx_readVersion();
        icn85xx_trace("fwVersion : 0x%x\n", fwVersion); 
        icn85xx_trace("current version: 0x%x\n", curVersion);
  
        if(R_OK == icn87xx_fw_update(firmware))
        {
            icn85xx_ts->code_loaded_flag = 1;
            printk("ICN87XX_WITHOUT_FLASH, update default fw ok\n");
        }
        else
        {
            icn85xx_ts->code_loaded_flag = 0;
            printk("ICN87XX_WITHOUT_FLASH, update error\n"); 
        }
     
    }
    else if(icn85xx_ts->ictype == ICN85XX_WITH_FLASH_87)
    {
        printk("icn85xx_update 87 with flash\n");
           
        #if TPD_ICN85XX_COMPILE_FW_WITH_DRIVER
            icn85xx_set_fw(sizeof(icn85xx_fw), (unsigned char *)&icn85xx_fw[0]);
        #endif
        
        fwVersion = icn85xx_read_fw_Ver(firmware);
        curVersion = icn85xx_readVersion();
        icn85xx_trace("fwVersion : 0x%x\n", fwVersion); 
        icn85xx_trace("current version: 0x%x\n", curVersion); 
             
        
        
        #if TPD_ICN85XX_FORCE_UPDATE_FW        
            if(R_OK == icn87xx_fw_update(firmware))
            {
                icn85xx_ts->code_loaded_flag = 1;
                icn85xx_trace("ICN87XX_WITH_FLASH, update default fw ok\n");
            }
            else
            {
                icn85xx_ts->code_loaded_flag = 0;
                icn85xx_trace("ICN87XX_WITH_FLASH, update error\n"); 
            }    
         
        #else
            if(fwVersion > curVersion)
            {
                retry = 5;
                while(retry > 0)
                {
                    if(R_OK == icn87xx_fw_update(firmware))
                    {
                        break;
                    }
                    retry--;
                    icn85xx_error("icn87xx_fw_update failed.\n");   
                }
            }
        #endif
    }



#if SUPPORT_PROC_FS
	sema_init(&icn85xx_ts->sem, 1);
	init_proc_node();
#endif

//*************************************************//
#ifdef TPD_PROXIMITY
		hwmsen_detach(ID_PROXIMITY);
		obj_ps.self = NULL;
		obj_ps.polling = 0;//interrupt mode
		obj_ps.sensor_operate = tpd_ps_operate_inc85xx;//gsl1680p_ps_operate;
		if((errpro = hwmsen_attach(ID_PROXIMITY, &obj_ps)))
		{
			icn85xx_trace("attach fail = %d\n", errpro);
		}
		wake_lock_init(&ps_lock, WAKE_LOCK_SUSPEND, "ps wakelock");
#endif

//****************************************************//


#ifdef  TPD_SLIDE_WAKEUP
    input_set_capability(tpd->dev, EV_KEY, KEY_POWER);
    input_set_capability(tpd->dev, EV_KEY, KEY_C);
    input_set_capability(tpd->dev, EV_KEY, KEY_S);
    input_set_capability(tpd->dev, EV_KEY, KEY_V);
    input_set_capability(tpd->dev, EV_KEY, KEY_Z);
    input_set_capability(tpd->dev, EV_KEY, KEY_M);
    input_set_capability(tpd->dev, EV_KEY, KEY_E);
    input_set_capability(tpd->dev, EV_KEY, KEY_O);
    input_set_capability(tpd->dev, EV_KEY, KEY_W);
    input_set_capability(tpd->dev, EV_KEY, KEY_UP);
    input_set_capability(tpd->dev, EV_KEY, KEY_DOWN);
    input_set_capability(tpd->dev, EV_KEY, KEY_LEFT);
    input_set_capability(tpd->dev, EV_KEY, KEY_RIGHT);
	input_set_capability(tpd->dev, EV_KEY, KEY_POWER);
	mutex_init(&twd.lock);
	twd.gestrue_pre_en = twd.gestrue_en = false;
	twd.suspended = false;
	//twd.glove_mode = GLOVE_MODE_DISABLE;
	cls = class_create(THIS_MODULE,"tp_gesture_switch");
	if(cls){
		retval=class_create_files(cls,&cls_attr,ARRAY_SIZE(cls_attr));

	}
#endif

#if 0
	thread = kthread_run(touch_event_handler, 0, TPD_DEVICE);
	if(IS_ERR(thread))
	{
		retval = PTR_ERR(thread);
		icn85xx_trace(TPD_DEVICE " failed to create kernel thread: %d\n", retval);
	}
#endif
   
#if 0 
	mt_set_gpio_dir(GPIO_CTP_EINT_PIN, GPIO_DIR_IN);
	mt_set_gpio_pull_select(GPIO_CTP_EINT_PIN, GPIO_PULL_UP);   
	mt_eint_registration(CUST_EINT_TOUCH_PANEL_NUM, EINTF_TRIGGER_FALLING, tpd_eint_interrupt_handler, 1);
	mt_eint_unmask(CUST_EINT_TOUCH_PANEL_NUM);
#else
	
	XM_lock ();
	SetGPIOPadDirection (INT_GPIO, euInputPad);
	GPIO_IRQ_Request (INT_GPIO, euHightLevelTrig, tp_icn85xx_intr);
	EnableGPIOPadIRQ (INT_GPIO);
	XM_unlock ();
#endif

#if SUPPORT_CHECK_ESD
	INIT_DELAYED_WORK(&icn85xx_esd_event_work, icn85xx_esd_check_work);
	icn85xx_esd_workqueue = create_workqueue("icn85xx_esd");

	icn85xx_ts->clk_tick_cnt = 1 * HZ;      // HZ: clock ticks in 1 second generated by system
	icn85xx_trace("Clock ticks for an esd cycle: %d", icn85xx_ts->clk_tick_cnt);
	spin_lock_init(&icn85xx_ts->esd_lock);

	spin_lock(&icn85xx_ts->esd_lock);
	if (!icn85xx_ts->esd_running)
    {
		icn85xx_ts->esd_running = 1;
		spin_unlock(&icn85xx_ts->esd_lock);
		icn85xx_trace("Esd started");
		queue_delayed_work(icn85xx_esd_workqueue, &icn85xx_esd_event_work, icn85xx_ts->clk_tick_cnt);
	}
#endif
	

	//tpd_load_status = 1;

	icn85xx_trace("Touch Panel Device Probe %s\n", (retval < TPD_OK) ? "FAIL" : "PASS");

	return retval;
}

static void icn85xx_ts_release(void)
{
	icn85xx_trace("%s() Line:%d\n", __FUNCTION__, __LINE__);
#ifdef LINUX
	//	input_report_abs(tpd->dev, ABS_MT_TOUCH_MAJOR, 0);
	input_report_key(tpd->dev, BTN_TOUCH, 0);
	input_mt_sync(tpd->dev);
	input_sync(tpd->dev);
#endif

       if(tp_down==1)
       {
           // printf("icn85xx_ts_release\n");
	      tp_down = 0;
		XM_TouchEventProc (MAKELONG(icn85xx_ts->point_info[0].u16PosX, icn85xx_ts->point_info[0].u16PosY), TOUCH_TYPE_UP);
       }
}


static int icn85xx_report_value_A(void)
{
	icn85xx_trace("%s() Line:%d\n", __FUNCTION__, __LINE__);
	
	char buf[POINT_NUM * POINT_SIZE + 3] = {0};
	int ret = -1;
	int i;
	int keys[TPD_KEY_COUNT];

#ifdef TPD_SLIDE_WAKEUP
	icn85xx_gesture_handler();
#endif

	ret = icn85xx_i2c_rxdata(0x1000, buf, POINT_NUM * POINT_SIZE + 2);
	if(ret < 0)
	{
		icn85xx_trace("%s read_data i2c_rxdata failed: %d\n", __func__, ret);
		return ret;
	}
	
#ifdef TPD_PROXIMITY
	icn85xx_proximity_handler(buf[0]>>7);	
#endif

#ifdef TPD_HAVE_BUTTON 
		unsigned char button;
		static unsigned char button_last;

		button = buf[0]&0x0f;
		icn85xx_trace("button=%d\n", button);    

		if((button_last != 0) && (button == 0))
		{
			tpd_up(tpd_keys_dim_local[i][0],tpd_keys_dim_local[i][1], 0);
			button_last = button;
			return 1;       
		}
		if(button != 0)
		{

			//tpd_down(tpd_keys_dim_local[i][0],tpd_keys_dim_local[i][1], 0, 0);
			for(i = 0; i < TPD_KEY_COUNT; i++)
			{

				if (button & (0x01 << i))
				{

					icn85xx_info("%s: button tpd_down=%d , i = %d\n",__func__, button,i);

					tpd_down(tpd_keys_dim_local[i][0],tpd_keys_dim_local[i][1], 0, 0);
				}     

			}   
			button_last = button;
			return 1;
		}
#endif
		
	icn85xx_ts->point_num = buf[1];
	//icn85xx_trace(" icn85xx zby %d \n",icn85xx_ts->point_num );
	if(icn85xx_ts->point_num == 0)
	{
		icn85xx_ts_release();
		return 1;
	}

	for(i = 0; i < icn85xx_ts->point_num; i++)
	{
		if(buf[8 + POINT_SIZE * i] != 4)
			break;
	}

	if(i == icn85xx_ts->point_num)
	{
		icn85xx_ts_release();
		return 1;
	}

	for(i = 0; i < icn85xx_ts->point_num; i++)
	//for(i = 0; i < 1; i++)
	{
		icn85xx_ts->point_info[i].u8ID = buf[2 + POINT_SIZE*i];
		icn85xx_ts->point_info[i].u16PosX = (buf[4 + POINT_SIZE*i]<<8) + buf[3 + POINT_SIZE*i];
		icn85xx_ts->point_info[i].u16PosY = (buf[6 + POINT_SIZE*i]<<8) + buf[5 + POINT_SIZE*i];
		icn85xx_ts->point_info[i].u8Pressure = 200;//buf[7 + POINT_SIZE*i];
		icn85xx_ts->point_info[i].u8EventId = buf[8 + POINT_SIZE*i];    

		if(1 == icn85xx_ts->revert_x_flag)
			icn85xx_ts->point_info[i].u16PosX = TPD_RES_X - icn85xx_ts->point_info[i].u16PosX;
		if(1 == icn85xx_ts->revert_y_flag)
			icn85xx_ts->point_info[i].u16PosY = TPD_RES_Y - icn85xx_ts->point_info[i].u16PosY;
		icn85xx_trace("u8ID %d\n", icn85xx_ts->point_info[i].u8ID);
		icn85xx_trace ("u16PosX %d\n", icn85xx_ts->point_info[i].u16PosX);
		icn85xx_trace("u16PosY %d\n", icn85xx_ts->point_info[i].u16PosY);
		icn85xx_trace("u8Pressure %d\n", icn85xx_ts->point_info[i].u8Pressure);
		icn85xx_trace("u8EventId %d\n", icn85xx_ts->point_info[i].u8EventId);
		
		switch(icn85xx_ts->point_info[i].u8EventId)
		{
			case 1:		// DOWN
			      if(tp_down==0)
			      	{
				   tp_down = 1;
				    timeout_to_check_tp_release = XM_GetTickCount() + TP_TIMEOUT;
				    printf ("DOWN x=%d, y=%d\n", icn85xx_ts->point_info[i].u16PosX, icn85xx_ts->point_info[i].u16PosY);
				   XM_TouchEventProc (MAKELONG(icn85xx_ts->point_info[i].u16PosX, icn85xx_ts->point_info[i].u16PosY), TOUCH_TYPE_DOWN);
			      	}
				break;
				
			case 2:		// MOVE
				
                            if(tp_down==0)
			      	{
			      	     printf ("MOVE x=%d, y=%d\n", icn85xx_ts->point_info[i].u16PosX, icn85xx_ts->point_info[i].u16PosY);
				     tp_down = 1;
				    XM_TouchEventProc (MAKELONG(icn85xx_ts->point_info[i].u16PosX, icn85xx_ts->point_info[i].u16PosY), TOUCH_TYPE_DOWN);
                             }
				timeout_to_check_tp_release = XM_GetTickCount() + TP_TIMEOUT;		// 更新超时时间
				break;
				
			case 4:		// UP
				printf ("UP x=%d, y=%d\n", icn85xx_ts->point_info[i].u16PosX, icn85xx_ts->point_info[i].u16PosY);
				tp_down = 0;
				XM_TouchEventProc (MAKELONG(icn85xx_ts->point_info[i].u16PosX, icn85xx_ts->point_info[i].u16PosY), TOUCH_TYPE_UP);
				break;
				
			case 3:
				timeout_to_check_tp_release = XM_GetTickCount() + TP_TIMEOUT;		// 更新超时时间
				break;
				
			default:
				//printf ("event %d x=%d, y=%d\n", icn85xx_ts->point_info[i].u8EventId, icn85xx_ts->point_info[i].u16PosX, icn85xx_ts->point_info[i].u16PosY);
				break;
		}

#ifdef LINUX
		input_report_key(tpd->dev, BTN_TOUCH, 1);
		input_report_abs(tpd->dev, ABS_MT_TRACKING_ID, icn85xx_ts->point_info[i].u8ID);
		input_report_abs(tpd->dev, ABS_MT_TOUCH_MAJOR, 1);
		input_report_abs(tpd->dev, ABS_MT_POSITION_X, icn85xx_ts->point_info[i].u16PosX);
		input_report_abs(tpd->dev, ABS_MT_POSITION_Y, icn85xx_ts->point_info[i].u16PosY);
		//input_report_abs(tpd->dev, ABS_MT_WIDTH_MAJOR, 1);
		input_mt_sync(tpd->dev);
	//	icn85xx_point_info("point: %d ===x = %d,y = %d, press = %d ====\n",i, icn85xx_ts->point_info[i].u16PosX,icn85xx_ts->point_info[i].u16PosY, icn85xx_ts->point_info[i].u8Pressure);
#endif
	}
#ifdef LINUX
	input_sync(tpd->dev);
#endif

	return 0;
}

#if CTP_REPORT_PROTOCOL
static void icn85xx_report_value_B(void)
{
	struct icn85xx_ts_data *icn85xx_ts = i2c_get_clientdata(i2c_client);
	char buf[POINT_NUM * POINT_SIZE + 3] = {0};
	static unsigned char finger_last[POINT_NUM + 1] = {0};
	unsigned char finger_current[POINT_NUM + 1] = {0};
	unsigned int position = 0;
	int temp = 0;
	int ret = -1;

	icn85xx_info("==icn85xx_report_value_B ==\n");

	ret = icn85xx_i2c_rxdata(0x1000, buf, POINT_NUM * POINT_SIZE + 2);
	if(ret < 0)
	{
		icn85xx_error("%s read_data i2c_rxdata failed: %d\n", __func__, ret);
		return ret;
	}

	icn85xx_ts->point_num = buf[1];
	if(icn85xx_ts->point_num > 0)
	{
		for(position = 0; position < icn85xx_ts->point_num; position++)
		{
			temp = buf[2 + POINT_SIZE * position] + 1;
			finger_current[temp] = 1;
			icn85xx_ts->point_info[temp].u8ID = buf[2 + POINT_SIZE*position];
			icn85xx_ts->point_info[temp].u16PosX = (buf[4 + POINT_SIZE*position]<<8) + buf[3 + POINT_SIZE*position];
			icn85xx_ts->point_info[temp].u16PosY = (buf[6 + POINT_SIZE*position]<<8) + buf[5 + POINT_SIZE*position];
			icn85xx_ts->point_info[temp].u8Pressure = buf[7 + POINT_SIZE*position];
			icn85xx_ts->point_info[temp].u8EventId = buf[8 + POINT_SIZE*position];

			if(icn85xx_ts->point_info[temp].u8EventId == 4)
				finger_current[temp] = 0;

			if(1 == icn85xx_ts->revert_x_flag)
				icn85xx_ts->point_info[temp].u16PosX = icn85xx_ts->screen_max_x - icn85xx_ts->point_info[temp].u16PosX;


			if(1 == icn85xx_ts->revert_y_flag)
				icn85xx_ts->point_info[temp].u16PosY = icn85xx_ts->screen_max_y - icn85xx_ts->point_info[temp].u16PosY;

			icn85xx_info("temp %d\n", temp);
			icn85xx_info("u8ID %d\n", icn85xx_ts->point_info[temp].u8ID);
			icn85xx_info("u16PosX %d\n", icn85xx_ts->point_info[temp].u16PosX);
			icn85xx_info("u16PosY %d\n", icn85xx_ts->point_info[temp].u16PosY);
			icn85xx_info("u8Pressure %d\n", icn85xx_ts->point_info[temp].u8Pressure);
			icn85xx_info("u8EventId %d\n", icn85xx_ts->point_info[temp].u8EventId);
		}
	}
	else
	{
		for(position = 1; position < POINT_NUM + 1; position++)
		{
			finger_current[position] = 0;
		}
		icn85xx_info("no touch\n");
	}

	for(position = 1; position < POINT_NUM + 1; position++)
	{
		if((finger_current[position] == 0) && (finger_last[position] != 0))
		{
			input_mt_slot(tpd->dev, position - 1);
			input_mt_report_slot_state(tpd->dev, MT_TOOL_FINGER, false);
			icn85xx_point_info("one touch up: %d\n", position);
		}
		else if(finger_current[position])
		{
			input_mt_slot(tpd->dev, position - 1);
			input_mt_report_slot_state(tpd->dev, MT_TOOL_FINGER, true);
			input_report_key(tpd->dev, BTN_TOUCH, 1);
			input_report_abs(tpd->dev, ABS_MT_TOUCH_MAJOR, 1);
			input_report_abs(tpd->dev, ABS_MT_PRESSURE, 200);
			input_report_abs(tpd->dev, ABS_MT_POSITION_X, icn85xx_ts->point_info[position].u16PosX);
			input_report_abs(tpd->dev, ABS_MT_POSITION_Y, icn85xx_ts->point_info[position].u16PosY);
			icn85xx_point_info("===position: %d, x = %d,y = %d, press = %d ====\n", position, icn85xx_ts->point_info[position].u16PosX,icn85xx_ts->point_info[position].u16PosY, icn85xx_ts->point_info[position].u8Pressure);
		}
	}
	input_sync(tpd->dev);

	for(position = 1; position < POINT_NUM + 1; position++)
		finger_last[position] = finger_current[position];
}
#endif

static void tp_icn85xxTask (void) 
{
	int probe_count = 3;
	unsigned int ticket_no_response;		// 无响应计数
	XM_printf ("icn85xx probe...\n");
tp_probe:
	while(tpd_probe () < 0)
	{
		if(icn85xx_ts)
		{
			kernel_free (icn85xx_ts);
			icn85xx_ts = NULL;
		}
		OS_Delay (100);
		probe_count --;
		if(probe_count == 0)
		{
			OS_Terminate (NULL);
		}
	}
	XM_printf ("icn85xx probe ok\n");
	
	// 创建16hz的定时器
	OS_CREATETIMER (&TpTimer, TpTicketCallback, 64);
	
	ticket_no_response = 0;
	while(1)
	{
		OS_U8 tp_event = 0;
		
		tp_event = OS_WaitEvent (		XMSYS_TP_EVENT
										 	|	XMSYS_TP_EVENT_TICKET
										);
		
		if(tp_event & (XMSYS_TP_EVENT) )
		{
			ticket_no_response = 0;		// 清除无响应计数
			if(icn85xx_ts->work_mode == 0)
			{
#if CTP_REPORT_PROTOCOL
				icn85xx_report_value_B();
#else
				icn85xx_report_value_A();
#endif
			}
		}
		if(tp_event & (XMSYS_TP_EVENT_TICKET) )
		{
			if(tp_down)
			{
				if(XM_GetTickCount() >= timeout_to_check_tp_release)
				{
					// 超时
					if(icn85xx_report_value_A() != (-1))
					{
						ticket_no_response ++;
					}
				}
			}
			else
			{
				// 检查是否无应答
				if(icn85xx_i2c_check_id () < 0)
				{
					printf ("ticket_no_response=%d\n", ticket_no_response);
					ticket_no_response ++;
				}
				else
				{
					ticket_no_response = 0;
				}
			}
		}
		
		if(ticket_no_response >= 32)	// 16*32=512ms
		{
			// TP无响应超过512ms, 准备重新初始化
			printf ("TP no response\n");
			break;
		}
	}
	
	OS_DeleteTimer (&TpTimer);
	XM_lock ();
	//DisableGPIOPadIRQ (INT_GPIO);
	XM_unlock ();
	XM_printf ("icn85xx re-probe...\n");
	
	if(icn85xx_ts)
	{
		kernel_free (icn85xx_ts);
		icn85xx_ts = NULL;
	}
	if(tp_device)
	{
		xm_i2c_close (tp_device);
		tp_device = NULL;
	}
	
	goto tp_probe;
}

void XMSYS_TpInit (void)
{
	// GPIO I2C
	GPIO_I2C_Init ();
	OS_CREATETASK(&TCB_icn85xxTask, "icn85xxTask", tp_icn85xxTask, XMSYS_icn85xx_TASK_PRIORITY, Stackicn85xxTask);
}


void XMSYS_TpExit (void)
{
	
}

int tp_icn85xx_reset (void)
{
	return 0;
}

#endif


