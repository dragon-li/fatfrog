#########################################################################
# File Name: debug.sh
# Author: liyl
# mail: liyunlong_88@126.com
# Created Time: 2017年06月10日 星期六 10时51分03秒
#########################################################################
#!/bin/bash
adb pull /data/tombstones/tombstone_00 ./
$NDK/ndk-stack -sym obj/local/armeabi-v7a/ -dump tombstone_00 |tee 2.log
adb shell rm -rf /data/tombstones/tombstone_*
