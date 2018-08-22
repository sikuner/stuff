
#include "networking.h"
#include "common.h"
#include "cfgable.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <curl/curl.h>

// HOSTS, Beeba配置的服务, 根据从服务器配置的文件

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

static size_t write_data(void *ptr, size_t size, size_t nmemb, void *stream)
{
	size_t written = fwrite(ptr, size, nmemb, (FILE *)stream);
	return written;
}

int acquire_resolvd_conf(char *url, char *outfile)
{
	if(!url || !outfile)
	{
		fprintf(stderr, " Url or outfile is NULL! \n");
		return -1;
	}
	
	CURL *curl_handle = NULL;
	int res = NET_ERR_SUCCESS;
	
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
	
#if 0	
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
#endif
	
	if(header.memory)
	{
		free(header.memory);
		header.memory = NULL;
		header.size = 0;
	}
	
	return res;
}

int do_action(struct service *srv)
{
	int ret = -1;
	
	switch(srv->action)
	{
	case 0: // 默认处理方式
		{
			ret = request_header(srv->param);
		}
		break;
	case 101:
		{
			ret = request_header(srv->param);
			bool b = check_btools();
			if(0 == ret && b)
			{
				ret = 0;
			}
		}
		break;
	case 102:
		{
			if(check_xunfei())
			{
				ret = 0;
			}
		}
		break;
	default:
		{
			fprintf(stderr, "We does not support the action. \n");
		}
		break;
	}
	
	return ret;
}

int check_hosts(void)
{
	int si;
	int ret = 0;
	
	int sum = 0, count_ok = 0;
	
	struct resolvd_conf *conf = &(resolvd_get_db()->conf);
	
	for(si = 0; NULL != conf->services[si] && NULL != conf->services[si]->host; si++)
	{
		sum++;
		
		struct ipaddrn* ips = nslookup(conf->services[si]->host, "127.0.0.1");
		char ipaddr[32] = { 0 };
		
		if(ips)
		{   // 暂时只取第一个(一般程序,也是取DNS第一个IP)
			strcpy(ipaddr, ips->ipaddr);
			
			ret = do_action(conf->services[si]);
			
			if(0 == ret)
			{
				conf->services[si]->ipok = strdup(ipaddr);
				conf->services[si]->state = STATE_CHECK_OK;
				count_ok++;
			}
			else
			{
				conf->services[si]->state = STATE_CHECK_FAIL;
			}
		}
		
		fprintf(stderr, "conf->services[%d]->state: %d \n", si, conf->services[si]->state);
		fprintf(stderr, "conf->services[%d]->ipok: %s \n", si, conf->services[si]->ipok);
		
		fprintf(stderr, "conf->services[%d]->name: %s \n", si, conf->services[si]->name);
		fprintf(stderr, "conf->services[%d]->host: %s \n", si, conf->services[si]->host);
		fprintf(stderr, "conf->services[%d]->action: %d \n", si, conf->services[si]->action);
		fprintf(stderr, "conf->services[%d]->param: %s \n", si, conf->services[si]->param);
	}
	
	if(sum == count_ok)
	{
		ret = 0;
	}
	else
	{
		ret = count_ok;
	}
	
	return ret;
}

int repair_host(void)
{
	int si = 0, di = 0, ii = 0;
	int ret = 0;
	
	int sum = 0, count_ok = 0;
	
	char *host = NULL;
	char ipaddr[32] = { 0 };
	
	struct resolvd_conf *conf = &(resolvd_get_db()->conf);
	
	// 通过更换不同的DNS, 尝试修复
	for(si = 0; NULL != conf->services[si] && STATE_CHECK_FAIL == conf->services[si]->state; si++)
	{
		for(di = 0; NULL != conf->dns[di]; di++)
		{
			struct ipaddrn* ips = nslookup(conf->services[si]->host, conf->dns[di]);
			host = conf->services[si]->host;
			
			if(ips)
			{
				// 暂时只取第一个(一般程序,也是取DNS第一个IP)
				strcpy(ipaddr, ips->ipaddr);
				
				set_list_address(host, ipaddr);
				commit_dhcp();
				restart_dnsmasq();
				
				ret = do_action(conf->services[si]);
				
				if(0 == ret)
				{
					if(conf->services[si]->ipok)
					{
						free(conf->services[si]->ipok);
						conf->services[si]->ipok = NULL;
					}
					
					conf->services[si]->ipok = strdup(ipaddr);
					conf->services[si]->state = STATE_CHECK_OK;
					count_ok++;
				}
				else
				{
					conf->services[si]->state = STATE_CHECK_FAIL;
					
					del_list_address(host, ipaddr);
					commit_dhcp();
					restart_dnsmasq();
				}
			}
		}
	}
	
	// 通过配置文件中指定的IP列表, 尝试修复
	for(si = 0; NULL != conf->services[si] && STATE_CHECK_FAIL == conf->services[si]->state; si++)
	{
		host = conf->services[si]->host;
		
		for(ii = 0; NULL != conf->services[si]->ipaddr[ii]; ii++)
		{
			strcpy(ipaddr, conf->services[si]->ipaddr[ii]);
			
			set_list_address(host, ipaddr);
			commit_dhcp();
			restart_dnsmasq();
			
			ret = do_action(conf->services[si]);
			
			if(0 == ret)
			{
				if(conf->services[si]->ipok)
				{
					free(conf->services[si]->ipok);
					conf->services[si]->ipok = NULL;
				}
				
				conf->services[si]->ipok = strdup(ipaddr);
				conf->services[si]->state = STATE_CHECK_OK;
			}
			else
			{
				conf->services[si]->state = STATE_CHECK_FAIL;
				
				del_list_address(host, ipaddr);
				commit_dhcp();
				restart_dnsmasq();
			}
		}
	}
	
	return 0;
}

