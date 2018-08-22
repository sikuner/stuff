
#include "conf.h"
#include "log.h"
#include "view_manager.h"
#include "utf8_strings.h"
#include "application.h"

#include <unistd.h>
#include <time.h>
#include <string.h>
#include <stdio.h>
#include <mpd/idle.h>
#include <mpd/client.h>
#include <libubox/uloop.h>
#include <errno.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

static bool dispatchKeyEventRecord(PT_InputEvent ptInputEvent);
static bool onKeyEventRecord(PT_InputEvent ptInputEvent);
static void EnterRecord(void);
static void ExitRecord(void);

static T_ViewAction g_tRecordViewAction = {
	.id 				= VIEW_RECORD,
	.dispatchKeyEvent 	= dispatchKeyEventRecord,
	.onKeyEvent			= onKeyEventRecord,
	.Enter				= EnterRecord,
	.Exit				= ExitRecord
};

static volatile int in_recording = 0;
static time_t record_begin = 0;
static int recorded_sec = 0;
static char record_basename[512] = { 0 };
static char record_name[512] = { 0 };

static void clear_record(void);
static void stop_record(void);

static void uloop_recording_cb(struct uloop_timeout *timeout)
{
	char buf[16] = { 0 };
	int rec_sec = time(NULL) - record_begin;
	if(rec_sec > recorded_sec)
	{
		recorded_sec = rec_sec;
		sprintf(buf, "%01d\'%02d\"", rec_sec/60, rec_sec%60);
		ShowLine2(buf);
	}
	if(rec_sec < RECORD_TIMELIMIT)
	{
		uloop_timeout_set(timeout, 5*TIMER_ALIVE_UNIT);
	}
	else
	{
		stop_record();		
		ToastBmp(NULL, ICON_RECORDED, TOAST_LONG_PERIOD);
		in_recording = 0;
		recorded_sec = 0;
		ClearAllText();
	}
}
static struct uloop_timeout uloop_recording_timer = {
	.cb = uloop_recording_cb
};

static void skip_play_cb(struct uloop_timeout *timeout)
{
	uloop_timeout_set(&uloop_play_timer, TIMER_ALIVE_UNIT);
	mpdclient_search_play(record_basename);
}
static struct uloop_timeout skip_play_timer = {
	.cb = skip_play_cb
};

static struct uloop_process record_proc;
static void record_proc_cb(struct uloop_process *p, int ret)
{	
	uloop_timeout_cancel(&uloop_recording_timer);
	in_recording = 0;
	
	uloop_timeout_set(&skip_play_timer, TOAST_SHORT_PERIOD);
	mpdclient_recorded_prepare(RECORDS);
}
static bool start_record(char *mp3_filename)
{
	int count = 20;
	int rc = -1;
	bool r = false; 
	
	pid_t pid = vfork();
	if (pid < 0)
	{
		fprintf(stderr, "fork error!");
		return false;
	}
	if (pid > 0) // > 0, 处于父进程中. add后返回
	{
		record_proc.pid = pid;
		record_proc.cb = record_proc_cb;
		uloop_process_add(&record_proc);
		
		while (count-- > 0)
		{
			rc = access(mp3_filename, F_OK);
			fprintf(stderr, "access [%s] rc: %d \n", mp3_filename, rc);
			if(0 == rc)
			{
				r = true;
				break;
			}
			
			usleep(100*1000);
		}
		
		return r;
	}
	
	execlp("mp3_record", "mp3_record", mp3_filename, NULL);
	exit(1);
}
static void stop_record(void)
{
	if(record_proc.pending)
	{
		int ret = kill(record_proc.pid, SIGUSR1);
		fprintf(stderr, "ret: %d, strerror(%d): %s \n", ret, errno, strerror(errno));
	}
}

static char* get_record_basename(void)
{
	static char mp3_name[512] = { 0 };
	
	time_t t;
	struct tm *p;
	
	time(&t);
	p = localtime(&t);
	
	memset(mp3_name, 0, sizeof(mp3_name));
	sprintf(mp3_name, "REC_%04d%02d%02d_%02d%02d%02d.mp3",
		(1900+p->tm_year), (1+p->tm_mon), p->tm_mday, p->tm_hour, p->tm_min, p->tm_sec);
	fprintf(stderr, "mp3_name: %s \n", mp3_name);
	
	return mp3_name;
}

