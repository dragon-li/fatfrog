#########################################################################
# File Name: valgrind.sh
# Author: liyl
# mail: liyunlong_88@126.com
# Created Time: 2017年06月16日 星期五 11时08分24秒
#########################################################################
#!/bin/bash
export TMPDIR=/data/data/com.liyl.player
export VALGRIND_LIB=/data/local/valgrind/lib64
#VGPARAMS=' --error-limit=no --trace-children=yes --tool=memcheck  --show-reachable=yes --leak-check=full --keep-stacktraces=alloc-then-free --track-origins=yes'
VGPARAMS=' --error-limit=no --trace-children=yes --tool=memcheck  --show-reachable=yes --leak-check=full'
exec /data/local/valgrind/valgrind $VGPARAMS $*
