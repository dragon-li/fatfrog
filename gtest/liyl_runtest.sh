#! /bin/bash

cd ../
ctags -R
cscope -Rbq

cd ./gtest/

rm -rf obj/
echo [==========] build tests

$NDK/ndk-build NDK_DEBUG=1 -C ../jni/
$NDK/ndk-build NDK_DEBUG=1 -C ./jni/ -B


testfile="libs/armeabi-v7a/omx_test"
if [ ! -f "$testfile" ] 
then
 echo [==========] libs/armeabi-v7a/omx_test not exist
 exit 1
fi
echo [==========] upload files
adb shell rm -rf /data/local/tmp/
adb shell mkdir -p  /data/local/tmp/
adb push ../obj/local/armeabi-v7a/libc_liyl_malloc_debug.so /data/local/tmp/
adb push ../obj/local/armeabi-v7a/libfoundation.so /data/local/tmp/
adb push ../obj/local/armeabi-v7a/lib_soft_avcdec.so /data/local/tmp/
adb push ../obj/local/armeabi-v7a/lib_soft_aacdec.so /data/local/tmp/
adb push ../obj/local/armeabi-v7a/libhttp.so /data/local/tmp/
adb push ../obj/local/armeabi-v7a/libomx.so /data/local/tmp/
adb push ../obj/local/armeabi-v7a/libnuplayer.so  /data/local/tmp/
adb push ./obj/local/armeabi-v7a/omx_test  /data/local/tmp/
adb push ./libs/armeabi-v7a/gdbserver /data/local/tmp/
adb push ./libs/armeabi-v7a/gdb.setup /data/local/tmp/
adb shell sync

echo [==========] run tests
adb shell setprop libc.debug.malloc.env_enabled true
adb shell setprop libc.debug.malloc 0 #android 6.0
adb shell export LIBC_DEBUG_MALLOC_ENABLE=1
adb shell setprop libc.debug.malloc.options \"backtrace fill guard=1024 leak_track free_track=16384\"
#adb shell setprop libc.debug.malloc.options \"backtrace fill leak_track\"
adb shell setprop libc.debug.malloc.program /data/local/tmp/omx_test 
#adb shell "LD_LIBRARY_PATH=/data/local/tmp /data/local/tmp/omx_test --gtest_filter=UT_StrongPointer.test_0"
#adb shell "LD_LIBRARY_PATH=/data/local/tmp /data/local/tmp/omx_test --gtest_filter=SharedBufferTest.TestAlloc"
#adb shell "LD_LIBRARY_PATH=/data/local/tmp /data/local/tmp/omx_test --gtest_filter=SharedBufferTest.TestEditResize"
#adb shell "LD_LIBRARY_PATH=/data/local/tmp /data/local/tmp/omx_test --gtest_filter=AtomicTest.TestMultThreadAcquireLock"
#adb shell "LD_LIBRARY_PATH=/data/local/tmp /data/local/tmp/omx_test --gtest_filter=AtomicTest.TestMultThreadAcquireAtomic"
#adb shell "LD_LIBRARY_PATH=/data/local/tmp /data/local/tmp/omx_test --gtest_filter=AtomicTest.TestMultThreadAcquireOrg"
#adb shell "LD_LIBRARY_PATH=/data/local/tmp /data/local/tmp/omx_test --gtest_filter=AtomicTest_ARM.TestMultThreadAcquireLock_ARM"
#adb shell "LD_LIBRARY_PATH=/data/local/tmp /data/local/tmp/omx_test --gtest_filter=AtomicTest_ARM.TestMultThreadAcquireAtomic_ARM"
#adb shell "LD_LIBRARY_PATH=/data/local/tmp /data/local/tmp/omx_test --gtest_filter=AtomicTest_ARM.TestMultThreadAcquireOrg_ARM"
#adb shell "LD_LIBRARY_PATH=/data/local/tmp /data/local/tmp/omx_test --gtest_filter=ThreadTest.TestGetTid"
#adb shell "LD_LIBRARY_PATH=/data/local/tmp /data/local/tmp/omx_test --gtest_filter=MallocTest.TestMalloc"
#adb shell "LD_LIBRARY_PATH=/data/local/tmp /data/local/tmp/omx_test --gtest_filter=MallocTest.TestMalloc1"
#adb shell "LD_LIBRARY_PATH=/data/local/tmp /data/local/tmp/omx_test --gtest_filter=MallocTest.TestMalloc2"
#adb shell "LD_LIBRARY_PATH=/data/local/tmp /data/local/tmp/omx_test --gtest_filter=MallocTest.TestMalloc3"
#adb shell "LD_LIBRARY_PATH=/data/local/tmp /data/local/tmp/omx_test --gtest_filter=MallocTest.TestMalloc4"
#adb shell "LD_LIBRARY_PATH=/data/local/tmp /data/local/tmp/omx_test --gtest_filter=MallocTest.TestMalloc5"
#adb shell "LD_LIBRARY_PATH=/data/local/tmp /data/local/tmp/omx_test --gtest_filter=MallocDebugConfigTest.*"
#adb shell "LD_LIBRARY_PATH=/data/local/tmp /data/local/tmp/omx_test --gtest_filter=MallocDebugTest.*"
#adb shell "LD_LIBRARY_PATH=/data/local/tmp /data/local/tmp/omx_test --gtest_filter=HttpDownloaderTest.*"
#adb shell "LD_LIBRARY_PATH=/data/local/tmp /data/local/tmp/omx_test --gtest_filter=HttpDataSourceTest.*"
#adb shell "LD_LIBRARY_PATH=/data/local/tmp /data/local/tmp/omx_test --gtest_filter=MediaCodecListTest.*"
adb shell "LD_LIBRARY_PATH=/data/local/tmp /data/local/tmp/omx_test --gtest_filter=MediaCodecTest.*"
