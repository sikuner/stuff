
#include "log.h"
#include "conf.h"
#include "view_manager.h"
#include "application.h"
#include "network.h"
#include "utf8_strings.h"
#include "network.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <libubox/uloop.h>

//////////////////////////////////////////////////////////////////////////////////
static bool wifi_first		 = true;
static bool wifi_first_done	 = false;

static void wifi_reconnect_cb(struct uloop_timeout *timeout)
{
	int link_state = 0;
	bool conn_ok = network_check_connected(&link_state);
	
	ShowWifiV(link_state, false);
	
	if(!wifi_first_done)
	{
		wifi_first_done = true;
		
		if(VIEW_SET == getApp()->curView)
		{
			if(!conn_ok)
				mpdclient_local_load(DOWNLOADS);
			uloop_timeout_set(&uloop_play_timer, TIMER_ALIVE_UNIT);
		}
	}
}
static struct uloop_timeout wifi_reconnect_timer = {
	.cb = wifi_reconnect_cb
};

static void wifi_daemon_cb(struct uloop_timeout *timeout)
{
	int link_state = 0;
	bool conn_ok = network_check_connected(&link_state);
	
	if(wifi_first)
	{
		wifi_first = false;
		if(!conn_ok) // 没连上
		{
			ShowWifiV(link_state, true);
			
			if(VIEW_SET == getApp()->curView)
			{
				ShowLine1(STRING_BLANK);
				ShowLine2(STRING_CONNECTING);				
				ShowLine3(STRING_BLANK);
			}
			uloop_timeout_set(&wifi_reconnect_timer, TIMER_RECONNECT_TIMEOUT);
		}
		else // 已连上网络
		{
			ShowWifiV(link_state, false);
			wifi_first_done = true;
			
			if(VIEW_SET == getApp()->curView)
			{
				ShowLine2(get_wlan_essid());
				ShowLine1(STRING_CONNECT_OK);
				ShowLine3(STRING_CONNECT_OK_TIPS);
				uloop_timeout_set(&uloop_play_timer, CONN_OK_PLAY_PERIOD);
			}
		}
	}
	else // 不是第一次
	{
		if(!wifi_first_done && conn_ok) // 离线 => 第一次在线
		{
			ShowWifiV(link_state, false);
			uloop_timeout_cancel(&wifi_reconnect_timer);
			wifi_first_done = true;
			
			if(VIEW_SET == getApp()->curView)
			{
				ShowLine2(get_wlan_essid());
				ShowLine1(STRING_CONNECT_OK);
				ShowLine3(STRING_CONNECT_OK_TIPS);
				uloop_timeout_set(&uloop_play_timer, CONN_OK_PLAY_PERIOD);
			}
		}
		
		if(link_state != getApp()->wifi_linked_state)
		{
			ShowWifiV(link_state, false);
		}
	}
	
	DBG_PRINTF("link_state: %d, conn_ok: %d \n", link_state, conn_ok);
	
	getApp()->wifi_linked_state = link_state;
	getApp()->wifi_conn_ok = conn_ok;
	uloop_timeout_set(timeout, TIMER_NETWORK_PERIOD);
}
static struct uloop_timeout wifi_daemon_timer = {
	.cb = wifi_daemon_cb
};

int wifi_daemon(bool on)
{
	uloop_timeout_cancel(&wifi_daemon_timer);
	uloop_timeout_cancel(&wifi_reconnect_timer);
	
	if(on)
		uloop_timeout_set(&wifi_daemon_timer, 5);
	
	return 0;
}

