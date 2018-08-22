
#include "network.h"
#include "task.h"
#include "db.h"

#include <sys/mman.h>
#include <unistd.h>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

// 共享dns处理对象
static CURLSH *sharedns_handle 		= NULL;
static char down_proxy[MAX_LINE] 	= { 0 }; // 代理
// 全局下载设置 
static int down_timeout 			= 30;    // 下载过程中的超时时间(s)
static int down_max_try_times	 	= 3;	 //下载线程失败最大重试次数

static TaskInfo taskInfo = {
	
	.taskId = 0,			// 任务ID
	.status = 0,			// 状态	
	.errorno = 0,			// 错误码
	.speed = 0,             // 即时速度(字节/秒)		
	.received = 0,     		// 下载有效字节数(可能存在回退的情况)	
	.total = 0,         	// 该任务总大小(字节)
	.filename = "", 		// 下载保存文件名.
	.url = ""    			// 任务URL
	
};

static char dt_filename[MAX_PATH] 		= { 0 };
static char dt_cfg_filename[MAX_PATH] 	= { 0 };

static unsigned char *mmap_dt 			= NULL; 			// 共享内存, ".dt"
static TaskConfig *mmap_dt_cfg 			= NULL;				// 共享内存, ".dt.cfg"

static ThreadInfo *thread_info 			= NULL;

static volatile bool down_task_pause 	= true;

static int task_support_range 			= -1;

static int def_task_func_exit(TaskInfo *ti)
{
	return 0;
}
static int def_task_status(TaskInfo *ti)
{
	return 0;
}
static int def_task_progress(TaskInfo *ti)
{
	return 0;
}

static task_func_exit_handler 	task_func_exit 	= def_task_func_exit;
static task_status_handler 		task_status	 	= def_task_status;
static task_progress_handler 	task_progress 	= def_task_progress;

void sharedns_init(void)
{
	CURLcode code = (CURLcode)curl_global_init(CURL_GLOBAL_ALL); (void)code;
	if (!sharedns_handle)  
	{
		sharedns_handle = curl_share_init();  
		curl_share_setopt(sharedns_handle, CURLSHOPT_SHARE, CURL_LOCK_DATA_DNS);  
	}
}
void sharedns_cleanup(void)
{
	if (sharedns_handle)
	{
		curl_share_cleanup(sharedns_handle);
		sharedns_handle = NULL;		
	}
	
	curl_global_cleanup();
}

static CURL* create_share_curl(void) 	// 创建curl对象
{
	CURL *curl_handle = curl_easy_init();
	curl_easy_setopt(curl_handle, CURLOPT_SHARE, sharedns_handle);	
	curl_easy_setopt(curl_handle, CURLOPT_DNS_CACHE_TIMEOUT, 60 * 5);
	
	return curl_handle;
}

static bool down_thread_stop(void)
{
	down_task_pause = true;
	
	if(thread_info && thread_info->received<thread_info->total)
	{
		if(thread_info->curl
		 &&thread_info->threadId
		 &&pthread_kill(thread_info->threadId, 0)!=ESRCH
		 &&pthread_join(thread_info->threadId, NULL)==0)
		{
			return true;
		}
	}
	
	return false;
}

static void down_thread_clear(void)
{
	if(thread_info)
	{
		if(thread_info->curl)
		{
			curl_easy_cleanup(thread_info->curl);
			thread_info->curl = NULL;
		}
		
		free(thread_info);
		thread_info = NULL;
	}
}

