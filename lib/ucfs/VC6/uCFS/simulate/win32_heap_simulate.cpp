//****************************************************************************
//
//	Copyright (C) 2010~2014 ZhuoYongHong
//
//	Author	ZhuoYongHong
//
//	File name: win32_heap_simulate.c
//	  Win32 ȫ�ֶѷ���ӿ�ģ��
//
//	Revision history
//
//		2010.08.21	ZhuoYongHong Initial version
//
//****************************************************************************
#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdio.h>
#include <direct.h>
#include <afx.h>
#include <assert.h>

#include "..\..\include\xm_heap_malloc.h"
#include "..\..\include\xmprintf.h"

#ifdef HEAP_MALLOC_DEBUG


#define NODE_ADD_SIZE				100		//һ��������Ľ�����
#define CMS_NOADDR			0x00000001		//����ʾ��ַ
typedef void *HMEMORY;
typedef void *HCHECKMEM;
struct _checkmem
{
	char param [ 128 ] ;					//���Ӳ���
	int addr ;								//��ַ
	int size ;								//��С
} ;

struct checkmem
{
	struct _checkmem *data ;			//�������
	int total ;								//�ռ��С
	int count ;								//������
	char logfile [ 512 ] ;				//��־�ļ�
	int max ;								//�ڴ���������������ڴ�����ֵ
	int size ;								//��ǰʹ�õ��ڴ��С
	int style ;
} ;

//--------------------private--------------------------------------
static int RAlloc (							//�ռ䲻��,���·���ռ�
	struct checkmem *check_data ) ;

static void WriteData (						//�����������ļ�
	struct checkmem *check_data ,
	struct _checkmem *data ,
	int flag ) ;

static HCHECKMEM heap_log;


static int RAlloc (struct checkmem *check_data )
{
	struct _checkmem *tmp ;
	int size ;

	tmp = check_data ->data ;
	//һ���Զ�����ʮ�����ռ�
	size = sizeof ( struct _checkmem ) * ( check_data ->total + NODE_ADD_SIZE ) ;
	check_data ->data = (struct _checkmem *) GlobalAlloc ( 0 , size ) ;
	if ( !check_data ->data )
	{
		//�ָ�ԭ����
		check_data ->data = tmp ;
		return -1 ;
	}
	memset ( check_data ->data , 0 , size ) ;
	//�ָ�ԭ����¼
	if ( tmp )
	{
		memcpy ( check_data ->data , tmp , sizeof ( struct _checkmem ) * check_data ->total ) ;
		GlobalFree ( tmp ) ;
	}
	check_data ->total += NODE_ADD_SIZE ;

	return 0 ;
}


/*
 * 0-�������
 * 1-��ȷ���ͷŲ���
 * 2-������ͷŲ���
 * 3-δ�ͷ�
 */
