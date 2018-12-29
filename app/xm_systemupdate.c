
#include <stdio.h>
#include <string.h>
#include <io.h>
#include <malloc.h>
#include <windows.h>
#include "xm_systemupdate.h"
#include "crc16.h"


unsigned int UpdatePackagechecksum(unsigned int *s, unsigned int len )
{
	unsigned int count = 0;
	count = checksum_int(s,3);// 0 , 1, 2
	//check_sum不纳入校验和计算   // 3
	count+= checksum_int(s+4,len-4);// 4 , 5, 6....

	return count;
}


void write_updatapackage(XM_SYSTEMUPDATE_INFO info , FILE *fp)
{
	int size = sizeof(info);
	int length=info.length ; // 这个字段是32字节对齐的。
	if( size > length )
		printf("警告! 数据不正确!!\n");
	//首先填充 数据 结构体
	fwrite(&info,1,size,fp);
	size = length - size;
	// 补充满足32字节对齐 
	while(size--)
		fwrite("\0",1,sizeof(char),fp);
}

XM_SYSTEMUPDATE_INFO  Resinfo;

#ifdef _BUILD_UPDATE_PACKAGE_
int	XM_BuildSystemUpdatePackage (const char *binary_file_name,		// 二进制程序代码文件
											  const char *package_file_name,		// 升级包文件
											  const char *ini_file_name			// INI配置文件
																							//		GetPrivateProfileStringA
																							//		WritePrivateProfileStringA
																							// brand_id、brand_type、brand_version
																							//		及update_comment由INI配置文件指定
											  )
{
	int len=0;
	short int checksum=0;
//	int binary_offset;
	int binary_length;
	char *binstr;
//	unsigned int *start;
	char str[1024];
	char file_name[64];
	FILE *binfp,*upfp;
	XM_SYSTEMUPDATE_INFO info;

	memset(file_name , 0 , sizeof(char) ) ;
	sprintf(file_name,".\\");
	sprintf(file_name+2,"%s",ini_file_name);

	memset(&info,0,sizeof(info));
	info.id			= XMSYS_SYSTEMUPDATE_ID;
	info.version	= XMSYS_SYSTEMUPDATE_VERSION_1_0;
	info.length     = (sizeof(XM_SYSTEMUPDATE_INFO)+32)&0xffffffe0;

	//打开并读取bin文件
	if( (binfp = fopen(binary_file_name, "rb")) == NULL )
		return 0;
	binary_length = filelength( _fileno(binfp) ) ;
	binstr = (char*)malloc((binary_length+1)*sizeof(char)); 
	fread(binstr, 1 , binary_length , binfp) ;
	binstr[ binary_length ]=0;
	info.binary_crc = crc16_ccitt( binstr, binary_length );
	info.binary_length = binary_length;
	info.binary_offset = (sizeof( XM_SYSTEMUPDATE_INFO )+32)&0xffffffe0;//要求 32字节对齐。

	//
	info.brand_type = 0;
	info.brand_version = 0;
	GetPrivateProfileStringA (
			"ARK1960_INITIAL_SETTING",
			"BRAND_ID",
			"0",
			str,
			sizeof(str),
			file_name
			);
	memcpy(info.brand_id, str,strlen(str));

	GetPrivateProfileStringA (
			"ARK1960_INITIAL_SETTING",
			"BRAND_TYPE",
			"0",
			str,
			sizeof(str),
			file_name
			);
	info.brand_type= atoi(str);


	GetPrivateProfileStringA (
			"ARK1960_INITIAL_SETTING",
			"BRAND_VERSION",
			"0",
			str,
			sizeof(str),
			file_name
			);
	info.brand_version = atoi(str);

	GetPrivateProfileStringA (
			"ARK1960_INITIAL_SETTING",
			"UPDATE_COMMENT",
			"0",
			str,
			sizeof(str),
			file_name
			);
	memcpy(info.update_comment ,str,strlen(str) ) ;

	info.checksum = UpdatePackagechecksum((unsigned int *)(&info) , sizeof(XM_SYSTEMUPDATE_INFO)/4 ) ;

	//数据处理完毕，下面开始写入文件  package_file_name
	if( (upfp = fopen( package_file_name, "wb") ) == NULL )
	{

		printf("Please check if file read only!!\n");
		return 0;
	}
	//1632 结构体
	write_updatapackage(info, upfp );
	fwrite(binstr, 1 ,  binary_length, upfp );

	memcpy(&Resinfo,&info,sizeof(info));

	_fcloseall();
	return 1;
}
#endif


// 检查系统升级包(包括系统升级信息结构及二进制数据)(checksum, ID, 长度，CRC)
// 0  --> 升级包数据OK，并将升级信息结构填充到systemupdate_info
// -1 --> 升级包数据异常
int	XM_CheckSystemUpdatePackage (char *package_buffer, 
											  int package_length,
											  XM_SYSTEMUPDATE_INFO *systemupdate_info
											  )
{
	int ret=0;
	systemupdate_info = (XM_SYSTEMUPDATE_INFO *)package_buffer;

#if 1 // 这里对比生成数据包时候 留下的数据结构体 备份，检验是否相同 ，可以不使用 
	if(memcmp(systemupdate_info, &Resinfo,sizeof(XM_SYSTEMUPDATE_INFO) )==0)
		printf("data is correct .\n");
	else
	{	
		printf("data is error!\n");
		ret=1;
	}
#endif

	//数据结构体校验和 每个32bit数据 相加
	if(systemupdate_info->checksum == UpdatePackagechecksum((unsigned int *)(systemupdate_info) , sizeof(XM_SYSTEMUPDATE_INFO)/4 ) )
		printf("data check sum is correct.\n");
	else
	{	
		printf("data check sum is Error!\n");
		ret=1;
	}

	//二进制文件 CRC16校验和
	if( systemupdate_info->binary_crc == crc16_ccitt( package_buffer+systemupdate_info->binary_offset, systemupdate_info->binary_length ))
		printf("data binary_crc is correct.\n");
	else
	{	
		printf("data binary_crc is Error!\n");
		ret=1;
	}

	return ret;
}