static void munmap_temp_file(void) // 卸载中间临时文件映射的内存
{
	if(mmap_dt)
	{
		munmap(mmap_dt, taskInfo.total);
		mmap_dt = NULL;
	}
	if(mmap_dt_cfg)
	{
		munmap(mmap_dt_cfg, sizeof(TaskConfig));
		mmap_dt_cfg = NULL;
	}
}
static int mmap_temp_file(void) // 卸载中间临时文件映射的内存
{
	int dt_fd = -1;
	int dt_cfg_fd = -1;
	
	dt_fd = open(dt_filename, O_CREAT|O_RDWR, 0666);
	if(dt_fd < 0)
	{
		DBG_PRINTF("Cannot open file %s, Try again later.\n", dt_filename);
		return -1;
	}
	if(ftruncate(dt_fd, taskInfo.total) < 0)
	{
		DBG_PRINTF("Cannot truncate file %s, Try again later.\n", dt_filename);
		return -1;
	}
	mmap_dt = (unsigned char *)mmap(NULL, taskInfo.total, PROT_READ|PROT_WRITE, MAP_SHARED, dt_fd, 0);
	if((unsigned char *)-1 == mmap_dt)
	{
		return -1;
	}
	if(close(dt_fd) < 0)
	{
		DBG_PRINTF("Cannot close file %s, Try again later.\n", dt_filename);
		return -1;
	}
	
	dt_cfg_fd = open(dt_cfg_filename, O_CREAT|O_RDWR, 0666);
	if(dt_cfg_fd < 0)
	{
		DBG_PRINTF("Cannot open file %s, Try again later.\n", dt_cfg_filename);
		return -1;
	}
	if(ftruncate(dt_cfg_fd, sizeof(TaskConfig)) < 0)
	{
		DBG_PRINTF("Cannot truncate file %s, Try again later.\n", dt_cfg_filename);		
		return -1;
	}
	mmap_dt_cfg = (TaskConfig *)mmap(NULL, sizeof(TaskConfig), PROT_READ|PROT_WRITE, MAP_SHARED, dt_cfg_fd, 0);
	if((TaskConfig *)-1 == mmap_dt_cfg)
	{
		return -1;
	}
	if(close(dt_cfg_fd) < 0)
	{
		DBG_PRINTF("Cannot close file %s, Try again later.\n", dt_cfg_filename); 		
		return -1;
	}
	
	return 0;
}

static bool check_ssl(CURL *curl, const char *url)
{
	
	return true;
}

static void sleep_ex(int msec)
{
	int milliseconds = 0;
	while(milliseconds < msec)
	{
		if(down_task_pause)
		{		
			DBG_PRINTF("sleep_ex Pause occurred \n");
			break;
		}
		
		usleep(100*1000);
		milliseconds += 100;
	}
}

int get_url_length(const char *url, unsigned int *length)
{
	if(NULL == url)
	{
		return -1;
	}
	
	double file_length = -1.0;
	int try_times = 0;
	CURL *curl = NULL;
	CURLcode code;
	long lCode = 0;
	char *redirect_url = NULL;
	
retry0:
	curl = create_share_curl();
	
retry1:
	if (0 != strlen(down_proxy))
	{
		curl_easy_setopt(curl, CURLOPT_PROXY, down_proxy);
	}
	if (!check_ssl(curl, url_strip_param(url))) // 支持SSL 
	{
		return -1;
	}
	
	curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "GET"); // 兼顾重定向-强行用GET方法
	curl_easy_setopt(curl, CURLOPT_URL, url_strip_param(url));
	curl_easy_setopt(curl, CURLOPT_HEADER, 1); 
	curl_easy_setopt(curl, CURLOPT_NOBODY, 1);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, down_timeout); // 整个请求超时控制
	curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L); // 优化性能，防止超时崩溃 
	
	code = (CURLcode)curl_easy_perform(curl);
	if (code == CURLE_OK)
	{
		curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &lCode);
		if (lCode == 301 || lCode == 302)
		{
			curl_easy_getinfo(curl, CURLINFO_REDIRECT_URL, &redirect_url);
			url = redirect_url;
			goto retry1;
		}
		else if(lCode >= 200 && lCode < 300)
		{
			curl_easy_getinfo(curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &file_length); 
		}
	}
	
	if (curl)
	{
		curl_easy_cleanup(curl);
		curl = NULL;
	}
	
	if (file_length < 1.0) // 0字节文件大小也要重试
	{
		if (lCode == 404) // 资源不存在,就不重试了
		{
			DBG_PRINTF("URL resource does not exist! CURLcode=%d, HttpCode: %d \n", code, lCode);
			return -1;
		}
		if(CURLE_COULDNT_RESOLVE_HOST == code)
		{
			DBG_PRINTF("Couldn't resolve host. The given remote host was not resolved. CURLcode=%d, HttpCode: %d \n", code, lCode);
			return -1;
		}
		
		if (try_times < down_max_try_times) // 重试获取文件长度
		{
			try_times++;
			DBG_PRINTF("获取文件长度失败, lastHttpCode:%d, CURLcode:%d, 尝试第%d次重试 \n", lCode, code, try_times);
			
			if (network_get_connected()) // 若断网，则不必重试 
			{
				float val = pow(2.0, try_times);
				usleep(val*1000*1000);
				
				goto retry0;
			}
		}
		else
		{
			DBG_PRINTF("重试获取文件长度失败, LastHttpCode:%d \n", lCode);
			return -1;
		}
	}
	
	if(length)
	{
		*length = (unsigned int)file_length;
	}
	
	DBG_PRINTF("file_length: %u \n", (unsigned int)file_length);
	return 0;
}

