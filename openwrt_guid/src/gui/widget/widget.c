
#include "log.h"
#include "widget.h"
#include "conf.h"
#include "common.h"
#include "fonts_manager.h"
#include "encoding_manager.h"
#include "utf8_strings.h"

#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <pthread.h>
#include <libubox/blobmsg_json.h>
#include <libubus.h>

static T_DispOpr 	 *g_ptDispOpr 	  = NULL;
static T_EncodingOpr *g_ptEncodingOpr = NULL;

///////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////Display & Widget///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////

int SelectAndInitDisplay(char *pcName)
{
	int iError;
	g_ptDispOpr = GetDispOpr(pcName);
	if (!g_ptDispOpr)
	{
		return -1;
	}
	
	iError = g_ptDispOpr->DeviceInit();
	return iError;
}

void DisplayExit(void)
{
	if (g_ptDispOpr->DeviceExit)
	{
		g_ptDispOpr->DeviceExit();
	}
}

int SetFontsInit(char *szTureTypeFile)
{
	fprintf(stderr, " === TureTypeFile: %s \n", szTureTypeFile);
	
	int iError = -1;
	PT_FontOpr ptFontOpr;
	PT_FontOpr ptTmp;
	int iRet = -1;
	
	unsigned int dwFontSize = DEFAULT_FONT_SIZE;
	
	ptFontOpr = g_ptEncodingOpr->ptFontOprSupportedHead;
	while (ptFontOpr)
	{
		if (0 == strcmp(ptFontOpr->name, "freetype"))
		{
			iError = ptFontOpr->FontInit(szTureTypeFile, dwFontSize);
		}
		DBG_PRINTF("%s, %d\n", ptFontOpr->name, iError);
		
		ptTmp = ptFontOpr->ptNext;
		
		if (iError == 0)
		{
			/* 比如对于ascii编码的文件, 可能用ascii字体也可能用gbk字体, 
			 * 所以只要有一个FontInit成功, SetTextDetail最终就返回成功
			 */
			iRet = 0;
		}
		else
		{
			DelFontOprFrmEncoding(g_ptEncodingOpr, ptFontOpr);
		}
		
		ptFontOpr = ptTmp;
	}
	
	DBG_PRINTF("END SetFontsInit iRet: %d \n", iRet);
	return iRet;
}

int SelectEncodingOprOnName(char *szEncodingName)
{
	if(NULL==szEncodingName)
	{
		return -1;
	}
	
	g_ptEncodingOpr = SelectEncodingOpr(szEncodingName);
	if(NULL==g_ptEncodingOpr)
	{
		return -1;
	}
	
	return 0;
}

///////////////////////////////////////////////////////////////////////////
////////////////////////////Tools//////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

static int getLocaltime(int *hour, int *minute)
{
	time_t t;
	struct tm *p;
	
	time(&t);
	p = localtime(&t);
	
//	DBG_PRINTF("%04d/%02d/%02d %02d:%02d:%02d", 
//		(1900+p->tm_year), (1+p->tm_mon), p->tm_mday, p->tm_hour, p->tm_min, p->tm_sec);
	
	if(NULL==hour || NULL==minute)
	{
		return -1;
	}
	
	*hour = p->tm_hour;
	*minute  = p->tm_min;
	
	return 0;
}

static int SetPixelMemRect(MemRect *memRect, int x, int y, unsigned int color)
{
	if(NULL==memRect)
		return -1;		// 参数出错
		
	if(x<0||x>=memRect->width||y<0||y>=memRect->height)
	{
		DBG_PRINTF("AccessViolation width: %d, height: %d, x: %d, y: %d",
			memRect->width, memRect->height, x, y);
		return -1;
	}
	
	int index = y * memRect->width+ x;
	
	// 商数quotient, 余数remainder 
	int q = index / 8;
	int r = index % 8;
	
	if(color)
		*(memRect->pBuf+q) |= 1<<r; 	// 对应位 置1
	else
		*(memRect->pBuf+q) &= ~(1<<r); 	// 对应位 置0
	
	return 0;
}
static int GetPixelMemRect(MemRect *memRect, int x, int y)
{
	if(NULL==memRect)
		return -1;		// 参数出错
	
	if(x<0||x>=memRect->width||y<0||y>=memRect->height)
	{
		DBG_PRINTF("AccessViolation width: %d, height: %d, x: %d, y: %d",
			memRect->width, memRect->height, x, y);
		return -1;
	}
	
	int index = y * memRect->width + x;
	
	// 商数quotient, 余数remainder 
	int q = index / 8;
	int r = index % 8;
	
	return (int)((*(memRect->pBuf+q)&(1<<r))>>r);
}
static int ShowMemRect(MemRect *memRect)
{
	if(NULL==memRect)
		return -1;
	
	int x, y;
	
	for(y = 0; y < memRect->height; y++)
	{
		for(x = 0; x < memRect->width; x++)
		{
			fprintf(stderr, "%c", (GetPixelMemRect(memRect, x, y)==1)?'X':'_');
		}
		
		fprintf(stderr, "\n");
	}
	
	return 0;
}

static int ShowFontBitMapMemRect(MemRect *memRect, T_FontBitMap *ptFontBitMap)
{
	int x;
	int y;
	unsigned char ucByte = 0;
	int i = 0;
	int bit;
	
	if (ptFontBitMap->iBpp == 1)
	{
		for (y = ptFontBitMap->iYTop; y < ptFontBitMap->iYMax; y++)
		{
			i = (y - ptFontBitMap->iYTop) * ptFontBitMap->iPitch;
			for (x = ptFontBitMap->iXLeft, bit = 7; x < ptFontBitMap->iXMax; x++)
			{
				if (bit == 7)
				{
					ucByte = ptFontBitMap->pucBuffer[i++];
				}
				if (ucByte & (1<<bit))
				{
					SetPixelMemRect(memRect, x, y, COLOR_FOREGROUND);
				}
				else /* 使用背景色, 不用描画 */
				{
					SetPixelMemRect(memRect, x, y, COLOR_BACKGROUND);
				}
				bit--;				
				if (bit < 0)
				{
					bit = 7;
				}
			}
		}
	}
	else if (ptFontBitMap->iBpp == 8)
	{
		for (y = ptFontBitMap->iYTop; y < ptFontBitMap->iYMax; y++)
			for (x = ptFontBitMap->iXLeft; x < ptFontBitMap->iXMax; x++)
			{
				if (ptFontBitMap->pucBuffer[i++])
				{
					SetPixelMemRect(memRect, x, y, COLOR_FOREGROUND);					
				}
			}
	}
	else
	{
		DBG_PRINTF("ShowOneFont error, can't support %d bpp\n", ptFontBitMap->iBpp);
		return -1;
	}
	
	return 0;
}

static CharInfo* NewCharInfo(int unicode, int advanceX)
{
	CharInfo *ci = (CharInfo*)malloc(sizeof(CharInfo));
	if (ci == NULL)
	{
		DBG_PRINTF("Failed to malloc memory");
		return NULL;
	}
	
	ci->unicode = unicode;
	ci->advanceX = advanceX;
	
	return ci;
}

