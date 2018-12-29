
#include "hardware.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "xm_file.h"
#include "xm_printf.h"
#include "rtos.h"
#include "arkn141_codec.h"
#include "ark1960_testcase.h"

void SetJpegQLevel(char level );
int jpeg_encode_main(int argc, char *argv[]);
int jpeg_decode_main(int argc, char *argv[]);


//        JPEGENC_YUV420_PLANAR = 0,  /* YYYY... UUUU... VVVV */
//        JPEGENC_YUV420_SEMIPLANAR = 1,  /* YYYY... UVUVUV...    */
//        JPEGENC_YUV422_INTERLEAVED_YUYV = 2,    /* YUYVYUYV...          */

enum {
	ARKN141_YUV420 = 0,
	ARKN141_Y_UV420,
	ARKN141_YUYV
};



// < 0 失败
// >= 0 JPEG解码消耗的时间
int arkn141_jpg2yuv_process_file (const char *src_jpg_file, const char *dst_yuv_file, 
								  const char *bmp_file,
								  int w, int h)
{
	char *argv[32];
	int argc = 0;
	char string[512];
	char *cmd = string;
	memset (string, 0, sizeof(string));
	memset (argv, 0, sizeof(argv));
	
	sprintf (cmd, "arkn141_jpg2yuv");
	argv[argc] = cmd;
	argc ++;
	cmd += strlen(cmd) + 1;
	
	sprintf (cmd, "-i");
	argv[argc] = cmd;
	argc ++;
	cmd += strlen(cmd) + 1;
	
	sprintf (cmd, "%s", src_jpg_file);
	argv[argc] = cmd;
	argc ++;
	cmd += strlen(cmd) + 1;
	
	sprintf (cmd, "-o");
	argv[argc] = cmd;
	argc ++;
	cmd += strlen(cmd) + 1;
	
	sprintf (cmd, "%s", dst_yuv_file);
	argv[argc] = cmd;
	argc ++;
	cmd += strlen(cmd) + 1;
	
	arkn141_codec_open (ARKN141_CODEC_TYPE_JPEG_DECODER);
	jpeg_decode_main(argc, argv );	
	arkn141_codec_close (ARKN141_CODEC_TYPE_JPEG_DECODER);
	
	return 0;
}

int arkn141_jpg2yuv_process_directory (char *src_dir_path, char *dst_dir_path, int w, int h)
{
	char fileName[XM_MAX_FILEFIND_NAME];
	char src_file[64];
	char dst_file[64];
	char bmp_file[64];
	XMFILEFIND fileFind;
	int ret = -1;
	unsigned int max_codec_time = 0; 
	unsigned int min_codec_time = 0x7FFFFFFF;
	unsigned int avg_codec_time = 0;
	int codec_file_count = 0;

	check_and_create_directory(dst_dir_path);
	do
	{		
		printf ("process directory (%s)\n", src_dir_path);
		if(XM_FindFirstFile (src_dir_path, &fileFind, fileName, XM_MAX_FILEFIND_NAME))
		{
			do
			{
				// 检查目录项属性
				DWORD dwFileAttributes = fileFind.dwFileAttributes;
				// 检查该目录项是否是文件属性
				if((dwFileAttributes & (ATTR_DIRECTORY | ATTR_VOLUME_ID)) == 0x00)
				{
					// 文件属性
					char *ext;
					int jpg2yuv_ret = -1;
					sprintf (src_file, "%s%s", src_dir_path, fileName);
					ext = strchr (fileName, '.');
					if(ext)
						*ext = 0;
					sprintf (dst_file, "%s%s.yuv", dst_dir_path, fileName);
					sprintf (bmp_file, "%s%s.bmp", dst_dir_path, fileName);
					
					jpg2yuv_ret = arkn141_jpg2yuv_process_file (src_file, dst_file, bmp_file, w, h);
					if(jpg2yuv_ret < 0)
						XM_printf ("--> jpg2yuv NG, (%s)\n", src_file);
					else
					{
						XM_printf ("--> jpg2yuv OK, (%s), decode time = %d ms\n", src_file, jpg2yuv_ret);
						if(jpg2yuv_ret < min_codec_time)
							min_codec_time = jpg2yuv_ret;
						if(jpg2yuv_ret > max_codec_time)
							max_codec_time = jpg2yuv_ret;
						codec_file_count ++;
						avg_codec_time += jpg2yuv_ret;
					}
				}

			} while (XM_FindNextFile (&fileFind));
			XM_FindClose (&fileFind);
			ret = 0;
		}
		else
		{
			printf("Not find %s", src_dir_path );
		}
		
	} while(0);
	
	// 解码时间报告
	if(codec_file_count)
	{
		printf ("\n");
		printf ("     JPEG decode time report, w = %4d, h = %4d\n", w, h);
		printf ("     total decode file(OK) count %d\n", codec_file_count);
		printf ("     minimum decode time = %d ms\n",  min_codec_time);
		printf ("     maximum decode time = %d ms\n",  max_codec_time);
		printf ("     average decode time = %d ms\n",  (avg_codec_time + codec_file_count - 1) / codec_file_count);
		printf ("     FPGA ( 48MHz) fps = %f\n", (((double)1000.0) * codec_file_count) / avg_codec_time);
		printf ("     ASIC (200MHz) fps = %f\n", (((double)1000.0 * 200.0/48.0) * codec_file_count) / avg_codec_time);
		printf ("\n\n");
	}
	
	return ret;
}

