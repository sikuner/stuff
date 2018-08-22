
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

static bool dispatchKeyEventAirplay(PT_InputEvent ptInputEvent);
static bool onKeyEventAirplay(PT_InputEvent ptInputEvent);
static void EnterAirplay(void);
static void ExitAirplay(void);

static T_ViewAction g_tAirplayViewAction = {
	.id 				= VIEW_AIRPLAY,
	.dispatchKeyEvent 	= dispatchKeyEventAirplay,
	.onKeyEvent			= onKeyEventAirplay,
	.Enter				= EnterAirplay,
	.Exit				= ExitAirplay
};

static void airplay_guard_cb(struct uloop_timeout *timeout)
{
	getApp()->airplay = get_airplay_state();
	if(getApp()->airplay <= 0)
	{
		uloop_timeout_set(&uloop_play_timer, TIMER_ALIVE_UNIT);
	}
	else
	{
		uloop_timeout_set(timeout, AIRPLAY_GUARD_PERIOD);
	}
}
static struct uloop_timeout airplay_guard_timer = { // 定时器: 检测linein状态
	.cb = airplay_guard_cb
};

static bool dispatchKeyEventAirplay(PT_InputEvent ptInputEvent)
{
	if(SND_AIRPLAY_IN != ptInputEvent->iCode 
	&& SND_AIRPLAY_OUT != ptInputEvent->iCode)
		return false;
	
	if(MODE_WIFI != getApp()->curMode)
	{
		if(ACTION_DOWN == ptInputEvent->iValue)
		{
			Toast(NULL, get_mode_strerror(), DEFAULT_FONT_SIZE, TOAST_SHORT_PERIOD);
		}
		
		return true;
	}
	
	return onKeyEventAirplay(ptInputEvent);
}

static bool onKeyEventAirplay(PT_InputEvent ptInputEvent)
{
	if(ptInputEvent->iValue != ACTION_UP) // 只有当按键松开时,才开始处理
		return true;
	
	if (SND_AIRPLAY_IN == ptInputEvent->iCode)
	{
		EnterAirplay();
		ClearAllText();
		ShowLine2(STRING_AIRPLAY);
		
		uloop_timeout_set(&airplay_guard_timer, AIRPLAY_GUARD_PERIOD);
	}
	else if (SND_AIRPLAY_OUT == ptInputEvent->iCode)
	{
		uloop_timeout_cancel(&airplay_guard_timer);
		uloop_timeout_set(&uloop_play_timer, TIMER_ALIVE_UNIT);
	}
	
	return true;
}

static void EnterAirplay(void)
{
	if(getApp()->curView != VIEW_AIRPLAY)
	{
		T_ViewAction *view = View(getApp()->curView);
		if(view && view->Exit)
			view->Exit();
		
		mpdclient_set_event_handler(NULL);
		getApp()->curView = VIEW_AIRPLAY;		
	}
	
	fprintf(stderr, " <End> EnterAirplay curView: %d \n", getApp()->curView);
}

static void ExitAirplay(void)
{
	uloop_timeout_cancel(&airplay_guard_timer);
}

int AirplayViewInit(void)
{
	return RegisterViewAction(&g_tAirplayViewAction);
}