static void wifi_config_cb(struct uloop_timeout *timeout)
{
	int wifi_config = get_wifi_config();
	if(getApp()->wifi_config != wifi_config)
	{
		getApp()->wifi_config = wifi_config;
		
		if(WIFI_UNKNOWN==wifi_config)
		{
			getApp()->wifi_linked_state = WIFI_HIDE;
			ShowWifiV(getApp()->wifi_linked_state, false);
			
			if(VIEW_SET == getApp()->curView)
			{
				ShowLine1(STRING_BLANK);
				ShowLine2(STRING_WIFI_UNKNOWN);
				ShowLine3(STRING_BLANK);
			}
		}
		else if(WIFI_NOCARD==wifi_config)
		{
			getApp()->wifi_linked_state = WIFI_HIDE;
			ShowWifiV(getApp()->wifi_linked_state, false);	
			
			if(VIEW_SET == getApp()->curView)
			{
				ShowLine1(STRING_BLANK);
				ShowLine2(STRING_WIFI_NOCARD);
				ShowLine3(STRING_BLANK);
			}
		}
		else if(WIFI_DOWN==wifi_config)
		{
			getApp()->wifi_linked_state = WIFI_HIDE;
			ShowWifiV(getApp()->wifi_linked_state, false);
			
			if(VIEW_SET == getApp()->curView)
			{
				ShowLine1(STRING_BLANK);
	//			ShowLine2(STRING_WIFI_DOWN);
				ShowLine2(STRING_BLANK);
				ShowLine3(STRING_BLANK);
			}
		}
		else if(WIFI_SMARTLINK_MODE==wifi_config)
		{
			getApp()->wifi_linked_state = WIFI_FAIL;
			ShowWifiV(getApp()->wifi_linked_state, false);
			
			if(VIEW_SET == getApp()->curView)
			{
				ShowLine1(STRING_WAITING_NETWORKING);
				ShowLine2(STRING_SMARTLINK_MODE);			
				ShowLine3(STRING_BLANK);
			}
		}
		else if(WIFI_AP_MODE==wifi_config)
		{
			getApp()->wifi_linked_state = WIFI_FAIL;
			ShowWifiV(getApp()->wifi_linked_state, false);
			
			if(VIEW_SET == getApp()->curView)
			{
				ShowLine2(get_ap_tips());
				ShowLine1(STRING_NETWORKING_STEP);
				ShowLine3(STRING_BROWSER_ACCESS);
			}
		}
		else if(WIFI_CONFIGURED == wifi_config) // 网络已配置
		{
			if(VIEW_SET == getApp()->curView)
				ShowLine3(STRING_BLANK);
			
			wifi_daemon(true);
			return;
		}
	}
	
	uloop_timeout_set(timeout, TIMER_NETWORK_PERIOD);
}
static struct uloop_timeout wifi_config_timer = {
	.cb = wifi_config_cb
};

void wifi_config(void)
{
	wifi_first		 = true;
	wifi_first_done  = false;
	
	getApp()->wifi_config = WIFI_UNKNOWN;	
	getApp()->wifi_linked_state = WIFI_HIDE;
	getApp()->wifi_conn_ok = false;
	
	wifi_daemon(false);
	uloop_timeout_set(&wifi_config_timer, 5);
}

//////////////////////////////////////////////////////////////////////////////////

static bool dispatchKeyEventSet(PT_InputEvent ptInputEvent);
static bool onKeyEventSet(PT_InputEvent ptInputEvent);
static void EnterSet(void);
static void ExitSet(void);

static T_ViewAction g_tSetViewAction = {
	.id 				= VIEW_SET,
	.dispatchKeyEvent 	= dispatchKeyEventSet,
	.onKeyEvent 		= onKeyEventSet,
	.Enter				= EnterSet,
	.Exit				= ExitSet
};

static void clear_set(void);

static struct uloop_process sl_proc;
static void sl_proc_cb(struct uloop_process *p, int ret)
{	
	bool smartlink_mode = false;
	int count = 30;
	while (count-- > 0)
	{
		if(WIFI_SMARTLINK_MODE == get_wifi_config())
		{
			smartlink_mode = true;
			break;
		}
		
		usleep(100000);
	}
	
//	if(smartlink_mode)
//	{
		wifi_config();
//	}
}
static int start_smartlink(void)
{
	clear_set();
	
	pid_t pid = vfork();
	if (pid < 0)
	{
		fprintf(stderr, "fork error!");
		return -1;
	}
	if (pid > 0) // > 0, 处于父进程中. add后返回
	{
		sl_proc.pid = pid;
		sl_proc.cb = sl_proc_cb;
		uloop_process_add(&sl_proc);
		return 0;
	}
	
	execlp("reset_network.sh", "reset_network.sh", "start_smartlink", NULL);
	exit(1);
}

