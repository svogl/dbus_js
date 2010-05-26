
#
# let test fail if the passed string is not empty. emit log file if VERBOSE is set.
#
function fail_if_nempty() {
	str=$1
	#echo "CHECK: $str"
	if [ ! ".." == ".${str}." ] ; then 
		echo "ERROR: $TEST"
		if [ ! -z "$VERBOSE" ] ; then 
			echo "ERROR: LOG ----------- "
			cat ${OUT}
		fi
		exit 1
	fi
}

function fail_if_not_zero() {
	val=$1
	### optional: pass file to dump instead of OUT
	if [ $# == 2 ] ; then
		OUT=$2
	fi;
	#echo "CHECK: $str"
	if [ "0" != "${val}" ] ; then 
		echo "ERROR: $TEST"
		if [ ! -z "$VERBOSE" ] ; then 
			echo "ERROR: LOG ----------- "
			cat ${OUT}
		fi
		exit 1
	fi
}

function pass_if_nempty() {
	str=$1
	if [ ! ".." == ".${str}." ] ; then 
		echo "PASS : $TEST"
		rm ${OUT} ${ERR}
		exit 0
	fi
}


function fail_always() {
	fail_if_nempty "Failing unconditionally"
}

