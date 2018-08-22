
#ifndef __APPLICATION_H__
#define __APPLICATION_H__

#include <stdbool.h>

typedef struct tApplication {
	
	bool has_bt;
	
	int elec_percent; // [0, 100]
	
	int wifi_config;		// ��������״̬ WIFI_SMARTLINK_MODE, AP_MODE, CONFIGURED
	int wifi_linked_state; 	// [0, 3], 0-FAIL, 1-WEAKER, 2-WEAK, 3-OK
	bool wifi_conn_ok; 		// false-û����, true-����������
	
	int locked;
	int alarm;
	int bluetooth; 			// Bluetoothģʽ
	int linein;				// Lineinģʽ	
	int airplay;			// AirPlayģʽ
	int charge;
	int voltage;
	
	int download;			// ������ -1, 0, 1 -- ������
	
	int volume; 			// [0, 100]
	int channel; 			// [0, 4]
	
	int curView;			// 
	int curMode;			// Lineinģʽ, Bluetoothģʽ, WiFiģʽ
	
	int mpd_state;
	char *mpd_error;
	
	int low_count;			// �ϵ͵�ѹ(��ѹС��10%)�ϵ͵�ѹ����
	int off_count; 			// �ػ���ѹ(��ѹС��5%) �Զ��ػ�����
	int charge_off_count; 	// ��翪����,��⵽5%���� ����
	int off_remote; 		// Զ�̹ػ�, ��־
	
	int screensaver;		// ��������״̬
	
	int upgrade;			// �̼�����ֵ. 1 ǿ�Ƹ���, 0 ��ͨ����, -1 �޸���[�������������]
	
	
} App;

App* getApp();
int AppInit(void);

#endif // __APPLICATION_H__

