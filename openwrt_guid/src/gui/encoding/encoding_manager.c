
#include "log.h"
#include "conf.h"
#include "encoding_manager.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

static PT_EncodingOpr g_ptEncodingOprHead;

int RegisterEncodingOpr(PT_EncodingOpr ptEncodingOpr)
{
	PT_EncodingOpr ptTmp;

	if (!g_ptEncodingOprHead)
	{
		g_ptEncodingOprHead   = ptEncodingOpr;
		ptEncodingOpr->ptNext = NULL;
	}
	else
	{
		ptTmp = g_ptEncodingOprHead;
		while (ptTmp->ptNext)
		{
			ptTmp = ptTmp->ptNext;
		}
		ptTmp->ptNext	      = ptEncodingOpr;
		ptEncodingOpr->ptNext = NULL;
	}
	
	return 0;
}

void ShowEncodingOpr(void)
{
	int i = 0;
	PT_EncodingOpr ptTmp = g_ptEncodingOprHead;
	
	while (ptTmp)
	{
		printf("    %02d %s \n", i++, ptTmp->name);
		ptTmp = ptTmp->ptNext;
	}
}

PT_EncodingOpr GetEncodingOpr(char *szName)
{
	PT_EncodingOpr ptTmp = g_ptEncodingOprHead;
	
	while(ptTmp)
	{
		if(strcmp(szName, ptTmp->name)==0)
		{
			return ptTmp;
		}
		
		ptTmp = ptTmp->ptNext;
	}
	
	return NULL;
}

PT_EncodingOpr SelectEncodingOprForFile(unsigned char *pucFileBufHead)
{
	PT_EncodingOpr ptTmp = g_ptEncodingOprHead;
	
	while (ptTmp)
	{
		if (ptTmp->isSupport(pucFileBufHead))
			return ptTmp;
		else
			ptTmp = ptTmp->ptNext;
	}
	
	return NULL;
}

PT_EncodingOpr SelectEncodingOpr(char *szEncodingName)
{
	PT_EncodingOpr ptTmp = g_ptEncodingOprHead;
	
	while (ptTmp)
	{
		if ((0==strcmp(ptTmp->name, szEncodingName)))
			return ptTmp;
		else
			ptTmp = ptTmp->ptNext;
	}
	
	return NULL;
}

int AddFontOprForEncoding(PT_EncodingOpr ptEncodingOpr, PT_FontOpr ptFontOpr)
{
	PT_FontOpr ptFontOprCpy;
	
	if (!ptEncodingOpr || !ptFontOpr)
	{
		return -1;
	}
	else
	{
		ptFontOprCpy = malloc(sizeof(T_FontOpr));
		if (!ptFontOprCpy)
		{
			return -1;
		}
		else
		{
			memcpy(ptFontOprCpy, ptFontOpr, sizeof(T_FontOpr));
			ptFontOprCpy->ptNext = ptEncodingOpr->ptFontOprSupportedHead;	// 插入到第一个位置
			ptEncodingOpr->ptFontOprSupportedHead = ptFontOprCpy;
			return 0;
		}		
	}
}

int DelFontOprFrmEncoding(PT_EncodingOpr ptEncodingOpr, PT_FontOpr ptFontOpr)
{
	PT_FontOpr ptTmp;
	PT_FontOpr ptPre;
		
	if (!ptEncodingOpr || !ptFontOpr)
	{
		return -1;
	}
	else
	{
		ptTmp = ptEncodingOpr->ptFontOprSupportedHead;
		if (strcmp(ptTmp->name, ptFontOpr->name) == 0)
		{
			/* 删除头节点 */
			ptEncodingOpr->ptFontOprSupportedHead = ptTmp->ptNext;
			free(ptTmp);
			return 0;
		}

		ptPre = ptEncodingOpr->ptFontOprSupportedHead;
		ptTmp = ptPre->ptNext;
		while (ptTmp)
		{
			if (strcmp(ptTmp->name, ptFontOpr->name) == 0)
			{
				/* 从链表里取出、释放 */
				ptPre->ptNext = ptTmp->ptNext;
				free(ptTmp);
				return 0;
			}
			else
			{
				ptPre = ptTmp;
				ptTmp = ptTmp->ptNext;
			}
		}
		
		return -1;
	}
}

int EncodingInit(void)
{
	int iError;
	
	iError = Utf8EncodingInit();
	if (iError)
	{
		DBG_PRINTF("Utf8EncodingInit error!\n");
		return -1;
	}
	
	return 0;
}

