
#include "common.h"

#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdarg.h>
#include <openssl/md5.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>

void dbg_printf(const char *file, int line, const char *func, const char *format, ...)
{
	char szLog[1024] = { 0 };
	va_list ap;
	va_start(ap, format);
	vsprintf(szLog, format, ap);
	va_end(ap);
	
	fprintf(stderr, "%s:%d %s(): %s", file, line, func, szLog);
}

// 以URL的md5值作为文件名
char* get_md5sum(const char *str)
{
	if(NULL == str)
	{
		return NULL;
	}
	
	static char md5sum[33] = { 0 };
	memset(md5sum, 0, sizeof(md5sum));
	
	unsigned char md[16];
	char tmp[3] = { 0 };
	int i = 0;
	
	MD5((const unsigned char *)str, (unsigned long)strlen(str), md);
	for(i = 0; i < 16; i++)
	{
		sprintf (tmp, "%2.2x" ,md[i]);
		strcat (md5sum, tmp);
	}
//	printf("%s \n", md5sum);
	
	return md5sum;
}

char* url_strip_param(const char *url)
{
	if(NULL==url || 0==strlen(url))
	{
		return NULL;
	}
	
	static char url_strip[MAX_URL] = { 0 };
	
	char *begin = (char*)url;
	char *end = NULL;
	
	end = strstr(begin, "param=");
	if(end)
	{
		end -= 1; // 回退一个字符, 去掉连接字符 '?' 或 '&'
	}
	else
	{
		end = begin + strlen(begin);
	}
	
	memset(url_strip, 0, sizeof(url_strip));
	memcpy(url_strip, begin, end-begin);
	
//	fprintf(stderr, "url_strip: %s \n", url_strip);
	return url_strip;
}

char* get_file_type(const char *url) // 获取该文件名的类型后缀
{
	if(NULL==url || 0==strlen(url))
	{
		return NULL;
	}
	
	static char file_type[MAX_LINE] = { 0 };
	char type[MAX_LINE] = { 0 };
	
	char *types[] = { "mp3", "bac", "m4a", NULL };
	char *def_type = "mp3";
	
	char url_backup[MAX_URL] = { 0 } ;
	
	int i = 0;
	char *type_begin = NULL;
	char *type_end = NULL;
	
	memset(file_type, 0, sizeof(file_type));
	strcpy(url_backup, url);
	
	type_end = strchr(url_backup, '?');
	if(type_end)
	{
		*type_end = '\0';
	}
	else
	{
		type_end = url_backup + strlen(url_backup);
	}
	
	type_begin = strrchr(url_backup, '.');
	if(type_begin)
	{
		type_begin += 1;
	}
	else
	{
		type_begin = url_backup;
	}
	
	strcpy(type, type_begin);
	
//	fprintf(stderr, "get_file_type111 type: %s \n", type);
	
	for(i = 0; NULL != types[i]; i++)
	{
		if(0 == strcmp(types[i], type))
		{
			break;
		}
	}
	
	if(types[i])
	{
		strcpy(file_type, types[i]);
	}
	else
	{
		strcpy(file_type, def_type);
	}
	
//	fprintf(stderr, "get_file_type222 file_type: %s \n", file_type);
	return file_type;
}

char* get_url_title(char *url)
{
	if(NULL==url || 0==strlen(url))
	{
		return NULL;
	}
	
	char *param_begin = NULL;
	char *param_end = NULL, *param_end1 = NULL, *param_end2 = NULL;
	int param_len = 0;
	char *start = (char *)url;
	char *tmp = NULL;
	char enbase64[MAX_URL] = { 0 };
	char debase64[MAX_URL] = { 0 };
	char *title_begin=NULL, *title_end=NULL;
	int mod, i;
	
	static char title[MAX_PATH] = { 0 };
	memset(title, 0, sizeof(title));
	
	start = strstr(url, "param=");
	if(!start) // 没有参数"param="
	{
		return NULL;
	}
	
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
	fprintf(stderr, "debase64: %s\n", debase64);
	
	title_begin = strstr(debase64, "title=");
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
			strcpy(title, title_begin);
			DBG_PRINTF("title: %s \n", title);
			return title;
		}
	}
	
	return NULL;
}

char* get_complete_filename(char *url)
{
	if(NULL==url || 0==strlen(url))
	{
		return NULL;
	}
	
	char *param_begin = NULL;
	char *param_end = NULL, *param_end1 = NULL, *param_end2 = NULL;
	int param_len = 0;
	char *start = (char *)url;
	char *tmp = NULL;
	char enbase64[MAX_URL] = { 0 };
	char debase64[MAX_URL] = { 0 };
	char *title_begin=NULL, *title_end=NULL;
	int mod, i;
	bool has_title = false;
	
	char *vurl = NULL;
	char *md5sum = NULL;
	char *type = NULL;
	char title[MAX_PATH] = { 0 };
	
	static char filename[MAX_PATH] = { 0 };
	memset(filename, 0, sizeof(filename));
	
	type = get_file_type(url);
	vurl = url_strip_param(url);
	md5sum = get_md5sum(vurl);
	
	start = strstr(start, "param=");
	if(!start) // 没有参数"param="
	{
		sprintf(filename, DOWN_COMPLETE_PATH"/%s.%s", md5sum, type);
//		DBG_PRINTF("complete_filename: %s \n", filename);
		return filename;
	}
	
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
	fprintf(stderr, "debase64: %s\n", debase64);
	
	title_begin = strstr(debase64, "title=");
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
			strcpy(title, title_begin);
			has_title = true;
		}
	}
	
	if(has_title)
	{
		sprintf(filename, DOWN_COMPLETE_PATH"/%s_%s.%s", md5sum, title, type);
	}
	else
	{
		sprintf(filename, DOWN_COMPLETE_PATH"/%s.%s", md5sum, type);
	}
	
