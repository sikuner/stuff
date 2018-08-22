
#include "types.h"
#include "conf.h"
#include "log.h"
#include "common.h"
#include "uri_parse.h"
#include "input_manager.h"
#include "view_manager.h"
#include "application.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <linux/input.h>
#include <string.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <json/json_object.h>
#include <json/json_tokener.h>

int tv_diff(struct timeval *t1, struct timeval *t2)
{
	return
		(t1->tv_sec - t2->tv_sec) * 1000 +
		(t1->tv_usec - t2->tv_usec) / 1000;
}

void trim(char *src)
{
	char *begin = src;
	char *end   = src;
	
	while (*end++);
	
	if (begin == end) 
		return;
	
	while (*begin==' ' || *begin=='\t' || *begin=='\n')
		++begin;
	
	while (*end=='\0' || *end==' ' || *end=='\t'|| *end=='\n')
		--end;
	
	if (begin > end)
	{
		*src = '\0';
		return;
	}
	
	while (begin != end)
		*src++ = *begin++;
	
	*src++ = *end;
	*src = '\0';
	
	return;
}

int get_sysbeeba_value(const char *path)
{
	int fd, value, rc;
	char buffer[32] = { 0 };
	
	fd = open(path, O_RDONLY);
	if(fd < 0)
	{
		fprintf(stderr, "Failed to open %s \n", path);
		return -1;
	}
	
	rc = read(fd, buffer, sizeof(buffer));
	if(rc < 0)
	{
		fprintf(stderr, "Failed to read %s, strerror(%d): %s \n", path, errno, strerror(errno));
		return -1;
	}
	
	value = atoi(buffer);
	
	if(fd > 0)
	{
		close(fd);
		fd = -1;
	}
	
//	fprintf(stderr, "get_sysbeeba_value path: %s, value: %d \n", path, value);
	
	return value;
}

int get_alarm_start(void)
{
	int state = 1;
	char line[MAX_LINE] = { 0 };
	
	FILE *pp = popen("player_time.sh get_start", "r");
	if (NULL == pp) 
		return -1;
	
	while(NULL != fgets(line, sizeof(line), pp))
	{
		fprintf(stderr, "get_alarm_start111: %s \n", line);
		trim(line);
//		fprintf(stderr, "get_alarm_start222: %s, len: %d \n", line, strlen(line));
		if(0==strcmp(line, "OFF") || 0==strcmp(line, "0"))
		{	
			state = 0;
			break;
		}
	}
	
	pclose(pp);
	pp = NULL;
	
	fprintf(stderr, "get_alarm_start state: %d \n", state);
	return state;
}

int get_alarm_stop(void)
{
	int state = 1;
	char line[MAX_LINE] = { 0 };
	
	FILE *pp = popen("player_time.sh get_stop", "r");
	if (NULL == pp) 
		return -1;
	
	while(NULL != fgets(line, sizeof(line), pp))
	{
		fprintf(stderr, "get_alarm_stop111: %s \n", line);	
		trim(line);		
//		fprintf(stderr, "get_alarm_stop222: %s, len: %d \n", line, strlen(line));
		if(0==strcmp(line, "OFF") || 0==strcmp(line, "0"))
		{
			state = 0;
			break;
		}
	}
	
	pclose(pp);
	pp = NULL;
	
	fprintf(stderr, "get_alarm_stop state: %d \n", state);
	return state;
}

int get_linein_state(void)
{
	int result_linein = -1;
	
	int linein = -1;
	int count = 3;
	
	while (count-- > 0)
	{
		linein = get_sysbeeba_value(LINEIN_STATE_PATH);
		if(SND_LINEIN_IN==linein || SND_LINEIN_OUT==linein) 
		{
			result_linein = linein;
			break;
		}
		
		usleep(100*1000);
	}
	
	fprintf(stderr, "get_linein_state result_linein: %d \n", result_linein);
	return result_linein;
}

int get_charge_state(void)
{	
	int result_charge = -1;
	
	int charge = -1;
	int count = 3;
	
	while (count-- > 0)
	{
		charge = get_sysbeeba_value(POWER_CHARGE_PATH);
		if(PWR_CHARGING==charge || PWR_CHARGE_FULL==charge || PWR_UNCHARGE==charge) 
		{
			result_charge = charge;
			break;
		}
		
		usleep(100*1000);
	}
	
	DBG_PRINTF("result_charge: %d \n", result_charge);
	return result_charge;
}