static size_t header_info(char *ptr, size_t size, size_t nmemb, void *userdata) // 头信息 , 得出 是否支持 分片传输, 断点续传
{
	char *header = (char*)userdata;
	if(header)
	{
		strncat(header, ptr, size*nmemb);
	}
	
	return size*nmemb;
}

// -1 -- 错误, 0 -- 不支持, 1 -- 支持分片传输
static int check_support_range(char *url)	// 检测是否支持分片传输
{
	if(NULL == url)
	{
		return -1;
	}
	
	int range = 0;
	int try_times = 0;
	char header[4*MAX_LINE] = { 0 };
	CURL *curl;
	CURLcode code;
	float val;
	
	if (taskInfo.total > 0)
	{
		try_times = 0;
		
retry:
		if (down_task_pause)
		{
			DBG_PRINTF("遭到暂停 down_task_pause: %s \n", down_task_pause?"true":"false");
			return -1;
		}
		
		curl = create_share_curl();
		if (0 != strlen(down_proxy))
		{
			curl_easy_setopt(curl, CURLOPT_PROXY, down_proxy);
		}
		if(!check_ssl(curl, url_strip_param(url)))
		{
			return -1;
		}
		curl_easy_setopt(curl, CURLOPT_URL, url_strip_param(url));
		curl_easy_setopt(curl, CURLOPT_HEADER, 1); 
		curl_easy_setopt(curl, CURLOPT_NOBODY, 1);
		// 通过头部信息判断是否支持断点续传
		memset(header, 0, sizeof(header));
		curl_easy_setopt(curl, CURLOPT_HEADERDATA, header);
		curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, header_info);
		curl_easy_setopt(curl, CURLOPT_RANGE, "0-");
		// 整个请求超时控制
		curl_easy_setopt(curl, CURLOPT_TIMEOUT, down_timeout); 
		// 优化性能，防止超时崩溃 
		curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
		code = (CURLcode)curl_easy_perform(curl);
		curl_easy_cleanup(curl);
		curl = NULL;
		
		if (code == CURLE_OK)
		{
			if(NULL != strstr(header, "Content-Range: bytes")
			|| NULL != strstr(header, "Accept-Ranges: bytes") )
			{
				range = 1;
			}
		}
		else
		{
			if (try_times < down_max_try_times) // 重试判断是否支持多线程 
			{
				DBG_PRINTF("To judge whether to support the range of transmission. CURLcode: %d, iTryTimes: %d", code, try_times);
				if (!down_task_pause && network_get_connected()) // 没有暂停 且 网络连接 , 则 需重试 
				{
					val = pow(2.0, try_times); // 重试时间：一秒，两秒，四秒 
					sleep_ex(val*1000);
					
					try_times++;
					goto retry;
				}
			}
		}
	}
	
	DBG_PRINTF("range: %d \n", range);
	return range;
}

