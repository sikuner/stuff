
#ifndef __DB_H__
#define __DB_H__

#include "common.h"
#include "task.h"

#include <sqlite3.h>
#include <stdio.h>


int  db_init(void);
void db_release(void);

int  db_task_insert(const char *url);
int  db_task_del(const char *url);
int  db_task_update(const TaskRecord *task_rec);
int  db_task_select(const char *url, TaskRecord *task_rec);
int  db_task_del_all(void);
int  db_task_select_all(struct list_head *head);
int  db_task_range(int begin, int length, struct list_head *head);
int  db_task_count(int *count);

int  db_get_ready_task(TaskRecord *task_rec);

int  db_complete_insert(const TaskRecord *task_rec); // ºöÂÔlist/taskId×Ö¶Î,ÆäËû×Ö¶Î²åÈë
int  db_complete_del(const char *url);
int  db_complete_delete(const char *filename);

int  db_complete_select(const char *url, TaskRecord *task_rec);
int  db_complete_del_all(void);
int  db_complete_select_all(struct list_head *head);
int  db_complete_range(int begin, int length, struct list_head *head);
int  db_complete_count(int *count);

#endif // __DB_H__

