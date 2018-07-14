#########################################################################
# File Name: gdb_server.sh
# Author: liyl
# mail: liyunlong_88@126.com
# Created Time: 2017年06月10日 星期六 17时24分52秒
#########################################################################
export MYPID=`adb shell ps |busybox awk '/com.liyl.player/{print $2}'`
echo "app PID = $MYPID"
SO_LIB_DIR=/home/liyl/source/fatfrog/gtest/obj/local/armeabi-v7a
echo "defalt so_lib_dir = $SO_LIB_DIR"
echo $SO_LIB_DIR
MY_NDK_BIN_DIR=/home/liyl/android_env/android-ndk-r10e/toolchains/arm-linux-androideabi-4.9/prebuilt/linux-x86_64/bin
export MYPORT=8825
adb shell gdbserver :$MYPORT --attach $MYPID &

sleep 5
echo $MY_NDK_BIN_DIR
$MY_NDK_BIN_DIR/arm-linux-androideabi-gdb  --write -l 360000 -x gdb_app_cmd.sh 
