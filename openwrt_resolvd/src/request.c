
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <curl/curl.h>

#include "common.h"

#if 0

typedef struct tagMemoryStruct {
	
	char *memory;
	size_t size;
	
} MemoryStruct;

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

int request_header(char *url)
{
	if(!url)
	{
		fprintf(stderr, " Url is NULL! \n");
		return -1;
	}
	
	CURL *curl_handle = NULL;
	int res = 0;
	long lCode = 0;
	
	MemoryStruct header;
	
	header.memory = malloc(1);
	header.size = 0;
	
	curl_global_init(CURL_GLOBAL_ALL);
	
	curl_handle = curl_easy_init();
	if(curl_handle)
	{
		curl_easy_setopt(curl_handle, CURLOPT_CUSTOMREQUEST, "GET"); // 兼顾重定向-强行用GET方法
		curl_easy_setopt(curl_handle, CURLOPT_URL, url);
		
		curl_easy_setopt(curl_handle, CURLOPT_HEADER, 1); 
		curl_easy_setopt(curl_handle, CURLOPT_NOBODY, 1);
		curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT, 30); 	// 整个请求超时控制
		curl_easy_setopt(curl_handle, CURLOPT_NOSIGNAL, 1L); 	// 优化性能，防止超时崩溃 
		
		curl_easy_setopt(curl_handle, CURLOPT_VERBOSE, 1L);
		curl_easy_setopt(curl_handle, CURLOPT_NOPROGRESS, 1L);		
		curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION, 1L);
		
		curl_easy_setopt(curl_handle, CURLOPT_HEADERFUNCTION, write_memory);			
		curl_easy_setopt(curl_handle, CURLOPT_HEADERDATA, (void*)&header);
		
	    res = curl_easy_perform(curl_handle);
		if(res != CURLE_OK) 
		{
			fprintf(stderr, "curl_easy_perform() failed (%d): %s\n",
				res, curl_easy_strerror(res));
		}
		else
		{
			curl_easy_getinfo(curl_handle, CURLINFO_RESPONSE_CODE, &lCode);			
			DBG_PRINTF(" ResponseCode: %d \n", lCode);
		}
		
		curl_easy_cleanup(curl_handle);		
	}
	
	curl_global_cleanup();
	
	DBG_PRINTF(" Size: %d, Header: \n%s \n", header.size, header.memory);
	
	if(header.memory)
	{
		free(header.memory);
		header.memory = NULL;
		header.size = 0;
	}
	
	return 200 == lCode ? 0 : -1;
}

#else 

int request_header(char *url)
{
	if(!url)
	{
		fprintf(stderr, " Url is NULL! \n");
		return -1;
	}
	
	CURL *curl_handle = NULL;
	int res = 0;
	long lCode = 0;
	
	curl_global_init(CURL_GLOBAL_ALL);
	
	curl_handle = curl_easy_init();
	if(curl_handle)
	{
		curl_easy_setopt(curl_handle, CURLOPT_CUSTOMREQUEST, "GET");
		curl_easy_setopt(curl_handle, CURLOPT_URL, url);
		
		curl_easy_setopt(curl_handle, CURLOPT_HEADER, 1); 
		curl_easy_setopt(curl_handle, CURLOPT_NOBODY, 1);
		curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT, 30);
		curl_easy_setopt(curl_handle, CURLOPT_NOSIGNAL, 1L);
		
		curl_easy_setopt(curl_handle, CURLOPT_NOPROGRESS, 1L);		
		curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION, 1L);
		
	    res = curl_easy_perform(curl_handle);
		if(res != CURLE_OK) 
		{
			fprintf(stderr, "curl_easy_perform() failed (%d): %s\n",
				res, curl_easy_strerror(res));
		}
		else
		{
			curl_easy_getinfo(curl_handle, CURLINFO_RESPONSE_CODE, &lCode);			
		}
		
		curl_easy_cleanup(curl_handle);		
	}
	
	curl_global_cleanup();
	
	DBG_PRINTF(" Url: %s \n", url);
	DBG_PRINTF(" ResponseCode: %d \n", lCode);
	
	return 200 == lCode ? 0 : -1;
}

bool check_btools(void)
{
	bool ret = true;
	
	FILE *pp = NULL;
	char line[MAX_LINE] = { 0 };
	
	pp = popen("btools check2", "r");
	if (pp)
	{
		while(NULL != fgets(line, sizeof(line), pp))
		{
			if(strstr(line, "BEEBA_AUTH_TEST_OK"))
			{
				ret = true;
				break;
			}
			else if(strstr(line, "BEEBA_AUTH_TEST_FAIL"))
			{
				ret = false;
				break;
			}
		}
		
		pclose(pp);
	}
	
 	return ret;
}

bool check_xunfei(void)
{
	bool ret = false;
	
	FILE *pp = NULL;
	char line[MAX_LINE] = { 0 };
	
	pp = popen("sfvoice test", "r");
	if (pp)
	{
		while(NULL != fgets(line, sizeof(line), pp))
		{
			if(strstr(line, "BEEBA_TEST_OK"))
			{
				ret = true;
				break;
			}
		}
		
		pclose(pp);
	}
	
	return ret;
}

#endif

