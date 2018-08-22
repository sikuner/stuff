/* mpd.c
 */

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <syslog.h>

#include <mpd/client.h>

#include "config.h"

static struct mpd_connection *mpd_connection = NULL;

static void get_current_player_state_string(struct mpd_status *status, char *s)
{
	enum mpd_state i = mpd_status_get_state(status);

	if (i == MPD_STATE_PLAY) {
		strcpy(s, "Playing");
	} else if (i == MPD_STATE_PAUSE) {
		strcpy(s, "Paused");
	} else {
		strcpy(s, "Stop");
	}
}

static void get_current_playing_song(char **title, char **url)
{
	struct mpd_song* song = NULL;
	char *s = NULL;

	song = mpd_run_current_song(mpd_connection);
	if (song == NULL) {
		PDEBUG("song is null\n");
		*title = strdup("");
		*url = strdup("");
		goto out;
	}

	s = mpd_song_get_uri(song);
	if (s == NULL)
		s = "";
	*url = strdup(s);
	s = mpd_song_get_tag(song, MPD_TAG_NAME, 0);
	if (s == NULL)
		s = "";
	*title = strdup(s);

	mpd_song_free(song);
out:
	mpd_response_finish(mpd_connection);
}

int output_mpd_play()
{
	PDEBUG("%s", __func__);

	mpd_run_play(mpd_connection);

	return 0;
}

int output_mpd_stop(void)
{
	PDEBUG("%s", __func__);
	return mpd_run_stop(mpd_connection);
}

int output_mpd_pause(void)
{
	PDEBUG("%s", __func__);
	return mpd_run_pause(mpd_connection, true);
}

int output_mpd_get_volume()
{
	struct mpd_status *status;
	int vol = 0;

	if ((status = mpd_run_status(mpd_connection)) == NULL) {
		PERROR("get status failed: %s\n", mpd_connection_get_error_message(mpd_connection));
		return 0;
	}

	vol = mpd_status_get_volume(status);

	mpd_response_finish(mpd_connection);
	mpd_status_free(status);

	return vol;
}

void output_mpd_set_volume(int value)
{
	PDEBUG("mpd", "Set volume fraction to %d", value);
	mpd_run_set_volume(mpd_connection, value);
}

static void report_mpd_error(const char *event, struct mpd_connection *conn)
{
	enum mpd_error error;
	error = mpd_connection_get_error(conn);

	if (error == MPD_ERROR_SYSTEM) {
		int e = mpd_connection_get_system_error(conn);
        	syslog(LOG_SYSLOG, "BEEBALOGwarning?e=%s&p=code|%d|sys_err|%d", event, error, e);
	} else if (error == MPD_ERROR_SERVER) {
		int e = mpd_connection_get_server_error(conn);
        	syslog(LOG_SYSLOG, "BEEBALOGwarning?e=%s&p=code|%d|serv_err|%d", event, error, e);
	} else {
        	syslog(LOG_SYSLOG, "BEEBALOGwarning?e=%s&p=code|%d", event, error);
	}
}

#define MAX_RETRY (5)
static int mpd_connect(void)
{
	enum mpd_error error;
	int retry = MAX_RETRY;

try:
	if (mpd_connection != NULL) {
		mpd_connection_free(mpd_connection);
		mpd_connection = NULL;
	}

	mpd_connection = mpd_connection_new(NULL, 0, 0);
	if (mpd_connection == NULL) {
		PERROR("Out of memory\n");
		exit(0);
		return -1;
	}

	error = mpd_connection_get_error(mpd_connection);

	if (error != MPD_ERROR_SUCCESS) {
		PERROR("mpd connection failed: %s\n", mpd_connection_get_error_message(mpd_connection));
		if (retry-- > 0) {
			PERROR("retry ...\n");
			sleep(1);
			goto try;
		}

		report_mpd_error("playerd_mpd_connect_fail", mpd_connection);
		exit(EXIT_FAILURE);
		return -1;
	}

	mpd_connection_set_keepalive(mpd_connection, true);

	return 0;
}

void update_status_to_server()
{
	struct mpd_status *status;
	char cmd[20480];
	char state[24] = {'\0'};
	char *title = NULL;
	char *url = NULL;
	int vol = 0;

	if (mpd_connection == NULL) {
		mpd_connect();
		return;
	}

	status = mpd_run_status(mpd_connection);
	if (status == NULL) {
		const char *message = mpd_connection_get_error_message(mpd_connection);
		PERROR("get status failed: %s\n", message);
		mpd_connect();
		return;
	}

	vol = mpd_status_get_volume(status);
	get_current_player_state_string(status, state);
	get_current_playing_song(&title, &url);

	snprintf(cmd, sizeof(cmd), "{\"device_id\":\"%s\",\"state\":\"%s\",\"title\":\"%s\",\"url\":\"%s\",\"volume\":\"%d\"}", get_device_id(), state, title, url, vol);
	PDEBUG("report msg to server: %s\n", cmd);

	if (title != NULL)
		free(title);
	if (url != NULL)
		free(url);

	mpd_response_finish(mpd_connection);
	mpd_status_free(status);

	report_to_server(cmd);
}

static time_t g_time = (time_t)0L;

time_t get_update_status_time()
{
	return g_time;
}

void set_update_status_time(int t)
{
	g_time = time(NULL) + t;
	PDEBUG("========= t=%d, gt: %ld\n", t, g_time);
}

void *thread_update(void *arg)
{
	time_t t;
	struct mpd_status* status = NULL;

	while (1) {
		t = time(NULL);
//		PDEBUG("t=%ld, gt=%ld\n", t, g_time);
		if (t < get_update_status_time()) {
//			PDEBUG("update mpd status\n");

			update_status_to_server();
		}

		sleep(1);
	}

	return NULL;
}

int output_mpd_init(void)
{
	pthread_t thread;

	PDEBUG("%s", __func__);

	mpd_connect();

	pthread_create(&thread, NULL, thread_update, NULL);

	return 0;
}

