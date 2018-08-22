
#include "conf.h"
#include "log.h"
#include "utf8_strings.h"
#include "mpdclient.h"
#include "application.h"
#include "view_manager.h"

#include <stdio.h>
#include <string.h>
#include <libubox/uloop.h>

static T_ViewAction *g_ptViewActionHead = NULL;

static T_ViewAction *g_ptTargetView = NULL;
static volatile int g_interceptExceptMode = 0xFFFFFFFF; // 不拦截按键的掩码集合

void setInterceptExceptMode(int except_mode)
{
	g_interceptExceptMode = except_mode;
}

static bool onInterceptExceptMode(PT_InputEvent ptInputEvent)
{
	return g_interceptExceptMode & getButtonModeParse(ptInputEvent);
}

static bool onKeyEventGroup(PT_InputEvent ptInputEvent)
{
	return true;
}

bool dispatchKeyEventGroup(PT_InputEvent ptInputEvent)
{
	if(ptInputEvent->iValue == ACTION_DOWN) // 按下
	{
		idle_poweroff();		// 重置 空闲自动关机 定时器
		
		g_ptTargetView = NULL; 	// 每次Down事件, 都置为NULL
		if(onInterceptExceptMode(ptInputEvent))
		{
			T_ViewAction *ptTmp = g_ptViewActionHead;
			while (ptTmp)
			{
				if(ptTmp->dispatchKeyEvent(ptInputEvent))
				{
					printf("capturedView %d\n", ptTmp->id);
					g_ptTargetView = ptTmp;
					return true;
				}
				
				ptTmp = ptTmp->ptNext;
			}
		}
	}
	
	if(!onInterceptExceptMode(ptInputEvent))
	{
		fprintf(stderr, "XXXXXXXXXXXXXXX onInterceptExceptMode iCode:%d \n", ptInputEvent->iCode);
		if(getApp()->locked == STATE_LOCKED && ACTION_UP == ptInputEvent->iValue)
		{
			ToastBmp(NULL, ICON_LOCKED_TIPS, TOAST_SHORT_PERIOD);
		}
	}
	
	// 子View没有捕获Down事件, ViewGroup自身处理. 这里处理包括Down,Up,Hold
	if(NULL == g_ptTargetView)
	{
		return onKeyEventGroup(ptInputEvent);
	}
	
	// 这一步在Down中是不会执行到的, 只有Hold和Up才会执行到
	return g_ptTargetView->dispatchKeyEvent(ptInputEvent);
}

int RegisterViewAction(PT_ViewAction ptViewAction)
{
	PT_ViewAction ptTmp;

	if (!g_ptViewActionHead)
	{
		g_ptViewActionHead   = ptViewAction;
		ptViewAction->ptNext = NULL;
	}
	else
	{
		ptTmp = g_ptViewActionHead;
		while (ptTmp->ptNext)
		{
			ptTmp = ptTmp->ptNext;
		}
		ptTmp->ptNext = ptViewAction;
		ptViewAction->ptNext = NULL;
	}
	
	return 0;
}

T_ViewAction* View(int view_id)
{
	T_ViewAction *ptTmp = g_ptViewActionHead;
	
	while (ptTmp)
	{
		if (view_id == ptTmp->id)
		{
			return ptTmp;
		}
		ptTmp = ptTmp->ptNext;
	}
	
	return NULL;
}

static void idle_play_cb(struct uloop_timeout *timeout)
{
	const char *song_uri = NULL;
	char artist[MAX_LINE] = STRING_ARTIST_UNKNOWN;
	char title[MAX_LINE]  = STRING_TITLE_UNKNOWN;
	char error[MAX_LINE]  = "";
	
	song_uri = mpdclient_get_current_song_uri();
	if(!song_uri)
	{
		strcpy(artist, STRING_BLANK);
		strcpy(title, STRING_NO_SONG);
	}
	else
	{
		mpdclient_get_artist_title_error(artist, title, error);
	}
	
	ShowLine1(artist);
	ShowLine2(title);
	ShowLine3(STRING_BLANK);
}
static struct uloop_timeout idle_play_timer = {
	.cb = idle_play_cb
};

static void idle_update_cb(struct uloop_timeout *timeout)
{
	const char *song_uri = NULL;
	char artist[MAX_LINE] = STRING_ARTIST_UNKNOWN;
	char title[MAX_LINE]  = STRING_TITLE_UNKNOWN;
	char error[MAX_LINE]  = "";
	
	song_uri = mpdclient_get_current_song_uri();
	if(!song_uri)
	{
		strcpy(artist, STRING_BLANK);
		strcpy(title, STRING_NO_SONG);
	}
	else
	{
		mpdclient_get_artist_title_error(artist, title, error);
	}
	
	ShowLine1(artist);
	ShowLine2(title);
	ShowLine3(STRING_BLANK);
}
static struct uloop_timeout idle_update_timer = {
	.cb = idle_update_cb
};

void mpd_idleloop_proc(unsigned int idle)
{
	fprintf(stderr, "====== mpd_idleloop_proc idle=0x%08X \n", idle);
	
	if(MPD_IDLE_UPDATE & idle)
	{
		uloop_timeout_set(&idle_update_timer, 5*TIMER_ALIVE_UNIT);
	}
	if(MPD_IDLE_PLAYER & idle)
	{
		uloop_timeout_cancel(&idle_update_timer);
		uloop_timeout_set(&idle_play_timer, 2*TIMER_ALIVE_UNIT);
	}
}

