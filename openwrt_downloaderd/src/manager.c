
#include "manager.h"
#include "network.h"
#include "task.h"
#include "usrv.h"
#include "db.h"

#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>

static pthread_t manage_thrdId = (pthread_t)NULL;
static volatile bool manage_pause  = false;

static pthread_mutex_t mutex_event  = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  cond_event   = PTHREAD_COND_INITIALIZER;

static void send_task_event(void);

static int progress_cb(TaskInfo *ti)
{
	return notify_progress(ti);
}

static int status_cb(TaskInfo *ti)
{
	TaskRecord task_rec = { {0} };
	task_rec.taskId = ti->taskId;
	task_rec.status = ti->status;
	task_rec.errorno = ti->errorno;
	task_rec.received = ti->received;
	task_rec.total = ti->total;
	strcpy(task_rec.filename, ti->filename);
	strcpy(task_rec.url, ti->url);
	
	if(DT_COMPLETE != task_rec.status)
	{
		db_task_update(&task_rec);
	}
	
	return notify_status(&task_rec);
}

static int task_exit_noevent(TaskInfo *ti)
{
	TaskRecord task_rec;
	TaskRecord complete_rec;
	
	task_rec.taskId = ti->taskId;
	task_rec.status = ti->status;
	task_rec.errorno = ti->errorno;
	task_rec.received = ti->received;
	task_rec.total = ti->total;
	strcpy(task_rec.filename, ti->filename);
	strcpy(task_rec.url, ti->url);
	
	if(DT_COMPLETE == task_rec.status)
	{
		db_complete_insert(&task_rec);
		db_task_del(task_rec.url);
		
		int  ret = 0;
		char *dt_filename       = get_task_filename_dt(task_rec.url);
		char *complete_filename = get_complete_filename(task_rec.url);
		
		ret = rename(dt_filename, complete_filename);
//		DBG_PRINTF("222 rename(old:%s, new:%s) ret: %d \n", dt_filename, complete_filename, ret);
		(void)ret;
//		mp3_set_title(task_rec.url, complete_filename);
		
		db_complete_select(task_rec.url, &complete_rec);
		
		notify_complete(&task_rec, &complete_rec);
	}
	else
	{
		db_task_update(&task_rec);
	}
	
	task_idle();
	
	return 0;
}

static int task_exit(TaskInfo *ti)
{
	task_exit_noevent(ti);
	send_task_event();
	
	return 0;
}

static void network_connected_cb(void)
{
	DBG_PRINTF("Network Connection Success! \n");
	send_task_event();
}

static void get_task_event(void)
{
	if(manage_pause)
	{
		return;
	}
	
	pthread_mutex_lock(&mutex_event);
	pthread_cond_wait(&cond_event, &mutex_event);	
	pthread_mutex_unlock(&mutex_event);
}

static void send_task_event(void)
{
	pthread_mutex_lock(&mutex_event); 
	pthread_cond_signal(&cond_event);
	pthread_mutex_unlock(&mutex_event);
}

static void* manage_func(void *params)
{
	TaskRecord task_rec;
	int rc = -1;
	
	while(!manage_pause)
	{
		DBG_PRINTF(" Waiting for an event \n");
		get_task_event();
		DBG_PRINTF(" Get an event \n");
		
		if(manage_pause)
		{
			break;
		}
		
next_ready:
		if(network_get_connected()) // 处于已联网状态,才开始执行下载任务
		{
			rc = db_get_ready_task(&task_rec);
			if(rc < 0)
			{
				continue;
			}
			
			task_set_handler(task_exit, status_cb, progress_cb);
			rc = task_start(&task_rec); 
			DBG_PRINTF("task_start rc: %d, url: %s \n", rc, task_rec.url);
			if(rc < 0) // 启动出错
			{
				goto next_ready;
			}
		}
	}
	
	return NULL;
}

int manager_init(void)
{
	manage_thrdId = (pthread_t)NULL;
	manage_pause  = false;
	
	network_set_connected_handler(network_connected_cb);
	task_set_handler(task_exit, status_cb, progress_cb);
	pthread_create(&manage_thrdId, NULL, manage_func, NULL);
	
	return 0;
}