static int ParseStringMemRect(MemRect *memRect, char *szLine, int fontSize)
{
	if(NULL==memRect || NULL==szLine || NULL==g_ptEncodingOpr)
	{
		return -1;
	}	
	DBG_PRINTF("szLine: %s", szLine);
	
	int iError;
	int iLen = 0;
	unsigned char *pStart = (unsigned char *)szLine;
	unsigned char *pEnd	= (unsigned char *)szLine + strlen(szLine);
	unsigned int dwCode = 0;
	int advanceX = 0;	
	
	int lineWidth	= 0; 				// szLine文字的总长度
	int lineHeight	= fontSize; 		// szLine文字的总宽度
	int curOriginY = lineHeight*3/4;	// 目前只支持freetype, 原点暂设为其 行高的3/4处
	int minY = curOriginY, maxY = curOriginY;
	
	struct list_head chars_head = LIST_HEAD_INIT(chars_head);
	int chars_num = 0; // 字符个数
	
	T_FontOpr *ptFontOpr = NULL;
	T_FontBitMap tFontBitMap;
	
	tFontBitMap.iCurOriginX = 0;
	tFontBitMap.iCurOriginY = curOriginY; // 目前只支持freetype, 原点暂设为其 行高的3/4处 12
	
	CharInfo *charInfo = NULL;
	
	ptFontOpr = GetFontOpr("freetype");
	if(NULL==ptFontOpr)
	{
		return -1;
	}
	ptFontOpr->SetFontSize(fontSize);
	
	while(true)
	{
		iLen = g_ptEncodingOpr->GetCodeFrmBuf(pStart, pEnd, &dwCode);
 		if (0 == iLen)
		{
			break;
		}
		pStart += iLen;
		if((dwCode=='\n')||(dwCode == '\r'))
		{
			continue;
		}
		else if (dwCode == '\t')
		{
			/* TAB键用一个空格代替 */
			dwCode = ' ';
		}
//		DBG_PRINTF("dwCode = 0x%x\n", dwCode);
		iError = ptFontOpr->GetFontBitmap(dwCode, &tFontBitMap);
		if(0 == iError)
		{
			advanceX = tFontBitMap.iNextOriginX - tFontBitMap.iCurOriginX;
			lineWidth += advanceX;
			if(tFontBitMap.iYMax > maxY)
				maxY = tFontBitMap.iYMax;
			
			if(tFontBitMap.iYTop < minY)
				minY = tFontBitMap.iYTop;
			
			charInfo = NewCharInfo(dwCode, advanceX);
			if(NULL==charInfo)
				return -1;
			
			list_add_tail(&charInfo->list, &chars_head);
			if(++chars_num > MAX_CHARS_NUM)
				break;
			
			tFontBitMap.iCurOriginX = tFontBitMap.iNextOriginX;
			tFontBitMap.iCurOriginY = tFontBitMap.iNextOriginY;
		}
	}
	
	int actualWidth  	= lineWidth+1;
	int actualHeight 	= maxY - minY + 1;
	int actualOriginY 	= curOriginY - minY + 1;
	
//	DBG_PRINTF("[actualWidth:%d, actualHeight:%d, actualOriginY:%d, fontSize:%d] maxY:%d, minY:%d, originY:%d", 
//		actualWidth, actualHeight, actualOriginY, fontSize, 
//		maxY, minY, curOriginY);
	int memLength = (actualWidth*actualHeight+7)/8;
	unsigned char *pBuf = (unsigned char *)malloc(memLength);
	if(NULL == pBuf)
	{
		DBG_PRINTF("Can't malloc memory, ERROR!");
		return -1;
	}
	if(NULL != memRect->pBuf)
	{
		free(memRect->pBuf);		
		memRect->pBuf = NULL;
		memRect->width = 0;
		memRect->height = 0;
	}
	memset(pBuf, 0, memLength);
	memRect->width 	= actualWidth;
	memRect->height = actualHeight;
	memRect->pBuf 	= pBuf;
	
	// 解析字体, 并拷贝到内存	
	tFontBitMap.iCurOriginX = 0;
	tFontBitMap.iCurOriginY = actualOriginY;
	list_for_each_entry(charInfo, &chars_head, list) 
	{
		iError = ptFontOpr->GetFontBitmap(charInfo->unicode, &tFontBitMap);
//		DBG_PRINTF("dwCode:0x%X, iXLeft:%d, iXMax:%d, actualWidth: %d", charInfo->unicode, tFontBitMap.iXLeft, tFontBitMap.iXMax, actualWidth);
		if(0 == iError)
		{
			if(ShowFontBitMapMemRect(memRect, &tFontBitMap) < 0)
			{
				DBG_PRINTF("Fail to show a word!");
				return -1;
			}
			
			tFontBitMap.iCurOriginX = tFontBitMap.iNextOriginX;
			tFontBitMap.iCurOriginY = tFontBitMap.iNextOriginY;
		}
	}
	
	// 销毁字符链表
	struct list_head *pos = chars_head.next; 
	struct list_head *tmp = NULL;
	while(pos != &chars_head)
	{
		tmp = pos->next;
		list_del(pos);
		
		charInfo = container_of(pos, CharInfo, list);
		free(charInfo);
		charInfo = NULL;
		
		pos = tmp;
	}
	
	return 0;
}

static int ParseBmpMemRect(MemRect *memRect, char *filename) // 根据bmp文件名,解析bmp内容到内存
{
	if(NULL==memRect || NULL==filename)
	{
		return -1;
	}
	
	struct stat tStat;
	
	int fdBmp = open(filename, O_RDONLY);
	if (fdBmp < 0)
	{
		printf("can't open %s \n", filename);
		return -1;
	}
	if(fstat(fdBmp, &tStat) < 0)
	{
		printf("can't get fstat \n");
		return -1;
	}
	unsigned char *pBmpMem = (unsigned char *)mmap(NULL , tStat.st_size, PROT_READ, MAP_SHARED, fdBmp, 0);
	if (pBmpMem == (unsigned char *)-1)
	{
		printf("can't mmap for bmp file \n");
		return -1;
	}
	if(close(fdBmp) < 0)
	{
		printf("Cannot close file %s \n", filename);
		return -1;
	}
	
	BITMAPFILEHEADER *pBmpFileHeader;
	BITMAPINFOHEADER *pBmpInfoHeader;
	
	pBmpFileHeader = (BITMAPFILEHEADER *)pBmpMem;
	pBmpInfoHeader = (BITMAPINFOHEADER *)(pBmpMem + sizeof(BITMAPFILEHEADER));
	
	int bmpWidth  = pBmpInfoHeader->biWidth;
	int bmpHeight = pBmpInfoHeader->biHeight;
	int bmpBpp 	  = pBmpInfoHeader->biBitCount;
	int memLength = (bmpHeight*bmpWidth*bmpBpp+7)/8;
	
//	printf("bmpWidth: %d, bmpHeight: %d, bmpBpp: %d, memLength:%d \n", 
//		bmpWidth, bmpHeight, bmpBpp, memLength);
	
	if(1 != bmpBpp)
	{
		fprintf(stderr, "Bitmap bit depth must be 1. bmpBpp: %d \n", bmpBpp);
		return -1;
	}
	
	unsigned char *pBuf = (unsigned char *)malloc(memLength);
	if(NULL == pBuf)
	{
		fprintf(stderr, "Can't malloc memory, ERROR! \n");
		return -1;
	}
	if(NULL != memRect->pBuf)
	{
		free(memRect->pBuf);
		memRect->pBuf = NULL;
		memRect->width = 0;
		memRect->height = 0;
	}
	memset(pBuf, 0, memLength);
	memRect->width 	= bmpWidth;
	memRect->height = bmpHeight;
	memRect->pBuf 	= pBuf;
	
	int realWidth = bmpWidth * bmpBpp / 8;
	int alignWidth = (realWidth + 3) & ~0x3;   /* 向4取整 */
	
	unsigned char *pDataBegin = pBmpMem + pBmpFileHeader->bfOffBits;
	unsigned char *yRow = NULL;
	unsigned char *xByte = NULL;
	int x = 0, y = 0;
	int b = 0;
	
	for (y = 0; y < bmpHeight; y++)
	{
		for(x = 0; x < bmpWidth; x++)
		{
			yRow = pDataBegin + (bmpHeight - 1 - y) * alignWidth;
			xByte = yRow + x / 8;
			b = (*xByte >> (7 - x % 8)) & 0x1;
			
			SetPixelMemRect(memRect, x, y, b); // 画点
		}
	}
	
	// 释放共享内存
	if(NULL != pBmpMem)
	{
		if(munmap(pBmpMem, tStat.st_size) < 0)
			return -1;
		pBmpMem = NULL;
	}
	
	return 0;
}

///////////////////////////////////////////////////////////////////////////
////////////////////////////Text///////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

static int CopyMemRect(Rect *dst, Rect *src, MemRect *memRect) // 默认居中, 在dst中
{
	if(NULL==dst||NULL==src||NULL==memRect)
	{
		return -1; // 参数错误
	}
	
	// 内存区域的范围scope
	Rect ms = { .x=0, .y=0, .w=memRect->width, .h=memRect->height };
	int y_src_top		= max(src->y, ms.y);				// 上
	int y_src_bottom	= min(src->y+src->h, ms.y+ms.h);	// 下
	int x_src_left		= max(src->x, ms.x);				// 左
	int x_src_right 	= min(src->x+src->w, ms.x+ms.w);	// 右
	
	int y_dst_top		= dst->y + (dst->h - src->h)/2; 	// 上
	int x_dst_left		= dst->x + (dst->w - src->w)/2; 	// 左
	
	int x_dst, y_dst;
	int x_src, y_src, val;
	
	unsigned int color = COLOR_BACKGROUND;
	
	for(y_src = y_src_top; y_src < y_src_bottom; y_src++)
	{
		y_dst = y_dst_top+(y_src-y_src_top);
		if(y_dst < WINDOW_HEIGHT && y_dst >= 0)
		{
			for(x_src = x_src_left; x_src < x_src_right; x_src++)
			{
				x_dst = x_dst_left+(x_src-x_src_left);
				if(x_dst < WINDOW_WIDTH && x_dst >= 0)
				{
					val = GetPixelMemRect(memRect, x_src, y_src);
					color = val > 0 ? COLOR_FOREGROUND : COLOR_BACKGROUND;
					g_ptDispOpr->ShowPixel(x_dst, y_dst, color);
				}
			}
		}
	}
	
	return 0;
}

