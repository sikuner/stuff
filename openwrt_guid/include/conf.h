
#ifndef __CONF_H__
#define __CONF_H__

#define MAX_LINE				1024
#define MAX_CHARS_NUM			48

#define WINDOW_WIDTH    		128
#define WINDOW_HEIGHT   		64

#define STATUS_X    			0
#define STATUS_Y   				0
#define STATUS_W    			128
#define STATUS_H   				16

#define MD5_STRING_LENGTH		32				/* md5�ַ������� */

#define LINE_HEIGHT             16
#define DEFAULT_FONT_SIZE       13

#define CHANNEL_FONT_SIZE		20
#define VOLUME_FONT_SIZE        28

#define LATEST_BUTTON_UP_SUM    10

#define COLOR_BACKGROUND   		0x000000  		/* ��ɫ���� */
#define COLOR_FOREGROUND   		0xFFFFFF  		/* ��ɫ���� */

#define TIMER_ALIVE_UNIT		100				/* ����������ʱ�����(��λ:����) */

#define SHIFT_STEP_PIXEL        1           	/* �ƶ�����(��λ:����) */
#define SHIFT_PERIOD_MSEC     	20         		/* �ƶ����(��λ:����) */

#define TIME_UPDATE_PERIOD		10*1000			/* ʱ����¼��(��λ:����) */

#define CHANNEL_KEY_TIMEOUT		5*1000			/* channelģʽ������ʱ(��λ:����) */
#define VOLUME_KEY_TIMEOUT		1*1000			/* volumeģʽ������ʱ(��λ:����) */

#define CHANNEL_PLAY_PERIOD		5*1000			/* channelģʽʱ,���޲���,��5s�˳�����playģʽ */
#define VOLUME_PLAY_PERIOD		1*1000			/* volumeģʽʱ,���޲���,��1s�˳� */
#define LOCK_PLAY_PERIOD		2*1000			/* lockģʽʱ,���޲���,��2s�˳� */
#define PLAY_POWEROFF_PERIOD	2*1000			/* �ػ�,��Ҫ����2�� */
#define POWEROFF_DELAY_PERIOD	6*1000			/* �ػ�,��ʱʱ��6�� */
#define VOICE_BACK_PERIOD		3*1000		
#define LONG_SET_PERIOD			2*1000			/* ����SET��2��,����SmartLink����ģʽ */
#define LONG_RESET_PERIOD		10*1000			/* ����SET��10��,����reset, �ָ��������� */
#define LONG_LOCK_PERIOD		2*1000			/* ��/�ر�ͯ��,��Ҫ����2�� */

#define LONG_INFO_PERIOD		2*1000			/* �Ȱ͹ؼ���Ϣ,��Ҫ����light�� 2�� */

#define LONG_BLUETOOTH_PERIOD	2*1000			/* ��/�ر�����ģʽ,��Ҫ����2�� */
#define LONG_DOWNLOAD_PERIOD	2*1000			// ������smartlinkģʽʱ,����DOWNLOAD�� ����apģʽ

#define AB_REPEAT_PERIOD		1*1000			/* AB����,��Ҫ�����ж��Ƿ񳬹�B�� */

#define TIMER_NETWORK_PERIOD	3*1000			/* �������״̬,��������, 3s���һ�� */
#define TIMER_RECONNECT_TIMEOUT	120*1000		/* ������ʱ 120s */

#define BATTERY_DETECT_PERIOD	60*1000			/* ��ص���������� */

#define IDLE_SCREENSAVER_PERIOD 1200*1000		/* ����������ʾ, 20min */
#define IDLE_POWEROFF_PERIOD 	300*1000		/* �����Զ��ػ�, 5min */
#define REMOTE_POWEROFF_PERIOD 	30*1000			/* Զ��ָ��ػ�, 30s */

#define UPGRADE_TIMEOUT_PERIOD 	300*1000		/* ��������ʱ, 300s */

#define RECORD_TIMELIMIT		5*60   	    	// ¼��ʱ��,��

#define BATTERY_LOW_LEVEL 		10				/* �͵�������, 10% */
#define BATTERY_LOW_COUNT 		3				/* ������⵽�ϵ͵�������, ����ʾ */
#define BATTERY_OFF_LEVEL 		5				/* �͵����ػ�, 5% */
#define BATTERY_OFF_COUNT 		3				/* ������⵽�ػ���������, �͹ػ� */

#define CHARGE_OFF_COUNT 		8				/* ��翪����,��⵽5%���� ���� */

#define DELAY_POWERON_PERIOD	300				/* �������ʱ���� */

