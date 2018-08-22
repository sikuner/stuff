
#include "conf.h"
#include "log.h"
#include "types.h"
#include "view_manager.h"
#include "utf8_strings.h"
#include "application.h"

#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <mpd/idle.h>
#include <mpd/client.h>
#include <libubox/uloop.h>

static bool dispatchKeyEventPlay(PT_InputEvent ptInputEvent);
static bool onKeyEventPlay(PT_InputEvent ptInputEvent);
static void EnterPlay(void);
static void ExitPlay(void);

static T_ViewAction g_tPlayViewAction = {
	.id 				= VIEW_PLAY,
	
	.dispatchKeyEvent 	= dispatchKeyEventPlay,
	.onKeyEvent			= onKeyEventPlay,
	
	.Enter				= EnterPlay,
	.Exit				= ExitPlay
};

static char *channels[] = { RHYMES, ENGLISH, HABITS, STORIES, RECORDS, DOWNLOADS };

void ShowPlayView(void)
{	
	if(MODE_WIFI != getApp()->curMode)
	{
		return;
	}
	
	EnterPlay();
	
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

////////////////////////////poweroff begin///////////////////////////////////
static bool poweroff()
{
//	fprintf(stderr, " xxxx poweroff() xxxx \n");
	
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
	
	execlp("poweroff", "poweroff", NULL);
	exit(1);
}
static struct uloop_process poweroff_sound_proc;
static void poweroff_sound_cb(struct uloop_process *p, int ret)
{
//	fprintf(stderr, " xxxx poweroff_sound_cb() xxxx \n");
	amp_close();
	uloop_end();
	poweroff();
}
static int poweroff_sound(char *filename)
{
	if(NULL==filename || 0==strlen(filename))
	{
		return -1;
	}
	
	pid_t pid = vfork();
	if (pid < 0)
	{
		fprintf(stderr, "fork error!");
		return -1;
	}
	if (pid > 0) // > 0, 处于父进程中. add后返回
	{
		poweroff_sound_proc.pid = pid;
		poweroff_sound_proc.cb = poweroff_sound_cb;
		uloop_process_add(&poweroff_sound_proc);
		return 0;
	}
	
	execlp("/usr/bin/playsound.sh", "playsound.sh", filename, NULL);
	exit(1);
}
static bool kill_start_guid(void)
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
	
	execlp("killall", "killall", "start_guid.sh", NULL);
	exit(1);
}

static void poweroff_delay_cb(struct uloop_timeout *timeout)
{
//	fprintf(stderr, " xxxx poweroff_delay_cb() xxxx \n");
	amp_close();
	uloop_end();
	poweroff();
}
static struct uloop_timeout poweroff_delay_timer = {
	.cb = poweroff_delay_cb
};

static void uloop_poweroff_cb(struct uloop_timeout *timeout)
{
//	fprintf(stderr, " xxxx uloop_poweroff_cb() xxxx \n");
	
	char *sound = NULL;
	char *tips  = NULL;
	Rect full_rect = { .x = 0, .y = 0, .w = WINDOW_WIDTH, .h = WINDOW_HEIGHT };
	
	kill_start_guid();
	
	if(MODE_WIFI == getApp()->curMode)
	{
		mpdclient_pause();		
	}
	else if(MODE_BLUETOOTH == getApp()->curMode)
	{
		bt_stop_nocb();
	}
	
	if(getApp()->charge_off_count >= CHARGE_OFF_COUNT)
	{
		sound = SOUND_CHARGE_OFF;
		tips  = ICON_CHARGE_OFF;
		
		uloop_timeout_set(&poweroff_delay_timer, POWEROFF_DELAY_PERIOD);
		poweroff_sound(sound);
	}
	else if(getApp()->off_count >= BATTERY_OFF_COUNT)
	{
		sound = SOUND_LOW_POWER_OFF;
		tips  = ICON_LOW_POWER_OFF;
		
		uloop_timeout_set(&poweroff_delay_timer, POWEROFF_DELAY_PERIOD);
		poweroff_sound(sound);
	}
	else
	{
//		sound = SOUND_BYEBYE;
		tips  = ICON_POWEROFF;
		
		uloop_timeout_set(&poweroff_delay_timer, 10*TIMER_ALIVE_UNIT);
	}
	
	ToastBmp(&full_rect, tips, 100*TOAST_LONG_PERIOD);
	
	InputDevicesExit();
	WidgetExit();
}
struct uloop_timeout uloop_poweroff_timer = {
	.cb = uloop_poweroff_cb
};
///////////////////////////poweroff end////////////////////////////////////////////

