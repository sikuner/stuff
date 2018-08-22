
#include <assert.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#include <stdio.h>
#include <signal.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include <math.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "types.h"
#include "conf.h"
#include "utf8_strings.h"
#include "mpdclient.h"
#include "uri_parse.h"

#define RECONNECTION_PERIOD		10	// 超时重连间隔
static char *channels[] = { "rhymes", "english", "habits",  "stories", "records", "downloads" };
static char *channels_cn[] = { STRING_RHYMES, STRING_ENGLISH, STRING_HABITS,
	STRING_STORIES, STRING_RECORDS, STRING_DOWNLOADS };

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
	
	mpd_connection_set_keepalive(conn, true);
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

// -1 error, 0 disabled, 1 enabled
int  mpdclient_enabled(char *output_name)
{
	if(!output_name)
		return -1;
	
	struct mpd_connection *conn = NULL;
	struct mpd_output *output = NULL;
	const char *name = NULL;
	bool enabled = false;
	int ret = -1;
	
	conn = mpdclient_get_connection();
	if(!conn)
		return -1;
	
	mpd_send_outputs(conn);
	
	while ((output = mpd_recv_output(conn)) != NULL) 
	{
		name = mpd_output_get_name(output);
		if(!strcmp(name, output_name))
		{
			enabled = mpd_output_get_enabled(output);
			ret = enabled ? 1 : 0;
			break;
		}
		
		mpd_output_free(output);
	}
	
	mpd_response_finish(conn);
	
	return ret;
}

int  mpdclient_disable(char *output_name)
{
	if(!output_name)
		return -1;
	
	struct mpd_connection *conn = NULL;
	struct mpd_output *output = NULL;
	int id = -1;
	const char *name = NULL;
	
	conn = mpdclient_get_connection();
	if(!conn)
		return -1;
	
	mpd_send_outputs(conn);
	
	while ((output = mpd_recv_output(conn)) != NULL) 
	{
		name = mpd_output_get_name(output);
		if(!strcmp(name, output_name))
		{
			id = mpd_output_get_id(output);
			break;
		}
		
		mpd_output_free(output);
	}
	
	mpd_response_finish(conn);
	
	if(id < 0) 		// 没有找到
		return -1;
	
	if (!mpd_command_list_begin(conn, false)) 
		return -1;
	
	mpd_send_disable_output(conn, id);
	
	if (!mpd_command_list_end(conn) || !mpd_response_finish(conn)) 
		return -1;
	
	return 0;
}

int  mpdclient_enable(char *output_name)
{
	if(!output_name)
		return -1;
	
	struct mpd_connection *conn = NULL;
	struct mpd_output *output = NULL;
	int id = -1;
	const char *name = NULL;
	
	conn = mpdclient_get_connection();
	if(!conn)
		return -1;
	
	mpd_send_outputs(conn);
	
	while ((output = mpd_recv_output(conn)) != NULL) 
	{
		name = mpd_output_get_name(output);
		if(!strcmp(name, output_name))
		{
			id = mpd_output_get_id(output);
			break;
		}
		
		mpd_output_free(output);
	}
	
	mpd_response_finish(conn);
	
	if(id < 0) 		// 没有找到
		return -1;
	
	if (!mpd_command_list_begin(conn, false)) 
		return -1;
	
	mpd_send_enable_output(conn, id);
	
	if (!mpd_command_list_end(conn) || !mpd_response_finish(conn)) 
		return -1;
	
	return 0;
}

bool mpdclient_local_play(const char *path) 
{
	struct mpd_connection *conn = mpdclient_get_connection();
	if(NULL == conn) return false; 
	
	mpd_run_clear(conn);
	mpd_run_update(conn, path);
	mpd_run_idle_mask(conn, MPD_IDLE_UPDATE);
	mpd_run_update(conn, path);
	mpd_run_idle_mask(conn, MPD_IDLE_UPDATE);
	
	if (!mpd_command_list_begin(conn, false))
	{
		return false;
	}
	
	mpd_send_clear(conn);
	mpd_send_update(conn, path);
	mpd_send_add(conn, path);
	mpd_send_repeat(conn, true);
	mpd_send_random(conn, false);	
	mpd_send_single(conn, false);
	mpd_send_consume(conn, false);
	mpd_send_play(conn);
	
	if (!mpd_command_list_end(conn))
	{
		printError(conn); 
		return false;
	}
	
	return mpd_response_finish(conn);
}

