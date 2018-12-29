
#ifdef _TP_GT9XX_

#include "tp_gt9xx.h"
#include "gpio.h"
#include "xm_core.h"
#include "gpioi2c.h"
#include "xm_core.h"
#include "xm_uart.h"
#include "xm_key.h"
#include "xm_dev.h"
#include "xm_i2c.h"


#define	XM_printf(...)	
#define	printk	XM_printf
#define	msleep	OS_Delay


// RST  SD1_D1/GP102
// INT	SD1_D0/GP101
#define	INT_GPIO				GPIO101
#define	RST_GPIO				GPIO102
#define	GPIO_CTP_EINT_PIN		INT_GPIO


#define	XMSYS_gt9xx_TASK_PRIORITY		254
#define	XMSYS_gt9xx_TASK_STACK_SIZE		0x800

#define	XMSYS_TP_EVENT						0x01
#define	XMSYS_TP_EVENT_TICKET				0x02

#define TPD_GT9XX_I2C_DMA_SUPPORT    	0

#define	TP_TIMEOUT		64		// 64毫秒超时释放检测

#define TPD_OK   0
#define TPD_KEY_COUNT	3

#define I2C_RETRY_NUM			3

static OS_TASK TCB_gt9xxTask;
struct goodix_ts_data *ts_init = NULL;
static OS_TIMER TpTimer;

static volatile int tp_down = 0;
static unsigned int timeout_to_check_tp_release;
__no_init static OS_STACKPTR int Stackgt9xxTask[XMSYS_gt9xx_TASK_STACK_SIZE/4];     /* Task stacks */

#define	TP_HW_I2C

#ifdef TP_HW_I2C		// 使用硬件I2C, 与sensor复用
#undef gt9xx_i2c_read_bytes
#undef gt9xx_i2c_write_bytes

static xm_i2c_client_t tp_device = NULL;
static unsigned int tp_address = 0;
#define GOODIX_I2C_DEV_ADDR  (0X28>>1)

static int gt9xx_i2c_read_bytes (unsigned int slvaddr, unsigned char* addr, int size, char *rxdata, int length)
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
								 "GT9XX",
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

	/*
	if(tp_device)
		xm_i2c_close (tp_device);
	*/
	return ret;
}


static int gt9xx_i2c_write_bytes (unsigned int slvaddr, unsigned char* addr, int size, char *txdata, int length)
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
								 "GT9XX",
								 XM_I2C_ADDRESSING_7BIT,
								 &io_ret);
			if(tp_device == NULL)
				break;
			tp_address = slvaddr;
		}
		 
		if(xm_i2c_write (tp_device, (u8_t *)addr, size, 100) < 0)
			break;
		
		/*
		if(xm_i2c_write  (tp_device, (u8_t *)txdata, length, 100) < 0)
		{
			break;
		}
		*/
		
		ret = 0;
		
	} while (0);

	/*
	if(tp_device)
		xm_i2c_close (tp_device);
	*/
	return ret;
}

int gtp_i2c_write(unsigned short addr, char *txdata, int length)
{
	int ret = -1;
	int retries = 0;
	uchar i;
	unsigned char tmp_buf[3];
	
	tmp_buf[0] = (unsigned char)(addr >> 8);
	tmp_buf[1] = (unsigned char)(addr );
	tmp_buf[2] = (unsigned char)*txdata;
		
	while(retries < I2C_RETRY_NUM)
	{
		ret = gt9xx_i2c_write_bytes (GOODIX_I2C_DEV_ADDR, tmp_buf, 3, txdata, length);
		if(ret == 0)
			break;

		retries++;
	}
	if(retries >= I2C_RETRY_NUM)
	{
		XM_printf("%s i2c write error: %d, txdata_len = %d\n", __func__, ret, length);
		ret = -1;
	}	
	return ret;

}

