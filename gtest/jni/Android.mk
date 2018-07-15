LOCAL_PATH := $(call my-dir)

#USER_LIBC := 1
USER_MALLOC_DEBUG := 1
#ENABLE_DEBUG_MALLOC_TEST := !USER_LIBC
#ENABLE_DEBUG_MALLOC_TEST := 1

ifeq ($(USER_LIBC),1)
include $(CLEAR_VARS)
LOCAL_MODULE := libc_static_liyl_debug
LOCAL_SRC_FILES := ../../obj/local/armeabi-v7a/libc_static_liyl_debug.a
LOCAL_EXPORT_C_INCLUDES += $(LOCAL_PATH)/../../jni/libc_bionic
include $(PREBUILT_STATIC_LIBRARY)
endif

ifeq ($(USER_MALLOC_DEBUG),1)
include $(CLEAR_VARS)
LOCAL_MODULE := libc_liyl_malloc_debug
LOCAL_SRC_FILES := ../../obj/local/armeabi-v7a/libc_liyl_malloc_debug.so
include $(PREBUILT_SHARED_LIBRARY)
endif

include $(CLEAR_VARS)
LOCAL_MODULE := libfoundation
LOCAL_SRC_FILES := ../../obj/local/armeabi-v7a/libfoundation.so
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libhttp
LOCAL_SRC_FILES := ../../obj/local/armeabi-v7a/libhttp.so
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libomx
LOCAL_SRC_FILES := ../../obj/local/armeabi-v7a/libomx.so
include $(PREBUILT_SHARED_LIBRARY)


include $(CLEAR_VARS)
LOCAL_MODULE := libnuplayer
LOCAL_SRC_FILES := ../../obj/local/armeabi-v7a/libnuplayer.so
include $(PREBUILT_SHARED_LIBRARY)


################################################################

include $(CLEAR_VARS)
LOCAL_ARM_MODE := arm

MY_CPP_LIST := $(wildcard $(LOCAL_PATH)/*.cpp)
LOCAL_CXXFLAGS += -std=c++11

#foundation
LOCAL_C_INCLUDES += $(LOCAL_PATH)                        \
    $(LOCAL_PATH)/../../jni/foundation/system/core       \
    $(LOCAL_PATH)/../../jni/foundation/system/core/utils \
    $(LOCAL_PATH)/../../jni/foundation/system/android    \
    $(LOCAL_PATH)/../../jni

ifeq ($(USER_MALLOC_DEBUG),1)
LOCAL_SHARED_LIBRARIES += libc_liyl_malloc_debug
endif

ifeq ($(USER_LIBC),1)
#USER_LIBC=1 && ENABLE_DEBUG_MALLOC_TEST=0
LOCAL_WHOLE_STATIC_LIBRARIES += libc_static_liyl_debug
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../../jni/libc_bionic
else
LOCAL_CFLAGS += -DDEBUG=1 -DUSE_DL_MALLOC=1
LOCAL_SRC_FILES +=  ../../jni/libc_bionic/dlmalloc/mymalloc.c        \
                    ../../jni/libc_bionic/dlmalloc/dlmalloc.c

LOCAL_C_INCLUDES += $(LOCAL_PATH)/../../jni/libc_bionic/dlmalloc     \
                    $(LOCAL_PATH)/../../jni/libc_bionic/malloc_debug \
                    $(LOCAL_PATH)/../../jni/libc_bionic/private      \
                    $(LOCAL_PATH)/../../jni/libc_bionic

ifeq ($(ENABLE_DEBUG_MALLOC_TEST),1)
MY_CPP_LIST += $(wildcard $(LOCAL_PATH)/malloc_debug/*.cpp)
else
LOCAL_SRC_FILES +=  ../../jni/libc_bionic/libc_init_common.cpp     \
					../../jni/libc_bionic/libc_init_dynamic.cpp    \
					../../jni/libc_bionic/malloc_common.cpp
endif
endif


LOCAL_MODULE := omx_test
LOCAL_SRC_FILES += $(MY_CPP_LIST:$(LOCAL_PATH)/%=%)
LOCAL_STATIC_LIBRARIES += googletest_main \
                          cpufeatures

LOCAL_SHARED_LIBRARIES += libfoundation libhttp libnuplayer

LOCAL_LDLIBS += -llog

LOCAL_CFLAGS += -g -fPIE -D_STDATOMIC_HAVE_ATOMIC
LOCAL_LDFLAGS += -fpie

$(info ======$(LOCAL_SRC_FILES)================)
$(info ======$(LOCAL_C_INCLUDES)================)
$(info ======$(LOCAL_CFLAGS)================)

include $(BUILD_EXECUTABLE)
$(call import-module,third_party/googletest)
$(call import-module, android/cpufeatures)
