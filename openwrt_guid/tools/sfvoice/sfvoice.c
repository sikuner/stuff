
#define _GNU_SOURCE
#include <stdio.h>
#include <malloc.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <fcntl.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <time.h>
#include <locale.h>
#include <alsa/asoundlib.h>
#include <assert.h>
#include <termios.h>
#include <signal.h>
#include <sys/poll.h>
#include <sys/uio.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <endian.h>
#include <stdbool.h>
#include <pthread.h>
#include <syslog.h>

#include "qisr.h"
#include "msp_cmn.h"
#include "msp_errors.h"

#ifndef LLONG_MAX
#define LLONG_MAX    2147483648LL
#endif

#ifndef le16toh
#include <asm/byteorder.h>
#define le16toh(x) __le16_to_cpu(x)
#define be16toh(x) __be16_to_cpu(x)
#define le32toh(x) __le32_to_cpu(x)
#define be32toh(x) __be32_to_cpu(x)
#endif

////////////////////////BEEBA BEGIN////////////////////////////
//#define PCM_NAME		"default"
#define PCM_NAME		"hw:0,2"

#define BITS			16   	// 量化位数
#define CHANNELS		1   	// 声道数目
#define RATE			16000 	// 采样频率

#define TIMELIMIT		10  	// 录音时间,秒

#define CHUNK_BYTES		1024   	// 每次获取字节数

static int byte_per_second 		= RATE*CHANNELS*BITS/8;
static int short_press_period 	= 500; 	// 单位: 毫秒. <0.5s, 表示短按
static int short_press_duration = 3000; // 单位: 毫秒. 短按录音3.0s
static int voice_short_flag 	= 0; 	// 短按范围标识

#define	VOICE_SHORT_FLAG 		"/tmp/voice_short_flag"
#define	VOICE_RECORDING_FLAG 	"/tmp/voice_recording_flag"

#define	XFYUN_RESULT_FILE		"/tmp/kw.txt"

////////////////////////BEEBA END////////////////////////////

enum {
	VUMETER_NONE,
	VUMETER_MONO,
	VUMETER_STEREO
};

static char *command = "sfvoice";
static snd_pcm_t *handle;
static char *pcm_name = PCM_NAME;
static struct {
	snd_pcm_format_t format;
	unsigned int channels;
	unsigned int rate;
} hwparams={.format = SND_PCM_FORMAT_S16_LE,.rate = RATE,.channels = CHANNELS};

static int timelimit = TIMELIMIT;
static int quiet_mode = 0;
static int open_mode = 0;
static snd_pcm_stream_t stream = SND_PCM_STREAM_CAPTURE;
static int mmap_flag = 0;
static int interleaved = 1;

static volatile int in_aborting = 0;
static volatile int sigusr2_pending = 0;

static u_char *audiobuf = NULL;
static snd_pcm_uframes_t chunk_size = CHUNK_BYTES;
static unsigned int period_time = 0;
static unsigned int buffer_time = 0;
static snd_pcm_uframes_t period_frames = 0;
static snd_pcm_uframes_t buffer_frames = 0;
static int avail_min = -1;
static int start_delay = 1;
static int stop_delay = 0;
static int monotonic = 0;
static int can_pause = 0;
static int fatal_errors = 0;
static int verbose = 0;
static int vumeter = VUMETER_STEREO;
static size_t bits_per_sample = 0, bits_per_frame = 0;
static size_t chunk_bytes = 0;
static int test_nowait = 0;
static snd_output_t *log = NULL;
static int dump_hw_params = 0;
static off64_t pbrec_count = LLONG_MAX, fdcount = 0;

//////////////////////////////////////////////////////////////////////////////////
//环形缓冲区//
//////////////////////////////////////////////////////////////////////////////////
#define CIRCLE_BUFFER_SIZE			(160*1024)
static unsigned char *circle_buffer = NULL;
static int circle_read_pos  		= 0;
static int circle_write_pos 		= 0;
static volatile int in_ending 		= 0;

static bool isFull(void)
{
	return (((circle_write_pos + 1) % CIRCLE_BUFFER_SIZE) == circle_read_pos);
}
static bool isEmpty(void)
{
	return (circle_write_pos == circle_read_pos);
}
static bool isEnd(void)
{
	return isEmpty()&&in_ending; // 当 empty和end 时, 表明该录音pcm流结束
}
static bool PutData(char cVal)
{
	if (isFull())
		return false;
	else
	{
		circle_buffer[circle_write_pos] = cVal;
		circle_write_pos = (circle_write_pos + 1) % CIRCLE_BUFFER_SIZE;
		return true;
	}	
}
static bool GetData(char *pcVal)
{
	if (isEmpty())
		return false;
	else
	{
		*pcVal = circle_buffer[circle_read_pos];
		circle_read_pos = (circle_read_pos + 1) % CIRCLE_BUFFER_SIZE;
		return true;
	}
}

//////////////////////////////////////////////////////////////////////////////////
// 讯飞听写, 读出环形缓冲区数据转换为文字
//////////////////////////////////////////////////////////////////////////////////

