
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
#include <libubus.h>

#include "network.h"
#include "common.h"

static bool network_conn_ok = false;		// false-没联网, true-已连接网络
static network_connected_handler connected_cb = NULL;

static int get_ip_sum(void) // 有效IP的个数(剔除127.0.0.1)
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

static char* get_wlan_interface(void)
{
	static char wlan_name[64] = WLAN_INTERFACE;
	return wlan_name;
}

#if 0
static int get_wireless_linked(void)
{
	
	return 1;
}
#else
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
#endif

static void network_daemon_cb(struct uloop_timeout *timeout)
{
	int linked = 0;
	int ip_sum = 0;
	bool conn_ok = false;
	
	linked = get_wireless_linked();
	ip_sum = get_ip_sum();
	
	conn_ok = (linked>0) && (ip_sum>0);
	
	if(!network_conn_ok && conn_ok) // 离线=>在线, 向管理者发送一个联网的事件
	{
		if(connected_cb)
		{
			connected_cb();
		}
	}
	
	if(network_conn_ok != conn_ok)
	{
		network_conn_ok = conn_ok;
	}
	
	DBG_PRINTF("network_conn_ok: %s \n", network_conn_ok?"OK":"NO");
	
	uloop_timeout_set(timeout, TIMER_NETWORK_PERIOD);
}
static struct uloop_timeout network_daemon_timer = {
	.cb = network_daemon_cb
};

void network_daemon(bool on)
{
	if(on)
	{
		uloop_timeout_set(&network_daemon_timer, TIMER_NETWORK_PERIOD);
	}
	else
	{
		uloop_timeout_cancel(&network_daemon_timer);
	}
}

bool network_get_connected(void)
{
	return network_conn_ok;
}

void network_set_connected_handler(network_connected_handler h) // 当网络连接成功,会触发事件
{
	connected_cb = h;
}

