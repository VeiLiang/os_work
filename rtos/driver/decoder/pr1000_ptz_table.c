#ifdef __LINUX_SYSTEM__
/* __KERNEL__ */
#ifdef __KERNEL__
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/fcntl.h>
#include <linux/mm.h>
#include <linux/miscdevice.h>
#include <linux/proc_fs.h>
#else
#include <stdint.h>
#endif /* __KERNEL__ */
#include "pr1000_ptz_table.h"

#else //#ifdef __LINUX_SYSTEM__

#include <stdint.h>

#include "pr1000_ptz_table.h"

#endif // __LINUX_SYSTEM__

//////////////////////////////////////////////////////////////////////////////////////// 
#ifndef DONT_SUPPORT_HELP_STRING
const char _STR_PR1000_PTZ_COMMAND[max_pr1000_ptz_table_command][40] = {
	"PR1000_PTZ_OSD_TOP",			//0
	"PR1000_PTZ_OSD_RIGHT",		//1
	"PR1000_PTZ_OSD_BOTTOM",		//2
	"PR1000_PTZ_OSD_LEFT",			//3
	"PR1000_PTZ_OSD_CENTER",		//4
	"PR1000_PTZ_IRIS_PLUS",		//5
	"PR1000_PTZ_IRIS_MINUS",		//6
	"PR1000_PTZ_FOCUS_PLUS",		//7
	"PR1000_PTZ_FOCUS_MINUS",		//8
	"PR1000_PTZ_ZOOM_PLUS",		//9
	"PR1000_PTZ_ZOOM_MINUS",		//10
	"PR1000_PTZ_DIR_TOP",			//11
	"PR1000_PTZ_DIR_RIGHT",		//12
	"PR1000_PTZ_DIR_BOTTOM",		//13
	"PR1000_PTZ_DIR_LEFT",			//14
	"PR1000_PTZ_DIR_RIGHT_TOP",		//15
	"PR1000_PTZ_DIR_RIGHT_BOTTOM",		//16
	"PR1000_PTZ_DIR_LEFT_TOP",		//17
	"PR1000_PTZ_DIR_LEFT_BOTTOM",		//18
	"PR1000_PTZ_LENS_CLOSE",		//19
	"PR1000_PTZ_DIR_CLOSE",		//20
	"PR1000_PTZ_RESERVED0",		//21
};
#endif // DONT_SUPPORT_HELP_STRING
/////////////////////////////////////////////////////////////////////////////////////////////