int gtp_i2c_read(unsigned short addr, char *rxdata, int length)
{
	int ret = -1;
	int retries = 0;
	unsigned char tmp_buf[2];
	tmp_buf[0] = (unsigned char)(addr>>8);
	tmp_buf[1] = (unsigned char)(addr);
	
	while(retries < I2C_RETRY_NUM)
	{
		ret = gt9xx_i2c_read_bytes (GOODIX_I2C_DEV_ADDR, tmp_buf, 2, rxdata, length);
		if(ret == length)
			break;

		retries++;
	}
	if(retries >= I2C_RETRY_NUM)
	{
		XM_printf("%s i2c read error: %d, rxdata_len = %d\n", __func__, ret, length);
		ret = -1;
	}	
	return ret;
}
#endif


static void TpTicketCallback (void)
{
	OS_SignalEvent(XMSYS_TP_EVENT_TICKET, &TCB_gt9xxTask); /* 通知事件 */
	OS_RetriggerTimer (&TpTimer);
}

void gd9xx_ts_reset(void)
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

int gt9xx_read_reg(unsigned short addr, char *pdata)
{
	int ret = -1;

	ret = gtp_i2c_read(addr, pdata, 1);
	if(ret < 0)
	{
		XM_printf("addr: 0x%x: 0x%x\n", addr, *pdata); 
	}
	return ret;
}

int gd9xx_read_ver(unsigned short addr, int *pdata)
{
	int ret = -1;
	uchar buf[8] = {0};

	ret = gtp_i2c_read(GTP_REG_VERSION, buf, 6);
	if (ret < 0)
	{
		XM_printf("GTP read version failed");
		return ret;
	}

	if (pdata)
	{
		*pdata = (buf[5] << 8) | buf[4];
	}

	if (buf[3] == 0x00)
	{
		XM_printf("IC Version: %c%c%c_%02x%02x\n", buf[0], buf[1], buf[2], buf[5], buf[4]);
	}
	else
	{
		if (buf[5] == 'S' || buf[5] == 's')
		{
		}
		XM_printf("IC ), X_MAX: 480, Y_MAX: 800, TRIGGER: 0x01: %c%c%c%c_%02x%02x", buf[0], buf[1], buf[2], buf[3], buf[4], buf[5]);
	}
	return ret;
}

int GT9xx_write_coor_clear()
{
	int ret = -1;
	uchar data_coor_clear = 0x00;

	ret = gtp_i2c_write(GTP_READ_COOR_STATUS, &data_coor_clear, 1);
	if (ret < 0)
	{
		XM_printf("GTP write coor clear failed");
		return ret;
	}
	return ret;
}


static int gt9xx_i2c_check_id (void)
{
	int  ret = -1;
	char value = 0;
	ret = gt9xx_read_reg(GTP_REG_SENSOR_ID, &value);
	if(ret > 0)
	{
		//XM_printf("GTP i2c id:%d\n",value);
		return ret;  
	}
	return -1;
}


static int gd9xx_i2c_test(void)
{
	int  ret = -1;
	char value = 0;
	char buf[3];
	int  retry = 0;

	char test[3] = {GTP_REG_CONFIG_DATA >> 8, GTP_REG_CONFIG_DATA & 0xff};
	while(retry++ < 5)
	{
		ret = gtp_i2c_read(GTP_REG_CONFIG_DATA, test, 3);
		if (ret > 0)
		{
			XM_printf("GTP i2c test successful.\n");
			return ret;
		}
		XM_printf("GTP i2c test failed time %d.",retry);
		msleep(10);
	}
	return ret;
}

static void gt9xx_hw_init(void)
{
	unsigned int val;
	uchar j;
	
	XM_printf("GT9XX_hw_init!\n");
	
	// RST  SD1_D1/GP102
	// INT	SD1_D0/GP101
	val = rSYS_PAD_CTRL09;
	val &= ~( (1 << 6) | (1 << 7) );
	rSYS_PAD_CTRL09 = val;

	SetGPIOPadDirection (INT_GPIO, euOutputPad);
	SetGPIOPadDirection (RST_GPIO, euOutputPad);
	
	SetGPIOPadData (RST_GPIO, euDataLow);
	OS_Delay (1);
	SetGPIOPadData (INT_GPIO, euDataLow);
	OS_Delay (1);
	SetGPIOPadData (INT_GPIO, euDataHigh);
	OS_Delay (1);
	SetGPIOPadData (RST_GPIO, euDataHigh);
	OS_Delay (10);
    SetGPIOPadData (INT_GPIO, euDataLow);
	OS_Delay (80);
	SetGPIOPadDirection (INT_GPIO, euInputPad);
	OS_Delay (60);
	
	for(j=0;j<5;j++)
	{
		GT9xx_write_coor_clear();
	}
}

