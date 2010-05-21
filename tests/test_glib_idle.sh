#!/bin/bash

TEST=test_glib_idle

OUT=/tmp/${TEST}.out
ERR=/tmp/${TEST}.err
DOUT=/tmp/${TEST}.dbus

. common.sh 

#
# check if idle function executes. Should run for 10 iters and exit.
#

sys_js test_glib_idlefunc.js >${OUT} 2>&1 &
PID=$!
#
# sleep for 2 seconds - the process must have finished!
#
sleep 1

if [ -d /proc/$PID ] ; then 
	echo "ERROR: $TEST : process was still alive!"
	kill $PID
	exit 1
fi

ERRSTR=`grep -i Error ${OUT}`
fail_if_nempty "$ERRSTR"

ERRSTR=`grep -i Success ${OUT}`
if [ ! ".." == ".${ERRSTR}." ] ; then 
	echo "PASS : $TEST"
	rm ${OUT} ${ERR}
	exit 0
fi

## clean up
echo "ERROR: $TEST : ${ERRSTR}"
exit 1

