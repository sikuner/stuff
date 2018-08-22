#!/bin/sh

while [ 1 ]; do
	name=`cat /factory/.s`
	topic="C_"$name
	echo "name="$name
	echo "topic="$topic
	/usr/bin/playerd -h comm.beeba.cn -t $topic -i $name -c -q 2 -u user -P password 1>/dev/null 2>&1
	sleep 6
done
