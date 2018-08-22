
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE    := norco_utils
LOCAL_SRC_FILES := GpioJNI.cpp SerialPort.cpp SerialPort485.cpp
LOCAL_LDLIBS    := -llog -lm  
LOCAL_CFLAGS := -static
LOCAL_STATIC_LIBRARIES += libstdc++
LOCAL_SHARED_LIBRARIES := libcutils

LOCAL_C_INCLUDES := $(JNI_H_INCLUDE)
LOCAL_PRELINK_MODULE := true

include $(BUILD_SHARED_LIBRARY)