int get_battery_voltage(void)
{
	int sample_max  = 6;	// 最多采样6次
	int valid_count = 3; 	// 合法值取3次
	int val = -1;
	int result = -1;
	int value[3] = { 0 };
	
	int sample_index = 0;
	int valid_index = 0;
	int min_val = 0;
	int max_val = 0;
	
	while(sample_index < sample_max)
	{
		if(valid_index < valid_count)
		{
			val = get_sysbeeba_value(POWER_VOLTAGE_PATH);	// voltage * 3.0 * 3.3 / 256.0; // 值=>电压 算法		
			if(val > 130 && val < 300) // 130 * 3.0 * 3.3 / 256.0 = 5.03, 无论是充电还是放电,根据曲线都是大于5V的
			{
				value[valid_index] = val;
				valid_index++;
			}
		}
		
		usleep(150*1000);
		sample_index++;
	}
	
	if(valid_index == valid_count)
	{
		min_val = min(value[0], value[1]);
		min_val = min(min_val, value[2]);
		max_val = max(value[0], value[1]);
		max_val = max(max_val, value[2]);
		
		result = value[0] + value[1] + value[2] - min_val - max_val;
	}
	
//	fprintf(stderr, "get_battery_voltage result: %d \n", result);
	return result;
}

int parse_xf_result(char *filename, char *content, int *rc, char *text, char *strerrcode) // 获取 讯飞听写 结果
{
	if(NULL == filename)
		return -1;
	
	int ret = -1;
	
	int fd = -1;
	int file_length = -1;	
	int mem_length = -1;
	struct stat tStat;
	char *mm = NULL;
	char *content0 = NULL;
	
	struct json_object *content_obj = NULL;
	struct json_object *rc_obj = NULL;
	struct json_object *text_obj = NULL;	
	struct json_object *error_obj = NULL;
	struct json_object *code_obj = NULL;
	
	int rc0 = -1;
	const char *text0 = NULL;
	const char *code = NULL;
	
	fd = open(filename, O_RDONLY);
	if (fd < 0)
	{
		fprintf(stderr, "can't open %s. \n", filename);
		return -1;
	}
	
	if(fstat(fd, &tStat) < 0)
	{
		fprintf(stderr, "can't get fstat. \n");
		goto xf_exit;
	}
	file_length = tStat.st_size;
	
	mm = (char *)mmap(NULL , file_length, PROT_READ, MAP_SHARED, fd, 0);
	if (mm == (char *)-1)
	{
		fprintf(stderr, "can't mmap file \n");
		goto xf_exit;
	}
	
	mem_length = tStat.st_size + 1;
	content0 = (char *)malloc(mem_length);
	if(!content0)
	{
		fprintf(stderr, "Out of memory! \n");
		goto xf_exit;
	}
	memset(content0, 0, mem_length);
	memcpy(content0, mm, file_length);
	
	if(content)
	{
		strcpy(content, content0);
	}
	
	content_obj = json_tokener_parse(content0);	
	if (!content_obj)
	{
		fprintf(stderr, "Failed to json_tokener_parse() \n");
		goto xf_exit;	
	}
	rc_obj = json_object_object_get(content_obj, "rc");
	if (!rc_obj)
	{
		fprintf(stderr, "Failed to json_object_object_get() \n");
		goto xf_exit;	
	}
	
	rc0 = json_object_get_int(rc_obj);
	if(rc)
	{
		*rc = rc0;
	}
	
	if(0==rc0 || 4==rc0) // 正确,可以获取听写结果文本
	{
		text_obj = json_object_object_get(content_obj, "text");
		if (!text_obj)
		{
			fprintf(stderr, "Failed to json_object_object_get() \n");
			goto xf_exit;
		}
		text0 = json_object_get_string(text_obj);
		if (!text0)
		{
			fprintf(stderr, "Failed to json_object_get_string() \n");
			goto xf_exit;
		}
		if(text)
		{
			strcpy(text, text0);
		}
	}
	else // 1, 2, 3.  获取错误码
	{
		error_obj = json_object_object_get(content_obj, "error");
		if (!text_obj)
		{
			fprintf(stderr, "Failed to json_object_object_get() \n");
			goto xf_exit;
		}
		code_obj = json_object_object_get(error_obj, "code");
		if (!code_obj)
		{
			fprintf(stderr, "Failed to json_object_object_get() \n");
			goto xf_exit;
		}
		code = json_object_get_string(code_obj);
		if (!code)
		{
			fprintf(stderr, "Failed to json_object_get_string() \n");
			goto xf_exit;
		}
		if(strerrcode) 
		{
			strcpy(strerrcode, code);
		}
	}
	
	fprintf(stderr, " content: %s \n", content0);
	fprintf(stderr, " rc: %d \n", rc0);
	fprintf(stderr, " text: %s \n", text0);
	fprintf(stderr, " strerrcode: %s \n", code);
	
	ret = 0; // 解析成功
	
xf_exit:
	if(content_obj)
	{
		json_object_put(content_obj);
		content_obj = NULL;
	}
	if (content0)
	{	
		free(content0);
		content0 = NULL;
	}
	if(mm)
	{
		munmap(mm, file_length);
		mm = NULL;
	}
	if(fd > 0)
	{
		close(fd);
		fd = -1;
	}
	
	return ret;
}