#define	BUFFER_SIZE	4096
#define FRAME_LEN	640 
#define HINTS_SIZE  100

const char *login_params = "appid = 536617d4, work_dir = /tmp"; 
const char *session_begin_params = "sub=iat,ptt=0,ssm=1,sch=1,nbs=1,auf=audio/L16;rate=16000,aue=raw,ent=video16k,rst=json,rse=utf8,nlp_version=2.0";

static off64_t calc_count(void);
static void set_params(void);
static ssize_t pcm_read(u_char *data, size_t rcount);
static void print_time(char *tag);
static int write_file(char *filename, char *content);

#define error(...) do {\
	fprintf(stderr, "%s: %s:%d: ", command, __FUNCTION__, __LINE__); \
	fprintf(stderr, __VA_ARGS__); \
	putc('\n', stderr); \
} while (0)

static void print_time(char *tag)
{
	struct timeval now;
	gettimeofday(&now, 0);
	fprintf(stderr, "%s time:%-24.24s.%06lu \n", tag, ctime(&now.tv_sec), now.tv_usec);
}

/*
 *	Subroutine to clean up before exit.
 */
static void prg_exit(int code) 
{
	if (handle)
		snd_pcm_close(handle);
	exit(code);
}

static void signal_handler(int sig)
{
	if (in_aborting)
		return;

	in_aborting = 1;
	if (verbose==2)
		putchar('\n');
	if (!quiet_mode)
		fprintf(stderr, "Aborted by signal %s...\n", strsignal(sig));
	if (handle)
		snd_pcm_abort(handle);
	if (sig == SIGABRT) {
		/* do not call snd_pcm_close() and abort immediately */
		handle = NULL;
		prg_exit(EXIT_FAILURE);
	}
	signal(sig, signal_handler);
}
/* call on SIGUSR2 signal. */
static void signal_handler_sigusr2 (int sig)
{
	sigusr2_pending = 1;
}

//////////////////////////////////////////////////////////////////////////////////
// 录音数据,写入环形缓冲区 //
//////////////////////////////////////////////////////////////////////////////////
static pthread_t record_threadId = (pthread_t)NULL;
static pthread_mutex_t circle_buffer_mutex  = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t circle_buffer_cond = PTHREAD_COND_INITIALIZER;

static int chunk_write(char *buffer, int nbyte)
{
	int i;
	
	for (i = 0; i < nbyte; i++)
		if (!PutData(buffer[i]))
			break;
	
	pthread_mutex_lock(&circle_buffer_mutex);
	pthread_cond_signal(&circle_buffer_cond);
	pthread_mutex_unlock(&circle_buffer_mutex);
	
	return i;
}

static int chunk_read(char *buffer, int nbyte)
{
	int result = 0;
	int count = nbyte;
	char ch;
	
	while (true)
	{
		while ((result<count) && GetData(&ch))
		{
			buffer[result] = ch;
			result++;
		}
		
		if((result<count) && !isEnd())
		{
			print_time("<cond_wait 111>");
			pthread_mutex_lock(&circle_buffer_mutex);
			pthread_cond_wait(&circle_buffer_cond, &circle_buffer_mutex); 
			pthread_mutex_unlock(&circle_buffer_mutex);
			print_time("<cond_wait 222>");
		}
		else
		{
			return result;
		}
	}
	
	return result;
}

static void* recording(void *args)
{
	print_time("<record start>");
	
	int count, rest, actual_count; 
	
	rest = actual_count = count = calc_count();
	
	set_params();
	
	/* capture */
	fdcount = 0;
	in_aborting = 0;
	while (rest > 0 && !in_aborting) 
	{
		size_t c = (rest <= (off64_t)chunk_bytes) ?
			(size_t)rest : chunk_bytes;
		size_t f = c * 8 / bits_per_frame;
		if (pcm_read(audiobuf, f) != f)
			break;
		
		chunk_write((char*)audiobuf, c);
		
		count -= c;
		rest -= c;
		fdcount += c;
		
		if(voice_short_flag && (fdcount>byte_per_second*short_press_period/1000))
		{
			voice_short_flag = 0;
			remove(VOICE_SHORT_FLAG);
		}
		
		if(sigusr2_pending)
		{
			if(fdcount < (byte_per_second*short_press_period/1000)) // 短按
				actual_count = (byte_per_second*short_press_duration/1000);
			else
				signal_handler(SIGINT);
			
			sigusr2_pending = 0;
		}
		if(fdcount > actual_count)
			signal_handler(SIGINT);
	}
	
	fprintf(stderr, " fdcount = %d \n", (int)fdcount);
	in_ending = 1;
	chunk_write(NULL, 0);
	
	if (handle != NULL) 
	{ 
		snd_pcm_drain(handle);
		snd_pcm_close(handle);
		handle = NULL;
	}
	
	remove(VOICE_RECORDING_FLAG);
	print_time("<record done>");
	return NULL;
}

