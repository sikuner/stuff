
#include <errno.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <libubox/uloop.h>

#include "conf.h"
#include "log.h"
#include "common.h"
#include "utf8_strings.h"
#include "application.h"
#include "view_manager.h"
#include "widget.h"
#include "network.h"
#include "mpdclient.h"
#include "view_manager.h"

static bool dispatchKeyEventBluetooth(PT_InputEvent ptInputEvent);
static bool onKeyEventBluetooth(PT_InputEvent ptInputEvent);
static void EnterBluetooth(void);
static void ExitBluetooth(void);

static T_ViewAction g_tBluetoothViewAction = {
	.id 				= VIEW_BLUETOOTH,
	.dispatchKeyEvent 	= dispatchKeyEventBluetooth,
	.onKeyEvent			= onKeyEventBluetooth,
	.Enter				= EnterBluetooth,
	.Exit				= ExitBluetooth
};

static struct uloop_process bt_start_proc;
static void bt_start_cb(struct uloop_process *p, int ret)
{
	bool started = false;
	
	int count = 25;
	while (count-- > 0)
	{
		getApp()->bluetooth = get_bt_state();
		if(BT_STATE_STARTED == getApp()->bluetooth
		|| BT_STATE_CONNECTED == getApp()->bluetooth)
		{
			started = true;
			break;
		}
		
		usleep(200*1000);
	}
	
	DBG_PRINTF("started: %d \n", started);
	
	if(started)
	{
		if(MODE_WIFI == getApp()->curMode)
		{
			network_stop();
			wifi_daemon(false);
			getApp()->wifi_linked_state = WIFI_FAIL;
			getApp()->wifi_conn_ok = false;
		}
		
		UpdateMode();
		UpdateModeIcon();
		
		ShowLine1(STRING_BLUETOOTH_UNCN);
		ShowLine2(get_bt_name());
		ShowLine3(STRING_BLANK);
	}
	else
	{
		Toast(NULL, STRING_BT_RETRY, DEFAULT_FONT_SIZE, TOAST_LONG_PERIOD);
		
		bt_stop_nocb();
		getApp()->bluetooth = BT_STATE_STOPPED;
		
		UpdateMode();		
		UpdateModeIcon();
		
		if(MODE_WIFI == getApp()->curMode)
		{
			uloop_timeout_set(&uloop_play_timer, 10*TIMER_ALIVE_UNIT);			
			network_start();
			wifi_daemon(true);
		}
	}
}

static bool bt_start(void)
{
	pid_t pid = vfork();
	if (pid < 0)
	{
		fprintf(stderr, "fork error!");
		return false;
	}
	if (pid > 0) // > 0, 处于父进程中. add后返回
	{
		bt_start_proc.pid = pid;
		bt_start_proc.cb = bt_start_cb;
		uloop_process_add(&bt_start_proc);
		return true;
	}
	
	execlp("bt.sh", "bt.sh", "start", NULL);
	exit(1);
}

static struct uloop_process bt_stop_proc;
static void bt_stop_cb(struct uloop_process *p, int ret)
{
	bool stopped = false;
	
	int count = 20;
	while (count-- > 0)
	{
		getApp()->bluetooth = get_bt_state();
		if(BT_STATE_STOPPED == getApp()->bluetooth)
		{
			stopped = true;
			break;
		}
		
		usleep(200*1000);
	}
	
	DBG_PRINTF("stopped: %d \n", stopped);
	
	(void)stopped;
//	if(stopped)
	{
		playsound(SOUND_BLUETOOTH_CLOSE);
		getApp()->bluetooth = BT_STATE_STOPPED;
		
		UpdateMode();
		UpdateModeIcon();
		
		if(MODE_WIFI == getApp()->curMode)
		{
			uloop_timeout_set(&uloop_play_timer, 10*TIMER_ALIVE_UNIT);
			network_start();
			wifi_daemon(true);
		}
	}
}

static bool bt_stop(void)
{
	pid_t pid = vfork();
	if (pid < 0)
	{
		fprintf(stderr, "fork error!");
		return false;
	}
	if (pid > 0) // > 0, 处于父进程中. add后返回
	{
		bt_stop_proc.pid = pid;
		bt_stop_proc.cb = bt_stop_cb;
		uloop_process_add(&bt_stop_proc);
		return true;
	}
	
	execlp("bt.sh", "bt.sh", "stop", NULL);
	exit(1);
}

static void uloop_bluetooth_cb(struct uloop_timeout *timeout)
{
	ShowLine1(STRING_BLANK);
	ShowLine3(STRING_BLANK);
	
	getApp()->bluetooth = get_bt_state();
	if(BT_STATE_STARTED == getApp()->bluetooth
	|| BT_STATE_CONNECTED == getApp()->bluetooth) // 是 开启状态(开启或已连接)
	{
		ShowLine2(STRING_BLUETOOTH_STOP);
		bt_stop();
	}
	else
	{
		playsound(SOUND_BLUETOOTH_OPEN);
		ShowLine2(STRING_BLUETOOTH_START);
		EnterBluetooth();
		mpdclient_stop();
		bt_start();
	}
}
static struct uloop_timeout uloop_bluetooth_timer = {
	.cb = uloop_bluetooth_cb
};