static int MarqueeLine(int shift_len, Rect *dst, MemRect *memRect)
{	
	if(NULL==dst || NULL==memRect) 
	{
		return -1;
	}
	
	if(shift_len < (memRect->width - dst->w)) // 1, 区域铺满
	{
		Rect rtDst = { .x=dst->x, .y=dst->y, .w=dst->w, .h=dst->h };
		Rect rtSrc = { .x=shift_len, .y=0, .w=dst->w, .h=memRect->height };
		
		CopyMemRect(&rtDst, &rtSrc, memRect);
	}
	else if(shift_len < (memRect->width + dst->w/2 - dst->w)) // 2, 前面显示,尾部空白
	{
		Rect rtSrc = { .x=shift_len, .y=0, .w=memRect->width-shift_len, .h=memRect->height };
		Rect rtDst = { .x=dst->x, .y=dst->y, .w=memRect->width-shift_len, .h=dst->h };
		Rect rtGap = { .x=rtDst.x+rtDst.w, .y=dst->y, .w=dst->w-rtDst.w, .h=dst->h };
		
		g_ptDispOpr->FillRect(&rtGap, COLOR_BACKGROUND);
		CopyMemRect(&rtDst, &rtSrc, memRect);
	}
	else if(shift_len < memRect->width)	// 3. 前面显示, 中间空白, 尾部显示
	{
		Rect rtSrcB = { .x=shift_len, .y=0, .w=memRect->width-shift_len, .h=memRect->height }; // 前面部分
		Rect rtSrcE = { .x=0, .y=0, .w=((shift_len+dst->w)-(memRect->width+dst->w/2)), .h=memRect->height};	// 尾部部分
		
		Rect rtDstB = { .x=dst->x, .y=dst->y, .w=memRect->width-shift_len, .h=dst->h };
		Rect rtGapM = { .x=rtDstB.x+rtDstB.w, .y=rtDstB.y, .w=dst->w/2, .h=dst->h };
		Rect rtDstE = { .x=rtGapM.x+rtGapM.w, .y=rtGapM.y, .w=rtSrcE.w, .h=dst->h };
		
		CopyMemRect(&rtDstB, &rtSrcB, memRect);				
		g_ptDispOpr->FillRect(&rtGapM, COLOR_BACKGROUND);
		CopyMemRect(&rtDstE, &rtSrcE, memRect);
	}
	else if(shift_len < (memRect->width + dst->w/2)) // 4. 前面空白, 后面显示
	{
		Rect rtSrc = { .x=0, .y=0, .w=((shift_len+dst->w)-(memRect->width+dst->w/2)), .h=memRect->height };
		
		Rect rtGap = { .x=dst->x, .y=dst->y, .w=dst->w-rtSrc.w, .h=dst->h };
		Rect rtDst = { .x=rtGap.x+rtGap.w, .y=dst->y, .w=rtSrc.w, .h=dst->h };
		
		g_ptDispOpr->FillRect(&rtGap, COLOR_BACKGROUND);
		CopyMemRect(&rtDst, &rtSrc, memRect);
	}
	
	return g_ptDispOpr->UpdateRect(dst);	
}


static Rect 	g_rtLine1 		= { .x = 0, .y = 16, .w = 128, .h = 16 };
static MemRect 	g_mrLine1 		= { .width = 0, .height = 0, .pBuf = NULL };

static int 		g_line1Waiting	= 0;
static int 		g_line1Times 	= 0;

static void line1_waiting_cb(struct uloop_timeout *timeout)
{
	char *waiting[] = { ICON_WAITING_0, ICON_WAITING_1, ICON_WAITING_2, ICON_WAITING_3, ICON_WAITING_4 };
	char *icon = waiting[g_line1Waiting%(sizeof(waiting)/sizeof(*waiting))];
	
	g_ptDispOpr->FillRect(&g_rtLine1, COLOR_BACKGROUND);
	
	ParseBmpMemRect(&g_mrLine1, icon);
	
	Rect src = { .x=0, .y=0, .w=g_mrLine1.width, .h=g_mrLine1.height };
	CopyMemRect(&g_rtLine1, &src, &g_mrLine1);
	g_ptDispOpr->UpdateRect(&g_rtLine1);
	
	g_line1Waiting++;
	uloop_timeout_set(timeout, LINE1_WAITING_PERIOD);
}
static struct uloop_timeout line1_waiting_timer = {
	.cb = line1_waiting_cb
};
static void line1_twinkle_cb(struct uloop_timeout *timeout)
{
	static int twinkle = 0;
	
	if(1 == twinkle%2)
	{
		Rect rtMem = { .x=0, .y=0, .w=g_mrLine1.width, .h=g_mrLine1.height };
		CopyMemRect(&g_rtLine1, &rtMem, &g_mrLine1);
	}
	else
	{
		g_ptDispOpr->FillRect(&g_rtLine1, COLOR_BACKGROUND);
	}
	
	g_ptDispOpr->UpdateRect(&g_rtLine1);
	
	twinkle++;
	uloop_timeout_set(timeout, LINE1_TWINKLE_PERIOD);
}
static struct uloop_timeout line1_twinkle_timer = {
	.cb = line1_twinkle_cb
};
static void line1_marquee_cb(struct uloop_timeout *timeout)
{
	int marquee_len = (g_line1Times*SHIFT_STEP_PIXEL)%(g_mrLine1.width+g_rtLine1.w/2);
	MarqueeLine(marquee_len, &g_rtLine1, &g_mrLine1);
	
	g_line1Times++;
	uloop_timeout_set(timeout, SHIFT_PERIOD_MSEC);
}
static struct uloop_timeout line1_marquee_timer = {
	.cb = line1_marquee_cb
};

int ShowLine1Waiting(void)				// 在语音搜索时,等待响应图标
{
	uloop_timeout_cancel(&line1_waiting_timer);
	uloop_timeout_cancel(&line1_twinkle_timer);
	uloop_timeout_cancel(&line1_marquee_timer);
	
	g_line1Waiting = 0;
	uloop_timeout_set(&line1_waiting_timer, TIMER_ALIVE_UNIT);	
	
	return 0;
}

int ShowLine1Twinkle(char *szLine)		// 第1行显示文字, 闪烁
{
	if(!szLine)
		return -1;
	
	uloop_timeout_cancel(&line1_waiting_timer);
	uloop_timeout_cancel(&line1_twinkle_timer);
	uloop_timeout_cancel(&line1_marquee_timer);
	
	g_ptDispOpr->FillRect(&g_rtLine1, COLOR_BACKGROUND);
	
	ParseStringMemRect(&g_mrLine1, szLine, DEFAULT_FONT_SIZE);
	
	Rect src = { .x=0, .y=0, .w=g_mrLine1.width, .h=g_mrLine1.height };
	CopyMemRect(&g_rtLine1, &src, &g_mrLine1);
	g_ptDispOpr->UpdateRect(&g_rtLine1);
	
	uloop_timeout_set(&line1_twinkle_timer, LINE1_TWINKLE_PERIOD);
	
	return 0;
}

int ShowLine1(char *szLine)			// 第1行显示文字
{
	if(!szLine)
		return -1;
	
	uloop_timeout_cancel(&line1_waiting_timer);
	uloop_timeout_cancel(&line1_twinkle_timer);
	uloop_timeout_cancel(&line1_marquee_timer);
	
	g_ptDispOpr->FillRect(&g_rtLine1, COLOR_BACKGROUND);
	
	ParseStringMemRect(&g_mrLine1, szLine, DEFAULT_FONT_SIZE);
	
	if(g_mrLine1.width > g_rtLine1.w)
	{
		g_line1Times = 0;
		uloop_timeout_set(&line1_marquee_timer, SHIFT_PERIOD_MSEC);
	}
	else
	{
		Rect src = { .x=0, .y=0, .w=g_mrLine1.width, .h=g_mrLine1.height };
		CopyMemRect(&g_rtLine1, &src, &g_mrLine1);
		g_ptDispOpr->UpdateRect(&g_rtLine1);
	}
	
	return 0;
}

static Rect 	g_rtLine2 		= { .x = 0, .y = 32, .w = 128, .h = 16 };
static MemRect 	g_mrLine2 		= { .width = 0, .height = 0, .pBuf = NULL };
static int 		g_line2Times 	= 0;

static void line2_marquee_cb(struct uloop_timeout *timeout)
{	
	int shift_len = (g_line2Times*SHIFT_STEP_PIXEL)%(g_mrLine2.width+g_rtLine2.w/2);
	MarqueeLine(shift_len, &g_rtLine2, &g_mrLine2);
	
	g_line2Times++;
	uloop_timeout_set(timeout, SHIFT_PERIOD_MSEC);
}
static struct uloop_timeout line2_marquee_timer = {
	.cb = line2_marquee_cb
};
int ShowLine2(char *szLine)
{
	if(!szLine)
		return -1;
	
	uloop_timeout_cancel(&line2_marquee_timer); // 取消 定时器
	
	g_ptDispOpr->FillRect(&g_rtLine2, COLOR_BACKGROUND);
	
	ParseStringMemRect(&g_mrLine2, szLine, DEFAULT_FONT_SIZE);	
	
	if(g_mrLine2.width > g_rtLine2.w)
	{
		g_line2Times = 0;
		uloop_timeout_set(&line2_marquee_timer, SHIFT_PERIOD_MSEC);
	}
	else
	{
		Rect src = { .x=0, .y=0, .w=g_mrLine2.width, .h=g_mrLine2.height };
		CopyMemRect(&g_rtLine2, &src, &g_mrLine2);
		g_ptDispOpr->UpdateRect(&g_rtLine2);
	}
	
	return 0;
}

static Rect 	g_rtLine3 		= { .x = 0, .y = 48, .w = 128, .h = 16 };
static MemRect 	g_mrLine3 		= { .width = 0, .height = 0, .pBuf = NULL };
static int 		g_line3Times	= 0;

