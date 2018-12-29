//****************************************************************************
//
//	Copyright (C) 2012 ZhuoYongHong
//
//	Author	ZhuoYongHong
//
//	File name: app_time.c
//	  ?±??????????
//
//	Revision history
//
//		2012.09.19	ZhuoYongHong Initial version
//
//****************************************************************************
#include "app.h"


static const int white_datetime_char_offset[] = {
	ROM_T18_FONT_S18_WHITE_DATETIME_CHAR_01_PNG,
	ROM_T18_FONT_S18_WHITE_DATETIME_CHAR_02_PNG,
	ROM_T18_FONT_S18_WHITE_DATETIME_CHAR_03_PNG,
	ROM_T18_FONT_S18_WHITE_DATETIME_CHAR_04_PNG,
	ROM_T18_FONT_S18_WHITE_DATETIME_CHAR_05_PNG,
	ROM_T18_FONT_S18_WHITE_DATETIME_CHAR_06_PNG,
	ROM_T18_FONT_S18_WHITE_DATETIME_CHAR_07_PNG,
	ROM_T18_FONT_S18_WHITE_DATETIME_CHAR_08_PNG,
	ROM_T18_FONT_S18_WHITE_DATETIME_CHAR_09_PNG,
	ROM_T18_FONT_S18_WHITE_DATETIME_CHAR_10_PNG,
	ROM_T18_FONT_S18_WHITE_DATETIME_CHAR_DOT_PNG,
	ROM_T18_FONT_S18_WHITE_DATETIME_CHAR_B_PNG,
	ROM_T18_FONT_S18_WHITE_DATETIME_CHAR_G_PNG,
	ROM_T18_FONT_S18_WHITE_DATETIME_CHAR_COLON_PNG,
};

static const int white_datetime_char_length[] = {
	ROM_T18_FONT_S18_WHITE_DATETIME_CHAR_01_PNG_SIZE,
	ROM_T18_FONT_S18_WHITE_DATETIME_CHAR_02_PNG_SIZE,
	ROM_T18_FONT_S18_WHITE_DATETIME_CHAR_03_PNG_SIZE,
	ROM_T18_FONT_S18_WHITE_DATETIME_CHAR_04_PNG_SIZE,
	ROM_T18_FONT_S18_WHITE_DATETIME_CHAR_05_PNG_SIZE,
	ROM_T18_FONT_S18_WHITE_DATETIME_CHAR_06_PNG_SIZE,
	ROM_T18_FONT_S18_WHITE_DATETIME_CHAR_07_PNG_SIZE,
	ROM_T18_FONT_S18_WHITE_DATETIME_CHAR_08_PNG_SIZE,
	ROM_T18_FONT_S18_WHITE_DATETIME_CHAR_09_PNG_SIZE,
	ROM_T18_FONT_S18_WHITE_DATETIME_CHAR_10_PNG_SIZE,
	ROM_T18_FONT_S18_WHITE_DATETIME_CHAR_DOT_PNG_SIZE,
	ROM_T18_FONT_S18_WHITE_DATETIME_CHAR_B_PNG_SIZE,
	ROM_T18_FONT_S18_WHITE_DATETIME_CHAR_G_PNG_SIZE,
	ROM_T18_FONT_S18_WHITE_DATETIME_CHAR_COLON_PNG_SIZE,
};

// ?????¨????????????????×?????????????????X×?±ê?á??????(????????×?????????????X×?±ê????)
XMCOORD AP_TextOutWhiteDataTimeString (HANDLE hWnd, XMCOORD x, XMCOORD y, char *text, int size)
{
	char *ch;
	XMRECT rc;
	int index;
	HANDLE char_address;

	ch = text;
	while(size > 0)
	{
		if(*ch >= '0' && *ch <= '9')
		{
			index = *ch - '0';
		}
		else if(*ch == '.')
		{
			index = 10;
		}
		else if(*ch == 'B')
		{
			index = 11;
		}
		else if(*ch == 'G')
		{
			index = 12;
		}
		else if(*ch == ':')
		{
			index = 13;
		}
		else // (*ch == ' ')
		{
			// ??×???
			x += 4;
			ch ++;
			size --;
			continue;
		}

		rc.left = x;
		rc.right = x;
		rc.top = y;
		rc.bottom = y;
		XM_RomImageDraw (white_datetime_char_offset[index], white_datetime_char_length[index], hWnd, &rc, XMGIF_DRAW_POS_LEFTTOP);

		x += DATETIME_CHAR_WIDTH;

		ch ++;
		size --;
	}

	return x;
}

