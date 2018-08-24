#!/bin/bash
#########################################################################
# File Name: debug.sh
# Author: liyl
# mail: liyunlong_88@126.com
# Created Time: 2017年06月10日 星期六 10时51分03秒
#########################################################################
function pull_tombstone()
{
	echo "param is $#"
    if [ $# == 1 ] ;then
		echo "cpu vendor is $1"
        case $1 in
            letv) tombstonedir=/sdcard/logs/recentLogs/;
                adb pull $tombstonedir ./tombstonedir/;
                unzip tombstonedir/*.zip  -d tombstonedir/;
                gzip -d tombstonedir/log/SYSTEM_TOMBSTONE*.gz;
                rm tombstone_00;
                mv tombstonedir/log/SYSTEM_TOMBSTONE*.txt tombstone_00;
                rm -rf tombstonedir/
                ;;
            mtk|qcom) tombstonefile=/data/tombstones/tombstone_00
                ;;
            *)
                echo "Can't find tombstonefile for unknown cpu vendor: $1"
                tombstonefile=xxxxxxxx
                ;;
        esac

    else
        adb pull /data/tombstones/tombstone_00 ./
    fi

}

pull_tombstone $1
$NDK/ndk-stack -sym obj/local/armeabi-v7a/ -dump tombstone_00 |tee 2.log
#adb shell rm -rf /data/tombstones/tombstone_*
#adb shell rm -rf /sdcard/logs/recentLogs/
