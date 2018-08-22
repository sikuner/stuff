
#include "cfgable.h"
#include "common.h"
#include "networking.h"
#include "manager.h"

#include <stdlib.h>
#include <stdio.h>

#if 0
int main_loop(int argc, char *argv[])
{
	int si = 0, di = 0;
	int ret = 0;
	char *res_cfg = RESOLVD_CONF_PATH;
	
	struct resolvd_config *config = &(resolvd_get_db()->cfg);
	
	config_load(res_cfg, config);
	
	for(si = 0; NULL != config->services[si]->host; si++)
	{
		for(di = 0; NULL != config->dns[di]; di++)
		{
			struct ipaddrn* ips = nslookup(config->services[si]->host, config->dns[di]);
			char *host = config->services[si]->host;
			char ipaddr[32] = { 0 };
			
			if(ips)
			{   
				// 暂时只取第一个(一般程序,也是取DNS第一个IP)
				strcpy(ipaddr, ips->ipaddr);
				
				set_list_address(host, ipaddr);
				commit_dhcp();
				restart_dnsmasq();
				
				switch(config->services[si]->action)
				{
				case 0:
					{
						ret = request_header(config->services[si]->param);
					}
					break;
				case 101:
					{
						ret = request_header(config->services[si]->param);
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
						else
						{
							ret = -1;
						}
					}
					break;
				default:
					{
						fprintf(stderr, "We does not support the action. \n");
					}
					break;
				}
				
				if(0 == ret)
				{
					config->services[si]->ipok = strdup(ipaddr);
				}
				else
				{
					del_list_address(host, ipaddr);
					commit_dhcp();
					restart_dnsmasq();
				}
			}
		}
	}
	
	return 0;
}
#endif

