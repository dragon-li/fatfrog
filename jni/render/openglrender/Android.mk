LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)


LOCAL_ARM_MODE := arm

LOCAL_MODULE            := librender
LOCAL_MODULE_TAGS       := optional

LOCAL_SRC_FILES         += AndroidOpenglRender.cpp ../api/IRender.cpp

LOCAL_SHARED_LIBRARIES  += libfoundation
LOCAL_STATIC_LIBRARIES  += libcommon
LOCAL_LDLIBS    := -llog -landroid -lEGL -lGLESv2

LOCAL_SANITIZE := signed-integer-overflow

LOCAL_LDFLAGS := -Wl,-Bsymbolic

include $(BUILD_SHARED_LIBRARY)
