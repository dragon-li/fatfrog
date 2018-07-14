#########################################################################
# File Name: gdb_cmd.sh
# Author: liyl
# mail: liyunlong_88@126.com
# Created Time: 2017年06月10日 星期六 17时38分10秒
#########################################################################
#!/bin/bash
shell adb forward tcp:8825 tcp:8825
target remote localhost:8825
#source ./libs/armeabi-v7a/gdb.setup

# default allow pending breakpoint,YES
set breakpoint pending on
b xxx.cpp:123