static void mpd_state_cb(struct uloop_timeout *timeout)
{
	idle_poweroff();		// 重置 空闲自动关机 定时器
	ShowPlayerState(getApp()->mpd_state);
	
	if(getApp()->mpd_error && mpdclient_get_idle_event_handler())
	{
		int ret = -1;
		char artist[MAX_LINE] = { 0 };
		char title[MAX_LINE]  = { 0 };
		char line1[MAX_LINE]  = { 0 };
		char line2[MAX_LINE]  = { 0 };
		char line3[MAX_LINE]  = { 0 };
		
		ret = parse_mpd_error(getApp()->mpd_error, artist, title);
		if(ret < 0)
		{
			strcpy(line2, STRING_PLAY_ERROR);
		}
		else
		{
			strcpy(line1, artist);
			strcpy(line2, title);
			strcpy(line3, STRING_PLAY_ERROR);
		}
		
		Toast123(line1, line2, line3, TOAST_LONG_PERIOD);
		mpdclient_clearerror();
	}
	
	if(VIEW_AB == getApp()->curView
	&& MPD_STATE_PLAY == getApp()->mpd_state 
	&& uloop_timeout_remaining(&uloop_play_timer) <= 0)
	{
		uloop_timeout_set(&uloop_play_timer, TIMER_ALIVE_UNIT);
	}
}
static struct uloop_timeout mpd_state_timer = {
	.cb = mpd_state_cb
};
void mpd_state_proc(unsigned int idle)
{
	if(MPD_IDLE_PLAYER & idle)
	{
		int ret =  mpdclient_get_state_error_idle(&(getApp()->mpd_state), &(getApp()->mpd_error));
		DBG_PRINTF("ret: %d, mpd_state: %d, mpd_error: %s \n",
			ret,
			getApp()->mpd_state,
			getApp()->mpd_error);
		
		uloop_timeout_set(&mpd_state_timer, 2*TIMER_ALIVE_UNIT);
	}
}

static void uloop_play_cb(struct uloop_timeout *timeout)
{
	ShowPlayView();
}
struct uloop_timeout uloop_play_timer = { // 定时器: 回退到播放模式 
	.cb = uloop_play_cb
};

void startup(void)
{
	UpdateModeIcon();
	ShowTime(1);
	
	if(getApp()->alarm > 0)
		ShowAlarm(1);	
	
	battery_daemon(true);
	
	idle_poweroff();	// 重置 空闲自动关机 定时器
}

int  ShowWifiV(int state, bool twinkle)		// -1, 0, 1, 2, 3
{
	if(SND_LINEIN_IN == getApp()->linein
	|| BT_STATE_STARTED == getApp()->bluetooth
	|| BT_STATE_CONNECTED == getApp()->bluetooth)
	{
		return -1;
	}
	
	return ShowWifi(state, twinkle);
}

int ShowBluetoothV(int state) 				// state =0,ic_bluetooth.bmp; =1,ic_bluetooth_cn.bmp; -1,隐藏
{
	if(SND_LINEIN_IN == getApp()->linein
	|| BT_STATE_STOPPED == getApp()->bluetooth)
	{
		return -1;
	}
	
	return ShowBluetooth(state);
}

int ShowLineinV(int show) 					// show>0, 显示; show<0, 隐藏
{
	if(SND_LINEIN_IN != getApp()->linein)
	{
		return -1;
	}
	
	return ShowLinein(show);
}

int UpdateModeIcon(void)
{
	if(SND_LINEIN_IN == getApp()->linein)
	{
		return ShowLinein(1);
	}
	
	if(BT_STATE_STARTED == getApp()->bluetooth
	|| BT_STATE_CONNECTED == getApp()->bluetooth)
	{		
		return ShowBluetooth(getApp()->bluetooth);
	}
	
	return ShowWifi(getApp()->wifi_linked_state, false);
}

void UpdateMode(void)
{
	if(getApp()->linein == SND_LINEIN_IN)
	{
		getApp()->curMode = MODE_LINEIN;
		getApp()->curView = VIEW_LINEIN;
	}
	else if(BT_STATE_STARTED == getApp()->bluetooth
	     || BT_STATE_CONNECTED == getApp()->bluetooth)
	{
		getApp()->curMode = MODE_BLUETOOTH;
		getApp()->curView = VIEW_BLUETOOTH;
	}
	else
	{
		getApp()->curMode = MODE_WIFI;
	}
}

int ViewsInit(void)
{
	int iError = 0;
	
	iError |= UpgradeViewInit(); // 一定要放在第一个
	
	iError  = PlayViewInit();
	iError |= ChannelViewInit();
	iError |= VolumeViewInit();
	iError |= SetViewInit();
	iError |= RecordViewInit();
	iError |= VoiceViewInit();
	iError |= AbViewInit();
	iError |= DownloadViewInit();
	iError |= LockViewInit();
	iError |= LightViewInit();
	iError |= BluetoothViewInit();
	iError |= LineinViewInit();
	iError |= PowerViewInit();
	iError |= AirplayViewInit();
	
	return iError;
}

