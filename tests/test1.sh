#!/bin/bash

TEST=test1
OUT=/tmp/${TEST}.out
ERR=/tmp/${TEST}.err

#
# check if dbus lib can be loaded.
#
echo 'DSO.load("dbus")' | sys_js >${OUT} 2>${ERR}

echo ret = $?
ERRSTR=`grep Error ${ERR}`

if [ ! ".." == ".${ERRSTR}." ] ; then 
	echo "ERROR: $TEST : ${ERRSTR}"
	exit 1
fi

echo "PASS : $TEST"

## clean up
rm ${OUT} ${ERR}
exit 0
