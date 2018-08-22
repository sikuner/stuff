
#include "conf.h"
#include "log.h"
#include "utf8_strings.h"
#include "application.h"
#include "network.h"
#include "server.h"
#include "view_manager.h"

#include <errno.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <libubox/uloop.h>
#include <mpd/client.h>

static bool dispatchKeyEventDownload(PT_InputEvent ptInputEvent);
static bool onKeyEventDownload(PT_InputEvent ptInputEvent);
static void EnterDownload(void);
static void ExitDownload(void);

static T_ViewAction g_tDownloadViewAction = {
	.id 				= VIEW_DOWNLOAD,
	.dispatchKeyEvent 	= dispatchKeyEventDownload,
	.onKeyEvent			= onKeyEventDownload,
	.Enter				= EnterDownload,
	.Exit				= ExitDownload
};

static void down_stay_cb(struct uloop_timeout *timeout)
{
	getApp()->download = DOWN_NO_DOWNLOAD;
	ShowDown(getApp()->download);
}
static struct uloop_timeout down_stay_timer = {
	.cb = down_stay_cb
};
void ShowDownStay(void)			// 下载图标延时显示3s
{
	if(getApp()->download != DOWN_DOWNLOADING)
	{
		getApp()->download = DOWN_DOWNLOADING;
		ShowDown(getApp()->download);
	}
	
	uloop_timeout_set(&down_stay_timer, DOWN_STAY_PERIOD);
}

static void down_exist_async_result(int rc, char *url)
{	
	switch(rc)
	{
	case 2: // 该url在 已成功缓存列表 中
		{
			Toast(NULL, STRING_ALREADY_COMPLETE, DEFAULT_FONT_SIZE, TOAST_SHORT_PERIOD);
		}
		break;
	case 1: // 该url在 缓存任务列表 中
		{
			Toast(NULL, STRING_ALREADY_TASK, DEFAULT_FONT_SIZE, TOAST_SHORT_PERIOD);
		}
		break;
	case 0: // 两者均不在 已成功缓存列表 和 缓存任务列表
		{
			Toast(NULL, STRING_READY_DOWNLOAD, DEFAULT_FONT_SIZE, TOAST_SHORT_PERIOD);
			
			if(NULL!=url && 0!=strlen(url))
			{
				down_create_async(url);
			}
		}
		break;
	default: // <0, 出错情况
		{
			Toast(NULL, STRING_DOWNLOAD_ERROR, DEFAULT_FONT_SIZE, TOAST_SHORT_PERIOD);
		}
		break;
	}
}

static void uloop_long_download_cb(struct uloop_timeout *timeout)
{
	if(VIEW_SET == getApp()->curView)
	{
		ClearAllText();
		
		start_ap();
		
		ShowLine1(STRING_WAITING_NETWORKING);
		ShowLine2(STRING_WAITING);
	}
	else
	{
		const char *song_uri = mpdclient_get_current_song_uri();
		
		if( !network_get_connected() ) // 网络断开
		{
			Toast(NULL, STRING_DONT_DOWNLOAD, DEFAULT_FONT_SIZE, TOAST_SHORT_PERIOD);
		}
		else if((getApp()->airplay=get_airplay_state()) > 0) // AirPlay播放模式,不支持下载
		{
			Toast(NULL, STRING_NOTSUPPORTED, DEFAULT_FONT_SIZE, TOAST_SHORT_PERIOD);
		}
		else if(NULL == song_uri)
		{
			Toast(NULL, STRING_NOTSUPPORTED, DEFAULT_FONT_SIZE, TOAST_SHORT_PERIOD);
		}
		else if(NULL != strstr(song_uri, "qingting.fm")) // 过滤蜻蜓FM, 不支持下载
		{
			Toast(NULL, STRING_NOTSUPPORTED, DEFAULT_FONT_SIZE, TOAST_SHORT_PERIOD);
		}
		else if(NULL == strstr(song_uri, "://")) // 本地资源,无需下载
		{
			Toast(NULL, STRING_ALREADY_COMPLETE, DEFAULT_FONT_SIZE, TOAST_SHORT_PERIOD);
		}
		else // 支持下载的网络资源
		{
			down_exist_async(song_uri, down_exist_async_result);
		}
	}
}
static struct uloop_timeout uloop_long_download_timer = {
	.cb = uloop_long_download_cb
};

static bool dispatchKeyEventDownload(PT_InputEvent ptInputEvent)
{
	if(BTN_DOWNLOAD != ptInputEvent->iCode)
		return false;
	
	if(MODE_WIFI != getApp()->curMode)
	{
		if(ACTION_DOWN == ptInputEvent->iValue)
		{			
			Toast(NULL, get_mode_strerror(), DEFAULT_FONT_SIZE, TOAST_SHORT_PERIOD);
		}
		
		return true;
	}
	
	return onKeyEventDownload(ptInputEvent);
}

static bool onKeyEventDownload(PT_InputEvent ptInputEvent)
{
	if(ACTION_DOWN == ptInputEvent->iValue)
	{
		uloop_timeout_set(&uloop_long_download_timer, LONG_DOWNLOAD_PERIOD);
	}
	else if(ACTION_UP == ptInputEvent->iValue)
	{
		if(uloop_timeout_remaining(&uloop_long_download_timer) > 0) // 存在, 就是短按
		{
			uloop_timeout_cancel(&uloop_long_download_timer);
			
			ToastBmp(NULL, ICON_DOWN_TIPS, TOAST_SHORT_PERIOD);
		}
	}
	
	return true;
}

static void EnterDownload(void)
{
	if(getApp()->curView != VIEW_DOWNLOAD)
	{
		T_ViewAction *view = View(getApp()->curView);
		if(view && view->Exit)
			view->Exit();
		
		getApp()->curView = VIEW_DOWNLOAD;		
	}
	
	fprintf(stderr, " <End> EnterDownload curView: %d \n", getApp()->curView);
}

static void ExitDownload(void)
{
	
}

int DownloadViewInit(void)
{
	return RegisterViewAction(&g_tDownloadViewAction);
}

