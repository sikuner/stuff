
#include "conf.h"
#include "log.h"
#include "view_manager.h"
#include "utf8_strings.h"
#include "application.h"
#include "mpdclient.h"

#include <errno.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <libubox/uloop.h>

static bool dispatchKeyEventAb(PT_InputEvent ptInputEvent);
static bool onKeyEventAb(PT_InputEvent ptInputEvent);
static void EnterAb(void);
static void ExitAb(void);

static T_ViewAction g_tAbViewAction = {
	.id 				= VIEW_AB,
	.dispatchKeyEvent 	= dispatchKeyEventAb,
	.onKeyEvent			= onKeyEventAb,
	.Enter				= EnterAb,
	.Exit				= ExitAb
};

static volatile int in_recording = 0;
static volatile int in_playing = 0;

static time_t record_begin = 0;
static int recorded_sec = 0;

static int start_ab_rec(void)
{
	int enabled = mpdclient_enabled(AB_OUTPUT_NAME);
	if(enabled > 0)
		mpdclient_disable(AB_OUTPUT_NAME);
	
	return mpdclient_enable(AB_OUTPUT_NAME);
}
static int stop_ab_rec(void)
{
	int count = 3;
	int enabled = -1;
	
	mpdclient_disable(AB_OUTPUT_NAME);
	
	while (count-- > 0)
	{
		enabled = mpdclient_enabled(AB_OUTPUT_NAME);
		if(0 != enabled) // output Recorder disabled
			mpdclient_disable(AB_OUTPUT_NAME);
		else
			break;
		
		usleep(100*1000);
	}
	
	if(0 != enabled)
	{
		mpd_kill();
	}
	
	return 0;
}

static int start_ab_play(void);

static void recording_cb(struct uloop_timeout *timeout)
{
	char buf[16] = { 0 };
	int rec_sec = time(NULL) - record_begin;
	
	if(rec_sec > recorded_sec)
	{
		recorded_sec = rec_sec;
		sprintf(buf, "%02d\"", rec_sec);
		ShowLine3(buf);
	}
	
	if(rec_sec < AB_RECORD_PERIOD)
	{
		uloop_timeout_set(timeout, 5*TIMER_ALIVE_UNIT);
	}
	else // 超时忽略此次操作
	{
		Toast(NULL, STRING_AB_TIMEOUT, DEFAULT_FONT_SIZE, TOAST_LONG_PERIOD);
		ClearAllText();
		
		recorded_sec = 0;
		in_recording = 0;
		
		stop_ab_rec();
		
		uloop_timeout_set(&uloop_play_timer, 20*TIMER_ALIVE_UNIT);
		mpdclient_play();
	}
}
static struct uloop_timeout recording_timer = {
	.cb = recording_cb
};

static struct uloop_process ab_play_proc; // 循环播放
static void ab_play_cb(struct uloop_process *p, int ret)
{
	if(in_playing > 0)
	{
		start_ab_play();
	}
	else
	{
		uloop_timeout_set(&uloop_play_timer, TIMER_ALIVE_UNIT);
		mpdclient_play();
	}
}
static int start_ab_play(void)
{
	pid_t pid = vfork();
	if (pid < 0)
	{
		fprintf(stderr, "fork error!");
		return -1;
	}
	if (pid > 0) // > 0, 处于父进程中. add后返回
	{
		ab_play_proc.pid = pid;
		ab_play_proc.cb = ab_play_cb;
		uloop_process_add(&ab_play_proc);
		
		return 0;
	}
	
//  aplay --vumeter=stereo /tmp/ab.wav
	execlp("aplay", "aplay", "--vumeter=stereo", AB_RECORD_FILE, NULL);
	exit(1);
}
static void stop_ab_play(void)
{
	if(ab_play_proc.pending)
		kill(ab_play_proc.pid, SIGINT);
}

static void clear_ab(void)
{
	int count = 0;
	pid_t pid;
	int ret;
	
	in_recording = 0;
	in_playing = 0;
	
	record_begin = 0;
	recorded_sec = 0;
	
	uloop_timeout_cancel(&recording_timer);
	uloop_timeout_cancel(&uloop_play_timer);
	
	stop_ab_rec();
	
	if(ab_play_proc.pending)
	{
		uloop_process_delete(&ab_play_proc);
		kill(ab_play_proc.pid, SIGINT);
		
		count = 10;
		while (count-- > 0)
		{
			pid = waitpid(ab_play_proc.pid, &ret, WNOHANG);
			if(pid > 0)
				break;
			
			usleep(100*1000);
		}
	}
	
	count = 3;
	while (count-- > 0)
	{
		if(0 == access(AB_RECORD_FILE, F_OK)) // 文件存在
			remove(AB_RECORD_FILE);
		else
			break;
		
		usleep(100*1000);
	}
}

static bool dispatchKeyEventAb(PT_InputEvent ptInputEvent)
{
	if(BTN_AB!=ptInputEvent->iCode)
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
	
	return onKeyEventAb(ptInputEvent);
}

static bool onKeyEventAb(PT_InputEvent ptInputEvent)
{
	if(ACTION_DOWN == ptInputEvent->iValue)
	{
		if(in_playing<=0 && in_recording<=0)
		{
			getApp()->mpd_state = mpdclient_get_state();
			getApp()->airplay = get_airplay_state();
		}
		
		return true;
	}
	
	if(in_playing > 0) // 已在播放中
	{
		in_playing = 0;
		stop_ab_play();
	}
	else if(in_recording > 0) // 已在录音中
	{
		uloop_timeout_cancel(&recording_timer);
		
		ShowLine1(STRING_AB_REPEAT);
		ShowLine2(STRING_AB_STOP);
		ShowLine3(STRING_BLANK);
		
		mpdclient_pause();
		
		stop_ab_rec();
		
		if(0 == start_ab_play())
		{
			in_playing = 1;
		}
	}
	else
	{
		if(getApp()->airplay > 0)
		{
			Toast(NULL, STRING_AIRPLAY_UNSUPPORTED, DEFAULT_FONT_SIZE, TOAST_SHORT_PERIOD);
		}
		else if(MPD_STATE_PLAY != getApp()->mpd_state)
		{
			Toast(NULL, STRING_AB_TRY_PLAY, DEFAULT_FONT_SIZE, TOAST_SHORT_PERIOD);
		}
		else
		{
			clear_ab();
			
			if(0 == start_ab_rec())
			{
				EnterAb();
				in_recording = 1;
				
				record_begin = time(NULL);
				recorded_sec = 0;
				
				ShowLine1(STRING_AB_REPEAT);
				ShowLine2(STRING_AB_STARTED);
				ShowLine3("00\"");
				uloop_timeout_set(&recording_timer, TIMER_ALIVE_UNIT);
			}
		}
	}
	
 	return true;
}

static void EnterAb(void)
{
	if(getApp()->curView != VIEW_AB)
	{
		T_ViewAction *view = View(getApp()->curView);
		if(view && view->Exit)
			view->Exit();
		
		ToastCancel();	
		mpdclient_set_event_handler(NULL); 
		getApp()->curView = VIEW_AB;		
	}
}

static void ExitAb(void)
{
	clear_ab();
}

int AbViewInit(void)
{
	return RegisterViewAction(&g_tAbViewAction);
}