// 扫描 \\JPG2YUV\\JPG\\目录下的子目录，测试JPG解码
// 子目录使用wh命名，w,h均使用4位10进制命名
//	如1920x1280, 目录名为19201280
// 如 640x 480, 目录名为06400480
// 宽高必须为16的倍数
// repeat_test_count 重复测试的次数
int arkn141_jpg2yuv_test (char *volume_name, unsigned int repeat_test_count)
{

	// 检查\\RGB2YUV\\RGB\\目录是否存在
	XMFILEFIND fileFind;
	char fileName[XM_MAX_FILEFIND_NAME];
	char jpg_path[64];
	char yuv_path[64];
	char jpg_testcase_path[64];
	char yuv_testcase_path[64];
	unsigned int index;
	

	printf ("arkn141_jpg2yuv_test begin ...\n");
	sprintf (jpg_path, "%s:\\MINITEST\\RAW\\JPEG\\", volume_name);
	
	if(repeat_test_count == 0)
		repeat_test_count = 1;
	if(repeat_test_count > 9)
		repeat_test_count = 9;
	
	for (index = 0; index < repeat_test_count; index ++)
	{
	
		sprintf (yuv_path, "%s:\\MINITEST\\ON2DEC\\JPEG\\YUV%d", volume_name, index);
		// 检查目录是否存在
		
		
		printf ("loop = %d, check path(%s)\n", index, yuv_path);
		if(check_and_create_directory (yuv_path))
		{
			XM_printf ("can't create YUV path (%s)\n", yuv_path);
			return -1;
		}
		strcat (yuv_path, "\\");
		
		if(XM_FindFirstFile (jpg_path, &fileFind, fileName, XM_MAX_FILEFIND_NAME))
		{
			printf ("XM_FindFirstFile ok\n");
			do
			{
				// 检查目录项属性
				DWORD dwFileAttributes = fileFind.dwFileAttributes;
				//printf ("fileName=%s\n", fileName);
				// 检查该目录项是否是文件属性
				if((dwFileAttributes & (ATTR_DIRECTORY | ATTR_VOLUME_ID)) == 0x00)
				{
					// 文件属性
				}
				else if((dwFileAttributes & (ATTR_DIRECTORY | ATTR_VOLUME_ID)) == ATTR_DIRECTORY)
				{
					// 目录属性
					// 解析宽高信息
					if( fileName[0] == '.' && fileName[1] == '\0' )
					{
					}
					else if( fileName[0] == '.' && fileName[1] == '.' && fileName[2] == '\0' )
					{
					}
					else if(strlen(fileName) != 8)
					{
						XM_printf ("ERROR, skip directory (%s)\n", fileName);
					}
					else
					{
						char wt[5], ht[5];
						int w, h;
						w = 0;
						h = 0;
						memset (wt, 0, sizeof(wt));
						memset (ht, 0, sizeof(ht));
						memcpy (wt, fileName, 4);
						memcpy (ht, fileName+4, 4);
						sscanf (wt, "%04d", &w);
						sscanf (ht, "%04d", &h);
						if(h == 0 || w == 0)
						{
							XM_printf ("ERROR, skip directory (%s)\n", fileName);
						}
						else if( (h & 0xF) || (w & 0xF) )
						{
							XM_printf ("ERROR, skip directory (%s), w(%d) h(%d) must be multiple of 16\n", fileName, w, h);
						}
						else
						{
							sprintf (jpg_testcase_path, "%s%04d%04d\\", jpg_path, w, h);
							sprintf (yuv_testcase_path, "%s%04d%04d\\", yuv_path, w, h);
							// 创建yuv目录
							if(check_and_create_directory (yuv_testcase_path))
							{
								// 目录NG
								XM_printf ("can't create testcase directory(%s)\n", yuv_testcase_path);
							}
							else
							{
								// 目录OK
								XM_printf ("\n\n\tProcess JPG, w=%d, h=%d\n", w, h);
								arkn141_jpg2yuv_process_directory (jpg_testcase_path, yuv_testcase_path, w, h);
							}
						}
					}
				}
			} while (XM_FindNextFile (&fileFind));
			XM_FindClose (&fileFind);
		}
		else
		{
			XM_printf ("can't locate jpg path(%s)\n", jpg_path);
			return -1;
		}
	}

	printf ("arkn141_jpg2yuv_test finish\n");
		
	return 0;
}



