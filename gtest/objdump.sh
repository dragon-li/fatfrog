#########################################################################
# File Name: objdump.sh
# Author: liyl
# mail: liyunlong_88@126.com
# Created Time: 2017年06月16日 星期五 11时53分28秒
#########################################################################
#!/bin/bash
$NDK/toolchains/arm-linux-androideabi-4.9/prebuilt/linux-x86_64/bin/arm-linux-androideabi-objdump  -S -d -C obj/local/armeabi-v7a/omx_test >omx_test.s
