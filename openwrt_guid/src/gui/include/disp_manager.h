
#ifndef __DISP_MANAGER_H__
#define __DISP_MANAGER_H__

#include <stdbool.h>
#include "types.h"

typedef struct DispOpr {
	char *name;
	int  iXres;
	int  iYres;
	int  iBpp;
	int  (*DeviceInit)(void);
	int  (*DeviceExit)(void);
	
	int  (*ShowPixel)(int x, int y, unsigned int color);
	int  (*FillRect)(Rect *rect, unsigned int color);
	int  (*UpdateRect)(Rect *rect);
	
	void (*SetEnableS)(bool enable);
	int  (*ShowPixelS)(int x, int y, unsigned int color);
	int  (*FillRectS)(Rect *rect, unsigned int color);
	int  (*UpdateRectS)(Rect *rect);
	
	void (*SetEnableT)(bool enable);
	int  (*ShowPixelT)(int x, int y, unsigned int color);
	int  (*FillRectT)(Rect *rect, unsigned int color);
	int  (*UpdateRectT)(Rect *rect);
	
	struct DispOpr *ptNext;
} T_DispOpr, *PT_DispOpr;

int RegisterDispOpr(PT_DispOpr ptDispOpr);
PT_DispOpr GetDispOpr(char *pcName);
void ShowDispOpr(void);

int DisplayInit(void);

#if SDL_SUPPORT
	int SDLInit(void);
#else
	int OledInit(void);
#endif

#endif // __DISP_MANAGER_H__

