LOCAL_PATH := $(call my-dir)


#liyl add
LOCAL_ARM_MODE := arm
libc_malloc_debug_src_files := \
    BacktraceData.cpp \
    Config.cpp \
    DebugData.cpp \
    debug_disable.cpp \
    FreeTrackData.cpp \
    GuardData.cpp \
    malloc_debug.cpp \
    TrackData.cpp \

# ==============================================================
# libc_malloc_debug_backtrace.a
# ==============================================================
# Used by libmemunreachable
include $(CLEAR_VARS)

LOCAL_MODULE := libc_malloc_debug_backtrace

#liyl add
LOCAL_ARM_MODE := arm

LOCAL_SRC_FILES := \
    backtrace.cpp \
    MapData.cpp \

LOCAL_CXX_STL := libc++_static

#LOCAL_STATIC_LIBRARIES += \
    libc_logging \

#LOCAL_C_INCLUDES += bionic/libc
LOCAL_EXPORT_C_INCLUDE_DIRS += $(LOCAL_PATH)

LOCAL_SANITIZE := never
LOCAL_NATIVE_COVERAGE := false

# -Wno-error=format-zero-length needed for gcc to compile.
LOCAL_CFLAGS := \
    -Wall \
    -Werror \
    -Wno-error=format-zero-length \

include $(BUILD_STATIC_LIBRARY)

# ==============================================================
# libc_malloc_debug.so
# ==============================================================
include $(CLEAR_VARS)

LOCAL_MODULE := libc_liyl_malloc_debug

#liyl add
LOCAL_ARM_MODE := arm

LOCAL_SRC_FILES := \
    $(libc_malloc_debug_src_files) \

LOCAL_CXX_STL := libc++_static

# Only need this for arm since libc++ uses its own unwind code that
# doesn't mix with the other default unwind code.
LOCAL_STATIC_LIBRARIES_arm := libunwind_llvm

LOCAL_STATIC_LIBRARIES += \
    libc_malloc_debug_backtrace \

LOCAL_LDFLAGS_32 := -Wl,--version-script,$(LOCAL_PATH)/exported32.map
LOCAL_LDFLAGS_64 := -Wl,--version-script,$(LOCAL_PATH)/exported64.map
LOCAL_ALLOW_UNDEFINED_SYMBOLS := true

LOCAL_SANITIZE := never
LOCAL_NATIVE_COVERAGE := false

# -Wno-error=format-zero-length needed for gcc to compile.
LOCAL_CFLAGS := \
    -Wall \
    -Werror \
    -fno-stack-protector \
    -Wno-error=format-zero-length \

include $(BUILD_SHARED_LIBRARY)