static int init(void)
{
	int err;
	
	err = snd_pcm_open(&handle, pcm_name, stream, open_mode);
	if (err < 0) 
	{
		error("audio open error: %s", snd_strerror(err));
		return -1;
	}
	
	audiobuf = (u_char *)malloc(CHUNK_BYTES);
	if (audiobuf == NULL)
	{
		error("not enough memory");
		return -1;
	}
	
	circle_buffer = (unsigned char *)malloc(CIRCLE_BUFFER_SIZE);
	if (circle_buffer==NULL) 
	{
		error("not enough memory");
		return -1;
    }
	
	err = pthread_create(&record_threadId, NULL, recording, NULL);
	if(err != 0)
	{
		error("Failed to pthread_create() ");
		return -1;
	}
	
	return 0;
}

void record_wait(void)
{	
	if(record_threadId && pthread_kill(record_threadId, 0)!=ESRCH && pthread_join(record_threadId, NULL)==0)
	{
		record_threadId = (pthread_t)NULL;
	}
}

static void release(void)
{
	if (audiobuf != NULL) 
	{ 
		free(audiobuf);
		audiobuf = NULL; 
	}
	if (circle_buffer != NULL) 
	{ 
		free(circle_buffer);
		circle_buffer = NULL; 
	}
	if (handle != NULL) 
	{ 
		snd_pcm_drain(handle);
		snd_pcm_close(handle);
		handle = NULL;
	}
	
	if(0 == access(VOICE_SHORT_FLAG, F_OK))
		remove(VOICE_SHORT_FLAG);
	if(0 == access(VOICE_RECORDING_FLAG, F_OK))
		remove(VOICE_RECORDING_FLAG);
}

//////////////////////////////////////////////////////////////////////////////////
// 将内容写入指定文件
//////////////////////////////////////////////////////////////////////////////////

static int write_file(char *filename, char *content)
{
	if(NULL==filename || NULL==content)
	{
		return -1;
	}
	
	FILE *filp = fopen(filename, "w");
    if (filp == NULL) 
	{
        fprintf(stderr, "Open file error: %s \n", strerror(errno));
		return -1;
    }
	
	if (0 > fwrite(content, 1, strlen(content), filp)) 
	{
		perror("Write sound data file failed \n");
		return -1;
	}
	
	fclose(filp);
	
	return 0;
}

void iat_run()
{
	const char*		session_id					=	NULL;
	char			rec_result[BUFFER_SIZE]		=	{ 0 };	
	char			hints[HINTS_SIZE]			=	{ 0 };
	int 			result_size 				= 	BUFFER_SIZE - 1;
	int				aud_stat					=	MSP_AUDIO_SAMPLE_CONTINUE ;
	int				ep_stat						=	MSP_EP_LOOKING_FOR_SPEECH;	
	int				rec_stat					=	MSP_REC_STATUS_SUCCESS ;
	int				errcode						=	MSP_SUCCESS ;
	
	int				pcm_count					=	0;
	int				read_size					=	0;
	int 			counter = 0;
	
	char 			frame_buffer[2*FRAME_LEN] 	= 	{ 0 };
	int 			len 						= 	0;
	int 			ret 						= 	0;
	
	fprintf(stdout, "\n开始语音听写 ...\n");
	session_id = QISRSessionBegin(NULL, session_begin_params, &errcode);
	if (MSP_SUCCESS != errcode)
	{
		syslog(LOG_SYSLOG, "BEEBALOGwarning?e=QISRSessionBeginFailed&p=ret|%d", ret);
		error("Failed to QISRSessionBegin. errcode: %d", errcode);
		return;
	}
	while(true)
	{
		len = sizeof(frame_buffer);
		memset(frame_buffer, 0, len);
		read_size = chunk_read(frame_buffer, len);
		aud_stat = MSP_AUDIO_SAMPLE_CONTINUE;
		if (0 == pcm_count)
			aud_stat = MSP_AUDIO_SAMPLE_FIRST;
		fprintf(stderr, "<[%d]%d> \n", counter++, read_size);
		ret = QISRAudioWrite(session_id, (const void *)frame_buffer, len, aud_stat, &ep_stat, &rec_stat);
		if (MSP_SUCCESS != ret)
		{
			error("Failed to QISRAudioWrite ret: %d", ret);
			break;
		}
		pcm_count += len;
		if (MSP_REC_STATUS_SUCCESS == rec_stat) //已经有部分听写结果
		{
			const char *rslt = QISRGetResult(session_id, &rec_stat, 0, &errcode);
			if (MSP_SUCCESS != errcode)
			{
				syslog(LOG_SYSLOG, "BEEBALOGwarning?e=QISRGetResult_Failed&p=ret|%d", ret);				
				error("Failed to QISRGetResult. errcode: %d", errcode);
				break;
			}
			if (NULL != rslt)			
				strncat(rec_result, rslt, result_size);
		}
		if (MSP_EP_AFTER_SPEECH == ep_stat)
			break;
		if(read_size != len)
			break;
	}
	errcode = QISRAudioWrite(session_id, NULL, 0, MSP_AUDIO_SAMPLE_LAST, &ep_stat, &rec_stat);
	if (MSP_SUCCESS != errcode)
	{
		syslog(LOG_SYSLOG, "BEEBALOGwarning?e=QISRAudioWrite_Failed&p=ret|%d", ret);
		error("Failed to QISRAudioWrite. errcode: %d", errcode);
	}
	while (MSP_REC_STATUS_COMPLETE != rec_stat) 
	{
		const char *rslt = QISRGetResult(session_id, &rec_stat, 0, &errcode);
		if (MSP_SUCCESS != errcode)
		{
			error("Failed to QISRGetResult. errcode: %d", errcode);
			break;
		}
		if (NULL != rslt)
			strncat(rec_result, rslt, result_size);
		
		usleep(1000); //防止频繁占用CPU
	}
	ret = QISRSessionEnd(session_id, hints);
	if(ret != MSP_SUCCESS) 
	{
		error("Failed to QISRSessionEnd ret: %d", ret);
		syslog(LOG_SYSLOG, "BEEBALOGwarning?e=QISRSessionEnd_Failed&p=ret|%d", ret);
	}
	printf("\n语音听写结束\n");	
	print_time("IAT");
	printf("=============================================================\n");
	printf("%s\n", rec_result);
	printf("=============================================================\n");
	if(0 > write_file(XFYUN_RESULT_FILE, rec_result))
	{
		fprintf(stderr, "Failed to write_file() \n");
	}
}

