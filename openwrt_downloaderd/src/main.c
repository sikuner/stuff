
#include "db.h"
#include "usrv.h"
#include "task.h"
#include "common.h"
#include "network.h"
#include "manager.h"

#include <stdio.h>
#include <libubox/uloop.h>

static void signal_handler(int sig)
{
	fprintf(stderr, "beeba down signal_handler sig: %d \n", sig);
	ubus_term();
}

int main(int argc, char *argv[])
{
	DBG_PRINTF(" xxxx Beeba Down xxxx \n");
	
	signal(SIGTERM, signal_handler);
	signal(SIGINT,  signal_handler);
	
	int ret = 0;
	
	down_check_directory();
	
	ret = uloop_init();
	if(ret < 0)
	{
		DBG_PRINTF("Failed to uloop_init \n");
		return -1;
	}
	
	ret = ubus_init();
	if(ret < 0)
	{
		DBG_PRINTF("ubus_init failed \n");
		return -1;
	}
	
	ret = db_init();
	if(ret < 0)
	{
		DBG_PRINTF("db_init failed \n");
		return -1;
	}
	
	//////////////////////////////////////////
	////////////////TODO/////////////////////
	//////////////////////////////////////////
	sharedns_init();
	
	manager_init();
	network_daemon(true);
	
	//////////////////////////////////////////	
	uloop_run();
	
	sharedns_cleanup();
	
	ubus_done();
	uloop_done();
	
	db_release();
	
	return 0;
}