// < 0 表示错误
// >= 0 表示JPEG编码的时间
int arkn141_yuv2jpg_process_file (const char *src_yuv_file, 
								  const char *dst_jpg_file, 
								  int w, 
								  int h,
								  int yuv_format,
								  int slice_height		// 16为最小的slice单元
								)
{
	char *argv[32];
	int argc = 0;
	char string[512];
	char *cmd = string;
	memset (string, 0, sizeof(string));
	memset (argv, 0, sizeof(argv));

	sprintf (cmd, "arkn141_yuv2jpg");
	argv[argc] = cmd;
	argc ++;
	cmd += strlen(cmd) + 1;
	
	
	sprintf (cmd, "-w");
	argv[argc] = cmd;
	argc ++;
	cmd += strlen(cmd) + 1;
	
	sprintf (cmd, "%d", w);
	argv[argc] = cmd;
	argc ++;
	cmd += strlen(cmd) + 1;
	
	sprintf (cmd, "-h");
	argv[argc] = cmd;
	argc ++;
	cmd += strlen(cmd) + 1;
	
	sprintf (cmd, "%d", h);
	argv[argc] = cmd;
	argc ++;
	cmd += strlen(cmd) + 1;
	
	sprintf (cmd, "-b");
	argv[argc] = cmd;
	argc ++;
	cmd += strlen(cmd) + 1;
	
	sprintf (cmd, "0");
	argv[argc] = cmd;
	argc ++;
	cmd += strlen(cmd) + 1;
	
	sprintf (cmd, "-g");
	argv[argc] = cmd;
	argc ++;
	cmd += strlen(cmd) + 1;
	
	sprintf (cmd, "%d", yuv_format);
	argv[argc] = cmd;
	argc ++;
	cmd += strlen(cmd) + 1;
	
	sprintf (cmd, "-i");
	argv[argc] = cmd;
	argc ++;
	cmd += strlen(cmd) + 1;
	
	sprintf (cmd, "%s", src_yuv_file);
	argv[argc] = cmd;
	argc ++;
	cmd += strlen(cmd) + 1;
	
	sprintf (cmd, "-o");
	argv[argc] = cmd;
	argc ++;
	cmd += strlen(cmd) + 1;
	
	sprintf (cmd, "%s", dst_jpg_file);
	argv[argc] = cmd;
	argc ++;
	cmd += strlen(cmd) + 1;
	
	if(slice_height && slice_height >= 16)
	{
		//	"-p","1",   //表示需要分割成Sclice
		//"-R","2",  //应该表示分割成2*16行的小片	
		
		sprintf (cmd, "-p");
		argv[argc] = cmd;
		argc ++;
		cmd += strlen(cmd) + 1;
		
		sprintf (cmd, "1");
		argv[argc] = cmd;
		argc ++;
		cmd += strlen(cmd) + 1;

		sprintf (cmd, "-R");
		argv[argc] = cmd;
		argc ++;
		cmd += strlen(cmd) + 1;
		
		sprintf (cmd, "%d", slice_height/16);
		argv[argc] = cmd;
		argc ++;
		cmd += strlen(cmd) + 1;
	}
		
	
	arkn141_codec_open (ARKN141_CODEC_TYPE_JPEG_ENCODER);
	SetJpegQLevel (9);		// 9, high image quality
	jpeg_encode_main (argc, argv);	
	arkn141_codec_close (ARKN141_CODEC_TYPE_JPEG_ENCODER);
	return 0;
}