static void tp_gt9xx_intr (void)
{
	XM_printf("TPD interrupt has been triggered\n");
	OS_SignalEvent(XMSYS_TP_EVENT, &TCB_gt9xxTask); /* 通知事件 */
}

static int  tpd_probe(void)
{
	int retval = TPD_OK;
	int err = 0;
	int ver_gt9xx = 0;
	int retry;
	int ret;

	ts_init = kernel_malloc(sizeof(*ts_init));
	if(!ts_init)
	{
		XM_printf("Alloc gd9xx_ts memory failed.\n");
		return -1;
	}
	memset(ts_init, 0, sizeof(*ts_init));
	ts_init->work_mode = 0;

	gt9xx_hw_init();
	OS_Delay (100); 
	err = gd9xx_i2c_test();
	if (err <= 0)
	{
		kernel_free (ts_init);
		ts_init = NULL;
		XM_printf("gd9xx_iic_test  failed.\n");
		return -1;

	}
	ret = gd9xx_read_ver(GTP_REG_VERSION,&ver_gt9xx);
	if(ret >0)
	{
		XM_printf("GTP ver:%x\n",ver_gt9xx);
	}
   
	XM_lock ();
	SetGPIOPadDirection (INT_GPIO, euInputPad);
	GPIO_IRQ_Request (INT_GPIO, euHightLevelTrig, tp_gt9xx_intr);
	EnableGPIOPadIRQ (INT_GPIO);
	XM_unlock ();
		
	XM_printf("Touch Panel Device Probe %s\n", (retval < TPD_OK) ? "FAIL" : "PASS");

	return retval;
}

static void gt9xx_ts_release(void)
{	
    if(tp_down==1)
    {
        XM_printf("gt9xx_ts_release\n");
	    tp_down = 0;
		XM_TouchEventProc (MAKELONG(ts_init->point_info[0].u16PosX, ts_init->point_info[0].u16PosY), TOUCH_TYPE_UP);
     }
}


static int gt9xx_report_value_A(void)
{	
	char buf[POINT_NUM * POINT_SIZE] = {0};
	int ret = -1;
	int i;
	uint pointx = 0,pointy = 0;
	static uint pointx_up = 0,pointy_up = 0;
	static uint touch_up_flag = 0;
	int keys[TPD_KEY_COUNT];
	uchar CoorStatus;
	uchar touchnum = 0;

	ret = gtp_i2c_read(GTP_READ_COOR_STATUS, &CoorStatus, 1);
	if(ret < 0)
	{
		XM_printf("%s read_data i2c_rxdata failed: %d\n", __func__, ret);
		GT9xx_write_coor_clear();
		return ret;
	}
			
	touchnum = CoorStatus & 0x0f;
	XM_printf(" touchnum = %d CoorStatus = %x\n",touchnum ,CoorStatus);
	XM_printf("CoorStatus = %d && tp_down =%d\n",CoorStatus,tp_down);
	 if(CoorStatus == 0x80 && tp_down == 1){
		 tp_down = 0;
		 touch_up_flag = 0;
		 timeout_to_check_tp_release = XM_GetTickCount() + TP_TIMEOUT;
		 XM_printf("point%d,pX=%d,pY=%d \r\n",i,pointx_up,pointy_up);
		 XM_TouchEventProc (MAKELONG(pointx_up, pointy_up), TOUCH_TYPE_UP);
		 XM_printf("GTP touch up !!!!!!!!!!!!!\n");
 	}
	 
	if(touchnum == 0)
	{
		//gt9xx_ts_release();
		GT9xx_write_coor_clear();
		return 1;
	}

	ret = gtp_i2c_read(GTP_READ_COOR_ADDR, buf, touchnum*POINT_SIZE);
	if(ret < 0)
	{
		GT9xx_write_coor_clear();
		return ret;
	}

	if(i == touchnum)
	{
		//gt9xx_ts_release();
		GT9xx_write_coor_clear();
		return 1;
	}
	for (i = 0; i < touchnum; i++)
	    {
	        pointx = buf[i*8+1] | (buf[i*8+2]<<8);
	        pointy = buf[i*8+3] | (buf[i*8+4]<<8);
		 if(CoorStatus != 0x80 && touch_up_flag == 0){
		 	 if(tp_down==0)
			      	{
				   tp_down = 1;
				   touch_up_flag = 1;
				   pointx_up = pointx;
				   pointy_up = pointy;
				 timeout_to_check_tp_release = XM_GetTickCount() + TP_TIMEOUT;
				 XM_TouchEventProc (MAKELONG(pointx, pointy), TOUCH_TYPE_DOWN);
				 XM_printf("GTP touch down !!!!!!!!!!!!!\n");
		 	 	}
		 	}
		 else{
		 	/*
		 	tp_down =0;
			 timeout_to_check_tp_release = XM_GetTickCount() + TP_TIMEOUT;
			 XM_TouchEventProc (MAKELONG(pointx, pointy), TOUCH_TYPE_UP);
			 XM_printf("GTP touch up2 !!!!!!!!!!!!!\n");
			 */
			 GT9xx_write_coor_clear();
		 	}
	    }
	GT9xx_write_coor_clear();
	return 0;
}

