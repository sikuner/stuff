
#include "conf.h"
#include "log.h"
#include "view_manager.h"
#include "utf8_strings.h"
#include "application.h"
#include "widget.h"

#include <errno.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <libubox/uloop.h>

static bool dispatchKeyEventLock(PT_InputEvent ptInputEvent);
static bool onKeyEventLock(PT_InputEvent ptInputEvent);
static void EnterLock(void);
static void ExitLock(void);

static T_ViewAction g_tLockViewAction = {
	.id 				= VIEW_LOCK,
	.dispatchKeyEvent 	= dispatchKeyEventLock,
	.onKeyEvent			= onKeyEventLock,
	.Enter				= EnterLock,
	.Exit				= ExitLock
};

static void uloop_long_lock_cb(struct uloop_timeout *timeout)
{
	if(getApp()->locked == STATE_LOCKED)
	{
		getApp()->locked = STATE_UNLOCKED;
		ShowLock(-1);
		setInterceptExceptMode(0xFFFFFFFF);
	}
	else
	{
		getApp()->locked = STATE_LOCKED;
		ShowLock(1);
		setInterceptExceptMode(MOD_LIGHT|MOD_LOCK|MOD_CHARGING|MOD_CHARGE_FULL|
			MOD_UNCHARGE|MOD_LINEIN_IN|MOD_LINEIN_OUT|MOD_BLUETOOTH_CONN|
			MOD_BLUETOOTH_UNCN|MOD_AIRPLAY_IN|MOD_AIRPLAY_OUT); 
	}
}

static struct uloop_timeout uloop_long_lock_timer = {
	.cb = uloop_long_lock_cb
};

static bool dispatchKeyEventLock(PT_InputEvent ptInputEvent)
{
	if(BTN_LOCK != ptInputEvent->iCode)
		return false;
	
	return onKeyEventLock(ptInputEvent);
}

static bool onKeyEventLock(PT_InputEvent ptInputEvent)
{
	if(ACTION_DOWN == ptInputEvent->iValue)
	{
		uloop_timeout_set(&uloop_long_lock_timer, LONG_LOCK_PERIOD);
	}
	else if(ACTION_UP == ptInputEvent->iValue)
	{
		if(uloop_timeout_remaining(&uloop_long_lock_timer) > 0) // 存在, 就是短按
		{
			uloop_timeout_cancel(&uloop_long_lock_timer);
			
			if(getApp()->locked == STATE_LOCKED)
			{
				ToastBmp(NULL, ICON_LOCKED_TIPS, TOAST_SHORT_PERIOD);
			}
			else
			{
				ToastBmp(NULL, ICON_LOCK_TIPS, TOAST_SHORT_PERIOD);
			}
		}
	}
	
	return true;
}

static void EnterLock(void)
{
	
}

static void ExitLock(void)
{
	
}

int LockViewInit(void)
{
	return RegisterViewAction(&g_tLockViewAction);
}

