
#ifndef __CFGABLE_H__
#define __CFGABLE_H__

#include <time.h>
#include "common.h"

#define RESPONSE_OK						10000

#define BEEBA_TTS_URL  					"http://api.beeba.cn/tts/%s/play?data={\"text\":\"%s\"}"

#define DEFAULT_SN						"fb3f1b7ce495aac63fcd6a7e47c63a10"
#define DEFAULT_TEXT_CN					"比巴科技"
#define DEFAULT_TEXT_EN					"Beeba Technology"

#define TTS_ERROR_URL  					"http://api.beeba.cn/tts/%s/error"

#define TTS_RESPONSE_URL  				"http://api.beeba.cn/memberv2/playback/logid/"

#define BEEBA_VERSION_PATH				"/etc/beeba_version"

#define BEEBA_SN_PATH					"/factory/.s"

#define SOUND_PRELUDE					"/usr/share/sound/tip.mp3"

#define BB_TTS_SUFFIX 					"bb"
#define XF_TTS_SUFFIX 					"xf"

#define PLAYTTS_CACHE_PATH				"/tmp/mnt/internal/playtts/"

#define BB_TTS_PATH 					PLAYTTS_CACHE_PATH"btts.mp3"
#define XF_TTS_PATH 					PLAYTTS_CACHE_PATH"xtts.wav"

typedef enum {

	TTS_UNKNOWN = 0,
	
	TTS_XUNFEI = 1,
	TTS_BEEBA = 2,
	
	TTS_LAST /* never use! */
	
} TTS_VENDOR;

typedef enum {

	PTTS_FAIL = -1,
	PTTS_OK = 0,
	
	PTTS_RESPONSE = 1,
	PTTS_CBED,						// 已经回调
	PTTS_NOREC, 					// 记录不存在
	PTTS_BBFMT, 					// 比巴TTS, 请求响应格式错误
	
	PTTS_LAST /* never use! */
	
} PTTSCode; // playtts

typedef int (*tts_main_func)(void);

typedef struct tagGlobalConfig
{
	char text[MAX_LINE];
	int logid;
	char sn[MAX_LINE];
	
	int vendor;
	char outfile[MAX_PATH];
	char cachefile[MAX_PATH];
	tts_main_func tts_main;
	
	time_t start_time;
	char volume[MAX_LINE];	
	char sysver[MAX_LINE];	
	int resp_status;
	
} GlobalConfig;

GlobalConfig* getConfig();

#endif // __CFGABLE_H__