static void tp_gt9xxTask (void) 
{
	int probe_count = 3;
	unsigned int ticket_no_response;		// 无响应计数
	XM_printf ("gt9xx probe...\n");
	
tp_probe:
	while(tpd_probe () < 0)
	{
		if(ts_init)
		{
			kernel_free (ts_init);
			ts_init = NULL;
		}
		OS_Delay (100);
		probe_count --;
		if(probe_count == 0)
		{
		    XM_printf("tp_gt9xxTask Terminate\n");
			OS_Terminate (NULL);
		}
	}
	XM_printf ("GT9XX Probe OK\n");
	
	// 创建16hz的定时器
	OS_CREATETIMER (&TpTimer, TpTicketCallback, 64);
	
	ticket_no_response = 0;
	while(1)
	{
		OS_U8 tp_event = 0;
		
		tp_event = OS_WaitEvent (XMSYS_TP_EVENT
									|XMSYS_TP_EVENT_TICKET
								  );
		
		if(tp_event & (XMSYS_TP_EVENT) )
		{
			ticket_no_response = 0;		// 清除无响应计数
			if(ts_init->work_mode == 0)
			{
				gt9xx_report_value_A();
			}
		}
		if(tp_event & (XMSYS_TP_EVENT_TICKET) )//szynafeng2163.
		{
			if(tp_down)
			{
				if(XM_GetTickCount() >= timeout_to_check_tp_release)
				{
					// 超时
					if(gt9xx_report_value_A() != (-1))
					{
						ticket_no_response ++;
					}
				}
			}
			else
			{
				// 检查是否无应答
				if(gt9xx_i2c_check_id () < 0)
				{
					XM_printf ("ticket_no_response=%d\n", ticket_no_response);
					ticket_no_response ++;
				}
				else
				{
					ticket_no_response = 0;
				}
			}
		}
		
		if(ticket_no_response >= 32) // 16*32=512ms
		{
			// TP无响应超过512ms, 准备重新初始化
			XM_printf ("TP no response\n");
			break;
		}
	}
	
	OS_DeleteTimer (&TpTimer);
	XM_printf ("gt9xx re-probe...\n");
	
	if(ts_init)
	{
		kernel_free (ts_init);
		ts_init = NULL;
	}
	#ifdef TP_HW_I2C	
	if(tp_device)
	{
		xm_i2c_close (tp_device);
		tp_device = NULL;
	}
	#endif
	probe_count = 3;
	goto tp_probe;
}

void XMSYS_TpInit (void)
{
	// GPIO I2C
	GPIO_I2C_Init ();
	OS_CREATETASK(&TCB_gt9xxTask, "gt9xxTask", tp_gt9xxTask, XMSYS_gt9xx_TASK_PRIORITY, Stackgt9xxTask);
}


void XMSYS_TpExit (void)
{
	
}

int tp_icn85xx_reset (void)
{
	return 0;
}
#endif


