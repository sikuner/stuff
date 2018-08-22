
#include "log.h"
#include "conf.h"
#include "common.h"
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

static bool dispatchKeyEventLight(PT_InputEvent ptInputEvent);
static bool onKeyEventLight(PT_InputEvent ptInputEvent);
static void EnterLight(void);
static void ExitLight(void);

static T_ViewAction g_tLightViewAction = {
	.id 				= VIEW_LIGHT,
	.dispatchKeyEvent 	= dispatchKeyEventLight,
	.onKeyEvent			= onKeyEventLight,
	.Enter				= EnterLight,
	.Exit				= ExitLight
};

// 工厂调试模式: 实现, toast显示期间屏蔽所有按键. 密码：上、下、右、左
static bool accord_start_debug(void)
{
	T_InputEvent* inputs = get_latest_button_up();
	
	if(inputs[0].iCode == BTN_LIGHT		// 左
	&& inputs[1].iCode == BTN_LOCK		// 右
	&& inputs[2].iCode == BTN_RECORD	// 下
	&& inputs[3].iCode == BTN_BLUETOOTH	// 上
	&& tv_diff(&(inputs[0].tTime), &(inputs[1].tTime)) < KEY_COMBINE_DELTA	
	&& tv_diff(&(inputs[1].tTime), &(inputs[2].tTime)) < KEY_COMBINE_DELTA
	&& tv_diff(&(inputs[2].tTime), &(inputs[3].tTime)) < KEY_COMBINE_DELTA)
	{
		return true;
	}
	
	return false;
}

static void uloop_info_cb(struct uloop_timeout *timeout)
{
	char line1[MAX_LINE] = { 0 };
	char line2[MAX_LINE] = { 0 };
	char line3[MAX_LINE] = { 0 };
	sprintf(line1, "%-6s %12s", get_beeba_version(), get_sn());
	sprintf(line2, "%-3s %15s", get_model_pid(), get_valid_ip());
	sprintf(line3, "%s",    get_wlan_essid());
	
	Toast123_fs(line1, line2, line3, DEFAULT_FONT_SIZE-1,  8*TOAST_SHORT_PERIOD);
}
static struct uloop_timeout uloop_info_timer = {
	.cb = uloop_info_cb
};

static void debug_tips_cb(struct uloop_timeout *timeout)
{
	if(getApp()->locked != STATE_LOCKED)
		setInterceptExceptMode(0xFFFFFFFF);
	
	ToastCancel();
}
static struct uloop_timeout debug_tips_timer = {
	.cb = debug_tips_cb
};

static bool dispatchKeyEventLight(PT_InputEvent ptInputEvent)
{
	if(BTN_LIGHT != ptInputEvent->iCode)
		return false;
	
	return onKeyEventLight(ptInputEvent);
}

static bool onKeyEventLight(PT_InputEvent ptInputEvent)
{
	if(ACTION_DOWN == ptInputEvent->iValue)
	{
		uloop_timeout_set(&uloop_info_timer, LONG_INFO_PERIOD);
	}
	else if(ACTION_UP == ptInputEvent->iValue)
	{
		if(uloop_timeout_remaining(&uloop_info_timer) > 0) // 存在, 就是短按
		{
			uloop_timeout_cancel(&uloop_info_timer);
			
			if( accord_start_debug() )
			{
				Toast(NULL, STRING_FACTORY_DEBUG, DEFAULT_FONT_SIZE, 3*TOAST_LONG_PERIOD);
				start_debug(); // 开启工厂调试
				
				// 屏蔽按键
				setInterceptExceptMode(MOD_LIGHT|MOD_LOCK|MOD_CHARGING|MOD_CHARGE_FULL|
					MOD_UNCHARGE|MOD_LINEIN_IN|MOD_LINEIN_OUT|MOD_BLUETOOTH_CONN|
					MOD_BLUETOOTH_UNCN|MOD_AIRPLAY_IN|MOD_AIRPLAY_OUT); 
				uloop_timeout_set(&debug_tips_timer, 3*TOAST_SHORT_PERIOD);
			}
			else // 不是组合操作,就执行点灯操作
			{
				led_toggle();
			}
		}
	}
	
	return true;
}

static void EnterLight(void)
{
	// 灯光按钮,不需要模式切换. 在任何模式下都可以点灯
}

static void ExitLight(void)
{
	
}

int LightViewInit(void)
{
	return RegisterViewAction(&g_tLightViewAction);
}