bool mpdclient_local_load(const char *path)
{
	struct mpd_connection *conn = mpdclient_get_connection();
	if(NULL == conn) return false; 
	
	if (!mpd_command_list_begin(conn, false))
	{
		return false;
	}
	
	mpd_send_clear(conn);
	mpd_send_update(conn, path);
	mpd_send_add(conn, path);
	mpd_send_repeat(conn, true);
	mpd_send_random(conn, false);	
	mpd_send_single(conn, false);
	mpd_send_consume(conn, false);
	
	if (!mpd_command_list_end(conn))
	{
		printError(conn); 
		return false;
	}
	
	return mpd_response_finish(conn);
}

bool mpdclient_recorded_prepare(const char *path) // 播放结束后准备, clear,update,add,play_mode
{
	struct mpd_connection *conn = mpdclient_get_connection();
	if(NULL == conn) return false; 
	
	mpd_run_clear(conn);
	mpd_run_update(conn, path);
	mpd_run_idle_mask(conn, MPD_IDLE_UPDATE);
	mpd_run_update(conn, path);
	mpd_run_idle_mask(conn, MPD_IDLE_UPDATE);
	
	if (!mpd_command_list_begin(conn, false))
		return false;
	
	mpd_send_add(conn, path);
	mpd_send_repeat(conn, false);
	mpd_send_random(conn, false);
	mpd_send_single(conn, true);
	mpd_send_consume(conn, false);
	
	if (!mpd_command_list_end(conn))
		return false;
	
	return mpd_response_finish(conn);
}

bool mpdclient_search_play(const char *s) // 参考 cmd_searchplay
{
	struct mpd_connection *conn = mpdclient_get_connection();
	if(NULL == conn) return false; 
	
	int song_id = -1;
	
	mpd_search_queue_songs(conn, false);
	const char *pattern = s;
	mpd_search_add_any_tag_constraint(conn, MPD_OPERATOR_DEFAULT, pattern);
	mpd_search_commit(conn);
	
	struct mpd_song *song = mpd_recv_song(conn);
	if (song != NULL) 
	{
		song_id = mpd_song_get_id(song);
		mpd_song_free(song);
	}
	if (!mpd_response_finish(conn))
		return false;
	
	fprintf(stderr, "mpdclient_search_play song_id: %d \n", song_id);
	if(song_id < 0)
		return false;
	
	if (!mpd_run_play_id(conn, song_id))
		return false;
	
	return true;
}

bool mpdclient_next(void)
{
	struct mpd_connection *conn = mpdclient_get_connection();
	if(NULL == conn) return false; 
	
	return mpd_run_next(conn);
}

