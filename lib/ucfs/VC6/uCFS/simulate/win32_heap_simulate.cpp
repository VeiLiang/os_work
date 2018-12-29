//****************************************************************************
//
//	Copyright (C) 2010~2014 ZhuoYongHong
//
//	Author	ZhuoYongHong
//
//	File name: win32_heap_simulate.c
//	  Win32 全局堆分配接口模拟
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


#define NODE_ADD_SIZE				100		//一次性申请的结点个数
#define CMS_NOADDR			0x00000001		//不显示地址
typedef void *HMEMORY;
typedef void *HCHECKMEM;
struct _checkmem
{
	char param [ 128 ] ;					//附加参数
	int addr ;								//地址
	int size ;								//大小
} ;

struct checkmem
{
	struct _checkmem *data ;			//结点数据
	int total ;								//空间大小
	int count ;								//结点个数
	char logfile [ 512 ] ;				//日志文件
	int max ;								//内存操作过程中申请内存的最大值
	int size ;								//当前使用的内存大小
	int style ;
} ;

//--------------------private--------------------------------------
static int RAlloc (							//空间不足,重新分配空间
	struct checkmem *check_data ) ;

static void WriteData (						//更新数据至文件
	struct checkmem *check_data ,
	struct _checkmem *data ,
	int flag ) ;

static HCHECKMEM heap_log;


static int RAlloc (struct checkmem *check_data )
{
	struct _checkmem *tmp ;
	int size ;

	tmp = check_data ->data ;
	//一次性多申请十个结点空间
	size = sizeof ( struct _checkmem ) * ( check_data ->total + NODE_ADD_SIZE ) ;
	check_data ->data = (struct _checkmem *) GlobalAlloc ( 0 , size ) ;
	if ( !check_data ->data )
	{
		//恢复原来的
		check_data ->data = tmp ;
		return -1 ;
	}
	memset ( check_data ->data , 0 , size ) ;
	//恢复原来记录
	if ( tmp )
	{
		memcpy ( check_data ->data , tmp , sizeof ( struct _checkmem ) * check_data ->total ) ;
		GlobalFree ( tmp ) ;
	}
	check_data ->total += NODE_ADD_SIZE ;

	return 0 ;
}


/*
 * 0-申请操作
 * 1-正确的释放操作
 * 2-错误的释放操作
 * 3-未释放
 */