#define LINE1_TWINKLE_PERIOD 	500         	/* Line1������˸���� */
#define LINE1_WAITING_PERIOD 	500         	/* Line1�ȴ���˸���� */

#define XFYUN_IAT_TIMEOUT		6*1000			// Ѷ������д��ʱ, 6s
#define BBYUN_OD_TIMEOUT		6*1000			// �Ȱ��Ƶ㲥��ʱ, 6s

#define TOAST_SHORT_PERIOD		2000   			// Toast��ʾ��ʱ�� 2s
#define TOAST_LONG_PERIOD		3500   			// Toast��ʾ��ʱ�� 3.5s

#define CONN_OK_PLAY_PERIOD		6*1000 			// �������ӳɹ�,��ת����״̬�ȴ�ʱ��

#define LINEIN_GUARD_PERIOD    	2*1000     		// linein״̬��,���linein״̬ ���� 2s

#define AIRPLAY_GUARD_PERIOD    2*1000  		// airplay״̬��,���airplay״̬ ���� 2s

#define DELAY_BATTERY_PERIOD    4500     		// ��ؼ��,�ӳ�����

#define DOWN_STAY_PERIOD    	3500     		// ����ͼ����ʾʱ��Ϊ3.5s

#define WATCH_DOWN_PERIOD    	3500     		// ����down��Ϣ

#define START_RECORD_PERIOD		2*1000			/* ����2s¼�� */

#define SN_LENGTH				12

#define KEY_COMBINE_DELTA		2000   			// ���빤������ģʽ, ��ϼ����

#define AB_RECORD_PERIOD		60				// AB¼�����ʱ��(��λ:s)

#define AB_RECORD_FILE 			"/tmp/ab.wav" 	// AB¼���ļ�

// ͼ��Ҫ��, ��ʽΪBMP��ʽ, ��λ���Ϊ1

#define RESOURCE_HOME			"/usr/share/guid"

// ����ͼ�� volume [0, 25]
#define ICON_VOLUME_0			RESOURCE_HOME"/images/ic_volume_0.bmp"
#define ICON_VOLUME_1			RESOURCE_HOME"/images/ic_volume_1.bmp"
#define ICON_VOLUME_2			RESOURCE_HOME"/images/ic_volume_2.bmp"
#define ICON_VOLUME_3			RESOURCE_HOME"/images/ic_volume_3.bmp"
#define ICON_VOLUME_4			RESOURCE_HOME"/images/ic_volume_4.bmp"
#define ICON_VOLUME_5			RESOURCE_HOME"/images/ic_volume_5.bmp"
#define ICON_VOLUME_6			RESOURCE_HOME"/images/ic_volume_6.bmp"
#define ICON_VOLUME_7			RESOURCE_HOME"/images/ic_volume_7.bmp"
#define ICON_VOLUME_8			RESOURCE_HOME"/images/ic_volume_8.bmp"
#define ICON_VOLUME_9			RESOURCE_HOME"/images/ic_volume_9.bmp"

#define ICON_VOLUME_10			RESOURCE_HOME"/images/ic_volume_10.bmp"
#define ICON_VOLUME_11			RESOURCE_HOME"/images/ic_volume_11.bmp"
#define ICON_VOLUME_12			RESOURCE_HOME"/images/ic_volume_12.bmp"
#define ICON_VOLUME_13			RESOURCE_HOME"/images/ic_volume_13.bmp"
#define ICON_VOLUME_14			RESOURCE_HOME"/images/ic_volume_14.bmp"
#define ICON_VOLUME_15			RESOURCE_HOME"/images/ic_volume_15.bmp"
#define ICON_VOLUME_16			RESOURCE_HOME"/images/ic_volume_16.bmp"
#define ICON_VOLUME_17			RESOURCE_HOME"/images/ic_volume_17.bmp"
#define ICON_VOLUME_18			RESOURCE_HOME"/images/ic_volume_18.bmp"
#define ICON_VOLUME_19			RESOURCE_HOME"/images/ic_volume_19.bmp"

#define ICON_VOLUME_20			RESOURCE_HOME"/images/ic_volume_20.bmp"
#define ICON_VOLUME_21			RESOURCE_HOME"/images/ic_volume_21.bmp"
#define ICON_VOLUME_22			RESOURCE_HOME"/images/ic_volume_22.bmp"
#define ICON_VOLUME_23			RESOURCE_HOME"/images/ic_volume_23.bmp"
#define ICON_VOLUME_24			RESOURCE_HOME"/images/ic_volume_24.bmp"
#define ICON_VOLUME_25			RESOURCE_HOME"/images/ic_volume_25.bmp"

