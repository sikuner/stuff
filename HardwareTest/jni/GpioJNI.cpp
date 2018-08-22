
#include <stdio.h>
#include "com_norco_utils_GpioJNI.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "android/log.h"
static const char *TAG = "gpio_test";
#define LOGI(fmt, args...) __android_log_print(ANDROID_LOG_INFO,  TAG, fmt, ##args)
#define LOGD(fmt, args...) __android_log_print(ANDROID_LOG_DEBUG, TAG, fmt, ##args)
#define LOGE(fmt, args...) __android_log_print(ANDROID_LOG_ERROR, TAG, fmt, ##args)

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(arr) (int)(sizeof(arr)/sizeof((arr)[0]))
#endif
#define GPIO_MAX 36
#define GPIO_MIN 0

#define IMX_GPIO_NR(bank, nr)           (((bank) - 1) * 32 + (nr))

struct _num_gpio_ {
	int num;
	int gpio;
} gpio_list[] = {
{ 0,  	IMX_GPIO_NR(5, 18),}, //pin 1
{ 1,	IMX_GPIO_NR(5, 19),}, //pin 3
{ 2,	IMX_GPIO_NR(5, 20),}, //pin 4
{ 3,	IMX_GPIO_NR(5, 21),}, //pin 5
{ 4,	IMX_GPIO_NR(6, 11),}, //pin 6
                              //pin 7
{ 5,	IMX_GPIO_NR(6, 8), }, //pin 8
{ 6,	IMX_GPIO_NR(5, 22),}, //pin 9
{ 7,	IMX_GPIO_NR(6, 7), }, //pin 10
{ 8,	IMX_GPIO_NR(5, 23),}, //pin 11
{ 9,	IMX_GPIO_NR(6, 9), }, //pin 12
{ 10,	IMX_GPIO_NR(5, 24),}, //pin 13
{ 11,	IMX_GPIO_NR(6, 10),}, //pin 14
{ 12,	IMX_GPIO_NR(5, 25),}, //pin 15
{ 13,	IMX_GPIO_NR(2, 0), }, //pin 16
{ 14,	IMX_GPIO_NR(5, 26),}, //pin 17
{ 15,	IMX_GPIO_NR(2, 1), }, //pin 18
{ 16,	IMX_GPIO_NR(5, 27),}, //pin 19
{ 17,	IMX_GPIO_NR(2, 2), }, //pin 20
{ 18,	IMX_GPIO_NR(5, 28),}, //pin 21
{ 19,	IMX_GPIO_NR(2, 3), }, //pin 22
{ 20,	IMX_GPIO_NR(5, 29),}, //pin 23
{ 21,	IMX_GPIO_NR(2, 4), }, //pin 24
{ 22,	IMX_GPIO_NR(5, 30),}, //pin 25
{ 23,	IMX_GPIO_NR(2, 5), }, //pin 26
{ 24,	IMX_GPIO_NR(5, 31),}, //pin 27
{ 25,	IMX_GPIO_NR(2, 6), }, //pin 28
{ 26,	IMX_GPIO_NR(6, 0), }, //pin 29
{ 27,	IMX_GPIO_NR(2, 7), }, //pin 30
{ 28,	IMX_GPIO_NR(6, 1), }, //pin 31
{ 29,	IMX_GPIO_NR(1, 9), }, //pin 32
{ 30,	IMX_GPIO_NR(6, 2), }, //pin 33
{ 31,	IMX_GPIO_NR(1, 2), }, //pin 34
{ 32,	IMX_GPIO_NR(6, 3), }, //pin 35
{ 33,	IMX_GPIO_NR(1, 4), }, //pin 36
{ 34,	IMX_GPIO_NR(6, 4), }, //pin 37
{ 35,	IMX_GPIO_NR(1, 30),}, //pin 38
{ 36,	IMX_GPIO_NR(6, 5), }, //pin 39
};

static int get_gpio(int pin){
	int i=0;

	for( ;i<ARRAY_SIZE(gpio_list); i++){
		if(gpio_list[i].num == pin){
			return gpio_list[i].gpio;
		}
	}
	return 0;
}

static int export_gpio(int gpio, int mode){
	char buff[256] = {0};
	char* export_file;

	if(mode)
		export_file = "/sys/class/gpio/export";
	else
		export_file = "/sys/class/gpio/unexport";

	if(!access(export_file, W_OK|F_OK)){
		int fd = open(export_file, O_WRONLY);
		if(fd < 0){
			printf("%s: Unable to open %s %d-\n", __func__, export_file, fd);
		}else{
			memset(buff, 0, sizeof(buff));
			sprintf(buff, "%d", gpio);
			write(fd, buff, strlen(buff)+1);
			close(fd);
			return 0;
		}
	}else{
		printf("%s: Unable to access %s -\n", __func__, export_file);
	}
	return -1;
}