int arkn141_yuv2jpg_process_directory (char *src_dir_path, char *dst_dir_path, int w, int h, int yuv_format, int slice_height)
{
	char fileName[XM_MAX_FILEFIND_NAME];
	char src_file[64];
	char dst_file[64];
	XMFILEFIND fileFind;
	int ret = -1;
	unsigned int max_codec_time = 0; 
	unsigned int min_codec_time = 0x7FFFFFFF;
	unsigned int avg_codec_time = 0;
	int codec_file_count = 0;

	check_and_create_directory(dst_dir_path);
	do
	{
		printf ("process directory (%s)\n", src_dir_path);
		if(XM_FindFirstFile (src_dir_path, &fileFind, fileName, XM_MAX_FILEFIND_NAME))
		{
			do
			{
				// 检查目录项属性
				DWORD dwFileAttributes = fileFind.dwFileAttributes;
				// 检查该目录项是否是文件属性
				if((dwFileAttributes & (ATTR_DIRECTORY | ATTR_VOLUME_ID)) == 0x00)
				{
					// 文件属性
					char *ext;
					int yuv2jpg_ret;
					sprintf (src_file, "%s%s", src_dir_path, fileName);
					ext = strchr (fileName, '.');
					if(ext)
						*ext = 0;
					sprintf (dst_file, "%s%s.jpg", dst_dir_path, fileName);
					
					yuv2jpg_ret = arkn141_yuv2jpg_process_file (src_file, dst_file, w, h, yuv_format, slice_height);
					if(yuv2jpg_ret < 0)
						XM_printf ("--> yuv2jpg NG, (%s)\n", src_file);
					else
					{
						XM_printf ("--> yuv2jpg OK, (%s), encode time = %d ms\n", src_file, yuv2jpg_ret);
						if(yuv2jpg_ret < min_codec_time)
							min_codec_time = yuv2jpg_ret;
						if(yuv2jpg_ret > max_codec_time)
							max_codec_time = yuv2jpg_ret;
						codec_file_count ++;
						avg_codec_time += yuv2jpg_ret;
					}
				}

			} while (XM_FindNextFile (&fileFind));
			XM_FindClose (&fileFind);
			ret = 0;
		}
	} while(0);
		
	// 编码时间报告
	if(codec_file_count && avg_codec_time)
	{
		printf ("\n");
		printf ("     JPEG encode time report, w = %4d, h = %4d\n", w, h);
		printf ("     total encode file(OK) count %d\n", codec_file_count);
		printf ("     minimum encode time = %d ms\n",  min_codec_time);
		printf ("     maximum encode time = %d ms\n",  max_codec_time);
		printf ("     average encode time = %d ms\n",  (avg_codec_time + codec_file_count - 1) / codec_file_count);
		
		printf ("     FPGA ( 48MHz) fps = %f\n", (((double)1000.0) * codec_file_count) / avg_codec_time);
		printf ("     ASIC (200MHz) fps = %f\n", (((double)1000.0 * 200.0/48.0) * codec_file_count) / avg_codec_time);
		printf ("\n\n");
	}
	return ret;
}


