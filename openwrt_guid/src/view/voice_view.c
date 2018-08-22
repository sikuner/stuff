
#include <errno.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <libubox/uloop.h>
#include <json/json_object.h>
#include <json/json_tokener.h>

#include "conf.h"
#include "log.h"
#include "utf8_strings.h"
#include "application.h"
#include "view_manager.h"
#include "common.h"

static bool dispatchKeyEventVoice(PT_InputEvent ptInputEvent);
static bool onKeyEventVoice(PT_InputEvent ptInputEvent);
static void EnterVoice(void);
static void ExitVoice(void);

static T_ViewAction g_tVoiceViewAction = {
	.id 				= VIEW_VOICE,
	.dispatchKeyEvent 	= dispatchKeyEventVoice,
	.onKeyEvent			= onKeyEventVoice,
	.Enter				= EnterVoice,
	.Exit				= ExitVoice
};

#define	VOICE_SHORT_FLAG 		"/tmp/voice_short_flag"
#define	VOICE_RECORDING_FLAG 	"/tmp/voice_recording_flag"

#define	XFYUN_RESULT_FILE		"/tmp/kw.txt"			// sfvoice result
#define VOICE_RESULT_FILE 		"/tmp/voice_result.txt" // btools result by alex

enum return_result_code {

	RESULT_OK,
	OUT_OF_MEMORY,
	KEYWORD_NULL,
	NETWORK_ISSUE,
	NEED_LOGIN,
	INVALID_REQUEST,
	SONG_NOT_FOUND,

};

static char xf_result_content[2*MAX_LINE] 		=  { 0 };

static void clear_voice(void);

static void voice_restore_cb(struct uloop_timeout *timeout)
{
	if(PLAYER_STATE_PLAY == getApp()->mpd_state) 
	{
//		mpdclient_play(); // 取消,恢复播放
	}
	uloop_timeout_set(&uloop_play_timer, TIMER_ALIVE_UNIT);
}
static struct uloop_timeout voice_restore_timer = { // 定时器: 回退到播放模式 
	.cb = voice_restore_cb
};

static void voice_timeout_cb(struct uloop_timeout *timeout);

struct uloop_timeout voice_timeout_timer = { 	// 定时器: 回退到播放模式 
	.cb = voice_timeout_cb
};

static struct uloop_process od_proc; 			// on demand - 点播
static void od_proc_cb(struct uloop_process *p, int ret)
{
	int od_result = -1;
	char od_msg[MAX_LINE] = { 0 };
	
	get_od_result(VOICE_RESULT_FILE, &od_result, od_msg);
	if(RESULT_OK == od_result)
	{
		uloop_timeout_set(&uloop_play_timer, 1000);
	}
	else
	{
		ShowLine1(STRING_VOICE_OD_ERR);
		ShowLine3(od_msg);
		clear_voice();
		uloop_timeout_set(&voice_restore_timer, 2000);
	}
}

static bool start_od(void)
{
	pid_t pid = vfork();
	if (pid < 0)
	{
		fprintf(stderr, "fork error!");
		return false;
	}
	if (pid > 0) // > 0, 处于父进程中. add后返回
	{
		od_proc.pid = pid;
		od_proc.cb = od_proc_cb;
		uloop_process_add(&od_proc);
		
		uloop_timeout_set(&voice_timeout_timer, BBYUN_OD_TIMEOUT);
		
		return true;
	}
	
	char cmd_line[4*MAX_LINE] = { 0 };
	sprintf(cmd_line, "btools voice %s 1>/var/log/voice_search.log 2>&1", xf_result_content);
//	fprintf(stderr, "cmd_line: %s \n", cmd_line);
	
//	execlp("btools", "btools", "voice", xf_result_content, NULL);	
	execlp("/bin/sh", "/bin/sh", "-c", cmd_line, NULL); 
	exit(1);
}

