LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_ARM_MODE := arm
LOCAL_CFLAGS += -D_STDATOMIC_HAVE_ATOMIC  -Wno-multichar -Werror -Wall -Wno-unused-function
LOCAL_CXXFLAGS += -std=c++11
LOCAL_CXX_STL += libc++_static
LOCAL_CFLAGS += -g -fPIC
LOCAL_LDFLAGS += -fpic 


LOCAL_SRC_FILES := api/IOMX.cpp
LOCAL_SRC_FILES +=                    \
        FrameDropper.cpp              \
        OMX.cpp                       \
        OMXMaster.cpp                 \
        OMXNodeInstance.cpp           \
        OMXUtils.cpp                  \
        SimpleSoftOMXComponent.cpp    \
        SoftOMXComponent.cpp          \
        SoftOMXPlugin.cpp             \
        SoftVideoDecoderOMXComponent.cpp \



LOCAL_C_INCLUDES += \
                    $(LOCAL_PATH)                                   \
                    $(LOCAL_PATH)/../common/include                 \
                    $(LOCAL_PATH)/../foundation/system/core         \
                    $(LOCAL_PATH)/../foundation/ui                  \
                    $(LOCAL_PATH)/../foundation/system/android      \
                    $(LOCAL_PATH)/../foundation/media/include       \



LOCAL_SHARED_LIBRARIES := libfoundation

LOCAL_WHOLE_STATIC_LIBRARIES := libcommon

LOCAL_MODULE:= libomx
LOCAL_CLANG := true
LOCAL_SANITIZE := unsigned-integer-overflow signed-integer-overflow
LOCAL_LDFLAGS += -Wl,-Bsymbolic

include $(BUILD_SHARED_LIBRARY)

################################################################################

include $(call all-makefiles-under,$(LOCAL_PATH))