int get_od_result(char *filename, int *result, char *msg) // 获取 比巴云搜索的 结果
{
	if(NULL==filename || NULL==result || NULL==msg)
	{
		return -1;
	}
	
	FILE *filp = NULL;	
	long file_size = 0, read_size = 0;	
	char *content = NULL;
	struct json_object *content_obj = NULL, *od_result_obj = NULL, *od_msg_obj = NULL;
	int od_result = 0;
	const char *od_msg = NULL;
	
	filp  = fopen(filename, "rb");
	if(NULL == filp)
	{
		fprintf(stderr, "\nopen [%s] failed! \n", filename);
		return -1;
	}
	fseek(filp, 0, SEEK_END);
	file_size = ftell(filp);
	fseek(filp, 0, SEEK_SET);
	
	if(file_size < 6)
	{
		fprintf(stderr, "file_size < 6 \n");
		goto od_result_exit; 
	}
	
	content = (char *)malloc(file_size+1);
	if (NULL == content)
	{
		fprintf(stderr, "\nout of memory! \n");
		goto od_result_exit; 
	}
	memset(content, 0, file_size+1);
	
	read_size = fread((void *)content, 1, file_size, filp);
	if (read_size != file_size)
	{
		fprintf(stderr, "\nread [%s] error!\n", filename);
		goto od_result_exit;	
	}
	
	content_obj = json_tokener_parse(content);
	if (NULL == content_obj)
	{
		fprintf(stderr, "Failed to json_tokener_parse() \n");
		goto od_result_exit;	
	}
	
	od_result_obj = json_object_object_get(content_obj, "result");
	if (NULL == od_result_obj)
	{
		fprintf(stderr, "Failed to json_object_object_get() \n");
		goto od_result_exit;	
	}
	
	od_result = json_object_get_int(od_result_obj);
	
	od_msg_obj = json_object_object_get(content_obj, "msg");
	if (NULL == od_msg_obj)
	{
		fprintf(stderr, "Failed to json_object_object_get() \n");
		goto od_result_exit;	
	}
	od_msg = json_object_get_string(od_msg_obj);
	if (NULL == od_msg)
	{
		fprintf(stderr, "Failed to json_object_get_string() \n");
		goto od_result_exit;	
	}
	if(result)
	{
		*result = od_result;
	}
	if(msg)
	{
		strcpy(msg, od_msg);
	}
	
	fprintf(stderr, "get_od_result result: %d \n", od_result);
	fprintf(stderr, "get_od_result msg: %s \n", od_msg);
	
	if(content_obj)
	{
		json_object_put(content_obj);
		content_obj = NULL;
	}
	if(NULL != content)
	{
		free(content);
		content = NULL;
	}
	if (NULL != filp)
	{
		fclose(filp);
		filp = NULL;
	}
	
	return 0;
	
od_result_exit:
	if(content_obj)
	{
		json_object_put(content_obj);
		content_obj = NULL;
	}
	if(NULL != content)
	{
		free(content);
		content = NULL;
	}
	if (NULL != filp)
	{
		fclose(filp);
		filp = NULL;
	}
	
	return -1;
}

