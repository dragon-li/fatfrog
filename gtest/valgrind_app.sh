#########################################################################
# File Name: valgrind_app.sh
# Author: liyl
# mail: liyunlong_88@126.com
# Created Time: 2017年07月11日 星期二 14时21分44秒
#########################################################################
#!/bin/bash
adb push vg_app.sh /data/local/
adb shell chmod a+x /data/local/vg_app.sh
adb shell setprop wrap.com.liyl.player \'logwrapper /data/local/vg_app.sh\'
#kill app
export MYPID=`adb shell ps |busybox awk '/com.liyl.player/{print $2}'`
echo "app PID = $MYPID"
adb shell kill -9 $MYPID
