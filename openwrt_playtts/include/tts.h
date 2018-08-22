
#ifndef __TTS_H__
#define __TTS_H__

typedef struct tagMemoryStruct {
	
	char *memory;
	size_t size;
	
} MemoryStruct;

typedef struct tagWriteThis {
	
	const char *readptr;
	long sizeleft;
	
} WriteThis;

int xf_main(void);

int bb_main(void);

int report_tts_error(char *content, char *sn);

int response_tts(void);

int playtts(void);

#endif // __TTS_H__

