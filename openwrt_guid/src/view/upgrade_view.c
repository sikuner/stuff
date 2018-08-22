
#include "conf.h"
#include "log.h"
#include "view_manager.h"
#include "utf8_strings.h"
#include "application.h"

#include <errno.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <libubox/uloop.h>

static bool dispatchKeyEventUpgrade(PT_InputEvent ptInputEvent);
static bool onKeyEventUpgrade(PT_InputEvent ptInputEvent);
static void EnterUpgrade(void);
static void ExitUpgrade(void);

static T_ViewAction g_tUpgradeViewAction = {
	.id 				= VIEW_UPGRADE,
	.dispatchKeyEvent 	= dispatchKeyEventUpgrade,
	.onKeyEvent			= onKeyEventUpgrade,
	.Enter				= EnterUpgrade,
	.Exit				= ExitUpgrade
};

void SendUpgradeEvent(void)
{
	T_InputEvent tInputEvent;
	
	get_uptime(&tInputEvent.tTime);
	
	tInputEvent.iType = EV_BTN;
	
	tInputEvent.iCode = SIM_UPGRADE;
	
	tInputEvent.iValue = ACTION_DOWN;
	dispatchKeyEventGroup(&tInputEvent);
	
	tInputEvent.iValue = ACTION_UP;
	dispatchKeyEventGroup(&tInputEvent);	
}

// -1 错误, 0 不满足, 1 满足
int BatteryUpgradeCond(void)
{
	int charge  = -1;
	int voltage = -1;
	
	charge  = get_charge_state();
	voltage = get_battery_voltage();
	if(charge<0 || voltage<0)
		return -1;
	
	int  cur_percent = 0;
	bool cur_charge = false;
	
	if(PWR_CHARGING==charge || PWR_CHARGE_FULL==charge)
		cur_charge = true;
	
	cur_percent = get_battery_percent(voltage, cur_charge);
	
	if(( cur_charge && cur_percent>10)  // 充电时,电量在10%以上
	|| (!cur_charge && cur_percent>20)) // 未充电时,电量在20%以上
	{
		return 1;
	}
	
	return 0;
}

static void upgrade_quit_cb(struct uloop_timeout *timeout)
{
	ShowLineinMode();
	ShowBluetoothMode();
	ShowWiFiMode();
}
static struct uloop_timeout upgrade_quit_timer = {
	.cb = upgrade_quit_cb
};
static void upgrade_cb(struct uloop_timeout *timeout)
{
	getApp()->upgrade = -1; // 恢复原来的状态,不再拦截按键
	
	Rect fullRect = { .x = 0, .y = 0, .w = WINDOW_WIDTH, .h = WINDOW_HEIGHT }; // 全屏提示
	Toast(&fullRect, STRING_UPGRADE_TIMEOUT, DEFAULT_FONT_SIZE, TOAST_LONG_PERIOD);
	uloop_timeout_set(&upgrade_quit_timer, TOAST_SHORT_PERIOD);
}
static struct uloop_timeout upgrade_timer = {
	.cb = upgrade_cb
};

static bool dispatchKeyEventUpgrade(PT_InputEvent ptInputEvent)
{
	if(getApp()->upgrade < 0)
	{
		return false;
	}
	if((MOD_CHARGING|MOD_CHARGE_FULL|MOD_UNCHARGE) 
	  & getButtonModeParse(ptInputEvent))
	{
		return false;
	}
	
	return onKeyEventUpgrade(ptInputEvent);
}

static bool onKeyEventUpgrade(PT_InputEvent ptInputEvent)
{
	if(ACTION_DOWN == ptInputEvent->iValue)
	{
		if(SIM_UPGRADE == ptInputEvent->iCode)
		{
			EnterUpgrade();
			
			StatusEnable(false);
			
			char *ic_upgrade[] = { ICON_UPGRADE_NORMAL, ICON_UPGRADE_FORCE };
			Rect fullRect = { .x = 0, .y = 0, .w = WINDOW_WIDTH, .h = WINDOW_HEIGHT };
			
			ShowBmp(&fullRect, ic_upgrade[getApp()->upgrade]);
		}
		else if(BTN_PLAY == ptInputEvent->iCode)
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
		
		if(BTN_PLAY == ptInputEvent->iCode)
		{
			Rect fullRect = { .x = 0, .y = 0, .w = WINDOW_WIDTH, .h = WINDOW_HEIGHT }; // 全屏提示
			Toast(&fullRect, STRING_UPGRADE_START, DEFAULT_FONT_SIZE, TOAST_SHORT_PERIOD*60);
			
			uloop_timeout_set(&upgrade_timer, UPGRADE_TIMEOUT_PERIOD);			
			start_upgrade();
		}
		else if(BTN_VOICE==ptInputEvent->iCode && 0==getApp()->upgrade)
		{
			getApp()->upgrade = -1;
			uloop_timeout_set(&upgrade_quit_timer, TIMER_ALIVE_UNIT);
		}
 	}
	
 	return true;
}

static void EnterUpgrade(void)
{
	if(getApp()->curView != VIEW_UPGRADE)
	{
		T_ViewAction *view = View(getApp()->curView);
		if(view && view->Exit)
			view->Exit();
		
		mpdclient_set_event_handler(NULL);
		getApp()->curView = VIEW_UPGRADE; 	
	}
	
//	DBG_PRINTF("curView: %d, curMode: %d \n",
//		getApp()->curView,
//		getApp()->curMode);
}

static void ExitUpgrade(void)
{
	StatusEnable(true);
	Rect fullRect = { .x = 0, .y = 0, .w = WINDOW_WIDTH, .h = WINDOW_HEIGHT };
	UpdateRect(&fullRect);
	
//	DBG_PRINTF("curView: %d, curMode: %d \n",
//		getApp()->curView,
//		getApp()->curMode);
}

int UpgradeViewInit(void)
{
	return RegisterViewAction(&g_tUpgradeViewAction);
}

