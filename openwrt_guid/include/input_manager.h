
#ifndef __INPUT_MANAGER_H__
#define __INPUT_MANAGER_H__

#include <sys/time.h>
#include <pthread.h>
#include <linux/input.h>
#include <stdbool.h>

/* 输入事件类别iType */
#ifndef EV_BTN
#define EV_BTN		0x0000FF01
#endif
#ifndef EV_PWR
#define EV_PWR		0x0000FF02
#endif
#ifndef EV_SND
#define EV_SND		0x0000FF03
#endif

// 键值 iCode
typedef enum {
	
	MOD_SET				= 0x00000001,	// SET, 设置网络
	
	MOD_CHANNELUP 		= 0x00000002,	// CH上
	MOD_CHANNELDOWN		= 0x00000004,	// CH下
	MOD_AB				= 0x00000008,	// AB复读
	MOD_LAST			= 0x00000010,	// 上一曲
	MOD_NEXT			= 0x00000020,	// 下一曲
	MOD_DOWNLOAD		= 0x00000040,	// 缓存下载
	MOD_PLAY			= 0x00000080,	// 播放/暂停, 长按3秒:开关机
	
	MOD_VOLUMEUP		= 0x00000100, 	// 音量+
	MOD_VOLUMEDOWN		= 0x00000200,	// 音量-
	MOD_BLUETOOTH		= 0x00000400,	// 蓝牙
	MOD_LIGHT			= 0x00000800,	// 点灯
	MOD_LOCK			= 0x00001000,	// 童锁 Child lock
	MOD_RECORD			= 0x00002000,	// 录音
	MOD_VOICE			= 0x00004000,	// 语音搜索
	
	MOD_CHARGING 		= 0x00008000,	// 充电中
	MOD_CHARGE_FULL 	= 0x00010000,	// 充满状态
	MOD_UNCHARGE 		= 0x00020000,	// 未充电状态, 即正常状态
	
	MOD_LINEIN_IN 		= 0x00040000,	// linein 插入
	MOD_LINEIN_OUT 		= 0x00080000,	// linein 拔出
	
	MOD_BLUETOOTH_CONN	= 0x00100000,	// 蓝牙 已连接
	MOD_BLUETOOTH_UNCN	= 0x00200000,	// 蓝牙 断开连接
	
	MOD_AIRPLAY_IN		= 0x00400000,	// Airplay 已连接
	MOD_AIRPLAY_OUT		= 0x00800000,	// Airplay 断开连接
	
	MOD_UPGRADE			= 0x01000000,	// 固件更新
	
} E_ButtonMod;

// 键值 iCode
typedef enum {

	BTN_SET				= KEY_ESC,		// SET, 设置网络 	(1) // 长按进入 smartlink 模式
	
	BTN_CHANNELUP 		= KEY_PAGEUP,	// CH上				(104)
	BTN_CHANNELDOWN		= KEY_PAGEDOWN,	// CH下				(109)
	BTN_AB				= KEY_A,		// AB复读			(30)
	BTN_LAST			= KEY_LEFT,		// 上一曲			(105)
	BTN_NEXT			= KEY_RIGHT,	// 下一曲			(106)
	BTN_DOWNLOAD		= KEY_D,		// 缓存下载			(32) // 当处于smartlink模式时,长按此键 进入ap模式
	BTN_PLAY			= KEY_SPACE,	// 播放/暂停, 长按3秒:开关机	(57)
	
	BTN_VOLUMEUP		= KEY_UP, 		// 音量+			(103)
	BTN_VOLUMEDOWN		= KEY_DOWN,		// 音量-			(108)
	BTN_BLUETOOTH		= KEY_B,		// 蓝牙				(48)
	BTN_LIGHT			= KEY_L,		// 点灯				(38)
	BTN_LOCK			= KEY_C,		// 童锁 Child lock	(46)
	BTN_RECORD			= KEY_R,		// 录音				(19)
	BTN_VOICE			= KEY_V,		// 语音搜索			(47)
	
	PWR_CHARGING 		= KEY_G,		// (34) 充电中
	PWR_CHARGE_FULL 	= KEY_F,		// (33) 充满状态
	PWR_UNCHARGE 		= KEY_U,		// (22) 未充电状态, 即正常状态
	
	SND_LINEIN_IN 		= KEY_I,		// (23) linein 插入
	SND_LINEIN_OUT 		= KEY_O,		// (24) linein 拔出
	
	SND_BLUETOOTH_CONN 	= KEY_Y,		// (21) 蓝牙 已连接
	SND_BLUETOOTH_UNCN 	= KEY_N,		// (49) 蓝牙 断开连接
	
	SND_AIRPLAY_IN 		= KEY_LEFTBRACE,	// (26) linein 插入
	SND_AIRPLAY_OUT 	= KEY_RIGHTBRACE,	// (27) linein 拔出
	
	SIM_UPGRADE 		= KEY_M			// (50) 开机检测固件升级,事件触发. SIM - simulation模拟
	
} E_ButtonCode;

// 按键状态iValue, 每次按下有三种事件. ACTION_DOWN(1个),ACTION_HOLD(0个或多个),ACTION_UP(1个)
#define ACTION_DOWN			0x01	// 按下
#define ACTION_HOLD			0x02	// 压住
#define ACTION_UP			0x03	// 弹起

typedef struct InputEvent {
	struct timeval tTime;   /* 发生这个输入事件时的时间 */
	int iType;  			/* 类别 */
	int iCode;				/* 键值 */
	int iValue; 			/* 按键状态 */
}T_InputEvent, *PT_InputEvent;

typedef struct InputOpr {
	char *name;          		/* 输入模块的名字 */
	int (*DeviceInit)(void);  	/* 设备初始化函数 */
	int (*DeviceExit)(void);  	/* 设备退出函数 */
	struct InputOpr *ptNext;
}T_InputOpr, *PT_InputOpr;

E_ButtonMod getButtonModeParse(PT_InputEvent ptInputEvent);

int RegisterInputOpr(PT_InputOpr ptInputOpr);
void ShowInputOpr(void);

int InputDevicesInit(void);
int InputDevicesExit(void);

int InputInit(void);
int EventsInit(void);
int KeyboardInit(void);
int ButtonInit(void);

bool dispatchKeyEvent(struct input_event *event);

T_InputEvent* get_latest_button_up(void);

#endif // __INPUT_MANAGER_H__

