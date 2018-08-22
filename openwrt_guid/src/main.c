
#include "log.h"
#include "conf.h"
#include "server.h"
#include "widget.h"
#include "network.h"
#include "input_manager.h"
#include "view_manager.h"
#include "application.h"
#include "mpdclient.h"
#include "disp_manager.h"

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <libubox/uloop.h>

static void signal_handler(int sig)
{
	fprintf(stderr, "signal_handler sig: %d \n", sig);
	ubus_term();
}
static void signal_handler_sigusr1(int sig)
{
	fprintf(stderr, "signal_handler_sigusr1 sig: %d \n", sig);	
	uloop_timeout_set(&uloop_poweroff_timer, TIMER_ALIVE_UNIT);
}
static void signal_handler_sigusr2(int sig)
{
	fprintf(stderr, "signal_handler_sigusr2 sig: %d \n", sig);	
	remote_poweroff();
}

int main(int argc, char *argv[])
{
	DBG_PRINTF(" xxxxx Beeba Gui Main (CompileDate: %s %s) xxxxx  \n", __DATE__, __TIME__);
	
	if(2==argc && 0==strcmp(argv[1], "date"))	
		return 0;
	
	signal(SIGTERM, signal_handler);
	signal(SIGINT,  signal_handler);
	signal(SIGUSR1, signal_handler_sigusr1);	// 即时关机
	signal(SIGUSR2, signal_handler_sigusr2);	// 延时关机
	
	int ret = 0;
	
	mpdclient_check_directory();
	
	ret = uloop_init();
	if(ret < 0)
	{
		DBG_PRINTF("Failed to uloop_init \n");
		return -1;
	}
	
	ret = InputInit();
	if (ret)
	{
		DBG_PRINTF("InputInit error!\n");
		return -1;
	}
	
	ret = ViewsInit();
	if (ret)
	{
		DBG_PRINTF("ViewsInit error!\n");
		return -1;
	}
	
	ret = InputDevicesInit();
	if (ret)
	{
		DBG_PRINTF("InputDevicesInit error!\n");
		return -1;
	}
	
	ret = ubus_init();
	if(ret < 0)
	{
		DBG_PRINTF("ubus_init failed");
		return -1;
	}
	
	//////////////////////////////////////////
	////////////////TODO/////////////////////
	//////////////////////////////////////////
	AppInit();
	
	ret = WidgetInit();
	if (ret < 0)
	{
		DBG_PRINTF("WidgetInit error!\n");
		return -1;
	}
	
	DBG_PRINTF(" getApp()->upgrade: %d \n", getApp()->upgrade);
	if(getApp()->upgrade >= 0)
	{
		int bat = BatteryUpgradeCond();
		DBG_PRINTF(" BatteryUpgradeCond: %d \n", bat);
		if(bat > 0)
		{
			network_stop();
			SendUpgradeEvent();
		}
		else
		{
			getApp()->upgrade = -1;
			getApp()->curMode = MODE_WIFI;
			getApp()->curView = VIEW_SET;
		}
	}
	if(getApp()->upgrade < 0)
	{
		ShowLineinMode();
		ShowBluetoothMode();
	}
	
	startup();
	wifi_config();
	
	mpdclient_set_player_state_handler(mpd_state_proc);
	mpdclient_start_idleloop();
	
	//////////////////////////////////////////
	uloop_run();
	
	ubus_done();
	uloop_done();
	
	return 0;
}

