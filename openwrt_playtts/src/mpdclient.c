
#include <assert.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#include <stdio.h>
#include <stdbool.h>
#include <signal.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include <math.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <mpd/client.h>

#define RECONNECTION_PERIOD				10	// 超时重连间隔

static struct mpd_connection* mpdclient_new_connection(void)
{
	struct mpd_connection *conn = mpd_connection_new(NULL, 0, 30000);
	if(NULL == conn)
	{
		fprintf(stderr, "%s", "Out of memory \n");
		return NULL;
	}
	if (mpd_connection_get_error(conn) != MPD_ERROR_SUCCESS) 
	{
		fprintf(stderr, "%s \n", mpd_connection_get_error_message(conn));
		mpd_connection_free(conn);
		return NULL;
	}
	
//	mpd_connection_set_keepalive(conn, true);
	return conn;
}

//static void mpdclient_close_connection(struct mpd_connection *conn)
//{
//	mpd_connection_free(conn);
//}

static void printError(struct mpd_connection *conn)
{
	assert(mpd_connection_get_error(conn) != MPD_ERROR_SUCCESS);
	const char *message = mpd_connection_get_error_message(conn);
	fprintf(stderr, "mpd error(%d): %s\n", mpd_connection_get_error(conn), message);
}

static struct mpd_connection* mpdclient_get_connection() // 单例
{
	static struct mpd_connection *single_conn = NULL;
	
	static time_t last = 0;
	time_t now = time(NULL);
	if(now-last > RECONNECTION_PERIOD) // 超时刷新一次连接
	{
		if(NULL != single_conn)
		{
			mpd_connection_free(single_conn);
			single_conn = NULL;
		}
	}
	last = now;
	
	if(NULL == single_conn)
	{
		single_conn = mpdclient_new_connection();
	}
	if(NULL != single_conn)
	{
		if(MPD_ERROR_SUCCESS==mpd_connection_get_error(single_conn))
		{
			return single_conn;
		}
		else
		{
			fprintf(stderr, ">>>mpdclient_get_connection \n");
			printError(single_conn);
			mpd_connection_free(single_conn);			
			single_conn = NULL;
		}
	}
	
	struct mpd_connection *retry_conn = NULL;	
	int max_retry = 3;
	int retry = 0;
	int sleep_ms = 0;
	
Retry:	
	fprintf(stderr, " ==> retry_conn \n");
	retry_conn = mpdclient_new_connection();
	if(NULL != retry_conn)
	{
		single_conn = retry_conn;
		return single_conn;
	}
	else
	{
		if(retry >= max_retry)
			return NULL;
		sleep_ms = pow(2.0, retry++)*100;
		usleep(sleep_ms*1000);
		
		goto Retry;
	}
	
	return NULL;
}

bool mpdclient_pause(void)
{
	struct mpd_connection *conn = mpdclient_get_connection();
	if(NULL == conn) return false; 
	
	return mpd_run_pause(conn, true);
}

bool mpdclient_play(void)
{
	struct mpd_connection *conn = mpdclient_get_connection();
	if(NULL == conn) return false; 
	
	return mpd_run_play(conn);
}

bool mpdclient_stop(void)
{
	struct mpd_connection *conn = mpdclient_get_connection();
	if(NULL == conn) return false; 
	
	return mpd_run_stop(conn);
}

bool mpdclient_clear(void)
{
	struct mpd_connection *conn = mpdclient_get_connection();
	if(NULL == conn) return false; 
	
	return mpd_run_clear(conn);
}

int mpdclient_get_state(void)
{
	struct mpd_connection *conn = mpdclient_get_connection();
	if(NULL == conn) return -1; 
	
	struct mpd_status *status = mpd_run_status(conn);
	if (status == NULL) return -1;
	
	enum mpd_state state = mpd_status_get_state(status);
	mpd_status_free(status);
	
	return state;
}

bool mpdclient_toggle(void)
{
	if(MPD_STATE_PLAY == mpdclient_get_state())
	{
		mpdclient_pause();
	}
	else
	{
		mpdclient_play();
	}
	
	return true;
}

int mpdclient_get_volume(void)
{
	struct mpd_connection *conn = mpdclient_get_connection();
	if(NULL == conn) return -1; 
	
	struct mpd_status *status;
	status = mpd_run_status(conn);
	if (NULL == status) return -1;
	
	int vol = mpd_status_get_volume(status);
	mpd_status_free(status);
	
	return vol;
}

bool mpdclient_set_volume(int vol)
{
	struct mpd_connection *conn = mpdclient_get_connection();
	if(NULL == conn) return false; 
	
	return mpd_run_set_volume(conn, vol);
}

const char* mpdclient_get_error_message(void)
{
	struct mpd_connection *conn = mpdclient_get_connection();
	if(NULL == conn) return NULL; 
	
	return mpd_connection_get_error_message(conn);
}