void WriteData (							//дһ����¼
	struct checkmem *check_data ,
	struct _checkmem *data ,
	int flag )
{
	int hFile ;
	char msg [ 512 ] ;

	//д�ļ�
	//begin
	hFile = _open ( check_data ->logfile , _O_RDWR ) ;
	//�ļ�������
	if ( hFile < 0 )
	{
		//�����ļ�
		hFile = _open (
			check_data ->logfile ,
			_O_RDWR | _O_CREAT ,
			_S_IREAD | _S_IWRITE ) ;
		//����ʧ��
		if ( hFile < 0 )
			return ;

		memset ( msg , 0 , sizeof ( msg ) ) ;
		sprintf ( msg , "\n����ڴ�ʹ������ %d  �ֽ�\n" , check_data ->max ) ;
		_write ( hFile , msg , sizeof ( msg ) ) ;
		memset ( msg , 0 , sizeof ( msg ) ) ;
		sprintf ( msg , "\n��ǰ�ڴ�ʹ������ %d  �ֽ�\n" , check_data ->size ) ;
		_write ( hFile , msg , sizeof ( msg ) ) ;
		memset ( msg , 0 , sizeof ( msg ) ) ;
		//����ʾ��ַ
		if ( check_data ->style & CMS_NOADDR )
			strcpy ( msg , "\n��С    ״̬     �����ļ�       ��������\n" ) ;
		else
			strcpy ( msg , "\n��ַ    ��С    ״̬     �����ļ�       ��������\n" ) ;
		_write ( hFile , msg , strlen ( msg ) ) ;
	}
	else
	{
		_lseek ( hFile , 0 , SEEK_SET ) ;
		memset ( msg , 0 , sizeof ( msg ) ) ;
		sprintf ( msg , "\n����ڴ�ʹ������ %d  �ֽ�\n" , check_data ->max ) ;
		_write ( hFile , msg , sizeof ( msg ) ) ;
		memset ( msg , 0 , sizeof ( msg ) ) ;
		sprintf ( msg , "\n��ǰ�ڴ�ʹ������ %d  �ֽ�\n" , check_data ->size ) ;
		_write ( hFile , msg , sizeof ( msg ) ) ;
		_lseek ( hFile , 0 , SEEK_END ) ;
	}

	memset ( msg , 0 , sizeof ( msg ) ) ;

#if 0
	//�������
	if ( flag == 0 )
	{
		//����ʾ��ַ
		if ( check_data ->style & CMS_NOADDR )
			sprintf ( msg ,
				"%-10d����		%s\n" ,
				data ->size ,
				data ->param ) ;
		else
			sprintf ( msg ,
				"0x%-10x%-10d����		%s\n" ,
				data ->addr ,
				data ->size ,
				data ->param ) ;
	}
	else if ( flag == 1 )
	{
		//����ʾ��ַ
		if ( check_data ->style & CMS_NOADDR )
			sprintf ( msg ,
				"%-10d�ͷ�		%s\n" ,
				data ->size ,
				data ->param ) ;
		else
			sprintf ( msg ,
				"0x%-10x%-10d�ͷ�		%s\n" ,
				data ->addr ,
				data ->size ,
				data ->param ) ;
	}
	else 
#endif
		if ( flag == 2 )
	{
		//����ʾ��ַ
		if ( check_data ->style & CMS_NOADDR )
			sprintf ( msg ,
				"%-10d������ͷŲ���		%s\n" ,
				data ->size ,
				data ->param ) ;
		else
			sprintf ( msg ,
				"0x%-10x%-10d������ͷŲ���		%s\n" ,
				data ->addr ,
				data ->size ,
				data ->param ) ;
	}
	else if ( flag == 3 )
	{
		//����ʾ��ַ
		if ( check_data ->style & CMS_NOADDR )
			sprintf ( msg ,
				"%-10dδ�ͷŵ��ڴ�		%s\n" ,
				data ->size ,
				data ->param ) ;
		else
			sprintf ( msg ,
				"0x%-10x%-10dδ�ͷŵ��ڴ�		%s\n" ,
				data ->addr ,
				data ->size ,
				data ->param ) ;
	}
	_write ( hFile , msg , strlen ( msg ) ) ;
	_close ( hFile ) ;
}

//---------------------------------------------------------
HCHECKMEM _CMOpen (
	char *logfile ,
	int style )
{
	struct checkmem *check ;

	if ( !logfile )
		return NULL ;
	if ( strlen ( logfile ) <= 0 )
		return NULL ;

	check = ( struct checkmem * ) GlobalAlloc ( 0 , sizeof ( struct checkmem ) ) ;
	if ( !check )
		return NULL ;
	memset ( check , 0 , sizeof ( struct checkmem ) ) ;

	strcpy ( check ->logfile , logfile ) ;
	check ->style = style ;
	remove ( logfile ) ;
	return check ;
}

void _CMAddAlloc (
	HCHECKMEM check ,
	int addr ,
	int size ,
	char *param )
{
	struct _checkmem *tmp ;
	struct checkmem *check_data ;

	if ( !check )
		return ;
	if ( addr <= 0 )
		return ;

	check_data = ( struct checkmem * ) check ;
	//�ռ�����
	if ( check_data ->count == check_data ->total )
	{
		if ( RAlloc ( check_data ) < 0 )
			return ;
	}
	tmp = check_data ->data ;
	if ( param )
		strcpy ( tmp [ check_data ->count ] .param , param ) ;
	tmp [ check_data ->count ] .addr = addr ;
	tmp [ check_data ->count ] .size = size ;
	check_data ->count ++ ;
	check_data ->size += size ;
	if ( check_data ->size > check_data ->max )
		check_data ->max = check_data ->size ;

	//д�ļ�
	WriteData ( check_data , tmp + check_data ->count - 1 , 0 ) ;
}

