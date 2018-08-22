 
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <curl/curl.h>

#include "cfgable.h"
#include "common.h"
#include "tts.h"

static size_t write_memory(void *ptr, size_t size, size_t nmemb, void *userp)
{
	size_t realsize = size * nmemb;
	MemoryStruct *mem = (MemoryStruct *)userp;
	
	mem->memory = realloc(mem->memory, mem->size + realsize + 1);
	if(mem->memory == NULL) 
	{
	    /* out of memory! */
	    fprintf(stderr, "not enough memory (realloc returned NULL)\n");
	    return 0;
	}
	
	memcpy(&(mem->memory[mem->size]), ptr, realsize);
	mem->size += realsize;
	mem->memory[mem->size] = 0;
	
	return realsize;
}

static size_t write_data(void *ptr, size_t size, size_t nmemb, void *stream)
{
	size_t written = fwrite(ptr, size, nmemb, (FILE *)stream);
	return written;
}

static int bb_tts(char *url, char *outfile)
{
	if(!url || !outfile)
	{
		fprintf(stderr, " Url or outfile is NULL! \n");
		return -1;
	}
	
	CURL *curl_handle = NULL;
	int res = PTTS_FAIL;
	
	MemoryStruct header;
	
	char *filename = outfile;
	FILE *filp;
	
	filp = fopen(filename, "wb");
	if(!filp)
	{
		fprintf(stderr, " Fail to open file! \n");
		return -1;
	}
	
	header.memory = malloc(1);
	header.size = 0;
	
	curl_global_init(CURL_GLOBAL_ALL);
	
	curl_handle = curl_easy_init();
	if(curl_handle)
	{
		curl_easy_setopt(curl_handle, CURLOPT_URL, url);
		
		curl_easy_setopt(curl_handle, CURLOPT_VERBOSE, 1L);
		curl_easy_setopt(curl_handle, CURLOPT_NOPROGRESS, 1L);		
		curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION, 1L);
		
		curl_easy_setopt(curl_handle, CURLOPT_HEADERFUNCTION, write_memory);			
		curl_easy_setopt(curl_handle, CURLOPT_HEADERDATA, (void*)&header);
		
		curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_data);
		curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, filp);
		
	    res = curl_easy_perform(curl_handle);
		if(res != CURLE_OK) 
		{
			fprintf(stderr, "curl_easy_perform() failed (%d): %s\n",
				res, curl_easy_strerror(res));
		}
		
		curl_easy_cleanup(curl_handle);		
	}
	
	curl_global_cleanup();
	
	fclose(filp);
	
	DBG_PRINTF(" Size: %d, Header: %s \n", header.size, header.memory);
	
	if(header.size > 0)
	{
		if(strstr(header.memory, "Content-type: audio/mp3"))
		{
			res = PTTS_OK;
		}
		else
		{
			res = PTTS_BBFMT;
		}
	}
	
	if(header.memory)
	{
		free(header.memory);
		header.memory = NULL;
		header.size = 0;
	}
	
	return res;
}

static char * get_content_from_file(char *path)
{
	int fd = -1;
	int flength = -1;
	int mlength = -1;
	struct stat tStat;
	char *mp = NULL;
	char *content = NULL;
	
	fd = open(path, O_RDONLY);
	if (fd < 0)
	{
		fprintf(stderr, "can't open %s. \n", path);
		goto content_exit;
	}
	
	if(fstat(fd, &tStat) < 0)
	{
		fprintf(stderr, "can't get fstat. \n");
		goto content_exit;
	}
	
	flength = tStat.st_size;
	mp = (char *)mmap(NULL , flength, PROT_READ, MAP_SHARED, fd, 0);
	if (mp == (char *)-1)
	{
		fprintf(stderr, "can't mmap for outfile \n");
		goto content_exit;
	}
	
	mlength = tStat.st_size + 1;
	content = (char *)malloc(mlength);
	if(NULL == content)
	{
		fprintf(stderr, "Out of memory! \n");
		goto content_exit;
	}
	memset(content, 0, mlength);
	memcpy(content, mp, flength);
	
content_exit:
	
	if(mp)
	{
		munmap(mp, tStat.st_size);
		mp = NULL;
	}
	if(fd > 0)
	{
		close(fd);
		fd = -1;
	}
	
	return content;
}

int bb_main(void)
{
	int ret = -1;
	int ret2 = -1;
	char *content = NULL;
	char url[MAX_URL] = { 0 };	
	
	sprintf(url, BEEBA_TTS_URL, getConfig()->sn, getConfig()->text);
	
	DBG_PRINTF(" Url  : %s \n", url);
	
	ret = bb_tts(url, getConfig()->outfile);
	if(PTTS_OK != ret)
	{
		content = get_content_from_file(getConfig()->outfile);
		if(!content)
		{
			DBG_PRINTF(" Failed to get_content_from_file \n");
			return -1;
		}
		
		ret2 = report_tts_error(content, getConfig()->sn);
		DBG_PRINTF(" report_tts_error. ret2: %d \n", ret2);
		
		if(content)
		{
			free(content);
			content = NULL;
		}
	}
	
	return ret;
}

