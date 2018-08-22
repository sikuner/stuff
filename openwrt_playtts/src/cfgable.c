
#include "cfgable.h"

static GlobalConfig global = {
	
	.start_time = 0,
	
//	char text[MAX_LINE];
//	int logid;
//	char sn[MAX_LINE];
//	
	.vendor = TTS_XUNFEI,
//	char outfile[MAX_PATH];
//	char cachefile[MAX_PATH];
//	tts_main_func tts_main;
	
//	time_t start_time;
//	char volume[MAX_LINE];	
	.sysver = "0.0.0",
	.resp_status = -1
};

GlobalConfig* getConfig()
{
	return &global;
}

