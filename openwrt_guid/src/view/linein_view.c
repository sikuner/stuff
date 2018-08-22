
#include "log.h"
#include "conf.h"
#include "application.h"
#include "view_manager.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <libubox/uloop.h>
#include <mpd/client.h>

static bool dispatchKeyEventLinein(PT_InputEvent ptInputEvent);
static bool onKeyEventLinein(PT_InputEvent ptInputEvent);
static void EnterLinein(void);
static void ExitLinein(void);

static T_ViewAction g_tLineinViewAction = {
	.id 				= VIEW_LINEIN,
	.dispatchKeyEvent 	= dispatchKeyEventLinein,
	.onKeyEvent			= onKeyEventLinein,
	.Enter				= EnterLinein,
	.Exit				= ExitLinein
};

static void linein_guard_cb(struct uloop_timeout *timeout)
{
	int linein = get_linein_state();
	if(SND_LINEIN_OUT == linein)
	{
		getApp()->linein = SND_LINEIN_OUT;
		pam_close();
		
		network_start();
		wifi_daemon(true);
		uloop_timeout_set(&uloop_play_timer, TIMER_ALIVE_UNIT);
		
		UpdateMode();
		UpdateModeIcon();
		
		return;
	}
	
	uloop_timeout_set(timeout, LINEIN_GUARD_PERIOD);
}
static struct uloop_timeout linein_guard_timer = { // 定时器: 检测linein状态
	.cb = linein_guard_cb
};

static bool dispatchKeyEventLinein(PT_InputEvent ptInputEvent)
{
	if(SND_LINEIN_IN != ptInputEvent->iCode 
	&& SND_LINEIN_OUT != ptInputEvent->iCode)
		return false;
	
	return onKeyEventLinein(ptInputEvent);
}

static bool onKeyEventLinein(PT_InputEvent ptInputEvent)
{
	if(ptInputEvent->iValue != ACTION_UP) // 只有当按键松开时,才开始处理
		return true;
	
	if (SND_LINEIN_IN == ptInputEvent->iCode)
	{
		getApp()->linein = get_linein_state();
		if(SND_LINEIN_IN != getApp()->linein)
		{
			return true;
		}
		
		if(MODE_WIFI == getApp()->curMode)
		{
			network_stop();			
			wifi_daemon(false);
			getApp()->wifi_linked_state = WIFI_FAIL;
			getApp()->wifi_conn_ok = false;
		}
		
		EnterLinein();
		ClearAllText();
		mpdclient_stop();
		ShowLine2(STRING_LINEIN);
		
		pam_open();
		
		UpdateMode();
		UpdateModeIcon();
		uloop_timeout_set(&linein_guard_timer, LINEIN_GUARD_PERIOD);
	}
	else if (SND_LINEIN_OUT == ptInputEvent->iCode)
	{
		getApp()->linein = get_linein_state();
		if(SND_LINEIN_OUT != getApp()->linein)
		{
			return true;
		}
		
		uloop_timeout_cancel(&linein_guard_timer);
		
		pam_close();
		
//		DBG_PRINTF("getApp()->linein: %d, getApp()->wifi_linked_state: %d \n", 
//			getApp()->linein,
//			getApp()->wifi_linked_state);
		
		UpdateMode();
		UpdateModeIcon();
		
		network_start();
		wifi_daemon(true);
		uloop_timeout_set(&uloop_play_timer, TIMER_ALIVE_UNIT);
	}
	
	return true;
}

static void EnterLinein(void)
{
	if(getApp()->curView != VIEW_LINEIN)
	{
		T_ViewAction *view = View(getApp()->curView);
		if(view && view->Exit)
			view->Exit();
		
		// 断掉wifi, 断掉bluetooth
		mpdclient_set_event_handler(NULL);
		getApp()->curView = VIEW_LINEIN;		
	}
	
	fprintf(stderr, " <End> EnterLinein curView: %d \n", getApp()->curView);
}

static void ExitLinein(void)
{
	uloop_timeout_cancel(&linein_guard_timer);
	ToastCancel();
	ClearAllText();
}

int LineinViewInit(void)
{
	return RegisterViewAction(&g_tLineinViewAction);
}

int ShowLineinMode(void)
{
	getApp()->linein = get_linein_state();
	if(SND_LINEIN_IN != getApp()->linein)
	{
		return -1;
	}
	
	network_stop();	
	wifi_daemon(false);
	getApp()->wifi_linked_state = WIFI_FAIL;
	getApp()->wifi_conn_ok = false;
	
	EnterLinein();
	UpdateMode();
	UpdateModeIcon();
	ClearAllText();
	mpdclient_stop();
	ShowLine2(STRING_LINEIN);
	
	pam_open();
	
	uloop_timeout_set(&linein_guard_timer, LINEIN_GUARD_PERIOD);
	
	return 0;
}