int iat_test(void)
{
	const char*		session_id					=	NULL;
	char			hints[HINTS_SIZE]			=	{ 0 };
	int				aud_stat					=	MSP_AUDIO_SAMPLE_CONTINUE ;
	int				ep_stat						=	MSP_EP_LOOKING_FOR_SPEECH;	
	int				rec_stat					=	MSP_REC_STATUS_SUCCESS ;
	int				errcode						=	MSP_SUCCESS ;
	
	char 			frame_buffer[2*FRAME_LEN] 	= 	{ 0 };
	int 			len 						= 	0;
	int 			ret 						= 	0;
	
	session_id = QISRSessionBegin(NULL, session_begin_params, &errcode);
	if (MSP_SUCCESS != errcode)
	{
		syslog(LOG_SYSLOG, "BEEBALOGwarning?e=QISRSessionBeginFailed&p=ret|%d", ret);
		error("Failed to QISRSessionBegin. errcode: %d", errcode);
		return -1;
	}
	
	len = sizeof(frame_buffer);
	aud_stat = MSP_AUDIO_SAMPLE_FIRST;
	ret = QISRAudioWrite(session_id, (const void *)frame_buffer, len, aud_stat, &ep_stat, &rec_stat);
	if (MSP_SUCCESS != ret)
	{
		error("Failed to QISRAudioWrite ret: %d", ret);			
		return -1;
	}
	if (MSP_REC_STATUS_SUCCESS == rec_stat) //已经有部分听写结果
	{
		QISRGetResult(session_id, &rec_stat, 0, &errcode);
		if (MSP_SUCCESS != errcode)
		{
			syslog(LOG_SYSLOG, "BEEBALOGwarning?e=QISRGetResult_Failed&p=ret|%d", ret);				
			error("Failed to QISRGetResult. errcode: %d", errcode);				
			return -1;
		}
	}
	errcode = QISRAudioWrite(session_id, NULL, 0, MSP_AUDIO_SAMPLE_LAST, &ep_stat, &rec_stat);
	if (MSP_SUCCESS != errcode)
	{
		syslog(LOG_SYSLOG, "BEEBALOGwarning?e=QISRAudioWrite_Failed&p=ret|%d", ret);
		error("Failed to QISRAudioWrite. errcode: %d", errcode);		
		return -1;
	}
	while (MSP_REC_STATUS_COMPLETE != rec_stat) 
	{
		QISRGetResult(session_id, &rec_stat, 0, &errcode);
		if (MSP_SUCCESS != errcode)
		{
			error("Failed to QISRGetResult. errcode: %d", errcode);
			return -1;
		}
	}
	ret = QISRSessionEnd(session_id, hints);
	if(ret != MSP_SUCCESS) 
	{
		error("Failed to QISRSessionEnd ret: %d", ret);
		syslog(LOG_SYSLOG, "BEEBALOGwarning?e=QISRSessionEnd_Failed&p=ret|%d", ret);		
		return -1;
	}
	
	return 0;
}

