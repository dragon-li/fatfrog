LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_ARM_MODE := arm
LOCAL_CXXFLAGS += -std=c++11
LOCAL_SRC_FILES +=  src/IInterface.cpp     \
                    src/MediaDefs.cpp

LOCAL_C_INCLUDES += $(LOCAL_PATH)/include
LOCAL_EXPORT_C_INCLUDE_DIRS += $(LOCAL_PATH)/include
LOCAL_MODULE := libcommon
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
include $(call all-makefiles-under,$(LOCAL_PATH))
