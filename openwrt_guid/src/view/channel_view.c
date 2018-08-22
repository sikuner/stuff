
#include "log.h"
#include "conf.h"
#include "view_manager.h"
#include "utf8_strings.h"
#include "application.h"

#include <stdio.h>
#include <libubox/uloop.h>

static bool dispatchKeyEventChannel(PT_InputEvent ptInputEvent);
static bool onKeyEventChannel(PT_InputEvent ptInputEvent);
static void EnterChannel(void);
static void ExitChannel(void);

static T_ViewAction g_tChannelViewAction = {
	.id 				= VIEW_CHANNEL,
	.dispatchKeyEvent 	= dispatchKeyEventChannel,
	.onKeyEvent			= onKeyEventChannel,	
	.Enter				= EnterChannel,
	.Exit				= ExitChannel,
};

static char *icon_channels[] = { ICON_RHYMES_CH, ICON_ENGLISH_CH, ICON_HABITS_CH, 
	ICON_STORIES_CH, ICON_RECORDS_CH, ICON_DOWNLOADS_CH };
static int   channels_size = sizeof(icon_channels)/sizeof(icon_channels[0]);

static char *get_channel_icon(int channel)
{
	if(channel < 0)
	{
		channel = 0;
	}
	if(channel > channels_size - 1)
	{
		channel = channels_size - 1;
	}
	
	return icon_channels[channel];
}

static bool dispatchKeyEventChannel(PT_InputEvent ptInputEvent)
{
	if(BTN_CHANNELUP!=ptInputEvent->iCode && BTN_CHANNELDOWN!=ptInputEvent->iCode)
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
	
	return onKeyEventChannel(ptInputEvent);
}

#if 1

static int channel_up   = 0;
static int channel_down = 0;

static void channel_cb(struct uloop_timeout *timeout)
{
	getApp()->channel = -1;
	
	channel_up 	 = 0;
	channel_down = 0;
}
static struct uloop_timeout channel_timer = {
	.cb = channel_cb
};

static bool onKeyEventChannel(PT_InputEvent ptInputEvent)
{
	if(ACTION_DOWN == ptInputEvent->iValue)
	{
		if(uloop_timeout_remaining(&channel_timer) <= 0) // 不存在
		{
			channel_cb(NULL); // 立即初始化
		}
		uloop_timeout_set(&channel_timer, 2*TOAST_SHORT_PERIOD);
		
		if(BTN_CHANNELUP == ptInputEvent->iCode)
		{
			channel_up++;
			channel_down = 0;
			
			if(getApp()->channel < 0)
			{
				getApp()->channel = 0;
				channel_up = 5;
			}
			else if(5 == channel_up % 6)
			{
				getApp()->channel = (getApp()->channel + 1) % channels_size;				
			}
		}
		else
		{
			channel_up = 0;
			channel_down++;
			
			if(getApp()->channel < 0)
			{
				getApp()->channel = channels_size - 1;
				channel_down = 5;
			}
			else if(5 == channel_down % 6)
			{
				getApp()->channel = (getApp()->channel - 1 + channels_size) % channels_size;
			}
		}
	}
	else // 按键松开, 开始处理
	{
		ToastBmp(NULL, get_channel_icon(getApp()->channel), TOAST_LONG_PERIOD);
	}
	
	return true;
}

#else

static void clear_channel_cb(struct uloop_timeout *timeout)
{
	getApp()->channel = -1;	
}
static struct uloop_timeout clear_channel_timer = {
	.cb = clear_channel_cb
};

static bool onKeyEventChannel(PT_InputEvent ptInputEvent)
{	
	if(ptInputEvent->iValue != ACTION_UP) // 只有当按键松开时,才开始处理
	{
		return true;
	}
	
	if(BTN_CHANNELUP == ptInputEvent->iCode)
	{
		getApp()->channel = (getApp()->channel + 1) % channels_size;
	}
	else
	{
		getApp()->channel = (getApp()->channel - 1 + channels_size) % channels_size;
	}
	
	ToastBmp(NULL, get_channel_icon(getApp()->channel), TOAST_LONG_PERIOD);
	uloop_timeout_set(&clear_channel_timer, TOAST_LONG_PERIOD);
	
	return true;
}
#endif

static void EnterChannel(void)
{
	
}

static void ExitChannel(void)
{
	
}

int ChannelViewInit(void)
{
	return RegisterViewAction(&g_tChannelViewAction);
}

