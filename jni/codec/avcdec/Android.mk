LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_ARM_MODE := arm

LOCAL_MODULE            := lib_soft_avcdec
LOCAL_MODULE_TAGS       := optional

LOCAL_STATIC_LIBRARIES  := libavcdec
LOCAL_SRC_FILES         := SoftAVCDec.cpp

LOCAL_C_INCLUDES := $(TOP)/external/libavc/decoder
LOCAL_C_INCLUDES += $(TOP)/external/libavc/common
LOCAL_C_INCLUDES += $(TOP)/omx
LOCAL_C_INCLUDES += $(TOP)/common/include
LOCAL_C_INCLUDES += $(TOP)/prebuild/openmax

LOCAL_SHARED_LIBRARIES  += libomx
LOCAL_SHARED_LIBRARIES  += libfoundation
LOCAL_LIBS  += -llog

LOCAL_CLANG := true
LOCAL_SANITIZE := signed-integer-overflow

LOCAL_LDFLAGS := -Wl,-Bsymbolic

include $(BUILD_SHARED_LIBRARY)