static const int datetime_char_offset[] = {
	ROM_T18_FONT_S18_DATETIME_CHAR_01_PNG,
	ROM_T18_FONT_S18_DATETIME_CHAR_02_PNG,
	ROM_T18_FONT_S18_DATETIME_CHAR_03_PNG,
	ROM_T18_FONT_S18_DATETIME_CHAR_04_PNG,
	ROM_T18_FONT_S18_DATETIME_CHAR_05_PNG,
	ROM_T18_FONT_S18_DATETIME_CHAR_06_PNG,
	ROM_T18_FONT_S18_DATETIME_CHAR_07_PNG,
	ROM_T18_FONT_S18_DATETIME_CHAR_08_PNG,
	ROM_T18_FONT_S18_DATETIME_CHAR_09_PNG,
	ROM_T18_FONT_S18_DATETIME_CHAR_10_PNG,
	ROM_T18_FONT_S18_DATETIME_CHAR_11_PNG,
	ROM_T18_FONT_S18_DATETIME_CHAR_12_PNG,
	ROM_T18_FONT_S18_DATETIME_CHAR_13_PNG,
	ROM_T18_FONT_S18_DATETIME_CHAR_14_PNG,
	ROM_T18_FONT_S18_DATETIME_CHAR_15_PNG,
	ROM_T18_FONT_S18_DATETIME_CHAR_16_PNG,
	ROM_T18_FONT_S18_DATETIME_CHAR_17_PNG,
	ROM_T18_FONT_S18_DATETIME_CHAR_18_PNG,
	ROM_T18_FONT_S18_DATETIME_CHAR_19_PNG,
	ROM_T18_FONT_S18_DATETIME_CHAR_20_PNG,
	ROM_T18_FONT_S18_DATETIME_CHAR_21_PNG,
	ROM_T18_FONT_S18_DATETIME_CHAR_22_PNG,
	ROM_T18_FONT_S18_DATETIME_CHAR_23_PNG,
	ROM_T18_FONT_S18_DATETIME_CHAR_24_PNG,
	ROM_T18_FONT_S18_DATETIME_CHAR_25_PNG,
	ROM_T18_FONT_S18_DATETIME_CHAR_26_PNG,
	ROM_T18_FONT_S18_DATETIME_CHAR_27_PNG,
	ROM_T18_FONT_S18_DATETIME_CHAR_28_PNG,
	ROM_T18_FONT_S18_DATETIME_CHAR_29_PNG,
	ROM_T18_FONT_S18_DATETIME_CHAR_30_PNG,
	ROM_T18_FONT_S18_DATETIME_CHAR_31_PNG,
	ROM_T18_FONT_S18_DATETIME_CHAR_32_PNG,
	ROM_T18_FONT_S18_DATETIME_CHAR_33_PNG,
	ROM_T18_FONT_S18_DATETIME_CHAR_34_PNG,
	ROM_T18_FONT_S18_DATETIME_CHAR_35_PNG,
	ROM_T18_FONT_S18_DATETIME_CHAR_36_PNG,
	ROM_T18_FONT_S18_DATETIME_CHAR_37_PNG,
	ROM_T18_FONT_S18_DATETIME_CHAR_38_PNG,
	ROM_T18_FONT_S18_DATETIME_CHAR_39_PNG,
};
static const int datetime_char_length[] = {
	ROM_T18_FONT_S18_DATETIME_CHAR_01_PNG_SIZE,
	ROM_T18_FONT_S18_DATETIME_CHAR_02_PNG_SIZE,
	ROM_T18_FONT_S18_DATETIME_CHAR_03_PNG_SIZE,
	ROM_T18_FONT_S18_DATETIME_CHAR_04_PNG_SIZE,
	ROM_T18_FONT_S18_DATETIME_CHAR_05_PNG_SIZE,
	ROM_T18_FONT_S18_DATETIME_CHAR_06_PNG_SIZE,
	ROM_T18_FONT_S18_DATETIME_CHAR_07_PNG_SIZE,
	ROM_T18_FONT_S18_DATETIME_CHAR_08_PNG_SIZE,
	ROM_T18_FONT_S18_DATETIME_CHAR_09_PNG_SIZE,
	ROM_T18_FONT_S18_DATETIME_CHAR_10_PNG_SIZE,
	ROM_T18_FONT_S18_DATETIME_CHAR_11_PNG_SIZE,
	ROM_T18_FONT_S18_DATETIME_CHAR_12_PNG_SIZE,
	ROM_T18_FONT_S18_DATETIME_CHAR_13_PNG_SIZE,
	ROM_T18_FONT_S18_DATETIME_CHAR_14_PNG_SIZE,
	ROM_T18_FONT_S18_DATETIME_CHAR_15_PNG_SIZE,
	ROM_T18_FONT_S18_DATETIME_CHAR_16_PNG_SIZE,
	ROM_T18_FONT_S18_DATETIME_CHAR_17_PNG_SIZE,
	ROM_T18_FONT_S18_DATETIME_CHAR_18_PNG_SIZE,
	ROM_T18_FONT_S18_DATETIME_CHAR_19_PNG_SIZE,
	ROM_T18_FONT_S18_DATETIME_CHAR_20_PNG_SIZE,
	ROM_T18_FONT_S18_DATETIME_CHAR_21_PNG_SIZE,
	ROM_T18_FONT_S18_DATETIME_CHAR_22_PNG_SIZE,
	ROM_T18_FONT_S18_DATETIME_CHAR_23_PNG_SIZE,
	ROM_T18_FONT_S18_DATETIME_CHAR_24_PNG_SIZE,
	ROM_T18_FONT_S18_DATETIME_CHAR_25_PNG_SIZE,
	ROM_T18_FONT_S18_DATETIME_CHAR_26_PNG_SIZE,
	ROM_T18_FONT_S18_DATETIME_CHAR_27_PNG_SIZE,
	ROM_T18_FONT_S18_DATETIME_CHAR_28_PNG_SIZE,
	ROM_T18_FONT_S18_DATETIME_CHAR_29_PNG_SIZE,
	ROM_T18_FONT_S18_DATETIME_CHAR_30_PNG_SIZE,
	ROM_T18_FONT_S18_DATETIME_CHAR_31_PNG_SIZE,
	ROM_T18_FONT_S18_DATETIME_CHAR_32_PNG_SIZE,
	ROM_T18_FONT_S18_DATETIME_CHAR_33_PNG_SIZE,
	ROM_T18_FONT_S18_DATETIME_CHAR_34_PNG_SIZE,
	ROM_T18_FONT_S18_DATETIME_CHAR_35_PNG_SIZE,
	ROM_T18_FONT_S18_DATETIME_CHAR_36_PNG_SIZE,
	ROM_T18_FONT_S18_DATETIME_CHAR_37_PNG_SIZE,
	ROM_T18_FONT_S18_DATETIME_CHAR_38_PNG_SIZE,
	ROM_T18_FONT_S18_DATETIME_CHAR_39_PNG_SIZE,
};


