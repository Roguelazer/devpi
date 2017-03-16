CONFIG_MODULE_FORCE_UNLOAD=y
# flags passed to gcc for compilation - -v:verbose, -H:show include files
#KBUILD_CFLAGS += -v
# for debugging make itself, use --debug=i in make command for targets

# debug build:
EXTRA_CFLAGS=-ggdb -O0

KVERSION = $(shell uname -r)

obj-m += devpi.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	rm -f test

.PHONY: test
test: devpi.ko
	if sudo lsmod | grep -q devpi ; then sudo rmmod devpi ; fi
	sudo insmod devpi.ko
	echo "decimal" | sudo tee /proc/sys/dev/pi/mode > /dev/null
	[ "$$(sudo head -c 20 /dev/pi)" = "31415926535897932384" ] && echo "Decimal OK" || echo "Decimal Failure"
	echo "pie" | sudo tee /proc/sys/dev/pi/mode > /dev/null
	[ "$$(sudo head -n 1 /dev/pi)" = "apple" ] && echo "Pie OK" || echo "Pie Failure"
