
#include <stdlib.h>
#include <stdio.h>
#include <libubox/uloop.h>
#include <unistd.h>

#include "cfgable.h"
#include "common.h"
#include "tts.h"
#include "mpdclient.h"

// x64  # make LINUX64=1
// mips # make

int config_init(char *text, int logid)
{
	if(!text || logid<0)
	{
		return -1;
	}
	
	getConfig()->start_time = time(NULL); 			// 开始时间戳	
	sprintf(getConfig()->text, "%s.", text); 	// 请求 文本
	getConfig()->logid = logid;						// LogID
	
	char *sn = get_beeba_sn();
	if(!sn)
	{
		fprintf(stderr, " fail to get_beeba_sn. \n");
		return -1;
	}
	strcpy(getConfig()->sn, sn);
	
	strcpy(getConfig()->volume, "n/a");
	int vol = mpdclient_get_volume();
	if(vol >= 0)
	{
		memset(getConfig()->volume, 0, sizeof(getConfig()->volume));
		sprintf(getConfig()->volume, "%d%", vol);
	}
	
	char *sysver = get_beeba_version();
	if(sysver)
	{
		strcpy(getConfig()->sysver, sysver);
	}
	
	getConfig()->resp_status = -1;
	
	char *md5 = get_md5sum(text);
	if(!md5)
	{
		fprintf(stderr, " fail to get md5sum. \n");
		return -1;
	}
	
	getConfig()->vendor = get_tts_vendor();
	if(getConfig()->vendor == TTS_BEEBA)
	{
		strcpy(getConfig()->outfile, BB_TTS_PATH);
		sprintf(getConfig()->cachefile, PLAYTTS_CACHE_PATH"%s.%s", md5, BB_TTS_SUFFIX);
		getConfig()->tts_main = bb_main;
	}
	else
	{
		strcpy(getConfig()->outfile, XF_TTS_PATH);
		sprintf(getConfig()->cachefile, PLAYTTS_CACHE_PATH"%s.%s", md5, XF_TTS_SUFFIX);
		getConfig()->tts_main = xf_main;
	}
	
	DBG_PRINTF("text        : %s \n", getConfig()->text);
	DBG_PRINTF("logid       : %d \n", getConfig()->logid);
	DBG_PRINTF("sn          : %s \n", getConfig()->sn);
	
	DBG_PRINTF("vendor      : %d \n", getConfig()->vendor);
	DBG_PRINTF("outfile     : %s \n", getConfig()->outfile);
	DBG_PRINTF("cachefile   : %s \n", getConfig()->cachefile);
	DBG_PRINTF("tts_main    : 0x%08X \n", getConfig()->tts_main);
	
	DBG_PRINTF("start_time  : %lu \n", getConfig()->start_time);
	DBG_PRINTF("volume      : %s \n", getConfig()->volume);
	DBG_PRINTF("sysver      : %s \n", getConfig()->sysver);
	DBG_PRINTF("resp_status : %d \n\n", getConfig()->resp_status);
	
	return 0;
}

int main(int argc, char *argv[])
{
	DBG_PRINTF(" xxxx Beeba PlayTTS xxxx \n");
	
	char *text = DEFAULT_TEXT_EN;
	int  logid = 0x000BEEBA;
	int ret = -1;
	
	if(argc != 3)
	{
		DBG_PRINTF("\n\n  Usage: %s <text> <logid> \n\n", argv[0]);
		return -1;
	}
	else
	{
		text  = argv[1];
		logid = atoi(argv[2]);
	}
	
	playtts_check_directory();
	
	ret = config_init(text, logid);
	if(ret < 0)
	{
		DBG_PRINTF(" Fail to config_init. \n");
		return -1;
	}
	
	ret = access(getConfig()->cachefile, F_OK);
	DBG_PRINTF(" cachefile: %s, ret: %d \n", getConfig()->cachefile, ret);
	if(0 != ret) // 不存在
	{
		ret = getConfig()->tts_main();
		if(PTTS_OK == ret)
		{
			rename(getConfig()->outfile, getConfig()->cachefile);
		}
	}
	
	// 得到 目标音频文件 后, 开始播放
	if(PTTS_OK == ret)
	{
		ret = playtts();
		if(0 == ret)
		{
			getConfig()->resp_status = 0;
		}
	}
	else
	{
		getConfig()->resp_status = ret;
	}
	
	ret = response_tts();
	DBG_PRINTF("response_tts ret: %d \n", ret);
	
	return 0;
}

