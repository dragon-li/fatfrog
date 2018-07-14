#########################################################################
# File Name: mybt.sh
# Author: liyl
# mail: liyunlong_88@126.com
# Created Time: 2017年06月11日 星期日 15时12分10秒
#########################################################################
#!/bin/bash
export MYPID=`adb shell ps |busybox awk '/omx_test/{print $2}'`
#export MYPID=`adb shell ps |busybox awk '/com.liyl.player/{print $2}'`
adb shell debuggerd -b $MYPID > 1.log
