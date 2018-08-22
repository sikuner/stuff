
#include "conf.h"
#include "common.h"
#include "application.h"
#include "input_manager.h"
#include "view_manager.h"

static App g_tApp = { 0 };

App* getApp()
{
	return &g_tApp;
}

int AppInit(void)
{	
	int alarm_start = -1;
	int alarm_stop = -1;
	
	g_tApp.has_bt = check_has_bt();
	
	g_tApp.wifi_config = WIFI_UNKNOWN;
	g_tApp.wifi_linked_state = WIFI_HIDE;
	g_tApp.wifi_conn_ok = false;
	
	g_tApp.locked = STATE_UNLOCKED;
	
	g_tApp.download = DOWN_UNKNOWN;
	
	alarm_start = get_alarm_start();
	alarm_stop  = get_alarm_stop();
	
	if (alarm_start>0 || alarm_stop>0)
		g_tApp.alarm = STATE_ALARMED;
	else
		g_tApp.alarm = STATE_UNALARMED;
	
	g_tApp.bluetooth = get_bt_state();
	
	g_tApp.linein  = get_linein_state();
	g_tApp.airplay = get_airplay_state();
	
	g_tApp.volume = mpdclient_get_volume();
	g_tApp.channel = -1; // [0, 4] 儿歌,英语,故事,我的录音,我的缓存
	
	g_tApp.charge  = get_charge_state();
	
	g_tApp.mpd_state = PLAYER_STATE_STOP;
	g_tApp.mpd_error = NULL;
	
	g_tApp.low_count = 0;
	g_tApp.off_count = 0;
	g_tApp.off_remote = 0;
	g_tApp.charge_off_count = 0;
	g_tApp.screensaver = 0;
	
	g_tApp.upgrade = check_upgrade(); // 固件更新
	
	if(g_tApp.upgrade >= 0)
	{
		g_tApp.curMode = MODE_UNKNOWN;
		g_tApp.curView = VIEW_UNKNOWN;
		
//		network_stop();
	}
	else if(g_tApp.linein == SND_LINEIN_IN)
	{
		g_tApp.curMode = MODE_LINEIN;
		g_tApp.curView = VIEW_LINEIN;
		
		network_stop();
	}
	else if(BT_STATE_STARTED == g_tApp.bluetooth
	     || BT_STATE_CONNECTED == g_tApp.bluetooth)
	{
		g_tApp.curMode = MODE_BLUETOOTH;
		g_tApp.curView = VIEW_BLUETOOTH;
		
		network_stop();		
	}
	else
	{
		g_tApp.curMode = MODE_WIFI;
		g_tApp.curView = VIEW_SET;
		
		network_start();
	}
	
	return 0;
}