static int msync_temp_file(ThreadInfo *ti)
{
	mmap_dt_cfg->begin = ti->begin;
	mmap_dt_cfg->received = ti->received;
	mmap_dt_cfg->total = ti->total;
	
	msync(mmap_dt_cfg, sizeof(TaskConfig), MS_SYNC);
	msync(mmap_dt, taskInfo.total, MS_SYNC);
	
	return 0;
}

static size_t write_data(char *ptr, size_t size, size_t nmemb, void *userdata)	//写数据
{
	uint32_t start = 0;
	uint32_t unreceived = 0;
	uint32_t to_write = 0;
	
	if(!thread_info)
	{
		return size*nmemb - 1;
	}
	
	if(thread_info->received < thread_info->total)
	{
		start = thread_info->begin + thread_info->received;
		unreceived = thread_info->total - thread_info->received;
		to_write = min((uint32_t)(size*nmemb), unreceived);
		memcpy(mmap_dt+start, ptr, to_write); // 写入共享内存区域
		thread_info->received += to_write; 
	}
	
	taskInfo.received += to_write;
	
	msync_temp_file(thread_info);
	
	if(down_task_pause)
	{
		if(!task_support_range) // 如果不支持分片传输时,就置0,重新下载
		{
			thread_info->received = 0;
			taskInfo.received = 0;
		}
		
		return size*nmemb - 1;
	}
	
	return size*nmemb; 
}
static int progress_info(void *userdata, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow) // 下载进度,可以得到 即时速度
{
	// 上传数据
	(void)ultotal;    
	(void)ulnow;
	
	static time_t last = 0;
    time_t now = time(NULL);	
	if (now - last < 1)	
	{
		return 0;	
	}
	last = now;
	
	// Defaults to bytes/second
	double speed = 0;
	curl_easy_getinfo(thread_info->curl, CURLINFO_SPEED_DOWNLOAD, &speed);
	
	if(speed < 1.1)
	{
		taskInfo.speed = 0;
	}
	else
	{
		taskInfo.speed = (int32_t)speed;
	}
	
	DBG_PRINTF("received/total(B):%d/%d, dlnow/dltotal(B):%d/%d, Percent(%):%d, Remaining(s):%d, Speed(B/s):%d \n", 
		taskInfo.received, taskInfo.total,
		dlnow, dltotal,
		taskInfo.received*100/taskInfo.total,
		(int)((taskInfo.total-taskInfo.received)/(max(taskInfo.speed, 1))),
		taskInfo.speed);
	
	task_progress(&taskInfo);
	
	return 0;
}

char* get_task_filename_dt(char *url)
{
	static char dt_file_name[MAX_PATH]		= { 0 };
	memset(dt_file_name, 0, sizeof(dt_file_name));
	
	sprintf(dt_file_name, DOWN_TASK_PATH"/%s.%s", get_md5sum(url_strip_param(url)), DOWNTASK_FILE_TMP_DT);
	
//	DBG_PRINTF("dt_file_name: %s \n", dt_file_name);
	return dt_file_name;
}

static int complete_proc(void) // 下载完, 处理临时文件,共享内存复位
{
	if(taskInfo.received==taskInfo.total
	&& NULL != thread_info
	&& thread_info->received==thread_info->total)
	{
		taskInfo.status = DT_COMPLETE;
		taskInfo.errorno = TASK_ERROR_UNKNOWN;
		
		munmap_temp_file();
		
		remove(dt_cfg_filename);
	}
	
	return 0;
}

