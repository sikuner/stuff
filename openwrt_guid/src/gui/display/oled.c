
#include "types.h"
#include "oled_api.h"
#include "log.h"
#include "conf.h"
#include "disp_manager.h"

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <pthread.h>


static int  OledDeviceInit(void);
static int  OledDeviceExit(void);

static int  OledShowPixel(int x, int y, unsigned int color);
static int  OledFillRect(Rect *rect, unsigned int color);
static int  OledUpdateRect(Rect *rect);

static void OledSetEnableS(bool enable);
static int  OledShowPixelS(int x, int y, unsigned int color);
static int  OledFillRectS(Rect *rect, unsigned int color);
static int  OledUpdateRectS(Rect *rect);

static void OledSetEnableT(bool enable);
static int  OledShowPixelT(int x, int y, unsigned int color);
static int  OledFillRectT(Rect *rect, unsigned int color);
static int  OledUpdateRectT(Rect *rect);

static T_DispOpr g_tOledOpr = {
	.name     		= "oled",
	.DeviceInit		= OledDeviceInit,
	.DeviceExit		= OledDeviceExit,
	
	.ShowPixel 		= OledShowPixel,	
	.FillRect 		= OledFillRect,
	.UpdateRect		= OledUpdateRect,
	
	.SetEnableS		= OledSetEnableS,
	.ShowPixelS 	= OledShowPixelS,
	.FillRectS 		= OledFillRectS,
	.UpdateRectS	= OledUpdateRectS, // 与text的OledUpdateRect相同
	
	.SetEnableT		= OledSetEnableT,
	.ShowPixelT 	= OledShowPixelT,	
	.FillRectT 		= OledFillRectT,
	.UpdateRectT	= OledUpdateRectT
};

static pthread_mutex_t g_mtxOled  = PTHREAD_MUTEX_INITIALIZER;

static volatile bool status_enable = true;
static volatile bool toast_enable  = false;

static unsigned char TextFrame  [WINDOW_HEIGHT][WINDOW_WIDTH/8] = { {0} };
static unsigned char StatusFrame[WINDOW_HEIGHT][WINDOW_WIDTH/8] = { {0} };
static unsigned char ToastFrame [WINDOW_HEIGHT][WINDOW_WIDTH/8] = { {0} };


static int OledDeviceInit(void)
{
	g_tOledOpr.iXres = WINDOW_WIDTH;
	g_tOledOpr.iYres = WINDOW_HEIGHT;
	g_tOledOpr.iBpp  = 1;
	
	return OledDrvInit();
}

static int OledDeviceExit(void)
{
	OledDrvRelease();
	return 0;
}

////////////////////////////////////////////////////////////////////
static int GetPixel(int x, int y)
{
	if(x<0||y<0||x>=WINDOW_WIDTH||y>=WINDOW_HEIGHT)
	{
		return -1;
	}
	
	// 商数quotient, 余数remainder 
	int q = x/8;
	int r = x%8;
	
	if(status_enable && x<STATUS_W && y<STATUS_H && x>=0 && y>=0)
	{
		return (int)((StatusFrame[y][q]&(1<<r))>>r);
	}
	
	return (int)((TextFrame[y][q]&(1<<r))>>r);
}

static int OledShowPixel(int x, int y, unsigned int color)
{
	if(x<0||y<0||x>=WINDOW_WIDTH||y>=WINDOW_HEIGHT)
	{
		return -1;
	}
	
	// 商数quotient, 余数remainder 
	int q = x/8;
	int r = x%8;
	
	if(color > 0)
	{
		TextFrame[y][q] |= 1<<r;		// 设置为1
	}
	else
	{
		TextFrame[y][q] &= ~(1<<r); 	// 设置为0
	}
	
	return 0;
}

static int OledFillRect(Rect *rect, unsigned int color)
{
	if(!rect)
		return -1;
	
	int y_top		= max(rect->y, 0); 							// 上
	int y_bottom	= min(rect->y + rect->h, WINDOW_HEIGHT);	// 下
	int x_left		= max(rect->x, 0); 							// 左
	int x_right 	= min(rect->x+rect->w, WINDOW_WIDTH);		// 右
	int x = 0, y = 0;
	
	for(y = y_top; y < y_bottom; y++)
	{
		for(x = x_left; x < x_right; x++)
		{
			OledShowPixel(x, y, color);
		}
	}
	
	return 0;
}