void _CMAddFree (
	HCHECKMEM check ,
	int addr ,
	int size ,
	char *param )
{
	struct _checkmem tmp = { 0 } ;
	struct checkmem *check_data ;
	bool find = false ;		//�Ƿ��ҵ�������ڴ� true-�ҵ�
	int i ;

	if ( !check )
		return ;
	if ( addr <= 0 )
		return ;

	check_data = ( struct checkmem * ) check ;

	for ( i = 0 ; i < check_data ->count ; i ++ )
	{
		//��ַ��� �ҵ�
		if (	check_data ->data [ i ] .addr == addr )
		{
			find = true ;
			tmp .addr = addr ;
			if ( param )
				strcpy ( tmp .param , param ) ;
			tmp .size = check_data ->data [ i ] .size ;

			check_data ->size -= check_data ->data [ i ] .size ;

			//ɾ���ý��
			for ( ; i < check_data ->count - 1 ; i ++ )
				check_data ->data [ i ] = check_data ->data [ i + 1 ] ;
			//������һ��
			memset ( check_data ->data + i , 0 , sizeof ( struct _checkmem ) ) ;
			check_data ->count -- ;
			break ;
		}
	}
	//�ҵ���
	if ( find )
		//д�ļ�
		WriteData ( check_data , &tmp , 1 ) ;
	//û�ҵ�
	else if ( !find )
	{
		tmp .addr = addr ;
		if ( param )
			strcpy ( tmp .param , param ) ;
		tmp .size = 0 ;
		//д�ļ�
		WriteData ( check_data , &tmp , 2 ) ;
	}
}

void _CMClose (
	HCHECKMEM *check )
{
	struct checkmem *check_data ;
	int i ;

	if ( !check )
		return ;
	if ( !( *check ) )
		return ;

	check_data = ( struct checkmem * ) ( *check ) ;
	//д�ļ�
	//begin
	if ( check_data ->count > 0 )
	{
		int hFile ;
		char msg [ 512 ] = { 0 } ;

		hFile = _open ( check_data ->logfile , _O_RDWR ) ;
		if ( hFile < 0 )
			return ;
		_lseek ( hFile , 0 , SEEK_END ) ;
		strcpy ( msg , "\n����Ϊ�����˵�δ�ͷŵ��ڴ��\n" ) ;
		_write ( hFile , msg , sizeof ( msg ) ) ;
		_close ( hFile ) ;

		for ( i = 0 ; i < check_data ->count ; i ++ )
			WriteData ( check_data , check_data ->data + i , 3 ) ;
	}
	//end
	//�ͷ��ڴ�
	GlobalFree ( check_data ->data ) ;
}


// �Ӷ��з����ڴ�. ʧ�ܷ���NULL��
void *	XM_heap_debug_malloc	(int size, char *file, int line)
{
	char szbuffer [ 512 ] ;
	sprintf (szbuffer, "%s %d", file, line);
	void *addr = malloc (size);
	_CMAddAlloc (heap_log, (int)addr, size, szbuffer);
	return addr;
}

// �Ӷ��з����ڴ棬����������ֽ�ȫ����0. ʧ�ܷ���NULL
void *	XM_heap_debug_calloc	(int size, char *file, int line)
{
	void *addr = XM_heap_debug_malloc (size, file, line);
	if(addr)
	{
		memset (addr, 0, size);
	}
	return addr;
}

// �ͷ��û�RAM����
void		XM_heap_debug_free	(void *mem, char *file, int line)
{
	char szbuffer [ 512 ] ;
	sprintf (szbuffer, "%s %d", file, line);
	_CMAddFree (heap_log, (int)mem, 0, szbuffer);
	free (mem);
}

void XM_heap_init (void)
{
	heap_log = _CMOpen (".\\heap_debug.txt", 0);
}

void XM_heap_exit (void)
{
	_CMClose (&heap_log);
}

#else

// �Ӷ��з����ڴ�. ʧ�ܷ���NULL��
void *	XM_heap_malloc	(int size)
{
	return malloc (size);
}

// �Ӷ��з����ڴ棬����������ֽ�ȫ����0. ʧ�ܷ���NULL
void *	XM_heap_calloc	(int size)
{
	return calloc (size, 1);
}

// �ͷ��û�RAM����
void		XM_heap_free	(void *mem)
{
	free (mem);
}

void XM_heap_init (void)
{
}

void XM_heap_exit (void)
{
}

#endif

