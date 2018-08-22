
#ifndef __MPDCLIENT_H__
#define __MPDCLIENT_H__

/**
 * MPD's playback state.
 */
enum player_state {
	/** no information available */
	PLAYER_STATE_UNKNOWN = 0,

	/** not playing */
	PLAYER_STATE_STOP = 1,

	/** playing */
	PLAYER_STATE_PLAY = 2,

	/** playing, but paused */
	PLAYER_STATE_PAUSE = 3,
};

bool mpdclient_pause(void);
bool mpdclient_play(void);
bool mpdclient_stop(void);
bool mpdclient_clear(void);
int  mpdclient_get_state(void);
bool mpdclient_toggle(void);

int  mpdclient_get_volume(void);
bool mpdclient_set_volume(int vol);

#endif // __MPDCLIENT_H__

