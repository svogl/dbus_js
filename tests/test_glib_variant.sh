#!/bin/bash
TEST=test_glib_variant
OUT=/tmp/${TEST}.out
DOUT=/tmp/${TEST}.dbus.out

source common.sh 

#
# check if timeout function executes. Should run for 10 iters and exit.
#

sys_js test_glib_variant.js >${OUT} 2>&1 &
PID=$!

sleep 1

dbus-send --session --print-reply --type=method_call --dest=at.beko.Test /dbusListeners at.beko.TestIF.play variant:string:"foo" >${DOUT} 2>&1
echo "RET " $? ${DOUT}
fail_if_not_zero $? ${DOUT}
dbus-send --session --print-reply --type=method_call --dest=at.beko.Test /dbusListeners at.beko.TestIF.pause variant:string:"bar" >${DOUT} 2>&1
echo "RET " $?
fail_if_not_zero $? ${DOUT}
dbus-send --session --print-reply --type=method_call --dest=at.beko.Test /dbusListeners at.beko.TestIF.quit variant:string:"baz" >${DOUT} 2>&1
echo "RET " $?
fail_if_not_zero $? ${DOUT}


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

