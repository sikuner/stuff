
#include "log.h"
#include "conf.h"
#include "application.h"
#include "view_manager.h"
#include "common.h"

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

static bool dispatchKeyEventPower(PT_InputEvent ptInputEvent);
static bool onKeyEventPower(PT_InputEvent ptInputEvent);
static void EnterPower(void);
static void ExitPower(void);

static T_ViewAction g_tPowerViewAction = {
	.id 				= VIEW_POWER,
	.dispatchKeyEvent 	= dispatchKeyEventPower,
	.onKeyEvent			= onKeyEventPower,
	.Enter				= EnterPower,
	.Exit				= ExitPower
};

static int 	idle_count = 0;
static int 	blink_count = 0;
static int	mpd_state = PLAYER_STATE_UNKNOWN;
static bool low_power_tips = false;

static int get_battery_detect_period(void)
{
	static int period = 0;	
	period += BATTERY_DETECT_PERIOD / 4;
	
	return period = min(period, BATTERY_DETECT_PERIOD);
}

static void light_blink_cb(struct uloop_timeout *timeout)
{
	if(blink_count > 59)
	{
		getApp()->screensaver = 0;
		return;
	}
	
	led_toggle();
	blink_count++;
	uloop_timeout_set(timeout, 5*TIMER_ALIVE_UNIT);
}
static struct uloop_timeout light_blink_timer = {
	.cb = light_blink_cb
};

static void idle_poweroff_cb(struct uloop_timeout *timeout)
{
	if( SND_LINEIN_OUT == getApp()->linein 						
	 && BT_STATE_STOPPED == getApp()->bluetooth								
	 && PWR_UNCHARGE == get_charge_state()						
	 && MPD_STATE_PLAY != mpdclient_get_state()
	 && 1 != get_airplay_state())
	{
//		fprintf(stderr, "idle poweroff idle_count: %d \n", idle_count);
		
		if(idle_count) // 0, 1
		{
			uloop_timeout_set(&uloop_poweroff_timer, TIMER_ALIVE_UNIT);
		}
		else
		{
			getApp()->screensaver = 1;
			Rect full_rect = { .x = 0, .y = 0, .w = WINDOW_WIDTH, .h = WINDOW_HEIGHT };
			ToastBmp(&full_rect, ICON_SCREENSAVER, IDLE_POWEROFF_PERIOD*2);
			
			blink_count = 0;
			uloop_timeout_set(&light_blink_timer, TIMER_ALIVE_UNIT);
		}
		
		idle_count++;
	}
	
	uloop_timeout_set(timeout, IDLE_POWEROFF_PERIOD);
}
static struct uloop_timeout idle_poweroff_timer = {
	.cb = idle_poweroff_cb
};

static void remote_poweroff_cb(struct uloop_timeout *timeout)
{
	uloop_timeout_set(&uloop_poweroff_timer, TIMER_ALIVE_UNIT);
}
static struct uloop_timeout remote_poweroff_timer = {
	.cb = remote_poweroff_cb
};
void remote_poweroff(void) 		// 远程关机
{
	uloop_timeout_set(&remote_poweroff_timer, REMOTE_POWEROFF_PERIOD);
	Rect full_rect = { .x = 0, .y = 0, .w = WINDOW_WIDTH, .h = WINDOW_HEIGHT };
	ToastBmp(&full_rect, ICON_REMOTE_OFF, -1);
	
	getApp()->off_remote = 1;
}

void idle_poweroff(void) 	// 重置 空闲自动关机 定时器
{
	idle_count = 0;
	uloop_timeout_set(&idle_poweroff_timer, IDLE_SCREENSAVER_PERIOD);
	
	if(getApp()->screensaver > 0)
	{
		uloop_timeout_cancel(&light_blink_timer);
		ToastCancel();
		led_off();
		
		getApp()->screensaver = 0;
	}
	if(getApp()->off_remote > 0)
	{
		uloop_timeout_cancel(&remote_poweroff_timer);
		ToastCancel();
		
		getApp()->off_remote = 0;
	}
}

static struct uloop_process power_low_sound_proc;
static void power_low_sound_cb(struct uloop_process *p, int ret)
{
	if(PLAYER_STATE_PLAY == mpd_state)
		mpdclient_play();
}
static int power_low_sound(char *filename)
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
		power_low_sound_proc.pid = pid;
		power_low_sound_proc.cb = power_low_sound_cb;
		uloop_process_add(&power_low_sound_proc);
		return 0;
	}
	
	execlp("/usr/bin/playsound.sh", "playsound.sh", filename, NULL);
	exit(1);
}