static void line3_marquee_cb(struct uloop_timeout *timeout)
{
	int shift_len = (g_line3Times*SHIFT_STEP_PIXEL)%(g_mrLine3.width+g_rtLine3.w/2);
	MarqueeLine(shift_len, &g_rtLine3, &g_mrLine3);
	
	g_line3Times++;
	uloop_timeout_set(timeout, SHIFT_PERIOD_MSEC);
}
static struct uloop_timeout line3_marquee_timer = {
	.cb = line3_marquee_cb
};
int ShowLine3(char *szLine)
{
//	DBG_PRINTF("szLine: %s \n", szLine);
	
	if(!szLine)
		return -1;
	
	uloop_timeout_cancel(&line3_marquee_timer); // 取消 定时器
	g_ptDispOpr->FillRect(&g_rtLine3, COLOR_BACKGROUND);
	
	ParseStringMemRect(&g_mrLine3, szLine, DEFAULT_FONT_SIZE);	
	
	if(g_mrLine3.width > g_rtLine3.w)
	{
		g_line3Times = 0;
		uloop_timeout_set(&line3_marquee_timer, SHIFT_PERIOD_MSEC);
	}
	else
	{
		Rect src = { .x=0, .y=0, .w=g_mrLine3.width, .h=g_mrLine3.height };
		CopyMemRect(&g_rtLine3, &src, &g_mrLine3);
		g_ptDispOpr->UpdateRect(&g_rtLine3);		
	}
	
	return 0;
}

static Rect 	g_rtLine0 		= { .x = 0, .y = 0, .w = 128, .h = 16 };
static MemRect 	g_mrLine0 		= { .width = 0, .height = 0, .pBuf = NULL };

int ShowLine0123(char *line0, char *line1, char *line2, char *line3) // for gui_cli
{
	if(!line0 || !line1 || !line2 || !line3)
		return -1;
	
	ParseStringMemRect(&g_mrLine0, line0, DEFAULT_FONT_SIZE);
	ParseStringMemRect(&g_mrLine1, line1, DEFAULT_FONT_SIZE);
	ParseStringMemRect(&g_mrLine2, line2, DEFAULT_FONT_SIZE);
	ParseStringMemRect(&g_mrLine3, line3, DEFAULT_FONT_SIZE);
	
	Rect src0 = { .x=0, .y=0, .w=g_mrLine0.width, .h=g_mrLine0.height };
	Rect src1 = { .x=0, .y=0, .w=g_mrLine1.width, .h=g_mrLine1.height };
	Rect src2 = { .x=0, .y=0, .w=g_mrLine2.width, .h=g_mrLine2.height };
	Rect src3 = { .x=0, .y=0, .w=g_mrLine3.width, .h=g_mrLine3.height };
	CopyMemRect(&g_rtLine0, &src0, &g_mrLine0);
	CopyMemRect(&g_rtLine1, &src1, &g_mrLine1);
	CopyMemRect(&g_rtLine2, &src2, &g_mrLine2);
	CopyMemRect(&g_rtLine3, &src3, &g_mrLine3);
	
	Rect FullRect = { .x = 0, .y = 0, .w = WINDOW_WIDTH, .h = WINDOW_HEIGHT };	
	g_ptDispOpr->UpdateRect(&FullRect);
	
	return 0;
}

static Rect 	g_rtBmp 		= { .x = 0, .y = 16, .w = 128, .h = 48 };
static MemRect 	g_mrBmp 		= { .width = 0, .height = 0, .pBuf = NULL };

int ShowBmp(Rect *dst, char *filename)
{
	if(!filename)
		return -1;
	
	if(dst)
	{
		g_rtBmp.x = dst->x;
		g_rtBmp.y = dst->y;
		g_rtBmp.w = dst->w;
		g_rtBmp.h = dst->h;		
	}
	else
	{
		g_rtBmp.x = 0;
		g_rtBmp.y = 16;
		g_rtBmp.w = 128;
		g_rtBmp.h = 48;		
	}
	
	g_ptDispOpr->FillRect(&g_rtBmp, COLOR_BACKGROUND);
	
	ParseBmpMemRect(&g_mrBmp, filename);
	
//	DBG_PRINTF("Rect(x:%d, y:%d, w:%d, h:%d), Bmp(w:%d, h:%d) \n",
//		g_rtBmp.x,
//		g_rtBmp.y,
//		g_rtBmp.w,
//		g_rtBmp.h,
//		g_mrBmp.width,
//		g_mrBmp.height);
//	ShowMemRect(&g_mrBmp);
	
	Rect src = { .x=0, .y=0, .w=g_mrBmp.width, .h=g_mrBmp.height };
	CopyMemRect(&g_rtBmp, &src, &g_mrBmp);
	
	return g_ptDispOpr->UpdateRect(&g_rtBmp);
}

int ClearAllText(void)
{
	uloop_timeout_cancel(&line1_waiting_timer);
	uloop_timeout_cancel(&line1_twinkle_timer);
	uloop_timeout_cancel(&line1_marquee_timer);
	uloop_timeout_cancel(&line2_marquee_timer);
	uloop_timeout_cancel(&line3_marquee_timer);
	
	Rect dst = { .x = 0, .y = 16, .w = 128, .h = 48 };
	g_ptDispOpr->FillRect(&dst, COLOR_BACKGROUND);
	g_ptDispOpr->UpdateRect(&dst);
	
	return 0;
}

int UpdateRect(Rect *rect)
{
	if(!g_ptDispOpr)
		return -1;
	
	return g_ptDispOpr->UpdateRect(rect);
}
///////////////////////////////////////////////////////////////////////////
////////////////////////////StatusBar//////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
void StatusEnable(bool enable)
{
	g_ptDispOpr->SetEnableS(enable);
}

static int CopyMemRectS(Rect *dst, Rect *src, MemRect *memRect) // 默认居中, 在dst中
{
	if(NULL==dst||NULL==src||NULL==memRect)
	{
		return -1; // 参数错误
	}
	
	// 内存区域的范围scope
	Rect ms = { .x=0, .y=0, .w=memRect->width, .h=memRect->height };
	int y_src_top		= max(src->y, ms.y);				// 上
	int y_src_bottom	= min(src->y+src->h, ms.y+ms.h);	// 下
	int x_src_left		= max(src->x, ms.x);				// 左
	int x_src_right 	= min(src->x+src->w, ms.x+ms.w);	// 右
	
	int y_dst_top		= dst->y + (dst->h - src->h)/2; 	// 上
	int x_dst_left		= dst->x + (dst->w - src->w)/2; 	// 左
	
	int x_dst, y_dst;
	int x_src, y_src, val;
	
	unsigned int color = COLOR_BACKGROUND;
	
	for(y_src = y_src_top; y_src < y_src_bottom; y_src++)
	{
		y_dst = y_dst_top+(y_src-y_src_top);
		if(y_dst < WINDOW_HEIGHT && y_dst >= 0)
		{
			for(x_src = x_src_left; x_src < x_src_right; x_src++)
			{
				x_dst = x_dst_left+(x_src-x_src_left);
				if(x_dst < WINDOW_WIDTH && x_dst >= 0)
				{
					val = GetPixelMemRect(memRect, x_src, y_src);
					color = val > 0 ? COLOR_FOREGROUND : COLOR_BACKGROUND;
					g_ptDispOpr->ShowPixelS(x_dst, y_dst, color);
				}
			}
		}
	}
	
	return 0;
}


static Rect 	g_rtBattery   	= { .x = 0, .y = 0, .w = 16, .h = 16 };	// 最左边
static MemRect 	g_mrBattery 	= { .width = 0, .height = 0, .pBuf = NULL };

static void battery_twinkle_cb(struct uloop_timeout *timeout)
{
	static int twinkle = 0;
	
	if(0 == twinkle % 2)
	{
		g_ptDispOpr->FillRectS(&g_rtBattery, COLOR_BACKGROUND);
	}
	else
	{
		Rect src = { .x=0, .y=0, .w=g_mrBattery.width, .h=g_mrBattery.height };
		CopyMemRectS(&g_rtBattery, &src, &g_mrBattery);	
	}
	
	g_ptDispOpr->UpdateRectS(&g_rtBattery);
	
	twinkle++;
	uloop_timeout_set(timeout, ICON_BATTERY_TWINKLE_PERIOD);
}
static struct uloop_timeout battery_twinkle_timer = { // 当前设备电量,低于10%时,该图标闪动
	.cb = battery_twinkle_cb
};
int ShowBattery(int percent, bool charge)
{
	if(percent<0 || percent>100)
		return -1;
	
	int rc = -1;
	char *ic_bat = NULL;
	
	uloop_timeout_cancel(&battery_twinkle_timer);
	
	rc = g_ptDispOpr->FillRectS(&g_rtBattery, COLOR_BACKGROUND);
	if(rc < 0)
		return -1;
	
	ic_bat = get_battery_icon(percent, charge);
	rc = ParseBmpMemRect(&g_mrBattery, ic_bat);
	if(rc < 0)
		return -1;
	
	Rect src = { .x=0, .y=0, .w=g_mrBattery.width, .h=g_mrBattery.height };
	CopyMemRectS(&g_rtBattery, &src, &g_mrBattery);
	g_ptDispOpr->UpdateRectS(&g_rtBattery);
	
	if(!charge && percent<BATTERY_LOW_LEVEL)
		uloop_timeout_set(&battery_twinkle_timer, TIMER_ALIVE_UNIT);
	
	return 0;
}