static int curl_init(void)
{
	CURL *_curl;
	
	if(thread_info)
	{
		_curl = create_share_curl();
		if(!_curl)
		{
			return -1;
		}
		
		if(0 != strlen(down_proxy))
		{
			curl_easy_setopt(_curl, CURLOPT_PROXY, down_proxy);
		}
		curl_easy_setopt(_curl, CURLOPT_URL, url_strip_param(taskInfo.url));
		check_ssl(_curl, taskInfo.url);
		
		curl_easy_setopt(_curl, CURLOPT_WRITEFUNCTION, &write_data);  
		curl_easy_setopt(_curl, CURLOPT_WRITEDATA, thread_info); 
		curl_easy_setopt(_curl, CURLOPT_FOLLOWLOCATION, 1L); 
		curl_easy_setopt(_curl, CURLOPT_NOSIGNAL, 1L);  
		curl_easy_setopt(_curl, CURLOPT_LOW_SPEED_LIMIT, 1L);  
		curl_easy_setopt(_curl, CURLOPT_LOW_SPEED_TIME, 30L); 
		curl_easy_setopt(_curl, CURLOPT_CONNECTTIMEOUT, down_timeout);
		curl_easy_setopt(_curl, CURLOPT_NOPROGRESS, 0L);
		curl_easy_setopt(_curl, CURLOPT_XFERINFOFUNCTION, &progress_info);
		curl_easy_setopt(_curl, CURLOPT_XFERINFODATA, thread_info);
		
		thread_info->curl = _curl;
		thread_info->try_times = 0;
		
		return 0;
	}
	
	return -1;
}

static void* down_func(void *param)
{
	bool is_received = false;
	CURLcode code = (CURLcode)CURLE_OK;
	uint32_t unreceived = 0;
	uint32_t begin_pos = 0;
	uint32_t end_pos = 0;
	int try_times = -1;
	
retry:
	if(!(thread_info->received < thread_info->total))
	{
		DBG_PRINTF("No more data available for download \n");
		goto down_thread_exit;
	}
	
	code = (CURLcode)CURLE_OK;
	unreceived = thread_info->total - thread_info->received;
	begin_pos = thread_info->begin + thread_info->received;
	end_pos = thread_info->begin + thread_info->total;
	if(task_support_range)
	{
		char range[32] = { 0 };
		sprintf(range, "%u-%u", begin_pos, end_pos-1); 
		DBG_PRINTF("range[X-Y]:[%s] \n", range);
		curl_easy_setopt(thread_info->curl, CURLOPT_RANGE, range);
	}
	else
	{
		thread_info->received = 0;
		taskInfo.received = 0;
	}
	
	taskInfo.status = DT_DOWNLOAD;
	task_update(&taskInfo);
	
	code = (CURLcode)curl_easy_perform(thread_info->curl);
	if(CURLE_OK != code)
	{
		DBG_PRINTF("Failed to receive data \n");
	}
	
	is_received = (unreceived>(thread_info->total-thread_info->received)); // 有时返回CURLE_OK，但却没接收到数据. true:收到一些数据
	if(is_received && CURLE_OK==code) // 成功
	{
		complete_proc();
		DBG_PRINTF("The file has been completely downloaded. Y(^_^)Y  \n");
	}
	else
	{
		// 若下载失败是由于暂停或中止或断网, 则不必重试 
		if(down_task_pause)
		{
			taskInfo.status = DT_PAUSE;
		}
		else if(!network_get_connected())
		{
			taskInfo.errorno = TASK_ERROR_TIMEOUT;
		}
		else
		{
			if(thread_info->try_times < down_max_try_times)
			{
				try_times = thread_info->try_times; // 重新初始化 重试次数要备份
				curl_easy_cleanup(thread_info->curl);
				thread_info->curl = NULL;
				if(0 == curl_init())
				{
					thread_info->try_times = try_times;
					float val = pow(2.0, thread_info->try_times);
					sleep_ex(val*1000);
					thread_info->try_times++;
					
					goto retry;
				}
			}
		}
	}
	
down_thread_exit:
	curl_easy_cleanup(thread_info->curl);
	thread_info->curl = NULL;
	
	task_func_exit(&taskInfo);
	
	return NULL;
}

static int create_down_thread(void)
{
	int ret = curl_init();
	if(ret < 0)
	{
		taskInfo.status = DT_ERROR;
		taskInfo.errorno = TASK_ERROR_CURL_PARAMETER;
		task_status(&taskInfo);
		return -1;
	}
	
	pthread_create(&thread_info->threadId, NULL, down_func, (void*)thread_info);
	
	return 0;
}

