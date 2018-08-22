
#ifndef __COMMON_H__
#define __COMMON_H__

#include <stdbool.h>
#include <sys/time.h>

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

int  tv_diff(struct timeval *t1, struct timeval *t2);

void trim(char *src);

int  get_sysbeeba_value(const char *path);

int  get_alarm_start(void);

int  get_alarm_stop(void);

int  get_linein_state(void);

int  get_charge_state(void);

int  get_battery_voltage(void);

int  get_battery_percent(int voltage, bool charge);

char* get_battery_icon(int percent, bool charge);

int  parse_xf_result(char *filename, char *content, int *rc, char *text, char *strerrcode); // ��ȡ Ѷ����д ���

int  get_od_result(char *filename, int *result, char *msg); // ��ȡ �Ȱ��������� ���

char* get_sn(void);

char* get_beeba_version(void);

char* get_mode_strerror(void);

int  get_airplay_state(void); // > 0, �� airplay�ѿ���

char* get_bt_name(void);

int  get_bt_state(void);

int  bt_prev(void);

int  bt_next(void);

int  bt_toggle(void);

int  bt_stop_nocb(void);

int  playsound(char *filename);

int  pam_open(void); // ����lineinģʽ���򿪹���. echo 0 > /sys/beeba-detect/mute

int  pam_close(void); // �˳�lineinģʽ���رչ���. echo 1 > /sys/beeba-detect/mute

int  amp_close(void); // echo 0 > /sys/beeba-pmc/amp

int  led_toggle(void);

int  led_state(void);

int  led_on(void);

int  led_off(void);

int  start_debug(void); // ��������ģʽ

int  start_upgrade(void); // �����̼�

char* get_volume_icon(int vol);

int  delay_poweron(int sec); // ��ʱ������� ����

int  parse_mpd_error(char *mpd_error, char *artist, char *title);

int  mpd_kill(void);

int  check_upgrade(void);

int  get_uptime(struct timeval *tv);

bool check_has_bt(void);

char* get_model_pid(void);

#endif // __COMMON_H__

