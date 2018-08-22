
#include "common.h"
#include "cfgable.h"

#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdarg.h>
#include <openssl/md5.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

void dbg_printf(const char *file, int line, const char *func, const char *format, ...)
{
	char szLog[1024] = { 0 };
	va_list ap;
	va_start(ap, format);
	vsprintf(szLog, format, ap);
	va_end(ap);
	
	fprintf(stderr, "%s:%d %s(): %s", file, line, func, szLog);
}

void trim(char *src)
{
	char *begin = src;
	char *end   = src;
	
	while (*end++);
	
	if (begin == end) 
		return;
	
	while (*begin==' ' || *begin=='\t' || *begin=='\n')
		++begin;
	
	while (*end=='\0' || *end==' ' || *end=='\t'|| *end=='\n')
		--end;
	
	if (begin > end)
	{
		*src = '\0';
		return;
	}
	
	while (begin != end)
		*src++ = *begin++;
	
	*src++ = *end;
	*src = '\0';
	
	return;
}

char* get_beeba_version(void)
{
	static char version[MAX_LINE] = { 0 };
	
	int fd, rc;
	char buffer[MAX_LINE] = { 0 };
	
	memset(version, 0, sizeof(version));
	
	fd = open(BEEBA_VERSION_PATH, O_RDONLY);
	if(fd < 0)
	{	
		fprintf(stderr, "Failed to open %s \n", BEEBA_VERSION_PATH);
		return NULL;
	}
	
	rc = read(fd, buffer, sizeof(buffer));
	if(rc < 0)
	{	
		fprintf(stderr, "Failed to read %s, strerror(%d): %s \n", BEEBA_VERSION_PATH, errno, strerror(errno));
		return NULL;
	}
	
	trim(buffer);
	strcpy(version, buffer);
	
	if(fd > 0)
	{
		close(fd);
		fd = -1;
	}
	
//	fprintf(stderr, "get_beeba_version: %s \n", version);
	return version;
}

char* get_beeba_sn(void)
{
	static char sn[MAX_LINE] = { 0 };
	
	int fd, rc;
	char buffer[MAX_LINE] = { 0 };
	
	memset(sn, 0, sizeof(sn));
	
	fd = open(BEEBA_SN_PATH, O_RDONLY);
	if(fd < 0)
	{	
		fprintf(stderr, "Failed to open %s \n", BEEBA_SN_PATH);
		return NULL;
	}
	
	rc = read(fd, buffer, sizeof(buffer));
	if(rc < 0)
	{	
		fprintf(stderr, "Failed to read %s, strerror(%d): %s \n", BEEBA_SN_PATH, errno, strerror(errno));
		return NULL;
	}
	
	trim(buffer);
	strcpy(sn, buffer);
	
	if(fd > 0)
	{
		close(fd);
		fd = -1;
	}
	
//	fprintf(stderr, "get_beeba_sn: %s \n", sn);
	return sn;
}

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

int get_tts_vendor(void)
{
	int vendor = TTS_XUNFEI;
	
	char line[MAX_LINE] = { 0 };
	FILE *pp = NULL;
	
	pp = popen("uci get device.beeba.tts_vendor 2>/dev/null", "r");
	if (!pp) goto vendor_exit;
	
	while(NULL != fgets(line, sizeof(line), pp))
	{		
		if(NULL != strstr(line, "beeba"))
		{
			vendor = TTS_BEEBA;
			break;
		}
	}
	
vendor_exit:	
	if(pp)
	{
		pclose(pp);
		pp = NULL;
	}
	
	return vendor;
}

int playsound(char *filename)
{
	if(NULL==filename || 0==strlen(filename))
	{
		return -1;
	}
	
	pid_t pid = vfork();
	if (pid < 0)
	{
		fprintf(stderr, "fork error!");
		return -1;
	}
	if (pid > 0) // > 0, 处于父进程中
	{
		return 0;
	}
	
	execlp("/usr/bin/madplay", "madplay", filename, NULL);
	exit(1);
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

int playtts_check_directory(void)
{
	char *playtts_dirs[] = { PLAYTTS_CACHE_PATH"/beeba", NULL };
	
	int i = 0;
	
	for(i = 0; playtts_dirs[i] != NULL; i++)
	{
		if(0 != access(playtts_dirs[i], F_OK))
		{
			create_path(playtts_dirs[i]);
		}
	}
	
	return 0;
}