static const int dateset_num_a_offset[] = {
	ROM_T18_COMMON_MENU_DATE_MENU_DATE_NUM_DATE_NUM_0_N_PNG,
	ROM_T18_COMMON_MENU_DATE_MENU_DATE_NUM_DATE_NUM_1_N_PNG,
	ROM_T18_COMMON_MENU_DATE_MENU_DATE_NUM_DATE_NUM_2_N_PNG,
	ROM_T18_COMMON_MENU_DATE_MENU_DATE_NUM_DATE_NUM_3_N_PNG,
	ROM_T18_COMMON_MENU_DATE_MENU_DATE_NUM_DATE_NUM_4_N_PNG,
	ROM_T18_COMMON_MENU_DATE_MENU_DATE_NUM_DATE_NUM_5_N_PNG,
	ROM_T18_COMMON_MENU_DATE_MENU_DATE_NUM_DATE_NUM_6_N_PNG,
	ROM_T18_COMMON_MENU_DATE_MENU_DATE_NUM_DATE_NUM_7_N_PNG,
	ROM_T18_COMMON_MENU_DATE_MENU_DATE_NUM_DATE_NUM_8_N_PNG,
	ROM_T18_COMMON_MENU_DATE_MENU_DATE_NUM_DATE_NUM_9_N_PNG,
};