int manager_pause(void)
{
	manage_pause = true;
	send_task_event();
	
	if(manage_thrdId 
	&& pthread_kill(manage_thrdId, 0) != ESRCH
	&& pthread_join(manage_thrdId, NULL) == 0)
	{
		return 0;
	}
	
	manage_thrdId = (pthread_t)NULL;
	
	DBG_PRINTF("manager_pause OK \n");
	return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////UBUS接口////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////

int down_hello(char *str)
{
	DBG_PRINTF(" Hello Down: %s \n", str);
	
	return 0;
}

int down_shutdown(void)
{
	DBG_PRINTF(" shutdown the application \n");
	
	manager_pause();
	task_stop();
	
	return 0;
}

int down_remove(char *path)
{
	if(NULL == path)
	{
		return -1;
	}
	
	char abs_path[MAX_PATH] = { 0 };
	sprintf(abs_path, MPD_DIRECTORY"/%s", path);
	
	DBG_PRINTF("abs_path: %s \n", abs_path);
	
	if(0 != access(abs_path, F_OK)) // 该文件不存在
	{
		return 1; 
	}
	
	if( strncmp(abs_path, PERM_DIR, strlen(PERM_DIR)) )
	{
		return 2; // 权限拒绝
	}
	
	char *filename = get_filename(abs_path);
	if(MD5_STRING_LENGTH == strlen(filename))
	{
		db_complete_delete(filename);
	}
	
	return remove(abs_path);
}

int down_rmdirx(char *path)
{
	if(NULL == path)
	{
		return -1;
	}
	
	int ret = 0;
	
	char abs_dir[MAX_PATH] = { 0 };
	sprintf(abs_dir, MPD_DIRECTORY"/%s", path);
	
	DBG_PRINTF("abs_dir: %s \n", abs_dir);
	
	if(0 != access(abs_dir, F_OK)) // 该文件不存在
	{
		return 1; 
	}
	
	if( strncmp(abs_dir, PERM_DIR, strlen(PERM_DIR)) )
	{
		return 2; // 权限拒绝
	}
	
	struct stat path_stat;
	
	if (lstat(abs_dir, &path_stat) < 0) 
	{
		DBG_PRINTF("can't stat '%s' \n", abs_dir);
		return -1; // 出错
	}
	
	if ( !S_ISDIR(path_stat.st_mode) ) 
	{
		return 3; // 该文件不是目录
	}
	
	if( !strncmp(abs_dir, DOWN_COMPLETE_PATH, strlen(DOWN_COMPLETE_PATH)) )
	{
		return down_complete_delete_all();
	}
	
	DIR *dp;
	struct dirent *d;
	char file_path[MAX_PATH] = { 0 };
	
	dp = opendir(abs_dir);
	if (dp == NULL) 
	{
		DBG_PRINTF("can't open '%s' \n", abs_dir);
		return -1;
	}
	
	while ((d = readdir(dp)) != NULL) 
	{
		if (!strcmp(d->d_name, ".") || !strcmp(d->d_name, ".."))
			continue;
		
		memset(file_path, 0, sizeof(file_path));
		sprintf(file_path, "%s/%s", abs_dir, d->d_name);
		
		if(lstat(file_path, &path_stat) < 0 || S_ISDIR(path_stat.st_mode))
		{
//			DBG_PRINTF("is directory. file_path: %s \n", file_path);
			continue;
		}
		
//		DBG_PRINTF("is file. file_path: %s \n", file_path);		
		if (unlink(file_path) < 0 && errno != ENOENT) 
		{
			DBG_PRINTF("can't unlink '%s' \n", file_path);
			ret = -1;
			goto out;
		}
	}
	
out:
	if (closedir(dp) < 0) 
	{
		DBG_PRINTF("can't close '%s' \n", abs_dir);
		return -1;
	}
	
	return ret;
}

int down_length(char *url, unsigned int *length)
{
	return get_url_length(url, length);	
}

int down_check(char *url, unsigned int *length)
{
	if(NULL==url || 0==strlen(url) || NULL==length)
	{
		return -1;
	}
	
	int ret = 0;
	TaskRecord task_rec;
	
	ret = db_complete_select(url, &task_rec);
	if(0 == ret) // 该url在 已下载列表 中
	{
		return 2;
	}
	
	ret = db_task_select(url, &task_rec);
	if(0 == ret) // 该url在 任务列表 中
	{
		return 1;
	}
	
	return get_url_length(url, length);	
}

int down_exist(char *url)
{
	if(NULL==url || 0==strlen(url))
	{
		return -1;
	}
	
	int ret = 0;
	TaskRecord task_rec;
	
	ret = db_complete_select(url, &task_rec);
	if(0 == ret) // 该url在 已下载列表 中
	{
		return 2;
	}
	
	ret = db_task_select(url, &task_rec);
	if(0 == ret) // 该url在 任务列表 中
	{
		return 1;
	}
	
	return 0;     // 都不在 已下载列表 和 任务列表
}

int down_query(char *url, TaskRecord *task_rec)
{
	if(NULL==url || 0==strlen(url) || NULL==task_rec)
	{
		return -1;
	}
	
	int rc = -1;
	TaskInfo ti;
	
	rc = db_complete_select(url, task_rec);
	if(0 == rc) // 该url在 已下载列表 中
	{
		return 2;
	}
	
	rc = task_info(&ti);
	if(0==rc && 0==strcmp(ti.url, url))
	{
		task_rec->taskId = ti.taskId;
		task_rec->status = ti.status;
		task_rec->errorno = ti.errorno;
		task_rec->received = ti.received;
		task_rec->total = ti.total;
		strcpy(task_rec->filename, ti.filename);
		strcpy(task_rec->url, ti.url);
		
		return 0;
	}
	
	rc = db_task_select(url, task_rec);
	if(0 == rc)
	{
		return 1;
	}
	
	return -1;
}

int down_task_create(char *url)
{
	if(NULL==url || 0==strlen(url))
	{
		return -1;
	}
	
	int rc;
	TaskRecord task_record;
	
	rc = db_complete_select(url, &task_record);
	if(0 == rc)
	{
		return 2;
	}
	
	rc = db_task_select(url, &task_record);
	if(0 == rc)
	{
		return 1;
	}
	
	rc = db_task_insert(url);
	if(0 != rc)
	{
		return -1;
	}
	
	if( !task_running() )
	{
		send_task_event();
	}
	
	return 0;
}

int down_task_delete(char *url)
{
	if(NULL==url || 0==strlen(url))
	{
		return -1;
	}
	
	int rc = -1;
	TaskRecord task_record;
	TaskInfo ti;
	
	rc = db_complete_select(url, &task_record);
	if(0 == rc) // 该url在 已下载列表 中
	{
		return 2;
	}
	
	rc = db_task_select(url, &task_record);
	if(0 != rc) // 下载任务列表中没有该任务
	{
		return 1;
	}
	else // 下载任务列表中存在该任务
	{
		rc = task_info(&ti);
		if(ti.taskId == task_record.taskId)
		{
			task_delete();
		}
		else
		{
			task_remove_tmp(task_record.filename);			
		}
		
		return db_task_del(url);
	}
}

int down_task_start(char *url)
{
	if(NULL==url || 0==strlen(url))
	{
		return -1;
	}
	
	int rc = -1;
	TaskRecord task_record;
	TaskInfo ti;
	
	rc = db_complete_select(url, &task_record);
	if(0 == rc) // 该url在 已下载列表 中
	{
		return 2;
	}
	
	rc = db_task_select(url, &task_record);
	if(0 != rc) // 下载任务列表 中没有该任务
	{
		return 1;
	}
	else // 下载任务列表中存在该任务
	{
		rc = task_info(&ti);
		if(task_running() && ti.taskId==task_record.taskId)
		{
			return 0;
		}
		else
		{
			return task_start(&task_record);
		}
	}
	
	return 0;
}

int down_task_stop(char *url)
{
	if(NULL==url || 0==strlen(url))
	{
		return -1;
	}
	
	int rc = -1;
	TaskRecord task_record;
	TaskInfo ti;
	
	rc = db_task_select(url, &task_record);
	if(rc < 0)
	{
		return 1; 	// 下载任务列表中没有该任务
	}
	else
	{
		rc = task_info(&ti);
		if(task_running() && ti.taskId==task_record.taskId)
		{
			return task_stop();
		}
		else
		{
			return 2; // 该任务没有执行, 不处于下载状态
		}
	}
	
	return 0;
}


int down_task_create_all(char **urls) // urls - url数组以NULL结尾
{
	if(NULL == urls)
	{
		return -1;
	}
	
	int i = 0;
	int count = 0;
	TaskRecord task_rec;
	
	for(i = 0; urls[i] != NULL; i++)
	{	
		if(0 == db_complete_select(urls[i], &task_rec)
		|| 0 == db_task_select(urls[i], &task_rec))
		{
			continue;
		}
		
		if(0 == db_task_insert(urls[i]))
		{
			count++;
		}
	}
	
	if(count>0 && !task_running())
	{
		send_task_event();
	}
	
	return count;
}

int down_task_delete_all(void)
{
	int rc = -1;
	
	task_set_handler(task_exit_noevent, NULL, NULL);
	task_delete();
	task_idle();
	
	struct list_head records_head = LIST_HEAD_INIT(records_head);
	rc = db_task_select_all(&records_head);
	if(rc < 0)
	{
		return -1;
	}
	
	TaskRecord *pRec = NULL;
	list_for_each_entry(pRec, &records_head, list) 
	{
		task_remove_tmp(pRec->filename);
	}
	
	rc = db_task_del_all();
	if(rc < 0)
	{
		return -1;
	}
	
	// 销毁链表
	struct list_head *pos = records_head.next; 
	struct list_head *tmp = NULL;
	while(pos != &records_head)
	{
		tmp = pos->next;
		list_del(pos);
		
		pRec = container_of(pos, TaskRecord, list);
		free(pRec);
		pRec = NULL;
		
		pos = tmp;
	}
	
	return 0;
}

int down_task_start_all(void)
{
	int rc = -1;
	
	task_set_handler(task_exit_noevent, status_cb, progress_cb);
	task_stop();
	
	struct list_head records_head = LIST_HEAD_INIT(records_head);
	rc = db_task_select_all(&records_head);
	if(rc < 0)
	{
		return -1;
	}
	
	TaskRecord *pRec = NULL;
	list_for_each_entry(pRec, &records_head, list) 
	{
		pRec->status = DT_READY;
		db_task_update(pRec);
	}
	
	// 销毁链表
	struct list_head *pos = records_head.next; 
	struct list_head *tmp = NULL;
	while(pos != &records_head)
	{
		tmp = pos->next;
		list_del(pos);
		
		pRec = container_of(pos, TaskRecord, list);
		free(pRec);
		pRec = NULL;
		
		pos = tmp;
	}
	
	if( !task_running() )
	{
		send_task_event();
	}
	
	return 0;
}

int down_task_stop_all(void)
{
	int rc = -1;
	
	task_set_handler(task_exit_noevent, NULL, NULL);
	task_stop();
	
	struct list_head records_head = LIST_HEAD_INIT(records_head);
	rc = db_task_select_all(&records_head);
	if(rc < 0)
	{
		return -1;
	}
	
	TaskRecord *pRec = NULL;
	list_for_each_entry(pRec, &records_head, list) 
	{
		pRec->status = DT_PAUSE;
		db_task_update(pRec);
	}
	
	// 销毁链表
	struct list_head *pos = records_head.next; 
	struct list_head *tmp = NULL;
	while(pos != &records_head)
	{
		tmp = pos->next;
		list_del(pos);
		
		pRec = container_of(pos, TaskRecord, list);
		free(pRec);
		pRec = NULL;
		
		pos = tmp;
	}
	
	return 0;
}

int down_task_current(TaskInfo *ti)
{
	return task_info(ti);
}

int down_task_range(int begin, int length, struct list_head *record_head)
{
	return db_task_range(begin, length, record_head);
}

int down_task_count(int *count)
{
	return db_task_count(count);
}

int down_complete_delete(char *url)
{
	if(NULL==url || 0==strlen(url))
	{
		return -1;
	}
	
	int rc = -1;	
	TaskRecord task_rec;
	
	rc = db_complete_select(url, &task_rec);
	if(rc < 0)
	{
		return 1; // 已下载列表中没有该文件
	}
	
	remove(get_complete_filename(url)); // 删除已下载文件
	
	rc = db_complete_del(url);
	if(rc < 0)
	{
		return -1;
	}
	
	return 0;
}

int down_complete_delete_all(void)
{
	int rc = -1;
	struct list_head records_head = LIST_HEAD_INIT(records_head);
	
	rc = db_complete_select_all(&records_head);
	if(rc < 0)
	{
		return -1;
	}
	
	TaskRecord *pRec = NULL;
	list_for_each_entry(pRec, &records_head, list) 
	{
		remove(get_complete_filename(pRec->url));
	}
	
	rc = db_complete_del_all();
	if(rc < 0)
	{
		return -1;
	}
	
	// 销毁字符链表
	struct list_head *pos = records_head.next; 
	struct list_head *tmp = NULL;
	while(pos != &records_head)
	{
		tmp = pos->next;
		list_del(pos);
		
		pRec = container_of(pos, TaskRecord, list);
		free(pRec);
		pRec = NULL;
		
		pos = tmp;
	}
	
	return 0;
}

int down_complete_range(int begin, int length, struct list_head *record_head)
{
	return db_complete_range(begin, length, record_head);
}

int down_complete_count(int *count)
{
	return db_complete_count(count);
}

// 四.订阅通知
// 1.进度 progress
// 	触发时机:当一个任务正在获取数据时,携带进度信息,每隔一秒间隔, 会广播此通知
// 2.状态 status
// 	触发时机:当一个下载任务状态发生改变时,会广播此通知
// 3.完成 complete
// 	触发时机:当一个下载任务完成,在退出进入下一个任务之前,会广播此通知


