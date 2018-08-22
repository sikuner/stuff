
#include "common.h"
#include "networking.h"

// 检查 DHCP服务, DNS服务

//	pgrep -f udhcpc | wc -l

static int udhcpc_exist(void)
{
	char cmd_line[512] = { 0 };
	FILE *pp = NULL;	
	char line[MAX_LINE] = { 0 };
	int udhcpc = 0;
	
	sprintf(cmd_line, "pgrep -f udhcpc | wc -l");
	pp = popen(cmd_line, "r");
	if (pp) 
	{
		if(NULL != fgets(line, sizeof(line), pp))
		{
			udhcpc = atoi(line);
		}
		
		pclose(pp);
	}
	
	return udhcpc;
}

// get_ping_delay(host, delay), delay == 0x0FFFFFFF 不通
int check_internet(void)
{
	int ret = NET_ERR_SUCCESS, r = 0;
	char *host;
	int delay;
	
	r = udhcpc_exist();
	if(r <= 0) // dhcp客户端 没有运行
	{
		ret = NET_ERR_DIE_UDHCPC; // dhcp客户端 没有运行
		goto internet_error;
	}
	
	host = "180.76.76.76"; // 百度公共DNS, "指引"未来
	r = get_ping_delay(host, &delay);
	if(0 != r)
	{
		ret = NET_ERR_FAIL;		// 调用失败
		goto internet_error;
	}
	if(0x0FFFFFFF == delay)
	{
		ret = NET_ERR_PUB_NET_REFUSE; // 公网访问失败
		goto internet_error;
	}
	
	host = "www.baidu.com";
	if(NULL == nslookup(host, "127.0.0.1"))
	{
		ret = NET_ERR_DNS_FAIL; // dns失败
		goto internet_error;
	}
	r = request_header(host);
	if(r < 0)
	{
		ret = NET_ERR_FIREWALL_BLOCKED; // firewall_blocked, 防火墙拦截
		goto internet_error;
	}
	
internet_error:
	
	return ret;
}