static const int dateset_num_a_len[] = {
	ROM_T18_COMMON_MENU_DATE_MENU_DATE_NUM_DATE_NUM_0_N_PNG_SIZE,
	ROM_T18_COMMON_MENU_DATE_MENU_DATE_NUM_DATE_NUM_1_N_PNG_SIZE,
	ROM_T18_COMMON_MENU_DATE_MENU_DATE_NUM_DATE_NUM_2_N_PNG_SIZE,
	ROM_T18_COMMON_MENU_DATE_MENU_DATE_NUM_DATE_NUM_3_N_PNG_SIZE,
	ROM_T18_COMMON_MENU_DATE_MENU_DATE_NUM_DATE_NUM_4_N_PNG_SIZE,
	ROM_T18_COMMON_MENU_DATE_MENU_DATE_NUM_DATE_NUM_5_N_PNG_SIZE,
	ROM_T18_COMMON_MENU_DATE_MENU_DATE_NUM_DATE_NUM_6_N_PNG_SIZE,
	ROM_T18_COMMON_MENU_DATE_MENU_DATE_NUM_DATE_NUM_7_N_PNG_SIZE,
	ROM_T18_COMMON_MENU_DATE_MENU_DATE_NUM_DATE_NUM_8_N_PNG_SIZE,
	ROM_T18_COMMON_MENU_DATE_MENU_DATE_NUM_DATE_NUM_9_N_PNG_SIZE,
};
static const int dateset_num_n_offset[] = {
	ROM_T18_COMMON_MENU_DATE_MENU_DATE_NUM_DATE_NUM_0_A_PNG,
	ROM_T18_COMMON_MENU_DATE_MENU_DATE_NUM_DATE_NUM_1_A_PNG,
	ROM_T18_COMMON_MENU_DATE_MENU_DATE_NUM_DATE_NUM_2_A_PNG,
	ROM_T18_COMMON_MENU_DATE_MENU_DATE_NUM_DATE_NUM_3_A_PNG,
	ROM_T18_COMMON_MENU_DATE_MENU_DATE_NUM_DATE_NUM_4_A_PNG,
	ROM_T18_COMMON_MENU_DATE_MENU_DATE_NUM_DATE_NUM_5_A_PNG,
	ROM_T18_COMMON_MENU_DATE_MENU_DATE_NUM_DATE_NUM_6_A_PNG,
	ROM_T18_COMMON_MENU_DATE_MENU_DATE_NUM_DATE_NUM_7_A_PNG,
	ROM_T18_COMMON_MENU_DATE_MENU_DATE_NUM_DATE_NUM_8_A_PNG,
	ROM_T18_COMMON_MENU_DATE_MENU_DATE_NUM_DATE_NUM_9_A_PNG,
};

static const int dateset_num_n_len[] = {
	ROM_T18_COMMON_MENU_DATE_MENU_DATE_NUM_DATE_NUM_0_A_PNG_SIZE,
	ROM_T18_COMMON_MENU_DATE_MENU_DATE_NUM_DATE_NUM_1_A_PNG_SIZE,
	ROM_T18_COMMON_MENU_DATE_MENU_DATE_NUM_DATE_NUM_2_A_PNG_SIZE,
	ROM_T18_COMMON_MENU_DATE_MENU_DATE_NUM_DATE_NUM_3_A_PNG_SIZE,
	ROM_T18_COMMON_MENU_DATE_MENU_DATE_NUM_DATE_NUM_4_A_PNG_SIZE,
	ROM_T18_COMMON_MENU_DATE_MENU_DATE_NUM_DATE_NUM_5_A_PNG_SIZE,
	ROM_T18_COMMON_MENU_DATE_MENU_DATE_NUM_DATE_NUM_6_A_PNG_SIZE,
	ROM_T18_COMMON_MENU_DATE_MENU_DATE_NUM_DATE_NUM_7_A_PNG_SIZE,
	ROM_T18_COMMON_MENU_DATE_MENU_DATE_NUM_DATE_NUM_8_A_PNG_SIZE,
	ROM_T18_COMMON_MENU_DATE_MENU_DATE_NUM_DATE_NUM_9_A_PNG_SIZE,
};

