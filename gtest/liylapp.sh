#! /bin/bash

echo [==========] run tests
adb shell setprop libc.debug.malloc.env_enabled true
#android 6.0
#adb shell setprop libc.debug.malloc 1
adb shell export LIBC_DEBUG_MALLOC_ENABLE=1
#adb shell stop
adb shell setprop libc.debug.malloc.options \"backtrace=16 fill guard=128 leak_track free_track=16384\"
adb shell setprop wrap.com.liyl.player "LD_PRELOAD=/data/local/tmp/libc_liyl_malloc_debug.so"
#adb shell setprop libc.debug.malloc.program app_process
#adb shell start
