
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

#include "conf.h"
#include "utf8_strings.h"
#include "network.h"
#include "view_manager.h"
#include "widget.h"
#include "application.h"
#include "common.h"

int get_wifi_config(void)
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

char* get_ap_tips(void)
{
	static char ap_tips[MAX_LINE] = { 0 };
	
	char line[MAX_LINE] = { 0 };
	
	FILE *pp = popen("uci get wireless.@wifi-iface[0].ssid", "r");
	if (NULL == pp) 
		return NULL;
	
	while(NULL != fgets(line, sizeof(line), pp))
	{
		fprintf(stderr, "ap_name: %s \n", line);
		trim(line);
		if(strlen(line) > 0)
			break;
	}
	
	pclose(pp);
	
	sprintf(ap_tips, STRING_LINK_AP, line);
	
	return ap_tips;
}

bool kill_wifi_config(void)
{
	pid_t pid = vfork();
	if (pid < 0)
	{
		fprintf(stderr, "fork error!");
		return false;
	}
	if (pid > 0) // > 0, 处于父进程中. add后返回
	{
		return true;
	}
	
	execlp("killall", "killall", "wifi_config.sh", NULL);
	exit(1);
}

static char* get_wlan_interface(void)
{
	static char wlan_name[64] = WLAN_INTERFACE;
	return wlan_name;
}

// 链接热点: -1 -- 错误, 0 -- 链接失败, 1 -- 链接成功
static int get_wireless_linked(void)
{
	char cmd_line[512] = { 0 };
	FILE *pp = NULL;
	int linked = 1;
	
	sprintf(cmd_line, "iwconfig %s | grep \"Access Point:\" | awk -F\'Access Point:\' \'{print $2}\' 2>/dev/null", get_wlan_interface());
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

static int get_wireless_quality(void)
{
	char cmd_line[512] = { 0 };
	FILE *pp = NULL;	
	char line[MAX_LINE] = { 0 };
	int percent = 0;
	
	sprintf(cmd_line, "wifi_signal_percent.sh");
	pp = popen(cmd_line, "r");
	if (pp) 
	{
		if(NULL != fgets(line, sizeof(line), pp))
		{
			percent = atoi(line);
		}
		
		pclose(pp);
	}
	
	return percent;
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

char* get_valid_ip(void) // 仅仅得到一个有效的IP
{
	static char vip[MAX_LINE] = { 0 };
	
	struct ifreq ifq[16];
	struct ifconf ifc;	
	int fd = 0;
	int total = 0, i = 0;
	char *ip = NULL;
	
	memset(vip, 0, sizeof(vip));
	
	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if(fd < 0)
		return NULL;
	ifc.ifc_len = sizeof(ifq);
	ifc.ifc_buf = (caddr_t)ifq;
	if(ioctl(fd, SIOCGIFCONF, (char *)&ifc) < 0)
		return NULL;
	
	total = ifc.ifc_len / sizeof(struct ifreq);
	if(ioctl(fd, SIOCGIFADDR, (char *)&ifq[total-1]) < 0)
		return NULL;
	
	close(fd);
	
	for(i = 0; i < total; i++)
	{
		ip = inet_ntoa(((struct sockaddr_in*)(&ifq[i].ifr_addr))->sin_addr);
		if(strcmp(ip, "127.0.0.1") != 0)
		{
			fprintf(stdout, "get_valid_ip ip: %s \n", ip);
			strcpy(vip, ip);
			break;
		}
	}
	
	return vip;
}

char* get_wlan_essid(void) // 获取已连接wifi的热点名称
{
	static char essid[MAX_LINE] = { 0 };
	
	char cmd_line[512] = { 0 };
	char line[MAX_LINE] = { 0 };
	FILE *pp = NULL;
	
	memset(essid, 0, sizeof(essid));
	
	sprintf(cmd_line, "iwconfig %s | grep ESSID | awk -F'\"' '{print $2}' 2>/dev/null", get_wlan_interface());
//	fprintf(stderr, "cmd_line: %s \n", cmd_line);
	pp = popen(cmd_line, "r");
	if (NULL == pp) 
		return NULL;
	
	if(NULL != fgets(line, sizeof(line), pp))
	{
		strcpy(essid, line);
	}
	
	trim(essid);
	
	pclose(pp);
	pp = NULL;
	
	fprintf(stderr, "essid: %s \n", essid);
	return essid;
}

int network_stop(void)
{
//	fprintf(stderr, " xxxx network_stop() xxxx \n");
	
	pid_t pid = vfork();
	if (pid < 0)
	{
		fprintf(stderr, "fork error!");
		return -1;
	}
	if (pid > 0) // > 0, 处于父进程中.
	{
		return 0;
	}
	
//	/etc/init.d/network stop; ifconfig wlan0 down
	execlp("/bin/sh", "/bin/sh", "-c", "/etc/init.d/network stop; ifconfig wlan0 down", NULL); 
	exit(1);
}

int network_start(void)
{
//	fprintf(stderr, " xxxx network_start() xxxx \n");
	
	pid_t pid = vfork();
	if (pid < 0)
	{
		fprintf(stderr, "fork error!");
		return -1;
	}
	if (pid > 0) // > 0, 处于父进程中.
	{
		return 0;
	}
	
//	ifconfig wlan0 up; /etc/init.d/network start
	execlp("/bin/sh", "/bin/sh", "-c", "ifconfig wlan0 up; /etc/init.d/network start", NULL); 
	exit(1);
}

////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////

bool network_check_connected(int *link_state)
{
	bool conn_ok = false;
	int linked = 0;
	int percent = 0;
	
	linked = get_wireless_linked();
	conn_ok = (linked>0) && (get_ip_sum()>0);
	if(link_state)
	{
		if(conn_ok)
		{
			percent = get_wireless_quality();
			if(percent < 0)
			{
				*link_state = 3;
			}
			else
			{
				if (percent > 80)
					*link_state = 3;
				else if (percent > 40)
					*link_state = 2;
				else
					*link_state = 1;
			}
		}
		else
		{
			*link_state = 0;
		}
	}
	
	return conn_ok;
}

bool network_get_connected(void)
{
	return getApp()->wifi_conn_ok;
}

