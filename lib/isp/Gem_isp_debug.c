// =============================================================================
// File        : Gem_isp.c
// Version     : v1.0
// Author      : Honglei Zhu
// Date        : 2014.12.5
// -----------------------------------------------------------------------------
// Description :
//
// -----------------------------------------------------------------------------
#include "hardware.h"
#include <xm_printf.h>
#include "Gem_isp.h"
#include "Gem_isp_colors.h"
#include "Gem_isp_denoise.h"
#include "Gem_isp_eris.h"
#include "Gem_isp_fesp.h"
#include "Gem_isp_enhance.h"
#include "Gem_isp_awb.h"
#include "Gem_isp_ae.h"
#include "Gem_isp_sys.h"
//#include "Gem_isp_sensor.h"
#include "Gem_isp_io.h"

#include "RTOS.h"
#include "xm_h264_cache_file.h"
#include "fs.h"
#include "xm_file.h"
#include "irqs.h"
#include "arkn141_isp_sensor_cfg.h"


#include "arkn141_isp_exposure_cmos.h"
#include "arkn141_isp_exposure.h"

#define	ISP_YUV_DEBUG

void write_ae_info(void * fp );

extern isp_sys_t p_sys;
extern isp_param_t s_isp;
extern isp_sen_t p_sen; 
extern isp_colors_t p_colors;
extern isp_denoise_t p_denoise;
extern isp_eris_t p_eris;
extern isp_fesp_t p_fesp;
extern isp_enhance_t p_enhance;
extern isp_awb_t p_awb;
extern isp_ae_t p_ae;
extern cmos_exposure_t isp_exposure;


static int check_device_and_create_directory( char *filepath ) ;

char c2i(char c )
{
   return c-'0';
}



void writestring(char *data ,void * fp )
{
   XM_fwrite ( data , 1, strlen(data) , fp ) ;
}

/*
   isp_write_file(char *filepath ,char *filetime, unsigned int filemount )
   filepath:  文件路径
   filetime： 文件时间，用于合并到文件名中
   filemount：写文件个数，最大数值为4
   TimeStamp  文件时间戳
*/
int isp_write_file(char *filepath ,char *filetime, unsigned int filemount ,unsigned int TimeStamp )
{
   unsigned int Yaddr , Uaddr , Vaddr;
   unsigned int write_count;
   unsigned int w_raw,yuv_format;
   char filename[128] ={0};  
   unsigned int i;
   void *fp;
   unsigned int order = 4;
   int id,ret = -1;
   
   i = Gem_read ( GEM_SYS_BASE+0x14 );
   w_raw = (i>>2)&0x1 ;   
   yuv_format = (i>>5)&0x3;
   
   do{
      if( check_device_and_create_directory( filepath ) == -1 )
         break;

      OS_Delay(1000);
      //writefile
      XM_printf ("sensor:%s height :%d   width :%d \r\n", SENSOR_NAME,IMAGE_H_SZ, IMAGE_V_SZ );
      // 获 得 最 新 刚 完 成 的 帧
      id = get_isp_framebufferno();
      
      if(filemount >4 )
         filemount = 4;
      XM_printf("%d\n" , OS_GetTime32() );
      //写入 参数 
      memset(  filename , 0 , 128 ) ;
      sprintf( filename,"%s%s%d%d.txt", filepath,filetime,order,id );
      XM_printf( "filename:%s\n" , filename ) ;
      fp = (void *)XM_fopen ( filename, "wb" );
      if( !fp )
      {
         XM_printf("cann't open file %s\n", filename );
         break ;
      }
      
      writestring (filename,fp);
      writestring ("\r\n\r\n",fp);
      write_ae_info( fp );
      XM_fclose( fp );
      FS_SetFileTime(filename, TimeStamp ) ;
      
      // 记录 yuv raw
      for( ; ( order && filemount ) ; order --,filemount-- )
      {
         XM_printf("%d\n" , OS_GetTime32() );
         //获得 数据 所在 地址
         get_isp_dispaddr( id , &Yaddr , &Uaddr , &Vaddr);
         XM_printf("id:%d\n", id ) ;
         memset(  filename , 0 , 128 ) ;
			
         sprintf( filename,"%s%s%d%d.yuv", filepath,filetime,order,id);
         XM_printf( "filename:%s\n" , filename ) ;
         
#ifdef ISP_YUV_DEBUG
         fp = (void *)XM_fopen ( filename, "wb" );
         if( !fp )
         {
            XM_printf("cann't open file %s\n", filename );
            return ret;
         }
         
         // write yuv separate data // 0：y_uv420 1:y_uv422 2:yuv420 3:yuv422 
			XM_printf ("yuv_format=%d\n", yuv_format);
			unsigned int frame_size;
         if( yuv_format == 0 )
         {
				// Y_UV420
				frame_size = isp_get_video_image_size ();
           // Y_UV420toYUV420(Yaddr, yuv_420buf , IMAGE_H_SZ , IMAGE_V_SZ );
            //write_count = XM_fwrite ( (void*)yuv_420buf , 1, frame_size , fp ) ;
				write_count = XM_fwrite ( (void*)Yaddr , 1, frame_size , fp ) ;
         }
         else if( yuv_format == 2 )
         {
				frame_size = isp_get_video_image_size ();
            write_count = XM_fwrite ( (void*)Yaddr , 1, frame_size , fp ) ;
         }
         else
         {
				// Y_UV422/YUV422
				frame_size = isp_get_video_image_size ();
				write_count = XM_fwrite ( (void*)Yaddr , 1, frame_size , fp ) ;
			}
			if(write_count != frame_size )
            XM_printf("Write YUV data error!!!\n");
			
			
   
         XM_fclose( fp );
#endif
			
         FS_SetFileTime(filename, TimeStamp ) ;
         XM_printf("%d\n" , OS_GetTime32() );
         // raw 如果支持写raw  则记录raw
         if(w_raw)
         {
            get_isp_rawaddr( id, &Yaddr  );
            memset(  filename , 0 , 128 ) ;
            sprintf( filename,"%s%s%d%d.raw", filepath,filetime,order,id);
            XM_printf( "filename:%s\n" , filename ) ;
            fp = (void *)XM_fopen ( filename, "wb" );
            if( !fp )
            {
               XM_printf("cann't open file %s\n", filename );
               return ret;
            }
            
				u32_t image_raw_size = isp_get_video_width() * isp_get_video_height () * 2;
            write_count = XM_fwrite ( (void*)Yaddr , 1, image_raw_size , fp ) ;
            if(write_count != image_raw_size )
               XM_printf("Write raw data error!!!\n");
            XM_fclose( fp );
            FS_SetFileTime(filename, TimeStamp ) ;
         }
         //找到下一个 完成更早的帧 
         id-- ;
         if( id < 0 )
           id = 3;
      }

      XM_printf("%d\n" , OS_GetTime32() );
      FS_CACHE_Clean("");
      
      ret = 0;
   }while(0);
   
   return ret;
}

