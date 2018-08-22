#!/bin/sh

#LOG_FILE=/var/log/downloaderd.log
LOG_FILE=/dev/null

while [ 1 ]; do
	/usr/bin/downloaderd 1>$LOG_FILE 2>&1
	sleep 1
	echo "restart downloaderd ......."
done
