#ifndef __PR1000_PTZ_TABLE_H__
#define __PR1000_PTZ_TABLE_H__

/************************************************************/
enum _pr1000_ptz_table_command {
	pr1000_ptz_table_command_OSD_TOP = 0,
	pr1000_ptz_table_command_OSD_RIGHT,
	pr1000_ptz_table_command_OSD_BOTTOM,
	pr1000_ptz_table_command_OSD_LEFT,
	pr1000_ptz_table_command_OSD_CENTER,
	pr1000_ptz_table_command_IRIS_PLUS,
	pr1000_ptz_table_command_IRIS_MINUS,
	pr1000_ptz_table_command_FOCUS_PLUS,
	pr1000_ptz_table_command_FOCUS_MINUS,
	pr1000_ptz_table_command_ZOOM_PLUS,
	pr1000_ptz_table_command_ZOOM_MINUS,
	pr1000_ptz_table_command_DIR_TOP,
	pr1000_ptz_table_command_DIR_RIGHT,
	pr1000_ptz_table_command_DIR_BOTTOM,
	pr1000_ptz_table_command_DIR_LEFT,
	pr1000_ptz_table_command_DIR_RIGHT_TOP,
	pr1000_ptz_table_command_DIR_RIGHT_BOTTOM,
	pr1000_ptz_table_command_DIR_LEFT_TOP,
	pr1000_ptz_table_command_DIR_LEFT_BOTTOM,
	pr1000_ptz_table_command_LENS_CLOSE,
	pr1000_ptz_table_command_DIR_CLOSE,
	pr1000_ptz_table_command_RESERVED0,
	max_pr1000_ptz_table_command
};

extern const char _STR_PR1000_PTZ_COMMAND[max_pr1000_ptz_table_command][40];
///////////////////////////////////////////////////////////////////////////////////////////////////////
typedef struct 
{
	const uint8_t *pCmd;
	uint8_t length;
}_pr1000PTZCmd;
//////////////////////////////////////////////////////////////////////////////////////////

#endif /* __PR1000_PTZ_TABLE_H__ */