static void battery_daemon_cb(struct uloop_timeout *timeout)
{
	static int last_percent = -1;
	static bool last_charge = false;
	
	static int last_charge_off_voltage = -1;
	
	int cur_percent = 0;
	bool cur_charge = false;
	
	int charge  = get_charge_state();
	int voltage = get_battery_voltage();
	
	if(charge<0 || voltage<0)
	{		
		uloop_timeout_set(timeout, 10*TIMER_ALIVE_UNIT);
		return;
	}
	
	if(PWR_CHARGE_FULL == charge) // [100]
	{
		cur_charge = true;
		cur_percent = get_battery_percent(voltage, cur_charge);
		
		if(cur_percent > 80) // 充满容错处理,电量至少大于80%才有可能是充满状态
			cur_percent = 100;
		
		getApp()->low_count = 0;
		low_power_tips = false;
	}
	else if(PWR_CHARGING == charge)
	{
		cur_charge = true;
		cur_percent = get_battery_percent(voltage, cur_charge);
		
		getApp()->low_count = 0;
		low_power_tips = false;
	}
	else // if(PWR_UNCHARGE == charge) 或者出错 -1 == charge
	{
		cur_charge = false;
		cur_percent = get_battery_percent(voltage, cur_charge);
		
		if(cur_percent < BATTERY_LOW_LEVEL
		&& !low_power_tips)
		{
			getApp()->low_count++;
			fprintf(stderr, "getApp()->low_count: %d \n", getApp()->low_count);
			if(getApp()->low_count >= BATTERY_LOW_COUNT)
			{
				ToastBmp(NULL, ICON_LOW_POWER, TOAST_LONG_PERIOD);
				
				if( SND_LINEIN_OUT == getApp()->linein						
				&& BT_STATE_STOPPED == getApp()->bluetooth								
				&& 1 != get_airplay_state())
				{
					mpd_state = mpdclient_get_state();
					if(PLAYER_STATE_PLAY == mpd_state)
						mpdclient_pause();
					
					power_low_sound(SOUND_LOW_POWER);
				}
				
				low_power_tips = true;
			}
		}
		else if(cur_percent <= BATTERY_OFF_LEVEL)			// 低电量(5%)自动关机
		{
			getApp()->off_count++;
			fprintf(stderr, "off_count: %d \n", getApp()->off_count);
			if(getApp()->off_count >= BATTERY_OFF_COUNT)
			{
				uloop_timeout_set(&uloop_poweroff_timer, TIMER_ALIVE_UNIT);
			}
		}
		else
		{
			getApp()->off_count = 0;
		}	
	}
	
	if(cur_charge && cur_percent <= BATTERY_OFF_LEVEL)
	{
		getApp()->charge_off_count++;
		if((getApp()->charge_off_count >= CHARGE_OFF_COUNT)
		&& (voltage <= last_charge_off_voltage))
		{
			delay_poweron(DELAY_POWERON_PERIOD);
			uloop_timeout_set(&uloop_poweroff_timer, TIMER_ALIVE_UNIT);
		}
		
		last_charge_off_voltage = voltage;
	}
	else
	{
		getApp()->charge_off_count = 0;
	}
	
	if(last_percent != cur_percent 
	|| last_charge != cur_charge)
	{
		ShowBattery(cur_percent, cur_charge);
	}
	
	last_percent = cur_percent;
	last_charge = cur_charge;
	
	getApp()->charge  = charge;
	getApp()->voltage = voltage;
	
	uloop_timeout_set(timeout, get_battery_detect_period());
}

static struct uloop_timeout battery_daemon_timer = {
	.cb = battery_daemon_cb
};

void battery_daemon(bool on)
{
	if(on)
	{
		uloop_timeout_set(&battery_daemon_timer, 5);
	}
	else
	{
		uloop_timeout_cancel(&battery_daemon_timer);
	}
}

static bool dispatchKeyEventPower(PT_InputEvent ptInputEvent)
{
	if(PWR_CHARGING != ptInputEvent->iCode 
	&& PWR_CHARGE_FULL != ptInputEvent->iCode
	&& PWR_UNCHARGE != ptInputEvent->iCode)
		return false;
	
	return onKeyEventPower(ptInputEvent);
}

static bool onKeyEventPower(PT_InputEvent ptInputEvent)
{
	if(ptInputEvent->iValue != ACTION_UP)
		return true;
	
	uloop_timeout_set(&battery_daemon_timer, TIMER_ALIVE_UNIT);
	
	return true;
}

static void EnterPower(void)
{
	
}

static void ExitPower(void)
{
	
}

int PowerViewInit(void)
{
	return RegisterViewAction(&g_tPowerViewAction);
}

