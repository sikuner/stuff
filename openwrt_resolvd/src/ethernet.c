
#include "common.h"
#include "networking.h"

#include <unistd.h>
#include <sys/types.h>			/* See NOTES */
#include <sys/socket.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <libubox/uloop.h>
#include <dirent.h>

// http://jingyan.baidu.com/article/d45ad148e1a8f869552b80a5.html
// 网络硬件配置

// /sys/class/net/wlan0/address
#define HWADDR_PATH 	"/sys/class/net/wlan0/address"

//检查mac是否合法,不全为0,845D开头 等等. get_sn.sh

static int get_wlan0_up(void)
{
	int ret = 0;
	FILE *pp = NULL;
	char line[MAX_LINE] = { 0 };
	
	pp = popen("ifconfig", "r");
	if (pp)
	{
		while(NULL != fgets(line, sizeof(line), pp))
		{
			if(strstr(line, "wlan0"))
			{
				ret = 1;
				break;
			}
		}
		
		pclose(pp);
	}
	
 	return ret;
}

int check_ethernet(void)
{
	int ret = NET_ERR_SUCCESS;
	
	ret = access(HWADDR_PATH, F_OK);
	if(0 != ret) // 文件不存在
	{
		ret = NET_ERR_NOCARD;
		goto eth_error;
	}
	
	if(get_wlan0_up() < 1)
	{
		ret = NET_ERR_IFDOWN;
		goto eth_error;
	}
	
eth_error:
	
	return ret;
}

// 链接热点: -1 -- 错误, 0 -- 链接失败, 1 -- 链接成功
static int get_wireless_linked(void)
{
	char cmd_line[512] = { 0 };
	FILE *pp = NULL;
	int linked = 1;
	
	sprintf(cmd_line, "iwconfig %s | grep \"Access Point:\" | awk -F\'Access Point:\' \'{print $2}\' 2>/dev/null", "wlan0");
//	fprintf(stderr, "cmd_line: %s \n", cmd_line);
	pp = popen(cmd_line, "r");
	if (NULL == pp) 
		return -1;
	
	char line[MAX_LINE] = { 0 };
	while(NULL != fgets(line, sizeof(line), pp))
	{
		if(NULL != strstr(line, "Not-Associated"))
		{
			linked = 0;
			break;
		}
	}
	
	pclose(pp);
	
	return linked;
}

static int get_ip_sum() // 有效IP的个数(剔除127.0.0.1)
{
	struct ifreq ifq[16];
	struct ifconf ifc;	
	int fd = 0;
	int total = 0, sum = 0, i = 0;
	char *ip = NULL;
	
	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if(fd < 0)
		return -1;
	ifc.ifc_len = sizeof(ifq);
	ifc.ifc_buf = (caddr_t)ifq;
	if(ioctl(fd, SIOCGIFCONF, (char *)&ifc) < 0)
		return -1;
	
	total = ifc.ifc_len / sizeof(struct ifreq);
	if(ioctl(fd, SIOCGIFADDR, (char *)&ifq[total-1]) < 0)
		return -1;
	
	close(fd);
	
	for(i = 0; i < total; i++)
	{
		ip = inet_ntoa(((struct sockaddr_in*)(&ifq[i].ifr_addr))->sin_addr);
		if(strcmp(ip, "127.0.0.1") != 0)
			sum++;
	}
	
	return sum;
}

typedef enum {

	WIFI_UNKNOWN = 0,
	WIFI_NOCARD,
	WIFI_DOWN,
	
	WIFI_SMARTLINK_MODE,
	WIFI_AP_MODE	,
	WIFI_CONFIGURED
	
} E_WIFI_CONFIG;

static int get_wifi_config(void)
{
	int mode = WIFI_UNKNOWN;
	
	FILE *pp = popen("wifi_config.sh get_mode", "r");
	if (NULL == pp) 
		return -1;
	
	char line[MAX_LINE] = { 0 };
	while(NULL != fgets(line, sizeof(line), pp))
	{
		fprintf(stderr, "xxx get_wifi_config: %s \n", line);
		
		if(NULL != strstr(line, "nocard"))
		{
			mode = WIFI_NOCARD;
		}
		else if(NULL != strstr(line, "down"))
		{
			mode = WIFI_DOWN;
		}
		else if(NULL != strstr(line, "smartlink_mode"))
		{
			mode = WIFI_SMARTLINK_MODE;
		}
		else if(NULL != strstr(line, "ap_mode"))
		{
			mode = WIFI_AP_MODE;
		}
		else if(NULL != strstr(line, "configured"))
		{
			mode = WIFI_CONFIGURED;
		}
	}
	
	pclose(pp);
	
	return mode;
}

int check_linked(void)
{
	int ret = NET_ERR_SUCCESS;
	
	if(WIFI_CONFIGURED != get_wifi_config())
	{
		ret = NET_ERR_UNCONFIGURED;
		goto linked_error;
	}
	
	if(1 != get_wireless_linked())
	{
		ret = NET_ERR_UNASSOCIATED;
		goto linked_error;
	}
	
	if(1 > get_ip_sum())
	{
		ret = NET_ERR_WITHOUT_IP;
		goto linked_error;
	}
	
linked_error:	
	
	return ret;
}