char* get_sn(void)
{
	static char sn[MAX_LINE] = { 0 };
	char line[MAX_LINE] = { 0 };
	
	memset(sn, 0, sizeof(sn));
	
	FILE *pp = popen("get_sn.sh", "r");
	if (NULL == pp) 
	{
		return NULL;
	}
	
	while(NULL != fgets(line, sizeof(line), pp))
	{
		trim(line);
		if(SN_LENGTH == strlen(line))
		{	
			strcpy(sn, line);
			break;
		}		
	}
	
	pclose(pp);
	pp = NULL;
	
	fprintf(stderr, "get_sn: %s \n", sn);
	return sn;
}

char* get_beeba_version(void)
{
	static char version[MAX_LINE] = { 0 };
	
	int fd, rc;
	char buffer[MAX_LINE] = { 0 };
	
	memset(version, 0, sizeof(version));
	
	fd = open(BEEBA_VERSION_PATH, O_RDONLY);
	if(fd < 0)
	{	
		fprintf(stderr, "Failed to open %s \n", BEEBA_VERSION_PATH);
		return NULL;
	}
	
	rc = read(fd, buffer, sizeof(buffer));
	if(rc < 0)
	{	
		fprintf(stderr, "Failed to read %s, strerror(%d): %s \n", BEEBA_VERSION_PATH, errno, strerror(errno));
		return NULL;
	}
	
	trim(buffer);
	strcpy(version, buffer);
	
	if(fd > 0)
	{
		close(fd);
		fd = -1;
	}
	
	fprintf(stderr, "get_beeba_version: %s \n", version);
	return version;
}

char* get_mode_strerror(void)
{
	static char mode_strerror[MAX_LINE] = { 0 };
	char *mode_str = NULL;
	
	switch(getApp()->curMode)
	{
	case MODE_WIFI:
		{
			mode_str = "W";
		}
		break;
	case MODE_BLUETOOTH:
		{
			mode_str = "B";
		}
		break;
	case MODE_LINEIN:
		{
			mode_str = "L";
		}
		break;
	default:
		{
			mode_str = "U";
		}
		break;
	}
	
	memset(mode_strerror, 0, sizeof(mode_strerror));
	sprintf(mode_strerror, STRING_UNSUPPORTED, mode_str);
	
	return (char*)mode_strerror;
}

int get_airplay_state(void) // > 0, 即 airplay已开启
{	
	int fd, rc;
	char buffer[MAX_LINE] = { 0 };
	int airplay_on = 0;
	
	fd = open(BEEBA_AIRPLAY_FLAG, O_RDONLY);
	if(fd < 0)
	{
		fprintf(stderr, "Failed to open %s \n", BEEBA_AIRPLAY_FLAG);
		return -1;
	}
	
	rc = read(fd, buffer, sizeof(buffer));
	if(rc < 0)
	{	
		fprintf(stderr, "Failed to read %s, strerror(%d): %s \n", BEEBA_AIRPLAY_FLAG, errno, strerror(errno));
		return -1;
	}
	
	if(NULL != strstr(buffer, "true"))
	{
		airplay_on = 1;
	}
	
	if(fd > 0)
	{
		close(fd);
		fd = -1;
	}
	
	fprintf(stderr, "get_airplay_state: %d \n", airplay_on);
	return airplay_on;
}

char* get_bt_name(void)
{
	static char bt_name[MAX_LINE] = { 0 };
	
	int fd, rc;
	char buffer[MAX_LINE] = { 0 };
	
	memset(bt_name, 0, sizeof(bt_name));
	
	fd = open(BT_NAME_FILE, O_RDONLY);
	if(fd < 0)
	{	
		fprintf(stderr, "Failed to open %s \n", BT_NAME_FILE);
		return NULL;
	}
	
	rc = read(fd, buffer, sizeof(buffer));
	if(rc < 0)
	{	
		fprintf(stderr, "Failed to read %s, strerror(%d): %s \n", BT_NAME_FILE, errno, strerror(errno));
		return NULL;
	}
	
	trim(buffer);
	strcpy(bt_name, buffer);
	
	if(fd > 0)
	{
		close(fd);
		fd = -1;
	}
	
	fprintf(stderr, "get_bt_name(): %s \n", bt_name);
	return bt_name;
}

