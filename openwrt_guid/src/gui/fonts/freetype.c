
#include "log.h"
#include "conf.h"
#include "fonts_manager.h"

#include <pthread.h>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H

static int FreeTypeFontInit(char *pcFontFile, unsigned int dwFontSize);
static int FreeTypeGetFontBitmap(unsigned int dwCode, PT_FontBitMap ptFontBitMap);
static void FreeTypeSetFontSize(unsigned int dwFontSize);

static FT_Library	g_tLibrary;
static FT_Face 	    g_tFace;
static FT_GlyphSlot g_tSlot;

static pthread_mutex_t g_mtxFT  = PTHREAD_MUTEX_INITIALIZER;


/* 分配、设置、注册T_FontOpr */
static T_FontOpr g_tFreeTypeFontOpr = {
	.name          = "freetype",
	.FontInit      = FreeTypeFontInit,
	.GetFontBitmap = FreeTypeGetFontBitmap,
	.SetFontSize   = FreeTypeSetFontSize
};

static int FreeTypeFontInit(char *pcFontFile, unsigned int dwFontSize)
{
	int iError;
	
	/* 显示矢量字体 */
	iError = FT_Init_FreeType(&g_tLibrary);			   /* initialize library */
	/* error handling omitted */
	if (iError)
	{
		DBG_PRINTF("FT_Init_FreeType error!\n");
		return -1;
	}
	
	iError = FT_New_Face(g_tLibrary, pcFontFile, 0, &g_tFace); /* create face object */
	/* error handling omitted */	
	if (iError)
	{
		DBG_PRINTF("FT_New_Face error!\n");
		return -1;
	}
	
	g_tSlot = g_tFace->glyph;
	
	iError = FT_Set_Pixel_Sizes(g_tFace, dwFontSize, 0);
	if (iError)
	{
		DBG_PRINTF("FT_Set_Pixel_Sizes error!\n");
		return -1;
	}
	
	return 0;
}

static int FreeTypeGetFontBitmap(unsigned int dwCode, PT_FontBitMap ptFontBitMap)
{
	int iError;
	int iPenX = ptFontBitMap->iCurOriginX;
	int iPenY = ptFontBitMap->iCurOriginY;
	
	pthread_mutex_lock(&g_mtxFT);
	
	/* load glyph image into the slot (erase previous one) */
	iError = FT_Load_Char(g_tFace, dwCode, FT_LOAD_RENDER | FT_LOAD_MONOCHROME);
	if (iError)
	{
		DBG_PRINTF("FT_Load_Char error\n");
		return -1;
	}
	
	ptFontBitMap->iXLeft       = iPenX + g_tSlot->bitmap_left;
	ptFontBitMap->iYTop        = iPenY - g_tSlot->bitmap_top;
	ptFontBitMap->iXMax        = ptFontBitMap->iXLeft + g_tSlot->bitmap.width;
	ptFontBitMap->iYMax        = ptFontBitMap->iYTop  + g_tSlot->bitmap.rows;
	ptFontBitMap->iBpp         = 1;
	ptFontBitMap->iPitch       = g_tSlot->bitmap.pitch;
	ptFontBitMap->iNextOriginX = iPenX + g_tSlot->advance.x / 64;
	ptFontBitMap->iNextOriginY = iPenY;
	ptFontBitMap->pucBuffer    = g_tSlot->bitmap.buffer;
	
#if 0	
	DBG_PRINTF("dwCode:%d,iPenY:%d,iYTop:%d,iYMax:%d,H:%d,W:%d,F:%d,advanceX:%d,advanceY:%d,iPitch:%d",
		dwCode,
		iPenY,
		ptFontBitMap->iYTop,
		ptFontBitMap->iYMax,
		ptFontBitMap->iYMax-ptFontBitMap->iYTop+1,
		ptFontBitMap->iXMax-ptFontBitMap->iXLeft+1,
		ptFontBitMap->iYMax-iPenY,
		g_tSlot->advance.x/64,
		g_tSlot->advance.y/64,
		ptFontBitMap->iPitch);
#endif
	
	pthread_mutex_unlock(&g_mtxFT);
	
	return 0;
}

static void FreeTypeSetFontSize(unsigned int dwFontSize)
{
	FT_Set_Pixel_Sizes(g_tFace, dwFontSize, 0);
}

int FreeTypeInit(void)
{
	return RegisterFontOpr(&g_tFreeTypeFontOpr);
}

