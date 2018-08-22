

#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <linux/input.h>
#include <libubox/uloop.h>

#include "log.h"
#include "input_manager.h"

#define EVENT_NUM				8
#define POWEROFF_DELAY_PERIOD	8*1000		/* 关机,超时时间8秒 */
#define PLAY_POWEROFF_PERIOD	2*1000		/* 关机,需要长按2秒 */

static struct input_event event;

static int amp_close(void) // echo 0 > /sys/beeba-pmc/amp
{	
	fprintf(stderr, " xxxx amp_close # echo 0 > /sys/beeba-pmc/amp xxxx \n");
	
	pid_t pid = vfork();
	if (pid < 0)
	{
		fprintf(stderr, "fork error!");
		return -1;
	}
	if (pid > 0) // > 0, 处于父进程中
	{
		return 0;
	}
	
	execlp("/bin/sh", "/bin/sh", "-c", "echo 0 > /sys/beeba-pmc/amp", NULL); 
	exit(1);
}

static int poweroff_tips()
{
	fprintf(stderr, " xXx poweroff_tips xXx \n");
	
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
	
	execlp("/bin/sh", "/bin/sh", "-c", "gui_cli -s \"\\n\\n关机\\n\"", NULL); 
	exit(1);
}

static bool poweroff()
{
	fprintf(stderr, " xXx poweroff xXx \n");
	
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

static bool poweroff_trigger()
{
	fprintf(stderr, " xXx poweroff_trigger xXx \n");
	
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
	
	execlp("/bin/sh", "/bin/sh", "-c", "echo \"o\" > /proc/sysrq-trigger", NULL); 
	exit(1);
}

static void poweroff2_delay_cb(struct uloop_timeout *timeout)
{
	poweroff_tips();
	amp_close();
	uloop_end();
//	poweroff();
	poweroff_trigger();
}
static struct uloop_timeout poweroff2_delay_timer = {
	.cb = poweroff2_delay_cb
};
static void uloop_poweroff2_cb(struct uloop_timeout *timeout)
{
	fprintf(stderr, "uloop_poweroff_cb \n");
	uloop_timeout_set(&poweroff2_delay_timer, POWEROFF_DELAY_PERIOD);
}
struct uloop_timeout uloop_poweroff2_timer = {
	.cb = uloop_poweroff2_cb
};

static bool onKeyEventPlay2(PT_InputEvent ptInputEvent)
{
	if(ACTION_DOWN == ptInputEvent->iValue 
	&& BTN_PLAY == ptInputEvent->iCode)
	{
		fprintf(stderr, "ACTION_DOWN & BTN_PLAY \n");
		uloop_timeout_set(&uloop_poweroff2_timer, PLAY_POWEROFF_PERIOD);
	}
	else
	{
		if(uloop_timeout_remaining(&uloop_poweroff2_timer) > 0) // 短按
		{
			uloop_timeout_cancel(&uloop_poweroff2_timer);
		}
	}
	
	return true;
}

static bool dispatchKeyEvent2(struct input_event *event)
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
	
	return onKeyEventPlay2(&tInputEvent);
}

static void button_cb(struct uloop_fd *fd, unsigned int events)
{
	int rc = read(fd->fd, &event, sizeof(event));
	if(rc > 0)
	{
//		printf("time:%-24.24s.%06lu, type:0x%04x, code:0x%04x, value:0x%08x \n",  
//			ctime(&event.time.tv_sec), event.time.tv_usec,	
//			event.type, 
//			event.code, 
//			event.value);
		switch (event.type) 
		{
		case EV_KEY:
			{
				dispatchKeyEvent2(&event);
			}
			break;	
		}
	}
}

static struct uloop_fd 	button_fds[EVENT_NUM];

static int ButtonDevInit(void)
{
	char          	name[64] = { 0 };
	int           	fd = 0;  	
    int           	rc;  
	int           	i;
    for (i = 0; i < EVENT_NUM; i++) 
	{
		button_fds[i].fd = -1;
		button_fds[i].cb = NULL;
        sprintf(name, "/dev/input/event%d", i);  
        if ((fd = open(name, O_RDONLY, 0)) >= 0) 
    	{
			button_fds[i].fd = fd;
			button_fds[i].cb = button_cb;
			
			rc = uloop_fd_add(&button_fds[i], ULOOP_READ);
			if(rc < 0)
			{
				fprintf(stderr, "Failed to add button_fds[%d] \n", i);
				return -1;
			}
			
			fprintf(stderr, "uloop_fd_add name: %s \n", name);
		}
	}
	
	return 0;
}

static int ButtonDevExit(void)
{
	int           	i;
    for (i = 0; i < EVENT_NUM; i++) 
	{
		if(button_fds[i].fd >= 0)
		{
			uloop_fd_delete(&button_fds[i]);
			close(button_fds[i].fd);
			
			button_fds[i].fd = -1;
			button_fds[i].cb = NULL;
		}
	}
	
	return 0;
}

int main(int argc, char *argv[])
{
	int ret = 0;
	
	ret = uloop_init();
	if(ret < 0)
	{
		DBG_PRINTF("Failed to uloop_init \n");
		return -1;
	}
	
///////////////////////////////////////////////////////
	ButtonDevInit();
	
//////////////////////////////////////////
	uloop_run();
	
	ButtonDevExit();
	uloop_done();
	
	return 0;
}

