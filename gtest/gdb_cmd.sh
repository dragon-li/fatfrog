#########################################################################
# File Name: gdb_cmd.sh
# Author: liyl
# mail: liyunlong_88@126.com
# Created Time: 2017年06月10日 星期六 17时38分10秒
#########################################################################
#!/bin/bash
shell adb forward tcp:8825 tcp:8825
target remote localhost:8825
source ./libs/armeabi-v7a/gdb.setup

# default allow pending breakpoint,YES
set breakpoint pending on
#b StrongPointer_test.cpp:61
#b AtomicTest.cpp:98
#condition 1 gStruct.count != gStruct.count1
#b mallocTest.cpp:73
#condition 1 tmp != 0

#b libc_init_dynamic.cpp:65
#commands 2
#silent
#p debug_malloc_usable_size(pointer)
#c
#end
