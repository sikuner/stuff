
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

#define TIMER_ALIVE_UNIT				100			/* ����������ʱ�����(��λ:����) */

#define TIMER_NETWORK_PERIOD			3*1000		/* �������״̬,��������, 3s���һ�� */

#define TIMER_WATCH_DOWN				3500		/* ����������,���Ӳ����ı����ؽ��̵���Ϣ */

#define MD5_STRING_LENGTH				32			/* md5�ַ������� */

#define TASK_TABLE 						"task"
#define COMPLETE_TABLE					"complete"

#define WLAN_INTERFACE					"wlan0"

#if 0

#define DOWN_DB 						"/var/lib/mpd/music/internal/task/down.db"

#define DOWN_TASK_PATH 					"/var/lib/mpd/music/internal/task"
#define DOWN_COMPLETE_PATH 				"/var/lib/mpd/music/internal/music/downloads/cache"

#define MPD_DIRECTORY 					"/var/lib/mpd/music"

#define PERM_DIR 						"/var/lib/mpd/music/internal/music" // ����ɾ��Ȩ�޵�Ŀ¼

#else

#define DOWN_DB 						"/tmp/mnt/internal/task/down.db"

#define DOWN_TASK_PATH 					"/tmp/mnt/internal/task"
#define DOWN_COMPLETE_PATH 				"/tmp/mnt/internal/music/downloads/cache"

#define MPD_DIRECTORY 					"/tmp/mnt"

#define PERM_DIR 						"/tmp/mnt/internal/music" 			// ����ɾ��Ȩ�޵�Ŀ¼

#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////

#define DBG_PRINTF(format, args...) 	dbg_printf(__FILE__, __LINE__, __FUNCTION__, format, ##args)
void dbg_printf(const char *file, int line, const char *func, const char *format, ...); 

int base64_out(char *buf, char *obuf, int len);

char* get_md5sum(const char *str);

char* url_strip_param(const char *url);

char* get_file_type(const char *url); // ��ȡ���ļ��������ͺ�׺

char* get_url_title(char *url);

char* get_complete_filename(char *url);

int down_check_directory(void);

// ��ȡ�ļ���
// /tmp/mnt/internal/music/downloads/cache/1ffea764686f5fb3a78fbc0f0ffda762_��������.mp3 => 1ffea764686f5fb3a78fbc0f0ffda762
char* get_filename(char *path);

char* strduptrim(char *line);

int mp3_set_title(char *url, char *filename);

#endif // __COMMON_H__

