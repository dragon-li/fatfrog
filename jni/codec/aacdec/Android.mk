LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_ARM_MODE := arm

LOCAL_SRC_FILES := \
      SoftAAC2.cpp \
      DrcPresModeWrap.cpp

LOCAL_C_INCLUDES := \
      ../../foundation/system/core            \
      ../../foundation/media/include          \
	  ../../prebuild/openmax                  \
      ../../external/aac/libAACdec/include    \
      ../../external/aac/libPCMutils/include  \
      ../../external/aac/libFDK/include       \
      ../../external/aac/libMpegTPDec/include \
      ../../external/aac/libSBRdec/include    \
      ../../external/aac/libSYS/include

LOCAL_CFLAGS :=

LOCAL_CFLAGS += -Werror
LOCAL_CLANG := true
LOCAL_SANITIZE := signed-integer-overflow unsigned-integer-overflow

LOCAL_STATIC_LIBRARIES := libFraunhoferAAC

LOCAL_SHARED_LIBRARIES := libomx libfoundation

LOcAL_LDLIBS := -llog

LOCAL_MODULE := lib_soft_aacdec
LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)