/*
   isp_write_file(char *filepath ,char *filetime, unsigned int filemount )
   filepath:  文件路径
   filetime： 文件时间，用于合并到文件名中
   filemount：写文件个数，最大数值为4
   TimeStamp  文件时间戳
*/
int isp_multi_ae_write_file(char *filepath ,char *filetime, unsigned int filemount ,unsigned int TimeStamp, unsigned int order )
{
   unsigned int Yaddr , Uaddr , Vaddr;
   unsigned int write_count;
   unsigned int w_raw,yuv_format;
   char filename[128] ={0};  
   unsigned int i;
   void *fp;
  // unsigned int order = 4;
   int id,ret = -1;
   
   i = Gem_read ( GEM_SYS_BASE+0x14 );
   w_raw = (i>>2)&0x1 ;   
   yuv_format = (i>>5)&0x3;
   
   do{
      if( check_device_and_create_directory( filepath ) == -1 )
         break;

      OS_Delay(1000);
      //writefile
      XM_printf ("sensor:%s height :%d   width :%d \r\n", SENSOR_NAME,IMAGE_H_SZ, IMAGE_V_SZ );
      // 获 得 最 新 刚 完 成 的 帧
      id = get_isp_framebufferno();
      
      if(filemount >4 )
         filemount = 4;
      XM_printf("%d\n" , OS_GetTime32() );
      //写入 参数 
      memset(  filename , 0 , 128 ) ;
      sprintf( filename,"%s%s%d%d.txt", filepath,filetime,order,id );
      XM_printf( "filename:%s\n" , filename ) ;
      fp = (void *)XM_fopen ( filename, "wb" );
      if( !fp )
      {
         XM_printf("cann't open file %s\n", filename );
         break ;
      }
      
      writestring (filename,fp);
      writestring ("\r\n\r\n",fp);
      write_ae_info( fp );
      XM_fclose( fp );
      FS_SetFileTime(filename, TimeStamp ) ;
      
      // 记录 yuv raw
      for( ; ( order && filemount ) ; order --,filemount-- )
      {
         XM_printf("%d\n" , OS_GetTime32() );
         //获得 数据 所在 地址
         get_isp_dispaddr( id , &Yaddr , &Uaddr , &Vaddr);
         XM_printf("id:%d\n", id ) ;
         memset(  filename , 0 , 128 ) ;
         sprintf( filename,"%s%s%d%d.yuv", filepath,filetime,order,id);
         XM_printf( "filename:%s\n" , filename ) ;
         
         fp = (void *)XM_fopen ( filename, "wb" );
         if( !fp )
         {
            XM_printf("cann't open file %s\n", filename );
            return ret;
         }
         
         // write yuv separate data // 0：y_uv420 1:y_uv422 2:yuv420 3:yuv422 
			XM_printf ("yuv_format=%d\n", yuv_format);
			unsigned int frame_size;
         if( yuv_format == 0 )
         {
				frame_size = isp_get_video_image_size ();
            //Y_UV420toYUV420(Yaddr, yuv_420buf , IMAGE_H_SZ , IMAGE_V_SZ );
           // write_count = XM_fwrite ( (void*)yuv_420buf , 1, isp_get_video_image_size () , fp ) ;
				write_count = XM_fwrite ( (void*)Yaddr , 1, frame_size , fp ) ;
         }
         else if( yuv_format == 2 )
         {
            //write_count = XM_fwrite ( (void*)Yaddr , 1, isp_get_video_image_size () , fp ) ;
				frame_size = isp_get_video_image_size ();
				write_count = XM_fwrite ( (void*)Yaddr , 1, frame_size , fp ) ;
         }
         else
			{
				// Y_UV422/YUV422
            // XM_printf("时序错误！！！\n");
				frame_size = isp_get_video_image_size ();
				write_count = XM_fwrite ( (void*)Yaddr , 1, frame_size , fp ) ;
			}
         if(write_count != frame_size)	//isp_get_video_image_size() )
            XM_printf("Write YUV data error!!!\n");
   
         XM_fclose( fp );
         FS_SetFileTime(filename, TimeStamp ) ;
         XM_printf("%d\n" , OS_GetTime32() );
         // raw 如果支持写raw  则记录raw
         if(w_raw)
         {
            get_isp_rawaddr( id, &Yaddr  );
            memset(  filename , 0 , 128 ) ;
            sprintf( filename,"%s%s%d%d.raw", filepath,filetime,order,id);
            XM_printf( "filename:%s\n" , filename ) ;
            fp = (void *)XM_fopen ( filename, "wb" );
            if( !fp )
            {
               XM_printf("cann't open file %s\n", filename );
               return ret;
            }
            
				u32_t image_raw_size = isp_get_video_width() * isp_get_video_height () * 2;
            write_count = XM_fwrite ( (void*)Yaddr , 1, image_raw_size , fp ) ;
            if(write_count != image_raw_size )
               XM_printf("Write raw data error!!!\n");
            XM_fclose( fp );
            FS_SetFileTime(filename, TimeStamp ) ;
         }
         //找到下一个 完成更早的帧 
         id-- ;
         if( id < 0 )
           id = 3;
      }

      XM_printf("%d\n" , OS_GetTime32() );
      FS_CACHE_Clean("");
      
      ret = 0;
   }while(0);
   
   return ret;
}