// gcc -o iat_beeba iat_beeba.c -lmsc -lrt -ldl -lpthread -lasound
// gcc -o sfvoice sfvoice.c -lmsc -lrt -ldl -lpthread -lasound
// mips-linux-gnu-gcc -o iat_beeba iat_beeba.c -lmsc -lrt -ldl -lpthread -lasound
// 
int main(int argc, char *argv[])
{	
	signal(SIGUSR2, signal_handler_sigusr2);
	signal(SIGINT,  signal_handler);
	signal(SIGABRT, signal_handler);
	signal(SIGUSR1, signal_handler);
	
	int test = 0;
	int ret = 0;
	
	write_file(VOICE_RECORDING_FLAG, "");
	write_file(VOICE_SHORT_FLAG, "");
	write_file(XFYUN_RESULT_FILE, "");
	voice_short_flag = 1;
	
	if(2==argc && 0==strcmp(argv[1], "test"))
		test = 1;
	
	ret = MSPLogin(NULL, NULL, login_params);
	if (MSP_SUCCESS != ret)
	{
		fprintf(stderr, "MSPLogin failed , Error code %d. \n", ret);
		goto exit; 
	}
	
	if (1 == test) // 测试
	{
		ret = iat_test();		
		
		if(0 == ret)
			fprintf(stdout, "BEEBA_TEST_OK \n");
		else
			fprintf(stdout, "BEEBA_TEST_FAIL \n");
		
		MSPLogout();
		return 0;
	}
	
	ret = init();
	if (ret < 0) 
	{
		error("audio open error: %s", snd_strerror(ret));
		return -1;
	}
//	print_time("<init OK>");
	
	iat_run();
//	print_time("<iat_run Done>");
	
exit:
	record_wait();
	release();	
	MSPLogout(); 
	snd_config_update_free_global();
	
//	print_time("<main exit>");
	return 0;
}

static void show_available_sample_formats(snd_pcm_hw_params_t* params)
{
	snd_pcm_format_t format;

	fprintf(stderr, "Available formats:\n");
	for (format = 0; format <= SND_PCM_FORMAT_LAST; format++) {
		if (snd_pcm_hw_params_test_format(handle, params, format) == 0)
			fprintf(stderr, "- %s\n", snd_pcm_format_name(format));
	}
}

