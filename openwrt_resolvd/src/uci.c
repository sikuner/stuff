
#include <uci.h>
#include <stdlib.h>

// 
// ����ʵ��
// 1.dbus�ӿ�,ֻ�����ø�����������ָ��dns����.û������hosts�Ľӿ�
// 2.��д�ļ�/etc/resolv.conf����dns,��д�ļ�/etc/hosts����hosts.
// 3.ͨ��uci��ɾ�б�server���ø�����������ָ��DNS����,��ɾ�б�address����hosts
// ���,ѡ��uci�ӿ�
// uci add_list dhcp.@dnsmasq[0].address='/comm.beeba.cn/192.168.31.140'
// uci add_list dhcp.@dnsmasq[0].address='/dev.voicecloud.cn/192.168.31.1'
// uci add_list dhcp.@dnsmasq[0].address='/api.beeba.cn/192.168.31.151'
//

#define DNSMASQ_ADDRESS     "dhcp.@dnsmasq[0].address"

void del_all_address(void)
{
	system("uci delete dhcp.@dnsmasq[0].address");
}

void del_list_address(char *host, char *ipaddr)
{
	char del_cmd[256] = { 0 };
	
	sprintf(del_cmd, "uci del_list %s=/%s/%s", DNSMASQ_ADDRESS, host, ipaddr);
	system(del_cmd);
}

void set_list_address(char *host, char *ipaddr)
{
	char add_cmd[256] = { 0 };
	
	del_list_address(host, ipaddr);
	
	sprintf(add_cmd, "uci add_list %s=/%s/%s", DNSMASQ_ADDRESS, host, ipaddr);
	system(add_cmd);	
}

void commit_dhcp(void)
{
	system("uci commit dhcp");
}

void restart_dnsmasq(void)
{
	system("/etc/init.d/dnsmasq restart");
}