static void start_record_cb(struct uloop_timeout *timeout)
{
	strcpy(record_basename, get_record_basename());
	sprintf(record_name, "%s/%s/%s", RECORDS_DIR, "user", record_basename);
	EnterRecord();
	clear_record();
	
	getApp()->mpd_state = mpdclient_get_state();
	if( PLAYER_STATE_PLAY == getApp()->mpd_state )
	{
		mpdclient_pause();
	}
	
	if( start_record(record_name) )
	{
		ShowLine1Twinkle(STRING_RECORDING);
		ShowLine2("0\'00\"");
		ShowLine3(STRING_RECORDING_TIPS);
		in_recording = 1;
		record_begin = time(NULL);
		recorded_sec = 0;
		uloop_timeout_set(&uloop_recording_timer, TIMER_ALIVE_UNIT);
	}
	else
	{
		if(PLAYER_STATE_PLAY == getApp()->mpd_state) 
		{
			mpdclient_play();
		}
		
		uloop_timeout_set(&uloop_play_timer, TIMER_ALIVE_UNIT);
	}
}
struct uloop_timeout start_record_timer = {
	.cb = start_record_cb
};

static void clear_record(void)
{
	if(record_proc.pending)
	{
		uloop_process_delete(&record_proc);
		int ret = kill(record_proc.pid, SIGKILL);
		fprintf(stderr, "ret: %d, strerror(%d): %s \n", ret, errno, strerror(errno));
	}
	
	in_recording = 0;
	recorded_sec = 0;
	uloop_timeout_cancel(&uloop_recording_timer);
	uloop_timeout_cancel(&start_record_timer);
}

static bool dispatchKeyEventRecord(PT_InputEvent ptInputEvent)
{
	if(BTN_RECORD != ptInputEvent->iCode)
		return false;
	
	if(MODE_WIFI != getApp()->curMode)
	{
		if(ACTION_DOWN == ptInputEvent->iValue)
		{	
			Toast(NULL, get_mode_strerror(), DEFAULT_FONT_SIZE, TOAST_SHORT_PERIOD);
		}
		
		return true;
	}
	
	return onKeyEventRecord(ptInputEvent);
}

static bool onKeyEventRecord(PT_InputEvent ptInputEvent)
{	
	if(ACTION_DOWN == ptInputEvent->iValue)
	{
		if(!in_recording)
		{
			uloop_timeout_set(&start_record_timer, START_RECORD_PERIOD);
		}
		else // 录音中
		{
			stop_record();
			ToastBmp(NULL, ICON_RECORDED, TOAST_LONG_PERIOD);
			ClearAllText();
			in_recording = 0;
			uloop_timeout_cancel(&uloop_recording_timer);
		}
	}
	else if(ACTION_UP == ptInputEvent->iValue)
	{
		if(uloop_timeout_remaining(&start_record_timer) > 0)
		{
			uloop_timeout_cancel(&start_record_timer);
			ToastBmp(NULL, ICON_RECORD_TIPS, TOAST_SHORT_PERIOD);
		}
	}
	
	return true;
}

static void EnterRecord(void)
{	
	if(getApp()->curView != VIEW_RECORD)
	{
		T_ViewAction *view = View(getApp()->curView);
		if(view && view->Exit)
			view->Exit();
		
		mpdclient_set_event_handler(NULL);
		getApp()->curView = VIEW_RECORD;		
	}
}

static void ExitRecord(void)
{
	if(record_proc.pending)
	{
		uloop_process_delete(&record_proc);
		int ret = kill(record_proc.pid, SIGTERM);
		fprintf(stderr, "ret: %d, strerror(%d): %s \n", ret, errno, strerror(errno));
	}
	
	in_recording = 0;
	recorded_sec = 0;
	uloop_timeout_cancel(&uloop_recording_timer);
	uloop_timeout_cancel(&start_record_timer);
}

int RecordViewInit(void)
{
	return RegisterViewAction(&g_tRecordViewAction);
}