static int continue_download(void)
{
	bool info_valid = false; // 当前分块信息是否有效
	if(mmap_dt_cfg->total==taskInfo.total
	&& 0==strcmp(mmap_dt_cfg->url, taskInfo.url)
	&& mmap_dt_cfg->begin<taskInfo.total
	&& mmap_dt_cfg->received<taskInfo.total)
	{
		DBG_PRINTF("The thread block information is valid. \n");
		info_valid = true;
	}
	
	thread_info = (ThreadInfo*)malloc(sizeof(ThreadInfo));
	if(!thread_info)
	{
		return -1;
	}
	memset(thread_info, 0, sizeof(ThreadInfo));
	
	if(info_valid && task_support_range)
	{
		thread_info->begin = mmap_dt_cfg->begin;
		thread_info->received = mmap_dt_cfg->received;
		thread_info->total = mmap_dt_cfg->total;
		taskInfo.received = mmap_dt_cfg->received;
		
		DBG_PRINTF("断点续传 可以. \n");
	}
	else // 重新下载, 将参数复位至首次下载状态
	{
		memset(mmap_dt, 0, taskInfo.total);
		memset(mmap_dt_cfg, 0, sizeof(TaskConfig));
		mmap_dt_cfg->begin = 0;
		mmap_dt_cfg->received = 0;
		mmap_dt_cfg->total = taskInfo.total;
		strcpy(mmap_dt_cfg->url, taskInfo.url);
		
		thread_info->total = taskInfo.total;
		
		DBG_PRINTF("断点续传 不可以, 需要重新下载. \n");
	}
	
	return create_down_thread();
}

static bool task_pause(void)
{
	down_task_pause = true;
	
	if(thread_info && thread_info->received<thread_info->total)
	{
		if(thread_info->curl
		 &&thread_info->threadId
		 &&pthread_kill(thread_info->threadId, 0)!=ESRCH
		 &&pthread_join(thread_info->threadId, NULL)==0)
		{
			DBG_PRINTF("Pause download thread successfully \n");
			return true;
		}
	}
	
	DBG_PRINTF("The download thread was not started \n");
	return false;
}

static int task_clear(void)
{
	down_thread_stop();
	down_thread_clear();
	munmap_temp_file();
	
	return 0;
}

int task_start(TaskRecord *task_rec)
{
	if(NULL==task_rec || 0==strlen(task_rec->url))
	{
		return -2;
	}
	
	task_clear();
	
	int ret = -1;
	bool cfg_file_exists = false;
	
	taskInfo.taskId = task_rec->taskId;
	taskInfo.status = task_rec->status;
	taskInfo.errorno = task_rec->errorno;
	taskInfo.speed = 0;
	taskInfo.received = task_rec->received;
	taskInfo.total = task_rec->total;
	strcpy(taskInfo.filename, task_rec->filename);
	strcpy(taskInfo.url, task_rec->url);
	
	down_task_pause = false;
	
	ret = get_url_length(taskInfo.url, &taskInfo.total);
	if(ret < 0 || taskInfo.total < 1)
	{
		taskInfo.errorno = TASK_ERROR_RESOURCE_NOT_FOUND;
		taskInfo.status = DT_ERROR;		
		task_status(&taskInfo);
		
		return -1;
	}
	task_support_range = check_support_range(taskInfo.url);
	
	memset(dt_filename, 0, sizeof(dt_filename));
	memset(dt_cfg_filename, 0, sizeof(dt_cfg_filename));
	sprintf(dt_filename, 	 DOWN_TASK_PATH"/%s.%s", taskInfo.filename, DOWNTASK_FILE_TMP_DT);
	sprintf(dt_cfg_filename, DOWN_TASK_PATH"/%s.%s", taskInfo.filename, DOWNTASK_FILE_TMP_DT_CFG);	
	
	if(0 == access(dt_cfg_filename, F_OK))
	{
		cfg_file_exists = true;
	}
	
	ret = mmap_temp_file();
	if(ret < 0)
	{
		taskInfo.status = DT_ERROR;
		taskInfo.errorno = TASK_ERROR_DISK_CREATE;
		task_status(&taskInfo);
		
		return -1;
	}
	
	if(!cfg_file_exists) // 没有下载过
	{
		mmap_dt_cfg->begin = 0;
		mmap_dt_cfg->received = 0;
		mmap_dt_cfg->total = taskInfo.total;
		strcpy(mmap_dt_cfg->url, taskInfo.url);
		
		msync(mmap_dt_cfg, sizeof(TaskConfig), MS_SYNC);
		
		thread_info = (ThreadInfo*)malloc(sizeof(ThreadInfo));
		if(!thread_info)
		{
			taskInfo.status = DT_ERROR;
			taskInfo.errorno = TASK_ERROR_DISK_WRITE;
			task_status(&taskInfo);
			
			return -1;
		}
		
		memset(thread_info, 0, sizeof(ThreadInfo));
		thread_info->begin = 0;
		thread_info->received = 0;
		thread_info->total = taskInfo.total;
		
		return create_down_thread();
	}
	else // 根据上次的临时文件, 进行断点续传
	{
//		DBG_PRINTF("根据上次的临时文件, 进行断点续传 \n");
		return continue_download();
	}
}