static Rect 	g_rtWifi = { .x = 19, .y = 0, .w = 16, .h = 16 };
static MemRect 	g_mrWifi = { .width = 0, .height = 0, .pBuf = NULL };

static void wifi_twinkle_cb(struct uloop_timeout *timeout)
{
	static int twinkle = 0;
	if(0 == twinkle%2)
	{
		// 内存区域的范围scope
		Rect rtMem = { .x=0, .y=0, .w=g_mrWifi.width, .h=g_mrWifi.height };		
		CopyMemRectS(&g_rtWifi, &rtMem, &g_mrWifi);
		g_ptDispOpr->UpdateRectS(&g_rtWifi);
	}
	else
	{
		g_ptDispOpr->FillRectS(&g_rtWifi, COLOR_BACKGROUND);
		g_ptDispOpr->UpdateRectS(&g_rtWifi);
	}
	
	twinkle++;
	uloop_timeout_set(timeout, ICON_WIFI_TWINKLE_PERIOD);
}
static struct uloop_timeout wifi_twinkle_timer = { // WIFI图标闪烁
	.cb = wifi_twinkle_cb
};
int ShowWifi(int state, bool twinkle)
{
	if(-1!=state&&0!=state&&1!=state&&2!=state&&3!=state)
		return -1;
	
	char *ic_wifi[] = { ICON_WIFI_FAIL, ICON_WIFI_WEAKER, ICON_WIFI_WEAK, ICON_WIFI_OK };
	
	uloop_timeout_cancel(&wifi_twinkle_timer); // 取消 定时器
	
	if(state < 0) // 隐藏
	{
		g_ptDispOpr->FillRectS(&g_rtWifi, COLOR_BACKGROUND);
	}
	else
	{
		ParseBmpMemRect(&g_mrWifi, ic_wifi[state]);
		
		// 内存区域的范围scope
		Rect rtMem = { .x=0, .y=0, .w=g_mrWifi.width, .h=g_mrWifi.height };		
		CopyMemRectS(&g_rtWifi, &rtMem, &g_mrWifi);
		g_ptDispOpr->UpdateRectS(&g_rtWifi);
		
		if(twinkle) // 闪烁
			uloop_timeout_set(&wifi_twinkle_timer, ICON_WIFI_TWINKLE_PERIOD);
	}
	
	return 0;
}


static Rect 	g_rtBluetooth  	= { .x = 19, .y = 0, .w = 16, .h = 16 };
static MemRect 	g_mrBluetooth 	= { .width = 0, .height = 0, .pBuf = NULL };

int ShowBluetooth(int state)
{
	uloop_timeout_cancel(&wifi_twinkle_timer); // 取消 定时器
	
	g_ptDispOpr->FillRectS(&g_rtBluetooth, COLOR_BACKGROUND);
	
	if(state == BT_STATE_CONNECTED)
	{
		ParseBmpMemRect(&g_mrBluetooth, ICON_BLUETOOTH_CN);
		Rect rtMem = { .x=0, .y=0, .w=g_mrBluetooth.width, .h=g_mrBluetooth.height };
		CopyMemRectS(&g_rtBluetooth, &rtMem, &g_mrBluetooth);
	}
	else if(state == BT_STATE_STARTED)
	{
		ParseBmpMemRect(&g_mrBluetooth, ICON_BLUETOOTH);
		Rect rtMem = { .x=0, .y=0, .w=g_mrBluetooth.width, .h=g_mrBluetooth.height };
		CopyMemRectS(&g_rtBluetooth, &rtMem, &g_mrBluetooth);
	}
	
	return g_ptDispOpr->UpdateRectS(&g_rtBluetooth);
}

static Rect 	g_rtLinein  = { .x = 19, .y = 0, .w = 16, .h = 16 };
static MemRect 	g_mrLinein 	= { .width = 0, .height = 0, .pBuf = NULL };

int ShowLinein(int show) 		// show>0, 显示; show<0, 隐藏
{
	uloop_timeout_cancel(&wifi_twinkle_timer); // 取消 定时器
	
	if(show < 0)
	{
		g_ptDispOpr->FillRectS(&g_rtLinein, COLOR_BACKGROUND);
	}
	else
	{
		ParseBmpMemRect(&g_mrLinein, ICON_LINEIN);
		
		// 内存区域的范围scope
		Rect rtMem = { .x=0, .y=0, .w=g_mrLinein.width, .h=g_mrLinein.height };
		CopyMemRectS(&g_rtLinein, &rtMem, &g_mrLinein);
	}
	
	return g_ptDispOpr->UpdateRectS(&g_rtLinein);
}


static Rect  dynamic_icon_fullrect 	= { .x=38, .y=0, .w=48, .h=16 };
static Rect  dynamic_icon_pos[3] 	= {{.x=38, .y=0, .w=16, .h=16}, {.x=54, .y=0, .w=16, .h=16}, { .x=70, .y=0, .w=16, .h=16}};
static char *dynamic_icon_name[4] 	= { NULL, NULL, NULL, NULL };

static MemRect g_mrPlayer 			= { .width = 0, .height = 0, .pBuf = NULL };
static MemRect g_mrAlarm 			= { .width = 0, .height = 0, .pBuf = NULL };
static MemRect g_mrDown 			= { .width = 0, .height = 0, .pBuf = NULL };
static MemRect g_mrLock 			= { .width = 0, .height = 0, .pBuf = NULL };

static int sort_dynamic_icons(void) // 重新布局图标
{
	int i = 0;
	int size = sizeof(dynamic_icon_name)/sizeof(dynamic_icon_name[0]);
	int count = 0;
	bool player = false;
	int pi = 0;
	char *tmp = NULL;
	
	for(i = 0; i < size; i++) // 将空位移到最后
	{
		if(NULL != dynamic_icon_name[i])
		{
			dynamic_icon_name[count] = dynamic_icon_name[i];
			if(count < i) dynamic_icon_name[i] = NULL;
			count++;
		}
	}
	
	for(i = 0; i < count; i++)
	{
		if(0 == strcmp(dynamic_icon_name[i], ICON_PLAYER_NAME))
		{
			player = true;
			pi = i;
			break;
		}
	}
	
	if(player)
	{
		if(count < size) // 不满时 位于第1个
		{
			if(0 != pi)
			{
				tmp = dynamic_icon_name[0];
				dynamic_icon_name[0] = dynamic_icon_name[pi];
				dynamic_icon_name[pi] = tmp;
			}
		}
		else // 满时, 位于最后一个
		{
			if(size-1 != pi)
			{
				tmp = dynamic_icon_name[size-1];
				dynamic_icon_name[size-1] = dynamic_icon_name[pi];
				dynamic_icon_name[pi] = tmp;				
			}
		}
	}
	
	return 0;
}
static int relayout_dynamic_icons(void) // 重新布局图标
{
	int i = 0;
	int size = sizeof(dynamic_icon_pos)/sizeof(dynamic_icon_pos[0]);
	Rect srcMem;	
	Rect *dst;
	MemRect *memRect = NULL;
	
	g_ptDispOpr->FillRectS(&dynamic_icon_fullrect, COLOR_BACKGROUND);
	
	for(i = 0; i < size && NULL != dynamic_icon_name[i]; i++)
	{
		if(0==strcmp(dynamic_icon_name[i], ICON_PLAYER_NAME))
		{
			memRect = &g_mrPlayer;
		}
		else if(0==strcmp(dynamic_icon_name[i], ICON_ALARM_NAME))
		{
			memRect = &g_mrAlarm;
		}
		else if(0==strcmp(dynamic_icon_name[i], ICON_DOWN_NAME))
		{
			memRect = &g_mrDown;
		}
		else if(0==strcmp(dynamic_icon_name[i], ICON_LOCK_NAME))
		{
			memRect = &g_mrLock;			
		}
		
		srcMem.x = srcMem.y = 0;
		srcMem.w = memRect->width;
		srcMem.h = memRect->height;
		dst = &dynamic_icon_pos[i];
		
		CopyMemRectS(dst, &srcMem, memRect);
	}
	
	return g_ptDispOpr->UpdateRectS(&dynamic_icon_fullrect);
}


int ShowDown(int show)
{
	int size = sizeof(dynamic_icon_name)/sizeof(dynamic_icon_name[0]);
	int down_index = 0;
	
	for(down_index = 0; down_index < size; down_index++)
	{
		if(NULL==dynamic_icon_name[down_index] 
		|| 0==strcmp(dynamic_icon_name[down_index], ICON_DOWN_NAME))
		{
			dynamic_icon_name[down_index] = ICON_DOWN_NAME;
			break;
		}
	}
	
	if(show > 0)
	{
		ParseBmpMemRect(&g_mrDown, ICON_DOWN);
	}
	else
	{
		dynamic_icon_name[down_index] = NULL;
	}
	
	sort_dynamic_icons();
	relayout_dynamic_icons();
	
	return 0;
}