void WriteData (							//写一条记录
	struct checkmem *check_data ,
	struct _checkmem *data ,
	int flag )
{
	int hFile ;
	char msg [ 512 ] ;

	//写文件
	//begin
	hFile = _open ( check_data ->logfile , _O_RDWR ) ;
	//文件不存在
	if ( hFile < 0 )
	{
		//创建文件
		hFile = _open (
			check_data ->logfile ,
			_O_RDWR | _O_CREAT ,
			_S_IREAD | _S_IWRITE ) ;
		//创建失败
		if ( hFile < 0 )
			return ;

		memset ( msg , 0 , sizeof ( msg ) ) ;
		sprintf ( msg , "\n最大内存使用量： %d  字节\n" , check_data ->max ) ;
		_write ( hFile , msg , sizeof ( msg ) ) ;
		memset ( msg , 0 , sizeof ( msg ) ) ;
		sprintf ( msg , "\n当前内存使用量： %d  字节\n" , check_data ->size ) ;
		_write ( hFile , msg , sizeof ( msg ) ) ;
		memset ( msg , 0 , sizeof ( msg ) ) ;
		//不显示地址
		if ( check_data ->style & CMS_NOADDR )
			strcpy ( msg , "\n大小    状态     所在文件       所在行数\n" ) ;
		else
			strcpy ( msg , "\n地址    大小    状态     所在文件       所在行数\n" ) ;
		_write ( hFile , msg , strlen ( msg ) ) ;
	}
	else
	{
		_lseek ( hFile , 0 , SEEK_SET ) ;
		memset ( msg , 0 , sizeof ( msg ) ) ;
		sprintf ( msg , "\n最大内存使用量： %d  字节\n" , check_data ->max ) ;
		_write ( hFile , msg , sizeof ( msg ) ) ;
		memset ( msg , 0 , sizeof ( msg ) ) ;
		sprintf ( msg , "\n当前内存使用量： %d  字节\n" , check_data ->size ) ;
		_write ( hFile , msg , sizeof ( msg ) ) ;
		_lseek ( hFile , 0 , SEEK_END ) ;
	}

	memset ( msg , 0 , sizeof ( msg ) ) ;

#if 0
	//申请操作
	if ( flag == 0 )
	{
		//不显示地址
		if ( check_data ->style & CMS_NOADDR )
			sprintf ( msg ,
				"%-10d申请		%s\n" ,
				data ->size ,
				data ->param ) ;
		else
			sprintf ( msg ,
				"0x%-10x%-10d申请		%s\n" ,
				data ->addr ,
				data ->size ,
				data ->param ) ;
	}
	else if ( flag == 1 )
	{
		//不显示地址
		if ( check_data ->style & CMS_NOADDR )
			sprintf ( msg ,
				"%-10d释放		%s\n" ,
				data ->size ,
				data ->param ) ;
		else
			sprintf ( msg ,
				"0x%-10x%-10d释放		%s\n" ,
				data ->addr ,
				data ->size ,
				data ->param ) ;
	}
	else 
#endif
		if ( flag == 2 )
	{
		//不显示地址
		if ( check_data ->style & CMS_NOADDR )
			sprintf ( msg ,
				"%-10d错误的释放操作		%s\n" ,
				data ->size ,
				data ->param ) ;
		else
			sprintf ( msg ,
				"0x%-10x%-10d错误的释放操作		%s\n" ,
				data ->addr ,
				data ->size ,
				data ->param ) ;
	}
	else if ( flag == 3 )
	{
		//不显示地址
		if ( check_data ->style & CMS_NOADDR )
			sprintf ( msg ,
				"%-10d未释放的内存		%s\n" ,
				data ->size ,
				data ->param ) ;
		else
			sprintf ( msg ,
				"0x%-10x%-10d未释放的内存		%s\n" ,
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
	//空间已满
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

	//写文件
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
	bool find = false ;		//是否找到申请的内存 true-找到
	int i ;

	if ( !check )
		return ;
	if ( addr <= 0 )
		return ;

	check_data = ( struct checkmem * ) check ;

	for ( i = 0 ; i < check_data ->count ; i ++ )
	{
		//地址相等 找到
		if (	check_data ->data [ i ] .addr == addr )
		{
			find = true ;
			tmp .addr = addr ;
			if ( param )
				strcpy ( tmp .param , param ) ;
			tmp .size = check_data ->data [ i ] .size ;

			check_data ->size -= check_data ->data [ i ] .size ;

			//删除该结点
			for ( ; i < check_data ->count - 1 ; i ++ )
				check_data ->data [ i ] = check_data ->data [ i + 1 ] ;
			//清空最后一个
			memset ( check_data ->data + i , 0 , sizeof ( struct _checkmem ) ) ;
			check_data ->count -- ;
			break ;
		}
	}
	//找到了
	if ( find )
		//写文件
		WriteData ( check_data , &tmp , 1 ) ;
	//没找到
	else if ( !find )
	{
		tmp .addr = addr ;
		if ( param )
			strcpy ( tmp .param , param ) ;
		tmp .size = 0 ;
		//写文件
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
	//写文件
	//begin
	if ( check_data ->count > 0 )
	{
		int hFile ;
		char msg [ 512 ] = { 0 } ;

		hFile = _open ( check_data ->logfile , _O_RDWR ) ;
		if ( hFile < 0 )
			return ;
		_lseek ( hFile , 0 , SEEK_END ) ;
		strcpy ( msg , "\n以下为分配了但未释放的内存块\n" ) ;
		_write ( hFile , msg , sizeof ( msg ) ) ;
		_close ( hFile ) ;

		for ( i = 0 ; i < check_data ->count ; i ++ )
			WriteData ( check_data , check_data ->data + i , 3 ) ;
	}
	//end
	//释放内存
	GlobalFree ( check_data ->data ) ;
}


// 从堆中分配内存. 失败返回NULL。
void *	XM_heap_debug_malloc	(int size, char *file, int line)
{
	char szbuffer [ 512 ] ;
	sprintf (szbuffer, "%s %d", file, line);
	void *addr = malloc (size);
	_CMAddAlloc (heap_log, (int)addr, size, szbuffer);
	return addr;
}

// 从堆中分配内存，并将分配的字节全部清0. 失败返回NULL
void *	XM_heap_debug_calloc	(int size, char *file, int line)
{
	void *addr = XM_heap_debug_malloc (size, file, line);
	if(addr)
	{
		memset (addr, 0, size);
	}
	return addr;
}

// 释放用户RAM到堆
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

// 从堆中分配内存. 失败返回NULL。
void *	XM_heap_malloc	(int size)
{
	return malloc (size);
}

// 从堆中分配内存，并将分配的字节全部清0. 失败返回NULL
void *	XM_heap_calloc	(int size)
{
	return calloc (size, 1);
}

// 释放用户RAM到堆
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