static bool dispatchKeyEventBluetooth(PT_InputEvent ptInputEvent)
{
  	if(BTN_BLUETOOTH != ptInputEvent->iCode
	&& SND_BLUETOOTH_CONN != ptInputEvent->iCode
	&& SND_BLUETOOTH_UNCN != ptInputEvent->iCode)
	{
		return false;
	}
	
	if(!getApp()->has_bt) // 没有蓝牙设备时
	{
		Toast(NULL, STRING_NONSUPPORT_BLUETOOTH, DEFAULT_FONT_SIZE, TOAST_SHORT_PERIOD);
		return true;
	}
	
	if(MODE_LINEIN == getApp()->curMode)
	{
		if(ACTION_DOWN == ptInputEvent->iValue)
		{	
			Toast(NULL, get_mode_strerror(), DEFAULT_FONT_SIZE, TOAST_SHORT_PERIOD);
		}
		
		return true;
	}
	
	return onKeyEventBluetooth(ptInputEvent);
}

static bool onKeyEventBluetooth(PT_InputEvent ptInputEvent)
{
	if(BTN_BLUETOOTH == ptInputEvent->iCode)
	{
		if(ACTION_DOWN == ptInputEvent->iValue)
		{
			uloop_timeout_set(&uloop_bluetooth_timer, LONG_BLUETOOTH_PERIOD);
		}
		else if(ACTION_UP == ptInputEvent->iValue)
		{
			if(uloop_timeout_remaining(&uloop_bluetooth_timer) > 0) // 存在, 就是短按
			{
				uloop_timeout_cancel(&uloop_bluetooth_timer);
				
				if(BT_STATE_STARTED == getApp()->bluetooth
				|| BT_STATE_CONNECTED == getApp()->bluetooth)
				{
					ToastBmp(NULL, ICON_CLOSE_BT_TIPS, TOAST_SHORT_PERIOD);
				}
				else
				{
					ToastBmp(NULL, ICON_START_BT_TIPS, TOAST_SHORT_PERIOD);
				}
			}
		}
	}
	else
	{
		if(ACTION_UP==ptInputEvent->iValue && VIEW_BLUETOOTH==getApp()->curView)
		{
			if(SND_BLUETOOTH_CONN == ptInputEvent->iCode)
			{
				playsound(SOUND_BLUETOOTH_CONNECTED);
				ShowLine1(STRING_BLUETOOTH_CONN);
			}
			else
			{
				playsound(SOUND_BLUETOOTH_DISCONNECTED);
				ShowLine1(STRING_BLUETOOTH_UNCN);
			}
			
			getApp()->bluetooth = get_bt_state();
			UpdateMode();
			UpdateModeIcon();
		}
	}
	
	return true;
}

static void EnterBluetooth(void)
{
	if(getApp()->curView != VIEW_BLUETOOTH)
	{
		T_ViewAction *view = View(getApp()->curView);
		if(view && view->Exit)
			view->Exit();
		
		// 断掉wifi
		mpdclient_set_event_handler(NULL);
		getApp()->curView = VIEW_BLUETOOTH;		
	}
	
	DBG_PRINTF("curView: %d, curMode: %d \n",
		getApp()->curView,
		getApp()->curMode);
}

static void ExitBluetooth(void)
{
	uloop_process_delete(&bt_start_proc);
	uloop_process_delete(&bt_stop_proc);
	
	bt_stop_nocb();
	getApp()->bluetooth = BT_STATE_STOPPED;
	
	UpdateMode();
	UpdateModeIcon();
	ClearAllText();
	
	DBG_PRINTF("curView: %d, curMode: %d \n",
		getApp()->curView,
		getApp()->curMode);
}

int BluetoothViewInit(void)
{
	return RegisterViewAction(&g_tBluetoothViewAction);
}

int ShowBluetoothMode(void)
{
	getApp()->bluetooth = get_bt_state();
	if(BT_STATE_STARTED!=getApp()->bluetooth 	// 蓝牙已启动
	&& BT_STATE_CONNECTED!=getApp()->bluetooth)	// 蓝牙已连接
		return -1;
	
	network_stop();	
	wifi_daemon(false);
	getApp()->wifi_linked_state = WIFI_FAIL;
	getApp()->wifi_conn_ok = false;
	
	EnterBluetooth();
	UpdateMode();
	UpdateModeIcon();
	
	mpdclient_stop();
	
	ShowLine2(get_bt_name());
	ShowLine3(STRING_BLANK);
	
	if(BT_STATE_STARTED!=getApp()->bluetooth)
		ShowLine1(STRING_BLUETOOTH_UNCN);
	else if(BT_STATE_CONNECTED!=getApp()->bluetooth)
		ShowLine1(STRING_BLUETOOTH_CONN);
	else
		ShowLine1(STRING_BLANK);
	
	return 0;
}

