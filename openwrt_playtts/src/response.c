
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <curl/curl.h>
#include <json/json_object.h>
#include <json/json_tokener.h>

#include "cfgable.h"
#include "common.h"
#include "tts.h"
#include "mpdclient.h"

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

static size_t read_memory(void *ptr, size_t size, size_t nmemb, void *userp)
{
	WriteThis *pooh = (WriteThis *)userp;
	
	if(size*nmemb < 1)
	{
		return 0;
	}
	
	if(pooh->sizeleft) 
	{
		*(char *)ptr = pooh->readptr[0]; 	/* copy one single byte */
		pooh->readptr++;                 	/* advance pointer */
		pooh->sizeleft--;                	/* less data left */
		
		return 1;                        	/* we return 1 byte at a time! */
	}
	
	return 0;                          		/* no more data left to deliver */
}

static int response_status(char *url, char *data)
{
	if(!url || !data)
	{
		fprintf(stderr, " Url or Data is NULL! \n");
		return -1;
	}
	
	WriteThis pooh;
	MemoryStruct resp;
	
	CURL *curl;
	int res = PTTS_FAIL;
	
	int code = 0;
	struct json_object *resp_obj = NULL;
	struct json_object *code_obj = NULL;
	
	pooh.readptr = data;
	pooh.sizeleft = strlen(data);
	
	resp.memory = malloc(1);
	resp.size = 0;
	
	curl_global_init(CURL_GLOBAL_ALL);
	
	curl = curl_easy_init();
	if(curl)
	{
		curl_easy_setopt(curl, CURLOPT_URL, url);
		
		curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
	    curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
	    curl_easy_setopt(curl, CURLOPT_PUT, 1L);
		curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
		curl_easy_setopt(curl, CURLOPT_TIMEOUT, 60L);
		curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 30L);
		
		curl_easy_setopt(curl, CURLOPT_READFUNCTION, read_memory);
		curl_easy_setopt(curl, CURLOPT_READDATA, (void *)&pooh);
	    curl_easy_setopt(curl, CURLOPT_INFILESIZE, pooh.sizeleft);
		
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_memory);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&resp);
		
		res = curl_easy_perform(curl);
		if(res != CURLE_OK)
		{
			fprintf(stderr, "curl_easy_perform() failed res(%d): %s\n",
				res, curl_easy_strerror(res));
		}
		
		curl_easy_cleanup(curl);
	}
	
	curl_global_cleanup();
	
	DBG_PRINTF(" Size: %d, Resp : %s \n", resp.size, resp.memory);
	if(resp.size > 4)
	{
		resp_obj = json_tokener_parse(resp.memory);
		if (NULL == resp_obj)
		{
			fprintf(stderr, "Failed to json_tokener_parse() \n");
			goto json_fail;
		}
		code_obj = json_object_object_get(resp_obj, "code");
		if (NULL == code_obj)
		{
			fprintf(stderr, "Failed to json_object_object_get() \n");
			goto json_fail;
		}
		code  = json_object_get_int(code_obj);
		
	json_fail:
		DBG_PRINTF(" code  : %d \n", code);
		switch(code)
		{
		case 10000:
			res = PTTS_OK;
			break;				
		case 41103:
			res = PTTS_CBED;
			break;
		case 41111:
			res = PTTS_NOREC;
			break;
		default:
			res = PTTS_FAIL;
			break;
		}
	}
	
	if(resp_obj)
	{
		json_object_put(resp_obj);
		resp_obj = NULL;
	}
	if(resp.memory)
	{
		free(resp.memory);
		resp.memory = NULL;
		resp.size = 0;
	}
	
	return res;
}

int response_tts(void)
{
	char url[MAX_URL]       = { 0 };
	char data[MAX_LINE] 	= { 0 };
	int progress_time = -1;
	
	sprintf(url, TTS_RESPONSE_URL"%d", getConfig()->logid);
	
	progress_time = time(NULL) - getConfig()->start_time;
	sprintf(data, "data={\"progress_time\":\"%d\",\"volume\":\"%s\",\"sysver\":\"%s\",\"response_status\":\"%d\"}", 
		progress_time,
		getConfig()->volume,
		getConfig()->sysver,
		getConfig()->resp_status);
	
	DBG_PRINTF(" Url  : %s \n", url);
	DBG_PRINTF(" Data : %s \n", data);
	
	return response_status(url, data);
}

