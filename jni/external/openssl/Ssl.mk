local_c_flags :=

local_c_includes := $(log_c_includes) .

local_additional_dependencies := $(LOCAL_PATH)/android-config.mk $(LOCAL_PATH)/Ssl.mk

include $(LOCAL_PATH)/Ssl-config.mk

#######################################
# target static library
include $(CLEAR_VARS)
include $(LOCAL_PATH)/android-config.mk

# If we're building an unbundled build, don't try to use clang since it's not
# in the NDK yet. This can be removed when a clang version that is fast enough
# in the NDK.
ifeq (,$(TARGET_BUILD_APPS))
LOCAL_CLANG := true
else
LOCAL_SDK_VERSION := 9
endif

LOCAL_SRC_FILES += $(target_src_files)
LOCAL_CFLAGS += $(target_c_flags) -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=0 \
	-Wno-unused-parameter -Wno-missing-field-initializers -Wno-sometimes-uninitialized
LOCAL_CFLAGS += $(V_COMPILE_CFLAGS)
LOCAL_LDFLAGS += $(V_COMPILE_LDFLAGS)
LOCAL_C_INCLUDES += $(target_c_includes)
LOCAL_SHARED_LIBRARIES = $(log_shared_libraries)
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE:= libssl_static
LOCAL_ADDITIONAL_DEPENDENCIES := $(local_additional_dependencies)
include $(BUILD_STATIC_LIBRARY)