int get_bt_state(void)
{
	int state = BT_STATE_UNKNOWN;
	char line[MAX_LINE] = { 0 };
	FILE *pp = NULL;
	
	pp = popen("bt.sh state 2>/dev/null", "r");
	if (NULL == pp) 
	{
		return -1;
	}
	
	while(NULL != fgets(line, sizeof(line), pp))
	{
		fprintf(stderr, " xxxxxxxxxxxxxxxx get_bt_state: %s \n", line);
		
		if(NULL != strstr(line, "stopped"))
		{
			state = BT_STATE_STOPPED;
			break;
		}
		else if(NULL != strstr(line, "started"))
		{
			state = BT_STATE_STARTED;
			break;
		}
		else if(NULL != strstr(line, "connected"))
		{
			state = BT_STATE_CONNECTED;
			break;
		}
	}
	
	pclose(pp);
	pp = NULL;
	
	return state;
}

int bt_prev(void)
{
	fprintf(stderr, " xxxx bt_prev() xxxx \n");
	
	pid_t pid = vfork();
	if (pid < 0)
	{
		fprintf(stderr, "fork error!");
		return -1;
	}
	if (pid > 0) // > 0, 处于父进程中
	{
		return 0;
	}
	
	execlp("bt.sh", "bt.sh", "prev", NULL);
	exit(1);
}

int bt_next(void)
{
	fprintf(stderr, " xxxx bt_next() xxxx \n");
	
	pid_t pid = vfork();
	if (pid < 0)
	{
		fprintf(stderr, "fork error!");
		return -1;
	}
	if (pid > 0) // > 0, 处于父进程中
	{
		return 0;
	}
	
	execlp("bt.sh", "bt.sh", "next", NULL);
	exit(1);
}

int bt_toggle(void)
{
	fprintf(stderr, " xxxx bt_next() xxxx \n");
	
	pid_t pid = vfork();
	if (pid < 0)
	{
		fprintf(stderr, "fork error!");
		return -1;
	}
	if (pid > 0) // > 0, 处于父进程中
	{
		return 0;
	}
	
	execlp("bt.sh", "bt.sh", "toggle", NULL);
	exit(1);
}

int bt_stop_nocb(void)
{
	pid_t pid = vfork();
	if (pid < 0)
	{
		fprintf(stderr, "fork error!");
		return -1;
	}
	if (pid > 0) // > 0, 处于父进程中. add后返回
	{
		return 0;
	}
	
	execlp("bt.sh", "bt.sh", "stop", NULL);
	exit(1);
}

int playsound(char *filename)
{
	if(NULL==filename || 0==strlen(filename))
	{
		return -1;
	}
	
	pid_t pid = vfork();
	if (pid < 0)
	{
		fprintf(stderr, "fork error!");
		return -1;
	}
	if (pid > 0) // > 0, 处于父进程中
	{
		return 0;
	}
	
	execlp("/usr/bin/playsound.sh", "playsound.sh", filename, NULL);
	exit(1);
}

int pam_open(void) // 进入linein模式，打开功放. echo 0 > /sys/beeba-detect/mute
{	
	pid_t pid = vfork();
	if (pid < 0)
	{
		fprintf(stderr, "fork error!");
		return -1;
	}
	if (pid > 0) // > 0, 处于父进程中
	{
		return 0;
	}
	
	execlp("/bin/sh", "/bin/sh", "-c", "echo 0 > /sys/beeba-detect/mute", NULL); 
	exit(1);
}

int pam_close(void) // 进入linein模式，打开功放. echo 0 > /sys/beeba-detect/mute
{	
	pid_t pid = vfork();
	if (pid < 0)
	{
		fprintf(stderr, "fork error!");
		return -1;
	}
	if (pid > 0) // > 0, 处于父进程中
	{
		return 0;
	}
	
	execlp("/bin/sh", "/bin/sh", "-c", "echo 1 > /sys/beeba-detect/mute", NULL); 
	exit(1);
}