static void set_params(void)
{
	snd_pcm_hw_params_t *params;
	snd_pcm_sw_params_t *swparams;
	snd_pcm_uframes_t buffer_size;
	int err;
	size_t n;
	unsigned int rate;
	snd_pcm_uframes_t start_threshold, stop_threshold;
	snd_pcm_hw_params_alloca(&params);
	snd_pcm_sw_params_alloca(&swparams);
	err = snd_pcm_hw_params_any(handle, params);
	if (err < 0) {
		error("Broken configuration for this PCM: no configurations available");
		prg_exit(EXIT_FAILURE);
	}
	if (dump_hw_params) {
		fprintf(stderr, "HW Params of device \"%s\":\n",
			snd_pcm_name(handle));
		fprintf(stderr, "--------------------\n");
		snd_pcm_hw_params_dump(params, log);
		fprintf(stderr, "--------------------\n");
	}
	if (mmap_flag) {
		snd_pcm_access_mask_t *mask = alloca(snd_pcm_access_mask_sizeof());
		snd_pcm_access_mask_none(mask);
		snd_pcm_access_mask_set(mask, SND_PCM_ACCESS_MMAP_INTERLEAVED);
		snd_pcm_access_mask_set(mask, SND_PCM_ACCESS_MMAP_NONINTERLEAVED);
		snd_pcm_access_mask_set(mask, SND_PCM_ACCESS_MMAP_COMPLEX);
		err = snd_pcm_hw_params_set_access_mask(handle, params, mask);
	} else if (interleaved)
		err = snd_pcm_hw_params_set_access(handle, params,
						   SND_PCM_ACCESS_RW_INTERLEAVED);
	else
		err = snd_pcm_hw_params_set_access(handle, params,
						   SND_PCM_ACCESS_RW_NONINTERLEAVED);
	if (err < 0) {
		error("Access type not available");
		prg_exit(EXIT_FAILURE);
	}
	err = snd_pcm_hw_params_set_format(handle, params, hwparams.format);
	if (err < 0) {
		error("Sample format non available");
		show_available_sample_formats(params);
		prg_exit(EXIT_FAILURE);
	}
	err = snd_pcm_hw_params_set_channels(handle, params, hwparams.channels);
	if (err < 0) {
		error("Channels count non available");
		prg_exit(EXIT_FAILURE);
	}

	rate = hwparams.rate;
	err = snd_pcm_hw_params_set_rate_near(handle, params, &hwparams.rate, 0);
	assert(err >= 0);
	if ((float)rate * 1.05 < hwparams.rate || (float)rate * 0.95 > hwparams.rate) {
		if (!quiet_mode) {
			char plugex[64];
			const char *pcmname = snd_pcm_name(handle);
			fprintf(stderr, "Warning: rate is not accurate (requested = %iHz, got = %iHz)\n", rate, hwparams.rate);
			if (! pcmname || strchr(snd_pcm_name(handle), ':'))
				*plugex = 0;
			else
				snprintf(plugex, sizeof(plugex), "(-Dplug:%s)",
					 snd_pcm_name(handle));
			fprintf(stderr, "         please, try the plug plugin %s\n",
				plugex);
		}
	}
	rate = hwparams.rate;
	if (buffer_time == 0 && buffer_frames == 0) {
		err = snd_pcm_hw_params_get_buffer_time_max(params,
							    &buffer_time, 0);
		assert(err >= 0);
		if (buffer_time > 500000)
			buffer_time = 500000;
	}
	if (period_time == 0 && period_frames == 0) {
		if (buffer_time > 0)
			period_time = buffer_time / 4;
		else
			period_frames = buffer_frames / 4;
	}
	if (period_time > 0)
		err = snd_pcm_hw_params_set_period_time_near(handle, params,
							     &period_time, 0);
	else
		err = snd_pcm_hw_params_set_period_size_near(handle, params,
							     &period_frames, 0);
	assert(err >= 0);
	if (buffer_time > 0) {
		err = snd_pcm_hw_params_set_buffer_time_near(handle, params,
							     &buffer_time, 0);
	} else {
		err = snd_pcm_hw_params_set_buffer_size_near(handle, params,
							     &buffer_frames);
	}
	assert(err >= 0);
	monotonic = snd_pcm_hw_params_is_monotonic(params);
	can_pause = snd_pcm_hw_params_can_pause(params);
	err = snd_pcm_hw_params(handle, params);
	if (err < 0) {
		error("Unable to install hw params:");
		snd_pcm_hw_params_dump(params, log);
		prg_exit(EXIT_FAILURE);
	}
	snd_pcm_hw_params_get_period_size(params, &chunk_size, 0);
	snd_pcm_hw_params_get_buffer_size(params, &buffer_size);
	if (chunk_size == buffer_size) {
		error("Can't use period equal to buffer size (%lu == %lu)",
		      chunk_size, buffer_size);
		prg_exit(EXIT_FAILURE);
	}
	snd_pcm_sw_params_current(handle, swparams);
	if (avail_min < 0)
		n = chunk_size;
	else
		n = (double) rate * avail_min / 1000000;
	err = snd_pcm_sw_params_set_avail_min(handle, swparams, n);

	/* round up to closest transfer boundary */
	n = buffer_size;
	if (start_delay <= 0) {
		start_threshold = n + (double) rate * start_delay / 1000000;
	} else
		start_threshold = (double) rate * start_delay / 1000000;
	if (start_threshold < 1)
		start_threshold = 1;
	if (start_threshold > n)
		start_threshold = n;
	err = snd_pcm_sw_params_set_start_threshold(handle, swparams, start_threshold);
	assert(err >= 0);
	if (stop_delay <= 0) 
		stop_threshold = buffer_size + (double) rate * stop_delay / 1000000;
	else
		stop_threshold = (double) rate * stop_delay / 1000000;
	err = snd_pcm_sw_params_set_stop_threshold(handle, swparams, stop_threshold);
	assert(err >= 0);

	if (snd_pcm_sw_params(handle, swparams) < 0) {
		error("unable to install sw params:");
		snd_pcm_sw_params_dump(swparams, log);
		prg_exit(EXIT_FAILURE);
	}

	bits_per_sample = snd_pcm_format_physical_width(hwparams.format);
	bits_per_frame = bits_per_sample * hwparams.channels;
	chunk_bytes = chunk_size * bits_per_frame / 8;
	audiobuf = realloc(audiobuf, chunk_bytes);
	if (audiobuf == NULL) {
		error("not enough memory");
		prg_exit(EXIT_FAILURE);
	}

	/* stereo VU-meter isn't always available... */
	if (vumeter == VUMETER_STEREO) {
		if (hwparams.channels != 2 || !interleaved || verbose > 2)
			vumeter = VUMETER_MONO;
	}

	/* show mmap buffer arragment */
	if (mmap_flag && verbose) {
		const snd_pcm_channel_area_t *areas;
		snd_pcm_uframes_t offset, size = chunk_size;
		int i;
		err = snd_pcm_mmap_begin(handle, &areas, &offset, &size);
		if (err < 0) {
			error("snd_pcm_mmap_begin problem: %s", snd_strerror(err));
			prg_exit(EXIT_FAILURE);
		}
		for (i = 0; i < hwparams.channels; i++)
			fprintf(stderr, "mmap_area[%i] = %p,%u,%u (%u)\n", i, areas[i].addr, areas[i].first, areas[i].step, snd_pcm_format_physical_width(hwparams.format));
		/* not required, but for sure */
		snd_pcm_mmap_commit(handle, offset, 0);
	}

	buffer_frames = buffer_size;	/* for position test */
}

#ifndef timersub
#define	timersub(a, b, result) \
do { \
	(result)->tv_sec = (a)->tv_sec - (b)->tv_sec; \
	(result)->tv_usec = (a)->tv_usec - (b)->tv_usec; \
	if ((result)->tv_usec < 0) { \
		--(result)->tv_sec; \
		(result)->tv_usec += 1000000; \
	} \
} while (0)
#endif