bool mpdclient_prev(void)
{
	struct mpd_connection *conn = mpdclient_get_connection();
	if(NULL == conn) return false; 
	
	return mpd_run_previous(conn);
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

bool mpdclient_clearerror(void)
{
	struct mpd_connection *conn = mpdclient_get_connection();
	if(NULL == conn) return false; 
	
	return mpd_run_clearerror(conn);
}

int mpdclient_update(void)
{
	struct mpd_connection *conn = mpdclient_get_connection();
	if(NULL == conn) return false; 
	
	return mpd_run_update(conn, "/");
}

int mpdclient_update_wait(void)
{
	struct mpd_connection *conn = mpdclient_get_connection();
	if(NULL == conn) return -1; 
	
	int id = mpd_run_update(conn, "/");
	mpd_run_idle_mask(conn, MPD_IDLE_UPDATE);
	
	return id;
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

const char* mpdclient_get_current_song_uri(void)
{
	struct mpd_connection *conn = mpdclient_get_connection();
	if(NULL == conn) return NULL; 
	
	static char *song_uri = NULL;
	if(song_uri)
	{
		free(song_uri);
		song_uri = NULL;
	}
	
	struct mpd_song *song;
	song = mpd_run_current_song(conn);
	if(song != NULL)
	{
		song_uri = strdup(mpd_song_get_uri(song));
		mpd_song_free(song);
	}
	
	return song_uri;
}

bool mpdclient_current_uri_online(void)
{
	const char *song_uri = mpdclient_get_current_song_uri();
	if(song_uri && strstr(song_uri, "://")) // 在线播放
	{
		return true;
	}
	
	return false;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////
static pthread_t idleloop_threadId = (pthread_t)NULL;
static idle_event_handler idle_event_cb = NULL;
static idle_event_handler player_event_cb = NULL;
static bool first_conn_idle = true;

static struct mpd_connection* idleloop_get_connection()
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
	fprintf(stderr, " ==> retry_conn idleloop \n");
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


static void* idleloop(void *args)
{
	fprintf(stderr, " ==> idleloop \n");
	
	struct mpd_connection *conn = NULL;
	
	while(true)
	{
		conn = idleloop_get_connection();
		if(NULL == conn) 
			continue;
		
		if(first_conn_idle) 
		{
			first_conn_idle = false;
			
			mpdclient_disable_idle(AB_OUTPUT_NAME);
			mpdclient_update_wait_idle();
//			mpdclient_pause_idle();
			
			if(0 == access(AB_RECORD_FILE, F_OK))
				remove(AB_RECORD_FILE);
		}
		
		enum mpd_idle idle = mpd_run_idle(conn);
		if (idle==0 && mpd_connection_get_error(conn)!=MPD_ERROR_SUCCESS)
			continue;
		
		fprintf(stderr, " xxx> idleloop mpd_state: %s \n", mpdclient_get_state_idle_name());
		
		if(player_event_cb)
		{
			player_event_cb(idle);
		}
		
		if(idle_event_cb)
		{
			idle_event_cb(idle);
		}
	}
	
	if(NULL != conn)
	{
		mpd_connection_free(conn);
		conn = NULL;
	}
	
	return NULL;
}

void mpdclient_start_idleloop(void)
{
	pthread_create(&idleloop_threadId, NULL, idleloop, NULL);
}

void mpdclient_set_event_handler(idle_event_handler cb)
{
	idle_event_cb = cb;
}
void* mpdclient_get_idle_event_handler(void)
{
	return (void*)idle_event_cb;
}
void mpdclient_set_player_state_handler(idle_event_handler cb)
{
	player_event_cb = cb;
}

int mpdclient_update_wait_idle(void)
{
	struct mpd_connection *conn = idleloop_get_connection();
	if(NULL == conn) return -1; 
	
	int id = mpd_run_update(conn, "/");
	mpd_run_idle_mask(conn, MPD_IDLE_UPDATE);
	
	return id;
}

bool mpdclient_pause_idle(void)
{
	struct mpd_connection *conn = idleloop_get_connection();
	if(NULL == conn) return false; 
	
	return mpd_run_pause(conn, true);
}

int  mpdclient_disable_idle(char *output_name)
{
	if(!output_name)
		return -1;
	
	struct mpd_connection *conn = NULL;
	struct mpd_output *output = NULL;
	int id = -1;
	const char *name = NULL;
	
	conn = idleloop_get_connection();
	if(!conn)
		return -1;
	
	mpd_send_outputs(conn);
	
	while ((output = mpd_recv_output(conn)) != NULL) 
	{
		name = mpd_output_get_name(output);
		if(!strcmp(name, output_name))
		{
			id = mpd_output_get_id(output);
			break;
		}
		
		mpd_output_free(output);
	}
	
	mpd_response_finish(conn);
	
	if(id < 0)		// 没有找到
		return -1;
	
	if (!mpd_command_list_begin(conn, false)) 
		return -1;
	
	mpd_send_disable_output(conn, id);
	
	if (!mpd_command_list_end(conn) || !mpd_response_finish(conn)) 
		return -1;
	
	return 0;
}

int mpdclient_get_state_idle(void)
{
	struct mpd_connection *conn = idleloop_get_connection();
	if(NULL == conn) return -1; 
	
	struct mpd_status *status = mpd_run_status(conn);
	if (status == NULL) return -1;
	
	enum mpd_state state = mpd_status_get_state(status);
	
//	const char *error = mpd_status_get_error(status);
	
//	fprintf(stderr, " mpd_status state: %d, error: %s \n", 
//		state,
//		error);
	
	mpd_status_free(status);
	
	return state;
}

int mpdclient_get_state_error_idle(int *mpd_state, char **mpd_error)
{
	static char* mpd_status_error = NULL;
	
	struct mpd_connection *conn = NULL;
	struct mpd_status *status = NULL;
	
	enum mpd_state state = MPD_STATE_UNKNOWN;
	const char *error = NULL;
	
	conn = idleloop_get_connection();
	if(NULL == conn) return -1; 
	
	status = mpd_run_status(conn);
	if (status == NULL) return -1;
	
	state = mpd_status_get_state(status);
	error = mpd_status_get_error(status);
	
	if(mpd_state)
	{
		*mpd_state = state;
	}
	if(mpd_error)
	{
		*mpd_error = NULL;
		
		if(error)
		{
			if(mpd_status_error)
			{
				free(mpd_status_error);
				mpd_status_error = NULL;
			}
			
			mpd_status_error = strdup(error);
			*mpd_error = mpd_status_error;
		}
	}
	
	mpd_status_free(status);
	
	return 0;
}

static const char *const mpd_state_names[] = {
	"unknown",
	"stop",
	"play",
	"pause",
	NULL
};
const char* mpdclient_get_state_idle_name(void)
{
	int state = mpdclient_get_state_idle();
	if(state < 0)
		return NULL;
	
	return mpd_state_names[state];
}

/**
 * create_path
 *
 *   This function creates a file path, like mkdir -p. 
 *
 * Parameters:
 *
 *   path - the path to create
 *
 * Returns: 0 on success, -1 on failure
 * On failure, a message has been printed to stderr.
 */
static int create_path(const char *path)
{
	char *start;
	mode_t mode = S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;

	if (path[0] == '/')
		start = strchr(path + 1, '/');
	else
		start = strchr(path, '/');
	
	while (start) 
	{
		char *buffer = strdup(path);
		buffer[start-path] = 0x00;
		
		if (mkdir(buffer, mode)==-1 && errno!=EEXIST) 
		{
			fprintf(stderr, "Problem creating directory %s", buffer);
			perror(" ");
			free(buffer);
			return -1;
		}
		free(buffer);
		start = strchr(start + 1, '/');
	}
	
	return 0;
}

int mpdclient_check_directory(void)
{
	char *mpd_dirs[] = { RHYMES_DIR"/beeba", ENGLISH_DIR"/beeba", HABITS_DIR"/beeba", 
		STORIES_DIR"/beeba", RECORDS_DIR"/user/beeba", DOWNLOADS_DIR"/cache/beeba", NULL };
	
	int i = 0;
	
	for(i = 0; mpd_dirs[i] != NULL; i++)
	{
		if(0 != access(mpd_dirs[i], F_OK))
		{
			create_path(mpd_dirs[i]);
		}
	}
	
	return 0;
}

///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////

static int  mpd_get_artist_title_error(struct mpd_connection *connection, char *artist, char *title, char *error)
{
	if (!connection || !artist || !title || !error 
	|| MPD_ERROR_SUCCESS!=mpd_connection_get_error(connection)) 
		return -1;
	
	int ret = 0;
	const char *uri;
	struct mpd_song *song;
	struct mpd_status *status = NULL;
	
	const char *value = NULL;
	int size = 0;
	int i = 0;
	
	char uri2[MAX_URL] = { 0 };
	char *bname = NULL;
	char *name_begin = NULL;
	char *name_end = NULL;
	bool has_title = false;
	
	status = mpd_run_status(connection);
	if (!status)
		return -1;
	value = mpd_status_get_error(status);
	if(value)
		strcpy(error, value);
	mpd_status_free(status);
	
	song = mpd_run_current_song(connection);
	if (!song) 
		return -1;
	
	uri = mpd_song_get_uri(song);
	if(!uri)
		return -1;
	
	if(strstr(uri, "://")) // 网络资源
	{
		ret = parse_artist_title(uri, artist, title);
	}
	else
	{
		size = sizeof(channels)/sizeof(channels[0]);
		for(i = 0; i < size; i++)
		{
			if(strstr(uri, channels[i]))
			{
				strcpy(artist, channels_cn[i]);
				break;
			}
		}
		
		strcpy(uri2, uri);
		bname = basename(uri2);
		
		if(strlen(bname) > MD5_STRING_LENGTH + 1
		&& *(bname + MD5_STRING_LENGTH) == '_')
		{
			name_begin = bname + MD5_STRING_LENGTH + 1;
			name_end = strchr(name_begin, '.');
			if(!name_end)
				name_end = name_begin + strlen(name_begin);
			
			strncpy(title, name_begin, name_end-name_begin);
		}
		else
		{
			i = 0;
			while ((value = mpd_song_get_tag(song, MPD_TAG_TITLE, i++)) != NULL)
			{
				strcpy(title, value);
				has_title = true;
				break;
			}
			if(!has_title)
			{
				strcpy(title, bname);
			}
		}
	}
	mpd_song_free(song);
	
	return ret;
}

int  mpdclient_get_artist_title_error(char *artist, char *title, char *error)
{
	struct mpd_connection *conn = mpdclient_get_connection();
	if(!conn)
		return -1; 
	
	return mpd_get_artist_title_error(conn, artist, title, error);
}

int  mpdclient_get_artist_title_error_idle(char *artist, char *title, char *error)
{
	struct mpd_connection *conn = idleloop_get_connection();
	if(!conn)
		return -1;
	
	return mpd_get_artist_title_error(conn, artist, title, error);
}