int amp_close(void) // echo 0 > /sys/beeba-pmc/amp
{	
	fprintf(stderr, " xxxx amp_close # echo 0 > /sys/beeba-pmc/amp xxxx \n");
	
	pid_t pid = vfork();
	if (pid < 0)
	{
		fprintf(stderr, "fork error!");
		return -1;
	}
	if (pid > 0) // > 0, 处于父进程中
	{
		return 0;
	}
	
	execlp("/bin/sh", "/bin/sh", "-c", "echo 0 > /sys/beeba-pmc/amp", NULL); 
	exit(1);
}

int led_toggle(void)
{
	int fd, rc;
	char enable;
	
	fd = open(LED_ENABLE_PATH, O_RDWR);
	if(fd < 0)
	{
		fprintf(stderr, "Failed to open %s \n", LED_ENABLE_PATH);
		return -1;
	}
	
	rc = read(fd, &enable, sizeof(enable));
	if(rc < 0)
	{
		fprintf(stderr, "Failed to read %s \n", LED_ENABLE_PATH);
		return -1;
	}
	
	enable = (enable == '0') ? '1' : '0';
	
	rc = write(fd, &enable, sizeof(enable));
	if(rc < 0)
	{
		fprintf(stderr, "Failed to write %s \n", LED_ENABLE_PATH);
		return -1;
	}
	
	if(fd > 0)
	{
		close(fd);
		fd = -1;
	}
	
	return 0;
}

int led_state(void)
{
	int fd, rc;
	char enable;
	
	fd = open(LED_ENABLE_PATH, O_RDWR);
	if(fd < 0)
	{
		fprintf(stderr, "Failed to open %s \n", LED_ENABLE_PATH);
		return -1;
	}
	
	rc = read(fd, &enable, sizeof(enable));
	if(rc < 0)
	{
		fprintf(stderr, "Failed to read %s \n", LED_ENABLE_PATH);
		return -1;
	}
	
	if(fd > 0)
	{
		close(fd);
		fd = -1;
	}
	
	return (enable == '1') ? 1 : 0;
}

int led_on(void)
{
	int fd, rc;
	char enable;
	
	fd = open(LED_ENABLE_PATH, O_RDWR);
	if(fd < 0)
	{
		fprintf(stderr, "Failed to open %s \n", LED_ENABLE_PATH);
		return -1;
	}
	
	enable = '1';
	
	rc = write(fd, &enable, sizeof(enable));
	if(rc < 0)
	{
		fprintf(stderr, "Failed to write %s \n", LED_ENABLE_PATH);
		return -1;
	}
	
	if(fd > 0)
	{
		close(fd);
		fd = -1;
	}
	
	return 0;
}

int led_off(void)
{
	int fd, rc;
	char enable;
	
	fd = open(LED_ENABLE_PATH, O_RDWR);
	if(fd < 0)
	{
		fprintf(stderr, "Failed to open %s \n", LED_ENABLE_PATH);
		return -1;
	}
	
	enable = '0';
	
	rc = write(fd, &enable, sizeof(enable));
	if(rc < 0)
	{
		fprintf(stderr, "Failed to write %s \n", LED_ENABLE_PATH);
		return -1;
	}
	
	if(fd > 0)
	{
		close(fd);
		fd = -1;
	}
	
	return 0;
}

int start_debug(void) // 工厂调试模式
{
	pid_t pid = vfork();
	if (pid < 0)
	{
		fprintf(stderr, "fork error!");
		return -1;
	}
	if (pid > 0) // > 0, 处于父进程中
	{
		return 0;
	}
	
	execlp("/usr/bin/start_debug.sh", "/usr/bin/start_debug.sh", NULL); 
	exit(1);
}

int start_upgrade(void)
{
	pid_t pid = vfork();
	if (pid < 0)
	{
		fprintf(stderr, "fork error!");
		return -1;
	}
	if (pid > 0) // > 0, 处于父进程中
	{
		return 0;
	}
	
	execlp("/usr/bin/start_upgrade.sh", "start_upgrade.sh", NULL); 
	exit(1);
}

