
#ifndef __MANAGER_H__
#define __MANAGER_H__

#include "task.h"
#include "common.h"

#include <libubus.h>

int manager_init(void);
int manager_pause(void);

////////////////////////////////////////////////////////////////////////

int down_hello(char *str);
int down_shutdown(void);
int down_remove(char *path);
int down_rmdirx(char *path);

int down_length(char *url, unsigned int *length);

int down_check(char *url, unsigned int *length);
int down_exist(char *url);

int down_query(char *url, TaskRecord *task_rec);

int down_task_create(char *url);
int down_task_delete(char *url);
int down_task_start(char *url);
int down_task_stop(char *url);

int down_task_create_all(char **urls); // urls - url数组以NULL结尾
int down_task_delete_all(void);
int down_task_start_all(void);
int down_task_stop_all(void);

int down_task_current(TaskInfo *ti);
int down_task_range(int begin, int length, struct list_head *record_head);
int down_task_count(int *count);

int down_complete_delete(char *url);
int down_complete_delete_all(void);
int down_complete_range(int begin, int length, struct list_head *record_head);
int down_complete_count(int *count);

// 四.订阅通知
// 1.进度 progress
// 	触发时机:当一个任务正在获取数据时,携带进度信息,每隔一秒间隔, 会广播此通知
// 2.状态 status
// 	触发时机:当一个下载任务状态发生改变时,会广播此通知
// 3.完成 complete
// 	触发时机:当一个下载任务完成,在退出进入下一个任务之前,会广播此通知

#endif // __MANAGER_H__

