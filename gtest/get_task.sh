#!/bin/bash
#########################################################################
# File Name: get_task.sh
# Author: liyl
# mail: liyunlong_88@126.com
# Created Time: 2017年07月24日 星期一 11时26分20秒
#########################################################################
export MYPID=`adb shell ps |busybox awk '/com.liyl.player/{print $2}'`
echo "app PID = $MYPID"
for i in `seq 100`;
do	
	echo "$i "

	adb shell ls -la /proc/$MYPID/task/ > $i.task
	cat $i.task | sort >  ${i}_tast.status
	while read line;
	do
		taskId=`echo $line | awk '{gsub("\r","");print $NF}'`
		echo "\r\ncat /proc/$MYPID/task/$taskId/status" >> ${i}_tast.status 
		adb shell cat /proc/$MYPID/task/$taskId/status >> ${i}_tast.status&
		wait
		#wait behind thread exit


	done < $i.task 
	rm $i.task
	sleep 60
done


