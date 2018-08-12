
#########################

LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)


LOCAL_ARM_MODE := arm
LOCAL_CFLAGS += -D_STDATOMIC_HAVE_ATOMIC -Wno-multichar -Werror -Wall -Wno-unused-function
LOCAL_CXXFLAGS += -std=c++11
LOCAL_CXX_STL += libc++_static
LOCAL_CFLAGS += -g -fPIC
LOCAL_LDFLAGS += -fpic 


LOCAL_C_INCLUDES += $(LOCAL_PATH)/../api
LOCAL_SRC_FILES +=  ../api/IDataSource.cpp          \
                    ../api/DataSource.cpp

LOCAL_C_INCLUDES += \
                    $(LOCAL_PATH)/../../external/openssl/include    \
                    $(LOCAL_PATH)/../../external/curl/include       \
                    $(LOCAL_PATH)/../../common/include              \
                    $(LOCAL_PATH)/downloader




#downloader
LOCAL_SRC_FILES +=  \
                    downloader/ll_downloader.cpp\
                    downloader/ll_downloader_factory.cpp       \
                    downloader/ll_loadmanager.cpp              \
                    downloader/ll_http_downloader.cpp          \
                    downloader/ll_component.cpp                \
                    downloader/ll_http_component.cpp           \
                    downloader/ll_dummy_observer.cpp           \
                    downloader/dl_utils.cpp           


LOCAL_SRC_FILES +=  \
                    downloader/ll_callbackdispatcherthread.cpp 


#curl_wrapper
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../downloader/include    \


LOCAL_SRC_FILES +=  \
                    ./downloader/ll_curl_utils.cpp         \
                    ./downloader/ll_curl_wrapper.cpp

LOCAL_SRC_FILES +=  \
                    downloader/ll_curlmulti.cpp            \
                    downloader/ll_http_multi_component.cpp


LOCAL_SRC_FILES += HttpDataSource.cpp






# for logging
LOCAL_SHARED_LIBRARIES += libfoundation
LOCAL_STATIC_LIBRARIES += libcommon libcurl

#libssl_static libcrypto_static
LOCAL_LDLIBS    += -llog -lz

LOCAL_CLANG := true
LOCAL_SANITIZE := unsigned-integer-overflow signed-integer-overflow

LOCAL_LDFLAGS += -Wl,-Bsymbolic

LOCAL_MODULE := libhttp

include $(BUILD_SHARED_LIBRARY)
