#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <pthread.h>
#include <syslog.h>

#include "queue.h"
#include "config.h"

#define BUFFER 50

static struct DSQueue *report_queue = NULL;

static void *msg_processor(void *arg);

int msg_init(void)
{
	int ret = -1;
	static int inited = 0;
	pthread_t processor;

	if (inited == 1) {
		return 0;
	}

	inited = 1;

	report_queue = ds_queue_create(BUFFER);

	if (report_queue != NULL) {
		PDEBUG("create queue successful, and start proccessor\n");
		ret = 0;
		ret = pthread_create(&processor, NULL, msg_processor, NULL);
		if (ret < 0) {
			PERROR("create thread failed\n");
		}
	} else {
		PERROR("create queue failed\n");
	}

	return ret;
}

void msg_put(const char *string)
{
	char *msg = NULL;

	if (string == NULL) {
		return;
	}

	msg = strdup(string);
	if (msg != NULL) {
		PDEBUG("put a msg to queue [%s]\n", msg);
		ds_queue_put(report_queue, msg);
	} else {
		PERROR("put msg failed\n");
	}
}

static void *msg_processor(void *arg)
{
	struct msg_t *msg;

	PDEBUG("---- start msg_processor\n");

	while (1) {
		PDEBUG("wait a msg\n");
		msg = ds_queue_get(report_queue);
		if (msg == NULL) {
			PERROR("msg is null\n");
			continue;
		}

		PDEBUG("got a msg: [%s]\n", msg);
		parse_cmd(msg);
		free(msg);
	}

	PDEBUG("quit msg_processor\n");

	return NULL;
}
