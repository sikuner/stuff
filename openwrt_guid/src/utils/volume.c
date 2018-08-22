
#include "conf.h"
#include <stdio.h>

static char *icon_volume[101] = {

	[0]   = ICON_VOLUME_0,
	[1]   = ICON_VOLUME_1,
	[2]   = ICON_VOLUME_1,
	[3]   = ICON_VOLUME_1,
	[4]   = ICON_VOLUME_1,
	[5]   = ICON_VOLUME_2,
	[6]   = ICON_VOLUME_2,
	[7]   = ICON_VOLUME_2,
	[8]   = ICON_VOLUME_2,
	[9]   = ICON_VOLUME_3,
	
	[10]  = ICON_VOLUME_3,
	[11]  = ICON_VOLUME_3,
	[12]  = ICON_VOLUME_3,
	[13]  = ICON_VOLUME_4,
	[14]  = ICON_VOLUME_4,
	[15]  = ICON_VOLUME_4,
	[16]  = ICON_VOLUME_4,
	[17]  = ICON_VOLUME_5,
	[18]  = ICON_VOLUME_5,
	[19]  = ICON_VOLUME_5,
	
	[20]  = ICON_VOLUME_5,
	[21]  = ICON_VOLUME_6,
	[22]  = ICON_VOLUME_6,
	[23]  = ICON_VOLUME_6,
	[24]  = ICON_VOLUME_6,
	[25]  = ICON_VOLUME_7,
	[26]  = ICON_VOLUME_7,
	[27]  = ICON_VOLUME_7,
	[28]  = ICON_VOLUME_7,
	[29]  = ICON_VOLUME_8,
	
	[30]  = ICON_VOLUME_8,
	[31]  = ICON_VOLUME_8,
	[32]  = ICON_VOLUME_8,
	[33]  = ICON_VOLUME_9,
	[34]  = ICON_VOLUME_9,
	[35]  = ICON_VOLUME_9,
	[36]  = ICON_VOLUME_9,
	[37]  = ICON_VOLUME_10,
	[38]  = ICON_VOLUME_10,
	[39]  = ICON_VOLUME_10,
	
	[40]  = ICON_VOLUME_10,
	[41]  = ICON_VOLUME_11,
	[42]  = ICON_VOLUME_11,
	[43]  = ICON_VOLUME_11,
	[44]  = ICON_VOLUME_11,
	[45]  = ICON_VOLUME_12,
	[46]  = ICON_VOLUME_12,
	[47]  = ICON_VOLUME_12,
	[48]  = ICON_VOLUME_12,
	[49]  = ICON_VOLUME_13,
	
	[50]  = ICON_VOLUME_13,
	[51]  = ICON_VOLUME_13,
	[52]  = ICON_VOLUME_13,
	[53]  = ICON_VOLUME_14,
	[54]  = ICON_VOLUME_14,
	[55]  = ICON_VOLUME_14,
	[56]  = ICON_VOLUME_14,
	[57]  = ICON_VOLUME_15,
	[58]  = ICON_VOLUME_15,
	[59]  = ICON_VOLUME_15,
	
	[60]  = ICON_VOLUME_15,
	[61]  = ICON_VOLUME_16,
	[62]  = ICON_VOLUME_16,
	[63]  = ICON_VOLUME_16,
	[64]  = ICON_VOLUME_16,
	[65]  = ICON_VOLUME_17,
	[66]  = ICON_VOLUME_17,
	[67]  = ICON_VOLUME_17,
	[68]  = ICON_VOLUME_17,
	[69]  = ICON_VOLUME_18,
	
	[70]  = ICON_VOLUME_18,
	[71]  = ICON_VOLUME_18,
	[72]  = ICON_VOLUME_18,
	[73]  = ICON_VOLUME_19,
	[74]  = ICON_VOLUME_19,
	[75]  = ICON_VOLUME_19,
	[76]  = ICON_VOLUME_19,
	[77]  = ICON_VOLUME_20,
	[78]  = ICON_VOLUME_20,
	[79]  = ICON_VOLUME_20,
	
	[80]  = ICON_VOLUME_20,
	[81]  = ICON_VOLUME_21,
	[82]  = ICON_VOLUME_21,
	[83]  = ICON_VOLUME_21,
	[84]  = ICON_VOLUME_21,
	[85]  = ICON_VOLUME_22,
	[86]  = ICON_VOLUME_22,
	[87]  = ICON_VOLUME_22,
	[88]  = ICON_VOLUME_22,
	[89]  = ICON_VOLUME_23,
	
	[90]  = ICON_VOLUME_23,
	[91]  = ICON_VOLUME_23,
	[92]  = ICON_VOLUME_23,
	[93]  = ICON_VOLUME_24,
	[94]  = ICON_VOLUME_24,
	[95]  = ICON_VOLUME_24,
	[96]  = ICON_VOLUME_24,
	[97]  = ICON_VOLUME_25,
	[98]  = ICON_VOLUME_25,
	[99]  = ICON_VOLUME_25,
	
	[100] = ICON_VOLUME_25

};

char* get_volume_icon(int vol)
{
	if(vol < 0 || vol > 100)
	{
		return NULL;
	}
	
	return icon_volume[vol];
}

