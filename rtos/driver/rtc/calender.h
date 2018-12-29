#ifndef _CALENDER_H_
#define _CALENDER_H_

#ifdef	__cplusplus
extern "C" {
#endif

long					mdy_to_julian	(unsigned char month, unsigned char day, int year);
void					julian_to_mdy	(long julian, unsigned char *day, unsigned char *month, int *year);
unsigned char		julian_to_wday (long julian);
int					is_leap_year	( int year );
void					time_to_mdy		(time_t days, unsigned char *day, unsigned char *month, int *year);

#ifdef	__cplusplus
}
#endif


#endif