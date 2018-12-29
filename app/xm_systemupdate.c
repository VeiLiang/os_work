
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
	//check_sum������У��ͼ���   // 3
	count+= checksum_int(s+4,len-4);// 4 , 5, 6....

	return count;
}


void write_updatapackage(XM_SYSTEMUPDATE_INFO info , FILE *fp)
{
	int size = sizeof(info);
	int length=info.length ; // ����ֶ���32�ֽڶ���ġ�
	if( size > length )
		printf("����! ���ݲ���ȷ!!\n");
	//������� ���� �ṹ��
	fwrite(&info,1,size,fp);
	size = length - size;
	// ��������32�ֽڶ��� 
	while(size--)
		fwrite("\0",1,sizeof(char),fp);
}

XM_SYSTEMUPDATE_INFO  Resinfo;

#ifdef _BUILD_UPDATE_PACKAGE_
int	XM_BuildSystemUpdatePackage (const char *binary_file_name,		// �����Ƴ�������ļ�
											  const char *package_file_name,		// �������ļ�
											  const char *ini_file_name			// INI�����ļ�
																							//		GetPrivateProfileStringA
																							//		WritePrivateProfileStringA
																							// brand_id��brand_type��brand_version
																							//		��update_comment��INI�����ļ�ָ��
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

	//�򿪲���ȡbin�ļ�
	if( (binfp = fopen(binary_file_name, "rb")) == NULL )
		return 0;
	binary_length = filelength( _fileno(binfp) ) ;
	binstr = (char*)malloc((binary_length+1)*sizeof(char)); 
	fread(binstr, 1 , binary_length , binfp) ;
	binstr[ binary_length ]=0;
	info.binary_crc = crc16_ccitt( binstr, binary_length );
	info.binary_length = binary_length;
	info.binary_offset = (sizeof( XM_SYSTEMUPDATE_INFO )+32)&0xffffffe0;//Ҫ�� 32�ֽڶ��롣

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

	//���ݴ�����ϣ����濪ʼд���ļ�  package_file_name
	if( (upfp = fopen( package_file_name, "wb") ) == NULL )
	{

		printf("Please check if file read only!!\n");
		return 0;
	}
	//1632 �ṹ��
	write_updatapackage(info, upfp );
	fwrite(binstr, 1 ,  binary_length, upfp );

	memcpy(&Resinfo,&info,sizeof(info));

	_fcloseall();
	return 1;
}
#endif


// ���ϵͳ������(����ϵͳ������Ϣ�ṹ������������)(checksum, ID, ���ȣ�CRC)
// 0  --> ����������OK������������Ϣ�ṹ��䵽systemupdate_info
// -1 --> �����������쳣
int	XM_CheckSystemUpdatePackage (char *package_buffer, 
											  int package_length,
											  XM_SYSTEMUPDATE_INFO *systemupdate_info
											  )
{
	int ret=0;
	systemupdate_info = (XM_SYSTEMUPDATE_INFO *)package_buffer;

#if 1 // ����Ա��������ݰ�ʱ�� ���µ����ݽṹ�� ���ݣ������Ƿ���ͬ �����Բ�ʹ�� 
	if(memcmp(systemupdate_info, &Resinfo,sizeof(XM_SYSTEMUPDATE_INFO) )==0)
		printf("data is correct .\n");
	else
	{	
		printf("data is error!\n");
		ret=1;
	}
#endif

	//���ݽṹ��У��� ÿ��32bit���� ���
	if(systemupdate_info->checksum == UpdatePackagechecksum((unsigned int *)(systemupdate_info) , sizeof(XM_SYSTEMUPDATE_INFO)/4 ) )
		printf("data check sum is correct.\n");
	else
	{	
		printf("data check sum is Error!\n");
		ret=1;
	}

	//�������ļ� CRC16У���
	if( systemupdate_info->binary_crc == crc16_ccitt( package_buffer+systemupdate_info->binary_offset, systemupdate_info->binary_length ))
		printf("data binary_crc is correct.\n");
	else
	{	
		printf("data binary_crc is Error!\n");
		ret=1;
	}

	return ret;
}