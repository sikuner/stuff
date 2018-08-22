
#ifndef __NETWORKING_H__
#define __NETWORKING_H__

#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>

////////////////////////////////////////////////////////////////////////////////////////////

#if 0
/* Error values */
enum net_err_t {
	
	NET_ERR_FAIL = -1,
	NET_ERR_SUCCESS = 0,
	NET_ERR_NOCARD = 1,
	NET_ERR_IFDOWN = 2,
	NET_ERR_MACZERO = 3,
	NET_ERR_UNASSOCIATED = 4,
	NET_ERR_UNCONFIGURED = 5,
	NET_ERR_WITHOUT_IP = 6,
	NET_ERR_DIE_UDHCPC = 7,
	NET_ERR_PUB_NET_REFUSE = 8,
	NET_ERR_DNS_FAIL = 9,
	NET_ERR_FIREWALL_BLOCKED = 10,
	
};
#endif

////////////////////////////////////////////////////////////////////////////////////////////

struct ipaddrn {
	
	char ipaddr[32];
	struct ipaddrn* ia_next;
};

struct ipaddrn* nslookup(char *hostname, char *dns);

int get_ping_delay(char *host, int *delay);

int request_header(char *url);

bool check_btools(void);

bool check_xunfei(void);

////////////////////////////////////////////////////////////////////////////////////////////

void del_all_address(void);

void del_list_address(char *host, char *ipaddr);

void set_list_address(char *host, char *ipaddr);

void commit_dhcp(void);

void restart_dnsmasq(void);

int acquire_resolvd_conf(char *url, char *outfile);


#endif // __NETWORKING_H__