//	DBG_PRINTF("complete_filename: %s \n", filename);
	return filename;
}



/**
 * create_path
 *
 *   This function creates a file path, like mkdir -p. 
 *
 * Parameters:
 *
 *   path - the path to create
 *
 * Returns: 0 on success, -1 on failure
 * On failure, a message has been printed to stderr.
 */
static int create_path(const char *path)
{
	char *start;
	mode_t mode = S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;
	
	if (path[0] == '/')
		start = strchr(path + 1, '/');
	else
		start = strchr(path, '/');
	
	while (start) 
	{
		char *buffer = strdup(path);
		buffer[start-path] = 0x00;
		
		if (mkdir(buffer, mode)==-1 && errno!=EEXIST) 
		{
			fprintf(stderr, "Problem creating directory %s", buffer);
			perror(" ");
			free(buffer);
			return -1;
		}
		free(buffer);
		start = strchr(start + 1, '/');
	}
	
	return 0;
}

int down_check_directory(void)
{
	char *down_dirs[] = { DOWN_TASK_PATH"/beeba", DOWN_COMPLETE_PATH"/beeba", NULL };
	
	int i = 0;
	
	for(i = 0; down_dirs[i] != NULL; i++)
	{
		if(0 != access(down_dirs[i], F_OK))
		{
			create_path(down_dirs[i]);
		}
	}
	
	return 0;
}

char* get_filename(char *path)
{
	if(NULL == path)
	{
		return NULL;
	}
	
	static char filename[MAX_PATH] = { 0 };
	memset(filename, 0, sizeof(filename));
	
	char *slant = NULL; // 最后一个斜线
	char *dot   = NULL; 	// basename中的第一分割点
	
	slant = strrchr(path, '/');
	if(!slant)
	{
		slant = path;
	}
	else
	{
		slant += 1;
		dot = strchr(slant, '.');
		if(!dot)
		{
			dot = path + strlen(path);
		}
	}
	
	strncpy(filename, slant, min((int)(dot-slant), (int)MD5_STRING_LENGTH));
//	fprintf(stderr, "get_filename: path: %s, filename: %s \n", path, filename);
	
	return filename;
}

static void trim(char *str)
{
	char *begin = str;
	char *end   = str;
	
	while (*end++);
	
	if (begin == end) 
		return;
	
	while (*begin==' ' || *begin=='\t' || *begin=='\n')
		++begin;
	
	while (*end=='\0' || *end==' ' || *end=='\t'|| *end=='\n')
		--end;
	
	if (begin > end)
	{
		*str = '\0';
		return;
	}
	
	while (begin != end)
		*str++ = *begin++;
	
	*str++ = *end;
	*str = '\0';
	
	return;
}

char* strduptrim(char *line)
{
	if(NULL==line || 0==strlen(line))
	{
		return NULL;
	}
	
	static char linedup[2*MAX_LINE];
	
	memset(linedup, 0, sizeof(linedup));
	strcpy(linedup, line);
	trim(linedup);
	
	return linedup;
}

static int id3tag_set_title(char *title, char *filename)
{
	if(NULL==title || 0==strlen(title) 
	|| NULL==filename || 0==strlen(filename))
	{
		return -1;
	}
	
	pid_t pid = vfork();
	if (pid < 0)
	{
		fprintf(stderr, "id3tag_set_title fork error!");
		return -1;
	}
	if (pid > 0)
	{
		return 0;
	}
	
//	char song_title[MAX_LINE] = { 0 };
//	sprintf(song_title, "--song=%s", title);
//	/root/id3v2-okey --TPE1 "周杰伦" --TIT2 "枫" ./ffdc70afc83d9ecf3c23118d4ebda0c5_枫.mp3
	execlp("/usr/bin/id3v2", "id3v2", "--TIT2", title, filename, NULL);
	exit(1);
}

int mp3_set_title(char *url, char *filename)
{
	if(NULL==url || 0==strlen(url) 
	|| NULL==filename || 0==strlen(filename))
	{
		return -1;
	}
	
	char *type = get_file_type(url);
	if(NULL==type || strcmp(type, "mp3"))
	{
		DBG_PRINTF("type: %s is not mp3 \n", type);
		return -1;
	}
	
	char *title = get_url_title(url);
	if(NULL==title)
	{
		DBG_PRINTF("title is NULL \n");
		return -1;		
	}
	
	return id3tag_set_title(title, filename);
}