static bool dispatchKeyEventPlay(PT_InputEvent ptInputEvent)
{
	if(BTN_PLAY != ptInputEvent->iCode 
	&& BTN_NEXT != ptInputEvent->iCode 
	&& BTN_LAST != ptInputEvent->iCode)
	{
		return false;
	}
	
	if(MODE_LINEIN == getApp()->curMode && BTN_PLAY != ptInputEvent->iCode )
	{
		if(ACTION_DOWN == ptInputEvent->iValue)
		{
			Toast(NULL, get_mode_strerror(), DEFAULT_FONT_SIZE, TOAST_SHORT_PERIOD);
		}
		
		return true;
	}
	
	return onKeyEventPlay(ptInputEvent);
}

static bool onKeyEventPlay(PT_InputEvent ptInputEvent)
{
	if(ACTION_DOWN == ptInputEvent->iValue)
	{
		if(BTN_PLAY == ptInputEvent->iCode)
		{
			uloop_timeout_set(&uloop_poweroff_timer, PLAY_POWEROFF_PERIOD);
		}
		
		return true;
	}
	else
	{
		if(BTN_PLAY == ptInputEvent->iCode)
		{
			if(uloop_timeout_remaining(&uloop_poweroff_timer) > 0) // 短按
			{
				uloop_timeout_cancel(&uloop_poweroff_timer);
			}
			else // 长按 关机
			{
				return true;
			}
		}
		
		if(MODE_LINEIN == getApp()->curMode) // MODE_LINEIN不支持短按.仅支持长按关机
		{
			Toast(NULL, get_mode_strerror(), DEFAULT_FONT_SIZE, TOAST_SHORT_PERIOD);
			return true;
		}
		if(MODE_BLUETOOTH == getApp()->curMode) // MODE_BLUETOOTH支持短按. 上一曲/下一曲/播放/暂停
		{
			if(get_bt_state() != BT_STATE_CONNECTED)
			{
				Toast(NULL, STRING_BLUETOOTH_UNCONN, DEFAULT_FONT_SIZE, TOAST_SHORT_PERIOD);
				return true;
			}
			
			if(BTN_PLAY == ptInputEvent->iCode)
			{
				bt_toggle();
			}
			else if(BTN_NEXT == ptInputEvent->iCode)
			{
				bt_next();
			}
			else if(BTN_LAST == ptInputEvent->iCode)
			{
				bt_prev();
			}
			
			return true;
		}
		
		// 在WiFi模式下, 处理按键的逻辑
		if(BTN_PLAY == ptInputEvent->iCode)
		{
			if(getApp()->channel >= 0)
			{
				mpdclient_set_event_handler(NULL);
				ClearAllText();
				uloop_timeout_set(&uloop_play_timer, TIMER_ALIVE_UNIT);
				mpdclient_local_play(channels[getApp()->channel]);
				getApp()->channel = -1;
			}
			else
			{
				if(MPD_STATE_PLAY == mpdclient_get_state())
				{
					EnterPlay();
					mpdclient_pause();
				}
				else
				{
					if(!network_get_connected() && mpdclient_current_uri_online()) // 离线时,播放在线内容
					{
						Toast(NULL, STRING_OFFLINE,  DEFAULT_FONT_SIZE, TOAST_SHORT_PERIOD);
					}
					else
					{
						EnterPlay();
						mpdclient_play();
					}
				}
			}
		}
		else if(BTN_NEXT == ptInputEvent->iCode)
		{
			EnterPlay();
			mpdclient_next();
		}
		else if(BTN_LAST == ptInputEvent->iCode)
		{
			EnterPlay();
			mpdclient_prev();
		}
 	}
	
	return true;
}

static void EnterPlay(void)
{
	if(getApp()->curView != VIEW_PLAY)
	{
		T_ViewAction *view = View(getApp()->curView);
		if(view && view->Exit)
			view->Exit();
		
		getApp()->curView = VIEW_PLAY;		
	}
	
	mpdclient_set_event_handler(mpd_idleloop_proc);	
	ToastCancel();
	fprintf(stderr, " <End> EnterPlay curView: %d \n", getApp()->curView);
}

static void ExitPlay(void)
{
	uloop_timeout_cancel(&uloop_poweroff_timer);
	fprintf(stderr, " ExitPlay curView: %d \n", getApp()->curView);
}

int PlayViewInit(void)
{
	return RegisterViewAction(&g_tPlayViewAction);
}

int ShowWiFiMode(void)
{
	UpdateMode();
	if(MODE_WIFI != getApp()->curMode)
		return -1;
	
	if(!getApp()->wifi_conn_ok)
	{
		network_start();		
		wifi_daemon(true);
	}
	
	UpdateModeIcon();
	ShowPlayView();
	
	return 0;
}

