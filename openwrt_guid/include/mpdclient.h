
#ifndef __MPDCLIENT_H__
#define __MPDCLIENT_H__

#include <mpd/client.h>

int  mpdclient_enabled(char *output_name);
int  mpdclient_disable(char *output_name);
int  mpdclient_enable(char *output_name);

bool mpdclient_local_play(const char *path);
bool mpdclient_local_load(const char *path);
bool mpdclient_next(void);
bool mpdclient_prev(void);
bool mpdclient_pause(void);
bool mpdclient_play(void);
bool mpdclient_stop(void);
bool mpdclient_clear(void);
bool mpdclient_clearerror(void);
int  mpdclient_update(void);
int  mpdclient_update_wait(void);
int  mpdclient_get_state(void);
bool mpdclient_toggle(void);
const char* mpdclient_get_error_message(void);
const char* mpdclient_get_current_song_uri(void);
bool mpdclient_current_uri_online(void);

int  mpdclient_get_volume(void);
bool mpdclient_set_volume(int vol);

bool mpdclient_recorded_prepare(const char *path);
bool mpdclient_search_play(const char *s);

typedef void (*idle_event_handler)(unsigned int idle);
void  mpdclient_start_idleloop(void);
void  mpdclient_set_event_handler(idle_event_handler cb);
void* mpdclient_get_idle_event_handler(void);
void  mpdclient_set_player_state_handler(idle_event_handler cb);

int  mpdclient_update_wait_idle(void);
bool mpdclient_pause_idle(void);
int  mpdclient_disable_idle(char *output_name);
int  mpdclient_get_state_idle(void);
int  mpdclient_get_state_error_idle(int *mpd_state, char **mpd_error);
const char* mpdclient_get_state_idle_name(void);

int  mpdclient_check_directory(void);

int  mpdclient_get_artist_title_error(char *artist, char *title, char *error); 		// 歌手/歌单, 歌名, 出错信息
int  mpdclient_get_artist_title_error_idle(char *artist, char *title, char *error); // 歌手/歌单, 歌名, 出错信息


#endif // __MPDCLIENT_H__