// ���ͼ�� battery
#define ICON_BATTERY_0			RESOURCE_HOME"/images/ic_battery_0.bmp"
#define ICON_BATTERY_1			RESOURCE_HOME"/images/ic_battery_1.bmp"
#define ICON_BATTERY_2			RESOURCE_HOME"/images/ic_battery_2.bmp"
#define ICON_BATTERY_3			RESOURCE_HOME"/images/ic_battery_3.bmp"
#define ICON_BATTERY_4			RESOURCE_HOME"/images/ic_battery_4.bmp"
#define ICON_BATTERY_5			RESOURCE_HOME"/images/ic_battery_5.bmp"
#define ICON_BATTERY_6			RESOURCE_HOME"/images/ic_battery_6.bmp"
#define ICON_BATTERY_7			RESOURCE_HOME"/images/ic_battery_7.bmp"
#define ICON_BATTERY_8			RESOURCE_HOME"/images/ic_battery_8.bmp"
#define ICON_BATTERY_9			RESOURCE_HOME"/images/ic_battery_9.bmp"
#define ICON_BATTERY_10			RESOURCE_HOME"/images/ic_battery_10.bmp"
#define ICON_BATTERY_11			RESOURCE_HOME"/images/ic_battery_11.bmp"

#define ICON_BATTERY_C0			RESOURCE_HOME"/images/ic_battery_c0.bmp"
#define ICON_BATTERY_C1			RESOURCE_HOME"/images/ic_battery_c1.bmp"
#define ICON_BATTERY_C2			RESOURCE_HOME"/images/ic_battery_c2.bmp"
#define ICON_BATTERY_C3			RESOURCE_HOME"/images/ic_battery_c3.bmp"
#define ICON_BATTERY_C4			RESOURCE_HOME"/images/ic_battery_c4.bmp"
#define ICON_BATTERY_C5			RESOURCE_HOME"/images/ic_battery_c5.bmp"
#define ICON_BATTERY_C6			RESOURCE_HOME"/images/ic_battery_c6.bmp"
#define ICON_BATTERY_C7			RESOURCE_HOME"/images/ic_battery_c7.bmp"
#define ICON_BATTERY_C8			RESOURCE_HOME"/images/ic_battery_c8.bmp"
#define ICON_BATTERY_C9			RESOURCE_HOME"/images/ic_battery_c9.bmp"
#define ICON_BATTERY_C10		RESOURCE_HOME"/images/ic_battery_c10.bmp"
#define ICON_BATTERY_C11		RESOURCE_HOME"/images/ic_battery_c11.bmp"

#define ICON_BATTERY_TWINKLE_PERIOD 500				/* �͵���ʱ,���ͼ����˸���(��λ:����) */

#define ICON_WIFI_TWINKLE_PERIOD 	500        		/* wifiͼ����˸ */

#define ENCODING_FORMAT			"utf-8"
#define MSYH_TTF				RESOURCE_HOME"/fonts/msyh.ttf"
#define MSYHL_TTF				RESOURCE_HOME"/fonts/msyhl.ttf"

#if SDL_SUPPORT
#define DISPLAY_DEVICE			"sdl"
#else
#define DISPLAY_DEVICE			"oled"
#endif

#define AB_OUTPUT_NAME			"Recorder"

#define WLAN_INTERFACE			"wlan0"

// ������ʾ
#define ICON_UPGRADE_NORMAL		RESOURCE_HOME"/images/ic_upgrade_normal.bmp"
#define ICON_UPGRADE_FORCE		RESOURCE_HOME"/images/ic_upgrade_force.bmp"

// ��������
#define ICON_SCREENSAVER		RESOURCE_HOME"/images/ic_screensaver.bmp"
// Զ�̹ػ�����
#define ICON_REMOTE_OFF			RESOURCE_HOME"/images/ic_remote_off.bmp"

#define ICON_OFFLINE_VOICE		RESOURCE_HOME"/images/ic_offline_voice.bmp"

// ����״̬
#define ICON_WIFI_FAIL			RESOURCE_HOME"/images/ic_wifi_fail.bmp"
#define ICON_WIFI_WEAKER		RESOURCE_HOME"/images/ic_wifi_weaker.bmp"
#define ICON_WIFI_WEAK			RESOURCE_HOME"/images/ic_wifi_weak.bmp"
#define ICON_WIFI_OK			RESOURCE_HOME"/images/ic_wifi_ok.bmp"

// lineinͼ��
#define ICON_LINEIN				RESOURCE_HOME"/images/ic_linein.bmp"

