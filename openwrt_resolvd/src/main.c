
#include <stdio.h>

#include "common.h"
#include "networking.h"
#include "cfgable.h"
#include "manager.h"

struct resolvd_db int_db;

struct resolvd_db* resolvd_get_db(void)
{
	return &int_db;
}

int main(int argc, char *argv[])
{
	DBG_PRINTF(" xxxxx Beeba DnsBetter Main (CompileDate: %s %s) xxxxx  \n", __DATE__, __TIME__);
	
#if 0	
	char *host = "comm.beeba.cn";
	char *ipaddr = "192.168.31.140";
	
	set_list_address(host, ipaddr);
	commit_dhcp();
	
	restart_dnsmasq();
	
	return 0;
#endif
	
#if 0	
	char *host = "www.mi.com";
	char *dns  = "180.76.76.76";
	
	struct ipaddrn *ian = nslookup(host, dns);
	while(ian)
	{
		fprintf(stderr, "ipaddr: %s \n", ian->ipaddr);
		ian = ian->ia_next;
	}
#endif	
	
#if 0
	char *filename = "./services.json";
	struct resolvd_config rscfg = { {0} };
	
	config_load(filename, &rscfg);
	
	return 0;
#endif
	
#if 0	
//	char *url = "http://download.beeba.cn/music/78/d8f4dcb3_78.bac";
//	char *url = "www.baidu.com";
	char *url = "www.google.com";
	int r = request_header(url);
	
	fprintf(stderr, "request_header r: %d \n", r);
	
	return 0;
#endif
	
	// ÍøÂçÕï¶Ï
#if 1	
	
	int ret = NET_ERR_SUCCESS;
	
	ret = check_ethernet();
	fprintf(stderr, "check_ethernet ret: %d \n", ret);
	
	ret = check_linked();
	fprintf(stderr, "check_linked ret: %d \n", ret);
	
	ret = check_internet();
	fprintf(stderr, "check_internet ret: %d \n", ret);
	
	ret = acquire_resolvd_conf(RESOLVD_CONF_URL, RESOLVD_CONF_PATH);
	fprintf(stderr, "acquire_services ret: %d \n", ret);
	
	struct resolvd_conf *conf = &(resolvd_get_db()->conf);
	ret = config_load(RESOLVD_CONF_PATH, conf);
	fprintf(stderr, "config_load ret: %d \n", ret);
	
	ret = check_hosts();
	fprintf(stderr, "check_hosts ret: %d \n", ret);
	
	if(ret > 0) // ³É¹¦
	{
		
	}
	else // ret == 0, 
	{
		
	}
	
#endif
	
	return 0;
}