static int OledUpdateRect(Rect *rect)
{
	if(!rect)
		return -1;
	
	if(toast_enable)
		return 1;
	
	Rect dst = { .x=rect->x, .y=rect->y, .w=rect->w, .h=rect->h };
	
	int y_top		= max(rect->y, 0); 							// 上
	int y_bottom	= min(rect->y + rect->h, WINDOW_HEIGHT);	// 下
	int x_left		= max(rect->x, 0); 							// 左
	int x_right 	= min(rect->x + rect->w, WINDOW_WIDTH);		// 右
	
	// [page_begin, page_end], [column_start, column_start+column_total)
	unsigned char page_begin	= y_top / 8;
	unsigned char page_end		= (y_bottom - 1) / 8;
	unsigned char column_start	= dst.x;
	unsigned char column_total	= dst.w;
	
	// [y_top, y_bottom) 调整后的数值, 都是 8 的倍数
	y_top = 8 * page_begin;
	y_bottom = 8 * page_end + 8;
	
	int strLen = (y_bottom-y_top)*(x_right-x_left)/8;
	unsigned char *data_buf = (unsigned char *)malloc(strLen);
	if(!data_buf)
	{
		DBG_PRINTF("Fail to malloc memory!!!");
		return -1;
	}
	
	unsigned char byte = 0;
	int xi = 0, pi = 0, i = 0, data_idx = 0;
	for(pi = page_begin; pi <= page_end; pi++)
	{
		for(xi = x_left; xi < x_right; xi++)
		{
			byte = 0x00;
			for(i = 0; i < 8; i++) // yi <= [pi*8, pi*8+8)
			{
				byte |= GetPixel(xi, pi*8+i)<<i;
			}
			data_buf[data_idx++] = byte;
		}
	}
	
//	DBG_PRINTF("strLen=%d, data_idx=%d", strLen, data_idx);
//	DBG_PRINTF("data_buf:0x%08x, page_begin:%d, page_end:%d, column_start:%d, column_total:%d", 
//		data_buf, page_begin, page_end, column_start, column_total);
	
	pthread_mutex_lock(&g_mtxOled);
	ShowPattern(data_buf, page_begin, page_end, column_start, column_total);
	pthread_mutex_unlock(&g_mtxOled);		
	
	free(data_buf);
	data_buf = NULL;
	
	return 0;
}

static void OledSetEnableS(bool enable)
{
	status_enable = enable;
}

static int  OledShowPixelS(int x, int y, unsigned int color)
{
	if(x<0||y<0||x>=WINDOW_WIDTH||y>=WINDOW_HEIGHT)
	{
		return -1;
	}
	
	// 商数quotient, 余数remainder 
	int q = x/8;
	int r = x%8;
	
	if(color > 0)
	{
		StatusFrame[y][q] |= 1<<r;		// 设置为1
	}
	else
	{
		StatusFrame[y][q] &= ~(1<<r); 	// 设置为0
	}
	
	return 0;
}

static int  OledFillRectS(Rect *rect, unsigned int color)
{
	if(!rect)
		return -1;
	
	int y_top		= max(rect->y, 0);							// 上
	int y_bottom	= min(rect->y + rect->h, WINDOW_HEIGHT);	// 下
	int x_left		= max(rect->x, 0);							// 左
	int x_right 	= min(rect->x+rect->w, WINDOW_WIDTH);		// 右
	int x = 0, y = 0;
	
	for(y = y_top; y < y_bottom; y++)
	{
		for(x = x_left; x < x_right; x++)
		{
			OledShowPixelS(x, y, color);
		}
	}
	
	return 0;
}

static int  OledUpdateRectS(Rect *rect)
{
	return OledUpdateRect(rect);
}

/////////////////////////////////////////////////////////////////////////////