static struct uloop_process voice_proc;
static void voice_proc_cb(struct uloop_process *p, int r)
{
	int ret = -1;
	int rc = -1;
	char text[MAX_LINE] = { 0 };
	char strerrcode[MAX_LINE] = { STRING_VOICE_NET_TIMEOUT };
	
	memset(xf_result_content, 0, sizeof(xf_result_content));
	ret = parse_xf_result(XFYUN_RESULT_FILE, xf_result_content, &rc, text, strerrcode);
	
	if(0==ret && (0==rc || 4==rc)) // text, 成功
	{
		ShowLine3(STRING_BLANK);
		ShowLine2(text);
		start_od();
	}
	else // 失败
	{
		ShowLine1(STRING_VOICE_SEARCH);		
		ShowLine2(STRING_VOICE_PARDON);		
		ShowLine3(strerrcode);
		
		clear_voice();
		uloop_timeout_set(&voice_restore_timer, 2000);
	}
}
static bool start_voice(void)
{
	int count = 20;
	int ret = -1;
	
	pid_t pid = vfork();
	if (pid < 0)
	{
		fprintf(stderr, "fork error!");
		return false;
	}
	if (pid > 0) // > 0, 处于父进程中. add后返回
	{
		voice_proc.pid = pid;
		voice_proc.cb = voice_proc_cb;
		
		while (count-- > 0)
		{
			ret = access(XFYUN_RESULT_FILE, F_OK);
			fprintf(stderr, "access ret: %d \n", ret);
			if(0 == ret)
				break;
			
			usleep(100*1000);
		}
		
		if(0 == ret)
		{
			uloop_process_add(&voice_proc);
			return true;
		}
		else
		{
			return false;
		}
	}
	
	execlp("sfvoice", "sfvoice", NULL);
	exit(1);
}
static void stop_voice(void)
{
	if(voice_proc.pending)
		kill(voice_proc.pid, SIGUSR2);
}

static void voice_recording_cb(struct uloop_timeout *timeout)
{
	if(access(VOICE_RECORDING_FLAG, F_OK) < 0) // 文件不存在, 录音结束了
	{
		ShowLine1Waiting();
		ShowLine2(STRING_VOICE_SEARCHING);
		ShowLine3(STRING_BLANK);
		uloop_timeout_set(&voice_timeout_timer, XFYUN_IAT_TIMEOUT);
		
		return;
	}
	
	uloop_timeout_set(timeout, TIMER_ALIVE_UNIT);
}
static struct uloop_timeout voice_recording_timer = {
	.cb = voice_recording_cb
};

static void uloop_searching_cb(struct uloop_timeout *timeout)
{
	if(access(VOICE_SHORT_FLAG, F_OK) < 0) // 文件不存在, 证明已经不是短按, 至少 >0.5s
	{
		ShowLine1Waiting();
		ShowLine2(STRING_VOICE_SEARCHING);
		ShowLine3(STRING_BLANK);
		uloop_timeout_set(&voice_timeout_timer, XFYUN_IAT_TIMEOUT);
	}
	else
	{
		ShowLine1(STRING_VOICE_SEARCH);
		ShowLine2(STRING_VOICE_RECORDING);
		ShowLine3(STRING_BLANK);
		uloop_timeout_set(&voice_recording_timer, TIMER_ALIVE_UNIT);
	}
}
static struct uloop_timeout uloop_searching_timer = {
	.cb = uloop_searching_cb
};

static int kill_sfvoice(void)
{
	pid_t pid = vfork();
	if (pid < 0)
	{
		fprintf(stderr, "fork error!");
		return -1;
	}
	if (pid > 0) // > 0, 处于父进程中. add后返回
	{
		return 0;
	}
	
	execlp("killall", "killall", "sfvoice", NULL);
	exit(1);
}

static int kill_btools(void)
{
	pid_t pid = vfork();
	if (pid < 0)
	{
		fprintf(stderr, "fork error!");
		return -1;
	}
	if (pid > 0) // > 0, 处于父进程中. add后返回
	{
		return 0;
	}
	
	execlp("killall", "killall", "btools", NULL);
	exit(1);
}

static void voice_timeout_cb(struct uloop_timeout *timeout)
{
	if(voice_proc.pending)
	{
		int rc = -1;
		char text[MAX_LINE] = { 0 };
		char strerrcode[MAX_LINE] = { STRING_VOICE_NET_TIMEOUT };
		memset(xf_result_content, 0, sizeof(xf_result_content));
		
		parse_xf_result(XFYUN_RESULT_FILE, xf_result_content, &rc, text, strerrcode);
		
		ShowLine3(strerrcode);
		ShowLine2(STRING_VOICE_NET_BAD);
		
		clear_voice();
		uloop_timeout_set(&voice_restore_timer, 2000);
	}
	else if(od_proc.pending)
	{
		int od_result = -1;
		char od_msg[MAX_LINE] = { "NULL" };
		get_od_result(VOICE_RESULT_FILE, &od_result, od_msg);
		if(RESULT_OK != od_result)
			ShowLine3(od_msg);
		
		ShowLine2(STRING_VOICE_TIMEOUT);
		clear_voice();
		uloop_timeout_set(&voice_restore_timer, 2000);
	}
}

