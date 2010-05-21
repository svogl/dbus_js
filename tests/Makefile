
#
# path to sys_js
#
export PATH=$PATH:/opt/smarthome/bin/

T=$(shell ls test*sh)

all: tests

tests: $T
	- for i in $T; do \
		echo "=========== $$i ==============" ; \
		./$$i ; \
	done

.PHONY: tests
