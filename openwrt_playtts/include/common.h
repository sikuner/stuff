
#ifndef __COMMON_H__
#define __COMMON_H__

#include <stdbool.h>
#include <string.h>

#define min(x,y) 	({ typeof(x) _x = (x); typeof(y) _y = (y); (void) (&_x == &_y); _x < _y ? _x : _y; })
#define max(x,y) 	({ typeof(x) _x = (x); typeof(y) _y = (y); (void) (&_x == &_y); _x > _y ? _x : _y; })

#ifndef MAX_PATH
#define MAX_PATH 512
#endif

#ifndef MAX_LINE
#define MAX_LINE 1024
#endif

#ifndef MAX_URL
#define MAX_URL  2048
#endif

#define DBG_PRINTF(format, args...) 	dbg_printf(__FILE__, __LINE__, __FUNCTION__, format, ##args)
void dbg_printf(const char *file, int line, const char *func, const char *format, ...); 

void trim(char *src);

char* get_beeba_version(void);

char* get_beeba_sn(void);

char* get_md5sum(const char *str);

int get_tts_vendor(void);

int playtts_check_directory(void);

#endif // __COMMON_H__

