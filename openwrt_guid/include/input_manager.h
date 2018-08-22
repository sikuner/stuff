
#ifndef __INPUT_MANAGER_H__
#define __INPUT_MANAGER_H__

#include <sys/time.h>
#include <pthread.h>
#include <linux/input.h>
#include <stdbool.h>

/* �����¼����iType */
#ifndef EV_BTN
#define EV_BTN		0x0000FF01
#endif
#ifndef EV_PWR
#define EV_PWR		0x0000FF02
#endif
#ifndef EV_SND
#define EV_SND		0x0000FF03
#endif

// ��ֵ iCode
typedef enum {
	
	MOD_SET				= 0x00000001,	// SET, ��������
	
	MOD_CHANNELUP 		= 0x00000002,	// CH��
	MOD_CHANNELDOWN		= 0x00000004,	// CH��
	MOD_AB				= 0x00000008,	// AB����
	MOD_LAST			= 0x00000010,	// ��һ��
	MOD_NEXT			= 0x00000020,	// ��һ��
	MOD_DOWNLOAD		= 0x00000040,	// ��������
	MOD_PLAY			= 0x00000080,	// ����/��ͣ, ����3��:���ػ�
	
	MOD_VOLUMEUP		= 0x00000100, 	// ����+
	MOD_VOLUMEDOWN		= 0x00000200,	// ����-
	MOD_BLUETOOTH		= 0x00000400,	// ����
	MOD_LIGHT			= 0x00000800,	// ���
	MOD_LOCK			= 0x00001000,	// ͯ�� Child lock
	MOD_RECORD			= 0x00002000,	// ¼��
	MOD_VOICE			= 0x00004000,	// ��������
	
	MOD_CHARGING 		= 0x00008000,	// �����
	MOD_CHARGE_FULL 	= 0x00010000,	// ����״̬
	MOD_UNCHARGE 		= 0x00020000,	// δ���״̬, ������״̬
	
	MOD_LINEIN_IN 		= 0x00040000,	// linein ����
	MOD_LINEIN_OUT 		= 0x00080000,	// linein �γ�
	
	MOD_BLUETOOTH_CONN	= 0x00100000,	// ���� ������
	MOD_BLUETOOTH_UNCN	= 0x00200000,	// ���� �Ͽ�����
	
	MOD_AIRPLAY_IN		= 0x00400000,	// Airplay ������
	MOD_AIRPLAY_OUT		= 0x00800000,	// Airplay �Ͽ�����
	
	MOD_UPGRADE			= 0x01000000,	// �̼�����
	
} E_ButtonMod;

// ��ֵ iCode
typedef enum {

	BTN_SET				= KEY_ESC,		// SET, �������� 	(1) // �������� smartlink ģʽ
	
	BTN_CHANNELUP 		= KEY_PAGEUP,	// CH��				(104)
	BTN_CHANNELDOWN		= KEY_PAGEDOWN,	// CH��				(109)
	BTN_AB				= KEY_A,		// AB����			(30)
	BTN_LAST			= KEY_LEFT,		// ��һ��			(105)
	BTN_NEXT			= KEY_RIGHT,	// ��һ��			(106)
	BTN_DOWNLOAD		= KEY_D,		// ��������			(32) // ������smartlinkģʽʱ,�����˼� ����apģʽ
	BTN_PLAY			= KEY_SPACE,	// ����/��ͣ, ����3��:���ػ�	(57)
	
	BTN_VOLUMEUP		= KEY_UP, 		// ����+			(103)
	BTN_VOLUMEDOWN		= KEY_DOWN,		// ����-			(108)
	BTN_BLUETOOTH		= KEY_B,		// ����				(48)
	BTN_LIGHT			= KEY_L,		// ���				(38)
	BTN_LOCK			= KEY_C,		// ͯ�� Child lock	(46)
	BTN_RECORD			= KEY_R,		// ¼��				(19)
	BTN_VOICE			= KEY_V,		// ��������			(47)
	
	PWR_CHARGING 		= KEY_G,		// (34) �����
	PWR_CHARGE_FULL 	= KEY_F,		// (33) ����״̬
	PWR_UNCHARGE 		= KEY_U,		// (22) δ���״̬, ������״̬
	
	SND_LINEIN_IN 		= KEY_I,		// (23) linein ����
	SND_LINEIN_OUT 		= KEY_O,		// (24) linein �γ�
	
	SND_BLUETOOTH_CONN 	= KEY_Y,		// (21) ���� ������
	SND_BLUETOOTH_UNCN 	= KEY_N,		// (49) ���� �Ͽ�����
	
	SND_AIRPLAY_IN 		= KEY_LEFTBRACE,	// (26) linein ����
	SND_AIRPLAY_OUT 	= KEY_RIGHTBRACE,	// (27) linein �γ�
	
	SIM_UPGRADE 		= KEY_M			// (50) �������̼�����,�¼�����. SIM - simulationģ��
	
} E_ButtonCode;

// ����״̬iValue, ÿ�ΰ����������¼�. ACTION_DOWN(1��),ACTION_HOLD(0������),ACTION_UP(1��)
#define ACTION_DOWN			0x01	// ����
#define ACTION_HOLD			0x02	// ѹס
#define ACTION_UP			0x03	// ����

typedef struct InputEvent {
	struct timeval tTime;   /* ������������¼�ʱ��ʱ�� */
	int iType;  			/* ��� */
	int iCode;				/* ��ֵ */
	int iValue; 			/* ����״̬ */
}T_InputEvent, *PT_InputEvent;

typedef struct InputOpr {
	char *name;          		/* ����ģ������� */
	int (*DeviceInit)(void);  	/* �豸��ʼ������ */
	int (*DeviceExit)(void);  	/* �豸�˳����� */
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

