

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/fb.h>
#include <string.h>
#include <pthread.h>

#include <SDL/SDL.h>

#include "log.h"
#include "conf.h"
#include "disp_manager.h"

static int SDLDeviceInit(void);
static int SDLDeviceExit(void);
static int SDLShowPixel(int x, int y, unsigned int color);
static int SDLUpdateRect(Rect *rect);
static int SDLFillRect(Rect *rect, unsigned int color);

static void SDLSetEnable2(bool enable);
static int  SDLShowPixel2(int x, int y, unsigned int color);
static int  SDLUpdateRect2(Rect *rect);
static int  SDLFillRect2(Rect *rect, unsigned int color);

static SDL_Surface *g_sScreen = NULL;
static pthread_mutex_t g_mtxSDL  = PTHREAD_MUTEX_INITIALIZER; // UpdateRect只允许一个线程刷新

static T_DispOpr g_tSDLOpr = {
	.name     		= "sdl",
	.DeviceInit		= SDLDeviceInit,
	.DeviceExit		= SDLDeviceExit,
	.ShowPixel 		= SDLShowPixel,
	.UpdateRect		= SDLUpdateRect,
	.FillRect 		= SDLFillRect,

	.SetEnable2		= SDLSetEnable2,
	.ShowPixel2 	= SDLShowPixel2,	
	.UpdateRect2	= SDLUpdateRect2,
	.FillRect2 		= SDLFillRect2
};

static int SDLDeviceInit(void)
{
	const SDL_VideoInfo *info;
	Uint8  video_bpp;
	Uint32 videoflags;
	
	// Initialize SDL 
	if ( SDL_Init(SDL_INIT_VIDEO) < 0 ) 
	{
		DBG_PRINTF("Couldn't initialize SDL: %s", SDL_GetError());
		return -1;
	}
	atexit(SDL_Quit);
	
	// Alpha blending doesn't work well at 8-bit color 
	info = SDL_GetVideoInfo();
	if ( info->vfmt->BitsPerPixel > 8 )
	{
		video_bpp = info->vfmt->BitsPerPixel;
	}
	else
	{
		video_bpp = 16;
	}
	videoflags = SDL_SWSURFACE | SDL_DOUBLEBUF;
	
	g_tSDLOpr.iXres = WINDOW_WIDTH;
	g_tSDLOpr.iYres = WINDOW_HEIGHT;
	g_tSDLOpr.iBpp  = video_bpp / 8;
	
	g_sScreen = SDL_SetVideoMode(WINDOW_WIDTH, WINDOW_HEIGHT, video_bpp, videoflags);
	if(NULL == g_sScreen)
	{
		DBG_PRINTF("Couldn't set %ix%i video mode: %s\n", WINDOW_WIDTH, WINDOW_HEIGHT, SDL_GetError());
		return -2;
	}
	
	SDL_WM_SetCaption("Beeba", 0);
	
	return 0;
}


static int SDLDeviceExit(void)
{
	SDL_FreeSurface(g_sScreen);
	g_sScreen = NULL;
	
	SDL_Quit();
	
	return 0;
}

static int SDLShowPixel(int x, int y, unsigned int color)
{
	unsigned char R = (color>>16)&0xFF;
	unsigned char G = (color>> 8)&0xFF;
	unsigned char B = (color>> 0)&0xFF;
	
	pthread_mutex_lock(&g_mtxSDL);
	unsigned int _color = SDL_MapRGB(g_sScreen->format, R, G, B);

	if(SDL_MUSTLOCK(g_sScreen))
	{
		if(SDL_LockSurface(g_sScreen) < 0)
		{
			return -1;
		}	
	}
	
	switch (g_sScreen->format->BytesPerPixel) 
	{	
		case 1: /* 假定是8-bpp */  
			{ 
				unsigned char *bufp;
				bufp = (unsigned char*)g_sScreen->pixels + y*g_sScreen->pitch + x;
				*bufp = _color;
			}	
			break;	 
		case 2: /* 可能是15-bpp 或者 16-bpp */	
			{ 
				unsigned short *bufp;
				bufp = (unsigned short *)g_sScreen->pixels + y*g_sScreen->pitch/2 + x;
				*bufp = _color;
			}
			break;
		case 3: /* 慢速的24-bpp模式，通常不用 */  
			{ 
				unsigned char *bufp;   
				bufp = (Uint8 *)g_sScreen->pixels + y*g_sScreen->pitch + x;   
				*(bufp+g_sScreen->format->Rshift/8) = R;   
				*(bufp+g_sScreen->format->Gshift/8) = G;   
				*(bufp+g_sScreen->format->Bshift/8) = B;   
			}	
			break;
		case 4: /* 可能是32-bpp */
			{
				Uint32 *bufp;	
				bufp = (Uint32 *)g_sScreen->pixels + y*g_sScreen->pitch/4 + x;	 
				*bufp = _color;	 
			}
			break;
	}
	
	if(SDL_MUSTLOCK(g_sScreen))
	{
		SDL_UnlockSurface(g_sScreen);	
 	}
	pthread_mutex_unlock(&g_mtxSDL);
	
	return 0;
}

static int SDLUpdateRect(Rect *rect)
{
	int x = 0, y = 0, w = 0, h = 0;
	if(NULL==rect)
	{
		x = 0;
		y = 0;
		w = WINDOW_WIDTH;
		h = WINDOW_HEIGHT;
	}
	else
	{
		x = rect->x;
		y = rect->y;
		w = rect->w;
		h = rect->h;
	}
	
	pthread_mutex_lock(&g_mtxSDL);
	SDL_UpdateRect(g_sScreen, x, y, w, h);	 
	pthread_mutex_unlock(&g_mtxSDL);
	
	return 0;
}

static int SDLFillRect(Rect *rect, unsigned int color)
{
	SDL_Rect sdl_rect = { .x=rect->x, .y=rect->y, .w=rect->w, .h=rect->h }; 
	
	pthread_mutex_lock(&g_mtxSDL);
	SDL_FillRect(g_sScreen, &sdl_rect, color);
	pthread_mutex_unlock(&g_mtxSDL);
	
	return 0;
}

int SDLInit(void)
{
	return RegisterDispOpr(&g_tSDLOpr);
}

static void SDLSetEnable2(bool enable)
{
	
}

static int  SDLShowPixel2(int x, int y, unsigned int color)
{
	return 0;
}

static int  SDLUpdateRect2(Rect *rect)
{
	return 0;
}

static int  SDLFillRect2(Rect *rect, unsigned int color)
{
	return 0;
}

