#!/bin/bash
TEST=test_cast_uint32
OUT=/tmp/${TEST}.out
DOUT=/tmp/${TEST}.dbus.out

source common.sh 

#
# check if timeout function executes. Should run for 10 iters and exit.
#

sys_js test_call_audio.js >${OUT} 2>&1 &
PID=$!

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
	rm ${OUT}
	exit 0
fi


## clean up
echo "ERROR: $TEST : ${ERRSTR}"
if [ ! -z "$VERBOSE" ] ; then 
	echo "ERROR: LOG ----------- "
	cat ${OUT}
fi
exit 1