/* I/O error handler */
static void xrun(void)
{
	snd_pcm_status_t *status;
	int res;
	
	snd_pcm_status_alloca(&status);
	if ((res = snd_pcm_status(handle, status))<0) {
		error("status error: %s", snd_strerror(res));
		prg_exit(EXIT_FAILURE);
	}
	if (snd_pcm_status_get_state(status) == SND_PCM_STATE_XRUN) {
		if (fatal_errors) {
			error("fatal %s: %s",
					stream == SND_PCM_STREAM_PLAYBACK ? "underrun" : "overrun",
					snd_strerror(res));
			prg_exit(EXIT_FAILURE);
		}
		if (monotonic) {
			fprintf(stderr, "%s !!!\n", "underrun");
		} else {
			struct timeval now, diff, tstamp;
			gettimeofday(&now, 0);
			snd_pcm_status_get_trigger_tstamp(status, &tstamp);
			timersub(&now, &tstamp, &diff);
			fprintf(stderr, "%s!!! (at least %.3f ms long)\n",
				stream == SND_PCM_STREAM_PLAYBACK ? "underrun" : "overrun",
				diff.tv_sec * 1000 + diff.tv_usec / 1000.0);
		}
		if (verbose) {
			fprintf(stderr, "Status:\n");
			snd_pcm_status_dump(status, log);
		}
		if ((res = snd_pcm_prepare(handle))<0) {
			error("xrun: prepare error: %s", snd_strerror(res));
			prg_exit(EXIT_FAILURE);
		}
		return;		/* ok, data should be accepted again */
	} if (snd_pcm_status_get_state(status) == SND_PCM_STATE_DRAINING) {
		if (verbose) {
			fprintf(stderr, "Status(DRAINING):\n");
			snd_pcm_status_dump(status, log);
		}
		if (stream == SND_PCM_STREAM_CAPTURE) {
			fprintf(stderr, "capture stream format change? attempting recover...\n");
			if ((res = snd_pcm_prepare(handle))<0) {
				error("xrun(DRAINING): prepare error: %s", snd_strerror(res));
				prg_exit(EXIT_FAILURE);
			}
			return;
		}
	}
	if (verbose) {
		fprintf(stderr, "Status(R/W):\n");
		snd_pcm_status_dump(status, log);
	}
	error("read/write error, state = %s", snd_pcm_state_name(snd_pcm_status_get_state(status)));
	prg_exit(EXIT_FAILURE);
}

/* I/O suspend handler */
static void suspend(void)
{
	int res;

	if (!quiet_mode)
		fprintf(stderr, "Suspended. Trying resume. "); fflush(stderr);
	while ((res = snd_pcm_resume(handle)) == -EAGAIN)
		sleep(1);	/* wait until suspend flag is released */
	if (res < 0) {
		if (!quiet_mode)
			fprintf(stderr, "Failed. Restarting stream. "); fflush(stderr);
		if ((res = snd_pcm_prepare(handle)) < 0) {
			error("suspend: prepare error: %s", snd_strerror(res));
			prg_exit(EXIT_FAILURE);
		}
	}
	if (!quiet_mode)
		fprintf(stderr, "Done.\n");
}

static void print_vu_meter_mono(int perc, int maxperc)
{
	const int bar_length = 50;
	char line[80];
	int val;

	for (val = 0; val <= perc * bar_length / 100 && val < bar_length; val++)
		line[val] = '#';
	for (; val <= maxperc * bar_length / 100 && val < bar_length; val++)
		line[val] = ' ';
	line[val] = '+';
	for (++val; val <= bar_length; val++)
		line[val] = ' ';
	if (maxperc > 99)
		sprintf(line + val, "| MAX");
	else
		sprintf(line + val, "| %02i%%", maxperc);
	fputs(line, stderr);
	if (perc > 100)
		fprintf(stderr, " !clip  ");
}

static void print_vu_meter_stereo(int *perc, int *maxperc)
{
	const int bar_length = 35;
	char line[80];
	int c;

	memset(line, ' ', sizeof(line) - 1);
	line[bar_length + 3] = '|';

	for (c = 0; c < 2; c++) {
		int p = perc[c] * bar_length / 100;
		char tmp[4];
		if (p > bar_length)
			p = bar_length;
		if (c)
			memset(line + bar_length + 6 + 1, '#', p);
		else
			memset(line + bar_length - p - 1, '#', p);
		p = maxperc[c] * bar_length / 100;
		if (p > bar_length)
			p = bar_length;
		if (c)
			line[bar_length + 6 + 1 + p] = '+';
		else
			line[bar_length - p - 1] = '+';
		if (maxperc[c] > 99)
			sprintf(tmp, "MAX");
		else
			sprintf(tmp, "%02d%%", maxperc[c]);
		if (c)
			memcpy(line + bar_length + 3 + 1, tmp, 3);
		else
			memcpy(line + bar_length, tmp, 3);
	}
	line[bar_length * 2 + 6 + 2] = 0;
	fputs(line, stderr);
}

static void print_vu_meter(signed int *perc, signed int *maxperc)
{
	if (vumeter == VUMETER_STEREO)
		print_vu_meter_stereo(perc, maxperc);
	else
		print_vu_meter_mono(*perc, *maxperc);
}