int delay_poweron(int sec) // 定时多少秒后 开机
{
	int rtc_fd, timer_fd;
	int rc;
	char buffer[MAX_LINE] = { 0 };
	long time = 0;	
	
	rtc_fd = open(RTC_PATH, O_RDONLY);
	if(rtc_fd < 0)
	{
		fprintf(stderr, "Failed to open %s \n", RTC_PATH);
		return -1;
	}
	
	rc = read(rtc_fd, buffer, sizeof(buffer));
	if(rc < 0)
	{	
		fprintf(stderr, "Failed to read %s, strerror(%d): %s \n", RTC_PATH, errno, strerror(errno));
		return -1;
	}
	
	if(rtc_fd > 0)
	{
		close(rtc_fd);
		rtc_fd = -1;
	}
	
	time = atol(buffer);
	DBG_PRINTF("rtc time: %ld", time);
	
	timer_fd = open(TIMER_PATH, O_RDWR);
	if(timer_fd < 0)
	{
		fprintf(stderr, "Failed to open %s \n", TIMER_PATH);
		return -1;
	}
	
	time += sec;
	memset(buffer, 0, sizeof(buffer));
	sprintf(buffer, "%ld", time);
	
	rc = write(timer_fd, buffer, sizeof(buffer));
	if(rc < 0)
	{
		fprintf(stderr, "Failed to write %s \n", TIMER_PATH);
		return -1;
	}
	
	if(timer_fd > 0)
	{
		close(timer_fd);
		timer_fd = -1;
	}
	
	return 0;
}

int parse_mpd_error(char *mpd_error, char *artist, char *title)
{
	if(!mpd_error || !artist || !title)
	{
		return -1;
	}
	
	char url[MAX_URL] = { 0 };
	char *tmp = NULL;
	
	tmp = strstr(mpd_error, "http://");
	if(!tmp)
	{
		return -1;
	}
	
	strcpy(url, tmp);
	return parse_artist_title(url, artist, title);
}

int mpd_kill(void)
{
	pid_t pid = vfork();
	if (pid < 0)
	{
		fprintf(stderr, "fork error!");
		return -1;
	}
	if (pid > 0) // > 0, 处于父进程中
	{
		return 0;
	}
	
	execlp("/usr/bin/mpd", "mpd", "--kill", NULL); 
	exit(1);
}

// <0 出错或没有更新, = 0 普通更新, > 0 强制更新
int check_upgrade(void)
{
	int ret;
	
	ret = access(FIRMWARE_VERSION_PATH, F_OK);
	DBG_PRINTF("version ret: %d \n", ret);
	if(0 != ret) // 文件不存在
		return -1;
	
	ret = get_sysbeeba_value(FIRMWARE_ENFORCE_PATH);
	if(ret > 0)
		ret = 1;
	
	DBG_PRINTF("en ret: %d \n", ret);
	return ret;
}

int get_uptime(struct timeval *tv)
{
	if(!tv)
		return -1;
	
	double uptime = 0.0;
	FILE *fp = NULL;
	
	fp = fopen ("/proc/uptime", "r");
	if (fp != NULL)
	{
		char buf[MAX_LINE];
		char *b = fgets (buf, MAX_LINE, fp);
		if (b == buf)
		{
			char *end_ptr;
			double upsecs = strtod (buf, &end_ptr);
			if (buf != end_ptr)
				uptime = upsecs > 0.0 ? upsecs : 0.0;
		}
		
		fclose (fp);
	}
	
	tv->tv_sec  = (time_t)uptime;
	tv->tv_usec = (uptime - tv->tv_sec) * 1000 * 1000;
	
	return 0;
}

// true - 有蓝牙设备, false - 没有蓝牙设备
bool check_has_bt(void)
{
	bool has_bt = true;
	
	FILE *pp = NULL;
	char line[MAX_LINE] = { 0 };
	
	pp = popen("uci get product.device.bluetooth", "r");
	if (pp)
	{
		while(NULL != fgets(line, sizeof(line), pp))
		{
			if(strstr(line, "false"))
			{
				has_bt = false;
				break;
			}
		}
		
		pclose(pp);
	}
	
 	return has_bt;
}

// 获取产品型号ID
char* get_model_pid(void)
{
	static char modelID[MAX_LINE] = { 0 };
	
	FILE *pp = NULL;
	char line[MAX_LINE] = { 0 };
	
	memset(modelID, 0, sizeof(modelID));
	pp = popen("get_real_pid.sh 2>/dev/null", "r");
	if (pp)
	{
		while(NULL != fgets(line, sizeof(line), pp))
		{
			if(strstr(line, "M"))
			{
				strcpy(modelID, line);
				break;
			}
		}
		
		pclose(pp);
	}
	
 	return modelID;
}