static void OledSetEnableT(bool enable)
{
	toast_enable = enable;
}

static int  OledShowPixelT(int x, int y, unsigned int color)
{
	if(x<0||y<0||x>=WINDOW_WIDTH||y>=WINDOW_HEIGHT)
	{
		return -1;
	}
	
	// 商数quotient, 余数remainder 
	int q = x/8;
	int r = x%8;
	
	if(color > 0)
	{
		ToastFrame[y][q] |= 1<<r;		// 设置为1
	}
	else
	{
		ToastFrame[y][q] &= ~(1<<r);	// 设置为0
	}
	
	return 0;
}

static int  OledFillRectT(Rect *rect, unsigned int color)
{
	if(!rect)
		return -1;
	
	int y_top		= max(rect->y, 0);							// 上
	int y_bottom	= min(rect->y + rect->h, WINDOW_HEIGHT);	// 下
	int x_left		= max(rect->x, 0);							// 左
	int x_right 	= min(rect->x+rect->w, WINDOW_WIDTH);		// 右
	int x = 0, y = 0;
	
	for(y = y_top; y < y_bottom; y++)
	{
		for(x = x_left; x < x_right; x++)
		{
			OledShowPixelT(x, y, color);
		}
	}
	
	return 0;
}

static int GetPixelT(int x, int y)
{
	if(x<0||y<0||x>=WINDOW_WIDTH||y>=WINDOW_HEIGHT)
	{
		return -1;
	}
	
	// 商数quotient, 余数remainder 
	int q = x/8;
	int r = x%8;
	
	return (int)((ToastFrame[y][q]&(1<<r))>>r);
}
static int  OledUpdateRectT(Rect *rect)
{
	if(!rect)
		return -1;
	
	if(!toast_enable)
		return 1;
	
	Rect dst = { .x=rect->x, .y=rect->y, .w=rect->w, .h=rect->h };
	
	int y_top		= max(rect->y, 0);							// 上
	int y_bottom	= min(rect->y + rect->h, WINDOW_HEIGHT);	// 下
	int x_left		= max(rect->x, 0);							// 左
	int x_right 	= min(rect->x+rect->w, WINDOW_WIDTH);		// 右
	
	// [page_begin, page_end], [column_start, column_start+column_total)
	unsigned char page_begin	= y_top / 8;
	unsigned char page_end		= (y_bottom - 1) / 8;
	unsigned char column_start	= dst.x;
	unsigned char column_total	= dst.w;
	
	// [y_top, y_bottom) 调整后的数值, 都是 8 的倍数
	y_top = 8 * page_begin;
	y_bottom = 8 * page_end + 8;
	
	int strLen = (y_bottom-y_top)*(x_right-x_left)/8;
	unsigned char *data_buf = (unsigned char *)malloc(strLen);
	if(!data_buf)
	{
		DBG_PRINTF("Fail to malloc memory!!!");
		return -1;
	}
	
	unsigned char byte = 0;
	int xi = 0, pi = 0, i = 0, data_idx = 0;
	for(pi = page_begin; pi <= page_end; pi++)
	{
		for(xi = x_left; xi < x_right; xi++)
		{
			byte = 0x00;
			for(i = 0; i < 8; i++) // yi <= [pi*8, pi*8+8)
			{
				byte |= GetPixelT(xi, pi*8+i)<<i;
			}
			data_buf[data_idx++] = byte;
		}
	}
	
//	DBG_PRINTF("strLen=%d, data_idx=%d", strLen, data_idx);
//	DBG_PRINTF("data_buf:0x%08x, page_begin:%d, page_end:%d, column_start:%d, column_total:%d", 
//		data_buf, page_begin, page_end, column_start, column_total);
	
	pthread_mutex_lock(&g_mtxOled);
	ShowPattern(data_buf, page_begin, page_end, column_start, column_total);
	pthread_mutex_unlock(&g_mtxOled);		
	
	free(data_buf);
	data_buf = NULL;
	
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////
int OledInit(void)
{
	return RegisterDispOpr(&g_tOledOpr);
}

