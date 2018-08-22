
#ifndef __WIDGET_H__
#define __WIDGET_H__

#include "types.h"
#include "fonts_manager.h"
#include <libubox/list.h>
#include <stdint.h>

typedef struct tagBITMAPFILEHEADER { /* bmfh */
	uint16_t  bfType; 
	uint32_t  bfSize;
	uint16_t  bfReserved1;
	uint16_t  bfReserved2;
	uint32_t  bfOffBits;
}  __attribute__ ((packed)) BITMAPFILEHEADER;

typedef struct tagBITMAPINFOHEADER { /* bmih */
	uint32_t  biSize;
	uint32_t  biWidth;
	uint32_t  biHeight;
	uint16_t  biPlanes;
	uint16_t  biBitCount;
	uint32_t  biCompression;
	uint32_t  biSizeImage;
	uint32_t  biXPelsPerMeter;
	uint32_t  biYPelsPerMeter;
	uint32_t  biClrUsed;
	uint32_t  biClrImportant;
}  __attribute__ ((packed)) BITMAPINFOHEADER;

typedef struct _tMemRect {
	int width;
	int height;
	unsigned char *pBuf;
} __attribute__ ((packed)) MemRect;

typedef struct _tCharInfo {
	struct list_head list;
	
	int unicode;				// 字符的unicode编码
	int advanceX; 				// 每个字符占居的宽度. 单位: 像素
} __attribute__ ((packed)) CharInfo;

///////////////////////////////////////////////////////////////////////////
////////////////////////////Display////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
int  SelectAndInitDisplay(char *szDisplay);
void DisplayExit(void);
int  SetFontsInit(char *szTureTypeFile);
int  SelectEncodingOprOnName(char *szEncodingName);

///////////////////////////////////////////////////////////////////////////
////////////////////////////Text///////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
int  ShowLine1Waiting(void);				// 在语音搜索时,等待响应图标
int  ShowLine1Twinkle(char *szLine);		// 第1行显示文字, 闪烁
int  ShowLine1(char *szLine);			// 第1行显示文字
int  ShowLine2(char *szLine);
int  ShowLine3(char *szLine);
int  ShowLine0123(char *line0, char *line1, char *line2, char *line3); // for gui_cli

int  ShowBmp(Rect *dst, char *filename);

int  ClearAllText(void);

int  UpdateRect(Rect *rect);
///////////////////////////////////////////////////////////////////////////
////////////////////////////StatusBar//////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
void StatusEnable(bool enable);

int  ShowBattery(int percent, bool charge);

int  ShowWifi(int state, bool twinkle); 	// -1, 0, 1, 2
int  ShowBluetooth(int state);
int  ShowLinein(int show); 					// show>0, 显示; show<0, 隐藏

int  ShowDown(int show); 					// show>0, 显示; show<=0, 隐藏
int  ShowLock(int show); 					// show>0, 锁; lock<=0, 隐藏
int  ShowAlarm(int show); 					// show>0, 显示; show<=0, 隐藏
int  ShowPlayerState(int mpd_state); 		// 显示播放器的状态, 显示 播放/暂停. 停止时不显示

int  ShowTime(int show);					// 显示时间: disp>0, 显示时间, disp<=0, 隐藏时间

///////////////////////////////////////////////////////////////////////////
////////////////////////////Toast//////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
int  Toast(Rect *dst, char *tips, int fontsize, int duration);
int  ToastBmp(Rect *dst, char *filename, int duration);
int  Toast123(char *line1, char *line2, char *line3, int duration);
int  Toast0123(char *line0, char *line1, char *line2, char *line3, int duration);

void ToastCancel(void);
int  ToastRelease(void);

///////////////////////////////////////////////////////////////////////////
////////////////////////////Widget////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
int  WidgetInit(void);
void WidgetExit(void);

#endif // __WIDGET_H__

