
#ifndef __ENCODING_MANAGER_H__
#define __ENCODING_MANAGER_H__

#include "fonts_manager.h"
#include "disp_manager.h"

typedef struct EncodingOpr {
	char *name;
	int iHeadLen;
	PT_FontOpr ptFontOprSupportedHead;
	int (*isSupport)(unsigned char *pucBufHead);
	int (*GetCodeFrmBuf)(unsigned char *pucBufStart, unsigned char *pucBufEnd, unsigned int *pdwCode);
	struct EncodingOpr *ptNext;
}T_EncodingOpr, *PT_EncodingOpr;

int RegisterEncodingOpr(PT_EncodingOpr ptEncodingOpr);
void ShowEncodingOpr(void);

PT_EncodingOpr GetEncodingOpr(char *szName);

PT_EncodingOpr SelectEncodingOprForFile(unsigned char *pucFileBufHead);
PT_EncodingOpr SelectEncodingOpr(char *szEncodingName);

int AddFontOprForEncoding(PT_EncodingOpr ptEncodingOpr, PT_FontOpr ptFontOpr);
int DelFontOprFrmEncoding(PT_EncodingOpr ptEncodingOpr, PT_FontOpr ptFontOpr);

int EncodingInit(void);
int Utf8EncodingInit(void);

#endif // __ENCODING_MANAGER_H__

