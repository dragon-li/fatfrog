LOCAL_PATH := $(call my-dir)

###############################################################

include $(CLEAR_VARS)
LOCAL_ARM_MODE := arm


LOCAL_CXXFLAGS += -std=c++11

LOCAL_CXXFLAGS += -DUSE_DL_MALLOC=1

LOCAL_SRC_FILES +=  libc_init_common.cpp     \
                    libc_init_dynamic.cpp    \
                    malloc_common.cpp

LOCAL_C_INCLUDES += $(LOCAL_PATH)

LOCAL_SRC_FILES +=  dlmalloc/mymalloc.c    \
                    dlmalloc/dlmalloc.c

LOCAL_C_INCLUDES += $(LOCAL_PATH)/dlmalloc \
                    $(LOCAL_PATH)/private 

LOCAL_EXPORT_C_INCLUDE_DIRS += $(LOCAL_PATH)

$(info ======$(LOCAL_SRC_FILES)================)
$(info ======$(LOCAL_C_INCLUDES)================)

LOCAL_MODULE := libc_static_liyl_debug 

LOCAL_CFLAGS := \
    -Wall \
    -Werror \
	-Wno-unused-function\
    -Wno-error=format-zero-length \

LOCAL_CFLAGS += -g
##clang
LOCAL_SANITIZE += unsigned-integer-overflow signed-integer-overflow

LOCAL_SANITIZE += never
LOCAL_NATIVE_COVERAGE += false

include $(BUILD_STATIC_LIBRARY)


################################################################

include $(CLEAR_VARS)
LOCAL_ARM_MODE := arm


LOCAL_CXXFLAGS += -std=c++11


LOCAL_STATIC_LIBRARIES += libc_static_liyl_debug


LOCAL_MODULE := libc_liyl_debug 

#LOCAL_LDLIBS    += -llog

LOCAL_CFLAGS += -g -fPIC
LOCAL_LDFLAGS += -fpic 

LOCAL_LDFLAGS_32 := -Wl,--version-script,$(LOCAL_PATH)/exported32.map
LOCAL_LDFLAGS_64 := -Wl,--version-script,$(LOCAL_PATH)/exported64.map

#clang
LOCAL_SANITIZE += unsigned-integer-overflow signed-integer-overflow

LOCAL_SANITIZE += never
LOCAL_NATIVE_COVERAGE += false
LOCAL_CXX_STL += libc++_static

include $(BUILD_SHARED_LIBRARY)
include $(call all-makefiles-under,$(LOCAL_PATH))