void write_ae_info(void * fp )
{
   //char data[512];
	char *buffer;
	char *data;
   int i,j;
   unsigned int data0,data1,data2,data3;
	
	buffer = kernel_calloc (0x20000, 1);
	if(buffer == NULL)
		return;
	data = buffer;
	
	sprintf (data, "sensor:      %s \r\n",SENSOR_NAME );
	data += strlen(data);
   
	sprintf (data, "\r\nheight :%d   width :%d\r\n", IMAGE_H_SZ, IMAGE_V_SZ );
	data += strlen(data);

   //添加所有使能 
   data0 = Gem_read ( GEM_AWB0_BASE+0x00 ) ;
	sprintf (data, "awb :%s\r\n", (data0&0x1)?"enable":"disable");
	data += strlen(data);

   data0 = Gem_read (GEM_COLORS_BASE+0x00) ;
	sprintf (data, "color matrix :%s\r\n", data0?"enable":"disable" );
	data += strlen(data);
	
   data0 = Gem_read (GEM_COLORS_BASE+0x18) ;
	sprintf (data, "gamma :%s\r\n", data0?"enable":"disable" );
	data += strlen(data);
	
   data0 = Gem_read ( GEM_DENOISE_BASE+0x00) ;
	sprintf (data, "enable2d = %d;\r\n", data0&0x7 );
	data += strlen(data);
	
	sprintf (data, "enable3d = %d;\r\n", (data0>>3)&0x7);
	data += strlen(data);
	
   data0 = Gem_read (GEM_ENHANCE_BASE+0x00) ;
	sprintf (data, "sharp :%s  \r\n", (data0&0x1)?"enable":"disable" );
	data += strlen(data);
	
   data0 = Gem_read (GEM_ENHANCE_BASE+0x04) ;
	sprintf (data, "bcst :%s  \r\n", (data0>>31)?"enable":"disable" );
	data += strlen(data);
	
   data0 = Gem_read (GEM_ERIS_BASE+0x00) ;
	sprintf (data, "eris:%s\r\n", (data0&0x1)?"enable":"disable" );
	data += strlen(data);
	
   data0 = Gem_read (GEM_LENS_BASE+0x00) ;
   sprintf (data, "lensshade:%s\r\n", (data0&0x1)?"enable":"disable"  );
	data += strlen(data);
	
   data0 = Gem_read (GEM_FIXPATT_BASE+0x00) ;
   sprintf (data, "Fixpatt :%s\r\n", (data0&0x1)?"enable":"disable" );
	data += strlen(data);
	
   data0 = Gem_read (GEM_BADPIX_BASE+0x00) ;
   sprintf (data, "Badpix :%s\r\n", (data0&0x1)?"enable":"disable" );
	data += strlen(data);
	
   data0 = Gem_read (GEM_CROSS_BASE+0x00) ;
   sprintf (data, "Crosstalk :%s\r\n", (data0&0x1)?"enable":"disable" );
	data += strlen(data);
	
   
   
   // ae finish
	sprintf (data, "\r\n=========================\r\nae info:\r\n");
	data += strlen(data);
	
	sprintf (data, "histoBand:\r\n");
	data += strlen(data);
	
   data0 = Gem_read (GEM_AE0_BASE+0x00);
   sprintf (data, "[0]:%d ", data0&0xff);
	data += strlen(data);
	
   sprintf (data, "[1]:%d ", (data0>>8)&0xff);
	data += strlen(data);
	
   sprintf (data, "[2]:%d ", (data0>>16)&0xff);
	data += strlen(data);
	
   sprintf (data, "[3]:%d\r\n", (data0>>24)&0xff);
	data += strlen(data);
   
   // winWeight 
	sprintf (data, "winWeight:\r\n");
	data += strlen(data);
	
   data0 = Gem_read (GEM_AE0_BASE+0x04);
   sprintf (data, "[0][0]:%d  ", (data0>>0)&0xf);
	data += strlen(data);
	
   sprintf (data, "[0][1]:%d  ", (data0>>4)&0xf);
	data += strlen(data);
	
   sprintf (data, "[0][2]:%d  \r\n", (data0>>8)&0xf);
	data += strlen(data);
	
   sprintf (data, "[1][0]:%d  ", (data0>>12)&0xf);
	data += strlen(data);
	
   sprintf (data, "[1][1]:%d  ", (data0>>16)&0xf);
	data += strlen(data);
	
   sprintf (data, "[1][2]:%d  \r\n", (data0>>20)&0xf);
	data += strlen(data);
	
   sprintf (data, "[2][0]:%d  ", (data0>>24)&0xf);
	data += strlen(data);
   sprintf (data, "[2][1]:%d  ", (data0>>28)&0xf);
	data += strlen(data);
   
   data0 = Gem_read (GEM_AE0_BASE+0x08);
   sprintf (data, "[2][2]:%d\r\n", (data0>>0)&0xf);
	data += strlen(data);
   
	
   
	sprintf (data, "\r\n");
	data += strlen(data);
	
	unsigned int GEM_SYS_BASE_0x14 = Gem_read (GEM_SYS_BASE + 0x14);
	//if( (GEM_SYS_BASE_0x14 & 1)/*debugmode*/ && (GEM_SYS_BASE_0x14 & 0x02) /*estenable*/ && (GEM_SYS_BASE_0x14 & 0x10) /*refenable*/)	// 使用采集好的RAW数据
	//	sprintf (data, "lumCurr   = 0x%08x // real time\r\n", 0);
	//else
   	sprintf (data, "lumCurr   = 0x%08x // real time\r\n", (Gem_read (GEM_AE1_BASE+0x00))&0xff);
	data += strlen(data);
	if( (GEM_SYS_BASE_0x14 & 1)/*debugmode*/ && (GEM_SYS_BASE_0x14 & 0x02) /*estenable*/ && (GEM_SYS_BASE_0x14 & 0x10) /*refenable*/)	// 使用采集好的RAW数据
		sprintf (data, "lumCurr   = 0x%08x\r\n", 0);
	else
   	sprintf (data, "lumCurr   = 0x%08x\r\n", p_ae.lumCurr);
	data += strlen(data);
	
	sprintf (data, "ae_hist_balance = %6d\n", isp_exposure.cmos_ae.histogram.hist_balance);
	data += strlen(data);
	sprintf (data, "ae_hist_error = %6d\n", isp_exposure.cmos_ae.histogram.hist_error);
	data += strlen(data);
	sprintf (data, "ae_lum = %6d  // real time\n", (u16_t)isp_ae_lum_read());
	data += strlen(data);	
	sprintf (data, "ae_compensation = %3d\n", isp_exposure.cmos_ae.ae_compensation);
	data += strlen(data);	
	sprintf (data, "ae_black_target = %3d\n", isp_exposure.cmos_ae.ae_black_target);
	data += strlen(data);	
	sprintf (data, "ae_bright_target = %3d\n", isp_exposure.cmos_ae.ae_bright_target);
	data += strlen(data);	
	sprintf (data, "ae_increment_step = %10d\n", isp_exposure.cmos_ae.increment_step);
	data += strlen(data);	
	sprintf (data, "ae_increment_offset = %10d\n", isp_exposure.cmos_ae.increment_offset);
	data += strlen(data);	
	sprintf (data, "ae_increment_max = %10d\n", isp_exposure.cmos_ae.increment_max);
	data += strlen(data);	
	sprintf (data, "ae_increment_min = %10d\n", isp_exposure.cmos_ae.increment_min);
	data += strlen(data);	
	sprintf (data, "ae_exposure = %10d\n", isp_exposure.last_exposure);
	data += strlen(data);	
	sprintf (data, "ae_new_exposure = %10d\n", isp_exposure.exposure);
	data += strlen(data);	
	sprintf (data, "ae_increment = %10d\n", isp_exposure.cmos_ae.increment);
	data += strlen(data);	
	sprintf (data, "ae_log_exposure = %10d\n", isp_exposure.physical_exposure);
	data += strlen(data);	
	sprintf (data, "ae_sensor_inttime = %10d\n", isp_exposure.cmos_inttime.exposure_ashort);
	data += strlen(data);	
	
	if(	isp_exposure.cmos_gain.again == 0 
		&& isp_exposure.cmos_gain.again_shift == 0
		&& isp_exposure.cmos_gain.dgain == 0
		&& isp_exposure.cmos_gain.dgain_shift == 0)
	{
		// 从脚本中运行
		sprintf (data, "ae_sensor_again = %f\n", (float)isp_exposure.cmos_gain.log_again);
		data += strlen(data);	
		sprintf (data, "ae_sensor_dgain = %f\n", (float)isp_exposure.cmos_gain.log_dgain);
		data += strlen(data);	
	}
	else
	{
		sprintf (data, "ae_sensor_again = %f\n", 
					(float)isp_exposure.cmos_gain.again / (float)(1 << isp_exposure.cmos_gain.again_shift));
		data += strlen(data);	
		sprintf (data, "ae_sensor_dgain = %f\n", 
					(float)isp_exposure.cmos_gain.dgain / (float)(1 << isp_exposure.cmos_gain.dgain_shift));
		data += strlen(data);	
	}
	
   
	sprintf (data, "\r\nae info histoGram :\r\n");
	data += strlen(data);
   isp_ae_info_read( &p_ae );
   sprintf (data, "[0] = %4d;\r\n", p_ae.histoGram[0]);
	data += strlen(data);
   sprintf (data, "[1] = %4d;\r\n", p_ae.histoGram[1]);
	data += strlen(data);
   sprintf (data, "[2] = %4d;\r\n", p_ae.histoGram[2]);
	data += strlen(data);
   sprintf (data, "[3] = %4d;\r\n", p_ae.histoGram[3]);
	data += strlen(data);
   sprintf (data, "[4] = %4d;\r\n", p_ae.histoGram[4]);
	data += strlen(data);
	sprintf (data, "\r\n");
	data += strlen(data);
	
   isp_ae_sts2_read( &p_ae );
   for (i = 0; i < 3; i++)
   {
     for (j = 0; j < 3; j++)
     {
         sprintf (data, "[%d][%d]:", i,j );
			data += strlen(data);
         
         sprintf (data, "yavg_s = %4d;", p_ae.yavg_s[i][j] );
			data += strlen(data);
         sprintf (data, "[0] = %4d; ", p_ae.histogram_s[i][j][0] );
			data += strlen(data);
         sprintf (data, "[1] = %4d; ", p_ae.histogram_s[i][j][1] );
			data += strlen(data);
         sprintf (data, "[2] = %4d; ", p_ae.histogram_s[i][j][2] );
			data += strlen(data);
         sprintf (data, "[3] = %4d; ", p_ae.histogram_s[i][j][3] );
			data += strlen(data);
         sprintf (data, "[4] = %4d;\r\n", p_ae.histogram_s[i][j][4] );
			data += strlen(data);
     }
   }
   //denoise   finish
	sprintf (data, "\r\ndenoise:\r\n");
	data += strlen(data);
   data0 = Gem_read ( GEM_DENOISE_BASE+0x00) ;
   sprintf (data, "enable2d = %d;\r\n", data0&0x7 );
	data += strlen(data);
   sprintf (data, "enable3d = %d;\r\n", (data0>>3)&0x7);
	data += strlen(data);
	sprintf (data, "sensitiv0 = %d;\r\n", (data0>>10) & 0x7 );
	data += strlen(data);
	sprintf (data, "sensitiv1 = %d;\r\n", (data0>>13) & 0x7 );
	data += strlen(data);
	sprintf (data, "sel_3d_table = %d;\r\n", (data0>>8) & 0x3 );
	data += strlen(data);
	sprintf (data, "sel_3d_matrix = %d; // 0 邻近 1 中心\r\n", (data0>>16) & 0x1 );
	data += strlen(data);
   

   data0 = Gem_read ( GEM_DENOISE_BASE+0x04) ;
   sprintf (data, "y_thres0 = %d;\r\n", data0&0x3ff );
	data += strlen(data);
   sprintf (data, "u_thres0 = %d;\r\n", (data0>>10)&0x3ff );
	data += strlen(data);
   sprintf (data, "v_thres0 = %d;\r\n", (data0>>20)&0x3ff );
	data += strlen(data);

   data0 = Gem_read ( GEM_DENOISE_BASE+0x08) ;
   sprintf (data, "y_thres1 = %d;\r\n", data0&0x3ff );
	data += strlen(data);
   sprintf (data, "u_thres1 = %d;\r\n", (data0>>10)&0x3ff );
	data += strlen(data);
   sprintf (data, "v_thres1 = %d;\r\n", (data0>>20)&0x3ff );
	data += strlen(data);
   
   data0 = Gem_read ( GEM_DENOISE_BASE+0x0c) ;
   sprintf (data, "y_thres2 = %d;\r\n", data0&0x3ff );
	data += strlen(data);
   sprintf (data, "u_thres2 = %d;\r\n", (data0>>10)&0x3ff );
	data += strlen(data);
   sprintf (data, "v_thres2 = %d;\r\n", (data0>>20)&0x3ff );
	data += strlen(data);
   
	sprintf (data, "noise0 :\r\n");
	data += strlen(data);
   for(i=0 ; i<=16 ; i++)
   {
      sprintf (data, "%5d   ", p_denoise.noise0[i] );
		data += strlen(data);
      if( (i%8) == 7 )
		{
			sprintf (data, "\r\n");
			data += strlen(data);
		}
   }
	sprintf (data, "\r\nnoise1 :\r\n");
	data += strlen(data);
   for(i=0 ; i<=16 ; i++)
   {
      sprintf (data, "%5d   ", p_denoise.noise1[i] );
		data += strlen(data);
      if((i%8) == 7 )
      {
			sprintf (data, "\r\n");
			data += strlen(data);
		}
   }
	sprintf (data, "\r\n");
	data += strlen(data);
   
   //awb finish
	sprintf (data, "\r\nawb:\r\n");
	data += strlen(data);
   data0 = Gem_read ( GEM_AWB0_BASE+0x00 ) ;
   sprintf (data, "awb enable:%s\r\n", (data0&0x1)?"enable":"disable");
	data += strlen(data);
   sprintf (data, "awb mode:%d //0 调试 1 第一种算法 2 第二种算法\r\n", (data0>>1)&0x3  );
	data += strlen(data);
   sprintf (data, "awb manual:%d //0=自动白平衡  1=手动白平衡\r\n", (data0>>3)&0x1  );
	data += strlen(data);
   
	sprintf (data, "awb weight:\r\n");
	data += strlen(data);
   for (i = 0; i < 3 ; i++ )
   {
      for(j=0; j < 3 ; j++ )
      {
         sprintf (data, "[%d][%d]:%d  ",i,j ,p_awb.weight[i][j] & 0x0F );
			data += strlen(data);
      }
		sprintf (data, "\r\n");
		data += strlen(data);
   }
   sprintf (data, "awb black:%d \r\n", (data0>>8)&0xff  );
	data += strlen(data);
   sprintf (data, "awb white:%d \r\n", (data0>>16)&0xffff  );
	data += strlen(data);
   
   data0 = Gem_read (GEM_AWB0_BASE+0x2c);
	// jitter 8 RW 白平衡色偏敏感度，有效范围0~255
   sprintf (data, "awb jitter:%d \r\n", (data0>>8) & 0xFF  );
	data += strlen(data);
   data1 = Gem_read (GEM_AWB0_BASE+0x04) ;
   data2 = Gem_read (GEM_AWB0_BASE+0x08) ;
   sprintf (data, "awb r2g_min:%d \r\n", data1&0xffff  );
	data += strlen(data);
   sprintf (data, "awb r2g_max:%d \r\n", data2&0xffff  );
	data += strlen(data);
   sprintf (data, "awb b2g_min:%d \r\n", data1>>16  );
	data += strlen(data);
   sprintf (data, "awb b2g_max:%d \r\n", data2>>16  );
	data += strlen(data);
   
   data1 = Gem_read (GEM_AWB0_BASE+0x0c) ;
   sprintf (data, "use_light[0]:%d r2g:%4d b2g:%4d\r\n",(data0>>0)&0x1,data1&0xffff,data1>>16 );
	data += strlen(data);
   data1 = Gem_read (GEM_AWB0_BASE+0x10) ;
   sprintf (data, "use_light[1]:%d r2g:%4d b2g:%4d\r\n",(data0>>1)&0x1,data1&0xffff,data1>>16 );
	data += strlen(data);
   data1 = Gem_read (GEM_AWB0_BASE+0x14) ;
   sprintf (data, "use_light[2]:%d r2g:%4d b2g:%4d\r\n",(data0>>2)&0x1,data1&0xffff,data1>>16 );
	data += strlen(data);
   data1 = Gem_read (GEM_AWB0_BASE+0x18) ;
   sprintf (data, "use_light[3]:%d r2g:%4d b2g:%4d\r\n",(data0>>3)&0x1,data1&0xffff,data1>>16 );
	data += strlen(data);
   data1 = Gem_read (GEM_AWB0_BASE+0x1c) ;
   sprintf (data, "use_light[4]:%d r2g:%4d b2g:%4d\r\n",(data0>>4)&0x1,data1&0xffff,data1>>16 );
	data += strlen(data);
   data1 = Gem_read (GEM_AWB0_BASE+0x20) ;
   sprintf (data, "use_light[5]:%d r2g:%4d b2g:%4d\r\n",(data0>>5)&0x1,data1&0xffff,data1>>16 );
	data += strlen(data);
   data1 = Gem_read (GEM_AWB0_BASE+0x24) ;
   sprintf (data, "use_light[6]:%d r2g:%4d b2g:%4d\r\n",(data0>>6)&0x1,data1&0xffff,data1>>16 );
	data += strlen(data);
   data1 = Gem_read (GEM_AWB0_BASE+0x28) ;
   sprintf (data, "use_light[7]:%d r2g:%4d b2g:%4d\r\n",(data0>>7)&0x1,data1&0xffff,data1>>16 );
	data += strlen(data);
   
   data0 = Gem_read (GEM_AWB0_BASE+0x30) ;
   sprintf (data, "gain_g2r:%4d  gain_g2b:%4d\r\n",data0&0xffff , data0>>16 );
	data += strlen(data);
	
	// gain_r2g gain_b2g
	data0 = Gem_read (GEM_AWB1_BASE+4);
   sprintf (data, "gain_r2g:%4d // R color temperature approximate value\r\n", data0&0xffff);
	data += strlen(data);
   sprintf (data, "gain_b2g:%4d // B color temperature approximate value\r\n", data0>>16 );
	data += strlen(data);
	
   
   // fesp finish
	sprintf (data, "\r\nfesp:\r\n");
	data += strlen(data);
   data0 = Gem_read (GEM_LENS_BASE+0x00) ;
   sprintf (data, "lensshade: %s scale:%d \r\n", (data0&0x1)?"enable":"disable" ,(data0>>1) );
	data += strlen(data);
   data0 = Gem_read (GEM_LENS_BASE+0x04) ;
   sprintf (data, "rcenterRx:%d rcenterRy:%d\r\n", data0&0xffff ,data0>>16 );
	data += strlen(data);
   data0 = Gem_read (GEM_LENS_BASE+0x08) ;
   sprintf (data, "rcenterGx:%d rcenterGy:%d\r\n", data0&0xffff ,data0>>16 );
	data += strlen(data);
   data0 = Gem_read (GEM_LENS_BASE+0x0c) ;
   sprintf (data, "rcenterBx:%d rcenterBy:%d\r\n", data0&0xffff ,data0>>16 );
	data += strlen(data);
	
	// lsc offset
	data0 = Gem_read (GEM_LENS_LSCOFST_BASE+0x00) ;
	sprintf (data, "lscofst:%d\r\n", data0 & 0xFFFF);
   data += strlen(data);
   
	sprintf (data, "lensshade Coeff:\r\n");
	data += strlen(data);
   for (i = 0; i < 3; i++)
   {
      for(j=0;j<65;j++ )
      {
         sprintf (data, "%5d ", p_fesp.Lensshade.coef[i*65+j] );
			data += strlen(data);
         if((j%8) == 7 )
         {
				sprintf (data, "\r\n");
				data += strlen(data);
			}
      }
		sprintf (data, "\r\n");
		data += strlen(data);
   }
   data0 = Gem_read (GEM_FIXPATT_BASE+0x00) ;
   sprintf (data, "\r\nFixpatt enable:%s  \r\n", (data0&0x1)?"enable":"disable" );
	data += strlen(data);
   sprintf (data, "Fixpatt mode:%d  \r\n", (data0>>1) & 0x01 );
	data += strlen(data);
   
   data0 = Gem_read (GEM_FIXPATT_BASE+0x04) ;
   sprintf (data, "Fixpatt rBlacklevel: %d  \r\n", data0&0xffff );
	data += strlen(data);
   sprintf (data, "Fixpatt grBlacklevel:%d  \r\n", data0>>16 );
	data += strlen(data);
   data0 = Gem_read (GEM_FIXPATT_BASE+0x08) ;
   sprintf (data, "Fixpatt gbBlacklevel:%d  \r\n", data0&0xffff );
	data += strlen(data);
   sprintf (data, "Fixpatt bBlacklevel: %d  \r\n", data0>>16  );
	data += strlen(data);
	sprintf (data, "Fixpatt.profile:\r\n");
	data += strlen(data);
   for(i=0 ; i<=16 ; i++)
   {
      sprintf (data, "%5d   ", p_fesp.Fixpatt.profile[i] );
		data += strlen(data);
      if( (i%8) == 7 )
      {
			sprintf (data, "\r\n");
			data += strlen(data);
		}
   }
	sprintf (data, "\r\n");
	data += strlen(data);
   
   data0 = Gem_read (GEM_BADPIX_BASE+0x00) ;
   sprintf (data, "\r\nBadpix enable:%s  \r\n", (data0&0x1)?"enable":"disable" );
	data += strlen(data);
   sprintf (data, "Badpix.mode:%d  \r\n", (data0>>1) & 0x01 );
	data += strlen(data);
   data0 = Gem_read (GEM_BADPIX_BASE+0x04) ;
   sprintf (data, "Badpix.thresh:%d  \r\n", data0&0xff );
	data += strlen(data);
	sprintf (data, "Badpix.profile:\r\n");
	data += strlen(data);
   for(i=0 ; i<16 ; i++)
   {
      sprintf (data, "%5d  ", p_fesp.Badpix.profile[i] );
		data += strlen(data);
      if( (i%8) == 7 )
      {
			sprintf (data, "\r\n");
			data += strlen(data);
		}
   }
   
	sprintf (data, "\r\n\r\n");
	data += strlen(data);
   data0 = Gem_read (GEM_CROSS_BASE+0x00) ;
   sprintf (data, "Crosstalk enable:%s  \r\n", (data0&0x1)?"enable":"disable" );
	data += strlen(data);
   sprintf (data, "p_fesp->Crosstalk.mode:%d  \r\n",      (data0 >> 1  ) & 0x03 );	// bit1-bit2, crosstalk mode
	data += strlen(data);
	sprintf (data, "p_fesp->Crosstalk.snsCgf:%d  \r\n",    (data0 >> 3  ) & 0x03 );	// bit3-bit4, snsCgf
	data += strlen(data);
	sprintf (data, "p_fesp->Crosstalk.thres0cgf:%d  \r\n", (data0 >> 16 ) & 0xFFFF );	// bit16-bit31, thres0cgf
	data += strlen(data);
   data0 = Gem_read (GEM_CROSS_BASE+0x04) ;
   sprintf (data, "p_fesp->Crosstalk.thresh:%d  // thres2cgf \r\n", data0 & 0xFFFF );
	data += strlen(data);
   sprintf (data, "p_fesp->Crosstalk.thres1cgf:%d  // thres1cgf\r\n", (data0 >> 16) & 0xFFFF );
	data += strlen(data);
   
	sprintf (data, "p_fesp->Crosstalk.profile:\r\n");
	data += strlen(data);
   for(i=0 ; i<=16 ; i++)
   {
      sprintf (data, "%5d  ", p_fesp.Crosstalk.profile[i] );
		data += strlen(data);
      if( (i%8) == 7 )
      {
			sprintf (data, "\r\n");
			data += strlen(data);
		}
   }
   
   // enhance finish
	sprintf (data, "\r\n\r\nenhance :\r\n");
	data += strlen(data);
	
   data0 = Gem_read (GEM_ENHANCE_BASE+0x00) ;
   sprintf (data, "sharp enable:%s  \r\n", (data0&0x1)?"enable":"disable" );
	data += strlen(data);
   sprintf (data, "sharp mode:%d  \r\n", (data0>>1)&0x1 );		// bit1 (0: heavy  1: soft)
	data += strlen(data);
   sprintf (data, "sharp coring:%d  \r\n", (data0>>5)&0x7 );		// bit5-bit7 sharpness coring sel (0-7)
	data += strlen(data);
   sprintf (data, "sharp strength:%d  \r\n", (data0>>8)&0xff );		// bit8-bit15 sharpness strenght (0-255)
	data += strlen(data);
   sprintf (data, "sharp gainmax:%d  \r\n", (data0>>16) & 0x3FF );	// bit16-bit25 sharpness gaminmax (0-1023)
	data += strlen(data);
   
   data0 = Gem_read (GEM_ENHANCE_BASE+0x04) ;
   sprintf (data, "\r\nbcst enable:%s  \r\n", ((data0>>31) & 1)?"enable":"disable" );
	data += strlen(data);
	// bcst bright   10   RW   明亮度，有效范围-256~255
	// bit0-bit9      bcst_bright    bright value (-256,255)
   sprintf (data, "bcst bright:%d  \r\n", ((int)((data0&0x1ff) << 23)) >> 23 );
	data += strlen(data);
   sprintf (data, "bcst offset0:%d  // 0~255  bright offset\r\n", (data0>>10)&0x3ff );	// bit10-bit19
	data += strlen(data);
   sprintf (data, "bcst offset1:%d  // 0~255  saturation offset\r\n", (data0>>20)&0x3ff );	// bit20-bit29
	data += strlen(data);
   
   data0 = Gem_read (GEM_ENHANCE_BASE+0x08) ;
   sprintf (data, "bcst contrast:%d  \r\n", data0&0x7ff );	// bit0-bit10	contrast  0-2043
	data += strlen(data);
   sprintf (data, "bcst satuation:%d  \r\n", (data0>>11)&0x7ff );	// bit11-bit21 satuation 0-2043
	data += strlen(data);
   sprintf (data, "bcst hue:%d  \r\n", (signed char)((data0>>24)&0xff) );	// bit24-bit31 hue       -128-127
	data += strlen(data);


   //eris
	sprintf (data, "\r\neris :\r\n");
	data += strlen(data);
   data0 = Gem_read (GEM_ERIS_BASE+0x00) ;
   sprintf (data, "p_eris->enable:%s  \r\n", (data0&0x1)?"enable":"disable" );
	data += strlen(data);
   sprintf (data, "p_eris->manual:%d  \r\n", (data0>>1)&0x1 );
	data += strlen(data);
	
	
   sprintf (data, "p_eris->varEris:%d  \r\n", (data0>>5) & 3 );
	data += strlen(data);
   sprintf (data, "p_eris->dfsEris:%d  // %s\r\n", (data0>>7) & 1,  ((data0>>7) & 1) ? "use resolt table" : "use resoli^spacev index" );
	data += strlen(data);
   sprintf (data, "p_eris->resoli:%d  \r\n", (data0>>8)&0xff );
	data += strlen(data);
   sprintf (data, "p_eris->spacev:%d  \r\n", (data0>>16)&0xff );
	data += strlen(data);
   sprintf (data, "p_eris->target:%d  \r\n", (data0>>24)&0xff );
	data += strlen(data);
   
   data0 = Gem_read (GEM_ERIS_BASE+0x04) ;
   sprintf (data, "p_eris->black:%d  \r\n", data0&0xffff );
	data += strlen(data);
   sprintf (data, "p_eris->white:%d  \r\n", data0>>16 );
	data += strlen(data);
   data0 = Gem_read (GEM_ERIS_BASE+0x08) ;
   sprintf (data, "p_eris->gain_max:%d  \r\n", data0&0xffff );
	data += strlen(data);
   sprintf (data, "p_eris->gain_min:%d  \r\n", data0>>16 );
	data += strlen(data);
   data0 = Gem_read (GEM_ERIS_BASE+0x0c) ;
   sprintf (data, "p_eris->cont_max:%d  \r\n", data0&0xffff );
	data += strlen(data);
   sprintf (data, "p_eris->cont_min:%d  \r\n", data0>>16   );
	data += strlen(data);
   data0 = Gem_read (GEM_ERIS_BASE+0x10) ;
   sprintf (data, "p_eris->gain_man:%d  \r\n", data0&0xffff );
	data += strlen(data);
   sprintf (data, "p_eris->cont_man:%d  \r\n", data0>>16 );
	data += strlen(data);
	

	sprintf (data, "p_eris->resolt :\r\n");
	data += strlen(data);
   for (i = 0; i < 33; i++)
   {
      sprintf (data, "%4d ",p_eris.resolt[i] );
		data += strlen(data);
      if( (i%8) == 7 )
		{
			sprintf (data, "\r\n");
			data += strlen(data);
		}
   }
	sprintf (data, "\r\np_eris->colort :\r\n");
	data += strlen(data);
   for (i = 0; i < 33; i++)
   {
      sprintf (data, "%4d ", p_eris.colort[i] );
		data += strlen(data);
      if( (i%8) == 7 )
      {
			sprintf (data, "\r\n");
			data += strlen(data);
		}
   }
	
	sprintf (data, "\r\n\r\n//ERIS_INFO:\r\n");
	data += strlen(data);
	
	// eris直方图阈值分段设置
	sprintf (data, "ErisHistBand:\r\n");
	data += strlen(data);
	
   data0 = Gem_read (GEM_ERIS_HIST_BASE+0x00);
	sprintf (data, "[0]:%d ", data0&0xff);
	data += strlen(data);
	
   sprintf (data, "[1]:%d ", (data0>>8)&0xff);
	data += strlen(data);
	
   sprintf (data, "[2]:%d ", (data0>>16)&0xff);
	data += strlen(data);
	
   sprintf (data, "[3]:%d\r\n", (data0>>24)&0xff);
	data += strlen(data);
	
	// eris亮度均值
	data0 = Gem_read (GEM_ERIS_INFO_BASE+0x14);
	if(isp_get_video_width() && isp_get_video_height())
		data0 = data0 / (isp_get_video_width() * isp_get_video_height());
	else
		data0 = 0;
	sprintf (data, "Erislum  = 0x%08x\r\n", data0);
	data += strlen(data);
	
	// eris直方图统计值
	unsigned int eris_hist[5];
	unsigned int eris_sum = 0;
	eris_hist[0] = Gem_read (GEM_ERIS_INFO_BASE+0x00);
	eris_hist[1] = Gem_read (GEM_ERIS_INFO_BASE+0x04);
	eris_hist[2] = Gem_read (GEM_ERIS_INFO_BASE+0x08);
	eris_hist[3] = Gem_read (GEM_ERIS_INFO_BASE+0x0c);
	eris_hist[4] = Gem_read (GEM_ERIS_INFO_BASE+0x10);
	
	sprintf (data, "//%d %d %d %d %d\r\n", eris_hist[0], eris_hist[1], eris_hist[2], eris_hist[3], eris_hist[4]);
	data += strlen(data);
	
	
	eris_sum = eris_hist[0] + eris_hist[1] + eris_hist[2] + eris_hist[3] + eris_hist[4];
	sprintf (data, "ErisHistGram :\r\n");
	data += strlen(data);
   sprintf (data, "[0] = %4d;\r\n", eris_hist[0] / (eris_sum/4096) );
	data += strlen(data);
   sprintf (data, "[1] = %4d;\r\n", eris_hist[1] / (eris_sum/4096));
	data += strlen(data);
   sprintf (data, "[2] = %4d;\r\n", eris_hist[2] / (eris_sum/4096));
	data += strlen(data);
   sprintf (data, "[3] = %4d;\r\n", eris_hist[3] / (eris_sum/4096));
	data += strlen(data);
   sprintf (data, "[4] = %4d;\r\n", eris_hist[4] / (eris_sum/4096));
	data += strlen(data);	
	sprintf (data, "\r\n");
	data += strlen(data);
  
   //colors
	sprintf (data, "\r\n\r\ncolors :\r\n");
	data += strlen(data);
   data0 = Gem_read (GEM_COLORS_BASE+0x00) ;
   sprintf (data, "p_colors->colorm.enable:%s  \r\n", data0?"enable":"disable" );
	data += strlen(data);
	sprintf (data, "colors matrixcoeff:\r\n");
	data += strlen(data);
   
   data0 = Gem_read (GEM_COLORS_BASE+0x04) ;
   sprintf (data, "[0][0]:%8d  ", (short)(data0&0xffff) );
	data += strlen(data);
   sprintf (data, "[0][1]:%8d  ",(short)(data0>>16) );
	data += strlen(data);
   
   data0 = Gem_read (GEM_COLORS_BASE+0x08) ;
   sprintf (data, "[0][2]:%8d  \r\n",(short)(data0&0xffff) );
	data += strlen(data);
   sprintf (data, "[1][0]:%8d  ",(short)(data0>>16) );
	data += strlen(data);
   
   data0 = Gem_read (GEM_COLORS_BASE+0x0c) ;
   sprintf (data, "[1][1]:%8d  ",(short)(data0&0xffff) );
	data += strlen(data);
   sprintf (data, "[1][2]:%8d  \r\n",(short)(data0>>16) );
	data += strlen(data);
   
   data0 = Gem_read (GEM_COLORS_BASE+0x10) ;
   sprintf (data, "[2][0]:%8d  ",(short)(data0&0xffff) );
	data += strlen(data);
   sprintf (data, "[2][1]:%8d  ",(short)(data0>>16) );
	data += strlen(data);
   
   data0 = Gem_read (GEM_COLORS_BASE+0x14) ;
   sprintf (data, "[2][2]:%8d  ",(short)(data0&0xffff) );
	data += strlen(data);
   
	sprintf (data, "\r\n");
	data += strlen(data);

   data0 = Gem_read (GEM_COLORS_BASE+0x18) ;
   sprintf (data, "\r\np_colors->gamma.enable:%s  \r\n", data0?"enable":"disable" );
	data += strlen(data);
   
	sprintf (data, "p_colors->gamma.gamma_lut: \r\n");
	data += strlen(data);
   for (i = 0; i < 65; i++)
   {
      sprintf (data, "%8d   ",  p_colors.gamma.gamma_lut[i]) ;
		data += strlen(data);
      if( (i%8) == 7 )
      {
			sprintf (data, "\r\n");
			data += strlen(data);
		}
   }
	sprintf (data, "\r\n");
	data += strlen(data);
   
   sprintf (data, "\r\np_colors->rgb2ypbpr_type:%d  \r\n", p_colors.rgb2ypbpr_type );
	data += strlen(data);
	sprintf (data, "p_colors->rgb2yuv.rgbcoeff:\r\n");
	data += strlen(data);
	
   if(p_colors.rgb2ypbpr_type == HDTV_type_0255 )
   {
		sprintf (data, "HDTV_type_0255\r\n");
		data += strlen(data);
	}
   else if(p_colors.rgb2ypbpr_type == HDTV_type_16235 )
   {
		sprintf (data, "HDTV_type_16235\r\n");
		data += strlen(data);
	}
   else if(p_colors.rgb2ypbpr_type == SDTV_type_0255 )
   {
		sprintf (data, "SDTV_type_0255\r\n");
		data += strlen(data);
	}
   else if(p_colors.rgb2ypbpr_type == SDTV_type_16235 )
   {
		sprintf (data, "SDTV_type_16235\r\n");
		data += strlen(data);
	}
   data0 = Gem_read (GEM_COLORS_BASE+0x20) ;
   sprintf (data, "[0][0]%5d  [0][1]%5d  ", (short)(data0&0xffff), (short)(data0>>16) );
	data += strlen(data);
   data0 = Gem_read (GEM_COLORS_BASE+0x24) ;
   sprintf (data, "[0][2]%5d  \r\n[1][0]%5d  ", (short)(data0&0xffff), (short)(data0>>16) );
	data += strlen(data);
   data0 = Gem_read (GEM_COLORS_BASE+0x28) ;
   sprintf (data, "[1][1]%5d  [1][2]%5d\r\n", (short)(data0&0xffff), (short)(data0>>16) );
	data += strlen(data);
   data0 = Gem_read (GEM_COLORS_BASE+0x2c) ;
   sprintf (data, "[2][0]%5d  [2][1]%5d  ", (short)(data0&0xffff), (short)(data0>>16) );
	data += strlen(data);
   data0 = Gem_read (GEM_COLORS_BASE+0x30) ;
   sprintf (data, "[2][2]%5d  \r\n[3][0]%5d  ", (short)(data0&0xffff), (short)(data0>>16) );
	data += strlen(data);
   data0 = Gem_read (GEM_COLORS_BASE+0x34) ;
   sprintf (data, "[3][1]%5d  [3][2]%5d\r\n", (short)(data0&0xffff), (short)(data0>>16) );
	data += strlen(data);
	
	sprintf (data, "\r\n// Demosaic \r\n");
	data += strlen(data);
	data0 = Gem_read ( GEM_DEMOSAIC_BASE+0x00 ) ;
	sprintf (data, "demosaic_coff_00_07 : %d	// 8bit, bit7-0 demk1 \n", data0 & 0xFF);
	data += strlen(data);
	sprintf (data, "demk : %d	// bit19-8 \n", (data0 >> 8) & 0xFFF);
	data += strlen(data);
	
	sprintf (data, "demosaic_coff_20_27 : %d  // 8bit \n", (data0 >> 20) & 0xFF);
	data += strlen(data);
	sprintf (data, "demosaic_mode : %d	// demmethod\n", (data0 >> 31) & 1);
	data += strlen(data);
	sprintf (data, "// Demosaic Horz Control\r\n");
	data += strlen(data);
	data0 = Gem_read ( GEM_DEMOSAIC_BASE+0x04 ) ;
	sprintf (data, "demosaic_horz_thread : %d	// 12bit, bit11-0 demDofst \n", data0 & 0xFFF);
	data += strlen(data);
	sprintf (data, "demDhv_ofst : %d	// bit23-12\n", (data0 >> 12) & 0xFFF);
	data += strlen(data);
	
   
   // sys
	sprintf (data, "\r\nsys: \r\n");
	data += strlen(data);
   data0 = Gem_read ( GEM_SYS_BASE+0x00 ) ;
   sprintf (data, "ckpolar :%d  \r\nvcpolar:%d \r\nhcpolar:%d \r\nvmanSftenable:%d \r\nvmskenable:%d //(自动丢帧) \r\nframeratei:%d \r\nframerateo:%d\r\n", 
                   (data0>>1)&0x1, (data0>>2)&0x1, (data0>>3)&0x1 ,(data0>>4)&0x1,(data0>>5)&0x1, (data0>>8)&0xff ,(data0>>16)&0xff);
	data += strlen(data);
	// 输出帧序列控制
   data0 = Gem_read ( GEM_VIFRASE_BASE + 0x00 ) ;	// vifrasel0
   sprintf (data, "//vifrasel0 : 0x%08x  \r\n", data0 );
	data += strlen(data);
   data0 = Gem_read ( GEM_VIFRASE_BASE + 0x04 ) ;	// vifrasel1
   sprintf (data, "//vifrasel1 : 0x%08x  \r\n", data0 );
	data += strlen(data);
	
   data0 = Gem_read ( GEM_SYS_BASE+0x04 ) ;
   sprintf (data, "imagewidth:%d  \r\nimageheight:%d\r\n", data0&0xffff, data0>>16  );
	data += strlen(data);
   
   data0 = Gem_read ( GEM_SYS_BASE+0x08 ) ;
   sprintf (data, "imagehblank:%d  \r\n", data0&0xffff  );
	data += strlen(data);
	
	
	// resizebit
	// Raw data resize (0:normal 1:cut 1bit 2: cut 2 bit 3: cut 3 bit)
	sprintf (data, "// resizebit  Raw data resize (0:normal 1:cut 1bit 2: cut 2 bit 3: cut 3 bit)\r\n");
	data += strlen(data);	
   sprintf (data, "resizebit: %d  \r\n", (data0>>16)&0x3  );
	data += strlen(data);
   
	sprintf (data, "// sensor bit: 0:8bit 1:10bit 2:12bit 3:14bit  \r\n");
	data += strlen(data);
   sprintf (data, "sensor bit: %d  \r\n", (data0>>18)&0x3  );
	data += strlen(data);
   
	sprintf (data, "\r\n");
	data += strlen(data);
	sprintf (data, "// bayer mode: 0:RGGB 1:GRBG 2:BGGR 3:GBRG  ov9712=2  pp1210 720P=1 1080P=0\r\n");
	data += strlen(data);
	
   sprintf (data, "bayer mode: %d  \r\n", (data0>>20)&0x3  );
	data += strlen(data);
	// sonyif
   sprintf (data, "sonyif: %d  \r\n", (data0>>22)&0x1  );
	data += strlen(data);
	
   
	sprintf (data, "\r\n");
	data += strlen(data);
   data0 = Gem_read ( GEM_SYS_BASE+0x0c ) ;
   sprintf (data, "vchkIntenable :%s  // 场开始\r\n", (data0&0x1)?"enable":"disable" );
	data += strlen(data);
   sprintf (data, "pabtIntenable :%s  // 点异常\r\n", ((data0>>1)&0x1)?"enable":"disable" );
	data += strlen(data);
   sprintf (data, "fendIntenable :%s  // 帧完成\r\n", ((data0>>2)&0x1)?"enable":"disable" );
	data += strlen(data);
   sprintf (data, "fabtIntenable :%s  // 地址异常\r\n", ((data0>>3)&0x1)?"enable":"disable" );
	data += strlen(data);
   sprintf (data, "babtIntenable :%s  // 总线异常\r\n", ((data0>>4)&0x1)?"enable":"disable" );
	data += strlen(data);
   sprintf (data, "ffiqIntenable :%s  // 快中断\r\n", ((data0>>5)&0x1)?"enable":"disable" );
	data += strlen(data);
   sprintf (data, "pendIntenable :%s  // 中止 ，如果一帧未完成，则完成该帧\r\n", ((data0>>6)&0x1)?"enable":"disable" );
	
	// 曝光统计完成中断
	data += strlen(data);
   sprintf (data, "infoIntenable :%s  // 曝光统计完成中断\r\n", ((data0>>7)&0x1)?"enable":"disable" );
	
	data += strlen(data);
   sprintf (data, "ffiqIntdelay :%d  // 快中断 参数*16 = ffiqIntdelay*16 行\r\n", (data0>>24)&0xff );
	data += strlen(data);
   
   data0 = Gem_read ( GEM_SYS_BASE+0x10 ) ;
   sprintf (data, "zonestridex:%d  \r\nzonestridey:%d\r\n", data0&0xffff  , data0>>16  );
	data += strlen(data);
   
   data0 = Gem_read ( GEM_SYS_BASE+0x14 ) ;
   sprintf (data, "debugmode:%d  \r\n", data0&0x1  );
	data += strlen(data);
   sprintf (data, "testenable:%d  \r\n", (data0>>1)&0x1  );
	data += strlen(data);
   sprintf (data, "rawmenable:%d  \r\n", (data0>>2)&0x1  );
	data += strlen(data);
   sprintf (data, "yuvenable:%d  \r\n", (data0>>3)&0x1  );
	data += strlen(data);
   sprintf (data, "refenable:%d  \r\n", (data0>>4)&0x1  );
	data += strlen(data);
   
	sprintf (data, "\r\n");
	data += strlen(data);
	sprintf (data, "// yuvformat : 0：y_uv420 1:y_uv422 2:yuv420 3:yuv422 \r\n");
	data += strlen(data);
   sprintf (data, "yuvformat : %d  \r\n", (data0>>5)&0x3  );
	data += strlen(data);
   sprintf (data, "dmalock :%d  \r\n", (data0>>7)&0x3  );
	data += strlen(data);
   sprintf (data, "hstride :%d  \r\n", data0>>16  );
	data += strlen(data);
   
	sprintf (data, "\r\n");
	data += strlen(data);
   data0 = Gem_read ( GEM_SYS_BASE+0x18 ) ;
   sprintf (data, "refaddr :0x%08x  \r\n", data0  );
	data += strlen(data);
   
   data0 = Gem_read ( GEM_SYS_BASE+0x1c ) ;
   sprintf (data, "rawaddr0 :0x%08x  \r\n", data0  );
	data += strlen(data);
   data0 = Gem_read ( GEM_SYS_BASE+0x20 ) ;
   sprintf (data, "rawaddr1 :0x%08x  \r\n", data0  );
	data += strlen(data);
   data0 = Gem_read ( GEM_SYS_BASE+0x24 ) ;
   sprintf (data, "rawaddr2 :0x%08x  \r\n", data0  );
	data += strlen(data);
   data0 = Gem_read ( GEM_SYS_BASE+0x28 ) ;
   sprintf (data, "rawaddr3 :0x%08x  \r\n", data0  );
	data += strlen(data);
   
   data0 = Gem_read ( GEM_SYS_BASE+0x2c ) ;
   sprintf (data, "yaddr0 :0x%08x  \r\n", data0  );
	data += strlen(data);
   data0 = Gem_read ( GEM_SYS_BASE+0x30 ) ;
   sprintf (data, "yaddr1 :0x%08x  \r\n", data0  );
	data += strlen(data);
   data0 = Gem_read ( GEM_SYS_BASE+0x34 ) ;
   sprintf (data, "yaddr2 :0x%08x  \r\n", data0  );
	data += strlen(data);
   data0 = Gem_read ( GEM_SYS_BASE+0x38 ) ;
   sprintf (data, "yaddr3 :0x%08x  \r\n", data0  );
	data += strlen(data);
   
   data0 = Gem_read ( GEM_SYS_BASE+0x3c ) ;
   sprintf (data, "uaddr0 :0x%08x  \r\n", data0  );
	data += strlen(data);
   data0 = Gem_read ( GEM_SYS_BASE+0x40 ) ;
   sprintf (data, "uaddr1 :0x%08x  \r\n", data0  );
	data += strlen(data);
   data0 = Gem_read ( GEM_SYS_BASE+0x44 ) ;
   sprintf (data, "uaddr2 :0x%08x  \r\n", data0  );
	data += strlen(data);
   data0 = Gem_read ( GEM_SYS_BASE+0x48 ) ;
   sprintf (data, "uaddr3 :0x%08x  \r\n", data0  );
	data += strlen(data);
   
   data0 = Gem_read ( GEM_SYS_BASE+0x4c ) ;
   sprintf (data, "vaddr0 :0x%08x  \r\n", data0  );
	data += strlen(data);
   data0 = Gem_read ( GEM_SYS_BASE+0x50 ) ;
   sprintf (data, "vaddr1 :0x%08x  \r\n", data0  );
	data += strlen(data);
   data0 = Gem_read ( GEM_SYS_BASE+0x54 ) ;
   sprintf (data, "vaddr2 :0x%08x  \r\n", data0  );
	data += strlen(data);
   data0 = Gem_read ( GEM_SYS_BASE+0x58 ) ;
   sprintf (data, "vaddr3 :0x%08x  \r\n", data0  );
	data += strlen(data);
	
	// sonysac1
   data0 = Gem_read ( GEM_SYS_BASE+0x60 ) ;
   sprintf (data, "sonysac1 :0x%08x  \r\n", data0 >> 16  );
	data += strlen(data);
   sprintf (data, "sonysac2 :0x%08x  \r\n", data0 & 0xFFFF  );
	data += strlen(data);
   data0 = Gem_read ( GEM_SYS_BASE+0x64 ) ;
   sprintf (data, "sonysac3 :0x%08x  \r\n", data0 >> 16  );
	data += strlen(data);
   sprintf (data, "sonysac4 :0x%08x  \r\n", data0 & 0xFFFF  );
	data += strlen(data);
   
   data0 = Gem_read ( GEM_SYS_BASE+0x00 ) ;
   sprintf (data, "ispenbale :%s  \r\n", (data0&0x1)?"enable":"disable"  );
   data += strlen(data);
	
	
	writestring (buffer, fp);
	kernel_free (buffer);
}