int ShowLock(int show)
{
	int size = sizeof(dynamic_icon_name)/sizeof(dynamic_icon_name[0]);
	int lock_index = 0;
	
	for(lock_index = 0; lock_index < size; lock_index++)
	{
		if(NULL==dynamic_icon_name[lock_index] 
		|| 0==strcmp(dynamic_icon_name[lock_index], ICON_LOCK_NAME))
		{
			dynamic_icon_name[lock_index] = ICON_LOCK_NAME;
			break;
		}
	}
	
	if(show > 0)
	{
		ParseBmpMemRect(&g_mrLock, ICON_LOCK);
	}
	else
	{
		dynamic_icon_name[lock_index] = NULL;
	}
	
	sort_dynamic_icons();
	relayout_dynamic_icons();
	
	return 0;
}

int ShowAlarm(int show)
{
	int size = sizeof(dynamic_icon_name)/sizeof(dynamic_icon_name[0]);
	int alarm_index = 0;
	
	for(alarm_index = 0; alarm_index < size; alarm_index++)
	{
		if(NULL==dynamic_icon_name[alarm_index] 
		|| 0==strcmp(dynamic_icon_name[alarm_index], ICON_ALARM_NAME))
		{
			dynamic_icon_name[alarm_index] = ICON_ALARM_NAME;
			break;
		}
	}
	
	if(show > 0)
	{
		ParseBmpMemRect(&g_mrAlarm, ICON_ALARM);
	}
	else
	{
		dynamic_icon_name[alarm_index] = NULL;
	}
	
	sort_dynamic_icons();
	relayout_dynamic_icons();
	
	return 0;
}

int ShowPlayerState(int mpd_state) // 显示播放器的状态, 显示 播放/暂停. 停止时不显示
{
	int size = sizeof(dynamic_icon_name)/sizeof(dynamic_icon_name[0]);
	int player_index = 0;
	
	for(player_index = 0; player_index < size; player_index++)
	{
		if(NULL==dynamic_icon_name[player_index] 
		|| 0==strcmp(dynamic_icon_name[player_index], ICON_PLAYER_NAME))
		{
			dynamic_icon_name[player_index] = ICON_PLAYER_NAME;
			break;
		}
	}
	
	switch(mpd_state)
	{
	case PLAYER_STATE_PLAY:
		{
			ParseBmpMemRect(&g_mrPlayer, ICON_PLAYER_PLAY);
		}
		break;
	case PLAYER_STATE_PAUSE:
		{
			ParseBmpMemRect(&g_mrPlayer, ICON_PLAYER_PAUSE);
		}
		break;
	default:
		{
			dynamic_icon_name[player_index] = NULL;
		}
		break;
	}
	
	sort_dynamic_icons();
	relayout_dynamic_icons();
	
	return 0;
}

static Rect 	g_rtTime	= { .x = 88, .y = 0, .w = 40, .h = 16 }; // 最右边
static MemRect 	g_mrTime	= { .width = 0, .height = 0, .pBuf = NULL };

static void update_time_cb(struct uloop_timeout *timeout)
{
	static int last_hour = 0;
	static int last_minute = 0;
	
	int hour = 0;
	int minute = 0;
	
	getLocaltime(&hour, &minute);
	
	if(last_hour!=hour || last_minute!=minute) // 时分改变被改变,才更新时间
	{
//		DBG_PRINTF("Time has changed");
		last_hour = hour;
		last_minute = minute;
		
		char *icon_num[] = { ICON_NUM_0, ICON_NUM_1, ICON_NUM_2, ICON_NUM_3, ICON_NUM_4,
					ICON_NUM_5, ICON_NUM_6, ICON_NUM_7, ICON_NUM_8, ICON_NUM_9 };	
		char *icon_colon = ICON_COLON;
		
		Rect rtH2 = { .x=g_rtTime.x+0*8, .y=g_rtTime.y, .w=g_rtTime.w/5, .h=g_rtTime.h};
		Rect rtH1 = { .x=g_rtTime.x+1*8, .y=g_rtTime.y, .w=g_rtTime.w/5, .h=g_rtTime.h};
		Rect rtCl = { .x=g_rtTime.x+2*8, .y=g_rtTime.y, .w=g_rtTime.w/5, .h=g_rtTime.h};
		Rect rtM2 = { .x=g_rtTime.x+3*8, .y=g_rtTime.y, .w=g_rtTime.w/5, .h=g_rtTime.h};
		Rect rtM1 = { .x=g_rtTime.x+4*8, .y=g_rtTime.y, .w=g_rtTime.w/5, .h=g_rtTime.h};
		
		ParseBmpMemRect(&g_mrTime, icon_num[hour/10]);
		Rect msH2 = { .x=0, .y=0, .w=g_mrTime.width, .h=g_mrTime.height };
		CopyMemRectS(&rtH2, &msH2, &g_mrTime);
		
		ParseBmpMemRect(&g_mrTime, icon_num[hour%10]);	
		Rect msH1 = { .x=0, .y=0, .w=g_mrTime.width, .h=g_mrTime.height };
		CopyMemRectS(&rtH1, &msH1, &g_mrTime);
		
		ParseBmpMemRect(&g_mrTime, icon_colon);
		Rect msC1 = { .x=0, .y=0, .w=g_mrTime.width, .h=g_mrTime.height };
		CopyMemRectS(&rtCl, &msC1, &g_mrTime);
		
		ParseBmpMemRect(&g_mrTime, icon_num[minute/10]);
		Rect msM2 = { .x=0, .y=0, .w=g_mrTime.width, .h=g_mrTime.height };
		CopyMemRectS(&rtM2, &msM2, &g_mrTime);
		
		ParseBmpMemRect(&g_mrTime, icon_num[minute%10]);
		Rect msM1 = { .x=0, .y=0, .w=g_mrTime.width, .h=g_mrTime.height };
		CopyMemRectS(&rtM1, &msM1, &g_mrTime);
		
		g_ptDispOpr->UpdateRectS(&g_rtTime);
	}
	
	uloop_timeout_set(timeout, TIME_UPDATE_PERIOD);
}

static struct uloop_timeout update_time_timer = {
	.cb = update_time_cb
};
int ShowTime(int show)			// 显示时间: disp>0, 显示时间, disp<=0, 隐藏时间
{
	uloop_timeout_cancel(&update_time_timer); // 取消 定时器
	g_ptDispOpr->FillRectS(&g_rtTime, COLOR_BACKGROUND);
	
	if(show > 0)
		uloop_timeout_set(&update_time_timer, TIMER_ALIVE_UNIT);
	
	return 0;
}

///////////////////////////////////////////////////////////////////////////
////////////////////////////Toast//////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
static int CopyMemRectT(Rect *dst, Rect *src, MemRect *memRect) // 默认居中, 在dst中
{
	if(NULL==dst||NULL==src||NULL==memRect)
	{
		return -1; // 参数错误
	}
	
	// 内存区域的范围scope
	Rect ms = { .x=0, .y=0, .w=memRect->width, .h=memRect->height };
	int y_src_top		= max(src->y, ms.y);				// 上
	int y_src_bottom	= min(src->y+src->h, ms.y+ms.h);	// 下
	int x_src_left		= max(src->x, ms.x);				// 左
	int x_src_right 	= min(src->x+src->w, ms.x+ms.w);	// 右
	
	int y_dst_top		= dst->y + (dst->h - src->h)/2; 	// 上
	int x_dst_left		= dst->x + (dst->w - src->w)/2; 	// 左
	
	int x_dst, y_dst;
	int x_src, y_src, val;
	
	unsigned int color = COLOR_BACKGROUND;
	
	for(y_src = y_src_top; y_src < y_src_bottom; y_src++)
	{
		y_dst = y_dst_top+(y_src-y_src_top);
		if(y_dst < WINDOW_HEIGHT && y_dst >= 0)
		{
			for(x_src = x_src_left; x_src < x_src_right; x_src++)
			{
				x_dst = x_dst_left+(x_src-x_src_left);
				if(x_dst < WINDOW_WIDTH && x_dst >= 0)
				{
					val = GetPixelMemRect(memRect, x_src, y_src);
					color = val > 0 ? COLOR_FOREGROUND : COLOR_BACKGROUND;
					g_ptDispOpr->ShowPixelT(x_dst, y_dst, color);
				}
			}
		}
	}
	
	return 0;
}
static int MarqueeLineT(int shift_len, Rect *dst, MemRect *memRect)
{
	if(!dst || !memRect) 
		return -1;
	
	if(shift_len < (memRect->width - dst->w)) // 1, 区域铺满
	{
		Rect rtDst = { .x=dst->x, .y=dst->y, .w=dst->w, .h=dst->h };
		Rect rtSrc = { .x=shift_len, .y=0, .w=dst->w, .h=memRect->height };
		
		CopyMemRectT(&rtDst, &rtSrc, memRect);
	}
	else if(shift_len < (memRect->width + dst->w/2 - dst->w)) // 2, 前面显示,尾部空白
	{
		Rect rtSrc = { .x=shift_len, .y=0, .w=memRect->width-shift_len, .h=memRect->height };
		Rect rtDst = { .x=dst->x, .y=dst->y, .w=memRect->width-shift_len, .h=dst->h };
		Rect rtGap = { .x=rtDst.x+rtDst.w, .y=dst->y, .w=dst->w-rtDst.w, .h=dst->h };
		
		g_ptDispOpr->FillRectT(&rtGap, COLOR_BACKGROUND);
		CopyMemRectT(&rtDst, &rtSrc, memRect);
	}
	else if(shift_len < memRect->width)	// 3. 前面显示, 中间空白, 尾部显示
	{
		Rect rtSrcB = { .x=shift_len, .y=0, .w=memRect->width-shift_len, .h=memRect->height }; // 前面部分
		Rect rtSrcE = { .x=0, .y=0, .w=((shift_len+dst->w)-(memRect->width+dst->w/2)), .h=memRect->height};	// 尾部部分
		
		Rect rtDstB = { .x=dst->x, .y=dst->y, .w=memRect->width-shift_len, .h=dst->h };
		Rect rtGapM = { .x=rtDstB.x+rtDstB.w, .y=rtDstB.y, .w=dst->w/2, .h=dst->h };
		Rect rtDstE = { .x=rtGapM.x+rtGapM.w, .y=rtGapM.y, .w=rtSrcE.w, .h=dst->h };
		
		CopyMemRectT(&rtDstB, &rtSrcB, memRect);		
		g_ptDispOpr->FillRectT(&rtGapM, COLOR_BACKGROUND);
		CopyMemRectT(&rtDstE, &rtSrcE, memRect);
	}
	else if(shift_len < (memRect->width + dst->w/2)) // 4. 前面空白, 后面显示
	{
		Rect rtSrc = { .x=0, .y=0, .w=((shift_len+dst->w)-(memRect->width+dst->w/2)), .h=memRect->height };
		
		Rect rtGap = { .x=dst->x, .y=dst->y, .w=dst->w-rtSrc.w, .h=dst->h };
		Rect rtDst = { .x=rtGap.x+rtGap.w, .y=dst->y, .w=rtSrc.w, .h=dst->h };
		
		g_ptDispOpr->FillRectT(&rtGap, COLOR_BACKGROUND);
		CopyMemRectT(&rtDst, &rtSrc, memRect);
	}
	
	return g_ptDispOpr->UpdateRectT(dst);
}

