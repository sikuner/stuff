
#ifndef __FONTS_MANAGER_H__
#define __FONTS_MANAGER_H__

typedef struct FontBitMap {
	int iXLeft;
	int iYTop;
	int iXMax;
	int iYMax;
	int iBpp;
	int iPitch;   /* ���ڵ�ɫλͼ, ��������֮��Ŀ�� */
	int iCurOriginX;
	int iCurOriginY;
	int iNextOriginX;
	int iNextOriginY;
	unsigned char *pucBuffer;
}T_FontBitMap, *PT_FontBitMap;

typedef struct FontOpr {
	char *name;
	int (*FontInit)(char *pcFontFile, unsigned int dwFontSize);
	int (*GetFontBitmap)(unsigned int dwCode, PT_FontBitMap ptFontBitMap);	
	void (*SetFontSize)(unsigned int dwFontSize);   /* ��������ߴ�(��λ:����) */
	struct FontOpr *ptNext;
}T_FontOpr, *PT_FontOpr;

int RegisterFontOpr(PT_FontOpr ptFontOpr);
void ShowFontOpr(void);
PT_FontOpr GetFontOpr(char *pcName);

int FontsInit(void);
int FreeTypeInit(void);

#endif // __FONTS_MANAGER_H__