/*
 * Class:     com_norco_utils_GpioJNI
 * Method:    getSum
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_com_norco_utils_GpioJNI_getSum
  (JNIEnv *env, jclass clazz) {

	return 8;
}

/*
 * Class:     com_norco_utils_GpioJNI
 * Method:    getLevel
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL Java_com_norco_utils_GpioJNI_getLevel
  (JNIEnv *env, jclass clazz, jint pin) {
	char name[256];
	char buff[256] = {0};
	int gpio = get_gpio(pin); 
	int value = -1;
	int iRet;

	if(pin > GPIO_MAX  || pin < GPIO_MIN){
		return -1;
	}
	
	if(gpio <= 0){
		return -2;
	}

	printf("%s %d", __func__, pin);

	/*export_gpio(gpio, 1);

	memset(name, 0, sizeof(name));
	sprintf(name, "/sys/devices/virtual/gpio/gpio%d/direction", gpio);
	if(!access(name, R_OK|F_OK)){
		int fd = open(name, O_RDONLY);
		if(fd < 0){
			printf("%s: Unable to open %s %d-\n", __func__, name, fd);
			goto exit;
		}else{
			memset(buff, 0, sizeof(buff));
			iRet = read(fd, buff, sizeof(buff)-1);
			buff[iRet] = '\0';
			close(fd);
			if(strncmp(buff, "in", 2)){ // 不等于in则退出
				goto exit;
			}
		}
	}else{
		printf("%s: Unable to access %s -\n", __func__, name);
		goto exit;
	}
*/
	memset(name, 0, sizeof(name));
	sprintf(name, "/sys/devices/virtual/gpio/gpio%d/value", gpio);
	if(!access(name, R_OK|F_OK)){
		int fd = open(name, O_RDONLY);
		if(fd < 0){
			printf("%s: Unable to open %s %d-\n", __func__, name, fd);
			goto exit;
		}else{
			memset(buff, 0, sizeof(buff));
			iRet = read(fd, buff, sizeof(buff)-1);
			buff[iRet] = '\0';
			value = atoi(buff);
			close(fd);
			goto exit;
		}
	}else{
		printf("%s: Unable to access %s -\n", __func__, name);
		goto exit;
	}

exit:
	export_gpio(gpio, 0);

	return value;
}

/*
 * Class:     com_norco_utils_GpioJNI
 * Method:    setLevel
 * Signature: (II)I
 */
JNIEXPORT jint JNICALL Java_com_norco_utils_GpioJNI_setLevel
  (JNIEnv *env, jclass clazz, jint pin, jint level){
	char name[256];
	char buff[256] = {0};


	int gpio = get_gpio(pin); 
	int value = -1;
	int iRet;
	//printf("%s pin:%d  level:%d \n", pin, level);
	//LOGI("bingking get_gpio  %d  %d  ",pin,level);
	if(pin > GPIO_MAX  || pin < GPIO_MIN){
		return -1;
	}
	LOGI("bingking get_gpio  %d  %d %d ",pin,level,gpio);
	if(gpio <= 0){
		return -1;
	}

	printf("%s pin:%d  level:%d gpio %d \n", __func__, pin, level, gpio);

	/*export_gpio(gpio, 1);
	memset(name, 0, sizeof(name));
	sprintf(name, "/sys/devices/virtual/gpio/gpio%d/direction", gpio);
	
	if(!access(name, R_OK|F_OK)){
		int fd = open(name, O_WRONLY);
		if(fd < 0){
			printf("%s: Unable to open %s %d-\n", __func__, name, fd);
			goto exit;
		}else{
			//memset(buff, 0, sizeof(buff));
			
			iRet = write(fd, "out", strlen("out")+1);
			close(fd);
			
		}
	}else{
		printf("%s: Unable to access %s -\n", __func__, name);
		goto exit;
	}
    */
	memset(name, 0, sizeof(name));
	sprintf(name, "/sys/devices/virtual/gpio/gpio%d/value", gpio);
	
	if(!access(name, R_OK|W_OK|F_OK)){
		int fd = open(name, O_WRONLY);
		if(fd < 0){
			printf("%s: Unable to open %s %d-\n", __func__, name, fd);
			goto exit;
		}else{
			memset(buff, 0, sizeof(buff));
			sprintf(buff, "%d", level);
			iRet = write(fd, buff, strlen(buff)+1);
			close(fd);
			goto exit;
		}
	}else{
		printf("%s: Unable to access %s -\n", __func__, name);
		goto exit;
	}

	return 0;

exit:
	export_gpio(gpio, 0);
	
}

