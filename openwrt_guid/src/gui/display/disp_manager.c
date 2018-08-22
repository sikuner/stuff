
#include "log.h"
#include "disp_manager.h"

#include <stdio.h>
#include <string.h>

static PT_DispOpr g_ptDispOprHead;

int RegisterDispOpr(PT_DispOpr ptDispOpr)
{
	PT_DispOpr ptTmp;

	if (!g_ptDispOprHead)
	{
		g_ptDispOprHead   = ptDispOpr;
		ptDispOpr->ptNext = NULL;
	}
	else
	{
		ptTmp = g_ptDispOprHead;
		while (ptTmp->ptNext)
		{
			ptTmp = ptTmp->ptNext;
		}
		ptTmp->ptNext	  = ptDispOpr;
		ptDispOpr->ptNext = NULL;
	}
	
	return 0;
}

PT_DispOpr GetDispOpr(char *pcName)
{
	PT_DispOpr ptTmp = g_ptDispOprHead;
	
	while (ptTmp)
	{
		if (strcmp(ptTmp->name, pcName) == 0)
		{
			return ptTmp;
		}
		ptTmp = ptTmp->ptNext;
	}
	return NULL;
}

void ShowDispOpr(void)
{
	int i = 0;
	PT_DispOpr ptTmp = g_ptDispOprHead;

	while (ptTmp)
	{
		printf("    %02d %s \n", i++, ptTmp->name);
		ptTmp = ptTmp->ptNext;
	}
}

int DisplayInit(void)
{
	int iError;
	
#if SDL_SUPPORT
	iError = SDLInit();
	if (iError)
	{
		DBG_PRINTF("SDLInit error!\n");
		return -1;
	}
#else
	iError = OledInit();
	if (iError)
	{
		DBG_PRINTF("OledInit error!\n");
		return -1;
	}
#endif
	
	return iError;
}

