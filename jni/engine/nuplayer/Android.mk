LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_ARM_MODE := arm

LOCAL_CFLAGS += -D_STDATOMIC_HAVE_ATOMIC -Wno-multichar -Werror -Wall -Wno-unused-function
LOCAL_CXXFLAGS += -std=c++11
LOCAL_CXX_STL += libc++_static
LOCAL_CFLAGS += -g -fPIC
LOCAL_LDFLAGS += -fpic 


LOCAL_MODULE            := libnuplayer
LOCAL_MODULE_TAGS       := optional

LOCAL_STATIC_LIBRARIES  :=

LOCAL_SRC_FILES         := ../api/IMediaCodecList.cpp

LOCAL_SRC_FILES         += ../common/avc_utils.cpp                \
                           ../common/ACodec.cpp                   \
                           ../common/MediaCodecInfo.cpp           \
                           ../common/MediaCodecList.cpp           \
                           ../common/CodecBase.cpp                \
                           ../common/SkipCutBuffer.cpp            \
                           ../common/MediaCodec.cpp


LOCAL_C_INCLUDES := $(TOP)/omx
LOCAL_C_INCLUDES += $(TOP)/common/include
LOCAL_C_INCLUDES += $(TOP)/prebuild/openmax

#engine/common
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../common

LOCAL_SHARED_LIBRARIES  += libomx
LOCAL_SHARED_LIBRARIES  += libfoundation
LOCAL_LIBS  += -llog

LOCAL_CLANG := true
LOCAL_SANITIZE := unsigned-integer-overflow signed-integer-overflow

LOCAL_LDFLAGS := -Wl,-Bsymbolic

include $(BUILD_SHARED_LIBRARY)