int check_device_and_create_directory( char *filepath ) 
{
   int ret = -1;
   if( FS_IsVolumeMounted("mmc:") == 0 )
   {
      XM_printf("mmc:.Not intert!\n");
      return -1;
   }
   if( FS_GetFreeSpace("mmc:") < 0x4000000)
   {
      XM_printf("mmc: Not enough space !\n");
      return -1;
   }
   if( check_and_create_directory( filepath ) == -1 )
      return -1;
   return 0;
}

#include "xm_core.h"
#include "xm_h264_codec.h"
void isp_write_file_record(char *addpath  ,unsigned int len )
{
	unsigned int old_mode;
   unsigned int filemount =1;
   char path[] = "mmc:\\MINITEST\\ISP\\";
   char filepath[256]  ={0} ;
   char addpaths[32]   ={0};
   char filetimestr[8]={0};
	int re_start_record = 0;
	
	if(XMSYS_H264CodecGetMode() == XMSYS_H264_CODEC_MODE_PLAY)
	{
		XM_printf ("isp_write_file_record faild, can't do in play mode\n");
		return;
	}
   
   U32 TimeStamp;
   FS_FILETIME FileTime;
	if(addpath)
	{
		FileTime.Year = c2i(addpath[0])*1000+c2i(addpath[1])*100+c2i(addpath[2])*10+c2i(addpath[3]);
		FileTime.Month = c2i(addpath[4])*10+c2i(addpath[5]);
		FileTime.Day = c2i(addpath[6])*10+c2i(addpath[7]);
		FileTime.Hour = c2i(addpath[9])*10+c2i(addpath[10]);
		FileTime.Minute = c2i(addpath[11])*10+c2i(addpath[12]);
		FileTime.Second = c2i(addpath[13])*10+c2i(addpath[14]);
		FS_FileTimeToTimeStamp (&FileTime, &TimeStamp);
 //  FS_SetFileTime("test.txt", TimeStamp);
	}
	else
	{
		XMSYSTEMTIME SystemTime;
		XM_GetLocalTime (&SystemTime);
		FileTime.Year = SystemTime.wYear;
		FileTime.Month = SystemTime.bMonth;
		FileTime.Day = SystemTime.bDay;
		FileTime.Hour = SystemTime.bHour;
		FileTime.Minute = SystemTime.bMinute;
		FileTime.Second = SystemTime.bSecond;
		FS_FileTimeToTimeStamp (&FileTime, &TimeStamp);
	}
   
   XM_printf("%d\n" , OS_GetTime32() );
   memset( filepath , 0 , 256 ) ;
   if( addpath )
   {
     memset(addpaths , 0 , 32 );
     memcpy(addpaths, addpath, len-1 );
     sprintf( filepath , "%s%s\\" , path , addpaths ) ;
     if( len>8 )
     {
        filemount = addpath[ len-1 ]  ;
        memset( filetimestr, 0 , 8 );
        strncpy( filetimestr, addpaths+9,6);
     }
     
   }
   else
	{
		strcpy( filepath ,  path  ) ;
		//sprintf (filetimestr, "%04d%02d%02d",   FileTime.Year, FileTime.Month, FileTime.Day);
		sprintf (filetimestr,   "%02d%02d%02d", FileTime.Hour, FileTime.Minute, FileTime.Second);
		
		filemount = 1;
		
	}
	
	old_mode = isp_get_work_mode ();
	if(isp_set_work_mode (ISP_WORK_MODE_RAW) < 0)
		return;
	
	OS_Delay (100);		// 等待3帧

   //stop isp 
   rISP_SYS &= ~1;
   //
   //OS_EnterRegion();
   isp_write_file( filepath , filetimestr, filemount ,TimeStamp );
   //OS_LeaveRegion();
		
		
#if ISP_3D_DENOISE_SUPPORT	
#else	
   //restart isp
	// 非3D模式
   rISP_SYS |= 1;
#endif
	
   //filenum ++;//用于文件名 
	isp_set_work_mode (old_mode);
	
#if ISP_3D_DENOISE_SUPPORT	
	// 关闭ISP, 彻底退出. 然后重新启动录像, 重新配置ISP, 这样ISP的3D不会出现异常.
	XMSYS_H264CodecRecorderStop ();
	
	XMSYS_H264CodecRecorderStart ();
#endif
}