/* peak handler */
static void compute_max_peak(u_char *data, size_t count)
{	
	signed int val, max, perc[2], max_peak[2];
	static	int	run = 0;
	size_t ocount = count;
	int	format_little_endian = snd_pcm_format_little_endian(hwparams.format);	
	int ichans, c;

	if (vumeter == VUMETER_STEREO)
		ichans = 2;
	else
		ichans = 1;

	memset(max_peak, 0, sizeof(max_peak));
	switch (bits_per_sample) {
	case 8: {
		signed char *valp = (signed char *)data;
		signed char mask = snd_pcm_format_silence(hwparams.format);
		c = 0;
		while (count-- > 0) {
			val = *valp++ ^ mask;
			val = abs(val);
			if (max_peak[c] < val)
				max_peak[c] = val;
			if (vumeter == VUMETER_STEREO)
				c = !c;
		}
		break;
	}
	case 16: {
		signed short *valp = (signed short *)data;
		signed short mask = snd_pcm_format_silence_16(hwparams.format);
		signed short sval;

		count /= 2;
		c = 0;
		while (count-- > 0) {
			if (format_little_endian)
				sval = le16toh(*valp);
			else
				sval = be16toh(*valp);
			sval = abs(sval) ^ mask;
			if (max_peak[c] < sval)
				max_peak[c] = sval;
			valp++;
			if (vumeter == VUMETER_STEREO)
				c = !c;
		}
		break;
	}
	case 24: {
		unsigned char *valp = data;
		signed int mask = snd_pcm_format_silence_32(hwparams.format);

		count /= 3;
		c = 0;
		while (count-- > 0) {
			if (format_little_endian) {
				val = valp[0] | (valp[1]<<8) | (valp[2]<<16);
			} else {
				val = (valp[0]<<16) | (valp[1]<<8) | valp[2];
			}
			/* Correct signed bit in 32-bit value */
			if (val & (1<<(bits_per_sample-1))) {
				val |= 0xff<<24;	/* Negate upper bits too */
			}
			val = abs(val) ^ mask;
			if (max_peak[c] < val)
				max_peak[c] = val;
			valp += 3;
			if (vumeter == VUMETER_STEREO)
				c = !c;
		}
		break;
	}
	case 32: {
		signed int *valp = (signed int *)data;
		signed int mask = snd_pcm_format_silence_32(hwparams.format);

		count /= 4;
		c = 0;
		while (count-- > 0) {
			if (format_little_endian)
				val = le32toh(*valp);
			else
				val = be32toh(*valp);
			val = abs(val) ^ mask;
			if (max_peak[c] < val)
				max_peak[c] = val;
			valp++;
			if (vumeter == VUMETER_STEREO)
				c = !c;
		}
		break;
	}
	default:
		if (run == 0) {
			fprintf(stderr, "Unsupported bit size %d.\n", (int)bits_per_sample);
			run = 1;
		}
		return;
	}
	max = 1 << (bits_per_sample-1);
	if (max <= 0)
		max = 0x7fffffff;

	for (c = 0; c < ichans; c++) {
		if (bits_per_sample > 16)
			perc[c] = max_peak[c] / (max / 100);
		else
			perc[c] = max_peak[c] * 100 / max;
	}

	if (interleaved && verbose <= 2) {
		static int maxperc[2];
		static time_t t=0;
		const time_t tt=time(NULL);
		if(tt>t) {
			t=tt;
			maxperc[0] = 0;
			maxperc[1] = 0;
		}
		for (c = 0; c < ichans; c++)
			if (perc[c] > maxperc[c])
				maxperc[c] = perc[c];

		putc('\r', stderr);
		print_vu_meter(perc, maxperc);
		fflush(stderr);
	}
	else if(verbose==3) {
		fprintf(stderr, "Max peak (%li samples): 0x%08x ", (long)ocount, max_peak[0]);
		for (val = 0; val < 20; val++)
			if (val <= perc[0] / 5)
				putc('#', stderr);
			else
				putc(' ', stderr);
		fprintf(stderr, " %i%%\n", perc[0]);
		fflush(stderr);
	}
}

/*
 *  read function
 */
static ssize_t pcm_read(u_char *data, size_t rcount)
{
	ssize_t r;
	size_t result = 0;
	size_t count = rcount;

	if (count != chunk_size) 
	{
		count = chunk_size;
	}
	
	while (count > 0 && !in_aborting) 
	{
		r = snd_pcm_readi(handle, data, count);
		if (r == -EAGAIN || (r >= 0 && (size_t)r < count)) 
		{
			if (!test_nowait)
				snd_pcm_wait(handle, 100);
		} 
		else if (r == -EPIPE) 
		{
			xrun();
		} 
		else if (r == -ESTRPIPE)
		{
			suspend();
		} 
		else if (r < 0) 
		{
			error("read error: %s", snd_strerror(r));
			prg_exit(EXIT_FAILURE);
		}
		if (r > 0) 
		{
			if (vumeter)
				compute_max_peak(data, r * hwparams.channels);
			
			result += r;
			count -= r;
			data += r * bits_per_frame / 8;
		}
	}
	
	return result;
}

/* calculate the data count to read from/to dsp */
static off64_t calc_count(void)
{
	off64_t count;

	if (timelimit == 0) 
	{
		count = pbrec_count;
	} 
	else 
	{
		count = snd_pcm_format_size(hwparams.format, hwparams.rate * hwparams.channels);
		count *= (off64_t)timelimit;
	}
	
	return count < pbrec_count ? count : pbrec_count;
}