static void clear_status(void)
{
	int count = 0;
	
	if(voice_proc.pending)
	{
		uloop_process_delete(&voice_proc);
		kill(voice_proc.pid, SIGKILL);
	}
	kill_sfvoice();
	
	if(od_proc.pending)
	{
		uloop_process_delete(&od_proc);
		kill(od_proc.pid, SIGKILL);
	}
	kill_btools();
	
	uloop_timeout_cancel(&voice_recording_timer);
	uloop_timeout_cancel(&uloop_searching_timer);
	uloop_timeout_cancel(&uloop_play_timer);
	uloop_timeout_cancel(&voice_timeout_timer);
	uloop_timeout_cancel(&voice_restore_timer);
	
	mpdclient_clearerror();
	
	count = 3;
	while (count-- > 0)
	{
		if(0 == access(VOICE_SHORT_FLAG, F_OK)) // 文件存在
			remove(VOICE_SHORT_FLAG);
		else
			break;
		
		usleep(100*1000);
	}
	
	count = 3;
	while (count-- > 0)
	{
		if(0 == access(VOICE_RECORDING_FLAG, F_OK)) // 文件存在
			remove(VOICE_RECORDING_FLAG);
		else
			break;
		
		usleep(100*1000);
	}
}

static void clear_results(void)
{
	int count = 0;
	
	count = 3;
	while (count-- > 0)
	{
		if(0 == access(XFYUN_RESULT_FILE, F_OK)) // 文件存在
			remove(XFYUN_RESULT_FILE);
		else
			break;
		
		usleep(100*1000);
	}
	
	count = 3;
	while (count-- > 0)
	{
		if(0 == access(VOICE_RESULT_FILE, F_OK)) // 文件存在
			remove(VOICE_RESULT_FILE);
		else
			break;
		
		usleep(100*1000);
	}
}

static void clear_voice(void)
{
	clear_status();
	clear_results();
}

static bool dispatchKeyEventVoice(PT_InputEvent ptInputEvent)
{
	if(BTN_VOICE != ptInputEvent->iCode)
		return false;
	
	if(MODE_WIFI != getApp()->curMode)
	{
		if(ACTION_DOWN == ptInputEvent->iValue)
		{
			Toast(NULL, get_mode_strerror(), DEFAULT_FONT_SIZE, TOAST_SHORT_PERIOD);
		}
		
		return true;
	}
	
	return onKeyEventVoice(ptInputEvent);
}

static bool onKeyEventVoice(PT_InputEvent ptInputEvent)
{
	static bool voice_ok = false;
	
	if(ACTION_DOWN == ptInputEvent->iValue) // 语音搜索 开始
	{
		voice_ok = false;
		
		if(!network_get_connected())
		{			
			ToastBmp(NULL, ICON_OFFLINE_VOICE, TOAST_SHORT_PERIOD);
		}
		else
		{
			clear_voice();
			ClearAllText();
			EnterVoice();
			mpdclient_pause();
			ShowLine1(STRING_VOICE_SEARCH);
			ShowLine2(STRING_VOICE_SEARCH_TIPS);
			ShowLine3(STRING_BLANK);
			voice_ok = start_voice();
//			fprintf(stderr, " xYz start_voice() voice_ok: %s \n", voice_ok ? "True" : "False");
		}
	}
	else if(ACTION_UP == ptInputEvent->iValue) 	// 语音搜索 结束
	{
		if(voice_ok)
		{
			stop_voice();
			uloop_timeout_set(&uloop_searching_timer, TIMER_ALIVE_UNIT);
		}
		else
		{
			clear_voice();
			uloop_timeout_set(&voice_restore_timer, TIMER_ALIVE_UNIT);
		}
	}
	
	return true;
}

static void EnterVoice(void)
{
	if(getApp()->curView != VIEW_VOICE)
	{
		T_ViewAction *view = View(getApp()->curView);
		if(view && view->Exit)
			view->Exit();
		
		ToastCancel();
		mpdclient_set_event_handler(NULL);
		getApp()->curView = VIEW_VOICE;	
		getApp()->mpd_state = mpdclient_get_state();
	}
}

static void ExitVoice(void)
{
	clear_status(); // 离开该模式,清掉程序状态信息
}

int VoiceViewInit(void)
{
	return RegisterViewAction(&g_tVoiceViewAction);
}

