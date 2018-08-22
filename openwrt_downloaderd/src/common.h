
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

#define TIMER_ALIVE_UNIT				100			/* 保活心跳定时器间隔(单位:毫秒) */

#define TIMER_NETWORK_PERIOD			3*1000		/* 检查网络状态,更新周期, 3s检查一次 */

#define TIMER_WATCH_DOWN				3500		/* 让其他程序,监视并订阅本下载进程的消息 */

#define MD5_STRING_LENGTH				32			/* md5字符串长度 */

#define TASK_TABLE 						"task"
#define COMPLETE_TABLE					"complete"

#define WLAN_INTERFACE					"wlan0"

#if 0

#define DOWN_DB 						"/var/lib/mpd/music/internal/task/down.db"

#define DOWN_TASK_PATH 					"/var/lib/mpd/music/internal/task"
#define DOWN_COMPLETE_PATH 				"/var/lib/mpd/music/internal/music/downloads/cache"

#define MPD_DIRECTORY 					"/var/lib/mpd/music"

#define PERM_DIR 						"/var/lib/mpd/music/internal/music" // 具有删除权限的目录

#else

#define DOWN_DB 						"/tmp/mnt/internal/task/down.db"

#define DOWN_TASK_PATH 					"/tmp/mnt/internal/task"
#define DOWN_COMPLETE_PATH 				"/tmp/mnt/internal/music/downloads/cache"

#define MPD_DIRECTORY 					"/tmp/mnt"

#define PERM_DIR 						"/tmp/mnt/internal/music" 			// 具有删除权限的目录

#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////

#define DBG_PRINTF(format, args...) 	dbg_printf(__FILE__, __LINE__, __FUNCTION__, format, ##args)
void dbg_printf(const char *file, int line, const char *func, const char *format, ...); 

int base64_out(char *buf, char *obuf, int len);

char* get_md5sum(const char *str);

char* url_strip_param(const char *url);

char* get_file_type(const char *url); // 获取该文件名的类型后缀

char* get_url_title(char *url);

char* get_complete_filename(char *url);

int down_check_directory(void);

// 获取文件名
// /tmp/mnt/internal/music/downloads/cache/1ffea764686f5fb3a78fbc0f0ffda762_旧情绵绵.mp3 => 1ffea764686f5fb3a78fbc0f0ffda762
char* get_filename(char *path);

char* strduptrim(char *line);

int mp3_set_title(char *url, char *filename);

#endif // __COMMON_H__