static const int dateset_num_color_view_offset[] = {
#if 0
	ROM_T18_COMMON_MENU_DATE_MENU_DATE_NUM_0T_PNG,
	ROM_T18_COMMON_MENU_DATE_MENU_DATE_NUM_1T_PNG,
	ROM_T18_COMMON_MENU_DATE_MENU_DATE_NUM_2T_PNG,
	ROM_T18_COMMON_MENU_DATE_MENU_DATE_NUM_3T_PNG,
	ROM_T18_COMMON_MENU_DATE_MENU_DATE_NUM_4T_PNG,
	ROM_T18_COMMON_MENU_DATE_MENU_DATE_NUM_5T_PNG,
	ROM_T18_COMMON_MENU_DATE_MENU_DATE_NUM_6T_PNG,
	ROM_T18_COMMON_MENU_DATE_MENU_DATE_NUM_7T_PNG,
	ROM_T18_COMMON_MENU_DATE_MENU_DATE_NUM_8T_PNG,
	ROM_T18_COMMON_MENU_DATE_MENU_DATE_NUM_9T_PNG,
#endif
	ROM_T18_FONT_S18_DATETIME_01_PNG,
	ROM_T18_FONT_S18_DATETIME_02_PNG,
	ROM_T18_FONT_S18_DATETIME_03_PNG,
	ROM_T18_FONT_S18_DATETIME_04_PNG,
	ROM_T18_FONT_S18_DATETIME_05_PNG,
	ROM_T18_FONT_S18_DATETIME_06_PNG,
	ROM_T18_FONT_S18_DATETIME_07_PNG,
	ROM_T18_FONT_S18_DATETIME_08_PNG,
	ROM_T18_FONT_S18_DATETIME_09_PNG,
	ROM_T18_FONT_S18_DATETIME_10_PNG,
};

static const int dateset_num_color_view_len[] = {

	ROM_T18_FONT_S18_DATETIME_01_PNG_SIZE,
	ROM_T18_FONT_S18_DATETIME_02_PNG_SIZE,
	ROM_T18_FONT_S18_DATETIME_03_PNG_SIZE,
	ROM_T18_FONT_S18_DATETIME_04_PNG_SIZE,
	ROM_T18_FONT_S18_DATETIME_05_PNG_SIZE,
	ROM_T18_FONT_S18_DATETIME_06_PNG_SIZE,
	ROM_T18_FONT_S18_DATETIME_07_PNG_SIZE,
	ROM_T18_FONT_S18_DATETIME_08_PNG_SIZE,
	ROM_T18_FONT_S18_DATETIME_09_PNG_SIZE,
	ROM_T18_FONT_S18_DATETIME_10_PNG_SIZE,
	#if 0
	ROM_T18_COMMON_MENU_DATE_MENU_DATE_NUM_0T_PNG_SIZE,
	ROM_T18_COMMON_MENU_DATE_MENU_DATE_NUM_1T_PNG_SIZE,
	ROM_T18_COMMON_MENU_DATE_MENU_DATE_NUM_2T_PNG_SIZE,
	ROM_T18_COMMON_MENU_DATE_MENU_DATE_NUM_3T_PNG_SIZE,
	ROM_T18_COMMON_MENU_DATE_MENU_DATE_NUM_4T_PNG_SIZE,
	ROM_T18_COMMON_MENU_DATE_MENU_DATE_NUM_5T_PNG_SIZE,
	ROM_T18_COMMON_MENU_DATE_MENU_DATE_NUM_6T_PNG_SIZE,
	ROM_T18_COMMON_MENU_DATE_MENU_DATE_NUM_7T_PNG_SIZE,
	ROM_T18_COMMON_MENU_DATE_MENU_DATE_NUM_8T_PNG_SIZE,
	ROM_T18_COMMON_MENU_DATE_MENU_DATE_NUM_9T_PNG_SIZE,
	#endif
};
static XM_IMAGE *datetime_char_image[39];
// 20180726 修改显示分辨率切换时datetime_char_image资源未能释放的bug
// 初始化分配的日期时间串显示资源
// 持续按键,分配不了这个空间datetime_char_image全部清0
static void AP_DateTimeCharImageInit (void)
{
	memset (datetime_char_image, 0, sizeof(datetime_char_image));
}