static struct uloop_process ap_proc;
static void ap_proc_cb(struct uloop_process *p, int ret)
{
	bool ap_mode = false;
	int count = 30;
	while (count-- > 0)
	{
		if(WIFI_AP_MODE == get_wifi_config())
		{
			ap_mode = true;
			break;
		}
		
		usleep(100000);
	}
	
//	if(ap_mode)
//	{
		wifi_config();
//	}
}
int start_ap(void)
{
	clear_set();
	
	pid_t pid = vfork();
	if (pid < 0)
	{
		fprintf(stderr, "fork error!");
		return -1;
	}
	if (pid > 0) // > 0, 处于父进程中. add后返回
	{
		ap_proc.pid = pid;
		ap_proc.cb = ap_proc_cb;
		uloop_process_add(&ap_proc);
		return 0;
	}
	
	execlp("reset_network.sh", "reset_network.sh", "start_ap", NULL);
	exit(1);
}
static void clear_set(void)
{
	uloop_timeout_cancel(&uloop_play_timer);
	uloop_timeout_cancel(&wifi_config_timer);
	uloop_timeout_cancel(&wifi_daemon_timer);
	uloop_timeout_cancel(&wifi_reconnect_timer);
	
	if(sl_proc.pending)
	{
		uloop_process_delete(&sl_proc);
		kill(sl_proc.pid, SIGKILL);
	}
	if(ap_proc.pending)
	{
		uloop_process_delete(&ap_proc);
		kill(ap_proc.pid, SIGKILL);
	}
}

static bool reset_system(void)
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
	
	execlp("reset_system.sh", "reset_system.sh", NULL);
	exit(1);
}
static void uloop_long_reset_cb(struct uloop_timeout *timeout)
{
	Rect dst = { .x = 0, .y = 0, .w = WINDOW_WIDTH, .h = WINDOW_HEIGHT };
	Toast(&dst, STRING_RESET,  DEFAULT_FONT_SIZE, TOAST_LONG_PERIOD);
	
	InputDevicesExit();
	WidgetExit();
	reset_system();
}
static struct uloop_timeout uloop_long_reset_timer = {
	.cb = uloop_long_reset_cb
};

static bool dispatchKeyEventSet(PT_InputEvent ptInputEvent)
{
	if (BTN_SET != ptInputEvent->iCode)
	{
		return false;
	}
	
	if(MODE_WIFI != getApp()->curMode)
	{
		if(ACTION_DOWN == ptInputEvent->iValue)
		{
			Toast(NULL, get_mode_strerror(), DEFAULT_FONT_SIZE, TOAST_SHORT_PERIOD);
		}
		
		return true;
	}
	
	return onKeyEventSet(ptInputEvent);
}

static bool onKeyEventSet(PT_InputEvent ptInputEvent)
{
	if(ACTION_DOWN == ptInputEvent->iValue)
	{
		uloop_timeout_set(&uloop_long_reset_timer, LONG_RESET_PERIOD);
	}
	else if(ACTION_UP == ptInputEvent->iValue)
	{
		if(uloop_timeout_remaining(&uloop_long_reset_timer) > 0) // 短按
		{
			uloop_timeout_cancel(&uloop_long_reset_timer);
			
			// 进入 网络设置
			EnterSet();
			ToastCancel();
			ClearAllText(); 
			mpdclient_stop();
			
			ShowLine1(STRING_WAITING_NETWORKING);
			ShowLine2(STRING_START_SMARTLINK);
			
			ShowWifiV(0, false);
			getApp()->wifi_linked_state = WIFI_FAIL;
			getApp()->wifi_conn_ok = false;
			
			kill_wifi_config();
			start_smartlink();
		}
	}
	
	return true;
}

static void EnterSet(void)
{
	if(getApp()->curView != VIEW_SET)
	{
		T_ViewAction *view = View(getApp()->curView);
		if(view && view->Exit)
			view->Exit();
		
		mpdclient_set_event_handler(NULL);
		getApp()->curView = VIEW_SET;		
	}
}

static void ExitSet(void)
{
	uloop_timeout_cancel(&uloop_play_timer);
}

int SetViewInit(void)
{
	return RegisterViewAction(&g_tSetViewAction);
}