// mpd_state ������״̬
#define ICON_PLAYER_NAME		"player"
#define ICON_PLAYER_PLAY		RESOURCE_HOME"/images/ic_play.bmp"
#define ICON_PLAYER_PAUSE		RESOURCE_HOME"/images/ic_pause.bmp"

// ¼����ʾ
#define ICON_RECORD_TIPS		RESOURCE_HOME"/images/ic_record_tips.bmp"

// ����ͼ��
#define ICON_DOWN_NAME			"down"
#define ICON_DOWN				RESOURCE_HOME"/images/ic_down.bmp"
#define ICON_DOWN_TIPS			RESOURCE_HOME"/images/ic_down_tips.bmp"

// ��ͼ��
#define ICON_LOCK_NAME			"lock"
#define ICON_LOCK				RESOURCE_HOME"/images/ic_lock.bmp"

#define ICON_LOCK_TIPS			RESOURCE_HOME"/images/ic_lock_tips.bmp"
#define ICON_LOCKED_TIPS		RESOURCE_HOME"/images/ic_locked_tips.bmp"

// ����ͼ��
#define ICON_ALARM_NAME			"alarm"
#define ICON_ALARM				RESOURCE_HOME"/images/ic_alarm.bmp"

// ����ͼ��
#define ICON_BLUETOOTH			RESOURCE_HOME"/images/ic_bluetooth.bmp"
#define ICON_BLUETOOTH_CN		RESOURCE_HOME"/images/ic_bluetooth_cn.bmp"
#define ICON_START_BT_TIPS		RESOURCE_HOME"/images/ic_start_bt_tips.bmp"
#define ICON_CLOSE_BT_TIPS		RESOURCE_HOME"/images/ic_close_bt_tips.bmp"

// ʱ������
#define ICON_COLON				RESOURCE_HOME"/images/ic_colon.bmp"		/* ð�� */
#define ICON_NUM_0				RESOURCE_HOME"/images/ic_num0.bmp"
#define ICON_NUM_1				RESOURCE_HOME"/images/ic_num1.bmp"
#define ICON_NUM_2				RESOURCE_HOME"/images/ic_num2.bmp"
#define ICON_NUM_3				RESOURCE_HOME"/images/ic_num3.bmp"
#define ICON_NUM_4				RESOURCE_HOME"/images/ic_num4.bmp"
#define ICON_NUM_5				RESOURCE_HOME"/images/ic_num5.bmp"
#define ICON_NUM_6				RESOURCE_HOME"/images/ic_num6.bmp"
#define ICON_NUM_7				RESOURCE_HOME"/images/ic_num7.bmp"
#define ICON_NUM_8				RESOURCE_HOME"/images/ic_num8.bmp"
#define ICON_NUM_9				RESOURCE_HOME"/images/ic_num9.bmp"

#define ICON_LOGO				RESOURCE_HOME"/images/ic_logo.bmp"

// �����ȴ�ͼ��
#define ICON_WAITING_0			RESOURCE_HOME"/images/ic_waiting_0.bmp"
#define ICON_WAITING_1			RESOURCE_HOME"/images/ic_waiting_1.bmp"
#define ICON_WAITING_2			RESOURCE_HOME"/images/ic_waiting_2.bmp"
#define ICON_WAITING_3			RESOURCE_HOME"/images/ic_waiting_3.bmp"
#define ICON_WAITING_4			RESOURCE_HOME"/images/ic_waiting_4.bmp"

#define ICON_POWEROFF			RESOURCE_HOME"/images/ic_poweroff.bmp"
#define ICON_LOW_POWER			RESOURCE_HOME"/images/ic_low_power.bmp"
#define ICON_LOW_POWER_OFF		RESOURCE_HOME"/images/ic_low_power_off.bmp"
#define ICON_CHARGE_OFF			RESOURCE_HOME"/images/ic_charge_off.bmp"

#define ICON_RECORDED			RESOURCE_HOME"/images/ic_recorded.bmp"

// channel ͼ��
#define ICON_RHYMES_CH			RESOURCE_HOME"/images/ic_rhymes_ch.bmp"
#define ICON_ENGLISH_CH			RESOURCE_HOME"/images/ic_english_ch.bmp"
#define ICON_HABITS_CH			RESOURCE_HOME"/images/ic_habits_ch.bmp"
#define ICON_STORIES_CH			RESOURCE_HOME"/images/ic_stories_ch.bmp"
#define ICON_RECORDS_CH			RESOURCE_HOME"/images/ic_records_ch.bmp"
#define ICON_DOWNLOADS_CH		RESOURCE_HOME"/images/ic_downloads_ch.bmp"

