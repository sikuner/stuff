
#include "log.h"
#include "conf.h"
#include "disp_manager.h"
#include "widget.h"
#include "srv_proc.h"
#include "input_manager.h"
#include "application.h"
#include "view_manager.h"

#include <string.h>


int do_shutdown()
{
	InputDevicesExit();
	WidgetExit();
	
	return 0;
}

int do_hello(int index)
{
//	char line1[] = "HELLO";
//	char line2[] = "Hello Beeba";
//	char line3[] = "abcdefghijklmNOPQRSTUVWXYZ";
	
//	ShowWifi(2, false);
//	ShowLock(1);
//	ShowDown(1);
//	ShowAlarm(1);
//	ShowTime(1);
	
//	ShowLine1(line1);
//	ShowLine2(line2);
//	ShowLine3(line3);
	
	return delay_poweron(index);
}

int do_notify_progress(void)
{
	ShowDownStay();
	return 0;
}

int do_airplay(char *act)
{
	if(NULL == act)
	{
		return -1;
	}
	
	T_InputEvent tInputEvent;
	
	get_uptime(&tInputEvent.tTime);
	
	tInputEvent.iType = EV_BTN;
	
	if(strstr(act, "start"))
	{
		tInputEvent.iCode = SND_AIRPLAY_IN;
	}
	else if(strstr(act, "stop"))
	{
		tInputEvent.iCode = SND_AIRPLAY_OUT;
	}
	else
	{
		return -1;
	}
	
	tInputEvent.iValue = ACTION_DOWN;
	dispatchKeyEventGroup(&tInputEvent);
	
	tInputEvent.iValue = ACTION_UP;
	dispatchKeyEventGroup(&tInputEvent);
	
	return 0;
}

int do_wifi(int state, int twinkle)
{	
	return ShowWifi(state, (bool)twinkle);
}

int do_down(int show)
{
	return ShowDown(show);
}

int do_lock(int show)
{
	return ShowLock(show);
}

int do_bluetooth(int state)
{
	return ShowBluetooth(state);
}

int do_alarm(int show)
{
	if(show > 0)
	{
		getApp()->alarm = STATE_LOCKED;
	}
	else
	{
		getApp()->alarm = STATE_UNLOCKED;
	}
	
	return ShowAlarm(show);
}

int do_time(int show)
{
	return ShowTime(show);
}

int do_neterr(char *str)
{
	if(getApp()->curView != VIEW_SET)
	{
		return -1;
	}
	
	char line[3][MAX_LINE] = { {0}, {0}, {0} };
	char *begin = NULL;	
	char *end = NULL;
	int i = 0;
	
	begin = str;
	for(i = 0; i < 3; i++)
	{
		end = strchr(begin, '\n');
		if(!end)
		{
			strcpy(line[i], begin);
			break;
		}
		
		strncpy(line[i], begin, end-begin);
		begin = end + 1;
	}
	
	ShowLine1(line[0]);
	ShowLine2(line[1]);
	ShowLine3(line[2]);
	
	return 0;
}

int do_line1(char *str)
{
	return ShowLine1(str);
}

int do_line2(char *str)
{
	return ShowLine2(str);
}

int do_line3(char *str)
{
	return ShowLine3(str);
}

/*
 * ubus call gui toast '{"str":"String", "dur":Integer, "style":Integer}'
 * str - 字符串. 均从第0行开始,第3行为最后一行. \n 换行
 * dur - 持续显示时间(单位:秒). >0,显示持续时间(), <=0,一直显示
 * style -   0,显示123行; 1,全屏显示0123行.
 * 
 * ubus call gui toast '{"str":"比巴\\n早教\\n音响", "dur":2, "style":1}'
 *
 */
int do_toast(char *str, int dur, int style)
{
	if(!str)
		return -1;
	
	char string[MAX_LINE] = { 0 };
	char array[4][MAX_LINE] = { {0} };
	char *begin = NULL;	
	char *end = NULL;
	int i = 0;
	
	strcpy(string, str);
	
	begin = string;
	for(i = 0; i < 4; i++)
	{
		end = strchr(begin, '\n');
		if(!end)
		{
			strcpy(array[i], begin);
			break;
		}
		
		strncpy(array[i], begin, end-begin);
		begin = end + 1;
	}
	
	switch(style)
	{
	case 0: // 1,2,3 行; 状态栏,不覆盖
		Toast123(array[1], array[2], array[3], dur*1000);
		break;
	case 1: // 0,1,2,3 行; 全屏显示
		Toast0123(array[0], array[1], array[2], array[3], dur*1000);
		break;
	default:
		Toast123(array[1], array[2], array[3], dur*1000);
		break;
	}
	
	return 0;
}