int task_delete(void)
{
	task_stop();
	remove(dt_filename);
	remove(dt_cfg_filename);
	
	return 0;
}

int task_stop(void)
{
	task_pause();
	down_thread_clear();
	munmap_temp_file();
	
	return 0;
}

int task_remove_tmp(char *filename)
{
	char tmp_dt[MAX_PATH] = { 0 };
	char tmp_dt_cfg[MAX_PATH] = { 0 };
	
	sprintf(tmp_dt, DOWN_TASK_PATH"/%s.%s", filename, DOWNTASK_FILE_TMP_DT);	
	sprintf(tmp_dt_cfg, DOWN_TASK_PATH"/%s.%s", filename, DOWNTASK_FILE_TMP_DT_CFG);	
	
	if(0 == access(tmp_dt, F_OK))
		remove(tmp_dt);
	
	if(0 == access(tmp_dt_cfg, F_OK))
		remove(tmp_dt_cfg);
	
	return 0;
}

bool task_running(void)
{
	if(thread_info && thread_info->threadId)
	{
		int pthread_kill_err = pthread_kill(thread_info->threadId, 0);
		if(ESRCH!=pthread_kill_err && EINVAL!=pthread_kill_err)
		{
			return true;
		}
	}
	
	return false;
}

int task_idle(void)
{
	down_task_pause = true;
	down_thread_clear();
	munmap_temp_file();
	
	taskInfo.taskId = 0;
	taskInfo.status = DT_NOITEM;
	taskInfo.errorno = TASK_ERROR_UNKNOWN;
	taskInfo.speed = 0;
	taskInfo.received = 0;
	taskInfo.total = 0;
	
	memset(taskInfo.filename, 0, sizeof(taskInfo.filename));
	memset(taskInfo.url, 0, sizeof(taskInfo.filename));
	
	return 0;
}

int task_info(TaskInfo *ti)
{
	if(NULL == ti)
	{
		return -1;
	}
	
	ti->taskId = taskInfo.taskId;
	ti->status = taskInfo.status;
	ti->errorno = taskInfo.errorno;
	ti->speed = taskInfo.speed;
	ti->received = taskInfo.received;
	ti->total = taskInfo.total;
	strcpy(ti->filename, taskInfo.filename);
	strcpy(ti->url, taskInfo.url);
	
	return 0;
}

int task_update(TaskInfo *ti)
{
	TaskRecord task_rec = { {0} };
	task_rec.taskId = ti->taskId;
	task_rec.status = ti->status;
	task_rec.errorno = ti->errorno;
	task_rec.received = ti->received;
	task_rec.total = ti->total;
	strcpy(task_rec.filename, ti->filename);
	strcpy(task_rec.url, ti->url);
	
	return db_task_update(&task_rec);
}

void task_set_handler(task_func_exit_handler tfeh, task_status_handler tsh, task_progress_handler tph)
{
	if(tfeh)
	{
		task_func_exit = tfeh;
	}
	if(tsh)
	{
		task_status = tsh;
	}
	if(tph)
	{
		task_progress = tph;
	}	
}

