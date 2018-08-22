
#include "conf.h"
#include "types.h"
#include "view_manager.h"
#include "application.h"
#include "utf8_strings.h"

#include <stdio.h>
#include <libubox/uloop.h>

static bool dispatchKeyEventVolume(PT_InputEvent ptInputEvent);
static bool onKeyEventVolume(PT_InputEvent ptInputEvent);
static void EnterVolume(void);
static void ExitVolume(void);

static T_ViewAction g_tVolumeViewAction = {
	.id 				= VIEW_VOLUME,
	.dispatchKeyEvent 	= dispatchKeyEventVolume,
	.onKeyEvent 		= onKeyEventVolume,
	.Enter				= EnterVolume,
	.Exit				= ExitVolume,
};

static bool dispatchKeyEventVolume(PT_InputEvent ptInputEvent)
{
	if(BTN_VOLUMEUP!=ptInputEvent->iCode && BTN_VOLUMEDOWN!=ptInputEvent->iCode)
		return false;
	
	return onKeyEventVolume(ptInputEvent);
}

static void get_volume_cb(struct uloop_timeout *timeout)
{
	getApp()->volume = mpdclient_get_volume();
}
static struct uloop_timeout get_volume_timer = {
	.cb = get_volume_cb
};

static void set_volume_cb(struct uloop_timeout *timeout)
{
	mpdclient_set_volume(getApp()->volume);
	ToastBmp(NULL, get_volume_icon(getApp()->volume), TOAST_SHORT_PERIOD);
}
static struct uloop_timeout set_volume_timer = {
	.cb = set_volume_cb
};

static bool onKeyEventVolume(PT_InputEvent ptInputEvent)
{
	if(ACTION_DOWN == ptInputEvent->iValue)
	{
		if(uloop_timeout_remaining(&get_volume_timer) <= 0) // 不存在
		{
			get_volume_cb(NULL); // 立即获取音量
		}
		uloop_timeout_set(&get_volume_timer, TOAST_LONG_PERIOD);
		
		if(BTN_VOLUMEUP == ptInputEvent->iCode)
			getApp()->volume = min(getApp()->volume+1, 100);
		else
			getApp()->volume = max(getApp()->volume-1, 0);
	}
	else 
	{
		if(uloop_timeout_remaining(&set_volume_timer) <= 0) // 不存在
		{
			set_volume_cb(NULL); // 立即显示音量,并设置
		}
		uloop_timeout_set(&set_volume_timer, TIMER_ALIVE_UNIT);
		
		ToastBmp(NULL, get_volume_icon(getApp()->volume), TOAST_SHORT_PERIOD);
	}
	
	return true;
}

static void EnterVolume(void)
{
	// 音量按钮,不需要模式切换. 在任何模式下都可以调节音量
}

static void ExitVolume(void)
{
	
}

int VolumeViewInit(void)
{
	return RegisterViewAction(&g_tVolumeViewAction);
}

