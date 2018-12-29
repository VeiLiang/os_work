#include <common_string.h>

int xm_strlen (const char *s)
{
	int count = 0;
	if(s == 0)
		return 0;
	while(*s)
	{
		s ++;
		count ++;
	}
	return count;
}

void *	xm_memcpy	(void *_s1, const void *_s2, int _n	)
{
	unsigned char *s1 = (unsigned char *)_s1;
	unsigned char *s2 = (unsigned char *)_s2;
	
	if(s1 == 0 || s2 == 0)
		return NULL;
	while(_n > 0)
	{
		*s1 = *s2;
		s1 ++;
		s2 ++;
		_n --;
	}	
	return _s1;
}

void *	xm_memset	(void *_s, int _c, int _n)
{
	signed char *s = (signed char *)_s;
	if(s == NULL || _n == 0)
		return NULL;
	while(_n > 0)
	{
		*s = (signed char)_c;
		s ++;
		_n --; 
	}
	return _s;
}

int xm_strcmpi(const char * dst, const char * src)
{
	int f,l;
	
	do {
		if ( ((f = (unsigned char)(*(dst++))) >= 'A') && (f <= 'Z') )
			f -= ('A' - 'a');
		
		if ( ((l = (unsigned char)(*(src++))) >= 'A') && (l <= 'Z') )
			l -= ('A' - 'a');
	} while ( f && (f == l) );
	
	return(f - l);
}

int xm_stricmp (const char * dst, const char * src)
{
	return( xm_strcmpi(dst,src) );
}

int xm_strnicmp (const char * dst, const char * src, int count)
{
	int f,l;

	if(count <= 0)
		return 0;

	do 
	{
		if ( ((f = (unsigned char)(*(dst++))) >= 'A') && (f <= 'Z') )
			f -= ('A' - 'a');

		if ( ((l = (unsigned char)(*(src++))) >= 'A') && (l <= 'Z') )
			l -= ('A' - 'a');
		count --;
	} while ( f && (f == l) && count > 0 );

	return(f - l);
}