
#include <pthread.h>
#include <stdio.h>

#include "conf.h"
#include "input_manager.h"
#include "view_manager.h"

static PT_InputOpr g_ptInputOprHead;

static const int const button_codes[] = {
	
	BTN_SET,
	
	BTN_CHANNELUP,
	BTN_CHANNELDOWN,
	BTN_AB,
	BTN_LAST,
	BTN_NEXT,
	BTN_DOWNLOAD,
	BTN_PLAY,
	
	BTN_VOLUMEUP,
	BTN_VOLUMEDOWN,
	BTN_BLUETOOTH,
	BTN_LIGHT,
	BTN_LOCK,
	BTN_RECORD,
	BTN_VOICE,
	
	PWR_CHARGING,
	PWR_CHARGE_FULL,
	PWR_UNCHARGE,
	
	SND_LINEIN_IN,
	SND_LINEIN_OUT,
	
	SND_BLUETOOTH_CONN,
	SND_BLUETOOTH_UNCN,
	
	SND_AIRPLAY_IN,
	SND_AIRPLAY_OUT,
	
	SIM_UPGRADE
};

E_ButtonMod getButtonModeParse(PT_InputEvent ptInputEvent)
{
	int i = 0;
	int size = sizeof(button_codes)/sizeof(button_codes[0]);
	for(i = 0; i < size; i++)
	{
		if(button_codes[i]==ptInputEvent->iCode)
		{
			return 1 << i;
		}
	}
	
	return 0;
}

int RegisterInputOpr(PT_InputOpr ptInputOpr)
{
	PT_InputOpr ptTmp;

	if (!g_ptInputOprHead)
	{
		g_ptInputOprHead   = ptInputOpr;
		ptInputOpr->ptNext = NULL;
	}
	else
	{
		ptTmp = g_ptInputOprHead;
		while (ptTmp->ptNext)
		{
			ptTmp = ptTmp->ptNext;
		}
		ptTmp->ptNext	  = ptInputOpr;
		ptInputOpr->ptNext = NULL;
	}

	return 0;
}

void ShowInputOpr(void)
{
	int i = 0;
	PT_InputOpr ptTmp = g_ptInputOprHead;

	while (ptTmp)
	{
		printf("%02d %s\n", i++, ptTmp->name);
		ptTmp = ptTmp->ptNext;
	}
}

static const int const push_button_codes[] = { // 按键值,而不是SND,PWR等值
	
	BTN_SET,
	
	BTN_CHANNELUP,
	BTN_CHANNELDOWN,
	BTN_AB,
	BTN_LAST,
	BTN_NEXT,
	BTN_DOWNLOAD,
	BTN_PLAY,
	
	BTN_VOLUMEUP,
	BTN_VOLUMEDOWN,
	BTN_BLUETOOTH,
	BTN_LIGHT,
	BTN_LOCK,
	BTN_RECORD,
	BTN_VOICE,
	
};

static T_InputEvent latest_button_up[LATEST_BUTTON_UP_SUM];

static void add_button_up(T_InputEvent *input) // 添加按键弹起的事件
{	
	if(NULL==input || ACTION_UP!=input->iValue)
	{
		return;
	}
	
	int push_button_sum = sizeof(push_button_codes)/sizeof(push_button_codes[0]);
	int i = 0;
	
	for(i = 0; i < push_button_sum; i++)
	{
		if(push_button_codes[i] == input->iCode)
		{
			break;
		}
	}
	if(i == push_button_sum) // input 不是按键事件
	{
		return;
	}
	
	for(i = LATEST_BUTTON_UP_SUM-2; i >= 0; i--) // 向右移动一格,留出[0]位
	{
		latest_button_up[i+1].tTime.tv_sec = latest_button_up[i].tTime.tv_sec;
		latest_button_up[i+1].tTime.tv_usec = latest_button_up[i].tTime.tv_usec;
		latest_button_up[i+1].iType = latest_button_up[i].iType;
		latest_button_up[i+1].iCode = latest_button_up[i].iCode;
		latest_button_up[i+1].iValue = latest_button_up[i].iValue;
	}
	
	latest_button_up[0].tTime.tv_sec 	= input->tTime.tv_sec;
	latest_button_up[0].tTime.tv_usec 	= input->tTime.tv_usec;
	latest_button_up[0].iType 			= input->iType;
	latest_button_up[0].iCode 			= input->iCode;
	latest_button_up[0].iValue 			= input->iValue;
}

T_InputEvent* get_latest_button_up(void)
{
	return latest_button_up;
}

bool dispatchKeyEvent(struct input_event *event)
{	
	static bool bReleased = true;
	
	T_InputEvent tInputEvent;
	tInputEvent.tTime = event->time;
	tInputEvent.iType = EV_BTN;
	tInputEvent.iCode = event->code;
	
	if(event->value && bReleased)
	{
		tInputEvent.iValue = ACTION_DOWN;
		bReleased = false;
	}
	else if(event->value && !bReleased)
	{
		tInputEvent.iValue = ACTION_HOLD;
	}
	else
	{
		tInputEvent.iValue = ACTION_UP;
		bReleased = true;
	}
	
	char *key_state[] = { "null", "down", "hold", "up" };	
	fprintf(stderr, "Key(0x%02X, %s) time:%lu.%lu \n", 
		tInputEvent.iCode, key_state[tInputEvent.iValue], 
	tInputEvent.tTime.tv_sec, tInputEvent.tTime.tv_usec);
	
	add_button_up(&tInputEvent); // 记录按键事件
	return dispatchKeyEventGroup(&tInputEvent);
}

int InputDevicesInit(void)
{
	PT_InputOpr ptTmp = g_ptInputOprHead;
	
	while (ptTmp)
	{
		if (ptTmp->DeviceInit() < 0)
		{
			return -1;
		}
		
		ptTmp = ptTmp->ptNext;
	}
	
	return 0;
}

int InputDevicesExit(void)
{
	PT_InputOpr ptTmp = g_ptInputOprHead;
	
	while (ptTmp)
	{
		if (ptTmp->DeviceExit() < 0)
		{
			return -1;
		}
		
		ptTmp = ptTmp->ptNext;
	}
	
	return 0;
}

int InputInit(void)
{
	int iError = 0;
	
	iError = ButtonInit();
	
	return iError;
}

