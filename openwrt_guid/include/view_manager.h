
#ifndef __VIEW_MANAGER_H__
#define __VIEW_MANAGER_H__

#include "input_manager.h"
#include "widget.h"
#include "network.h"
#include "mpdclient.h"
#include "utf8_strings.h"
#include "common.h"

// ModeID
typedef enum {
	
	MODE_UNKNOWN	= 0,	// no information available
	MODE_WIFI		= 1,	// WiFiģʽ
	MODE_BLUETOOTH	= 2,	// ����ģʽ		������ģʽ����, �κ�ʱ��ֻ��Ϊ����һ��
	MODE_LINEIN		= 3		// lineinģʽ
	
} E_ModeID;

// ViewID
typedef enum {

	VIEW_UNKNOWN	= 0,
	VIEW_PLAY		= 1,
	VIEW_CHANNEL	= 2,
	VIEW_VOLUME 	= 3, 
	VIEW_RECORD 	= 4, 
	VIEW_VOICE 		= 5, 
	VIEW_AB 		= 6, 
	VIEW_SET		= 7, 
	VIEW_DOWNLOAD	= 8, 
	VIEW_LOCK		= 9,
	VIEW_LIGHT		= 10,
	VIEW_BLUETOOTH	= 11,
	VIEW_LINEIN 	= 12,
	VIEW_POWER		= 13,
	VIEW_AIRPLAY	= 14,
	VIEW_UPGRADE	= 15,

} E_ViewID;

typedef struct ViewAction {
	
	int id;
	
	bool (*dispatchKeyEvent)(PT_InputEvent ptInputEvent); 	// ���˰����¼�
	bool (*onKeyEvent)(PT_InputEvent ptInputEvent);			// �������¼�
	
	void (*Enter)(void);									// �����ģʽ
	void (*Exit)(void);										// �˳���ģʽ
	
	struct ViewAction *ptNext;
} T_ViewAction, *PT_ViewAction;

extern struct uloop_timeout uloop_play_timer;
extern struct uloop_timeout uloop_poweroff_timer;

void wifi_config(void);

int  RegisterViewAction(PT_ViewAction ptViewAction);
T_ViewAction* View(int view_id);
void mpd_idleloop_proc(unsigned int idle);
void mpd_state_proc(unsigned int idle);
void ShowPlayView(void);
void ShowDownStay(void);			// ����ͼ����ʱ��ʾ3.5s
void startup(void);
void battery_daemon(bool on);
void idle_poweroff(void); 			// ���� �����Զ��ػ� ��ʱ��
void remote_poweroff(void); 		// Զ�̹ػ�
int  start_ap(void);
void SendUpgradeEvent(void);		// ���͹̼������¼�
int  BatteryUpgradeCond(void);		

int  ShowWifiV(int state, bool twinkle);
int  ShowBluetoothV(int state);
int  ShowLineinV(int show);
int  UpdateModeIcon(void);
void UpdateMode(void);

int  ShowLineinMode(void);
int  ShowBluetoothMode(void);
int  ShowWiFiMode(void);

int  ViewsInit(void);
int  PlayViewInit(void);
int  ChannelViewInit(void);
int  VolumeViewInit(void);
int  SetViewInit(void);
int  RecordViewInit(void);
int  VoiceViewInit(void);
int  AbViewInit(void);
int  DownloadViewInit(void);
int  LockViewInit(void);
int  LightViewInit(void);
int  BluetoothViewInit(void);
int  LineinViewInit(void);
int  PowerViewInit(void);
int  AirplayViewInit(void);
int  UpgradeViewInit(void);

void setInterceptKeyEvent(bool intercept); // ���ذ����¼�
bool dispatchKeyEventGroup(PT_InputEvent ptInputEvent);
void setInterceptExceptMode(int except_mode);

#endif // __VIEW_MANAGER_H__

