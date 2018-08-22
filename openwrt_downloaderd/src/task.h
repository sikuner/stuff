
#ifndef __TASK_H__
#define __TASK_H__

#include "common.h"

#include <stdint.h>
#include <curl/curl.h>
#include <pthread.h>
#include <libubus.h>

// 下载临时文件后缀, 及相应配置文件后缀
#define DOWNTASK_FILE_TMP_DT 			"dt"
#define DOWNTASK_FILE_TMP_DT_CFG 		"dt.cfg"

//检测是否支持多线程分片传输的字符串
#define RANGE_TEST_FLAG 				"RangeTest"

//用于检测是否支持多线程下载接收大小
#define RANGE_TEST_RECV_SIZE 			1024

enum  DOWN_TASK_STATUS
{
	DT_NOITEM = 0,			
	DT_ERROR,				// 错误
	DT_READY,				// 准备
	DT_PAUSE,				// 暂停
	DT_DOWNLOAD,			// 下载中
	DT_COMPLETE				// 已完成
};

enum TASK_ERROR_TYPE
{
	TASK_ERROR_UNKNOWN	   =			0x00,   // 未知错误
	TASK_ERROR_DISK_CREATE =			0x01,   // 创建文件失败
	TASK_ERROR_DISK_WRITE =				0x02,   // 写文件失败
	TASK_ERROR_DISK_READ =				0x03,   // 读文件失败
	TASK_ERROR_DISK_RENAME =			0x04,   // 重命名失败
	TASK_ERROR_DISK_PIECEHASH =			0x05,   // 文件片校验失败
	TASK_ERROR_DISK_FILEHASH =			0x06,   // 文件全文校验失败
	TASK_ERROR_DISK_DELETE =			0x07,   // 删除文件失败失败
	TASK_ERROR_DOWN_INVALID =			0x10,   // 无效的DOWN地址
	TASK_ERROR_PROXY_AUTH_TYPE_UNKOWN = 0x20,   // 代理类型未知
	TASK_ERROR_PROXY_AUTH_TYPE_FAILED = 0x21,   // 代理认证失败
	TASK_ERROR_HTTPMGR_NOT_IP =			0x30,   // http下载中无ip可用
	TASK_ERROR_TIMEOUT =				0x40,   // 任务超时
	TASK_ERROR_CANCEL =					0x41,   // 任务取消
    TASK_ERROR_TP_CRASHED=              0x42,   // MINITP崩溃
    TASK_ERROR_ID_INVALID =             0x43,   // TaskId 非法
	
	TASK_ERROR_THREAD_CREATE =          0x50,   // 线程创建失败
	TASK_ERROR_RESOURCE_NOT_FOUND =		0x51,	// 资源找不到
	TASK_ERROR_CURL_PARAMETER  =		0x52	// CURL参数设置错误
};

typedef struct tagTaskConfig
{
	uint32_t 	begin;  						// 开始处
	uint32_t 	received; 						// 已接收
	uint32_t 	total; 							// 总长度
	char   		url[MAX_URL];    				// URL

} __attribute__ ((packed)) TaskConfig;

typedef struct tagTaskRecord
{
	struct list_head list;
	
	int32_t 	taskId;							// 任务ID
	int32_t 	status;							// 状态	
	int32_t 	errorno;						// 错误码
	uint32_t 	received;     					// 下载有效字节数(可能存在回退的情况)	
	uint32_t 	total;         					// 该任务总大小(字节)
	char 		filename[MAX_PATH]; 			// 下载保存文件名.
	char 		url[MAX_URL];    				// 任务URL

} __attribute__ ((packed)) TaskRecord;

typedef struct tagTaskInfo
{
	int32_t 	taskId;							// 任务ID
	int32_t 	status;							// 状态	
	int32_t 	errorno;						// 错误码
	int32_t 	speed;             				// 即时速度(字节/秒)		
	uint32_t 	received;     					// 下载有效字节数(可能存在回退的情况)	
	uint32_t 	total;         					// 该任务总大小(字节)
	char 		filename[MAX_PATH]; 			// 下载保存文件名.
	char 		url[MAX_URL];    				// 任务URL

} __attribute__ ((packed)) TaskInfo;

typedef struct tagThreadInfo
{
	uint32_t 	begin;  			// 开始处
	uint32_t 	received; 			// 已接受
	uint32_t 	total; 				// 总长度
	
	CURL 		*curl;				// 
	pthread_t 	threadId;			// 
	int32_t 	try_times;  		//失败已经重试次数

} __attribute__ ((packed)) ThreadInfo;

typedef int (*task_func_exit_handler)(TaskInfo *ti);
typedef int (*task_status_handler)(TaskInfo *ti);
typedef int (*task_progress_handler)(TaskInfo *ti);

void sharedns_init(void);

void sharedns_cleanup(void);

int get_url_length(const char *url, unsigned int *length);

char* get_task_filename_dt(char *url);

int task_delete(void);

int task_start(TaskRecord *task_rec);

int task_stop(void);

int task_remove_tmp(char *filename);

bool task_running(void);

int task_idle(void);

int task_info(TaskInfo *ti);

int task_update(TaskInfo *ti);

void task_set_handler(task_func_exit_handler tfeh, task_status_handler tsh, task_progress_handler tph);

#endif // __TASK_H__

