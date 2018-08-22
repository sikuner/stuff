
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "types.h"

/* base64 encoding/decoding functions by Mixter */

static const char b64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/* http don't supports +/ */
static const char b64_http[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";

static char ascii[256] =
{
	64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
	64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
	64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 62, 64, 64, 64, 63,
	52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 64, 64, 64, 64, 64, 64,
	64, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14,
	15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 64, 64, 64, 64, 64,
	64, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
	41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 64, 64, 64, 64, 64,
	64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
	64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
	64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
	64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
	64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
	64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
	64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
	64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64
};

void base64_in (char *buf, char *obuf, int len)
{
	int i;
	for (i = 0; i < len - 2; i += 3)
	{
		*obuf++ = b64[(buf[i] >> 2) & 0x3F];
		*obuf++ = b64[((buf[i] & 0x3) << 4 | ((int) (buf[i + 1] & 0xF0) >> 4))];
		*obuf++ = b64[((buf[i + 1] & 0xF) << 2) | ((int) (buf[i + 2] & 0xC0) >> 6)];
		*obuf++ = b64[buf[i + 2] & 0x3F];
	}
	if (i < len)
	{
		*obuf++ = b64[(buf[i] >> 2) & 0x3F];
		if (i == (len - 1))
		{
			*obuf++ = b64[((buf[i] & 0x3) << 4)];
//        *obuf++ = '=';
		}
		else
		{
			*obuf++ = b64[((buf[i] & 0x3) << 4 | ((int) (buf[i + 1] & 0xf0) >> 4))];
			*obuf++ = b64[((buf[i + 1] & 0xf) << 2)];
		}
//      *obuf++ = '=';
	}
	*obuf++ = '\0';
}

void base64_in_http (unsigned char *buf, char *obuf, int len)
{
	int i;
	for (i = 0; i < len - 2; i += 3)
	{
		*obuf++ = b64_http[(buf[i] >> 2) & 0x3F];
		*obuf++ = b64_http[((buf[i] & 0x3) << 4 | ((int) (buf[i + 1] & 0xF0) >> 4))];
		*obuf++ = b64_http[((buf[i + 1] & 0xF) << 2) | ((int) (buf[i + 2] & 0xC0) >> 6)];
		*obuf++ = b64_http[buf[i + 2] & 0x3F];
	}
	if (i < len)
	{
		*obuf++ = b64_http[(buf[i] >> 2) & 0x3F];
		if (i == (len - 1))
		{
			*obuf++ = b64_http[((buf[i] & 0x3) << 4)];
//        *obuf++ = '=';
		}
		else
		{
			*obuf++ = b64_http[((buf[i] & 0x3) << 4 | ((int) (buf[i + 1] & 0xf0) >> 4))];
			*obuf++ = b64_http[((buf[i + 1] & 0xf) << 2)];
		}
//      *obuf++ = '=';
	}
	
	*obuf++ = '\0';
}

int base64_out(char *buf, char *obuf, int len)
{
	int nprbytes;
	int nobytes = 0;
	char *p = buf;
	while (ascii[(int) *(p++)] <= 63);

	nprbytes = len - 1;

	while (nprbytes > 4 && *buf != '\0')
	{
		*(obuf++) = (ascii[(int) *buf] << 2 | ascii[(int) buf[1]] >> 4);
		*(obuf++) = (ascii[(int) buf[1]] << 4 | ascii[(int) buf[2]] >> 2);
		*(obuf++) = (ascii[(int) buf[2]] << 6 | ascii[(int) buf[3]]);
		buf += 4;
		nprbytes -= 4;
		nobytes += 3;
	}
	if (nprbytes > 1)
	{
		*(obuf++) =
		    (ascii[(int) *buf] << 2 | ascii[(int) buf[1]] >> 4);
		nobytes += 1;
	}
	if (nprbytes > 2)
	{
		*(obuf++) =
		    (ascii[(int) buf[1]] << 4 | ascii[(int) buf[2]] >> 2);
		nobytes += 1;
	}
	if (nprbytes > 3)
	{
		*(obuf++) =
		    (ascii[(int) buf[2]] << 6 | ascii[(int) buf[3]]);
		nobytes += 1;
	}
	*(obuf)++ = '\0';

	return nobytes;
}

/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////

int parse_artist_title(const char *uri, char *artist, char *title) // 歌手/歌单, 歌名, 出错信息
{
	if(!uri)
		return -1;
	
	char *param_begin = NULL;
	char *param_end = NULL, *param_end1 = NULL, *param_end2 = NULL;
	int param_len = 0;
	char *start = (char *)uri;
	char *tmp = NULL;
	char enbase64[4096] = { 0 };
	char debase64[4096] = { 0 };
	char *artist_begin=NULL, *artist_end=NULL;
	char *album_begin=NULL, *album_end=NULL;
	char *title_begin=NULL, *title_end=NULL;
	int ret = 0;
	int mod, i;
	bool has_artist = false;
	
	start = strstr(uri, "param=");
	if(!start)
		return -1;
	
	start = start + strlen("param=");
	param_begin = start;
	
	param_end1 = strchr(param_begin, '&');
	param_end2 = strchr(param_begin, '?');
	
	if(param_end1 && param_end2)
	{
		param_end = min(param_end1, param_end2);
	}
	else if(param_end1)
	{
		param_end = param_end1;
	}
	else if(param_end2)
	{
		param_end = param_end2;
	}
	
	if(!param_end)
		param_end = param_begin + strlen(param_begin);
	param_len = param_end - param_begin;
	
	memcpy(enbase64, param_begin, param_len);
	
	// 特殊字符处理 '-'=>'+', '_'=>'/', 4字节对齐
	tmp = NULL;
	start = enbase64;
	while(NULL != (tmp = strchr(start, '-')))
	{
		*tmp = '+';
		start = tmp + 1;
	}
	tmp = NULL;
	start = enbase64;
	while(NULL != (tmp = strchr(start, '_')))
	{
		*tmp = '/';
		start = tmp + 1;
	}
	mod = strlen(enbase64) % 4;
    for (i = 0; (mod!=0&&i<4-mod); i++)
	{
		strcat(enbase64, "=");
	}
	
	base64_out(enbase64, debase64, param_len);
//	fprintf(stderr, "debase64: %s\n", debase64);
	
	title_begin  = strstr(debase64, "title=");
	artist_begin = strstr(debase64, "artist=");
	album_begin  = strstr(debase64, "album=");
	
	if(title_begin)
	{
		title_begin += strlen("title=");
		title_end = strchr(title_begin, '&');
		if(!title_end)
			title_end = title_begin + strlen(title_begin);		
		*title_end = '\0';
		
		if(NULL == strstr(title_begin, "null") 
		&& NULL == strstr(title_begin, "NULL")
		&& strlen(title_begin) > 1)
		{
			if(title)
				strcpy(title, title_begin);
		}
	}
	else
	{
		ret = -1;
	}
	
	if(artist_begin)
	{
		artist_begin += strlen("artist=");
		artist_end = strchr(artist_begin, '&');
		if(!artist_end)
			artist_end = artist_begin + strlen(artist_begin);
		*artist_end = '\0';
		
		if(NULL == strstr(artist_begin, "null") 
		&& NULL == strstr(artist_begin, "NULL")
		&& strlen(artist_begin) > 1)
		{
			if(artist)
				strcpy(artist, artist_begin);
			
			has_artist = true;
		}
	}
	else
	{
		ret = -1;
	}
	
	if(!has_artist)
	{
		if(album_begin)
		{
			album_begin += strlen("album=");
			album_end = strchr(album_begin, '&');
			if(!album_end)
				album_end = album_begin + strlen(album_begin);
			*album_end = '\0';
			
			if(NULL == strstr(album_begin, "null") 
			&& NULL == strstr(album_begin, "NULL")
			&& strlen(album_begin) > 1)
			{
				if(artist)
					strcpy(artist, album_begin);
			}
		}		
		else
		{
			ret = -1;
		}
	}
	
	return ret;
}

