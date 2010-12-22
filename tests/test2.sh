#!/bin/bash
TEST=test_dbus_variant_call
OUT=/tmp/${TEST}.out
ERR=/tmp/${TEST}.err
DOUT=/tmp/${TEST}.dbus

#
# check if service name can be exported and program terminated
#

sys_js test_busname.js >${OUT} 2>&1 >${OUT}&
PID=$!
echo pid $PID

dbus-send --session --print-reply --type=method_call --dest=at.beko.Test /  at.beko.Test.quit variant:int32:1234 >${DOUT} 2>&1 >${DOUT}

cat ${DOUT}

kill $PID

ERRSTR=`grep -i Error ${OUT}`
if [ ! ".." == ".${ERRSTR}." ] ; then 
	echo "ERROR: $TEST : ${ERRSTR}"
	exit 1
fi

ERRSTR=`grep -i Error ${DOUT}`
if [ ! ".." == ".${ERRSTR}." ] ; then 
	echo "ERROR: $TEST : ${ERRSTR}"
	exit 1
fi

echo "PASS : $TEST"

## clean up
rm ${OUT} ${ERR}
exit 0
