
#include "input_manager.h"

#include <unistd.h>
#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/input.h>
#include <libubox/uloop.h>

#define EVENT_NUM		8

static int ButtonDevInit(void);
static int ButtonDevExit(void);

static T_InputOpr g_tButtonOpr = {
	.name          = "button",
	.DeviceInit    = ButtonDevInit,
	.DeviceExit    = ButtonDevExit,
};

static struct input_event event;

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
				dispatchKeyEvent(&event);
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

int ButtonInit(void)
{
	return RegisterInputOpr(&g_tButtonOpr);
}

