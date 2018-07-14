# Google Android makefile for curl and libcurl
#
# This file is an updated version of Dan Fandrich's Android.mk, meant to build
# curl for ToT android with the android build system.

LOCAL_PATH:= $(call my-dir)

# Curl needs a version string.
# As this will be compiled on multiple platforms, generate a version string from
# the build environment variables.

MY_PLATFORM_VERSION := 19
MY_TARGET_ARCH_VARIANT := arm
version_string := "Android $(MY_PLATFORM_VERSION) $(MY_TARGET_ARCH_VARIANT)"
$(warning "==================$(version_string)=========")

curl_CFLAGS := -Wpointer-arith -Wwrite-strings -Wunused -Winline \
	-Wnested-externs -Wmissing-declarations -Wmissing-prototypes -Wno-long-long \
	-Wfloat-equal -Wno-multichar -Wno-sign-compare -Wno-format-nonliteral \
	-Wendif-labels -Wstrict-prototypes -Wdeclaration-after-statement \
	-Wno-system-headers -DHAVE_CONFIG_H -DOS='$(version_string)' -Werror \
	-DBUILDING_LIBCURL

curl_includes := \
	$(LOCAL_PATH)/include/ \
	$(LOCAL_PATH)/lib

#########################
# Build the libcurl static library

include $(CLEAR_VARS)
LOCAL_ARM_MODE := arm
include $(LOCAL_PATH)/lib/Makefile.inc

LOCAL_SRC_FILES := $(addprefix lib/,$(CSOURCES))
LOCAL_C_INCLUDES := $(curl_includes)
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../openssl/include
LOCAL_CFLAGS := $(curl_CFLAGS)
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)/include

LOCAL_MODULE:= libcurl
LOCAL_MODULE_TAGS := optional
LOCAL_WHOLE_STATIC_LIBRARIES := libcrypto_static libssl_static

include $(BUILD_STATIC_LIBRARY)

#########################
