
#ifndef __APPLICATION_H__
#define __APPLICATION_H__

#include <stdbool.h>

typedef struct tApplication {
	
	bool has_bt;
	
	int elec_percent; // [0, 100]
	
	int wifi_config;		// 网络配置状态 WIFI_SMARTLINK_MODE, AP_MODE, CONFIGURED
	int wifi_linked_state; 	// [0, 3], 0-FAIL, 1-WEAKER, 2-WEAK, 3-OK
	bool wifi_conn_ok; 		// false-没联网, true-已连接网络
	
	int locked;
	int alarm;
	int bluetooth; 			// Bluetooth模式
	int linein;				// Linein模式	
	int airplay;			// AirPlay模式
	int charge;
	int voltage;
	
	int download;			// 下载中 -1, 0, 1 -- 下载中
	
	int volume; 			// [0, 100]
	int channel; 			// [0, 4]
	
	int curView;			// 
	int curMode;			// Linein模式, Bluetooth模式, WiFi模式
	
	int mpd_state;
	char *mpd_error;
	
	int low_count;			// 较低电压(电压小于10%)较低电压计数
	int off_count; 			// 关机电压(电压小于5%) 自动关机计数
	int charge_off_count; 	// 充电开机后,检测到5%电量 计数
	int off_remote; 		// 远程关机, 标志
	
	int screensaver;		// 处于屏保状态
	
	int upgrade;			// 固件更新值. 1 强制更新, 0 普通更新, -1 无更新[出错或不满足条件]
	
	
} App;

App* getApp();
int AppInit(void);

#endif // __APPLICATION_H__

