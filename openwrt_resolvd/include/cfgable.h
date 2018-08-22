
#ifndef __CFGABLE_H__
#define __CFGABLE_H__


#define DNS_MAX			16			// DNS最多设置个数
#define SERVICE_MAX		32			// Service最多设置个数
#define IPADDR_MAX		16			// 每个服务解析出来的IP个数

/* Error values */
enum service_state {
	
	STATE_FAIL = -1,
	STATE_UNCHECK = 0, 				// 默认为 未检查
	STATE_CHECK_OK = 1,				// 
	STATE_CHECK_FAIL = 2,			// 
	
};

struct service {
	
	char *name;
	char *host;
	int   action;
	char *param;
	char *ipaddr[IPADDR_MAX];
	
	char *ipok;
	int   state;
	
} __attribute__ ((packed));

struct resolvd_conf {
	
	char *dns[DNS_MAX];
	
	struct service *services[SERVICE_MAX];
	
};

struct host {
	
	char *host;
	char *ipaddr;
	
	int state;
	
	struct host *next;
};

struct resolvd_db {
	
	struct resolvd_conf conf;
	
	struct host *host_head;
	
};

#define RESOLVD_CONF_URL    "http://192.168.31.140/download/resolvd.json"
#define RESOLVD_CONF_PATH   "./resolvd.json"

int config_load(char *filename, struct resolvd_conf *conf);

struct resolvd_db* resolvd_get_db(void);

#endif // __CFGABLE_H__

