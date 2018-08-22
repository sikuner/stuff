
#ifndef __NETWORK_H__
#define __NETWORK_H__

#include <stdbool.h>

typedef enum {

	WIFI_UNKNOWN = 0,
	WIFI_NOCARD,
	WIFI_DOWN,
	
	WIFI_SMARTLINK_MODE,
	WIFI_AP_MODE	,
	WIFI_CONFIGURED
	
} E_WIFI_CONFIG;

char* get_valid_ip(void); 	// 仅仅得到一个有效的IP
char* get_wlan_essid(void); // 获取已连接wifi的热点名称

int get_wifi_config(void);
char* get_ap_tips(void);
bool kill_wifi_config(void);

int network_stop(void);
int network_start(void);

bool network_check_connected(int *link_state);
bool network_get_connected(void);

#endif // __NETWORK_H__