static Rect 	g_rtToast 		= { .x = 0, .y = 16, .w = 128, .h = 48 };
static MemRect 	g_mrToast 		= { .width = 0, .height = 0, .pBuf = NULL };
static int 		g_toastTimes 	= 0;

static void toast_text_cb(struct uloop_timeout *timeout)
{
	int shift_len = (g_toastTimes*SHIFT_STEP_PIXEL)%(g_mrToast.width+g_rtToast.w/2);
	MarqueeLineT(shift_len, &g_rtToast, &g_mrToast);
	
	g_toastTimes++;
	uloop_timeout_set(timeout, SHIFT_PERIOD_MSEC);
}
static struct uloop_timeout toast_text_timer = {
	.cb = toast_text_cb
};
static void toast_duration_cb(struct uloop_timeout *timeout)
{
	uloop_timeout_cancel(&toast_text_timer);
	g_ptDispOpr->SetEnableT(false);
	
	Rect FullRect = { .x = 0, .y = 0, .w = WINDOW_WIDTH, .h = WINDOW_HEIGHT };
	g_ptDispOpr->UpdateRect(&FullRect);
}
static struct uloop_timeout toast_duration_timer = {
	.cb = toast_duration_cb
};

int Toast(Rect *dst, char *tips, int fontsize, int duration)
{
	if(!tips)
		return -1;
	
	if(dst)
	{
		g_rtToast.x = dst->x;
		g_rtToast.y = dst->y;
		g_rtToast.w = dst->w;
		g_rtToast.h = dst->h;		
	}
	else // dst为空, 默认为123行
	{
		g_rtToast.x = 0;
		g_rtToast.y = 16;
		g_rtToast.w = 128;
		g_rtToast.h = 48;		
	}
	
	uloop_timeout_cancel(&toast_text_timer);
	uloop_timeout_cancel(&toast_duration_timer);
	
	int ret = g_ptDispOpr->FillRectT(&g_rtToast, COLOR_BACKGROUND);
	if(0 != ret)
		return -1;
	
	ret = ParseStringMemRect(&g_mrToast, tips, fontsize);
	if(ret < 0)
		return -1;
	
	g_ptDispOpr->SetEnableT(true);
	
	if(g_mrToast.width > g_rtToast.w)
	{
		g_toastTimes = 0;
		uloop_timeout_set(&toast_text_timer, SHIFT_PERIOD_MSEC);
	}
	else
	{
		Rect src = { .x=0, .y=0, .w=g_mrToast.width, .h=g_mrToast.height };
		
		CopyMemRectT(&g_rtToast, &src, &g_mrToast);
		g_ptDispOpr->UpdateRectT(&g_rtToast);
	}
	
	if(duration > 0)
		uloop_timeout_set(&toast_duration_timer, duration);
	
	return 0;
}

static Rect 	g_rtLn0 		= { .x = 0, .y = 0, .w = 128, .h = 16 };
static Rect 	g_rtLn1 		= { .x = 0, .y = 16, .w = 128, .h = 16 };
static Rect 	g_rtLn2 		= { .x = 0, .y = 32, .w = 128, .h = 16 };
static Rect 	g_rtLn3 		= { .x = 0, .y = 48, .w = 128, .h = 16 };
static MemRect 	g_mrTLn0 		= { .width = 0, .height = 0, .pBuf = NULL };
static MemRect 	g_mrTLn1 		= { .width = 0, .height = 0, .pBuf = NULL };
static MemRect 	g_mrTLn2 		= { .width = 0, .height = 0, .pBuf = NULL };
static MemRect 	g_mrTLn3 		= { .width = 0, .height = 0, .pBuf = NULL };

int Toast123(char *ln1, char *ln2, char *ln3, int dur)
{
	if(!ln1 || !ln2 || !ln3)
		return -1;
	
	uloop_timeout_cancel(&toast_text_timer);
	uloop_timeout_cancel(&toast_duration_timer);
	
	Rect full_rect = { .x = 0, .y = 0, .w = WINDOW_WIDTH, .h = WINDOW_HEIGHT };
	g_ptDispOpr->FillRectT(&full_rect, COLOR_BACKGROUND);
	
	ParseStringMemRect(&g_mrTLn1, ln1, DEFAULT_FONT_SIZE);
	ParseStringMemRect(&g_mrTLn2, ln2, DEFAULT_FONT_SIZE);
	ParseStringMemRect(&g_mrTLn3, ln3, DEFAULT_FONT_SIZE);
	
	Rect src1 = { .x=0, .y=0, .w=g_mrTLn1.width, .h=g_mrTLn1.height };
	Rect src2 = { .x=0, .y=0, .w=g_mrTLn2.width, .h=g_mrTLn2.height };
	Rect src3 = { .x=0, .y=0, .w=g_mrTLn3.width, .h=g_mrTLn3.height };
	CopyMemRectT(&g_rtLn1, &src1, &g_mrTLn1);
	CopyMemRectT(&g_rtLn2, &src2, &g_mrTLn2);
	CopyMemRectT(&g_rtLn3, &src3, &g_mrTLn3);
	
	g_ptDispOpr->SetEnableT(true);
	Rect text_rect = { .x = 0, .y = 16, .w = 128, .h = 48 };
	g_ptDispOpr->UpdateRectT(&text_rect);
	
	if(dur > 0)
		uloop_timeout_set(&toast_duration_timer, dur);
	
	return 0;
}

int Toast123_fs(char *ln1, char *ln2, char *ln3, int fs,  int dur)
{
	if(!ln1 || !ln2 || !ln3)
		return -1;
	
	uloop_timeout_cancel(&toast_text_timer);
	uloop_timeout_cancel(&toast_duration_timer);
	
	Rect full_rect = { .x = 0, .y = 0, .w = WINDOW_WIDTH, .h = WINDOW_HEIGHT };
	g_ptDispOpr->FillRectT(&full_rect, COLOR_BACKGROUND);
	
	ParseStringMemRect(&g_mrTLn1, ln1, fs);
	ParseStringMemRect(&g_mrTLn2, ln2, fs);
	ParseStringMemRect(&g_mrTLn3, ln3, fs);
	
	Rect src1 = { .x=0, .y=0, .w=g_mrTLn1.width, .h=g_mrTLn1.height };
	Rect src2 = { .x=0, .y=0, .w=g_mrTLn2.width, .h=g_mrTLn2.height };
	Rect src3 = { .x=0, .y=0, .w=g_mrTLn3.width, .h=g_mrTLn3.height };
	CopyMemRectT(&g_rtLn1, &src1, &g_mrTLn1);
	CopyMemRectT(&g_rtLn2, &src2, &g_mrTLn2);
	CopyMemRectT(&g_rtLn3, &src3, &g_mrTLn3);
	
	g_ptDispOpr->SetEnableT(true);
	Rect text_rect = { .x = 0, .y = 16, .w = 128, .h = 48 };
	g_ptDispOpr->UpdateRectT(&text_rect);
	
	if(dur > 0)
		uloop_timeout_set(&toast_duration_timer, dur);
	
	return 0;
}

