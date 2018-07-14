#########################################################################
# File Name: gdb_server.sh
# Author: liyl
# mail: liyunlong_88@126.com
# Created Time: 2017年06月10日 星期六 17时24分52秒
#########################################################################
APP_BIN_NAME=$1
MOBILE_DIR=/data/local/tmp
if [ -n $APP_BIN_NAME ] ;then
	APP_BIN_NAME=omx_test
	echo "defalt debug $MOBILE_DIR/$APP_BIN_NAME"
fi
SO_LIB_DIR=$2
if [ -n $SO_LIB_DIR ] ;then
	SO_LIB_DIR=/home/liyl/source/fatfrog/gtest/obj/local/armeabi-v7a
	echo "defalt so_lib_dir = $SO_LIB_DIR"
fi
echo $SO_LIB_DIR
MY_NDK_BIN_DIR=/home/zhixinlyl/android_env/android-ndk-r10e/toolchains/arm-linux-androideabi-4.9/prebuilt/linux-x86_64/bin
export MYPORT=8825
#adb shell LD_LIBRARY_PATH=/data/local/tmp  $MOBILE_DIR/gdbserver :$MYPORT $MOBILE_DIR/$APP_BIN_NAME --gtest_filter=UT_StrongPointer.test_0&
#adb shell LD_LIBRARY_PATH=/data/local/tmp  $MOBILE_DIR/gdbserver :$MYPORT $MOBILE_DIR/$APP_BIN_NAME --gtest_filter=SharedBufferTest.TestEditResize &
#adb shell LD_LIBRARY_PATH=/data/local/tmp  $MOBILE_DIR/gdbserver :$MYPORT $MOBILE_DIR/$APP_BIN_NAME --gtest_filter=AtomicTest.TestMultThreadAcquireOrg&
#adb shell LD_LIBRARY_PATH=/data/local/tmp  $MOBILE_DIR/gdbserver :$MYPORT $MOBILE_DIR/$APP_BIN_NAME --gtest_filter=AtomicTest.TestMultThreadAcquireAtomic&
echo "adb shell LD_LIBRARY_PATH=/data/local/tmp  $MOBILE_DIR/gdbserver :$MYPORT $MOBILE_DIR/$APP_BIN_NAME --gtest_filter=MallocDebugTest.* &"
adb shell LD_LIBRARY_PATH=/data/local/tmp  $MOBILE_DIR/gdbserver :$MYPORT $MOBILE_DIR/$APP_BIN_NAME --gtest_filter=HttpDataSourceTest.*&
sleep 5
echo $MY_NDK_BIN_DIR
$MY_NDK_BIN_DIR/arm-linux-androideabi-gdb  --write -l 360000 $SO_LIB_DIR/$APP_BIN_NAME  -x gdb_cmd.sh