// YUV420 --> JPG 转换测试
// repeat_test_count 重复测试的次数
/* 源数据目录    \\YUV2JPG\\YUV\\， 为YUV420格式 */
/* 目标数据目录  \\YUV2JPG\\JPG0\\ */
/*               \\YUV2JPG\\JPG1\\ */
static const char *yuv_id[] = {
	"YUV420",
	"Y_UV420",
	"YUYV"
};
int arkn141_yuv2jpg_test_ex (char *volume_name, unsigned int repeat_test_count, int yuv_format, int slice_height)
{
	// 检查\\YUV2JPG\\YUV\\目录是否存在
	XMFILEFIND fileFind;
	char fileName[XM_MAX_FILEFIND_NAME];
	char jpg_path[64];
	char yuv_path[64];
	char jpg_testcase_path[64];
	char yuv_testcase_path[64];
	unsigned int index;
	
	//char yuv_name[32];
	const char *yuv_name = NULL;
	
	//memset (yuv_name, 0, sizeof(yuv_name));
	if(yuv_format == ARKN141_YUV420)
		//strcpy (yuv_name, yuv_id[0]);
		yuv_name = yuv_id[0];
	else if(yuv_format == ARKN141_Y_UV420)
		//strcpy (yuv_name, yuv_id[1]);
		yuv_name = yuv_id[1];
	else if(yuv_format == ARKN141_YUYV)
		//strcpy (yuv_name, yuv_id[2]);
		yuv_name = yuv_id[2];
	else
	{
		printf ("illegal format (%d)\n", yuv_format);
		return -1;
	}
	
	printf ("arkn141_yuv2jpg_test %s begin, YUV format (%s)\n", volume_name, yuv_name);
	
	sprintf (yuv_path, "%s:\\MINITEST\\RAW\\%s\\", volume_name, yuv_name);
	
	if(repeat_test_count == 0)
		repeat_test_count = 1;
	if(repeat_test_count > 9)
		repeat_test_count = 9;
	
	for (index = 0; index < repeat_test_count; index ++)
	{
		sprintf (jpg_path, "%s:\\ON2ENC\\JPG%d\\%s\\SLICE\\", volume_name, index, yuv_name);
		// 检查目录是否存在
		if(check_and_create_directory (jpg_path))
		{
			XM_printf ("can't create jpg path (%s)\n", jpg_path);
			return -1;
		}
		strcat (jpg_path, "\\");
		
		if(XM_FindFirstFile (yuv_path, &fileFind, fileName, XM_MAX_FILEFIND_NAME))
		{
			do
			{
				// 检查目录项属性
				DWORD dwFileAttributes = fileFind.dwFileAttributes;
				// 检查该目录项是否是文件属性
				if((dwFileAttributes & (ATTR_DIRECTORY | ATTR_VOLUME_ID)) == 0x00)
				{
					// 文件属性
				}
				else if((dwFileAttributes & (ATTR_DIRECTORY | ATTR_VOLUME_ID)) == ATTR_DIRECTORY)
				{
					// 目录属性
					// 解析宽高信息
					if( fileName[0] == '.' && fileName[1] == '\0' )
					{
					}
					else if( fileName[0] == '.' && fileName[1] == '.' && fileName[2] == '\0' )
					{
					}
					else if(strlen(fileName) != 8)
					{
						XM_printf ("ERROR, skip directory (%s)\n", fileName);
					}
					else
					{
						char wt[5], ht[5];
						int w, h;
						w = 0;
						h = 0;
						memset (wt, 0, sizeof(wt));
						memset (ht, 0, sizeof(ht));
						memcpy (wt, fileName, 4);
						memcpy (ht, fileName+4, 4);
						sscanf (wt, "%04d", &w);
						sscanf (ht, "%04d", &h);
						
						// the encoded picture width
						// has to be a multiple of 4 and the height a multiple of 2; the smallest encoded picture size
						// is 96x32; the maximum input picture size is 8192x8192 and the maximum encoded
						// picture size is 8176x8176.						
						
						if(h == 0 || w == 0)
						{
							XM_printf ("ERROR, skip directory (%s)\n", fileName);
						}
						else if( (h & 0x2) || (w & 0x3) )
						{
							XM_printf ("ERROR, skip directory (%s), w(%d) must be multiple of 4,  h(%d) must be multiple of 2\n", fileName, w, h);
						}
						else if( w < 96 || h < 32 )
						{
							XM_printf ("ERROR, the smallest encoded picture size is 96x32\n");
						}
						else if( w > 8176  || h > 8176 )
						{
							XM_printf ("ERROR, the maximum encoded picture size is 8176x8176\n");
						}
						else if(slice_height && h <= slice_height)
						{
							XM_printf ("SKIP, h (%d) less than slice height (%d)\n", h, slice_height);
						}
						else
						{
							sprintf (jpg_testcase_path, "%s%04d%04d\\", jpg_path, w, h);
							sprintf (yuv_testcase_path, "%s%04d%04d\\", yuv_path, w, h);
							// 创建jpg目录
							if(check_and_create_directory (jpg_testcase_path))
							{
								// 目录NG
								XM_printf ("can't create testcase directory(%s)\n", jpg_testcase_path);
							}
							else
							{
								// 目录OK
								XM_printf ("\n\tProcess YUV, w=%d, h=%d\n", w, h);
								if(slice_height && slice_height > h)
									slice_height = 0;		// 禁止slice功能
								
								arkn141_yuv2jpg_process_directory (yuv_testcase_path, jpg_testcase_path, w, h, yuv_format, slice_height);
							}
						}
					}
				}
			} while (XM_FindNextFile (&fileFind));
			XM_FindClose (&fileFind);
		}
		else
		{
			XM_printf ("can't locate yuv path(%s)\n", yuv_path);
			return -1;
		}
	}		
			
	printf ("ark1960_yuv2jpg_test %s end\n", volume_name);
	return 0;
}

int arkn141_yuv2jpg_test (char *volume_name)
{
	printf ("arkn141_yuv2jpg_test %s start ...\n", volume_name);
	
	// 
	arkn141_yuv2jpg_test_ex (volume_name, 2, ARKN141_YUV420, 0);
	arkn141_yuv2jpg_test_ex (volume_name, 2, ARKN141_Y_UV420, 0);
	arkn141_yuv2jpg_test_ex (volume_name, 2, ARKN141_YUYV, 0);

	printf ("arkn141_yuv2jpg_test %s finish\n", volume_name);
	
	return 0;
}

int arkn141_yuv2jpg_slice_test (char *volume_name)
{
	printf ("arkn141_yuv2jpg_slice_test %s start ...\n", volume_name);
	
	// SLICE测试
	arkn141_yuv2jpg_test_ex (volume_name, 1, ARKN141_Y_UV420, 64);

	printf ("arkn141_yuv2jpg_slice_test %s finish\n", volume_name);
	
	return 0;
}