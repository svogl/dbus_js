
#
# path to sys_js
#
export PATH=$PATH:/opt/smarthome/bin/

T=$(ls: test*)
all: tests


tests:
	echo "$(T)"
	for i in $(sh: ls test*.sh); do \
		test$i.sh ; \
	done
