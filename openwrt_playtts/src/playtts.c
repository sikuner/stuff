
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <curl/curl.h>

#include "cfgable.h"
#include "common.h"
#include "tts.h"
#include "mpdclient.h"

int playtts(void)
{
	int ret = -1;
	int mpd_state = -1;
	char command[MAX_LINE] = { 0 };
	
	mpd_state = mpdclient_get_state();
	if(PLAYER_STATE_PLAY == mpd_state)
	{
		mpdclient_pause();
	}
	
	ret = system("madplay /usr/share/sound/tip.mp3");
	
	if(TTS_BEEBA == getConfig()->vendor)
	{
		sprintf(command, "madplay %s", getConfig()->cachefile);
		ret = system(command);
	}
	else
	{
		sprintf(command, "aplay %s", getConfig()->cachefile);		
		ret = system(command);
	}
	
	DBG_PRINTF(" <PlayEnd> command: %s, ret: %d \n", command, ret);
	
//	usleep(100*1000);
	
	if(PLAYER_STATE_PLAY == mpd_state)
	{
		mpdclient_play();
	}
	
	return ret < 0 ? -1 : 0;
}

