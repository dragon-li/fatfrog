#########################################################################
# File Name: valgrind.sh
# Author: liyl
# mail: liyunlong_88@126.com
# Created Time: 2017年06月16日 星期五 11时08分24秒
#########################################################################
#!/bin/bash
adb push /home/zhixinlyl/android_env/valgrind_7.1/valgrind /data/local/
adb shell "VALGRIND_LIB=/data/local/valgrind/lib64 LD_LIBRARY_PATH=/data/local/tmp /data/local/valgrind/valgrind --trace-children=yes --tool=memcheck  --show-reachable=yes --leak-check=full --keep-stacktraces=alloc-then-free --track-origins=yes /data/local/tmp/omx_test --gtest_filter=MallocTest.TestMalloc"
