CONFIG_MODULE_FORCE_UNLOAD=y
# flags passed to gcc for compilation - -v:verbose, -H:show include files
# KBUILD_CFLAGS += -v
# for debugging make itself, use --debug=i in make command for targets

# debug build:
EXTRA_CFLAGS=-g -O0

KVERSION = $(shell uname -r)

obj-m += devpi.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
