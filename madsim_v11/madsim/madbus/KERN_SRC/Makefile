
DEBUG = y

DEPS = madbus.h madbusioctls.h maddefs.h madkonsts.h

madbus-objs := madbusmain.o  mbdevthread.o
#
obj-m		:= madbus.o
KERN_SRC	:= /lib/modules/$(shell uname -r)/build/
PWD			:= $(shell pwd)

modules:
	make -C $(KERN_SRC) M=$(PWD) modules

install:
	make -C $(KERN_SRC) M=$(PWD) modules_install
	depmod -a

clean:
	make -C $(KERN_SRC) M=$(PWD) clean