// 释放分配的日期时间串显示资源
static void AP_DateTimeCharImageExit (void)
{
	for (int i = 0; i < sizeof(datetime_char_image)/sizeof(datetime_char_image[0]); i ++)
	{
		if(datetime_char_image[i])
		{
			XM_ImageDelete (datetime_char_image[i]);
			datetime_char_image[i] = NULL;
		}
	}
}

XMBOOL AP_TextGetStringSize (char *text, int size, XMSIZE *pSize)
{
	//char *ch;
	//int index;
	if(text == NULL || size == 0 || pSize == NULL)
		return 0;
	
	pSize->cx = size * DATETIME_CHAR_WIDTH;
	pSize->cy = 16;
	
	return 1;
}

XMCOORD AP_TextOutNumString(HANDLE hWnd, XMCOORD x, XMCOORD y, u8_t num_width,char *text, int size,u8_t type)
{
    char *ch;
	XMRECT rc;
	int index;
	HANDLE char_address;
	AP_DateTimeCharImageInit ();

    int *num_offset;
    int *num_len;

    switch(type)
    {
        case 0:
            num_offset = (int *)dateset_num_n_offset;
            num_len = (int *)dateset_num_n_len;
            break;
        case 1:
            num_offset = (int *)dateset_num_a_offset;
            num_len = (int *)dateset_num_a_len;
            break;
        case 2:
            num_offset = (int *)datetime_char_offset;
            num_len = (int *)datetime_char_length;
            break;
        case 3:
            num_offset = (int *)dateset_num_color_view_offset;
            num_len = (int *)dateset_num_color_view_len;
        default:
            break;
    }
    
	ch = text;
	while(size > 0)
	{
		if(*ch >= '0' && *ch <= '9')
		{
			index = *ch - '0';
		}
		else // (*ch == ' ')
		{
			x += num_width;
			ch ++;
			size --;
			continue;
		}
		
		rc.left = x;
		rc.right = x;
		rc.top = y;
		rc.bottom = y;
		if(datetime_char_image[index] == NULL)
		{
			datetime_char_image[index] = XM_RomImageCreate (num_offset[index], num_len[index], XM_OSD_LAYER_FORMAT_ARGB888);
		}
		if(datetime_char_image[index])
		{
			XM_ImageDisplay (datetime_char_image[index], hWnd, rc.left, rc.top);
		}
		x += num_width;

		ch ++;
		size --;
	}
	
	AP_DateTimeCharImageExit ();
	return x;
}

XMCOORD AP_TextOutDataTimeString (HANDLE hWnd, XMCOORD x, XMCOORD y, char *text, int size)
{
	char *ch;
	XMRECT rc;
	int index;
	HANDLE char_address;
	AP_DateTimeCharImageInit ();

	ch = text;
	while(size > 0)
	{
		if(*ch >= '0' && *ch <= '9')
		{
			index = *ch - '0';
		}
		else if(*ch >= 'A' && *ch <= 'Z')
		{
			index = 10 + *ch - 'A';
		}
		else if(*ch >= 'a' && *ch <= 'z')
		{
			index = 10 + *ch - 'a';
		}
		else if(*ch == ':')
		{
			index = 36;
		}
		else if(*ch == '.')
		{
			index = 37;
		}
		else if(*ch == '/')
		{
			index = 38;
		}
		else // (*ch == ' ')
		{
			x += DATETIME_CHAR_WIDTH;
			ch ++;
			size --;
			continue;
		}
		
		rc.left = x;
		rc.right = x;
		rc.top = y;
		rc.bottom = y;
		if(datetime_char_image[index] == NULL)
		{
			datetime_char_image[index] = XM_RomImageCreate (datetime_char_offset[index], datetime_char_length[index], XM_OSD_LAYER_FORMAT_ARGB888);
		}
		if(datetime_char_image[index])
		{
			XM_ImageDisplay (datetime_char_image[index], hWnd, rc.left, rc.top);
		}
		x += DATETIME_CHAR_WIDTH;

		ch ++;
		size --;
	}
	
	AP_DateTimeCharImageExit ();
	return x;
}

