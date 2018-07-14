#########################

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)


LOCAL_ARM_MODE := arm
LOCAL_CFLAGS += -g -Wno-multichar -Werror -Wall -Wno-unused-function
LOCAL_CFLAGS += -D_STDATOMIC_HAVE_ATOMIC -fvisibility=protected
LOCAL_CXXFLAGS += -std=c++11
#LOCAL_CONLYFLAGS += -std=gnu99

#clang
LOCAL_SANITIZE += unsigned-integer-overflow signed-integer-overflow

enable_android := yes


LOCAL_C_INCLUDES += \
                    $(LOCAL_PATH)                                  \
                    $(LOCAL_PATH)/system/core                      \
                    $(LOCAL_PATH)/system/core/sync/include         \
                    $(LOCAL_PATH)/system/android



LOCAL_SRC_FILES +=  system/core/native_handle.c \
                    system/core/sync/sync.c

LOCAL_SRC_FILES +=  system/core/logger_write.cpp      \
                    system/core/sched_policy.cpp


#foundation/libutils
LOCAL_SRC_FILES +=  system/core/utils/Timers.cpp             \
                    system/core/utils/Threads.cpp            \
                    system/core/utils/SharedBuffer.cpp       \
                    system/core/utils/RefBase.cpp            \
                    system/core/utils/JenkinsHash.cpp        \
                    system/core/utils/NativeHandle.cpp       \
                    system/core/utils/ColorUtils.cpp         \
                    system/core/utils/VectorImplUnSafe.cpp

#foundation/media
LOCAL_C_INCLUDES += $(LOCAL_PATH)/media/include

LOCAL_SRC_FILES +=  media/src/AString.cpp                    \
                    media/src/ALooper.cpp                    \
                    media/src/ALooperRoster.cpp              \
                    media/src/AHandler.cpp                   \
                    media/src/ABuffer.cpp                    \
                    media/src/AMessage.cpp                   \
                    media/src/hexdump.cpp                    \
                    media/src/ParsedMessage.cpp              \
                    media/src/ANetworkSession.cpp            \
                    media/src/MetaData.cpp                   \
                    media/src/ABitReader.cpp                 \
					media/src/AHierarchicalStateMachine.cpp



#foundation/ui
LOCAL_C_INCLUDES += $(LOCAL_PATH)/ui

LOCAL_SRC_FILES +=  ui/Fence.cpp              \





#$(info ${LOCAL_SRC_FILES})

#$(info ${LOCAL_C_INCLUDES})

# for logging
LOCAL_LDLIBS    += -llog -lz
LOCAL_STATIC_LIBRARIES +=
LOCAL_MODULE := libfoundation

include $(BUILD_SHARED_LIBRARY)

##########################
include $(call all-makefiles-under,$(LOCAL_PATH))
