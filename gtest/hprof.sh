#########################################################################
# File Name: hprof.sh
# Author: liyl
# mail: liyunlong_88@126.com
# Created Time: 2017年07月23日 星期日 01时47分45秒
#########################################################################
#!/bin/bash
#
# Native heap dump not available. To enable, run these commands (requires root):
adb shell setprop libc.debug.malloc 1
adb shell stop
adb shell setprop libc.debug.malloc.program app_process
adb shell start
#adb root
#adb shell setenforce 0
#sleep 30
#export MYPID=`adb shell ps |busybox awk '/com.liyl.player/{print $2}'`
#echo "app PID = $MYPID"
#adb shell rm /sdcard/mem.trace
#adb shell am dumpheap -n $MYPID /sdcard/mem.trace 
#sleep 15 
#adb pull /sdcard/ee.trace ./
#java -version
#java -jar /home/memory_analyze.jar -lib symbols/lib/libplayer.so  ./mem.trace