int  Toast0123(char *ln0, char *ln1, char *ln2, char *ln3, int dur)
{
	if(!ln0 || !ln1 || !ln2 || !ln3)
		return -1;
	
	uloop_timeout_cancel(&toast_text_timer);
	uloop_timeout_cancel(&toast_duration_timer);
	
	Rect full_rect = { .x = 0, .y = 0, .w = WINDOW_WIDTH, .h = WINDOW_HEIGHT };
	g_ptDispOpr->FillRectT(&full_rect, COLOR_BACKGROUND);
	
	ParseStringMemRect(&g_mrTLn0, ln0, DEFAULT_FONT_SIZE);
	ParseStringMemRect(&g_mrTLn1, ln1, DEFAULT_FONT_SIZE);
	ParseStringMemRect(&g_mrTLn2, ln2, DEFAULT_FONT_SIZE);
	ParseStringMemRect(&g_mrTLn3, ln3, DEFAULT_FONT_SIZE);
	
	Rect src0 = { .x=0, .y=0, .w=g_mrTLn0.width, .h=g_mrTLn0.height };
	Rect src1 = { .x=0, .y=0, .w=g_mrTLn1.width, .h=g_mrTLn1.height };
	Rect src2 = { .x=0, .y=0, .w=g_mrTLn2.width, .h=g_mrTLn2.height };
	Rect src3 = { .x=0, .y=0, .w=g_mrTLn3.width, .h=g_mrTLn3.height };
	
	CopyMemRectT(&g_rtLn0, &src0, &g_mrTLn0);
	CopyMemRectT(&g_rtLn1, &src1, &g_mrTLn1);
	CopyMemRectT(&g_rtLn2, &src2, &g_mrTLn2);
	CopyMemRectT(&g_rtLn3, &src3, &g_mrTLn3);
	
	g_ptDispOpr->SetEnableT(true);
	Rect fullRect = { .x = 0, .y = 0, .w = WINDOW_WIDTH, .h = WINDOW_HEIGHT };
	g_ptDispOpr->UpdateRectT(&fullRect);
	
	if(dur > 0)
		uloop_timeout_set(&toast_duration_timer, dur);
	
	return 0;
}

int ToastBmp(Rect *dst, char *filename, int duration)
{
	if(!filename)
		return -1;
	
	int ret = 0;
	
	uloop_timeout_cancel(&toast_text_timer);
	uloop_timeout_cancel(&toast_duration_timer);
	
	if(dst)
	{
		g_rtToast.x = dst->x;
		g_rtToast.y = dst->y;
		g_rtToast.w = dst->w;
		g_rtToast.h = dst->h;		
	}
	else
	{
		g_rtToast.x = 0;
		g_rtToast.y = 16;
		g_rtToast.w = 128;
		g_rtToast.h = 48;		
	}
	
	ret = g_ptDispOpr->FillRectT(&g_rtToast, COLOR_BACKGROUND);
	if(0 != ret)
		return -1;
	
	ret = ParseBmpMemRect(&g_mrToast, filename);
	if(ret < 0)
		return -1;
	
//	ShowMemRect(&g_mrToast);
	
	g_ptDispOpr->SetEnableT(true);
	
	Rect src = { .x=0, .y=0, .w=g_mrToast.width, .h=g_mrToast.height };
	CopyMemRectT(&g_rtToast, &src, &g_mrToast);
	g_ptDispOpr->UpdateRectT(&g_rtToast);
	
	if(duration > 0)
		uloop_timeout_set(&toast_duration_timer, duration);
	
	return 0;
}

void ToastCancel(void)
{
	uloop_timeout_cancel(&toast_duration_timer);
	uloop_timeout_cancel(&toast_text_timer);
	
	g_ptDispOpr->SetEnableT(false);
	Rect FullRect = { .x = 0, .y = 0, .w = WINDOW_WIDTH, .h = WINDOW_HEIGHT };
	g_ptDispOpr->UpdateRect(&FullRect);
}

int ToastRelease(void)
{
	uloop_timeout_cancel(&toast_text_timer);
	uloop_timeout_cancel(&toast_duration_timer);
	
	if(NULL != g_mrToast.pBuf)
	{
		free(g_mrToast.pBuf);
		g_mrToast.pBuf = NULL;
		g_mrToast.width = 0;
		g_mrToast.height = 0;
	}
	
	return 0;
}

//////////////////////////////////////////////////////////////////////
////////////////////////////Widget////////////////////////////////////
//////////////////////////////////////////////////////////////////////
int WidgetInit(void)
{	
	int ret = 0;
	
	ret = FontsInit(); // 支持freetype字体
	if (ret < 0)
	{
		DBG_PRINTF("FontsInit error!\n");
		return -1;
	}
	ret = EncodingInit();
	if (ret < 0)
	{
		DBG_PRINTF("EncodingInit error!\n");
		return -1;
	}
	
	ret = SelectEncodingOprOnName(ENCODING_FORMAT);
	if (ret)
	{
		DBG_PRINTF("SetEncodingName error!\n");
		return -1;
	}
	
#if GUI_CLI_SUPPORT
	ret = SetFontsInit(MSYHL_TTF);
#else
	ret = SetFontsInit(MSYH_TTF);
#endif
	if (ret)
	{
		DBG_PRINTF("SetFontsInit error!\n");
		return -1;
	}
	
	ret = DisplayInit();
	if (ret < 0)
	{
		DBG_PRINTF("DisplayInit error!\n");
		return -1;
	}
	
	ret = SelectAndInitDisplay(DISPLAY_DEVICE);
	if (ret)
	{
		DBG_PRINTF("SelectAndInitDisplay error!\n");
		return -1;
	}
	
	return 0;
}

void WidgetExit(void)
{
	uloop_timeout_cancel(&line1_waiting_timer);
	uloop_timeout_cancel(&line1_twinkle_timer);
	uloop_timeout_cancel(&line1_marquee_timer);
	uloop_timeout_cancel(&line2_marquee_timer);
	uloop_timeout_cancel(&line3_marquee_timer);
	
	uloop_timeout_cancel(&battery_twinkle_timer);
	uloop_timeout_cancel(&wifi_twinkle_timer);
	uloop_timeout_cancel(&update_time_timer);
	
	uloop_timeout_cancel(&toast_text_timer);
	uloop_timeout_cancel(&toast_duration_timer);
	
	if(NULL!=g_mrLine0.pBuf)
	{
		free(g_mrLine0.pBuf);
		g_mrLine0.pBuf = NULL;
		g_mrLine0.width = 0;
		g_mrLine0.height= 0;
	}
	if(NULL!=g_mrLine1.pBuf)
	{
		free(g_mrLine1.pBuf);
		g_mrLine1.pBuf = NULL;
		g_mrLine1.width = 0;
		g_mrLine1.height= 0;
	}
	if(NULL!=g_mrLine2.pBuf)
	{
		free(g_mrLine2.pBuf);
		g_mrLine2.pBuf = NULL;
		g_mrLine2.width = 0;
		g_mrLine2.height= 0;
	}
	if(NULL!=g_mrLine3.pBuf)
	{
		free(g_mrLine3.pBuf);
		g_mrLine3.pBuf = NULL;
		g_mrLine3.width = 0;
		g_mrLine3.height= 0;
	}
	if(NULL!=g_mrBmp.pBuf)
	{
		free(g_mrBmp.pBuf);
		g_mrBmp.pBuf = NULL;
		g_mrBmp.width = 0;
		g_mrBmp.height= 0;
	}
	
	if(NULL!=g_mrBattery.pBuf)
	{
		free(g_mrBattery.pBuf);
		g_mrBattery.pBuf = NULL;
		g_mrBattery.width = 0;
		g_mrBattery.height = 0;
	}
	if(NULL!=g_mrWifi.pBuf)
	{
		free(g_mrWifi.pBuf);
		g_mrWifi.pBuf = NULL;
		g_mrWifi.width = 0;
		g_mrWifi.height = 0;
	}
	if(NULL!=g_mrBluetooth.pBuf)
	{
		free(g_mrBluetooth.pBuf);
		g_mrBluetooth.pBuf = NULL;
		g_mrBluetooth.width = 0;
		g_mrBluetooth.height = 0;
	}
	if(NULL!=g_mrAlarm.pBuf)
	{
		free(g_mrAlarm.pBuf);
		g_mrAlarm.pBuf = NULL;
		g_mrAlarm.width = 0;
		g_mrAlarm.height = 0;
	}
	if(NULL!=g_mrDown.pBuf)
	{
		free(g_mrDown.pBuf);
		g_mrDown.pBuf = NULL;
		g_mrDown.width = 0;
		g_mrDown.height = 0;
	}
	if(NULL!=g_mrLock.pBuf)
	{
		free(g_mrLock.pBuf);
		g_mrLock.pBuf = NULL;
		g_mrLock.width = 0;
		g_mrLock.height = 0;
	}
	if(NULL!=g_mrTime.pBuf)
	{
		free(g_mrTime.pBuf);
		g_mrTime.pBuf = NULL;
		g_mrTime.width = 0;
		g_mrTime.height = 0;
	}
	
	if(NULL!=g_mrToast.pBuf)
	{
		free(g_mrToast.pBuf);
		g_mrToast.pBuf = NULL;
		g_mrToast.width = 0;
		g_mrToast.height = 0;
	}
	
	DisplayExit();
}

