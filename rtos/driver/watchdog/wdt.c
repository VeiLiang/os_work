#include <hardware.h>
#include <common.h>
#include <watchdog.h>
#include <xm_base.h>
#include <printk.h>
#include <xm_dev.h>

/* Hardware timeout in seconds */
#define WDT_MAX_PRESCALER  ((1<<16))
#define WDT_MAX_COUT  ((1<<16))

/*WDTCR*/
#define WDTCR_CLKSEL		(1<<3)
#define WDTCR_INTEN		(1<<2)
#define WDTCR_RSTEN		(1<<1)
#define WDTCR_WDTEN		(1<<0)


#define WDTCR_DIV_16		0x00
#define WDTCR_DIV_32		0x01
#define WDTCR_DIV_64		0x02
#define WDTCR_DIV_128		0x03


t18xx_wdt_t * p_wdt = (t18xx_wdt_t *)WDT_BASE;

static int t18xx_wdt_settimeout(float timeout_seconds)
{
	unsigned int scaler;
	unsigned int timeout;

	timeout = (unsigned int)(timeout_seconds * 100);
	if(timeout >= WDT_MAX_COUT)
	{
		printk ("wdt timeout (%d) exceed maximum %d\n", timeout, WDT_MAX_COUT);
		timeout = 65536 - 1;
	}

	// 禁止wdt
	p_wdt->wdtcr = 0;

	//cacl the  scaler value, assure the clk is 100Hz after prescaler, the max timeout is about 655.36s
	scaler = arkn141_get_clks(ARKN141_CLK_APB) / (128  * 100);
	if(scaler > WDT_MAX_PRESCALER)
	{
		printk ("wdt scalar (%d) exceed maximum %d\n", scaler, WDT_MAX_PRESCALER);
		scaler = WDT_MAX_PRESCALER;
	}

	p_wdt->wdtpsr = scaler - 1;

	// 128分频
	p_wdt->wdtcr |= (WDTCR_DIV_128 << 4);
	// Prescaled and divided clock is used for down counting.
	p_wdt->wdtcr &= ~(1<< WDTCR_CLKSEL);

	p_wdt->wdtldr = timeout;

	return 0;
}

static void  watchdog_int_handler(void)
{
	// A write to this register clears the WDT interrupt status register, regardless of the value written.
	rWDT_ISR = 0;
	printk("watch dog interrupt, ticket=%d\n", XM_GetTickCount());
}

// 设置watchdog复位超时时间(秒单位)并启动watchdog
// timeout_seconds  0   --> 禁止watchdog
//                  > 0 --> 复位超时时间(秒单位)
void hw_watchdog_init (float timeout_seconds)
{
    return ;
	XM_lock ();
	if(timeout_seconds == 0.0)
	{
		p_wdt->wdtcr &= ~WDTCR_WDTEN;
	}
	else
	{
		t18xx_wdt_settimeout (timeout_seconds);

		// 设置复位Cycle计数，
		p_wdt->wdtrvr = 100;
		// enable wdt & reset
		p_wdt->wdtcr |=  WDTCR_RSTEN |WDTCR_WDTEN;
	}
	XM_unlock ();

}

// watchdog复位系统
void watchdog_reset (void)
{
	XM_lock ();
	hw_watchdog_init(0.01);		// 0.01秒后产生WatchDog异常
}


void wdt_test (void)
{
  	printf ("wdt_test\n");
	hw_watchdog_init(0.5);		// 1秒后产生WatchDog异常
	// 使能WatchDog，开启WatchDog中断，关闭WatchDog硬件复位
	p_wdt->wdtcr |=  WDTCR_INTEN |WDTCR_WDTEN;
	p_wdt->wdtcr &= ~WDTCR_RSTEN;

	request_irq (WDT_INT, PRIORITY_FIFTEEN, watchdog_int_handler);

	OS_Delay (10000);
	printf ("disable watchdog\n");
	hw_watchdog_init (0.0);

	OS_Delay (10000);
	printf ("10 seconds reset\n");
	hw_watchdog_init(10.0);		// 1秒后产生WatchDog异常
}



