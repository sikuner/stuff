
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <linux/input.h>
#include <string.h>
#include <stdlib.h>
#include <sys/mman.h>

#include <json/json_object.h>
#include <json/json_tokener.h>

#include "cfgable.h"
#include "common.h"

int config_load(char *filename, struct resolvd_conf *conf)
{
	if(!filename || !conf)
		return -1;
	
	int ret = 0;
	
	int fd = -1;
	int file_length = -1;	
	int mem_length = -1;
	struct stat tStat;
	char *mm = NULL;
	char *content = NULL;
	
	int i = 0, j = 0;
	int dns_len = 0;
	int services_len = 0;
	int ips_len = 0;
	
	const char *nameserver = NULL;
	const char *name       = NULL;
	const char *host       = NULL;
	int         action     = 0;
	const char *param      = NULL;
	const char *addr       = NULL;
	
	struct json_object *content_obj    = NULL;
	struct json_object *dns_obj        = NULL;
	struct json_object *services_obj   = NULL;
	struct json_object *ips_obj        = NULL;
	struct json_object *ns_obj         = NULL;
	struct json_object *nameserver_obj = NULL;
	struct json_object *service_obj    = NULL;
	struct json_object *name_obj	   = NULL;
	struct json_object *host_obj  	   = NULL;
	struct json_object *action_obj     = NULL;
	struct json_object *param_obj 	   = NULL;
	struct json_object *ip_obj         = NULL;
	struct json_object *addr_obj	   = NULL;
	
	fd = open(filename, O_RDONLY);
	if (fd < 0)
	{
		fprintf(stderr, "can't open %s. \n", filename);
		return -1;
	}
	
	if(fstat(fd, &tStat) < 0)
	{
		fprintf(stderr, "can't get fstat. \n");
		ret = -1;
		goto conf_exit;
	}
	file_length = tStat.st_size;
	
	mm = (char *)mmap(NULL , file_length, PROT_READ, MAP_SHARED, fd, 0);
	if (mm == (char *)-1)
	{
		fprintf(stderr, "can't mmap file \n");
		ret = -1;
		goto conf_exit;
	}
	
	mem_length = tStat.st_size + 1;
	content = (char *)malloc(mem_length);
	if(!content)
	{
		fprintf(stderr, "Out of memory! \n");
		ret = -1;
		goto conf_exit;
	}
	memset(content, 0, mem_length);
	memcpy(content, mm, file_length);
	
	content_obj = json_tokener_parse(content);	
	if (!content_obj)
	{
		fprintf(stderr, "Failed to json_tokener_parse() \n");
		ret = -1;
		goto conf_exit;	
	}
	
	dns_obj = json_object_object_get(content_obj, "dns");
	if (!dns_obj)
	{
		fprintf(stderr, "Failed to json_object_object_get() \n");
		ret = -1;
		goto conf_exit;	
	}
	
	dns_len = json_object_array_length(dns_obj);
	for(i = 0; i < min(dns_len, DNS_MAX); i++)
	{
		ns_obj = json_object_array_get_idx(dns_obj, i);
		nameserver_obj = json_object_object_get(ns_obj, "nameserver");
		nameserver = json_object_get_string(nameserver_obj);
		
		conf->dns[i] = strdup(nameserver);
	}
	
	services_obj = json_object_object_get(content_obj, "services");
	if (!dns_obj)
	{
		fprintf(stderr, "Failed to json_object_object_get() \n");
		ret = -1;
		goto conf_exit;	
	}
	
	services_len = json_object_array_length(services_obj);
	for(i = 0; i < min(services_len, SERVICE_MAX); i++)
	{
		struct service *s = (struct service *)malloc(sizeof(struct service));
		if(!s)
		{
			fprintf(stderr, "Out of memory. \n");
			ret = -1;
			goto conf_exit; 
		}
		memset(s, 0, sizeof(struct service));
		
		service_obj = json_object_array_get_idx(services_obj, i);
		
		name_obj    = json_object_object_get(service_obj, "name");
		host_obj    = json_object_object_get(service_obj, "host");
		action_obj  = json_object_object_get(service_obj, "action");
		param_obj   = json_object_object_get(service_obj, "param");
		
		name   = json_object_get_string(name_obj);
		host   = json_object_get_string(host_obj);
		action = json_object_get_int(action_obj);
		param  = json_object_get_string(param_obj);
		
		s->name   = strdup(name);
		s->host   = strdup(host);
		s->action = action;
		s->param  = strdup(param);
		
		ips_obj = json_object_object_get(service_obj, "ip");
		ips_len = json_object_array_length(ips_obj);
		for(j = 0; j < min(ips_len, IPADDR_MAX); j++)
		{
			ip_obj   = json_object_array_get_idx(ips_obj, j);
			addr_obj = json_object_object_get(ip_obj, "addr");
			addr     = json_object_get_string(addr_obj);
			s->ipaddr[j] = strdup(addr);
		}
		
		conf->services[i] = s;
	}
	
	for(i = 0; NULL != conf->dns[i]; i++)
	{
		printf("nameserver[%d] : %s\n", i, conf->dns[i]);
	}
	for(i = 0; NULL != conf->services[i]; i++)
	{
		printf("services[%d].name   : %s\n", i, conf->services[i]->name);
		printf("services[%d].host   : %s\n", i, conf->services[i]->host);
		printf("services[%d].action : %d\n", i, conf->services[i]->action);
		printf("services[%d].param  : %s\n", i, conf->services[i]->param);
		
		for(j = 0; NULL != conf->services[i]->ipaddr[j]; j++)
		{
			printf("services[%d].ipaddr[%d] : %s\n", i, j, conf->services[i]->ipaddr[j]);
		}		
	}
	
conf_exit:
	if(content_obj)
	{
		json_object_put(content_obj);
		content_obj = NULL;
	}
	if (content)
	{	
		free(content);
		content = NULL;
	}
	if(mm)
	{
		munmap(mm, file_length);
		mm = NULL;
	}
	if(fd > 0)
	{
		close(fd);
		fd = -1;
	}
	
	return ret;
}

