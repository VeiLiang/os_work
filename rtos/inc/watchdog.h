#ifndef _WATCHDOG_H_
#define _WATCHDOG_H_

void hw_watchdog_init (float timeout_seconds);

// watchdog¸´Î»ÏµÍ³
void watchdog_reset (void);

#endif /* _WATCHDOG_H_ */