void isp_write_multi_ae_file_record (unsigned int ae_comp, char *addpath  ,unsigned int len, unsigned int order )
{
   unsigned int filemount =1;
   char path[] = "mmc:\\MINITEST\\ISP\\";
   char filepath[256]  ={0} ;
   char addpaths[32]   ={0};
   char filetimestr[8]={0};
   
   U32 TimeStamp;
   FS_FILETIME FileTime;
   FileTime.Year = c2i(addpath[0])*1000+c2i(addpath[1])*100+c2i(addpath[2])*10+c2i(addpath[3]);
   FileTime.Month = c2i(addpath[4])*10+c2i(addpath[5]);
   FileTime.Day = c2i(addpath[6])*10+c2i(addpath[7]);
   FileTime.Hour = c2i(addpath[9])*10+c2i(addpath[10]);
   FileTime.Minute = c2i(addpath[11])*10+c2i(addpath[12]);
   FileTime.Second = c2i(addpath[13])*10+c2i(addpath[14]);
   FS_FileTimeToTimeStamp (&FileTime, &TimeStamp);
 //  FS_SetFileTime("test.txt", TimeStamp);
   
   XM_printf("%d\n" , OS_GetTime32() );
   memset( filepath , 0 , 256 ) ;
   if( addpath )
   {
     memset(addpaths , 0 , 32 );
     memcpy(addpaths, addpath, len-1 );
     sprintf( filepath , "%s%s\\" , path , addpaths ) ;
     if( len>8 )
     {
        filemount = addpath[ len-1 ]  ;
        memset( filetimestr, 0 , 8 );
        strncpy( filetimestr, addpaths+9,6);
     }
     
   }
   else
      strcpy( filepath ,  path  ) ;
	
	// 加入曝光补偿路径
	sprintf (filepath + strlen(filepath), "COMP_%d\\",  ae_comp);

   //stop isp 
   rISP_SYS &= ~1;
   //
   OS_EnterRegion();
  // isp_write_file( filepath , filetimestr, filemount ,TimeStamp );
	isp_multi_ae_write_file ( filepath , filetimestr, filemount ,TimeStamp, order );
   OS_LeaveRegion();
   //restart isp
   rISP_SYS |= 1;
   
   //filenum ++;//用于文件名 
}