// channel ������ԴĿ¼
#define RHYMES					"internal/music/rhymes"			// ����
#define ENGLISH					"internal/music/english"		// Ӣ��
#define HABITS					"internal/music/habits"			// ϰ��
#define STORIES					"internal/music/stories"		// �汾����
#define RECORDS					"internal/music/records"		// �ҵ�¼��
#define DOWNLOADS				"internal/music/downloads"		// �ҵĻ���

#if SDL_SUPPORT
#define RHYMES_DIR				"/var/lib/mpd/music/internal/music/rhymes"
#define ENGLISH_DIR				"/var/lib/mpd/music/internal/music/english"
#define HABITS_DIR				"/var/lib/mpd/music/internal/music/habits"
#define STORIES_DIR				"/var/lib/mpd/music/internal/music/stories"
#define RECORDS_DIR				"/var/lib/mpd/music/internal/music/records"
#define DOWNLOADS_DIR			"/var/lib/mpd/music/internal/music/downloads"
#else
#define RHYMES_DIR				"/tmp/mnt/internal/music/rhymes"
#define ENGLISH_DIR				"/tmp/mnt/internal/music/english"
#define HABITS_DIR				"/tmp/mnt/internal/music/habits"
#define STORIES_DIR				"/tmp/mnt/internal/music/stories"
#define RECORDS_DIR				"/tmp/mnt/internal/music/records"
#define DOWNLOADS_DIR			"/tmp/mnt/internal/music/downloads"
#endif

#define LED_ENABLE_PATH 		"/sys/beeba-led/enable"
#define LED_BRIGHTNESS_PATH		"/sys/beeba-led/brightness"
#define LED_BRIGHTNESS_VALUE	5

#define RTC_PATH 				"/sys/beeba-pmc/rtc"
#define TIMER_PATH 				"/sys/beeba-pmc/timer"

#define LINEIN_PAM_PATH 		"/sys/beeba-detect/mute"

#define BT_NAME_FILE 			"/tmp/bt_name"

enum BT_STATE {

	BT_STATE_UNKNOWN= -1,
	
	BT_STATE_STOPPED = 0,
	
	BT_STATE_STARTED = 1,
	
	BT_STATE_CONNECTED = 2

};

enum LOCK_STATE {

	STATE_UNLOCKED = 0,
	
	STATE_LOCKED = 1,

};

enum ALARM_STATE {

	STATE_UNALARMED = 0,
	
	STATE_ALARMED = 1,

};

enum WIFI_STATE {
	
	WIFI_HIDE = -1, 
	
	WIFI_FAIL = 0, 
	
	WIFI_WEAKER = 1, 
	
	WIFI_WEAK = 2,
	
	WIFI_OK = 3
	
};

enum DOWN_STATE {
	
	DOWN_UNKNOWN = -1,
	
	DOWN_NO_DOWNLOAD = 0,
	
	DOWN_DOWNLOADING = 1
	
};

#define LINEIN_STATE_PATH 				"/sys/beeba-detect/linein"
#define POWER_CHARGE_PATH 				"/sys/beeba-detect/charge"
#define POWER_VOLTAGE_PATH      		"/sys/beeba-pmc/voltage"

#define BEEBA_VERSION_PATH				"/etc/beeba_version"

#define BEEBA_AIRPLAY_FLAG				"/tmp/airplay.flag"

////////////////////////////////////////////////////////////
/////////////////��ʾ��/////////////////////////////////////
////////////////////////////////////////////////////////////

//#define SOUND_BYEBYE					"/usr/share/sound/byebye.mp3"

#define SOUND_LOW_POWER_OFF				"/usr/share/sound/low_power_off.mp3"
#define SOUND_LOW_POWER					"/usr/share/sound/low_power.mp3"
#define SOUND_CHARGE_OFF				"/usr/share/sound/poweroff_charging.mp3" // �䲻����,�ػ�

#define SOUND_BLUETOOTH_OPEN			"/usr/share/sound/bluetooth_open.mp3"
#define SOUND_BLUETOOTH_CLOSE			"/usr/share/sound/bluetooth_close.mp3"
#define SOUND_BLUETOOTH_CONNECTED		"/usr/share/sound/bluetooth_connected.mp3"
#define SOUND_BLUETOOTH_DISCONNECTED	"/usr/share/sound/bluetooth_disconnected.mp3"

#define FIRMWARE_VERSION_PATH			"/tmp/firmware/version"
#define FIRMWARE_ENFORCE_PATH			"/tmp/firmware/en"

#endif // __CONF_H__