// ???????????±??×?????
int AP_FormatDataTime (XMSYSTEMTIME *lpSystemTime, DWORD dwType, char *lpTextBuffer, int cbTextBuffer)
{
	char string[32];
	int size;
	memset (string, 0, sizeof(string));
	// ??????
	if(dwType == APP_DATETIME_FORMAT_1)
	{
		// ????1?? ?????±???°???? 2012/09/16 10:22
		sprintf (string, "%04d/%02d/%02d %02d:%02d", lpSystemTime->wYear, lpSystemTime->bMonth, lpSystemTime->bDay,
																	lpSystemTime->bHour, lpSystemTime->bMinute);
	}
	else if(dwType == APP_DATETIME_FORMAT_2)
	{
		// ????2?? ???????? 2012/09/16
		sprintf (string, "%04d / %02d / %02d", lpSystemTime->wYear, lpSystemTime->bMonth, lpSystemTime->bDay);
	}
	else if(dwType == APP_DATETIME_FORMAT_3)
	{
		// ????3?? ?????±?? 10:22
		sprintf (string, "%02d:%02d", lpSystemTime->bHour, lpSystemTime->bMinute);
	}
	else if(dwType == APP_DATETIME_FORMAT_4)
	{
		// ????4?? ?????±???? 10:22:20
		sprintf (string, "%02d : %02d : %02d", lpSystemTime->bHour, lpSystemTime->bMinute, lpSystemTime->bSecond);
	}
	else if(dwType == APP_DATETIME_FORMAT_5)
	{
		// ????5?? ???????? 09/16 10:22
		sprintf (string, "%02d/%02d %02d:%02d", lpSystemTime->bMonth, lpSystemTime->bDay,
																	lpSystemTime->bHour, lpSystemTime->bMinute);
	}
	else if(dwType == APP_DATETIME_FORMAT_6)
	{
		// ????5?? ???????? 09/16
		sprintf (string, "%02d/%02d", lpSystemTime->bMonth, lpSystemTime->bDay);
	}
	else if(dwType == APP_DATETIME_FORMAT_7)
	{
		// ????2?? ???????? 2012/09/16
		sprintf (string, "%04d/%02d/%02d", lpSystemTime->wYear, lpSystemTime->bMonth, lpSystemTime->bDay);
	}
	else
	{
		// ???§????
		return 0;
	}
	size = strlen(string);
	if(cbTextBuffer <= size)
		return 0;
	strcpy (lpTextBuffer, string);
	return size;
}

// ?????????±??
void AP_TextOutDateTime (HANDLE hWnd, XMCOORD x, XMCOORD y, XMSYSTEMTIME *lpSystemTime, DWORD dwType)
{
	char string[32];

	memset (string, 0, sizeof(string));

	AP_FormatDataTime (lpSystemTime, dwType, string, 32);
	AP_TextOutDataTimeString (hWnd, x, y, string, strlen(string));
	
}

VOID AP_VideoFileNameToDisplayName (const char *lpVideoFileName, char *lpDisplayName)
{
	const char *name = lpVideoFileName;
	char *text = lpDisplayName;

	memcpy (text, name, 4);
	text += 4;
	name += 4;
	*text ++ = '/';
	memcpy (text, name, 2);	
	text += 2;
	name += 2;
	*text ++ = '/';	
	memcpy (text, name, 2);
	text += 2;
	name += 2;
	*text ++ = ' ';
	*text ++ = ' ';
	memcpy (text, name, 2);
	text += 2;
	name += 2;
	*text ++ = ':';
	memcpy (text, name, 2);
	text += 2;
	name += 2;
	*text ++ = ':';
	memcpy (text, name, 2);
	text += 2;
	*text = 0;
}